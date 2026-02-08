/* ============================================================
 * STM32_08_DMA_Double_Buffer
 * ============================================================
 * Deskripsi:
 *   Program ini mengimplementasikan teknik SOFTWARE double 
 *   buffer pada STM32F103 menggunakan DMA half-complete dan
 *   full-complete interrupts untuk menciptakan efek ping-pong.
 *
 * CATATAN PENTING:
 *   STM32F103 TIDAK memiliki fitur hardware double-buffer
 *   (yang ada pada STM32F4/F7). Sebagai gantinya, kita
 *   menggunakan teknik software double-buffer:
 *     - Satu buffer besar dibagi menjadi dua "halaman"
 *     - DMA dalam mode circular mengisi buffer terus-menerus
 *     - Interrupt DMA Half-Complete → halaman pertama siap
 *     - Interrupt DMA Full-Complete → halaman kedua siap
 *     - Sementara DMA mengisi satu halaman, CPU memproses
 *       halaman yang lain → PING-PONG processing!
 *
 * Keunggulan Double Buffer:
 *   - Zero data loss: tidak perlu menghentikan DMA
 *   - CPU dan DMA bekerja paralel (overlap processing)
 *   - Throughput lebih tinggi dibanding single buffer
 *   - Latency lebih rendah (proses langsung saat data ready)
 *
 * Hardware:
 *   - STM32F103C8T6 Blue Pill
 *   - Potensiometer pada PA0 (ADC Channel 0)
 *   - LED indikator: PC13 (active low)
 *   - UART1: PA9 (TX), PA10 (RX) debug serial
 *
 * Alur Kerja:
 *   1. ADC1 dalam mode continuous, sample PA0
 *   2. DMA1 Channel 1 circular, buffer 512 samples
 *   3. Half-Complete IRQ: proses buffer[0..255]
 *   4. Full-Complete IRQ: proses buffer[256..511]
 *   5. Ulangi terus-menerus (ping-pong)
 *
 * Koneksi:
 *   PA0 → Potentiometer (wiper) → GND/3.3V
 *   PA9 → USB-Serial RX
 *   PA10 → USB-Serial TX
 * ============================================================ */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 * Deklarasi Handle Periferal
 * ============================================================ */
ADC_HandleTypeDef hadc1;            /* Handle ADC1 */
DMA_HandleTypeDef hdma_adc1;        /* Handle DMA1 Channel 1 */
UART_HandleTypeDef huart1;          /* Handle USART1 (debug) */

/* ============================================================
 * Double Buffer DMA
 * Buffer tunggal yang dibagi menjadi dua halaman (halves):
 *   Halaman A (First Half):  buffer[0 .. HALF_SIZE-1]
 *   Halaman B (Second Half): buffer[HALF_SIZE .. BUFFER_TOTAL-1]
 *
 * Diagram Ping-Pong:
 *   Waktu →
 *   DMA:  [===Fill A===][===Fill B===][===Fill A===][===Fill B===]
 *   CPU:  [            ][Process A   ][Process B   ][Process A   ]
 *   IRQ:       ^HalfCplt     ^FullCplt     ^HalfCplt     ^FullCplt
 * ============================================================ */
volatile uint16_t adc_dma_buffer[BUFFER_TOTAL];

/* ============================================================
 * Flag dan Kontrol Double Buffer
 * ============================================================ */
/* Flag untuk menandakan halaman mana yang siap diproses */
volatile uint8_t first_half_ready = 0;     /* Halaman A (0..HALF_SIZE-1) siap */
volatile uint8_t second_half_ready = 0;    /* Halaman B (HALF_SIZE..TOTAL-1) siap */

/* Identifier halaman yang sedang diproses (untuk tracking) */
volatile uint8_t current_processing_page = 0;  /* 0=A, 1=B */

