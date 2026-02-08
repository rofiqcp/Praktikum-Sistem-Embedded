/* ============================================================
 * STM32_05_DMA_ADC_Multi_Channel
 * ============================================================
 * Deskripsi:
 *   Program ini mendemonstrasikan penggunaan ADC1 dalam mode
 *   Scan dengan DMA untuk membaca dua channel ADC secara
 *   simultan. DMA1 Channel 1 mengisi buffer interleaved
 *   [ch0, ch1, ch0, ch1, ...] secara circular.
 *
 * Hardware:
 *   - STM32F103C8T6 Blue Pill
 *   - Potensiometer 1: PA0 (ADC Channel 0)
 *   - Potensiometer 2: PA1 (ADC Channel 1)
 *   - LED indikator: PC13 (active low)
 *   - UART1: PA9 (TX), PA10 (RX) untuk debug serial
 *
 * Fitur Utama:
 *   - ADC1 Scan Mode (2 channel berurutan)
 *   - DMA Circular Mode untuk transfer otomatis
 *   - Konversi ADC → tegangan (mV)
 *   - Statistik: min, max, rata-rata per channel
 *   - Output tagged [DATA] untuk parsing Python
 *
 * Koneksi:
 *   PA0 → Potentiometer 1 (wiper) → GND/3.3V
 *   PA1 → Potentiometer 2 (wiper) → GND/3.3V
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
ADC_HandleTypeDef hadc1;            /* Handle untuk ADC1 */
DMA_HandleTypeDef hdma_adc1;        /* Handle untuk DMA1 Channel 1 (ADC1) */
UART_HandleTypeDef huart1;          /* Handle untuk USART1 (debug) */

/* ============================================================
 * Buffer DMA ADC
 * Buffer diisi secara interleaved oleh DMA:
 *   [ch0_sample0, ch1_sample0, ch0_sample1, ch1_sample1, ...]
 * Ukuran total = NUM_ADC_CHANNELS * NUM_SAMPLES
 * ============================================================ */
volatile uint16_t adc_dma_buffer[ADC_BUFFER_SIZE];

/* Flag untuk menandakan konversi DMA selesai */
volatile uint8_t dma_transfer_complete = 0;
volatile uint8_t dma_half_complete = 0;

/* Penghitung konversi untuk statistik */
volatile uint32_t conversion_count = 0;

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
static void Process_ADC_Data(volatile uint16_t *buffer, uint16_t num_samples);
void Error_Handler(void);

/* ============================================================
 * Retarget printf ke UART1
 * Fungsi _write() digunakan oleh syscalls untuk mengarahkan
 * output printf ke USART1
 * ============================================================ */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 * DWT (Data Watchpoint and Trace) Initialization
 * Digunakan untuk menghitung cycle CPU secara presisi
 * ============================================================ */
static void DWT_Init(void)
{
    /* Aktifkan DWT di register DEMCR */
    DWT_DEMCR_REG |= (1 << 24);    /* Set bit TRCENA */
    DWT_CYCCNT_REG = 0;             /* Reset counter */
    DWT_CTRL_REG |= 1;              /* Enable cycle counter */
}

/* Ambil nilai cycle counter saat ini */
static uint32_t DWT_GetCycles(void)
{
    return DWT_CYCCNT_REG;
}

/* ============================================================
 * DMA Callbacks
 * Callback ini dipanggil oleh HAL saat DMA selesai transfer
 * ============================================================ */

/* Callback saat DMA transfer selesai penuh */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        dma_transfer_complete = 1;
        conversion_count++;
        LED_TOGGLE();   /* Toggle LED sebagai indikator visual */
    }
}

/* Callback saat DMA transfer setengah selesai */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        dma_half_complete = 1;
    }
}

/* ============================================================
 * Proses Data ADC
 * Memisahkan data interleaved, hitung statistik per channel,
 * konversi ke tegangan (mV), dan cetak hasilnya
 * ============================================================ */
