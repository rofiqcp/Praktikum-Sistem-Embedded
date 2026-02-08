/**
 * ============================================================================
 * STM32_10_DMA_DAC_Waveform - Generasi Gelombang via Timer PWM + DMA
 * ============================================================================
 * Deskripsi : Menghasilkan gelombang sinus, segitiga, gergaji, dan kotak
 *             menggunakan Timer2 PWM + DMA pada STM32F103C8.
 *             
 *             STM32F103C8 (Blue Pill) TIDAK memiliki DAC hardware!
 *             Sebagai alternatif, kita menggunakan Timer PWM dimana
 *             DMA secara otomatis memperbarui nilai CCR1 (duty cycle)
 *             dari tabel gelombang di memori.
 *
 * Prinsip   : DMA1 Channel 2 terhubung ke TIM2 Update Event.
 *             Setiap kali timer overflow (update), DMA mengambil
 *             nilai berikutnya dari tabel gelombang dan menulis ke
 *             TIM2->CCR1 secara otomatis tanpa intervensi CPU.
 *
 * Output    : PA0 (TIM2_CH1) - sinyal PWM dengan duty cycle bervariasi
 *             Jika di-filter dengan RC low-pass, menghasilkan sinyal analog.
 *
 * Hardware  :
 *   - STM32F103C8 (Blue Pill) @ 72MHz
 *   - LED pada PC13 (indikator status)
 *   - USART1 PA9(TX)/PA10(RX) @ 115200 baud
 *   - PWM output: PA0 (TIM2_CH1)
 *   - Opsional: RC low-pass filter pada PA0 (R=1kΩ, C=10µF → fc≈16Hz)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ==================== Handle Periferal ==================== */
static UART_HandleTypeDef huart1;
static TIM_HandleTypeDef  htim2;
static DMA_HandleTypeDef  hdma_tim2_up;

/* ==================== Tabel Gelombang ==================== */
/**
 * Tabel gelombang menyimpan nilai duty cycle (0 - PWM_RESOLUTION-1).
 * DMA akan membaca nilai-nilai ini secara berurutan dan menulis ke CCR1.
 * Mode circular: otomatis kembali ke awal setelah sampai akhir tabel.
 */
static uint32_t waveform_sine[WAVEFORM_TABLE_SIZE];
static uint32_t waveform_triangle[WAVEFORM_TABLE_SIZE];
static uint32_t waveform_sawtooth[WAVEFORM_TABLE_SIZE];
static uint32_t waveform_square[WAVEFORM_TABLE_SIZE];

/* Pointer ke tabel gelombang aktif */
static uint32_t *active_waveform = NULL;
static uint8_t   current_waveform_type = WAVEFORM_SINE;

/* ==================== Variabel Status ==================== */
static volatile uint32_t dma_half_count = 0;     /* Counter half-transfer */
static volatile uint32_t dma_full_count = 0;     /* Counter full-transfer */
static volatile uint32_t dma_error_count = 0;    /* Counter error */
static uint32_t waveform_switch_tick = 0;        /* Waktu ganti gelombang */
static uint32_t test_number = 0;

/* ==================== Prototipe Fungsi ==================== */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void USART1_Init(void);
static void TIM2_PWM_Init(void);
static void DMA_Init(void);
static void Error_Handler(void);

/* Fungsi gelombang */
static void Generate_Waveform_Tables(void);
static void Start_Waveform_DMA(uint32_t *table);
static void Switch_Waveform(uint8_t type);
static const char* Get_Waveform_Name(uint8_t type);
static void Print_Waveform_Table(const char *name, uint32_t *table, uint16_t size);
static void Print_Separator(void);

/* ==================== Retarget printf ke USART1 ==================== */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== Interrupt Handlers ==================== */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/**
 * DMA1_Channel2_IRQHandler - Interrupt DMA untuk TIM2 Update
 * Dipanggil saat half-transfer atau transfer complete.
 */
void DMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_tim2_up);
}

