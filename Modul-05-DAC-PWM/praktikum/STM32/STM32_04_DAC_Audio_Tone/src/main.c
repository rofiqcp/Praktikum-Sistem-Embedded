/**
 * ==========================================================
 *  Program     : DAC Audio Tone Generator
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Menghasilkan nada audio 440Hz (A4) dan 880Hz (A5)
 *                melalui DAC (F4) dengan timer-driven sample output.
 *                F103: PWM buzzer tone sebagai alternatif.
 *
 *  Perhitungan Frekuensi (F4 DAC):
 *    Untuk 440Hz dengan 64-sample lookup table:
 *      Timer rate = 440 × 64 = 28,160 Hz
 *      APB1 timer clock = 84 MHz (TIM6 on APB1, x2 multiplier)
 *      ARR = 84000000 / 28160 - 1 = 2982
 *    Untuk 880Hz: sample_skip = 2 → efektif 880Hz
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

/* ===================== Handle Peripheral ===================== */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;    /* PWM untuk F103 buzzer */

#ifdef STM32F4
DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim6;    /* Timer trigger untuk DAC sample output */
#endif

/* ===================== Tabel Sinus 64 titik (untuk audio) ===================== */
static const uint16_t sine_64[64] = {
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3348,
    3499, 3637, 3760, 3869, 3961, 4036, 4093, 4095,
    4095, 4093, 4066, 4000, 3916, 3808, 3677, 3526,
    3357, 3173, 2977, 2773, 2563, 2353, 2145, 1943,
    1750, 1569, 1404, 1256, 1128, 1020, 933, 869,
    829, 814, 815, 835, 869, 918, 983, 1061,
    1152, 1256, 1373, 1501, 1640, 1788, 1943, 2104,
    2269, 2437, 2605, 2773, 2937, 3096, 3248, 3392
};

/* Konfigurasi nada audio */
#define TONE_440HZ     0
#define TONE_880HZ     1

static volatile uint16_t sample_index = 0;
static volatile uint8_t current_tone = TONE_440HZ;
static volatile uint16_t sample_skip = 1;  /* Skip untuk mengatur frekuensi */

/* ===================== Retarget printf ===================== */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ===================== SysTick Handler ===================== */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/* ===================== Konfigurasi Clock ===================== */
void SystemClock_Config(void) {
#ifdef STM32F1
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#elif defined(STM32F4)
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 84;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

/* ===================== Inisialisasi UART1 ===================== */
void MX_USART1_UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ===================== Inisialisasi PWM TIM2 untuk buzzer (F103) ===================== */
void MX_TIM2_PWM_Init(uint32_t frequency) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Untuk buzzer: frekuensi PWM = 72MHz / (PSC+1) / (ARR+1) */
    uint32_t timer_clock = 72000000;
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    uint32_t timer_clock = 84000000;
#endif

    uint32_t period = (timer_clock / frequency) - 1;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = period;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = period / 2;  /* 50% duty cycle untuk nada */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

/* Fungsi untuk mengubah frekuensi buzzer PWM */
void Set_Buzzer_Frequency(uint32_t frequency) {
#ifdef STM32F1
    uint32_t timer_clock = 72000000;
#else
    uint32_t timer_clock = 84000000;
#endif
    uint32_t period = (timer_clock / frequency) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, period / 2);
}

/* Fungsi untuk mematikan buzzer */
void Buzzer_Off(void) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
}

#ifdef STM32F4
/* ===================== Inisialisasi DAC (F4) ===================== */
void MX_DAC_Init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

/* ===================== Inisialisasi TIM6 untuk sample rate ===================== */
void MX_TIM6_Init(void) {
    __HAL_RCC_TIM6_CLK_ENABLE();

    /*
     * Untuk menghasilkan nada 440Hz dengan 64-sample sine table:
     *   Timer interrupt rate = 440 × 64 = 28,160 Hz
     *   TIM6 clock = APB1 timer clock = 84 MHz
     *   (APB1 prescaler = /2, timer clock x2 multiplier)
     *   ARR = 84000000 / 28160 - 1 = 2982
     *
     * Verifikasi: 84000000 / (2982 + 1) = 28,160.24 Hz
     *             28,160.24 / 64 = 440.00 Hz ✓
     *
     * BUG LAMA: ARR = 2624 menghasilkan 84MHz/2625 = 32kHz
     *           32000/64 = 500Hz (BUKAN 440Hz!)
     */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 2982;  /* 84MHz / 28160 - 1 = 2982 → tepat 440Hz */
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim6);

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

    HAL_TIM_Base_Start_IT(&htim6);
}

/* ===================== TIM6 IRQ Handler ===================== */
void TIM6_DAC_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim6);
}

/* ===================== Timer Callback: output sampel audio ===================== */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        /* Output sampel sinus ke DAC */
        uint16_t idx = sample_index & 0x3F; /* Modulo 64 */
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, sine_64[idx]);
        sample_index += sample_skip;
    }
}
#endif

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();

#ifdef STM32F4
    MX_DAC_Init();
    MX_TIM6_Init();
    printf("\r\n=== DAC Audio Tone Generator (STM32F4) ===\r\n");
    printf("DAC Channel 1 pada PA4 (hubungkan ke speaker/amplifier)\r\n");
    printf("Timer rate: 440x64 = 28160 Hz, 64-point sine table\r\n");
    printf("ARR = 2982 → tepat 440Hz base tone\r\n\r\n");
#else
    MX_TIM2_PWM_Init(440);
    printf("\r\n=== PWM Buzzer Tone (STM32F103) ===\r\n");
    printf("CATATAN: F103 tidak punya DAC!\r\n");
    printf("PWM pada PA0 untuk passive buzzer\r\n\r\n");
#endif

    while (1) {
        /* === Nada 440Hz (A4) selama 2 detik === */
#ifdef STM32F4
        /*
         * 440Hz: sample_skip = 1
         * Timer fires at 28,160 Hz, stepping through 64 samples
         * → output frequency = 28160 / 64 = 440 Hz ✓
         */
        sample_skip = 1;
        current_tone = TONE_440HZ;
        printf("[AUDIO] Nada: 440 Hz (A4) - DAC sinus\r\n");
#else
        Set_Buzzer_Frequency(440);
        printf("[AUDIO] Nada: 440 Hz (A4) - PWM buzzer\r\n");
#endif
        HAL_Delay(2000);

        /* === Nada 880Hz (A5) selama 2 detik === */
#ifdef STM32F4
        /*
         * 880Hz: sample_skip = 2
         * Skips every other sample → effectively doubles frequency
         * → output frequency = 28160 / 64 * 2 = 880 Hz ✓
         */
        sample_skip = 2;
        current_tone = TONE_880HZ;
        printf("[AUDIO] Nada: 880 Hz (A5) - DAC sinus\r\n");
#else
        Set_Buzzer_Frequency(880);
        printf("[AUDIO] Nada: 880 Hz (A5) - PWM buzzer\r\n");
#endif
        HAL_Delay(2000);

        /* === Diam 1 detik === */
#ifdef STM32F4
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048); /* Tengah */
        sample_skip = 0;
        printf("[AUDIO] Diam (silence)\r\n");
#else
        Buzzer_Off();
        printf("[AUDIO] Diam (silence)\r\n");
#endif
        HAL_Delay(1000);

        printf("---\r\n");
    }
}
