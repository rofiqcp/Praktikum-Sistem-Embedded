/**
 * ==========================================================
 *  Program     : PWM Motor Speed Control
 *  Modul       : 05 - DAC & PWM
 *  MCU         : STM32F103C8 / STM32F401CC / STM32F411CE
 *  Deskripsi   : Mengontrol kecepatan dan arah motor DC
 *                menggunakan driver L298N.
 *                PA0 = PWM (ENA), PA1 = IN1, PA2 = IN2.
 *                Ramp kecepatan naik-turun, toggle arah putaran.
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

/* ===================== Definisi Pin Motor ===================== */
/* PA0 = PWM (ENA via TIM2_CH1) */
/* PA1 = IN1 (arah motor) */
/* PA2 = IN2 (arah motor) */
#define MOTOR_IN1_PIN   GPIO_PIN_1
#define MOTOR_IN2_PIN   GPIO_PIN_2
#define MOTOR_PORT      GPIOA

/* ===================== Konstanta ===================== */
#define MOTOR_FORWARD   0
#define MOTOR_REVERSE   1
#define MOTOR_STOP      2
#define PWM_MAX         999

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

/* ===================== Inisialisasi GPIO Motor (IN1, IN2) ===================== */
void MX_Motor_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = MOTOR_IN1_PIN | MOTOR_IN2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTOR_PORT, &GPIO_InitStruct);

    /* Motor berhenti secara default */
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN2_PIN, GPIO_PIN_RESET);
}

/* ===================== Inisialisasi PWM TIM2_CH1 (PA0 = ENA) ===================== */
void MX_TIM2_PWM_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
#ifdef STM32F1
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    htim2.Init.Prescaler = 71;  /* 72MHz / 72 = 1MHz */
#elif defined(STM32F4)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    htim2.Init.Prescaler = 83;  /* 84MHz / 84 = 1MHz */
#endif

    /* PWM ~1kHz: 1MHz / 1000 = 1kHz */
    htim2.Instance = TIM2;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PWM_MAX;
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

/* ===================== Kontrol Arah Motor ===================== */
void Motor_SetDirection(uint8_t dir) {
    switch (dir) {
        case MOTOR_FORWARD:
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN2_PIN, GPIO_PIN_RESET);
            break;
        case MOTOR_REVERSE:
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN2_PIN, GPIO_PIN_SET);
            break;
        case MOTOR_STOP:
        default:
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_IN2_PIN, GPIO_PIN_RESET);
            break;
    }
}

/* ===================== Set Kecepatan Motor ===================== */
void Motor_SetSpeed(uint16_t speed) {
    if (speed > PWM_MAX) speed = PWM_MAX;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
}

/* ===================== Program Utama ===================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    MX_Motor_GPIO_Init();
    MX_TIM2_PWM_Init();

    printf("\r\n=== PWM Motor Speed Control ===\r\n");
    printf("Motor DC + L298N Driver\r\n");
    printf("PA0 = PWM (ENA), PA1 = IN1, PA2 = IN2\r\n");
    printf("Ramp kecepatan + toggle arah\r\n\r\n");

    uint8_t current_dir = MOTOR_FORWARD;
    uint32_t cycle = 0;

    while (1) {
        cycle++;
        const char *dir_str = (current_dir == MOTOR_FORWARD) ? "MAJU" : "MUNDUR";

        printf("--- Siklus #%lu: Arah=%s ---\r\n", cycle, dir_str);

        /* Set arah motor */
        Motor_SetDirection(current_dir);

        /* Ramp naik: 0% -> 100% */
        printf("[MOTOR] Ramp NAIK...\r\n");
        for (uint16_t speed = 0; speed <= PWM_MAX; speed += 50) {
            Motor_SetSpeed(speed);
            float persen = (speed / (float)PWM_MAX) * 100.0f;

            if (speed % 200 == 0) {
                printf("[MOTOR] Kecepatan=%3u/%d (%5.1f%%) | Arah=%s\r\n",
                       speed, PWM_MAX, persen, dir_str);
            }
            HAL_Delay(50);
        }

        /* Tahan kecepatan maksimum 2 detik */
        Motor_SetSpeed(PWM_MAX);
        printf("[MOTOR] Kecepatan MAKS: 100%% | Tahan 2 detik\r\n");
        HAL_Delay(2000);

        /* Ramp turun: 100% -> 0% */
        printf("[MOTOR] Ramp TURUN...\r\n");
        for (int16_t speed = PWM_MAX; speed >= 0; speed -= 50) {
            Motor_SetSpeed((uint16_t)speed);
            float persen = (speed / (float)PWM_MAX) * 100.0f;

            if (speed % 200 == 0) {
                printf("[MOTOR] Kecepatan=%3d/%d (%5.1f%%) | Arah=%s\r\n",
                       speed, PWM_MAX, persen, dir_str);
            }
            HAL_Delay(50);
        }

        /* Stop motor 1 detik */
        Motor_SetDirection(MOTOR_STOP);
        Motor_SetSpeed(0);
        printf("[MOTOR] STOP - Jeda 1 detik\r\n\r\n");
        HAL_Delay(1000);

        /* Toggle arah */
        current_dir = (current_dir == MOTOR_FORWARD) ? MOTOR_REVERSE : MOTOR_FORWARD;
    }
}