static void Process_ADC_Data(volatile uint16_t *buffer, uint16_t num_samples)
{
    /* Variabel statistik untuk Channel 0 */
    uint32_t sum_ch0 = 0;
    uint16_t min_ch0 = ADC_MAX_VALUE;
    uint16_t max_ch0 = 0;

    /* Variabel statistik untuk Channel 1 */
    uint32_t sum_ch1 = 0;
    uint16_t min_ch1 = ADC_MAX_VALUE;
    uint16_t max_ch1 = 0;

    /* Variabel waktu untuk pengukuran durasi pemrosesan */
    uint32_t start_cycles, end_cycles, process_cycles;
    start_cycles = DWT_GetCycles();

    /* --------------------------------------------------------
     * Iterasi buffer interleaved:
     * Index genap (0, 2, 4, ...) = Channel 0
     * Index ganjil (1, 3, 5, ...) = Channel 1
     * -------------------------------------------------------- */
    for (uint16_t i = 0; i < num_samples; i++) {
        uint16_t idx = i * NUM_ADC_CHANNELS;

        /* Ambil sampel Channel 0 */
        uint16_t val_ch0 = buffer[idx];
        sum_ch0 += val_ch0;
        if (val_ch0 < min_ch0) min_ch0 = val_ch0;
        if (val_ch0 > max_ch0) max_ch0 = val_ch0;

        /* Ambil sampel Channel 1 */
        uint16_t val_ch1 = buffer[idx + 1];
        sum_ch1 += val_ch1;
        if (val_ch1 < min_ch1) min_ch1 = val_ch1;
        if (val_ch1 > max_ch1) max_ch1 = val_ch1;
    }

    end_cycles = DWT_GetCycles();
    process_cycles = end_cycles - start_cycles;

    /* Hitung rata-rata */
    uint16_t avg_ch0 = (uint16_t)(sum_ch0 / num_samples);
    uint16_t avg_ch1 = (uint16_t)(sum_ch1 / num_samples);

    /* Konversi ke tegangan (mV) */
    uint32_t voltage_ch0 = (avg_ch0 * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t voltage_ch1 = (avg_ch1 * ADC_VREF_MV) / ADC_MAX_VALUE;

    uint32_t min_v_ch0 = (min_ch0 * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t max_v_ch0 = (max_ch0 * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t min_v_ch1 = (min_ch1 * ADC_VREF_MV) / ADC_MAX_VALUE;
    uint32_t max_v_ch1 = (max_ch1 * ADC_VREF_MV) / ADC_MAX_VALUE;

    /* --------------------------------------------------------
     * Cetak data dengan tag [DATA] untuk parsing Python
     * Format: [DATA] CH0: avg_mV min_mV max_mV raw | CH1: avg_mV min_mV max_mV raw
     * -------------------------------------------------------- */
    printf("[DATA] CH0_AVG=%lu CH0_MIN=%lu CH0_MAX=%lu CH0_RAW=%u "
           "CH1_AVG=%lu CH1_MIN=%lu CH1_MAX=%lu CH1_RAW=%u "
           "CYCLES=%lu CNT=%lu\r\n",
           voltage_ch0, min_v_ch0, max_v_ch0, avg_ch0,
           voltage_ch1, min_v_ch1, max_v_ch1, avg_ch1,
           process_cycles, conversion_count);

    /* Cetak beberapa sampel mentah untuk visualisasi */
    printf("[DATA] SAMPLES");
    for (uint16_t i = 0; i < 16 && i < num_samples; i++) {
        uint16_t idx = i * NUM_ADC_CHANNELS;
        printf(" %u:%u", buffer[idx], buffer[idx + 1]);
    }
    printf("\r\n");
}

/* ============================================================
 * MAIN - Fungsi Utama Program
 * ============================================================ */
int main(void)
{
    /* ----------------------------------------------------------
     * Inisialisasi HAL Library
     * HAL_Init() mengkonfigurasi:
     *   - Flash prefetch buffer
     *   - SysTick timer (1ms tick)
     *   - NVIC priority grouping
     * ---------------------------------------------------------- */
    HAL_Init();

    /* ----------------------------------------------------------
     * Konfigurasi System Clock
     * HSE 8MHz → PLL x9 → SYSCLK 72MHz
     * APB1 = 36MHz (max), APB2 = 72MHz
     * ---------------------------------------------------------- */
    SystemClock_Config();

    /* ----------------------------------------------------------
     * Inisialisasi periferal
     * Urutan penting: GPIO → DMA → ADC → UART
     * DMA harus diinisialisasi SEBELUM ADC karena ADC
     * membutuhkan DMA handle yang sudah valid
     * ---------------------------------------------------------- */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_USART1_Init();
    DWT_Init();

    /* LED menyala sebentar sebagai tanda sistem siap */
    LED_ON();
    HAL_Delay(200);
    LED_OFF();

    /* ----------------------------------------------------------
     * Cetak header informasi sistem
     * ---------------------------------------------------------- */
    printf("\r\n========================================\r\n");
    printf("STM32_05_DMA_ADC_Multi_Channel\r\n");
    printf("========================================\r\n");
    printf("Target   : STM32F103C8T6 Blue Pill\r\n");
    printf("SYSCLK   : %lu MHz\r\n", SYSCLK_FREQ_HZ / 1000000);
    printf("ADC      : 2 channel scan mode\r\n");
    printf("  CH0    : PA0 (ADC_CHANNEL_0)\r\n");
    printf("  CH1    : PA1 (ADC_CHANNEL_1)\r\n");
    printf("DMA      : DMA1 Channel 1 Circular\r\n");
    printf("Buffer   : %d samples x %d channels = %d total\r\n",
           NUM_SAMPLES, NUM_ADC_CHANNELS, ADC_BUFFER_SIZE);
    printf("UART     : %d baud (PA9/PA10)\r\n", DEBUG_UART_BAUD);
    printf("========================================\r\n\r\n");

    /* ----------------------------------------------------------
     * Kalibrasi ADC
     * STM32F103 memiliki fitur kalibrasi internal yang
     * meningkatkan akurasi pembacaan ADC
     * ---------------------------------------------------------- */
    printf("[INFO] Kalibrasi ADC...\r\n");
    HAL_ADCEx_Calibration_Start(&hadc1);
    printf("[INFO] Kalibrasi ADC selesai\r\n");

    /* ----------------------------------------------------------
     * Mulai konversi ADC dengan DMA
     * HAL_ADC_Start_DMA() akan:
     *   1. Mengaktifkan DMA channel
     *   2. Mengaktifkan ADC
     *   3. Memulai konversi kontinyu
     * Data akan otomatis ditransfer ke buffer oleh DMA
     * ---------------------------------------------------------- */
    printf("[INFO] Memulai ADC DMA Circular Mode...\r\n");
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(&hadc1,
                                                  (uint32_t *)adc_dma_buffer,
                                                  ADC_BUFFER_SIZE);
    if (status != HAL_OK) {
        printf("[ERROR] Gagal memulai ADC DMA! Status: %d\r\n", status);
        Error_Handler();
    }
    printf("[INFO] ADC DMA berjalan. Menunggu data...\r\n\r\n");

    /* Variabel untuk timing cetak statistik */
    uint32_t last_print_tick = HAL_GetTick();
    uint32_t total_transfers = 0;

    /* ============================================================
     * Loop Utama
     * Menunggu flag DMA, kemudian memproses data
     * ============================================================ */
    while (1)
    {
        /* ----------------------------------------------------------
         * Cek apakah DMA transfer setengah selesai
         * Saat setengah buffer terisi, kita bisa mulai memproses
         * setengah pertama sementara DMA mengisi setengah kedua
         * ---------------------------------------------------------- */
        if (dma_half_complete) {
            dma_half_complete = 0;
            /* Proses setengah pertama buffer */
            Process_ADC_Data(&adc_dma_buffer[0], NUM_SAMPLES / 2);
            total_transfers++;
        }

        /* ----------------------------------------------------------
         * Cek apakah DMA transfer selesai penuh
         * Saat seluruh buffer terisi, proses setengah kedua
         * ---------------------------------------------------------- */
        if (dma_transfer_complete) {
            dma_transfer_complete = 0;
            /* Proses setengah kedua buffer */
            Process_ADC_Data(&adc_dma_buffer[ADC_BUFFER_SIZE / 2],
                             NUM_SAMPLES / 2);
            total_transfers++;
        }

        /* ----------------------------------------------------------
         * Cetak statistik periodik
         * ---------------------------------------------------------- */
        uint32_t current_tick = HAL_GetTick();
        if (current_tick - last_print_tick >= STATS_PRINT_INTERVAL_MS) {
            last_print_tick = current_tick;

            /* Hitung throughput: transfer per detik */
            uint32_t samples_per_sec = total_transfers * NUM_SAMPLES;

            printf("[DATA] STATS TRANSFERS=%lu SAMPLES_PER_SEC=%lu "
                   "DMA_COUNT=%lu UPTIME=%lu\r\n",
                   total_transfers, samples_per_sec,
                   conversion_count, current_tick / 1000);

            total_transfers = 0;
        }
    }
}

/* ============================================================
 * SystemClock_Config
 * Konfigurasi clock sistem:
 *   - HSE (High Speed External) = 8 MHz crystal
 *   - PLL multiplier = x9
 *   - SYSCLK = 8 MHz × 9 = 72 MHz
 *   - AHB = 72 MHz (tidak dibagi)
 *   - APB1 = 36 MHz (dibagi 2, max 36 MHz)
 *   - APB2 = 72 MHz (tidak dibagi)
 *   - ADC prescaler = /6 → ADC clock = 12 MHz (max 14 MHz)
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* Konfigurasi HSE dan PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;    /* 8MHz x 9 = 72MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Konfigurasi bus clock */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;     /* AHB = 72MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;      /* APB1 = 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;      /* APB2 = 72MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

    /* Konfigurasi ADC clock prescaler */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;   /* 72/6 = 12MHz */
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * MX_GPIO_Init
 * Inisialisasi GPIO:
 *   - PC13: Output push-pull untuk LED (active low)
 *   - PA0, PA1: Analog input untuk ADC
 * ============================================================ */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock GPIO */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13: Output push-pull, low speed */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* Matikan LED (set HIGH karena active low) */
    LED_OFF();

    /* PA0 dan PA1 akan dikonfigurasi sebagai analog
       oleh MX_ADC1_Init() melalui HAL ADC MspInit */
}

/* ============================================================
 * MX_DMA_Init
 * Inisialisasi DMA1
 * Harus dipanggil SEBELUM inisialisasi ADC
 *
 * Pada STM32F103, mapping DMA channel bersifat tetap:
 *   DMA1 Channel 1 → ADC1
 *   DMA1 Channel 2 → SPI1_RX / USART3_TX
 *   DMA1 Channel 3 → SPI1_TX
 *   DMA1 Channel 4 → USART1_TX
 *   DMA1 Channel 5 → USART1_RX
 * ============================================================ */
static void MX_DMA_Init(void)
{
    /* Aktifkan clock DMA1 */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Konfigurasi NVIC untuk DMA1 Channel 1 */
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/* ============================================================
 * MX_ADC1_Init
 * Inisialisasi ADC1 dengan Scan Mode + DMA:
 *   - Mode: Scan (multiple channel)
 *   - Trigger: Software start
 *   - Continuous conversion: Enable
 *   - DMA: Enable (circular)
 *   - Channel 0 (PA0): Rank 1
 *   - Channel 1 (PA1): Rank 2
 * ============================================================ */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* Konfigurasi ADC1 */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;          /* Mode scan aktif */
    hadc1.Init.ContinuousConvMode = ENABLE;              /* Konversi terus-menerus */
    hadc1.Init.DiscontinuousConvMode = DISABLE;          /* Tidak diskontinyu */
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;    /* Trigger software */
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;          /* Data rata kanan */
    hadc1.Init.NbrOfConversion = NUM_ADC_CHANNELS;       /* 2 channel */
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    /* Konfigurasi Channel 0 (PA0) - Rank 1 (dikonversi pertama) */
    sConfig.Channel = ADC_CH0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;   /* Waktu sampling panjang */
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }

    /* Konfigurasi Channel 1 (PA1) - Rank 2 (dikonversi kedua) */
    sConfig.Channel = ADC_CH1;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * HAL_ADC_MspInit
 * Callback HAL untuk inisialisasi low-level ADC:
 *   - Clock ADC
 *   - GPIO analog
 *   - DMA channel dan linking
 * ============================================================ */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hadc->Instance == ADC1) {
        /* Aktifkan clock ADC1 */
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* Konfigurasi PA0 dan PA1 sebagai analog input */
        GPIO_InitStruct.Pin = ADC_CH0_PIN | ADC_CH1_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(ADC_GPIO_PORT, &GPIO_InitStruct);

        /* Konfigurasi DMA1 Channel 1 untuk ADC1 */
        hdma_adc1.Instance = ADC_DMA_CHANNEL;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;    /* ADC → Memory */
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;        /* Alamat periferal tetap */
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;            /* Alamat memori naik */
        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD; /* 16-bit */
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;   /* 16-bit */
        hdma_adc1.Init.Mode = DMA_CIRCULAR;                  /* Mode circular */
        hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;         /* Prioritas tinggi */
        if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
            Error_Handler();
        }

        /* Link DMA handle ke ADC handle */
        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);
    }
}

