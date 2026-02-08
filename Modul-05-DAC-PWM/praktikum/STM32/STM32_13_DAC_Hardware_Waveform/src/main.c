/**
 * ============================================================================
 * PROGRAM STM32_13: DAC Hardware Waveform Generator (Fitur Khusus STM32)
 * ============================================================================
 *
 * FITUR KHUSUS STM32 YANG TIDAK ADA DI ESP32:
 * ============================================
 * Beberapa seri STM32 (F405, F407, F446, dll) memiliki DAC dengan generator
 * gelombang HARDWARE built-in yang dapat menghasilkan:
 *   1. Gelombang Segitiga (Triangle Wave) - amplitude terkontrol hardware
 *   2. Gelombang Noise (LFSR-based) - pseudo-random noise generator
 *
 * Generator ini bekerja SEPENUHNYA di hardware:
 *   - CPU usage = 0% (hardware generate waveform otomatis)
 *   - Timer trigger (TIM6/TIM7) mengontrol frekuensi output
 *   - DACEx functions: HAL_DACEx_TriangleWaveGenerate()
 *                      HAL_DACEx_NoiseWaveGenerate()
 *
 * Pada ESP32, untuk menghasilkan gelombang:
 *   - HARUS menggunakan software (update DAC value dalam timer ISR)
 *   - CPU usage tinggi (~20-50% tergantung frekuensi)
 *   - Kualitas gelombang tergantung timing software
 *
 * CATATAN PENTING TENTANG BOARD:
 * ==============================
 * STM32F103C8 (BluePill)  : TIDAK memiliki DAC -> menggunakan PWM fallback
 * STM32F401CC (BlackPill)  : TIDAK memiliki DAC -> menggunakan PWM fallback
 * STM32F411CE (BlackPill)  : TIDAK memiliki DAC -> menggunakan PWM fallback
 *
 * Untuk menggunakan fitur DAC hardware waveform, gunakan board seperti:
 *   - STM32F405RG, STM32F407VG, STM32F446RE, STM32F746ZG, dll.
 *
 * Program ini tetap berfungsi pada ketiga board target dengan menggunakan
 * PWM + software update sebagai demonstrasi alternatif, sambil menyertakan
 * kode DAC hardware yang terlindungi oleh #if defined(DAC) agar bisa
 * langsung digunakan pada board yang memiliki DAC.
 *
 * KONFIGURASI HARDWARE:
 *   - PA6 : Output gelombang (TIM3_CH1 PWM)
 *   - PA4 : Output DAC (jika tersedia, DAC_CHANNEL_1)
 *   - PA2 : USART2 TX
 *   - PA3 : USART2 RX
 *   - PC13: LED indikator mode aktif
 *
 * TARGET BOARD:
 *   - BluePill  STM32F103C8 (PWM software waveform)
 *   - BlackPill STM32F401CC (PWM software waveform)
 *   - BlackPill STM32F411CE (PWM software waveform)
 *   - (DAC hardware path siap untuk F405/F407/F446)
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

/* ---- Header HAL sesuai chip ---- */
#if defined(STM32F103xB)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Board tidak didukung!"
#endif

/* ---- Deteksi ketersediaan hardware DAC ---- */
#if defined(DAC) || defined(DAC1)
  #define HAS_HARDWARE_DAC  1
#else
  #define HAS_HARDWARE_DAC  0
#endif

/* ---- Deteksi ketersediaan TIM6 (untuk DAC trigger) ---- */
#if defined(TIM6)
  #define HAS_TIM6  1
#else
  #define HAS_TIM6  0
#endif

/* ---- Konstanta Waveform ---- */
#define WAVEFORM_TRIANGLE   0
#define WAVEFORM_NOISE      1
#define WAVEFORM_SAWTOOTH   2

#define TRIANGLE_STEPS      256       /* Langkah untuk satu periode segitiga */
#define PWM_PERIOD          999       /* Auto-reload TIM3 (10-bit PWM) */
#define UPDATE_FREQ_HZ      10000     /* Frekuensi update waveform (Hz) */

/* ---- Handle Peripheral ---- */
static UART_HandleTypeDef huart2;
static TIM_HandleTypeDef  htim3;      /* PWM output pada PA6 */
static TIM_HandleTypeDef  htim2;      /* Timer update waveform (software) */
#if HAS_HARDWARE_DAC && HAS_TIM6
static DAC_HandleTypeDef  hdac;
static TIM_HandleTypeDef  htim6;      /* DAC trigger timer */
#endif