/* ==================== Callback DMA ==================== */
/**
 * Callback saat DMA selesai transfer setengah tabel.
 * Berguna untuk teknik double-buffer (update separuh tabel
 * sementara separuh lainnya sedang di-output).
 */
void HAL_TIM_PeriodElapsedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        dma_half_count++;
    }
}

/**
 * Callback saat DMA selesai transfer seluruh tabel (1 siklus gelombang).
 * Dalam mode circular, DMA otomatis restart dari awal tabel.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        dma_full_count++;
        /* Toggle LED setiap siklus gelombang selesai */
        if (dma_full_count % 100 == 0)
        {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}

/* ==================== Konfigurasi Sistem Clock ==================== */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== Inisialisasi GPIO ==================== */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    LED_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== Inisialisasi USART1 ==================== */
static void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    USART_GPIO_CLK_ENABLE();
    USART_CLK_ENABLE();

    GPIO_InitStruct.Pin   = USART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(USART_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = USART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USART_RX_PORT, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = USART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== Inisialisasi DMA ==================== */
/**
 * DMA_Init - Konfigurasi DMA1 Channel 2 untuk TIM2 Update
 *
 * Setiap kali TIM2 overflow (update event), DMA membaca satu word
 * dari tabel gelombang dan menulisnya ke TIM2->CCR1.
 * Mode circular: otomatis kembali ke awal tabel.
 */
static void DMA_Init(void)
{
    DMA_CLK_ENABLE();

    hdma_tim2_up.Instance                 = PWM_DMA_CHANNEL;
    hdma_tim2_up.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tim2_up.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tim2_up.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tim2_up.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tim2_up.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_tim2_up.Init.Mode                = DMA_CIRCULAR;
    hdma_tim2_up.Init.Priority            = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(&hdma_tim2_up) != HAL_OK)
    {
        Error_Handler();
    }

    /* Link DMA ke TIM2 update channel */
    __HAL_LINKDMA(&htim2, hdma[TIM_DMA_ID_UPDATE], hdma_tim2_up);

    /* Aktifkan interrupt DMA */
    HAL_NVIC_SetPriority(PWM_DMA_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PWM_DMA_IRQn);
}

/* ==================== Inisialisasi Timer2 PWM ==================== */
/**
 * TIM2_PWM_Init - Konfigurasi TIM2 Channel 1 sebagai PWM output
 *
 * Timer clock: 72MHz / 72 = 1MHz
 * PWM period: 1MHz / 1000 = 1kHz
 * Duty cycle diatur oleh nilai CCR1 (0-999)
 * DMA mengubah CCR1 setiap update event → variasi duty cycle
 */
static void TIM2_PWM_Init(void)
{
    GPIO_InitTypeDef      GPIO_InitStruct = {0};
    TIM_OC_InitTypeDef    sConfigOC       = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    PWM_GPIO_CLK_ENABLE();
    PWM_TIM_CLK_ENABLE();

    /* PA0 = TIM2_CH1 - Alternate Function Push-Pull */
    GPIO_InitStruct.Pin   = PWM_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PWM_PORT, &GPIO_InitStruct);

    /* Konfigurasi TIM2 base */
    htim2.Instance               = PWM_TIMER;
    htim2.Init.Prescaler         = PWM_TIM_PRESCALER;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = PWM_TIM_PERIOD;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    /* Master output trigger → Update event */
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);

    /* Konfigurasi PWM Channel 1 */
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;  /* Duty awal 0% (akan diubah DMA) */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, PWM_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== Generasi Tabel Gelombang ==================== */
/**
 * Generate_Waveform_Tables - Buat semua tabel gelombang
 *
 * Nilai disimpan sebagai duty cycle (0 sampai PWM_RESOLUTION-1):
 *   - Sinus: sin(θ) dipetakan ke [0, PWM_RESOLUTION-1]
 *   - Segitiga: naik linear lalu turun
 *   - Gergaji: naik linear lalu reset
 *   - Kotak halus: transisi bertahap antara 0% dan 100%
 */