/* Penghitung untuk statistik */
volatile uint32_t half_complete_count = 0;  /* Jumlah half-complete interrupts */
volatile uint32_t full_complete_count = 0;  /* Jumlah full-complete interrupts */
volatile uint32_t total_process_count = 0;  /* Total pemrosesan berhasil */
volatile uint32_t overrun_count = 0;        /* Data yang terlewat (overrun) */

/* Hasil pemrosesan per halaman */
typedef struct {
    uint16_t min_val;           /* Nilai ADC minimum */
    uint16_t max_val;           /* Nilai ADC maksimum */
    uint32_t avg_val;           /* Nilai ADC rata-rata */
    uint32_t voltage_mv;        /* Tegangan dalam mV */
    uint32_t process_cycles;    /* CPU cycles untuk memproses */
    uint8_t  page_id;           /* ID halaman (A=0, B=1) */
} PageResult;

/* ============================================================
 * Deklarasi Fungsi Prototipe
 * ============================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetCycles(void);
static PageResult Process_Buffer_Page(volatile uint16_t *page_start,
                                       uint16_t num_samples,
                                       uint8_t page_id);
static void Print_Page_Result(PageResult *result);
void Error_Handler(void);

/* ============================================================
 * Retarget printf ke UART1
 * ============================================================ */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 * DWT Cycle Counter
 * ============================================================ */
static void DWT_Init(void)
{
    DWT_DEMCR_REG |= (1 << 24);
    DWT_CYCCNT_REG = 0;
    DWT_CTRL_REG |= 1;
}

static uint32_t DWT_GetCycles(void)
{
    return DWT_CYCCNT_REG;
}

/* ============================================================
 * DMA Callbacks - Inti dari Double Buffer
 * ============================================================ */

/* ----------------------------------------------------------
 * HAL_ADC_ConvHalfCpltCallback
 * Dipanggil saat DMA telah mengisi SETENGAH buffer pertama
 * (index 0 sampai HALF_SIZE-1)
 *
 * Pada titik ini:
 *   - Halaman A (first half) berisi data valid
 *   - DMA sedang mengisi halaman B (second half)
 *   - CPU bisa memproses halaman A dengan aman
 * ---------------------------------------------------------- */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        half_complete_count++;

        /* Cek apakah halaman sebelumnya sudah diproses */
        if (first_half_ready) {
            overrun_count++;    /* Data lama belum diproses! */
        }

        first_half_ready = 1;   /* Tandai halaman A siap */
    }
}

/* ----------------------------------------------------------
 * HAL_ADC_ConvCpltCallback
 * Dipanggil saat DMA telah mengisi SELURUH buffer
 * (index HALF_SIZE sampai BUFFER_TOTAL-1)
 *
 * Pada titik ini:
 *   - Halaman B (second half) berisi data valid
 *   - DMA akan kembali ke awal (circular) mengisi halaman A
 *   - CPU bisa memproses halaman B dengan aman
 * ---------------------------------------------------------- */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        full_complete_count++;

        /* Cek apakah halaman sebelumnya sudah diproses */
        if (second_half_ready) {
            overrun_count++;    /* Data lama belum diproses! */
        }

        second_half_ready = 1;  /* Tandai halaman B siap */
        LED_TOGGLE();           /* Toggle LED setiap siklus penuh */
    }
}

/* ============================================================
 * Process_Buffer_Page
 * Memproses satu halaman buffer:
 *   - Hitung statistik (min, max, rata-rata)
 *   - Konversi ke tegangan (mV)
 *   - Ukur waktu pemrosesan dengan DWT
 *
 * Args:
 *   page_start  : Pointer ke awal halaman
 *   num_samples : Jumlah sampel dalam halaman
 *   page_id     : Identifier halaman (0=A, 1=B)
 *
 * Returns:
 *   PageResult dengan semua statistik
 * ============================================================ */
