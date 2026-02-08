/**
 * ==========================================================
 *  Program     : PWM Frequency Sweep
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Melakukan sweep frekuensi PWM dari 100Hz
 *                sampai 20kHz dengan mengubah prescaler dan
 *                period TIM secara dinamis.
 *                TIM2_CH1 pada PA0.
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#define SYSTEM_CLOCK  72000000UL
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#define SYSTEM_CLOCK  84000000UL
#endif

#include <stdio.h>
#include <string.h>

/* ===================== Handle Peripheral ===================== */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;

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

/* ===================== Inisialisasi PWM TIM2 ===================== */
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

    /* Konfigurasi awal: 1kHz */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = (SYSTEM_CLOCK / 1000) - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = htim2.Init.Period / 2;  /* 50% duty */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

/* ===================== Fungsi Set Frekuensi PWM ===================== */
void PWM_SetFrequency(uint32_t freq_hz) {
    if (freq_hz == 0) return;

    /* Hitung prescaler dan period yang optimal */
    uint32_t timer_clock = SYSTEM_CLOCK;
    uint32_t prescaler = 0;
    uint32_t period = 0;

    /* Coba tanpa prescaler dulu */
    period = (timer_clock / freq_hz) - 1;

    /* Jika period terlalu besar (>65535 untuk 16-bit timer), naikkan prescaler */
    if (period > 65535) {
        prescaler = (period / 65536) + 1;
        period = (timer_clock / (prescaler + 1) / freq_hz) - 1;
    }

    /* Update prescaler dan period */
    __HAL_TIM_SET_PRESCALER(&htim2, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
    /* Set 50% duty cycle */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, period / 2);

    /* Generate update event untuk menerapkan perubahan segera */
    htim2.Instance->EGR = TIM_EGR_UG;
}

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_TIM2_PWM_Init();

    printf("\r\n=== PWM Frequency Sweep ===\r\n");
    printf("TIM2_CH1 pada PA0\r\n");
    printf("System Clock: %lu Hz\r\n", SYSTEM_CLOCK);
    printf("Sweep: 100 Hz -> 20 kHz\r\n");
    printf("Duty cycle: 50%%\r\n\r\n");

    /* Tabel frekuensi untuk sweep (Hz) */
    const uint32_t freq_table[] = {
        100, 200, 500, 1000, 2000, 3000, 4000, 5000,
        6000, 7000, 8000, 9000, 10000, 12000, 15000, 20000
    };
    const uint8_t freq_count = sizeof(freq_table) / sizeof(freq_table[0]);

    uint32_t sweep_num = 0;

    while (1) {
        sweep_num++;
        printf("--- Sweep #%lu ---\r\n", sweep_num);

        /* Sweep naik: 100Hz -> 20kHz */
        for (uint8_t i = 0; i < freq_count; i++) {
            PWM_SetFrequency(freq_table[i]);

            /* Hitung frekuensi aktual */
            uint32_t psc = htim2.Instance->PSC;
            uint32_t arr = htim2.Instance->ARR;
            uint32_t freq_actual = SYSTEM_CLOCK / ((psc + 1) * (arr + 1));

            printf("[SWEEP] Target=%5lu Hz | Aktual=%5lu Hz | PSC=%lu | ARR=%lu\r\n",
                   freq_table[i], freq_actual, psc, arr);

            HAL_Delay(500);
        }

        /* Sweep turun: 20kHz -> 100Hz */
        for (int8_t i = freq_count - 1; i >= 0; i--) {
            PWM_SetFrequency(freq_table[i]);

            uint32_t psc = htim2.Instance->PSC;
            uint32_t arr = htim2.Instance->ARR;
            uint32_t freq_actual = SYSTEM_CLOCK / ((psc + 1) * (arr + 1));

            printf("[SWEEP] Target=%5lu Hz | Aktual=%5lu Hz | PSC=%lu | ARR=%lu\r\n",
                   freq_table[i], freq_actual, psc, arr);

            HAL_Delay(500);
        }

        printf("\r\n");
        HAL_Delay(1000);
    }
}
