/**
 * ============================================================================
 * @file    config.h
 * @brief   Configuration for 4x4 Matrix Keypad Scanner
 * @project STM32_10_GPIO_Matrix_Keypad
 * ============================================================================
 *
 * Hardware Configuration:
 *   - 4 Row pins (output push-pull):  PA0, PA1, PA2, PA3
 *   - 4 Column pins (input pull-up):  PB0, PB1, PB3, PB4
 *     (PB2 avoided = BOOT1 pin on Blue Pill)
 *   - Standard 4x4 membrane keypad
 *
 * Wiring Diagram:
 *           COL0(PB0)  COL1(PB1)  COL2(PB3)  COL3(PB4)
 *   ROW0(PA0)  [1]        [2]        [3]        [A]
 *   ROW1(PA1)  [4]        [5]        [6]        [B]
 *   ROW2(PA2)  [7]        [8]        [9]        [C]
 *   ROW3(PA3)  [*]        [0]        [#]        [D]
 *
 * Scanning Method:
 *   1. Set all rows HIGH (idle)
 *   2. Set one row LOW at a time
 *   3. Read column pins (LOW = key pressed at intersection)
 *   4. Debounce with HAL_GetTick()
 *
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported STM32 target. Define STM32F103xB, STM32F401xC, or STM32F411xE"
#endif

/* ========================= Row Configuration (Output) ================ */
#define ROW_PORT        GPIOA
#define ROW0_PIN        GPIO_PIN_0
#define ROW1_PIN        GPIO_PIN_1
#define ROW2_PIN        GPIO_PIN_2
#define ROW3_PIN        GPIO_PIN_3
#define ROW_ALL_PINS    (ROW0_PIN | ROW1_PIN | ROW2_PIN | ROW3_PIN)

#define NUM_ROWS        4

/* ========================= Column Configuration (Input) ============== */
#define COL_PORT        GPIOB
#define COL0_PIN        GPIO_PIN_0
#define COL1_PIN        GPIO_PIN_1
#define COL2_PIN        GPIO_PIN_3      /* Skip PB2 = BOOT1 */
#define COL3_PIN        GPIO_PIN_4
#define COL_ALL_PINS    (COL0_PIN | COL1_PIN | COL2_PIN | COL3_PIN)

#define NUM_COLS        4

/* ========================= Debounce Configuration ==================== */
#define DEBOUNCE_MS         50      /* Debounce time (ms)                */
#define SCAN_INTERVAL_MS    10      /* Keypad scan interval (ms)         */

/* ========================= Key Map ===================================
 * Standard 4x4 keypad layout:
 *   1 2 3 A
 *   4 5 6 B
 *   7 8 9 C
 *   * 0 # D
 * ===================================================================== */
#define KEY_NONE        '\0'

#endif /* CONFIG_H */
