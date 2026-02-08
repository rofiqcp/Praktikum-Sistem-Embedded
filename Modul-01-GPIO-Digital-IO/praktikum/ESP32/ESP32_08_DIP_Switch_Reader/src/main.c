/**
 * ==========================================================================
 *  ESP32_08 - DIP Switch Reader (Parallel Input with Bit Masking)
 * ==========================================================================
 *  Reads 4 (or 8) DIP switches connected to GPIO pins, combines them into
 *  a single byte value using bit masking and shift operations, and displays
 *  the result in binary, hexadecimal, and decimal (interpreted as address).
 *
 *  Hardware:
 *    - 4x (or 8x) DIP switch
 *    - 4x (or 8x) 10 kΩ pull-up resistors to 3.3 V
 *      (especially required for GPIO34/35 which lack internal pull-ups)
 *
 *  Wiring (per switch – active LOW):
 *    3.3V ── 10kΩ ──┬── GPIOxx
 *                    │
 *                  [SW] ── GND
 *    Switch ON  → pin reads LOW  (0)
 *    Switch OFF → pin reads HIGH (1)  → inverted in software
 *
 *  Pin assignments (ESP32 classic):
 *    Bit 0 (LSB) → GPIO32
 *    Bit 1       → GPIO33
 *    Bit 2       → GPIO34  (input-only)
 *    Bit 3 (MSB) → GPIO35  (input-only)
 *
 *  API Used:
 *    - gpio_config()
 *    - gpio_get_level()
 *    - Bit masking: |= and <<
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "DIP_SW";

/* Array of switch GPIO pins in bit order (index = bit position) */
static const gpio_num_t sw_pins[] = {
    SW_BIT0_PIN,
    SW_BIT1_PIN,
    SW_BIT2_PIN,
    SW_BIT3_PIN,
#if DIP_SWITCH_BITS > 4
    SW_BIT4_PIN,
    SW_BIT5_PIN,
    SW_BIT6_PIN,
    SW_BIT7_PIN,
#endif
};

/**
 * @brief Configure all DIP switch GPIOs as inputs.
 *        Internal pull-ups are enabled where the hardware supports it.
 *        GPIO34/35 on ESP32 classic REQUIRE external pull-ups.
 */
static void gpio_init_switches(void)
{
    for (int i = 0; i < DIP_SWITCH_BITS; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << sw_pins[i]),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,   /* ignored on 34/35 */
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        ESP_LOGI(TAG, "  Bit %d → GPIO%d configured as input", i, sw_pins[i]);
    }
}

/**
 * @brief Read all switch pins and combine into a byte using bit masking.
 *        Switches are active-LOW, so we invert the reading.
 *
 * @return uint8_t  Combined value (bit 0 = SW0, bit 1 = SW1, …)
 */
static uint8_t read_dip_switches(void)
{
    uint8_t value = 0;

    for (int i = 0; i < DIP_SWITCH_BITS; i++) {
        int level = gpio_get_level(sw_pins[i]);

        /*
         * Switch ON  → pulls to GND → level=0 → we want bit=1
         * Switch OFF → pulled HIGH  → level=1 → we want bit=0
         * So invert: bit = !level
         */
        uint8_t bit = (level == 0) ? 1 : 0;

        /* Place this bit in the correct position using shift & OR */
        value |= (bit << i);
    }

    return value;
}

/**
 * @brief Convert a byte to binary string representation.
 *
 * @param val   Value to convert
 * @param bits  Number of bits to show
 * @param buf   Output buffer (must be at least bits+1 bytes)
 */
static void byte_to_binary_str(uint8_t val, int bits, char *buf)
{
    for (int i = bits - 1; i >= 0; i--) {
        *buf++ = (val & (1 << i)) ? '1' : '0';
    }
    *buf = '\0';
}

void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " ESP32_08 - DIP Switch Reader");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "Number of bits: %d", DIP_SWITCH_BITS);
    ESP_LOGI(TAG, "Value range   : 0 – %d", (1 << DIP_SWITCH_BITS) - 1);
    ESP_LOGI(TAG, "");

    gpio_init_switches();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Reading DIP switches every %d ms …", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "(Switch ON = bit 1, Switch OFF = bit 0)");
    ESP_LOGI(TAG, "");

    uint8_t prev_value = 0xFF;  /* impossible initial → always print first */
    char bin_str[9];

    while (1) {
        uint8_t value = read_dip_switches();

        /* Only log when value changes to keep output clean */
        if (value != prev_value) {
            byte_to_binary_str(value, DIP_SWITCH_BITS, bin_str);

            ESP_LOGI(TAG, "┌────────────────────────────────────┐");
            ESP_LOGI(TAG, "│  DIP Switch Value Changed          │");
            ESP_LOGI(TAG, "├────────────────────────────────────┤");
            ESP_LOGI(TAG, "│  Binary : 0b%s                %s",
                     bin_str, DIP_SWITCH_BITS == 4 ? "    │" : "│");
            ESP_LOGI(TAG, "│  Hex    : 0x%02X                     │", value);
            ESP_LOGI(TAG, "│  Decimal: %-3d                      │", value);
            ESP_LOGI(TAG, "│  Address: Device #%d               %s",
                     value, value < 10 ? " │" : "│");
            ESP_LOGI(TAG, "├────────────────────────────────────┤");

            /* Show individual switch states */
            for (int i = DIP_SWITCH_BITS - 1; i >= 0; i--) {
                ESP_LOGI(TAG, "│  SW%d (GPIO%2d) = %s              │",
                         i, sw_pins[i],
                         (value & (1 << i)) ? "ON " : "OFF");
            }
            ESP_LOGI(TAG, "└────────────────────────────────────┘");

            prev_value = value;
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