/* ---- Variabel Waveform ---- */
static volatile uint8_t   g_waveform_type  = WAVEFORM_TRIANGLE;
static volatile uint16_t  g_waveform_pos   = 0;     /* Posisi dalam waveform */
static volatile uint16_t  g_waveform_value = 0;     /* Nilai output saat ini */
static volatile uint32_t  g_update_count   = 0;     /* Jumlah update (SW mode) */
static volatile uint8_t   g_direction      = 0;     /* 0=naik, 1=turun */

/* ---- LFSR untuk noise generation ---- */
static volatile uint16_t  g_lfsr = 0xACE1u;  /* Seed LFSR */

/* ---- Deklarasi Fungsi ---- */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART2_Init(void);
static void TIM3_PWM_Init(void);
static void TIM2_WaveformUpdate_Init(void);
static uint16_t Generate_Triangle(void);
static uint16_t Generate_Noise(void);
static uint16_t Generate_Sawtooth(void);
static void Demo_SoftwareWaveform(uint8_t type, const char *name, uint32_t duration_ms);
static void Print_Header(void);
static void Print_Comparison(void);
#if HAS_HARDWARE_DAC
static void DAC_Init(void);
static void Demo_HardwareTriangle(void);
static void Demo_HardwareNoise(void);
#endif

/* ===========================================================================
 * Redirect printf ke USART2
 * =========================================================================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===========================================================================
 * Interrupt Handlers
 * =========================================================================== */
void SysTick_Handler(void) {
    HAL_IncTick();
}

void NMI_Handler(void) { }
void HardFault_Handler(void) { while (1); }

/* TIM2 interrupt untuk update waveform software */
void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim2);
}

/* ===========================================================================
 * Callback TIM Period Elapsed - Update waveform software
 * =========================================================================== */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        uint16_t val = 0;

        switch (g_waveform_type) {
            case WAVEFORM_TRIANGLE:
                val = Generate_Triangle();
                break;
            case WAVEFORM_NOISE:
                val = Generate_Noise();
                break;
            case WAVEFORM_SAWTOOTH:
                val = Generate_Sawtooth();
                break;
        }

        g_waveform_value = val;
        g_update_count++;

        /* Update PWM duty cycle */
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, val);
    }
}

/* ===========================================================================
 * Generator Gelombang Software
 * =========================================================================== */

/* Gelombang Segitiga: naik dari 0 ke PWM_PERIOD, lalu turun */
static uint16_t Generate_Triangle(void) {
    if (g_direction == 0) {
        g_waveform_pos += (PWM_PERIOD / TRIANGLE_STEPS);
        if (g_waveform_pos >= PWM_PERIOD) {
            g_waveform_pos = PWM_PERIOD;
            g_direction = 1;
        }
    } else {
        if (g_waveform_pos < (PWM_PERIOD / TRIANGLE_STEPS)) {
            g_waveform_pos = 0;
            g_direction = 0;
        } else {
            g_waveform_pos -= (PWM_PERIOD / TRIANGLE_STEPS);
        }
    }
    return g_waveform_pos;
}

/* Noise menggunakan LFSR (Linear Feedback Shift Register) */
static uint16_t Generate_Noise(void) {
    /* 16-bit Galois LFSR dengan tap 16,14,13,11 */
    uint16_t bit = ((g_lfsr >> 0) ^ (g_lfsr >> 2) ^
                    (g_lfsr >> 3) ^ (g_lfsr >> 5)) & 1u;
    g_lfsr = (g_lfsr >> 1) | (bit << 15);
    return (g_lfsr & 0x03FF); /* 10-bit output sesuai PWM range */
}

/* Gelombang Sawtooth: naik linier dari 0 ke max, lalu reset */
static uint16_t Generate_Sawtooth(void) {
    g_waveform_pos += (PWM_PERIOD / TRIANGLE_STEPS);
    if (g_waveform_pos >= PWM_PERIOD) {
        g_waveform_pos = 0;
    }
    return g_waveform_pos;
}

/* ===========================================================================
 * Konfigurasi System Clock
 * =========================================================================== */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

#if defined(STM32F103xB)
    /* F103: HSE 8MHz -> PLL -> 72MHz */
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
    /* F401: HSE 25MHz -> PLL -> 84MHz */
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
    /* F411: HSE 25MHz -> PLL -> 100MHz */
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
 * Inisialisasi GPIO
 * =========================================================================== */
