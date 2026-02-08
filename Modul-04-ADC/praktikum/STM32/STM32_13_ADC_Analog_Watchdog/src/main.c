/**
 * ============================================================================
 * PROGRAM STM32_13: ADC Analog Watchdog (Fitur Khusus STM32)
 * ============================================================================
 *
 * FITUR KHUSUS STM32 YANG TIDAK ADA DI ESP32:
 * ============================================
 * STM32 memiliki HARDWARE Analog Watchdog (AWD) yang terintegrasi di dalam
 * peripheral ADC. AWD memonitor nilai ADC secara HARDWARE dan menghasilkan
 * interrupt INSTAN ketika nilai melewati batas atas (High Threshold) atau
 * batas bawah (Low Threshold).
 *
 * Pada ESP32, monitoring threshold ADC HARUS dilakukan dengan SOFTWARE:
 *   - Baca ADC secara periodik (polling)
 *   - Bandingkan dengan threshold di kode program
 *   - Latency tergantung pada frekuensi polling (ms-level)
 *
 * Keunggulan Hardware AWD STM32:
 *   1. ZERO CPU usage - monitoring dilakukan oleh hardware ADC
 *   2. Response time ~microseconds (hardware interrupt)
 *   3. Tidak terpengaruh oleh beban CPU (task lain tidak delay monitoring)
 *   4. Window mode: bisa set HIGH dan LOW threshold sekaligus
 *   5. Bisa monitoring satu channel atau semua channel
 *
 * KONFIGURASI HARDWARE:
 *   - PA0 : Input ADC (potentiometer / sensor analog)
 *   - PA2 : USART2 TX (output serial)
 *   - PA3 : USART2 RX
 *   - PC13: Built-in LED (indikator AWD alert, active LOW)
 *
 * TARGET BOARD:
 *   - BluePill  STM32F103C8 (ADC1 12-bit, AWD hardware)
 *   - BlackPill STM32F401CC (ADC1 12-bit, AWD hardware)
 *   - BlackPill STM32F411CE (ADC1 12-bit, AWD hardware)
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>

/* ---- Header HAL yang sesuai dengan chip ---- */
#if defined(STM32F103xB)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Board tidak didukung! Gunakan F103, F401, atau F411."
#endif

/* ===========================================================================
 * Konfigurasi Threshold Analog Watchdog
 * ===========================================================================
 * ADC 12-bit: range 0 - 4095 (0V - 3.3V)
 * High Threshold = 3000 (~2.42V) -> alert jika tegangan TINGGI
 * Low Threshold  = 1000 (~0.81V) -> alert jika tegangan RENDAH
 * Window aman    = 1000 - 3000
 * =========================================================================== */
#define AWD_HIGH_THRESHOLD   3000
#define AWD_LOW_THRESHOLD    1000
#define ADC_RESOLUTION       4095

/* ---- Handle Peripheral ---- */
static ADC_HandleTypeDef  hadc1;
static UART_HandleTypeDef huart2;
static TIM_HandleTypeDef  htim2;

/* ---- Variabel Global ---- */
static volatile uint32_t g_awd_count       = 0;   /* Jumlah AWD interrupt */
static volatile uint32_t g_awd_value       = 0;   /* Nilai ADC saat AWD trigger */
static volatile uint8_t  g_awd_flag        = 0;   /* Flag AWD baru terjadi */
static volatile uint32_t g_awd_timestamp   = 0;   /* Timestamp AWD (ms) */
static volatile uint32_t g_sw_poll_count   = 0;   /* Jumlah deteksi software */
static volatile uint32_t g_sw_poll_time_us = 0;   /* Waktu software polling */

/* ---- Deklarasi Fungsi ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART2_Init(void);
static void ADC1_Init(void);
static void ADC1_AWD_Config(void);
static void TIM2_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetMicros(void);
static void Demo_HardwareAWD(void);
static void Demo_SoftwarePolling(void);
static void Print_Header(void);
static void Print_Comparison(void);

/* ===========================================================================
 * Redirect printf ke USART2
 * =========================================================================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===========================================================================
 * Handler Interrupt Sistem
 * =========================================================================== */
