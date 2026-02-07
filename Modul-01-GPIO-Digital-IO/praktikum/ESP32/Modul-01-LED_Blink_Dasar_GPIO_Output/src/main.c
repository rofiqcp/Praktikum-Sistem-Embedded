/**
 * @file main.c
 * @brief Program 01: LED Blink - Dasar GPIO Output (ESP-IDF)
 *
 * Deskripsi:
 * Program dasar untuk mengendalikan LED menggunakan GPIO ESP-IDF driver.
 * LED akan berkedip dengan interval yang dapat dikonfigurasi.
 * Menggunakan ESP-IDF native API: gpio_config(), gpio_set_level()
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1 (GPIO2 = LED built-in)
 * - Wemos Lolin S2 Mini  (GPIO15 = LED built-in)
 * - ESP32-S3 DevKitC-1   (GPIO48 = RGB LED / user LED)
 * - LED External dengan resistor 220Ω (opsional)
 *
 * Referensi:
 * - Kolban's ESP32 Book, hal 251-257 (GPIO)
 * - ESP-IDF GPIO API: esp_idf/components/driver/gpio
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LED_BLINK";

/* ==================== KONFIGURASI ==================== */
#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO    2        // Default: ESP32 DOIT DevKit built-in LED
#endif

#define BLINK_DELAY_MS     500      // Interval blink (ms)

/* ==================== VARIABEL GLOBAL ==================== */
static bool led_state = false;
static uint32_t blink_count = 0;

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 01: LED Blink - ESP-IDF");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi GPIO menggunakan gpio_config_t struct */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_LED_GPIO),   // Bitmask pin
        .mode = GPIO_MODE_OUTPUT,                     // Mode output
        .pull_up_en = GPIO_PULLUP_DISABLE,            // Tidak perlu pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,        // Tidak perlu pull-down
        .intr_type = GPIO_INTR_DISABLE,               // Tidak pakai interrupt
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "LED Pin: GPIO%d", CONFIG_LED_GPIO);
    ESP_LOGI(TAG, "Blink Interval: %d ms", BLINK_DELAY_MS);
    ESP_LOGI(TAG, "Program dimulai...");

    /* Loop utama - non-blocking blink menggunakan vTaskDelay */
    while (1) {
        /* Toggle LED state */
        led_state = !led_state;
        gpio_set_level(CONFIG_LED_GPIO, led_state);

        blink_count++;

        ESP_LOGI(TAG, "[%lld ms] LED: %s | Blink #%lu",
                 esp_timer_get_time() / 1000,
                 led_state ? "ON " : "OFF",
                 (unsigned long)blink_count);

        /* Delay menggunakan FreeRTOS (non-blocking, yield ke task lain) */
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY_MS));
    }
}

/**
 * PENJELASAN ESP-IDF GPIO API:
 *
 * 1. gpio_config_t struct:
 *    - pin_bit_mask : Bitmask GPIO yang akan dikonfigurasi (bisa multi-pin)
 *    - mode         : GPIO_MODE_INPUT / GPIO_MODE_OUTPUT / GPIO_MODE_INPUT_OUTPUT
 *    - pull_up_en   : Internal pull-up resistor
 *    - pull_down_en : Internal pull-down resistor
 *    - intr_type    : Interrupt type (disable/rising/falling/both/low/high)
 *
 * 2. gpio_set_level(gpio_num, level):
 *    - Set pin HIGH (1) atau LOW (0)
 *
 * 3. vTaskDelay(pdMS_TO_TICKS(ms)):
 *    - Delay yang cooperative (yield CPU ke task lain)
 *    - Berbeda dengan delay() Arduino yang blocking
 *
 * WIRING DIAGRAM:
 *
 *   ESP32 DOIT DevKit V1
 *   ┌─────────────┐
 *   │        GPIO2 ├───[LED built-in]  (Active HIGH)
 *   │             │
 *   │             ├───[220Ω]───[LED]───GND  (External)
 *   │         GND ├───────────────────┘
 *   └─────────────┘
 *
 *   Wemos Lolin S2 Mini
 *   ┌─────────────┐
 *   │       GPIO15 ├───[LED built-in]
 *   └─────────────┘
 *
 *   ESP32-S3 DevKitC-1
 *   ┌─────────────┐
 *   │       GPIO48 ├───[RGB LED / User LED]
 *   └─────────────┘
 */
