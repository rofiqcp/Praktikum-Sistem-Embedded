/**
 * ==========================================================================
 *  ESP32_10 - 4×4 Matrix Keypad Scanner
 * ==========================================================================
 *  Pin definitions for 4×4 matrix keypad row-column scanning
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- Matrix dimensions ---------- */
#define KEYPAD_ROWS     4
#define KEYPAD_COLS     4

/* ===== Board-specific pin mapping ===== */
#if defined(CONFIG_IDF_TARGET_ESP32)
    /* Row pins → OUTPUT (active-low scan) */
    #define ROW0_PIN    GPIO_NUM_16
    #define ROW1_PIN    GPIO_NUM_17
    #define ROW2_PIN    GPIO_NUM_18
    #define ROW3_PIN    GPIO_NUM_19

    /* Column pins → INPUT with pull-up */
    /* Using GPIO 32-35 (input-capable; 34/35 need external pull-up) */
    #define COL0_PIN    GPIO_NUM_32
    #define COL1_PIN    GPIO_NUM_33
    #define COL2_PIN    GPIO_NUM_34      /* input-only, external pull-up! */
    #define COL3_PIN    GPIO_NUM_35      /* input-only, external pull-up! */

#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    #define ROW0_PIN    GPIO_NUM_33
    #define ROW1_PIN    GPIO_NUM_34
    #define ROW2_PIN    GPIO_NUM_35
    #define ROW3_PIN    GPIO_NUM_36

    #define COL0_PIN    GPIO_NUM_37
    #define COL1_PIN    GPIO_NUM_38
    #define COL2_PIN    GPIO_NUM_39
    #define COL3_PIN    GPIO_NUM_40

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    #define ROW0_PIN    GPIO_NUM_16
    #define ROW1_PIN    GPIO_NUM_17
    #define ROW2_PIN    GPIO_NUM_18
    #define ROW3_PIN    GPIO_NUM_8

    #define COL0_PIN    GPIO_NUM_3
    #define COL1_PIN    GPIO_NUM_9
    #define COL2_PIN    GPIO_NUM_10
    #define COL3_PIN    GPIO_NUM_11

#else
    #error "Unsupported target – add pin mapping for your chip"
#endif

/* ---------- Debounce ---------- */
#define DEBOUNCE_MS         50      /* Debounce time in milliseconds      */
#define SCAN_INTERVAL_MS    10      /* Time between row scans             */
#define ROW_SETTLE_US       5       /* Microseconds to let row settle     */

#endif /* CONFIG_H */
