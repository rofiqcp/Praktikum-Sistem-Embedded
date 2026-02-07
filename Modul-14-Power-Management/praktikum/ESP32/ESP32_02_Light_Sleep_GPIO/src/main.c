/**
 * ============================================================================
 * PROJECT  : ESP32_02_Light_Sleep_GPIO
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Light Sleep dengan GPIO Wake-up
 *
 * DESKRIPSI:
 * Demonstrasi light sleep ESP32. Berbeda dengan deep sleep, pada light sleep
 * SRAM dipertahankan dan program melanjutkan dari titik terakhir (bukan reset).
 * Wake-up menggunakan GPIO interrupt (button press).
 *
 * Light sleep vs Deep sleep:
 * - Light sleep: ~0.8 mA, SRAM retained, resume dari titik tidur
 * - Deep sleep:  ~10 µA,  SRAM hilang, restart dari app_main()
 *
 * ============================================================================
 * WIRING
 * ============================================================================
 * Button → GPIO0 (BOOT button bawaan, active LOW)
 * LED    → GPIO2 (built-in)
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) LIGHT_SLEEP: Entering light sleep...
 *   I (xxx) LIGHT_SLEEP: Woke up! Cause: GPIO
 *   I (xxx) LIGHT_SLEEP: Sleep duration: 3245 ms
 *   I (xxx) LIGHT_SLEEP: Sleep count: 1
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#define LED_PIN     GPIO_NUM_2
#define BUTTON_PIN  GPIO_NUM_0   /* BOOT button */

static const char *TAG = "LIGHT_SLEEP";

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Light Sleep + GPIO Wakeup    ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");

    /* Konfigurasi LED */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    /* Konfigurasi Button sebagai wake-up source */
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    /* Enable GPIO wake-up: bangun saat GPIO0 = LOW (button ditekan) */
    gpio_wakeup_enable(BUTTON_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    /* Juga enable timer wake-up sebagai backup (30 detik) */
    esp_sleep_enable_timer_wakeup(30 * 1000000ULL);

    int sleep_count = 0;

    while (1) {
        /* LED menyala sebelum tidur */
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);

        ESP_LOGI(TAG, "Entering light sleep... (press BOOT button to wake)");
        vTaskDelay(pdMS_TO_TICKS(100));  /* Flush UART */

        /* Catat waktu sebelum tidur */
        int64_t before = esp_timer_get_time();

        /* Masuk light sleep — SRAM dipertahankan, resume di sini */
        esp_light_sleep_start();

        /* Program berlanjut dari sini setelah bangun */
        int64_t after = esp_timer_get_time();
        int64_t sleep_ms = (after - before) / 1000;
        sleep_count++;

        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        const char *cause_str = (cause == ESP_SLEEP_WAKEUP_GPIO) ? "GPIO" :
                                (cause == ESP_SLEEP_WAKEUP_TIMER) ? "TIMER" : "OTHER";

        ESP_LOGI(TAG, "Woke up! Cause: %s", cause_str);
        ESP_LOGI(TAG, "Sleep duration: %lld ms", sleep_ms);
        ESP_LOGI(TAG, "Sleep count: %d", sleep_count);
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
        ESP_LOGI(TAG, "---");

        /* Debounce: tunggu button dilepas */
        while (gpio_get_level(BUTTON_PIN) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