static void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED PC13 */
    GPIO_Init_s.Pin   = GPIO_PIN_13;
    GPIO_Init_s.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_LOW;
#if !defined(STM32F103xB)
    GPIO_Init_s.Pull  = GPIO_NOPULL;
#endif
    HAL_GPIO_Init(GPIOC, &GPIO_Init_s);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* ===========================================================================
 * Inisialisasi USART2 (PA2=TX, PA3=RX)
 * =========================================================================== */
static void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_Init_s = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

#if defined(STM32F103xB)
    GPIO_Init_s.Pin   = GPIO_PIN_2;
    GPIO_Init_s.Mode  = GPIO_MODE_AF_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);

    GPIO_Init_s.Pin   = GPIO_PIN_3;
    GPIO_Init_s.Mode  = GPIO_MODE_INPUT;
    GPIO_Init_s.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#else
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
 * Inisialisasi TIM3 sebagai PWM output pada PA6 (Channel 1)
 * Ini adalah output waveform menggunakan software update
 * =========================================================================== */
static void TIM3_PWM_Init(void) {
    GPIO_InitTypeDef       GPIO_Init_s = {0};
    TIM_OC_InitTypeDef     sConfigOC   = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6: TIM3_CH1 */
#if defined(STM32F103xB)
    GPIO_Init_s.Pin   = GPIO_PIN_6;
    GPIO_Init_s.Mode  = GPIO_MODE_AF_PP;
    GPIO_Init_s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#else
    GPIO_Init_s.Pin       = GPIO_PIN_6;
    GPIO_Init_s.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init_s.Pull      = GPIO_NOPULL;
    GPIO_Init_s.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init_s.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);
#endif

    /*
     * PWM Configuration:
     * Frekuensi PWM = TIM_CLK / (Prescaler+1) / (Period+1)
     * Untuk F103 (72MHz APB1*2=72MHz TIM):
     *   72MHz / 1 / 1000 = 72kHz PWM carrier
     * Untuk F401 (42MHz APB1*2=84MHz TIM):
     *   84MHz / 1 / 1000 = 84kHz PWM carrier
     * Untuk F411 (50MHz APB1*2=100MHz TIM):
     *   100MHz / 1 / 1000 = 100kHz PWM carrier
     *
     * PWM carrier cukup tinggi sehingga dengan LPF sederhana
     * bisa menghasilkan sinyal analog yang halus.
     */
    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 0;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = PWM_PERIOD;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity  = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode  = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

/* ===========================================================================
 * Inisialisasi TIM2 sebagai timer update waveform (interrupt periodik)
 * Frekuensi = UPDATE_FREQ_HZ (default 10kHz)
 * =========================================================================== */
static void TIM2_WaveformUpdate_Init(void) {
    uint32_t tim_clk;

    __HAL_RCC_TIM2_CLK_ENABLE();

#if defined(STM32F103xB)
    tim_clk = 72000000UL; /* APB1 prescaler = /2, tapi TIM clock x2 = 72MHz */
#elif defined(STM32F401xC)
    tim_clk = 84000000UL;
#elif defined(STM32F411xE)
    tim_clk = 100000000UL;
#else
    tim_clk = SystemCoreClock;
#endif

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = (tim_clk / 1000000U) - 1; /* 1MHz tick */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = (1000000U / UPDATE_FREQ_HZ) - 1; /* 10kHz */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim2);

    HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/* ===========================================================================
 * KODE DAC HARDWARE (hanya dikompilasi untuk chip dengan DAC)
 * ===========================================================================
 * Bagian ini mendemonstrasikan fitur DAC hardware waveform generation
 * yang EKSKLUSIF untuk STM32. Kode ini TIDAK akan dikompilasi untuk
 * F103C8, F401CC, F411CE karena mereka tidak memiliki peripheral DAC.
 *
 * Untuk menjalankan bagian ini, gunakan board seperti:
 *   - STM32F407VG (Discovery)
 *   - STM32F446RE (Nucleo-64)
 *   - STM32F746ZG (Nucleo-144)
 * =========================================================================== */
#if HAS_HARDWARE_DAC

