/**
 * @file main.c
 * @brief Program 08: DIP Switch Reader (Multiple Inputs) (ESP-IDF)
 *
 * Membaca 4-bit DIP switch dan menampilkan nilai dalam
 * format binary, decimal, dan hexadecimal.
 * Menggunakan ESP-IDF native GPIO API.
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "DIP_SWITCH";

/* ==================== KONFIGURASI ==================== */
#define NUM_SWITCHES    4
#define SW1_PIN         32      /* Bit 0 (LSB) */
#define SW2_PIN         33      /* Bit 1 */
#define SW3_PIN         25      /* Bit 2 */
#define SW4_PIN         26      /* Bit 3 (MSB) */

#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO 2       /* Status LED */
#endif

static const gpio_num_t SWITCH_PINS[NUM_SWITCHES] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};

/* ==================== VARIABEL ==================== */
static uint8_t last_value = 0xFF;

/* ==================== FUNCTIONS ==================== */

static uint8_t read_dip_switch(void)
{
    uint8_t value = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        /* Inverted karena pull-up: ON=LOW=1, OFF=HIGH=0 */
        if (gpio_get_level(SWITCH_PINS[i]) == 0) {
            value |= (1 << i);
        }
    }
    return value;
}

static void print_binary(uint8_t value)
{
    char buf[5];
    for (int i = NUM_SWITCHES - 1; i >= 0; i--) {
        buf[NUM_SWITCHES - 1 - i] = ((value >> i) & 1) ? '1' : '0';
    }
    buf[NUM_SWITCHES] = '\0';
    printf("B%s", buf);
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 08: DIP Switch Reader");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi switch pins (input + pull-up) */
    uint64_t sw_mask = 0;
    for (int i = 0; i < NUM_SWITCHES; i++) {
        sw_mask |= (1ULL << SWITCH_PINS[i]);
    }

    gpio_config_t sw_conf = {
        .pin_bit_mask = sw_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sw_conf);

    /* Konfigurasi LED pin (output) */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << CONFIG_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    ESP_LOGI(TAG, "DIP Switch Configuration:");
    ESP_LOGI(TAG, "  SW1 (Bit 0): GPIO%d", SW1_PIN);
    ESP_LOGI(TAG, "  SW2 (Bit 1): GPIO%d", SW2_PIN);
    ESP_LOGI(TAG, "  SW3 (Bit 2): GPIO%d", SW3_PIN);
    ESP_LOGI(TAG, "  SW4 (Bit 3): GPIO%d", SW4_PIN);
    ESP_LOGI(TAG, "Switch ON = 0 (grounded)");
    ESP_LOGI(TAG, "Switch OFF = 1 (pull-up)");
    ESP_LOGI(TAG, "Monitoring... (output on change)");

    while (1) {
        uint8_t current_value = read_dip_switch();

        if (current_value != last_value) {
            printf("DIP Switch: ");
            print_binary(current_value);
            printf("  Dec: %2d  Hex: 0x%X\n", current_value, current_value);

            /* LED indicator */
            gpio_set_level(CONFIG_LED_GPIO, (current_value > 0) ? 1 : 0);

            /* Interpretasi */
            switch (current_value) {
                case 0:
                    ESP_LOGI(TAG, "  -> All OFF");
                    break;
                case 15:
                    ESP_LOGI(TAG, "  -> All ON (Max value)");
                    break;
                default:
                    ESP_LOGI(TAG, "  -> Mode %d selected", current_value);
                    break;
            }

            last_value = current_value;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