void SysTick_Handler(void) {
    HAL_IncTick();
}

void NMI_Handler(void) { }
void HardFault_Handler(void) { while (1); }

/* Handler interrupt ADC - berbeda untuk F1 dan F4 */
#if defined(STM32F103xB)
void ADC1_2_IRQHandler(void) {
    HAL_ADC_IRQHandler(&hadc1);
}
#else
void ADC_IRQHandler(void) {
    HAL_ADC_IRQHandler(&hadc1);
}
#endif

/* ===========================================================================
 * CALLBACK ANALOG WATCHDOG (Inti dari fitur STM32-specific!)
 * ===========================================================================
 * Fungsi ini dipanggil secara OTOMATIS oleh hardware ketika nilai ADC
 * keluar dari jendela threshold [LOW_THRESHOLD .. HIGH_THRESHOLD].
 *
 * Pada ESP32, TIDAK ADA callback seperti ini - harus polling manual!
 * =========================================================================== */
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        g_awd_count++;
        g_awd_value     = HAL_ADC_GetValue(hadc);
        g_awd_flag       = 1;
        g_awd_timestamp  = HAL_GetTick();

        /* Toggle LED PC13 sebagai indikator visual */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

/* ===========================================================================
 * Konfigurasi System Clock
 * =========================================================================== */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef       RCC_OscInit = {0};
    RCC_ClkInitTypeDef       RCC_ClkInit = {0};

#if defined(STM32F103xB)
    /* ---- F103: HSE 8MHz -> PLL -> 72MHz ---- */
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2);

#elif defined(STM32F401xC)
    /* ---- F401: HSE 25MHz -> PLL -> 84MHz ---- */
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM       = 25;
    RCC_OscInit.PLL.PLLN       = 336;
    RCC_OscInit.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInit.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2);

#elif defined(STM32F411xE)
    /* ---- F411: HSE 25MHz -> PLL -> 100MHz ---- */
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM       = 25;
    RCC_OscInit.PLL.PLLN       = 200;
    RCC_OscInit.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInit.PLL.PLLQ       = 4;
    HAL_RCC_OscConfig(&RCC_OscInit);

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_3);
#endif
}

/* ===========================================================================
 * Inisialisasi GPIO (LED PC13)
 * =========================================================================== */
static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13 - Active LOW pada BluePill dan BlackPill */
    GPIO_Init_s.Pin   = GPIO_PIN_13;
    GPIO_Init_s.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_LOW;
#if defined(STM32F103xB)
    /* F103 tidak punya Pull field terpisah di output mode */
#else
    GPIO_Init_s.Pull  = GPIO_NOPULL;
#endif
    HAL_GPIO_Init(GPIOC, &GPIO_Init_s);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED mati */
}

/* ===========================================================================
 * Inisialisasi USART2 (PA2=TX, PA3=RX) untuk output serial
 * =========================================================================== */
static void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

#if defined(STM32F103xB)
    /* F103: PA2 AF Push-Pull (TX), PA3 Input Floating (RX) */
    GPIO_Init_s.Pin   = GPIO_PIN_2;
    GPIO_Init_s.Mode  = GPIO_MODE_AF_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);

    GPIO_Init_s.Pin   = GPIO_PIN_3;
    GPIO_Init_s.Mode  = GPIO_MODE_INPUT;
    GPIO_Init_s.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#else
    /* F401/F411: PA2, PA3 sebagai AF7 (USART2) */
    GPIO_Init_s.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_Init_s.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init_s.Pull      = GPIO_PULLUP;
    GPIO_Init_s.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init_s.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#endif

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
#if !defined(STM32F103xB)
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
#endif
    HAL_UART_Init(&huart2);
}

/* ===========================================================================
 * Inisialisasi ADC1 pada PA0 (Channel 0)
 * Mode: Continuous conversion dengan interrupt
 * =========================================================================== */
