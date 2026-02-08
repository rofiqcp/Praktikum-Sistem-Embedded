/**
 * ============================================================================
 * @file    main.c
 * @brief   STM32_10 - 4x4 Matrix Keypad Scanner with Debounce
 * @project STM32_10_GPIO_Matrix_Keypad
 * ============================================================================
 *
 * Description:
 *   Implements a row-column scanning algorithm for a 4x4 matrix keypad.
 *   Rows are configured as output push-pull (active-low scanning).
 *   Columns are configured as input with internal pull-up.
 *
 *   Scanning Algorithm:
 *     1. Set all rows HIGH (idle state)
 *     2. Drive one row LOW at a time
 *     3. Read all column pins
 *     4. A LOW column reading = key pressed at (row, col) intersection
 *     5. Look up key character from mapping table
 *     6. Debounce using HAL_GetTick() to prevent multiple triggers
 *
 * Key Map (standard 4x4 keypad):
 *         COL0   COL1   COL2   COL3
 *   ROW0   1      2      3      A
 *   ROW1   4      5      6      B
 *   ROW2   7      8      9      C
 *   ROW3   *      0      #      D
 *
 * Hardware:
 *   - 4x4 membrane keypad or tactile button matrix
 *   - Rows: PA0-PA3 (directly to keypad row pins)
 *   - Columns: PB0, PB1, PB3, PB4 (directly to keypad column pins)
 *     (PB2 skipped = BOOT1 pin on Blue Pill)
 *
 * Board Support: Blue Pill (F103C8), F401CC, F411CE
 *
 * ============================================================================
 */

#include "config.h"
#include <stdio.h>

/* ========================= Key Map ======================================= */
static const char keymap[NUM_ROWS][NUM_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* Row and column pin arrays for iteration */
static const uint16_t row_pins[NUM_ROWS] = {ROW0_PIN, ROW1_PIN, ROW2_PIN, ROW3_PIN};
static const uint16_t col_pins[NUM_COLS] = {COL0_PIN, COL1_PIN, COL2_PIN, COL3_PIN};

/* ========================= Function Prototypes =========================== */
void SystemClock_Config(void);
void Error_Handler(void);
void GPIO_Init(void);
char Keypad_Scan(void);
void Rows_Set_All_High(void);

/* ========================= Printf Redirect (ITM/SWO) ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

/* ========================= Debounce State ================================ */
static char     last_key       = KEY_NONE;
static uint32_t last_key_time  = 0;
static uint8_t  key_confirmed  = 0;

/* ========================= Main Program ================================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Enable GPIO port clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Initialize GPIO for keypad */
    GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 4x4 Matrix Keypad Scanner\r\n");
    printf("  Rows: PA0-PA3, Cols: PB0,1,3,4\r\n");
    printf("========================================\r\n\r\n");
    printf("Press keys on the keypad...\r\n\r\n");

    uint32_t key_count = 0;

    while (1) {
        char key = Keypad_Scan();

        if (key != KEY_NONE) {
            key_count++;
            printf("[%7lu ms] Key #%lu: '%c'",
                   HAL_GetTick(), (unsigned long)key_count, key);

            /* Additional info for special keys */
            if (key >= '0' && key <= '9') {
                printf("  (numeric: %d)", key - '0');
            } else if (key == '*') {
                printf("  (star/asterisk)");
            } else if (key == '#') {
                printf("  (hash/pound)");
            } else if (key >= 'A' && key <= 'D') {
                printf("  (function key %c)", key);
            }
            printf("\r\n");
        }

        HAL_Delay(SCAN_INTERVAL_MS);
    }
}

/* ========================= GPIO Initialization =========================== */
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure Row pins (PA0-PA3) as output push-pull, default HIGH */
    GPIO_InitStruct.Pin   = ROW_ALL_PINS;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ROW_PORT, &GPIO_InitStruct);

    /* Set all rows HIGH (idle) */
    Rows_Set_All_High();

    /* Configure Column pins (PB0, PB1, PB3, PB4) as input with pull-up */
    GPIO_InitStruct.Pin  = COL_ALL_PINS;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(COL_PORT, &GPIO_InitStruct);
}

/* ========================= Set All Rows HIGH ============================= */
void Rows_Set_All_High(void) {
    HAL_GPIO_WritePin(ROW_PORT, ROW_ALL_PINS, GPIO_PIN_SET);
}

/* ========================= Keypad Scan =================================== */
/**
 * @brief  Scan the 4x4 keypad matrix with debouncing.
 *         Drives each row LOW sequentially, reads column inputs.
 * @retval Detected key character, or KEY_NONE if no key pressed.
 */
char Keypad_Scan(void) {
    char detected_key = KEY_NONE;
    uint32_t now = HAL_GetTick();

    /* Scan each row */
    for (int row = 0; row < NUM_ROWS; row++) {
        /* Set all rows HIGH first */
        Rows_Set_All_High();

        /* Drive current row LOW */
        HAL_GPIO_WritePin(ROW_PORT, row_pins[row], GPIO_PIN_RESET);

        /* Small delay for signal to settle */
        /* Use a few NOP cycles instead of HAL_Delay for speed */
        for (volatile int d = 0; d < 20; d++) { __NOP(); }

        /* Read each column */
        for (int col = 0; col < NUM_COLS; col++) {
            if (HAL_GPIO_ReadPin(COL_PORT, col_pins[col]) == GPIO_PIN_RESET) {
                detected_key = keymap[row][col];
                break;
            }
        }

        if (detected_key != KEY_NONE) {
            break;
        }
    }

    /* Restore all rows HIGH */
    Rows_Set_All_High();

    /* Debounce logic */
    if (detected_key != KEY_NONE) {
        if (detected_key != last_key) {
            /* New key detected - start debounce timer */
            last_key      = detected_key;
            last_key_time = now;
            key_confirmed = 0;
            return KEY_NONE; /* Don't report yet */
        } else if (!key_confirmed && (now - last_key_time >= DEBOUNCE_MS)) {
            /* Same key held for debounce period - confirm it */
            key_confirmed = 1;
            return detected_key;
        }
        /* Key already confirmed, don't repeat */
        return KEY_NONE;
    } else {
        /* No key pressed - reset state */
        if (last_key != KEY_NONE) {
            last_key      = KEY_NONE;
            key_confirmed = 0;
        }
        return KEY_NONE;
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
