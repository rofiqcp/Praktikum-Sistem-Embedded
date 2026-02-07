/**
 * ============================================================================
 * PROJECT  : ESP32_01_Deep_Sleep_Timer
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF (Espressif IoT Development Framework)
 *
 * JUDUL    : Deep Sleep dengan Timer Wake-up
 *
 * DESKRIPSI:
 * Program ini mendemonstrasikan mode deep sleep ESP32 menggunakan
 * ESP-IDF API. ESP32 akan masuk deep sleep selama 5 detik, lalu
 * bangun otomatis via RTC timer. Boot count disimpan di RTC memory
 * agar tetap tersimpan antar siklus deep sleep.
 *
 * Konsep deep sleep ESP32:
 * - Hanya RTC controller, RTC memory (8KB), dan ULP yang aktif
 * - Konsumsi daya ~10 µA
 * - Setelah wake-up, program mulai dari app_main() (seperti cold boot)
 * - Variabel RTC_DATA_ATTR bertahan melewati deep sleep
 *
 * ============================================================================
 * WIRING
 * ============================================================================
 * LED Built-in → GPIO2 (onboard ESP32 DevKit)
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) DEEP_SLEEP: === Boot #1 ===
 *   I (xxx) DEEP_SLEEP: Wake-up cause: POWER ON / RESET (0)
 *   I (xxx) DEEP_SLEEP: LED ON for 500ms
 *   I (xxx) DEEP_SLEEP: Entering deep sleep for 5 seconds...
 *   [... 5 detik kemudian ...]
 *   I (xxx) DEEP_SLEEP: === Boot #2 ===
 *   I (xxx) DEEP_SLEEP: Wake-up cause: TIMER
 *   I (xxx) DEEP_SLEEP: LED ON for 500ms
 *   I (xxx) DEEP_SLEEP: Entering deep sleep for 5 seconds...
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "config.h"

static const char *TAG = "DEEP_SLEEP";

/* Boot count disimpan di RTC memory — bertahan saat deep sleep */
RTC_DATA_ATTR static int boot_count = 0;

/**
 * Cetak alasan wake-up dalam format yang mudah dibaca
 */
static void print_wakeup_cause(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake-up cause: TIMER");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wake-up cause: EXT0 (GPIO)");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wake-up cause: EXT1 (GPIO mask)");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            ESP_LOGI(TAG, "Wake-up cause: TOUCHPAD");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            ESP_LOGI(TAG, "Wake-up cause: ULP");
            break;
        default:
            ESP_LOGI(TAG, "Wake-up cause: POWER ON / RESET (%d)", cause);
            break;
    }
}

/**
 * Entry point utama ESP-IDF.
 * Dipanggil setiap kali ESP32 boot (termasuk wake-up dari deep sleep).
 */
void app_main(void)
{
    /* Increment boot counter (tersimpan di RTC memory) */
    boot_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Deep Sleep + Timer Wakeup    ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Boot #%d ===", boot_count);

    /* Cetak alasan bangun */
    print_wakeup_cause();

    /* Tampilkan info heap */
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    /* Konfigurasi dan nyalakan LED sebagai indikator bangun */
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "LED ON for 500ms");
    gpio_set_level(LED_GPIO_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LED_GPIO_PIN, 0);

    /* Konfigurasi timer wake-up: 5 detik */
    esp_err_t ret = esp_sleep_enable_timer_wakeup(SLEEP_5_SEC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure timer wakeup: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Entering deep sleep for %llu seconds...",
             SLEEP_5_SEC / 1000000ULL);

    /* Flush UART sebelum tidur */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Masuk deep sleep — tidak pernah return dari sini */
    esp_deep_sleep_start();
}