static void ADC1_Init(void) {
    GPIO_InitTypeDef         GPIO_Init_s = {0};
    ADC_ChannelConfTypeDef   sConfig     = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

#if defined(STM32F103xB)
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* PA0: Analog Input */
    GPIO_Init_s.Pin  = GPIO_PIN_0;
    GPIO_Init_s.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);

    /* Konfigurasi prescaler ADC (PCLK2/6 = 12MHz, max 14MHz) */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    /* Konfigurasi ADC1 */
    hadc1.Instance                   = ADC1;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    HAL_ADC_Init(&hadc1);

    /* Channel 0 (PA0) */
    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /* Kalibrasi ADC (khusus F103) */
    HAL_ADCEx_Calibration_Start(&hadc1);

#else
    /* ---- F401 / F411 ---- */
    __HAL_RCC_ADC1_CLK_ENABLE();

    GPIO_Init_s.Pin  = GPIO_PIN_0;
    GPIO_Init_s.Mode = GPIO_MODE_ANALOG;
    GPIO_Init_s.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = ENABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    sConfig.Offset       = 0;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
#endif
}

/* ===========================================================================
 * Konfigurasi Analog Watchdog (AWD) - INTI FITUR STM32!
 * ===========================================================================
 * AWD dikonfigurasi dalam mode WINDOW:
 *   - High Threshold: jika ADC > high_thresh -> interrupt
 *   - Low Threshold : jika ADC < low_thresh  -> interrupt
 *   - Window aman   : low_thresh <= ADC <= high_thresh -> tidak ada interrupt
 *
 * Ini dilakukan SEPENUHNYA oleh hardware ADC, TANPA campur tangan CPU!
 * =========================================================================== */
static void ADC1_AWD_Config(void) {
    ADC_AnalogWDGConfTypeDef AnalogWDGConfig = {0};

    AnalogWDGConfig.WatchdogMode  = ADC_ANALOGWATCHDOG_SINGLE_REG;
    AnalogWDGConfig.HighThreshold = AWD_HIGH_THRESHOLD;
    AnalogWDGConfig.LowThreshold  = AWD_LOW_THRESHOLD;
    AnalogWDGConfig.Channel       = ADC_CHANNEL_0;
    AnalogWDGConfig.ITMode        = ENABLE;

    if (HAL_ADC_AnalogWDGConfig(&hadc1, &AnalogWDGConfig) != HAL_OK) {
        printf("[ERROR] Gagal konfigurasi Analog Watchdog!\r\n");
        while (1);
    }

    /* Aktifkan interrupt ADC di NVIC */
#if defined(STM32F103xB)
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
#else
    HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
#endif

    printf("[AWD] Analog Watchdog dikonfigurasi:\r\n");
    printf("      High Threshold = %u (%.2fV)\r\n",
           AWD_HIGH_THRESHOLD, (float)AWD_HIGH_THRESHOLD * 3.3f / ADC_RESOLUTION);
    printf("      Low  Threshold = %u (%.2fV)\r\n",
           AWD_LOW_THRESHOLD, (float)AWD_LOW_THRESHOLD * 3.3f / ADC_RESOLUTION);
    printf("      Window aman    = %u - %u\r\n",
           AWD_LOW_THRESHOLD, AWD_HIGH_THRESHOLD);
}

/* ===========================================================================
 * Inisialisasi DWT Cycle Counter untuk pengukuran waktu presisi
 * =========================================================================== */
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t DWT_GetMicros(void) {
    return DWT->CYCCNT / (SystemCoreClock / 1000000U);
}

/* ===========================================================================
 * TIM2 Init untuk software polling benchmark
 * =========================================================================== */
static void TIM2_Init(void) {
#if defined(STM32F103xB)
    __HAL_RCC_TIM2_CLK_ENABLE();
#else
    __HAL_RCC_TIM2_CLK_ENABLE();
#endif

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = (SystemCoreClock / 1000000U) - 1; /* 1MHz tick */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFFFFFF;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start(&htim2);
}

/* ===========================================================================
 * Print header informasi program
 * =========================================================================== */