static void DAC_Init(void) {
    GPIO_InitTypeDef     GPIO_Init_s = {0};
    DAC_ChannelConfTypeDef sConfig   = {0};

    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA4: DAC_OUT1 (Analog Output) */
    GPIO_Init_s.Pin  = GPIO_PIN_4;
    GPIO_Init_s.Mode = GPIO_MODE_ANALOG;
    GPIO_Init_s.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_Init_s);

    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);

    /* Konfigurasi DAC Channel 1 */
    sConfig.DAC_Trigger      = DAC_TRIGGER_T6_TRGO;   /* Trigger dari TIM6 */
    sConfig.DAC_OutputBuffer  = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
}

/*
 * Demo: Hardware Triangle Wave Generator
 * STM32 DAC menghasilkan gelombang segitiga secara OTOMATIS di hardware.
 * CPU sama sekali TIDAK terlibat setelah konfigurasi!
 */
static void Demo_HardwareTriangle(void) {
    printf("\r\n=== DEMO: Hardware Triangle Wave (DAC) ===\r\n");
    printf("DAC menghasilkan segitiga secara HARDWARE pada PA4\r\n");
    printf("CPU usage = 0%% setelah konfigurasi!\r\n\r\n");

    /* Set base value (offset) untuk triangle wave */
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);

    /* Konfigurasi hardware triangle wave generator */
    /* DAC_TRIANGLEAMPLITUDE_4095 = amplitude penuh 12-bit */
    HAL_DACEx_TriangleWaveGenerate(&hdac, DAC_CHANNEL_1,
                                    DAC_TRIANGLEAMPLITUDE_4095);

    /* Mulai DAC dengan trigger dari TIM6 */
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

    printf("[HW-DAC] Triangle wave aktif - hardware generate otomatis\r\n");
    printf("[HW-DAC] Frekuensi dikontrol oleh TIM6 trigger\r\n");

    /* Biarkan berjalan selama 10 detik */
    for (int i = 0; i < 10; i++) {
        HAL_Delay(1000);
        printf("[HW-DAC] t=%ds - Triangle aktif (CPU bebas melakukan hal lain)\r\n", i + 1);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); /* LED toggle menunjukkan CPU bebas */
    }

    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
    printf("[HW-DAC] Triangle wave selesai.\r\n\r\n");
}

/*
 * Demo: Hardware Noise Wave Generator (LFSR-based)
 * STM32 DAC memiliki LFSR hardware yang menghasilkan pseudo-random noise.
 */
static void Demo_HardwareNoise(void) {
    printf("=== DEMO: Hardware Noise Wave (DAC LFSR) ===\r\n");
    printf("DAC menghasilkan noise LFSR secara HARDWARE pada PA4\r\n\r\n");

    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048);

    /* DAC_LFSRUNMASK_BITS11_0 = 12-bit LFSR noise */
    HAL_DACEx_NoiseWaveGenerate(&hdac, DAC_CHANNEL_1,
                                 DAC_LFSRUNMASK_BITS11_0);

    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

    printf("[HW-DAC] Noise LFSR aktif - hardware generate otomatis\r\n");

    for (int i = 0; i < 10; i++) {
        HAL_Delay(1000);
        printf("[HW-DAC] t=%ds - Noise aktif (CPU bebas)\r\n", i + 1);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
    printf("[HW-DAC] Noise wave selesai.\r\n\r\n");
}

#endif /* HAS_HARDWARE_DAC */

/* ===========================================================================
 * Demo Software Waveform (menggunakan PWM - fallback untuk semua board)
 * Ini adalah cara ESP32 harus melakukannya - CPU selalu terlibat!
 * =========================================================================== */