static PageResult Process_Buffer_Page(volatile uint16_t *page_start,
                                       uint16_t num_samples,
                                       uint8_t page_id)
{
    PageResult result;
    result.page_id = page_id;
    result.min_val = ADC_MAX_VALUE;
    result.max_val = 0;

    uint32_t start_cycles = DWT_GetCycles();
    uint32_t sum = 0;

    /* ----------------------------------------------------------
     * Iterasi semua sampel dalam halaman
     * Hitung min, max, dan sum untuk rata-rata
     * ---------------------------------------------------------- */
    for (uint16_t i = 0; i < num_samples; i++) {
        uint16_t val = page_start[i];
        sum += val;

        if (val < result.min_val) {
            result.min_val = val;
        }
        if (val > result.max_val) {
            result.max_val = val;
        }
    }

    /* Hitung rata-rata */
    result.avg_val = sum / num_samples;

    /* Konversi ke tegangan (mV) */
    result.voltage_mv = (result.avg_val * ADC_VREF_MV) / ADC_MAX_VALUE;

    uint32_t end_cycles = DWT_GetCycles();
    result.process_cycles = end_cycles - start_cycles;

    return result;
}

/* ============================================================
 * Print_Page_Result
 * Cetak hasil pemrosesan dengan format [DATA] untuk parsing
 * ============================================================ */