static void Print_Header(void) {
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32_13: ADC Analog Watchdog (Hardware AWD)\r\n");
    printf("  FITUR KHUSUS STM32 - Tidak tersedia di ESP32!\r\n");
    printf("============================================================\r\n");
#if defined(STM32F103xB)
    printf("  Board : BluePill STM32F103C8 @ %luMHz\r\n", SystemCoreClock / 1000000UL);
#elif defined(STM32F401xC)
    printf("  Board : BlackPill STM32F401CC @ %luMHz\r\n", SystemCoreClock / 1000000UL);
#elif defined(STM32F411xE)
    printf("  Board : BlackPill STM32F411CE @ %luMHz\r\n", SystemCoreClock / 1000000UL);
#endif
    printf("  ADC   : PA0 (ADC1 Channel 0, 12-bit)\r\n");
    printf("  LED   : PC13 (toggle saat AWD trigger)\r\n");
    printf("  UART  : PA2/PA3 (USART2, 115200 baud)\r\n");
    printf("============================================================\r\n");
    printf("\r\n");
    printf("PENJELASAN ANALOG WATCHDOG:\r\n");
    printf("  Hardware ADC secara OTOMATIS memonitor nilai konversi.\r\n");
    printf("  Jika nilai keluar dari jendela [LOW..HIGH], interrupt\r\n");
    printf("  langsung di-trigger TANPA intervensi CPU.\r\n");
    printf("  -> ESP32: harus polling + compare di software (lambat)\r\n");
    printf("  -> STM32: hardware interrupt < 1us (cepat)\r\n");
    printf("============================================================\r\n\r\n");
}

/* ===========================================================================
 * Demo 1: Hardware Analog Watchdog monitoring
 * CPU tidak perlu melakukan apa-apa - hardware yang memonitor!
 * =========================================================================== */
static void Demo_HardwareAWD(void) {
    printf("=== DEMO 1: Hardware Analog Watchdog ===\r\n");
    printf("Putar potentiometer untuk memicu AWD...\r\n");
    printf("Window aman: %u - %u (%.2fV - %.2fV)\r\n\r\n",
           AWD_LOW_THRESHOLD, AWD_HIGH_THRESHOLD,
           (float)AWD_LOW_THRESHOLD * 3.3f / ADC_RESOLUTION,
           (float)AWD_HIGH_THRESHOLD * 3.3f / ADC_RESOLUTION);

    /* Mulai konversi ADC continuous dengan interrupt AWD */
    HAL_ADC_Start_IT(&hadc1);

    uint32_t last_print = 0;
    uint32_t demo_start = HAL_GetTick();

    /* Jalankan demo selama 15 detik */
    while ((HAL_GetTick() - demo_start) < 15000) {
        /* Cetak status setiap 500ms */
        if ((HAL_GetTick() - last_print) >= 500) {
            last_print = HAL_GetTick();

            /* Baca nilai ADC saat ini */
            uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
            float    voltage = (float)adc_val * 3.3f / ADC_RESOLUTION;

            /* Tentukan status window */
            const char *status;
            if (adc_val > AWD_HIGH_THRESHOLD)
                status = "!! ABOVE HIGH !!";
            else if (adc_val < AWD_LOW_THRESHOLD)
                status = "!! BELOW LOW  !!";
            else
                status = "   OK (in window)";

            printf("[HW-AWD] ADC=%4lu (%.2fV) %s | AWD alerts=%lu\r\n",
                   adc_val, voltage, status, g_awd_count);
        }

        /* Cek apakah AWD baru saja trigger */
        if (g_awd_flag) {
            g_awd_flag = 0;
            float v = (float)g_awd_value * 3.3f / ADC_RESOLUTION;
            printf("  >>> AWD INTERRUPT! Nilai=%lu (%.2fV) pada t=%lums\r\n",
                   g_awd_value, v, g_awd_timestamp);

            if (g_awd_value > AWD_HIGH_THRESHOLD)
                printf("      Penyebab: Tegangan MELEBIHI batas atas (%u)\r\n",
                       AWD_HIGH_THRESHOLD);
            else
                printf("      Penyebab: Tegangan DI BAWAH batas bawah (%u)\r\n",
                       AWD_LOW_THRESHOLD);
        }
    }

    HAL_ADC_Stop_IT(&hadc1);
    printf("\r\n[HW-AWD] Demo selesai. Total AWD alerts: %lu\r\n\r\n", g_awd_count);
}