static void Demo_SoftwareWaveform(uint8_t type, const char *name,
                                   uint32_t duration_ms) {
    printf("=== DEMO: Software %s (PWM pada PA6) ===\r\n", name);
    printf("CPU harus update PWM setiap %d us (%.1f kHz update rate)\r\n",
           1000000 / UPDATE_FREQ_HZ, (float)UPDATE_FREQ_HZ / 1000.0f);
    printf("Ini adalah cara ESP32 harus generate waveform!\r\n\r\n");

    /* Reset state waveform */
    g_waveform_type  = type;
    g_waveform_pos   = 0;
    g_direction      = 0;
    g_update_count   = 0;

    /* Mulai timer interrupt untuk update waveform */
    HAL_TIM_Base_Start_IT(&htim2);

    uint32_t start_tick = HAL_GetTick();
    uint32_t last_print = 0;
    uint32_t last_count = 0;

    while ((HAL_GetTick() - start_tick) < duration_ms) {
        if ((HAL_GetTick() - last_print) >= 1000) {
            last_print = HAL_GetTick();

            uint32_t updates_per_sec = g_update_count - last_count;
            last_count = g_update_count;

            /* Hitung CPU usage estimasi */
            /* Setiap update ~ 1-2us interrupt overhead pada 72-100MHz */
            float cpu_usage = (float)updates_per_sec * 2.0f / 10000.0f;

            printf("[SW-%s] val=%4u | updates/s=%lu | CPU~%.1f%%\r\n",
                   name, g_waveform_value, updates_per_sec, cpu_usage);

            /* Visualisasi waveform di terminal */
            char bar[52];
            memset(bar, ' ', 51);
            bar[51] = '\0';
            uint8_t pos = (uint8_t)((uint32_t)g_waveform_value * 50 / PWM_PERIOD);
            if (pos > 50) pos = 50;
            for (uint8_t i = 0; i < pos; i++) bar[i] = '#';
            printf("         [%s]\r\n", bar);

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }

    HAL_TIM_Base_Stop_IT(&htim2);

    printf("[SW-%s] Selesai. Total updates: %lu\r\n\r\n", name, g_update_count);
}

/* ===========================================================================
 * Print Header
 * =========================================================================== */
static void Print_Header(void) {
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32_13: DAC Hardware Waveform Generator\r\n");
    printf("  FITUR KHUSUS STM32 - Tidak tersedia di ESP32!\r\n");
    printf("============================================================\r\n");
#if defined(STM32F103xB)
    printf("  Board : BluePill STM32F103C8 @ %luMHz\r\n", SystemCoreClock / 1000000UL);
    printf("  DAC   : TIDAK TERSEDIA -> menggunakan PWM fallback\r\n");
#elif defined(STM32F401xC)
    printf("  Board : BlackPill STM32F401CC @ %luMHz\r\n", SystemCoreClock / 1000000UL);
    printf("  DAC   : TIDAK TERSEDIA -> menggunakan PWM fallback\r\n");
#elif defined(STM32F411xE)
    printf("  Board : BlackPill STM32F411CE @ %luMHz\r\n", SystemCoreClock / 1000000UL);
    printf("  DAC   : TIDAK TERSEDIA -> menggunakan PWM fallback\r\n");
#endif
    printf("  Output: PA6 (TIM3_CH1 PWM, perlu LPF RC untuk sinyal analog)\r\n");
    printf("  UART  : PA2/PA3 (USART2, 115200 baud)\r\n");
    printf("============================================================\r\n");
    printf("\r\n");
    printf("PENJELASAN DAC HARDWARE WAVEFORM (fitur STM32):\r\n");
    printf("  STM32 dengan DAC (F405/F407/F446/dll) dapat menghasilkan\r\n");
    printf("  gelombang segitiga dan noise SEPENUHNYA di hardware:\r\n");
    printf("  - HAL_DACEx_TriangleWaveGenerate() -> segitiga HW\r\n");
    printf("  - HAL_DACEx_NoiseWaveGenerate()    -> noise LFSR HW\r\n");
    printf("  - CPU usage = 0%% setelah konfigurasi!\r\n");
    printf("  - Timer (TIM6) men-trigger DAC output secara otomatis\r\n");
    printf("\r\n");
    printf("  ESP32 HARUS menggunakan software:\r\n");
    printf("  - Timer ISR update DAC value setiap interval\r\n");
    printf("  - CPU usage = 10-50%% tergantung frekuensi\r\n");
    printf("  - Kualitas tergantung kestabilan timing software\r\n");
    printf("============================================================\r\n\r\n");

#if HAS_HARDWARE_DAC
    printf("[INFO] Hardware DAC terdeteksi! Menjalankan demo DAC hardware.\r\n\r\n");
#else
    printf("[INFO] Hardware DAC tidak tersedia pada board ini.\r\n");
    printf("       Menggunakan PWM + software sebagai demonstrasi.\r\n");
    printf("       Untuk demo DAC hardware, gunakan F405/F407/F446.\r\n\r\n");
#endif
}

/* ===========================================================================
 * Print Perbandingan
 * =========================================================================== */
static void Print_Comparison(void) {
    printf("============================================================\r\n");
    printf("  PERBANDINGAN: DAC Hardware vs Software Waveform\r\n");
    printf("============================================================\r\n\r\n");
    printf("  +---------------------+------------------+------------------+\r\n");
    printf("  | Parameter           | HW DAC (STM32)   | SW PWM (ESP32)   |\r\n");
    printf("  +---------------------+------------------+------------------+\r\n");
    printf("  | CPU usage           | 0%%               | 10-50%%           |\r\n");
    printf("  | Jitter              | 0 (hardware)     | Variable (ISR)   |\r\n");
    printf("  | Max frekuensi       | MHz range        | ~100 kHz         |\r\n");
    printf("  | Setup complexity    | Mudah (HAL)      | Manual coding    |\r\n");
    printf("  | Triangle wave       | 1 fungsi HAL     | ISR + lookup     |\r\n");
    printf("  | Noise generation    | HW LFSR          | SW LFSR          |\r\n");
    printf("  | Amplitude control   | HW register      | SW variable      |\r\n");
    printf("  | Output              | True analog (DAC) | PWM + LPF       |\r\n");
    printf("  | Tersedia di ESP32?  | TIDAK            | Ya (satu2nya)    |\r\n");
    printf("  +---------------------+------------------+------------------+\r\n\r\n");
    printf("  KESIMPULAN:\r\n");
    printf("  DAC hardware waveform STM32 sangat superior:\r\n");
    printf("  1. Zero CPU overhead -> CPU bebas untuk task lain\r\n");
    printf("  2. Zero jitter -> gelombang sempurna\r\n");
    printf("  3. True analog output -> tidak perlu LPF\r\n");
    printf("  4. Ideal untuk signal generator, audio, control systems\r\n");
    printf("============================================================\r\n\r\n");
}

/* ===========================================================================
 * FUNGSI UTAMA (main)
 * =========================================================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Inisialisasi peripheral */
    GPIO_Init();
    UART2_Init();
    TIM3_PWM_Init();
    TIM2_WaveformUpdate_Init();

    Print_Header();

#if HAS_HARDWARE_DAC
    /* ---- DEMO DAC HARDWARE (hanya pada board dengan DAC) ---- */
    DAC_Init();

    /* Demo 1: Hardware Triangle Wave */
    Demo_HardwareTriangle();

    /* Demo 2: Hardware Noise Wave */
    Demo_HardwareNoise();

    printf("\r\n--- Sekarang bandingkan dengan Software Waveform ---\r\n\r\n");
#endif

    /* ---- DEMO SOFTWARE WAVEFORM (semua board) ---- */
    /* Demo: Software Triangle Wave (cara ESP32 melakukannya) */
    Demo_SoftwareWaveform(WAVEFORM_TRIANGLE, "TRIANGLE", 8000);

    /* Demo: Software Noise (cara ESP32 melakukannya) */
    Demo_SoftwareWaveform(WAVEFORM_NOISE, "NOISE", 8000);

    /* Demo: Software Sawtooth (bonus) */
    Demo_SoftwareWaveform(WAVEFORM_SAWTOOTH, "SAWTOOTH", 5000);

    /* Tampilkan perbandingan */
    Print_Comparison();

    /* ---- Loop kontinu: cycle through waveforms ---- */
    printf("=== Mode Kontinu: Cycling Waveforms pada PA6 ===\r\n\r\n");

    uint8_t modes[] = {WAVEFORM_TRIANGLE, WAVEFORM_NOISE, WAVEFORM_SAWTOOTH};
    const char *names[] = {"Triangle", "Noise", "Sawtooth"};
    uint8_t mode_idx = 0;

    g_waveform_type = modes[0];
    g_waveform_pos  = 0;
    g_direction     = 0;
    HAL_TIM_Base_Start_IT(&htim2);

    uint32_t last_switch = HAL_GetTick();
    uint32_t last_report = 0;

    while (1) {
        /* Ganti waveform setiap 5 detik */
        if ((HAL_GetTick() - last_switch) >= 5000) {
            last_switch = HAL_GetTick();
            mode_idx = (mode_idx + 1) % 3;
            g_waveform_type = modes[mode_idx];
            g_waveform_pos  = 0;
            g_direction     = 0;
            printf("\r\n>> Beralih ke: %s wave\r\n", names[mode_idx]);
        }

        /* Report setiap 1 detik */
        if ((HAL_GetTick() - last_report) >= 1000) {
            last_report = HAL_GetTick();
            printf("[%s] val=%4u\r\n", names[mode_idx], g_waveform_value);
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}
