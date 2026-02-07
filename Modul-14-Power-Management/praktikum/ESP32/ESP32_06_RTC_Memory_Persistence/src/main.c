/**
 * ============================================================================
 * PROJECT  : ESP32_06_RTC_Memory_Persistence
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : RTC Memory Data Logger
 *
 * DESKRIPSI:
 * Demonstrasi penggunaan RTC memory (8KB) untuk menyimpan data antar
 * siklus deep sleep. Data sensor dikumpulkan setiap wake-up dan disimpan
 * dalam buffer di RTC memory. Ketika buffer penuh, data di-dump lalu reset.
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) RTC_LOG: === RTC Logger Boot #5 ===
 *   I (xxx) RTC_LOG: New reading: 23.4 °C
 *   I (xxx) RTC_LOG: --- Stored Data (5 readings) ---
 *   [00] 21.2 °C
 *   [01] 24.5 °C
 *   ...
 *   I (xxx) RTC_LOG: Sleeping for 10 seconds...
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#define MAX_READINGS    30
#define SLEEP_SEC       10

static const char *TAG = "RTC_LOG";

/* === Data di RTC Memory === */
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static float temperature_log[MAX_READINGS];
RTC_DATA_ATTR static float humidity_log[MAX_READINGS];
RTC_DATA_ATTR static int log_index = 0;
RTC_DATA_ATTR static int64_t total_awake_us = 0;

/**
 * Simulasi pembacaan sensor (ganti dengan driver sensor asli)
 */
static float read_temperature(void)
{
    return 20.0f + (float)(esp_random() % 200) / 10.0f;  /* 20.0 - 40.0 */
}

static float read_humidity(void)
{
    return 40.0f + (float)(esp_random() % 400) / 10.0f;   /* 40.0 - 80.0 */
}

void app_main(void)
{
    int64_t wake_start = esp_timer_get_time();

    boot_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 RTC Memory Data Logger       ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "=== Boot #%d ===", boot_count);

    /* Cetak alasan wake-up */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Wake cause: %s",
             (cause == ESP_SLEEP_WAKEUP_TIMER) ? "TIMER" : "POWER_ON/RESET");

    /* Baca sensor */
    float temp = read_temperature();
    float hum = read_humidity();

    /* Simpan ke RTC memory buffer */
    if (log_index < MAX_READINGS) {
        temperature_log[log_index] = temp;
        humidity_log[log_index] = hum;
        log_index++;
        ESP_LOGI(TAG, "New reading: %.1f °C, %.1f %% (index %d/%d)",
                 temp, hum, log_index, MAX_READINGS);
    }

    /* Tampilkan semua data yang tersimpan */
    ESP_LOGI(TAG, "--- Stored Data (%d readings) ---", log_index);
    for (int i = 0; i < log_index; i++) {
        printf("  [%02d] %.1f °C  |  %.1f %%\n",
               i, temperature_log[i], humidity_log[i]);
    }

    /* Statistik */
    if (log_index > 0) {
        float temp_sum = 0, hum_sum = 0;
        for (int i = 0; i < log_index; i++) {
            temp_sum += temperature_log[i];
            hum_sum += humidity_log[i];
        }
        ESP_LOGI(TAG, "Average: %.1f °C, %.1f %%",
                 temp_sum / log_index, hum_sum / log_index);
    }

    /* Cek buffer penuh */
    if (log_index >= MAX_READINGS) {
        ESP_LOGW(TAG, "*** Buffer FULL! (%d readings) ***", MAX_READINGS);
        ESP_LOGW(TAG, "In production: send data via WiFi/MQTT here");
        ESP_LOGW(TAG, "Resetting buffer...");
        log_index = 0;
    }

    /* Hitung total waktu aktif */
    int64_t awake_time = esp_timer_get_time() - wake_start;
    total_awake_us += awake_time;
    ESP_LOGI(TAG, "This wake: %lld ms | Total awake: %lld ms",
             awake_time / 1000, total_awake_us / 1000);

    /* Estimasi uptime total */
    int64_t total_time_s = (int64_t)boot_count * SLEEP_SEC + total_awake_us / 1000000;
    ESP_LOGI(TAG, "Estimated total uptime: %lld seconds", total_time_s);

    /* Setup deep sleep */
    esp_sleep_enable_timer_wakeup(SLEEP_SEC * 1000000ULL);
    ESP_LOGI(TAG, "Sleeping for %d seconds...", SLEEP_SEC);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_deep_sleep_start();
}