static void Print_Page_Result(PageResult *result)
{
    const char *page_name = (result->page_id == 0) ? "A" : "B";

    printf("[DATA] PAGE=%s AVG_MV=%lu MIN_RAW=%u MAX_RAW=%u "
           "AVG_RAW=%lu VOLTAGE=%lu CYCLES=%lu "
           "HALF_CNT=%lu FULL_CNT=%lu TOTAL=%lu OVERRUN=%lu\r\n",
           page_name,
           result->voltage_mv,
           result->min_val, result->max_val,
           result->avg_val,
           result->voltage_mv,
           result->process_cycles,
           half_complete_count, full_complete_count,
           total_process_count, overrun_count);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void)
{
    /* Inisialisasi HAL */
    HAL_Init();

    /* Konfigurasi System Clock: HSE 8MHz → PLL x9 → 72MHz */
    SystemClock_Config();

    /* Inisialisasi periferal (urutan penting!) */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_USART1_Init();
    DWT_Init();

    /* LED menyala sebentar */
    LED_ON();
    HAL_Delay(200);
    LED_OFF();

    /* Cetak header informasi */
    printf("\r\n========================================\r\n");
    printf("STM32_08_DMA_Double_Buffer\r\n");
    printf("========================================\r\n");
    printf("Target     : STM32F103C8T6 Blue Pill\r\n");
    printf("SYSCLK     : %lu MHz\r\n", SYSCLK_FREQ_HZ / 1000000);
    printf("ADC        : Channel 0 (PA0) continuous\r\n");
    printf("DMA        : DMA1 Channel 1 Circular\r\n");
    printf("Buffer     : %d total (%d per halaman)\r\n",
           BUFFER_TOTAL, HALF_SIZE);
    printf("Teknik     : Software Double Buffer\r\n");
    printf("UART       : %d baud (PA9/PA10)\r\n", DEBUG_UART_BAUD);
    printf("========================================\r\n");
    printf("\r\n[INFO] CATATAN: STM32F103 tidak memiliki hardware\r\n");
    printf("[INFO] double-buffer. Implementasi ini menggunakan\r\n");
    printf("[INFO] DMA Half/Full Complete interrupts untuk\r\n");
    printf("[INFO] menciptakan efek ping-pong secara software.\r\n\r\n");

    /* Diagram ping-pong */
    printf("[INFO] Diagram Ping-Pong:\r\n");
    printf("[INFO] DMA:  [===Fill A===][===Fill B===][===Fill A===]\r\n");
    printf("[INFO] CPU:  [            ][Process A   ][Process B   ]\r\n");
    printf("[INFO] IRQ:       ^Half        ^Full        ^Half\r\n\r\n");

    /* ----------------------------------------------------------
     * Kalibrasi ADC
     * ---------------------------------------------------------- */
    printf("[INFO] Kalibrasi ADC...\r\n");
    HAL_ADCEx_Calibration_Start(&hadc1);
    printf("[INFO] Kalibrasi selesai.\r\n");

    /* ----------------------------------------------------------
     * Mulai ADC + DMA Circular
     * Buffer total = BUFFER_TOTAL (512 samples)
     * DMA akan menghasilkan:
     *   - Half-Complete IRQ setelah 256 samples (halaman A penuh)
     *   - Full-Complete IRQ setelah 512 samples (halaman B penuh)
     *   - Kemudian kembali ke awal (circular) → ulangi
     * ---------------------------------------------------------- */
    printf("[INFO] Memulai ADC DMA Double Buffer...\r\n");
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(&hadc1,
                                                  (uint32_t *)adc_dma_buffer,
                                                  BUFFER_TOTAL);
    if (status != HAL_OK) {
        printf("[ERROR] Gagal memulai ADC DMA! Status: %d\r\n", status);
        Error_Handler();
    }
    printf("[INFO] Double buffer ping-pong aktif!\r\n\r\n");

    /* Variabel untuk loop */
    uint32_t last_stats_tick = HAL_GetTick();
    uint32_t process_a_count = 0;
    uint32_t process_b_count = 0;
    uint32_t total_cycles_a = 0;
    uint32_t total_cycles_b = 0;

    /* ============================================================
     * Loop Utama - Ping-Pong Processing
     * ============================================================ */
    while (1)
    {
        /* ----------------------------------------------------------
         * Cek apakah halaman A (first half) siap diproses
         *
         * Saat flag ini aktif:
         *   - buffer[0..HALF_SIZE-1] berisi data valid
         *   - DMA sedang mengisi buffer[HALF_SIZE..BUFFER_TOTAL-1]
         *   - Aman untuk memproses halaman A
         * ---------------------------------------------------------- */
        if (first_half_ready) {
            first_half_ready = 0;
            current_processing_page = 0;

            /* Proses halaman A */
            PageResult result = Process_Buffer_Page(
                &adc_dma_buffer[0],     /* Awal halaman A */
                HALF_SIZE,               /* 256 samples */
                0                        /* Page ID = A */
            );

            total_process_count++;
            process_a_count++;
            total_cycles_a += result.process_cycles;

            /* Cetak hasil dengan info halaman */
            Print_Page_Result(&result);

            /* Cetak beberapa sampel dari halaman A */
            printf("[DATA] SAMPLES_A");
            for (int i = 0; i < 8; i++) {
                printf(" %u", adc_dma_buffer[i]);
            }
            printf(" ... ");
            for (int i = HALF_SIZE - 4; i < HALF_SIZE; i++) {
                printf(" %u", adc_dma_buffer[i]);
            }
            printf("\r\n");
        }

        /* ----------------------------------------------------------
         * Cek apakah halaman B (second half) siap diproses
         *
         * Saat flag ini aktif:
         *   - buffer[HALF_SIZE..BUFFER_TOTAL-1] berisi data valid
         *   - DMA telah wrap around, mengisi buffer[0..] lagi
         *   - Aman untuk memproses halaman B
         * ---------------------------------------------------------- */
        if (second_half_ready) {
            second_half_ready = 0;
            current_processing_page = 1;

            /* Proses halaman B */
            PageResult result = Process_Buffer_Page(
                &adc_dma_buffer[HALF_SIZE],  /* Awal halaman B */
                HALF_SIZE,                    /* 256 samples */
                1                             /* Page ID = B */
            );

            total_process_count++;
            process_b_count++;
            total_cycles_b += result.process_cycles;

            /* Cetak hasil */
            Print_Page_Result(&result);

            /* Cetak beberapa sampel dari halaman B */
            printf("[DATA] SAMPLES_B");
            for (int i = HALF_SIZE; i < HALF_SIZE + 8; i++) {
                printf(" %u", adc_dma_buffer[i]);
            }
            printf(" ... ");
            for (int i = BUFFER_TOTAL - 4; i < BUFFER_TOTAL; i++) {
                printf(" %u", adc_dma_buffer[i]);
            }
            printf("\r\n");
        }

        /* ----------------------------------------------------------
         * Cetak statistik periodik
         * Setiap detik, cetak ringkasan performa double buffer
         * ---------------------------------------------------------- */
        uint32_t current_tick = HAL_GetTick();
        if (current_tick - last_stats_tick >= STATS_PRINT_INTERVAL_MS) {
            last_stats_tick = current_tick;

            /* Hitung rata-rata cycles per halaman */
            uint32_t avg_cycles_a = (process_a_count > 0) ?
                                    total_cycles_a / process_a_count : 0;
            uint32_t avg_cycles_b = (process_b_count > 0) ?
                                    total_cycles_b / process_b_count : 0;

            /* Throughput: samples per second */
            uint32_t samples_per_sec = total_process_count * HALF_SIZE;

            printf("[DATA] STATS TOTAL_PROC=%lu PAGE_A=%lu PAGE_B=%lu "
                   "OVERRUN=%lu AVG_CYC_A=%lu AVG_CYC_B=%lu "
                   "SPS=%lu UPTIME=%lu\r\n",
                   total_process_count, process_a_count, process_b_count,
                   overrun_count, avg_cycles_a, avg_cycles_b,
                   samples_per_sec, current_tick / 1000);

            /* Reset counter per interval */
            process_a_count = 0;
            process_b_count = 0;
            total_cycles_a = 0;
            total_cycles_b = 0;
        }
    }
}