static void Generate_Waveform_Tables(void)
{
    for (uint16_t i = 0; i < WAVEFORM_TABLE_SIZE; i++)
    {
        float phase = (float)i / (float)WAVEFORM_TABLE_SIZE;

        /* --- Gelombang Sinus --- */
        /* sin(2π·phase) menghasilkan -1..+1, dipetakan ke 0..PWM_RESOLUTION-1 */
        float sin_val = sinf(2.0f * 3.14159265f * phase);
        waveform_sine[i] = (uint32_t)((sin_val + 1.0f) * 0.5f * (PWM_RESOLUTION - 1));

        /* --- Gelombang Segitiga --- */
        /* Naik di paruh pertama, turun di paruh kedua */
        if (phase < 0.5f)
        {
            waveform_triangle[i] = (uint32_t)(phase * 2.0f * (PWM_RESOLUTION - 1));
        }
        else
        {
            waveform_triangle[i] = (uint32_t)((1.0f - phase) * 2.0f * (PWM_RESOLUTION - 1));
        }

        /* --- Gelombang Gergaji (Sawtooth) --- */
        /* Naik linear dari 0 ke maksimum */
        waveform_sawtooth[i] = (uint32_t)(phase * (PWM_RESOLUTION - 1));

        /* --- Gelombang Kotak Halus (Soft Square) --- */
        /* Transisi sigmoid antara rendah dan tinggi */
        if (phase < 0.1f)
        {
            /* Transisi naik */
            waveform_square[i] = (uint32_t)((phase / 0.1f) * (PWM_RESOLUTION - 1));
        }
        else if (phase < 0.45f)
        {
            /* Tinggi penuh */
            waveform_square[i] = PWM_RESOLUTION - 1;
        }
        else if (phase < 0.55f)
        {
            /* Transisi turun */
            float t = (phase - 0.45f) / 0.1f;
            waveform_square[i] = (uint32_t)((1.0f - t) * (PWM_RESOLUTION - 1));
        }
        else if (phase < 0.9f)
        {
            /* Rendah penuh */
            waveform_square[i] = 0;
        }
        else
        {
            /* Transisi naik akhir */
            float t = (phase - 0.9f) / 0.1f;
            waveform_square[i] = (uint32_t)(t * (PWM_RESOLUTION - 1));
        }
    }
}

/* ==================== Kontrol Gelombang ==================== */
/**
 * Start_Waveform_DMA - Mulai output gelombang dengan DMA
 *
 * Fungsi ini memulai PWM dan DMA secara bersamaan.
 * DMA akan membaca dari tabel gelombang dan menulis ke CCR1
 * setiap kali timer update (overflow).
 *
 * @param table : pointer ke tabel gelombang yang akan di-output
 */
static void Start_Waveform_DMA(uint32_t *table)
{
    /* Hentikan DMA dan PWM yang sedang berjalan */
    HAL_TIM_PWM_Stop_DMA(&htim2, PWM_CHANNEL);

    /* Reset counter DMA */
    dma_half_count = 0;
    dma_full_count = 0;

    /* Mulai PWM dengan DMA - DMA membaca tabel dan menulis ke CCR1 */
    if (HAL_TIM_PWM_Start_DMA(&htim2, PWM_CHANNEL, table,
                               WAVEFORM_TABLE_SIZE) != HAL_OK)
    {
        printf("ERROR: Gagal memulai PWM DMA!\r\n");
        Error_Handler();
    }

    active_waveform = table;
}

/**
 * Switch_Waveform - Ganti tipe gelombang yang sedang di-output
 * @param type : tipe gelombang (WAVEFORM_SINE, dll)
 */
