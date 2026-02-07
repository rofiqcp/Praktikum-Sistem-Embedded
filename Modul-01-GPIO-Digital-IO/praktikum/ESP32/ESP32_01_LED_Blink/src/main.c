/**
 * ============================================================================
 * ESP32 - Program 01: LED Blink (GPIO Output Dasar)
 * ============================================================================
 * Modul    : 01 - GPIO Digital I/O
 * Platform : ESP32 DevKit V1 / Lolin S2 / ESP32-S3
 * Framework: ESP-IDF
 * 
 * Deskripsi:
 *   Menyalakan dan mematikan LED built-in (GPIO2) secara periodik.
 *   Demonstrasi dasar konfigurasi GPIO output dan delay.
 * 
 * Hardware:
 *   - ESP32 DevKit V1 (LED built-in pada GPIO2)
 *   - Tidak perlu komponen tambahan
 * 
 * Wiring:
 *   Tidak ada (menggunakan LED on-board)
 * 
 * Pin Mapping:
 *   ESP32 DevKit V1 : GPIO2 (built-in LED)
 *   Lolin S2 Mini   : GPIO15 (built-in LED)
 *   ESP32-S3        : GPIO48 (built-in LED, RGB)
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* ---- Konfigurasi Pin ---- */
#include "config.h"

static const char *TAG = "LED_BLINK";

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 LED Blink - GPIO Output Dasar ===");
    ESP_LOGI(TAG, "LED Pin: GPIO%d", LED_PIN);

    /* Konfigurasi GPIO sebagai output */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "GPIO dikonfigurasi sebagai OUTPUT");
    ESP_LOGI(TAG, "LED akan berkedip setiap 500ms...");

    bool led_state = false;

    while (1) {
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state ? 1 : 0);
        ESP_LOGI(TAG, "LED: %s", led_state ? "ON" : "OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