/* ===========================================================================
 * Demo 2: Software Polling (cara ESP32 melakukannya)
 * CPU harus aktif membaca ADC dan membandingkan secara manual
 * =========================================================================== */
static void Demo_SoftwarePolling(void) {
    printf("=== DEMO 2: Software Polling (seperti ESP32) ===\r\n");
    printf("Bandingkan dengan Hardware AWD di atas...\r\n\r\n");

    g_sw_poll_count = 0;
    uint32_t poll_iter       = 0;
    uint32_t total_poll_us   = 0;
    uint32_t demo_start      = HAL_GetTick();
    uint32_t last_print      = 0;

    /* Jalankan demo selama 10 detik */
    while ((HAL_GetTick() - demo_start) < 10000) {
        /* ---- Mulai pengukuran waktu polling ---- */
        uint32_t t_start = DWT_GetMicros();

        /* Langkah 1: Mulai konversi ADC (software trigger) */
        HAL_ADC_Start(&hadc1);

        /* Langkah 2: Tunggu konversi selesai (BLOCKING!) */
        HAL_ADC_PollForConversion(&hadc1, 10);

        /* Langkah 3: Baca nilai */
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);

        /* Langkah 4: Bandingkan dengan threshold secara SOFTWARE */
        uint8_t out_of_window = 0;
        if (adc_val > AWD_HIGH_THRESHOLD || adc_val < AWD_LOW_THRESHOLD) {
            out_of_window = 1;
            g_sw_poll_count++;
        }

        HAL_ADC_Stop(&hadc1);

        uint32_t t_end   = DWT_GetMicros();
        uint32_t elapsed = t_end - t_start;
        total_poll_us   += elapsed;
        poll_iter++;

        /* Cetak setiap 500ms */
        if ((HAL_GetTick() - last_print) >= 500) {
            last_print = HAL_GetTick();
            float voltage = (float)adc_val * 3.3f / ADC_RESOLUTION;
            float avg_us  = (poll_iter > 0) ? (float)total_poll_us / poll_iter : 0;

            printf("[SW-POLL] ADC=%4lu (%.2fV) %s | deteksi=%lu | avg=%.1fus/poll\r\n",
                   adc_val, voltage,
                   out_of_window ? "!! OUT !!" : "   OK   ",
                   g_sw_poll_count, avg_us);
        }

        /* Delay kecil simulasi beban kerja lain */
        HAL_Delay(1);
    }

    g_sw_poll_time_us = (poll_iter > 0) ? total_poll_us / poll_iter : 0;

    printf("\r\n[SW-POLL] Demo selesai.\r\n");
    printf("  Total polling   : %lu iterasi\r\n", poll_iter);
    printf("  Total deteksi   : %lu kali\r\n", g_sw_poll_count);
    printf("  Rata-rata waktu : %luus per poll\r\n", g_sw_poll_time_us);
    printf("\r\n");
}

/* ===========================================================================
 * Perbandingan Hardware AWD vs Software Polling
 * =========================================================================== */
