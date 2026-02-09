/**
 * @file main.c
 * @brief STM32_09_GPIO_Port_Register - GPIO Register - Akses Register GPIO Langsung
 * 
 * FUNGSI: Direct register access (IDR, ODR, BSRR) vs HAL functions
 * Supported: STM32F103C8T6, STM32F401CCU6, STM32F411CEU6
 */



#include "config.h"

/* ========================= Function Prototypes =========================== */
void SystemClock_Config(void);
void Error_Handler(void);
void GPIO_Init(void);
void Demo_BSRR(void);
void Demo_ODR(void);
void Demo_IDR(void);
void Demo_Speed_Comparison(void);
void Print_Port_State(const char *label);

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

    /* Initialize LEDs using HAL (initial setup) */
    GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 GPIO Port Register Demo\r\n");
    printf("  LEDs on PA0-PA3 (direct register)\r\n");
    printf("========================================\r\n\r\n");

    while (1) {
        /* Demo 1: BSRR - Atomic Bit Set/Reset */
        printf("--- Demo 1: BSRR (Bit Set/Reset Register) ---\r\n");
        printf("BSRR provides ATOMIC pin control:\r\n");
        printf("  Lower 16 bits: write 1 to SET pin HIGH\r\n");
        printf("  Upper 16 bits: write 1 to RESET pin LOW\r\n\r\n");
        Demo_BSRR();
        HAL_Delay(DEMO_PAUSE_MS);

        /* Demo 2: ODR - Output Data Register */
        printf("--- Demo 2: ODR (Output Data Register) ---\r\n");
        printf("ODR allows read/write of entire port state.\r\n");
        printf("WARNING: ODR write is NOT atomic (read-modify-write)!\r\n\r\n");
        Demo_ODR();
        HAL_Delay(DEMO_PAUSE_MS);

        /* Demo 3: IDR - Input Data Register */
        printf("--- Demo 3: IDR (Input Data Register) ---\r\n");
        printf("IDR reflects actual pin levels (read-only).\r\n\r\n");
        Demo_IDR();
        HAL_Delay(DEMO_PAUSE_MS);

        /* Demo 4: Speed comparison */
        printf("--- Demo 4: Speed Comparison ---\r\n");
        printf("BSRR direct vs HAL_GPIO_WritePin()\r\n\r\n");
        Demo_Speed_Comparison();
        HAL_Delay(DEMO_PAUSE_MS);

        printf("\r\n===== Restarting demo cycle =====\r\n\r\n");
        HAL_Delay(DEMO_PAUSE_MS);
    }
}

/* ========================= GPIO Initialization =========================== */
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = LED_ALL_PINS;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* Start with all LEDs OFF */
    LED_PORT->BSRR = (uint32_t)LED_ALL_PINS << 16;  /* Reset all */
}

/* ========================= Demo: BSRR Register =========================== */
void Demo_BSRR(void) {
    /* Turn all LEDs OFF first */
    LED_PORT->BSRR = (uint32_t)LED_ALL_PINS << 16;
    Print_Port_State("Initial (all OFF)");
    HAL_Delay(PATTERN_DELAY_MS);

    /* Set PA0 HIGH using BSRR lower 16 bits */
    printf("  BSRR = 0x%04X (set PA0)\r\n", LED0_PIN);
    LED_PORT->BSRR = LED0_PIN;
    Print_Port_State("After SET PA0");
    HAL_Delay(PATTERN_DELAY_MS);

    /* Set PA1, PA2 HIGH simultaneously (atomic!) */
    printf("  BSRR = 0x%04X (set PA1+PA2)\r\n", LED1_PIN | LED2_PIN);
    LED_PORT->BSRR = LED1_PIN | LED2_PIN;
    Print_Port_State("After SET PA1+PA2");
    HAL_Delay(PATTERN_DELAY_MS);

    /* Reset PA0 using BSRR upper 16 bits */
    printf("  BSRR = 0x%08lX (reset PA0)\r\n", (unsigned long)((uint32_t)LED0_PIN << 16));
    LED_PORT->BSRR = (uint32_t)LED0_PIN << 16;
    Print_Port_State("After RESET PA0");
    HAL_Delay(PATTERN_DELAY_MS);

    /* Set PA3 and Reset PA1 in ONE atomic operation! */
    uint32_t bsrr_val = LED3_PIN | ((uint32_t)LED1_PIN << 16);
    printf("  BSRR = 0x%08lX (set PA3 + reset PA1 atomically)\r\n",
           (unsigned long)bsrr_val);
    LED_PORT->BSRR = bsrr_val;
    Print_Port_State("After SET PA3 + RESET PA1");
    HAL_Delay(PATTERN_DELAY_MS);

    /* Walking 1 using BSRR */
    printf("\r\n  Walking-1 pattern using BSRR:\r\n");
    for (int i = 0; i < LED_COUNT; i++) {
        uint16_t set_pin = (1 << i);
        uint32_t reset_others = (uint32_t)(LED_MASK & ~set_pin) << 16;
        LED_PORT->BSRR = set_pin | reset_others;
        printf("    BSRR = 0x%08lX -> ODR[3:0] = 0x%lX\r\n",
               (unsigned long)(set_pin | reset_others),
               (unsigned long)(LED_PORT->ODR & LED_MASK));
        HAL_Delay(PATTERN_DELAY_MS);
    }

    /* All OFF */
    LED_PORT->BSRR = (uint32_t)LED_ALL_PINS << 16;
    printf("\r\n");
}

