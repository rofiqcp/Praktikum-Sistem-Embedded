/**
 * ============================================================================
 * @file    main.c
 * @brief   STM32_12 - 8-LED Test Pattern Generator with Direct Port Write
 * @project STM32_12_LED_Test_Pattern
 * ============================================================================
 *
 * Description:
 *   Generates various LED test patterns using 8 LEDs connected to PA0-PA7.
 *   Uses direct port writes (GPIOA->ODR) for efficient simultaneous control
 *   of all 8 LEDs in a single bus write cycle.
 *
 *   Test Patterns:
 *     0. ALL ON     - All 8 LEDs lit (0xFF)
 *     1. ALL OFF    - All 8 LEDs off (0x00)
 *     2. WALKING 1  - Single LED walks left: 0x01->0x02->0x04->...->0x80
 *     3. WALKING 0  - Single LED off walks left: 0xFE->0xFD->0xFB->...->0x7F
 *     4. BINARY CNT - Binary counter 0x00 to 0xFF
 *     5. ALTERNATING- Alternating pattern: 0x55 <-> 0xAA (checkerboard)
 *
 *   This is commonly used for:
 *     - PCB bring-up and LED connectivity testing
 *     - Visual verification of GPIO port wiring
 *     - Manufacturing test sequences
 *
 * Hardware:
 *   - 8x LEDs with 220 ohm resistors on PA0 through PA7
 *   - Common cathode to GND
 *
 * Wiring:
 *   PA0 ---[220R]--- LED0 --- GND   (bit 0, LSB)
 *   PA1 ---[220R]--- LED1 --- GND
 *   PA2 ---[220R]--- LED2 --- GND
 *   PA3 ---[220R]--- LED3 --- GND
 *   PA4 ---[220R]--- LED4 --- GND
 *   PA5 ---[220R]--- LED5 --- GND
 *   PA6 ---[220R]--- LED6 --- GND
 *   PA7 ---[220R]--- LED7 --- GND   (bit 7, MSB)
 *
 * Board Support: Blue Pill (F103C8), F401CC, F411CE
 *
 * ============================================================================
 */

#include "config.h"
#include <stdio.h>

/* ========================= Function Prototypes =========================== */
void SystemClock_Config(void);
void Error_Handler(void);
void GPIO_Init(void);
void Set_LEDs(uint8_t pattern);
void Pattern_AllOn(void);
void Pattern_AllOff(void);
void Pattern_WalkingOne(void);
void Pattern_WalkingZero(void);
void Pattern_BinaryCount(void);
void Pattern_Alternating(void);
const char *Pattern_Name(uint8_t pattern_id);

/* ========================= Printf Redirect (ITM/SWO) ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

/* ========================= Pattern Function Table ======================== */
typedef void (*PatternFunc_t)(void);

static const PatternFunc_t pattern_funcs[NUM_PATTERNS] = {
    Pattern_AllOn,
    Pattern_AllOff,
    Pattern_WalkingOne,
    Pattern_WalkingZero,
    Pattern_BinaryCount,
    Pattern_Alternating
};

/* ========================= Main Program ================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Enable GPIO port clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Initialize LED GPIO */
    GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 8-LED Test Pattern Generator\r\n");
    printf("  LEDs on PA0-PA7 (direct ODR write)\r\n");
    printf("========================================\r\n\r\n");

    uint32_t cycle = 0;

    while (1) {
        cycle++;
        printf("===== Test Cycle #%lu =====\r\n\r\n", (unsigned long)cycle);

        for (uint8_t p = 0; p < NUM_PATTERNS; p++) {
            printf("[Pattern %d] %s\r\n", p, Pattern_Name(p));

            for (uint8_t rep = 0; rep < PATTERN_CYCLES; rep++) {
                pattern_funcs[p]();
            }

            /* All LEDs off between patterns */
            Set_LEDs(0x00);
            HAL_Delay(PATTERN_PAUSE_MS);
        }

        printf("\r\n");
    }
}

/* ========================= GPIO Initialization =========================== */
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure PA0-PA7 as output push-pull */
    GPIO_InitStruct.Pin   = LED_ALL_PINS;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* All LEDs off initially */
    Set_LEDs(0x00);
}

