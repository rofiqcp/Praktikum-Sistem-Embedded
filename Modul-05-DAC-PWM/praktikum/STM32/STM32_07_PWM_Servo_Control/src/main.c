/**
 * ==========================================================
 *  Program     : PWM Servo Control (SG90)
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Mengontrol servo SG90 menggunakan PWM 50Hz.
 *                TIM2_CH1 pada PA0.
 *                Pulse 1000us (0°) sampai 2000us (180°).
 *                Sweep terus-menerus 0° -> 180° -> 0°.
 *
 *  Perhitungan PWM 50Hz:
 *    F103: 72MHz / (72 * 20000) = 50Hz (PSC=71, ARR=19999)
 *    F4:   84MHz / (84 * 20000) = 50Hz (PSC=83, ARR=19999)
 *    Pulse 1ms = 1000, Pulse 2ms = 2000 (dari ARR=20000)
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>

/* ===================== Handle Peripheral ===================== */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;

/* ===================== Konstanta Servo ===================== */
#define SERVO_MIN_PULSE   500   /* 0.5ms = 0 derajat (beberapa servo) */
#define SERVO_MID_PULSE   1500  /* 1.5ms = 90 derajat */
#define SERVO_MAX_PULSE   2500  /* 2.5ms = 180 derajat */
#define SERVO_PERIOD      20000 /* 20ms periode (50Hz) */

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

/* ===================== Inisialisasi PWM TIM2 untuk Servo (50Hz) ===================== */
void MX_TIM2_Servo_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* F103: 72MHz / (72 * 20000) = 50Hz */
    htim2.Init.Prescaler = 71;
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* F4: 84MHz / (84 * 20000) = 50Hz */
    htim2.Init.Prescaler = 83;
#endif

    htim2.Instance = TIM2;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = SERVO_PERIOD - 1; /* 19999 */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = SERVO_MID_PULSE;  /* Mulai di tengah (90°) */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

/* ===================== Fungsi Set Sudut Servo ===================== */
void Servo_SetAngle(uint16_t angle) {
    if (angle > 180) angle = 180;

    /* Konversi sudut ke pulse width:
     * 0° = 500us, 180° = 2500us
     * pulse = 500 + (angle * 2000 / 180) */
    uint32_t pulse = SERVO_MIN_PULSE + ((uint32_t)angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_TIM2_Servo_Init();

    printf("\r\n=== PWM Servo Control (SG90) ===\r\n");
    printf("TIM2_CH1 pada PA0\r\n");
    printf("PWM 50Hz, Pulse: %d-%d us\r\n", SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    printf("Sweep: 0 -> 180 -> 0 derajat\r\n\r\n");

    uint16_t angle = 0;
    int8_t direction = 1;
    uint32_t sweep_count = 0;

    while (1) {
        /* Set posisi servo */
        Servo_SetAngle(angle);

        /* Cetak info setiap 10 derajat */
        if (angle % 30 == 0) {
            uint32_t pulse = SERVO_MIN_PULSE +
                           ((uint32_t)angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180);
            printf("[SERVO] Sudut=%3u deg | Pulse=%4lu us | Arah=%s\r\n",
                   angle, pulse,
                   (direction > 0) ? "CW " : "CCW");
        }

        /* Update sudut */
        if (direction > 0) {
            angle += 1;
            if (angle >= 180) {
                angle = 180;
                direction = -1;
                sweep_count++;
                printf("[SERVO] === Sweep #%lu: Mencapai 180 deg ===\r\n", sweep_count);
            }
        } else {
            if (angle > 0) {
                angle -= 1;
            } else {
                angle = 0;
                direction = 1;
                printf("[SERVO] === Sweep #%lu: Kembali ke 0 deg ===\r\n", sweep_count);
            }
        }

        HAL_Delay(15); /* ~15ms per derajat, total sweep ~2.7 detik */
    }
}