/* ============================================================
 * MX_USART1_Init
 * Inisialisasi USART1 untuk output debug:
 *   - Baud rate: 115200
 *   - 8 data bits, no parity, 1 stop bit
 *   - Mode: TX dan RX
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
 * Callback HAL untuk inisialisasi low-level UART:
 *   - Clock USART1
 *   - GPIO: PA9 (TX) AF push-pull, PA10 (RX) input floating
 * ============================================================ */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        /* Aktifkan clock USART1 dan GPIOA */
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA9 = USART1_TX: Alternate function push-pull */
        GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

        /* PA10 = USART1_RX: Input floating */
        GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);
    }
}

/* ============================================================
 * Interrupt Handlers
 * ============================================================ */

/* SysTick Handler - dipanggil setiap 1ms untuk HAL timing */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* DMA1 Channel 1 IRQ Handler - untuk transfer ADC1 */
void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

/* ============================================================
 * Error_Handler
 * Dipanggil saat terjadi error fatal
 * LED berkedip cepat sebagai indikator error
 * ============================================================ */
void Error_Handler(void)
{
    printf("[ERROR] Error_Handler dipanggil! Sistem berhenti.\r\n");
    __disable_irq();
    while (1) {
        LED_TOGGLE();
        /* Delay sederhana tanpa HAL (interrupt dimatikan) */
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