/* ========================= Set LEDs via ODR ============================== */
/**
 * @brief  Set all 8 LEDs simultaneously using direct port write.
 *         Preserves upper byte of GPIOA->ODR (PA8-PA15).
 * @param  pattern: 8-bit pattern where bit N controls LED N (PA N)
 */
void Set_LEDs(uint8_t pattern) {
    uint32_t odr = LED_PORT->ODR;
    odr = (odr & ~LED_MASK) | ((uint32_t)pattern & LED_MASK);
    LED_PORT->ODR = odr;
}

/* ========================= Pattern: All ON =============================== */
void Pattern_AllOn(void) {
    printf("  -> 0xFF (all ON)\r\n");
    Set_LEDs(0xFF);
    HAL_Delay(STEP_DELAY_MS * 4);
}

/* ========================= Pattern: All OFF ============================== */
void Pattern_AllOff(void) {
    printf("  -> 0x00 (all OFF)\r\n");
    Set_LEDs(0x00);
    HAL_Delay(STEP_DELAY_MS * 4);
}

/* ========================= Pattern: Walking One ========================== */
void Pattern_WalkingOne(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t val = (1 << i);
        Set_LEDs(val);
        printf("  -> 0x%02X (LED%d on)\r\n", val, i);
        HAL_Delay(STEP_DELAY_MS);
    }
    /* Walk back */
    for (int i = LED_COUNT - 2; i >= 0; i--) {
        uint8_t val = (1 << i);
        Set_LEDs(val);
        printf("  -> 0x%02X (LED%d on)\r\n", val, i);
        HAL_Delay(STEP_DELAY_MS);
    }
}

/* ========================= Pattern: Walking Zero ========================= */
void Pattern_WalkingZero(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t val = ~(1 << i) & 0xFF;
        Set_LEDs(val);
        printf("  -> 0x%02X (LED%d off)\r\n", val, i);
        HAL_Delay(STEP_DELAY_MS);
    }
    /* Walk back */
    for (int i = LED_COUNT - 2; i >= 0; i--) {
        uint8_t val = ~(1 << i) & 0xFF;
        Set_LEDs(val);
        printf("  -> 0x%02X (LED%d off)\r\n", val, i);
        HAL_Delay(STEP_DELAY_MS);
    }
}

/* ========================= Pattern: Binary Count ========================= */
void Pattern_BinaryCount(void) {
    /* Count 0 to 255 (0x00 to 0xFF) */
    for (uint16_t count = 0; count <= 255; count++) {
        Set_LEDs((uint8_t)count);
        /* Print every 16th value to reduce log spam */
        if ((count % 16) == 0) {
            printf("  -> 0x%02X (%3d)\r\n", (uint8_t)count, (uint8_t)count);
        }
        HAL_Delay(STEP_DELAY_MS / 4);  /* Faster for 256 steps */
    }
    printf("  -> Count complete (0-255)\r\n");
}

/* ========================= Pattern: Alternating ========================== */
void Pattern_Alternating(void) {
    for (int i = 0; i < 8; i++) {
        if (i % 2 == 0) {
            Set_LEDs(0x55);  /* 01010101 */
            printf("  -> 0x55 (alternating A)\r\n");
        } else {
            Set_LEDs(0xAA);  /* 10101010 */
            printf("  -> 0xAA (alternating B)\r\n");
        }
        HAL_Delay(STEP_DELAY_MS);
    }
}

/* ========================= Pattern Name Lookup =========================== */
const char *Pattern_Name(uint8_t pattern_id) {
    switch (pattern_id) {
        case PATTERN_ALL_ON:     return "ALL ON";
        case PATTERN_ALL_OFF:    return "ALL OFF";
        case PATTERN_WALK_ONE:   return "WALKING ONE (knight rider)";
        case PATTERN_WALK_ZERO:  return "WALKING ZERO (inverse)";
        case PATTERN_BINARY_CNT: return "BINARY COUNTER (0-255)";
        case PATTERN_ALTERNATE:  return "ALTERNATING (checkerboard)";
        default:                 return "UNKNOWN";
    }
}

/* ========================= System Clock Configuration ==================== */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef STM32F103xB
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