/* ========================= Demo: ODR Register ============================ */
void Demo_ODR(void) {
    /* Binary count 0-15 using ODR (set lower nibble) */
    printf("  Binary count 0-15 using ODR:\r\n");
    for (uint8_t count = 0; count <= 15; count++) {
        /* Read ODR, mask off lower nibble, OR in new value */
        uint32_t odr = LED_PORT->ODR;
        odr = (odr & ~LED_MASK) | (count & LED_MASK);
        LED_PORT->ODR = odr;

        printf("    ODR = 0x%04lX -> PA3=%d PA2=%d PA1=%d PA0=%d\r\n",
               (unsigned long)(LED_PORT->ODR & LED_MASK),
               (count >> 3) & 1, (count >> 2) & 1,
               (count >> 1) & 1, (count >> 0) & 1);
        HAL_Delay(PATTERN_DELAY_MS);
    }

    /* All OFF */
    LED_PORT->ODR &= ~LED_MASK;
    printf("\r\n");
}

/* ========================= Demo: IDR Register ============================ */
void Demo_IDR(void) {
    /* Set a known pattern via BSRR, then read back via IDR */
    uint8_t patterns[] = {0x05, 0x0A, 0x0F, 0x09, 0x06, 0x00};
    int num_patterns = sizeof(patterns) / sizeof(patterns[0]);

    for (int p = 0; p < num_patterns; p++) {
        /* Set pattern using ODR */
        LED_PORT->ODR = (LED_PORT->ODR & ~LED_MASK) | (patterns[p] & LED_MASK);
        HAL_Delay(1); /* Small delay for pin to settle */

        /* Read back via IDR */
        uint32_t idr_val = LED_PORT->IDR & LED_MASK;
        uint32_t odr_val = LED_PORT->ODR & LED_MASK;

        printf("  Set 0x%02X -> ODR[3:0]=0x%lX, IDR[3:0]=0x%lX %s\r\n",
               patterns[p],
               (unsigned long)odr_val,
               (unsigned long)idr_val,
               (odr_val == idr_val) ? "(match)" : "(MISMATCH!)");
        HAL_Delay(PATTERN_DELAY_MS);
    }
    printf("\r\n");
}

/* ========================= Demo: Speed Comparison ======================== */
void Demo_Speed_Comparison(void) {
    uint32_t start, end;
    volatile uint32_t dummy;

    /* Benchmark 1: Direct BSRR register access */
    start = HAL_GetTick();
    for (uint32_t i = 0; i < BENCHMARK_ITERS; i++) {
        LED_PORT->BSRR = LED0_PIN;              /* Set HIGH */
        LED_PORT->BSRR = (uint32_t)LED0_PIN << 16; /* Set LOW  */
    }
    end = HAL_GetTick();
    printf("  BSRR direct: %lu iterations in %lu ms\r\n",
           (unsigned long)BENCHMARK_ITERS, (unsigned long)(end - start));

    /* Benchmark 2: HAL_GPIO_WritePin */
    start = HAL_GetTick();
    for (uint32_t i = 0; i < BENCHMARK_ITERS; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_PORT, LED0_PIN, GPIO_PIN_RESET);
    }
    end = HAL_GetTick();
    printf("  HAL_GPIO_WritePin: %lu iterations in %lu ms\r\n",
           (unsigned long)BENCHMARK_ITERS, (unsigned long)(end - start));

    /* Benchmark 3: HAL_GPIO_TogglePin */
    start = HAL_GetTick();
    for (uint32_t i = 0; i < BENCHMARK_ITERS; i++) {
        HAL_GPIO_TogglePin(LED_PORT, LED0_PIN);
        HAL_GPIO_TogglePin(LED_PORT, LED0_PIN);
    }
    end = HAL_GetTick();
    printf("  HAL_GPIO_TogglePin: %lu iterations in %lu ms\r\n",
           (unsigned long)BENCHMARK_ITERS, (unsigned long)(end - start));

    /* Benchmark 4: ODR read-modify-write */
    start = HAL_GetTick();
    for (uint32_t i = 0; i < BENCHMARK_ITERS; i++) {
        LED_PORT->ODR |= LED0_PIN;   /* Set HIGH (read-modify-write) */
        LED_PORT->ODR &= ~LED0_PIN;  /* Set LOW  (read-modify-write) */
    }
    end = HAL_GetTick();
    printf("  ODR read-modify-write: %lu iterations in %lu ms\r\n",
           (unsigned long)BENCHMARK_ITERS, (unsigned long)(end - start));

    printf("\r\n  NOTE: BSRR is fastest AND atomic (interrupt-safe).\r\n");
    printf("        ODR read-modify-write is NOT interrupt-safe!\r\n");
    printf("        HAL adds function call overhead but is portable.\r\n\r\n");

    /* Leave LED OFF */
    LED_PORT->BSRR = (uint32_t)LED0_PIN << 16;
    (void)dummy;
}

/* ========================= Print Port State =============================== */
void Print_Port_State(const char *label) {
    uint32_t odr = LED_PORT->ODR & LED_MASK;
    printf("  [%s] ODR[3:0]=0x%lX  PA3=%lu PA2=%lu PA1=%lu PA0=%lu\r\n",
           label,
           (unsigned long)odr,
           (unsigned long)((odr >> 3) & 1),
           (unsigned long)((odr >> 2) & 1),
           (unsigned long)((odr >> 1) & 1),
           (unsigned long)((odr >> 0) & 1));
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
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
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
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
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
