/**
 * ==========================================================
 *  Program     : DAC Sine Wave Generator
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Menghasilkan gelombang sinus melalui DAC (F4)
 *                menggunakan lookup table 256 titik.
 *                TIM6 digunakan sebagai trigger update DAC.
 *                F103: modulasi duty cycle PWM sebagai alternatif.
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
TIM_HandleTypeDef htim2;    /* PWM untuk F103 */

#ifdef STM32F4
DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim6;    /* Timer trigger untuk DAC */
#endif

/* ===================== Lookup Table Sinus 256 titik ===================== */
/* Nilai 0-4095 (12-bit), offset ke tengah (2048) dengan amplitudo 2047 */
static const uint16_t sine_table[256] = {
    2048, 2098, 2148, 2198, 2248, 2298, 2348, 2397,
    2447, 2496, 2545, 2594, 2642, 2690, 2737, 2784,
    2831, 2877, 2923, 2968, 3013, 3057, 3100, 3143,
    3185, 3227, 3268, 3308, 3348, 3387, 3425, 3462,
    3499, 3535, 3570, 3604, 3637, 3669, 3700, 3731,
    3760, 3789, 3817, 3843, 3869, 3893, 3917, 3939,
    3961, 3981, 4000, 4019, 4036, 4052, 4066, 4080,
    4093, 4094, 4095, 4095, 4095, 4093, 4090, 4085,
    4080, 4074, 4066, 4058, 4048, 4038, 4026, 4013,
    4000, 3985, 3969, 3952, 3934, 3916, 3896, 3875,
    3854, 3831, 3808, 3783, 3758, 3732, 3705, 3677,
    3648, 3619, 3589, 3558, 3526, 3493, 3460, 3426,
    3392, 3357, 3321, 3285, 3248, 3211, 3173, 3135,
    3096, 3057, 3017, 2977, 2937, 2896, 2855, 2814,
    2773, 2731, 2689, 2647, 2605, 2563, 2521, 2479,
    2437, 2395, 2353, 2311, 2269, 2228, 2186, 2145,
    2104, 2063, 2023, 1983, 1943, 1903, 1864, 1826,
    1788, 1750, 1713, 1676, 1640, 1604, 1569, 1535,
    1501, 1468, 1436, 1404, 1373, 1343, 1313, 1284,
    1256, 1229, 1203, 1177, 1152, 1128, 1105, 1082,
    1061, 1040, 1020, 1001,  983,  965,  949,  933,
     918,  905,  892,  880,  869,  859,  850,  842,
     835,  829,  824,  820,  817,  815,  814,  814,
     815,  817,  820,  824,  829,  835,  842,  850,
     859,  869,  880,  892,  905,  918,  933,  949,
     965,  983, 1001, 1020, 1040, 1061, 1082, 1105,
    1128, 1152, 1177, 1203, 1229, 1256, 1284, 1313,
    1343, 1373, 1404, 1436, 1468, 1501, 1535, 1569,
    1604, 1640, 1676, 1713, 1750, 1788, 1826, 1864,
    1903, 1943, 1983, 2023, 2063, 2104, 2145, 2186,
    2228, 2269, 2311, 2353, 2395, 2437, 2479, 2521,
    2563, 2605, 2647, 2689, 2731, 2773, 2814, 2855
};

static volatile uint16_t sine_index = 0;

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

/* ===================== Inisialisasi PWM TIM2 (F103) ===================== */
void MX_TIM2_PWM_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 4095;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
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

/* ===================== Inisialisasi TIM6 sebagai trigger ===================== */
void MX_TIM6_Init(void) {
    __HAL_RCC_TIM6_CLK_ENABLE();

    /* Frekuensi update = 84MHz / (Prescaler+1) / (Period+1)
     * Untuk sinus 1kHz dengan 256 titik: update rate = 256kHz
     * 84MHz / 1 / 328 ~ 256kHz */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 327;
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

/* ===================== Timer Callback ===================== */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        /* Update DAC dengan nilai sinus berikutnya */
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, sine_table[sine_index]);
        sine_index = (sine_index + 1) & 0xFF; /* Wrap 0-255 */
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
    printf("\r\n=== DAC Sine Wave Generator (STM32F4) ===\r\n");
    printf("DAC Channel 1 pada PA4\r\n");
    printf("Lookup table: 256 titik, 12-bit\r\n");
    printf("TIM6 sebagai trigger update\r\n");
    printf("Frekuensi sinus ~1 kHz\r\n\r\n");
#else
    MX_TIM2_PWM_Init();
    printf("\r\n=== PWM Sine Wave Modulation (STM32F103) ===\r\n");
    printf("CATATAN: F103 tidak punya DAC!\r\n");
    printf("Modulasi duty cycle PWM pada PA0\r\n");
    printf("Lookup table: 256 titik\r\n\r\n");
#endif

    uint32_t print_counter = 0;

    while (1) {
#ifndef STM32F4
        /* F103: update PWM duty cycle secara software */
        for (uint16_t i = 0; i < 256; i++) {
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, sine_table[i]);
            /* Delay kecil untuk mengatur frekuensi output */
            for (volatile int d = 0; d < 50; d++);
        }
#endif

        /* Cetak info setiap 1 detik */
        if (print_counter % 100 == 0) {
            uint16_t idx = sine_index;
            float voltage = (sine_table[idx] / 4095.0f) * 3.3f;
            printf("[SINE] Index=%3u, Nilai=%4u, Tegangan=%.2f V\r\n",
                   idx, sine_table[idx], voltage);
        }
        print_counter++;
        HAL_Delay(10);
    }
}