static void Switch_Waveform(uint8_t type)
{
    uint32_t *table = NULL;

    switch (type)
    {
        case WAVEFORM_SINE:
            table = waveform_sine;
            break;
        case WAVEFORM_TRIANGLE:
            table = waveform_triangle;
            break;
        case WAVEFORM_SAWTOOTH:
            table = waveform_sawtooth;
            break;
        case WAVEFORM_SQUARE_SOFT:
            table = waveform_square;
            break;
        default:
            table = waveform_sine;
            type  = WAVEFORM_SINE;
            break;
    }

    current_waveform_type = type;
    Start_Waveform_DMA(table);

    printf("\r\n>>> Gelombang aktif: %s <<<\r\n", Get_Waveform_Name(type));
}

/**
 * Get_Waveform_Name - Dapatkan nama gelombang dari tipe
 */
static const char* Get_Waveform_Name(uint8_t type)
{
    switch (type)
    {
        case WAVEFORM_SINE:        return "SINUS";
        case WAVEFORM_TRIANGLE:    return "SEGITIGA";
        case WAVEFORM_SAWTOOTH:    return "GERGAJI";
        case WAVEFORM_SQUARE_SOFT: return "KOTAK_HALUS";
        default:                   return "TIDAK_DIKENAL";
    }
}

/**
 * Print_Waveform_Table - Cetak isi tabel gelombang
 */
static void Print_Waveform_Table(const char *name, uint32_t *table, uint16_t size)
{
    printf("Tabel %s (%u sampel):\r\n  ", name, size);
    uint16_t print_count = (size > 16) ? 16 : size;
    for (uint16_t i = 0; i < print_count; i++)
    {
        printf("%lu ", table[i]);
        if ((i + 1) % 8 == 0) printf("\r\n  ");
    }
    if (size > 16) printf("... (total %u sampel)", size);
    printf("\r\n");
}

static void Print_Separator(void)
{
    printf("================================================\r\n");
}