/* ============================================================
 * SystemClock_Config
 * HSE 8MHz → PLL x9 → SYSCLK 72MHz
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* HSE + PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Bus clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

    /* ADC clock prescaler: 72MHz / 6 = 12MHz */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * MX_GPIO_Init
 * - PC13: LED output
 * - PA0: Analog input (dikonfigurasi di ADC MspInit)
 * ============================================================ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    LED_OFF();
}

/* ============================================================
 * MX_DMA_Init
 * ============================================================ */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* NVIC DMA1 Channel 1 (ADC1) - prioritas tinggi */
    HAL_NVIC_SetPriority(ADC_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_DMA_IRQn);
}

/* ============================================================
 * MX_ADC1_Init
 * ADC1 continuous mode, single channel (PA0)
 * ============================================================ */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;          /* Single channel */
    hadc1.Init.ContinuousConvMode = ENABLE;               /* Kontinyu */
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /* Konfigurasi Channel 0 (PA0) */
    sConfig.Channel = ADC_CHANNEL_USED;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;     /* Sampling cukup */
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * HAL_ADC_MspInit
 * Low-level init ADC + DMA channel
 * ============================================================ */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA0: Analog input */
        GPIO_InitStruct.Pin = ADC_GPIO_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(ADC_GPIO_PORT, &GPIO_InitStruct);

        /* DMA1 Channel 1 - ADC1 */
        hdma_adc1.Instance = ADC_DMA_CHANNEL;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_adc1.Init.Mode = DMA_CIRCULAR;              /* CIRCULAR untuk ping-pong */
        hdma_adc1.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);
    }
}

/* ============================================================
 * MX_USART1_Init
 * ============================================================ */
static void MX_USART1_Init(void)
{
    huart1.Instance = DEBUG_UART;
    huart1.Init.BaudRate = DEBUG_UART_BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * HAL_UART_MspInit
 * ============================================================ */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA9 TX */
        GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

        /* PA10 RX */
        GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);
    }
}

/* ============================================================
 * Interrupt Handlers
 * ============================================================ */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

/* ============================================================
 * Error_Handler
 * ============================================================ */
void Error_Handler(void)
{
    printf("[ERROR] Error_Handler dipanggil! Sistem berhenti.\r\n");
    __disable_irq();
    while (1) {
        LED_TOGGLE();
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
