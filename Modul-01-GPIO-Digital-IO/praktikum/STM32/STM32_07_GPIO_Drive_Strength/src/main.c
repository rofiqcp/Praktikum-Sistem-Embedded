/**
 * @file main.c
 * @brief STM32_07_GPIO_Drive_Strength - GPIO Drive Strength - Konfigurasi Kekuatan GPIO
 * 
 * FUNGSI: Testing drive strength berbagai level dan karakteristik beban
 * Supported: STM32F103C8T6, STM32F401CCU6, STM32F411CEU6
 */



#include "config.h"
#include <string.h>

/* ========================= Function Prototypes =========================== */
void SystemClock_Config(void);
void Error_Handler(void);
void GPIO_Init_WithSpeed(uint32_t speed);
const char *Speed_To_String(uint32_t speed);

/* ========================= Printf Redirect (ITM/SWO) ==================== */
    return len;
}

/* ========================= Main Program ================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Enable GPIO port clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Speed settings to cycle through */
    uint32_t speeds[] = {
        GPIO_SPEED_FREQ_LOW,
        GPIO_SPEED_FREQ_MEDIUM,
        GPIO_SPEED_FREQ_HIGH,
#if defined(STM32F401xC) || defined(STM32F411xE)
        GPIO_SPEED_FREQ_VERY_HIGH,
#endif
    };
    uint8_t num_speeds = sizeof(speeds) / sizeof(speeds[0]);
    uint8_t speed_idx = 0;

    /* Initialize LED with first speed */
    GPIO_Init_WithSpeed(speeds[speed_idx]);

    printf("\r\n========================================\r\n");
    printf("  STM32 GPIO Output Speed Demo\r\n");
    printf("  LED on P%c%d\r\n", 'A', LED_PIN_NUM);
    printf("========================================\r\n\r\n");
    printf("[INFO] Starting with speed: %s\r\n", Speed_To_String(speeds[speed_idx]));

    uint32_t last_switch = HAL_GetTick();
    uint32_t last_toggle = HAL_GetTick();

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Toggle LED at fixed rate to demonstrate output behavior */
        if (now - last_toggle >= LED_TOGGLE_MS) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            last_toggle = now;
        }

        /* Switch speed setting every SPEED_SWITCH_MS */
        if (now - last_switch >= SPEED_SWITCH_MS) {
            speed_idx = (speed_idx + 1) % num_speeds;
            GPIO_Init_WithSpeed(speeds[speed_idx]);

            printf("[%7lu ms] Speed changed -> %s\r\n",
                   now, Speed_To_String(speeds[speed_idx]));

            /* Show register dump for educational purposes */
#if defined(STM32F401xC) || defined(STM32F411xE)
            printf("           GPIOA->OSPEEDR = 0x%08lX\r\n",
                   (unsigned long)LED_PORT->OSPEEDR);
#elif defined(STM32F103xC)
            printf("           GPIOA->CRL = 0x%08lX\r\n",
                   (unsigned long)LED_PORT->CRL);
#endif
            last_switch = now;
        }
    }
}

/* ========================= GPIO Initialization ========================== */
void GPIO_Init_WithSpeed(uint32_t speed) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = speed;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/* ========================= Speed to String =============================== */
const char *Speed_To_String(uint32_t speed) {
    switch (speed) {
        case GPIO_SPEED_FREQ_LOW:
            return "LOW (2 MHz slew rate)";
        case GPIO_SPEED_FREQ_MEDIUM:
#ifdef STM32F103xC
            return "MEDIUM (10 MHz slew rate)";
#else
            return "MEDIUM (25 MHz slew rate)";
#endif
        case GPIO_SPEED_FREQ_HIGH:
            return "HIGH (50 MHz slew rate)";
#if defined(STM32F401xC) || defined(STM32F411xE)
        case GPIO_SPEED_FREQ_VERY_HIGH:
            return "VERY_HIGH (100 MHz slew rate)";
#endif
        default:
            return "UNKNOWN";
    }
}

/* ========================= System Clock Configuration ==================== */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef STM32F103xC
    /*--- F103: 8MHz HSE -> PLL x9 -> 72MHz SYSCLK ---*/
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1 = 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   /* APB2 = 72MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

#elif defined(STM32F401xC) || defined(STM32F411xE)
    /*--- F401/F411: 25MHz HSE -> PLL -> 84MHz SYSCLK ---*/
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 25;
    RCC_OscInitStruct.PLL.PLLN       = 336;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;   /* 336/4 = 84MHz */
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1 = 42MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   /* APB2 = 84MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

#else
    /*--- Fallback: HSI ---*/
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
#endif
}

/* ========================= Error Handler ================================= */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        /* Stay here on error */
    }
}