/* ==================== Error Handler ==================== */
static void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/* ==================== Program Utama ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    USART1_Init();

    /* Banner program */
    printf("\r\n");
    Print_Separator();
    printf("  STM32_10_DMA_DAC_Waveform\r\n");
    printf("  Generasi Gelombang via Timer PWM + DMA\r\n");
    printf("  MCU: STM32F103C8 @ 72MHz\r\n");
    Print_Separator();
    printf("\r\n");

    printf("CATATAN: STM32F103C8 tidak memiliki DAC hardware!\r\n");
    printf("Menggunakan TIM2 PWM + DMA untuk variasi duty cycle.\r\n");
    printf("Output: PA0 (TIM2_CH1) - sambungkan RC filter untuk analog.\r\n\r\n");

    /* Inisialisasi periferal */
    DMA_Init();
    TIM2_PWM_Init();

    /* Buat tabel gelombang */
    printf("Menghasilkan tabel gelombang...\r\n");
    Generate_Waveform_Tables();
    printf("Selesai! %d sampel per gelombang.\r\n\r\n", WAVEFORM_TABLE_SIZE);

    /* Cetak informasi konfigurasi */
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Konfigurasi PWM + DMA\r\n", test_number);
    Print_Separator();
    printf("  Timer        : TIM2 Channel 1\r\n");
    printf("  Output pin   : PA0\r\n");
    printf("  Prescaler    : %d (timer clock = %lu Hz)\r\n",
           PWM_TIM_PRESCALER + 1, (uint32_t)(72000000UL / (PWM_TIM_PRESCALER + 1)));
    printf("  Period       : %d (PWM freq = %lu Hz)\r\n",
           PWM_TIM_PERIOD + 1, (uint32_t)(72000000UL / (PWM_TIM_PRESCALER + 1) / (PWM_TIM_PERIOD + 1)));
    printf("  Resolusi     : %d level duty cycle\r\n", PWM_RESOLUTION);
    printf("  DMA Channel  : DMA1 Channel 2 (TIM2_UP)\r\n");
    printf("  DMA Mode     : Circular (auto-repeat)\r\n");
    printf("  Sampel/siklus: %d\r\n", WAVEFORM_TABLE_SIZE);

    uint32_t wave_freq = 72000000UL / (PWM_TIM_PRESCALER + 1) / (PWM_TIM_PERIOD + 1) / WAVEFORM_TABLE_SIZE;
    printf("  Freq gelombang: ~%lu Hz\r\n", wave_freq);
    printf("[DATA] CONFIG,pwm_freq=%lu,wave_freq=%lu,samples=%d,resolution=%d\r\n",
           (uint32_t)(72000000UL / (PWM_TIM_PRESCALER + 1) / (PWM_TIM_PERIOD + 1)),
           wave_freq, WAVEFORM_TABLE_SIZE, PWM_RESOLUTION);
    printf("\r\n");

    /* Cetak sampel tabel gelombang */
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Tabel Gelombang\r\n", test_number);
    Print_Separator();

    Print_Waveform_Table("SINUS", waveform_sine, WAVEFORM_TABLE_SIZE);
    printf("\r\n");
    Print_Waveform_Table("SEGITIGA", waveform_triangle, WAVEFORM_TABLE_SIZE);
    printf("\r\n");
    Print_Waveform_Table("GERGAJI", waveform_sawtooth, WAVEFORM_TABLE_SIZE);
    printf("\r\n");
    Print_Waveform_Table("KOTAK_HALUS", waveform_square, WAVEFORM_TABLE_SIZE);
    printf("\r\n");

    /* Cetak data tabel untuk parsing Python */
    for (uint16_t i = 0; i < WAVEFORM_TABLE_SIZE; i++)
    {
        printf("[DATA] WAVE,idx=%u,sine=%lu,tri=%lu,saw=%lu,sqr=%lu\r\n",
               i, waveform_sine[i], waveform_triangle[i],
               waveform_sawtooth[i], waveform_square[i]);
    }

    HAL_Delay(DEMO_DELAY_MS);

    /* Mulai output gelombang sinus */
    test_number++;
    Print_Separator();
    printf("[TEST %lu] Mulai Output Gelombang\r\n", test_number);
    Print_Separator();

    Switch_Waveform(WAVEFORM_SINE);
    waveform_switch_tick = HAL_GetTick();

    printf("Gelombang akan berganti otomatis setiap %d detik.\r\n\r\n",
           WAVEFORM_SWITCH_MS / 1000);

    /* Loop utama - ganti gelombang secara periodik */
    uint32_t last_print = HAL_GetTick();
    uint32_t loop_count = 0;

    while (1)
    {
        /* Ganti tipe gelombang setiap WAVEFORM_SWITCH_MS */
        if (HAL_GetTick() - waveform_switch_tick >= WAVEFORM_SWITCH_MS)
        {
            waveform_switch_tick = HAL_GetTick();
            current_waveform_type = (current_waveform_type + 1) % NUM_WAVEFORM_TYPES;
            Switch_Waveform(current_waveform_type);
        }

        /* Cetak status periodik */
        if (HAL_GetTick() - last_print >= PRINT_INTERVAL_MS)
        {
            last_print = HAL_GetTick();
            loop_count++;

            /* Hitung statistik DMA */
            uint32_t cycles_completed = dma_full_count;
            uint32_t half_transfers   = dma_half_count;

            printf("[DATA] STATUS,wave=%s,cycles=%lu,half=%lu,errors=%lu,uptime=%lu\r\n",
                   Get_Waveform_Name(current_waveform_type),
                   cycles_completed, half_transfers,
                   dma_error_count, HAL_GetTick() / 1000);

            /* Cetak info detail setiap 10 laporan */
            if (loop_count % 10 == 0)
            {
                printf("  PWM aktif pada PA0, tipe: %s\r\n",
                       Get_Waveform_Name(current_waveform_type));
                printf("  Siklus DMA selesai: %lu\r\n", cycles_completed);
                printf("  Frekuensi output: ~%lu Hz\r\n", wave_freq);
            }
        }

        HAL_Delay(50);
    }
}