static void Print_Comparison(void) {
    printf("============================================================\r\n");
    printf("  PERBANDINGAN: Hardware AWD vs Software Polling\r\n");
    printf("============================================================\r\n");
    printf("\r\n");
    printf("  +-----------------------+-----------------+-----------------+\r\n");
    printf("  | Parameter             | Hardware AWD    | Software Poll   |\r\n");
    printf("  +-----------------------+-----------------+-----------------+\r\n");
    printf("  | Metode                | HW Interrupt    | CPU Polling     |\r\n");
    printf("  | Response time         | < 1 us          | %4lu us         |\r\n",
           g_sw_poll_time_us);
    printf("  | CPU usage monitoring  | 0%%              | ~100%%           |\r\n");
    printf("  | Akurasi threshold     | Exact (HW)      | Tergantung rate |\r\n");
    printf("  | Bisa miss event?      | TIDAK           | YA (jika sibuk) |\r\n");
    printf("  | Multi-channel         | Ya (all/single) | Manual iterasi  |\r\n");
    printf("  | Tersedia di ESP32?    | TIDAK           | Ya (satu2nya)   |\r\n");
    printf("  +-----------------------+-----------------+-----------------+\r\n");
    printf("\r\n");
    printf("  KESIMPULAN:\r\n");
    printf("  Hardware AWD STM32 memberikan monitoring ADC yang:\r\n");
    printf("  1. Lebih cepat (hardware interrupt vs software polling)\r\n");
    printf("  2. Lebih hemat CPU (0%% vs ~100%% selama monitoring)\r\n");
    printf("  3. Lebih reliable (tidak bisa miss event)\r\n");
    printf("  4. Ideal untuk safety-critical applications\r\n");
    printf("============================================================\r\n\r\n");
}

/* ===========================================================================
 * FUNGSI UTAMA (main)
 * =========================================================================== */
int main(void) {
    /* Inisialisasi HAL */
    HAL_Init();

    /* Konfigurasi System Clock */
    SystemClock_Config();

    /* Inisialisasi peripheral */
    GPIO_Init();
    UART2_Init();
    DWT_Init();
    TIM2_Init();
    ADC1_Init();

    /* Cetak informasi program */
    Print_Header();

    /* Konfigurasi Analog Watchdog */
    ADC1_AWD_Config();

    /* ---- Demo 1: Hardware AWD (fitur STM32-specific!) ---- */
    Demo_HardwareAWD();

    /* ---- Demo 2: Software Polling (cara ESP32) ---- */
    Demo_SoftwarePolling();

    /* ---- Tampilkan perbandingan ---- */
    Print_Comparison();

    /* ---- Loop monitoring kontinu ---- */
    printf("=== Mode Monitoring Kontinu (Hardware AWD aktif) ===\r\n");
    printf("Putar potentiometer untuk melihat AWD beraksi...\r\n\r\n");

    g_awd_count = 0;
    HAL_ADC_Start_IT(&hadc1);

    uint32_t last_report = 0;

    while (1) {
        /* Cetak status periodik setiap 1 detik */
        if ((HAL_GetTick() - last_report) >= 1000) {
            last_report = HAL_GetTick();
            uint32_t val = HAL_ADC_GetValue(&hadc1);
            float    v   = (float)val * 3.3f / ADC_RESOLUTION;

            /* Visualisasi bar graph */
            char bar[52];
            memset(bar, ' ', 51);
            bar[51] = '\0';
            uint8_t pos = (uint8_t)((uint32_t)val * 50 / ADC_RESOLUTION);
            if (pos > 50) pos = 50;
            for (uint8_t i = 0; i < pos; i++) bar[i] = '#';

            /* Tandai posisi threshold pada bar */
            uint8_t low_pos  = (uint8_t)((uint32_t)AWD_LOW_THRESHOLD * 50 / ADC_RESOLUTION);
            uint8_t high_pos = (uint8_t)((uint32_t)AWD_HIGH_THRESHOLD * 50 / ADC_RESOLUTION);
            if (low_pos  <= 50) bar[low_pos]  = '|';
            if (high_pos <= 50) bar[high_pos] = '|';

            printf("[%5lu] ADC=%4lu (%.2fV) [%s] alerts=%lu\r\n",
                   HAL_GetTick() / 1000, val, v, bar, g_awd_count);
        }

        /* Proses AWD event */
        if (g_awd_flag) {
            g_awd_flag = 0;
            printf("  >>>> AWD ALERT! val=%lu t=%lums <<<<\r\n",
                   g_awd_value, g_awd_timestamp);
        }
    }
}
