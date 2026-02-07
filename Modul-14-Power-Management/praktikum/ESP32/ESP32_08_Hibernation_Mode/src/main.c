/**
 * ============================================================================
 * PROJECT  : ESP32_08_Hibernation_Mode
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Hibernation Mode (Ultra-Low Power ~5µA)
 *
 * DESKRIPSI:
 * Hibernation adalah mode daya terendah ESP32 (~5µA). Berbeda dengan deep
 * sleep biasa, pada hibernation:
 * - RTC memory DIMATIKAN (RTC_DATA_ATTR hilang!)
 * - Hanya RTC timer yang aktif
 * - Internal 8MHz oscillator dimatikan
 * - Cocok untuk aplikasi yang hanya perlu bangun secara periodik
 *   tanpa perlu menyimpan state
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) HIBERNATE: === Hibernation Demo ===
 *   I (xxx) HIBERNATE: Note: RTC memory is OFF in hibernation
 *   I (xxx) HIBERNATE: This boot_count will always be 1
 *   I (xxx) HIBERNATE: boot_count = 1
 *   I (xxx) HIBERNATE: Entering HIBERNATION for 10 seconds...
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define LED_PIN     GPIO_NUM_2
#define SLEEP_SEC   10

static const char *TAG = "HIBERNATE";

/* Ini TIDAK akan bertahan di hibernation (RTC memory off) */
RTC_DATA_ATTR static int boot_count = 0;

void app_main(void)
{
    boot_count++;  /* Akan selalu 1 karena RTC memory direset */

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Hibernation Mode (~5µA)      ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Wake-up cause: %s",
             (cause == ESP_SLEEP_WAKEUP_TIMER) ? "TIMER" : "POWER_ON");

    ESP_LOGW(TAG, "Note: RTC memory is OFF in hibernation mode");
    ESP_LOGW(TAG, "boot_count will always be 1 (cannot persist)");
    ESP_LOGI(TAG, "boot_count = %d", boot_count);

    /* LED indicator */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_PIN, 0);

    /* === Konfigurasi Hibernation === */
    /* Matikan semua power domain kecuali RTC timer */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

    /* Hanya timer wake-up yang bisa digunakan dalam hibernation */
    esp_sleep_enable_timer_wakeup(SLEEP_SEC * 1000000ULL);

    ESP_LOGI(TAG, "Entering HIBERNATION for %d seconds...", SLEEP_SEC);
    ESP_LOGI(TAG, "Expected current: ~5 µA");
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Masuk deep sleep dengan konfigurasi hibernation */
    esp_deep_sleep_start();
}
