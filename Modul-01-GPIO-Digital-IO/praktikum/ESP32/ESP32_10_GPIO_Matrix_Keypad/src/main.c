/**
 * ==========================================================================
 *  ESP32_10 - 4×4 Matrix Keypad Scanner
 * ==========================================================================
 *  Implements row-column scanning for a standard 4×4 matrix keypad.
 *
 *  Scanning algorithm:
 *    1. Set ALL rows HIGH (idle)
 *    2. Pull ONE row LOW
 *    3. Read ALL column pins
 *    4. Any column reading LOW → key at that [row,col] is pressed
 *    5. Repeat for next row
 *    6. Debounce: only accept key if stable for DEBOUNCE_MS
 *
 *  Hardware:
 *    - 1x 4×4 matrix keypad module
 *    - 4x 10 kΩ external pull-up resistors on column pins
 *      (required for GPIO34/35 on ESP32 classic)
 *
 *  Wiring:
 *    Keypad R1 → GPIO16 (ROW0, output)
 *    Keypad R2 → GPIO17 (ROW1, output)
 *    Keypad R3 → GPIO18 (ROW2, output)
 *    Keypad R4 → GPIO19 (ROW3, output)
 *    Keypad C1 → GPIO32 (COL0, input + pull-up)
 *    Keypad C2 → GPIO33 (COL1, input + pull-up)
 *    Keypad C3 → GPIO34 (COL2, input + ext pull-up)
 *    Keypad C4 → GPIO35 (COL3, input + ext pull-up)
 *
 *  Key Map:
 *    ┌─────┬─────┬─────┬─────┐
 *    │  1  │  2  │  3  │  A  │
 *    ├─────┼─────┼─────┼─────┤
 *    │  4  │  5  │  6  │  B  │
 *    ├─────┼─────┼─────┼─────┤
 *    │  7  │  8  │  9  │  C  │
 *    ├─────┼─────┼─────┼─────┤
 *    │  *  │  0  │  #  │  D  │
 *    └─────┴─────┴─────┴─────┘
 *
 *  API Used:
 *    - gpio_config(), gpio_set_level(), gpio_get_level()
 *    - esp_rom_delay_us() for sub-ms settling time
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"        /* esp_rom_delay_us() */
#include "config.h"

static const char *TAG = "KEYPAD";

/* ---- Pin arrays ---- */
static const gpio_num_t row_pins[KEYPAD_ROWS] = {
    ROW0_PIN, ROW1_PIN, ROW2_PIN, ROW3_PIN
};

static const gpio_num_t col_pins[KEYPAD_COLS] = {
    COL0_PIN, COL1_PIN, COL2_PIN, COL3_PIN
};

/* ---- Key map lookup table ---- */
static const char key_map[KEYPAD_ROWS][KEYPAD_COLS] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

/* ---- Debounce state ---- */
static char     last_key        = '\0';
static int64_t  last_key_time   = 0;
static bool     key_reported    = false;

/* ------------------------------------------------------------------ */
/*  GPIO initialisation                                                */
/* ------------------------------------------------------------------ */
static void gpio_init_keypad(void)
{
    /* Row pins: output, default HIGH */
    for (int r = 0; r < KEYPAD_ROWS; r++) {
        gpio_config_t row_conf = {
            .pin_bit_mask = (1ULL << row_pins[r]),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&row_conf));
        gpio_set_level(row_pins[r], 1);     /* idle HIGH */
        ESP_LOGI(TAG, "  Row %d → GPIO%d (output)", r, row_pins[r]);
    }

    /* Column pins: input with pull-up */
    for (int c = 0; c < KEYPAD_COLS; c++) {
        gpio_config_t col_conf = {
            .pin_bit_mask = (1ULL << col_pins[c]),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,    /* no effect on 34/35 */
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&col_conf));
        ESP_LOGI(TAG, "  Col %d → GPIO%d (input + pull-up)", c, col_pins[c]);
    }
}

/* ------------------------------------------------------------------ */
/*  Scan the keypad – returns detected key or '\0' if none             */
/* ------------------------------------------------------------------ */
static char scan_keypad(void)
{
    char detected = '\0';

    for (int r = 0; r < KEYPAD_ROWS; r++) {
        /* Pull current row LOW */
        gpio_set_level(row_pins[r], 0);

        /* Short delay for electrical settling */
        esp_rom_delay_us(ROW_SETTLE_US);

        /* Read each column */
        for (int c = 0; c < KEYPAD_COLS; c++) {
            if (gpio_get_level(col_pins[c]) == 0) {
                /* Key press detected at [row, col] */
                detected = key_map[r][c];
            }
        }

        /* Restore row to HIGH */
        gpio_set_level(row_pins[r], 1);

        /* If we found a key, no need to keep scanning */
        if (detected != '\0') {
            break;
        }
    }

    return detected;
}

/* ------------------------------------------------------------------ */
/*  Debounced key handler                                              */
/* ------------------------------------------------------------------ */
static void handle_key(char key)
{
    int64_t now = esp_timer_get_time() / 1000;  /* ms */

    if (key == '\0') {
        /* No key pressed → reset state */
        if (last_key != '\0') {
            last_key     = '\0';
            key_reported = false;
        }
        return;
    }

    if (key != last_key) {
        /* New key → start debounce timer */
        last_key      = key;
        last_key_time = now;
        key_reported  = false;
        return;
    }

    /* Same key held – check if debounce period has passed */
    if (!key_reported && (now - last_key_time >= DEBOUNCE_MS)) {
        key_reported = true;

        /* Find row/col for extra info */
        int row = -1, col = -1;
        for (int r = 0; r < KEYPAD_ROWS && row == -1; r++) {
            for (int c = 0; c < KEYPAD_COLS; c++) {
                if (key_map[r][c] == key) {
                    row = r;
                    col = c;
                    break;
                }
            }
        }

        ESP_LOGI(TAG, "┌────────────────────────────┐");
        ESP_LOGI(TAG, "│  Key Pressed: '%c'           │", key);
        ESP_LOGI(TAG, "│  Position  : R%d × C%d        │", row, col);
        ESP_LOGI(TAG, "└────────────────────────────┘");
    }
}

/* ================================================================== */
void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " ESP32_10 - 4×4 Matrix Keypad Scanner");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "Key Map:");
    ESP_LOGI(TAG, "  ┌───┬───┬───┬───┐");
    ESP_LOGI(TAG, "  │ 1 │ 2 │ 3 │ A │  Row 0");
    ESP_LOGI(TAG, "  ├───┼───┼───┼───┤");
    ESP_LOGI(TAG, "  │ 4 │ 5 │ 6 │ B │  Row 1");
    ESP_LOGI(TAG, "  ├───┼───┼───┼───┤");
    ESP_LOGI(TAG, "  │ 7 │ 8 │ 9 │ C │  Row 2");
    ESP_LOGI(TAG, "  ├───┼───┼───┼───┤");
    ESP_LOGI(TAG, "  │ * │ 0 │ # │ D │  Row 3");
    ESP_LOGI(TAG, "  └───┴───┴───┴───┘");
    ESP_LOGI(TAG, "");

    gpio_init_keypad();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Scanning keypad … press any key");
    ESP_LOGI(TAG, "");

    while (1) {
        char key = scan_keypad();
        handle_key(key);
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}
