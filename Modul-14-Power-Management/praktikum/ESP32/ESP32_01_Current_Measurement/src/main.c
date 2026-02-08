/**
 * ==========================================================================
 * ESP32_01_Current_Measurement - Pengukuran Arus Mode Aktif
 * ==========================================================================
 * 
 * Modul 14 - Power Management
 * Program 1: Measure active current consumption, establish baseline
 * 
 * KONSEP:
 * - Mengukur konsumsi arus ESP32 dalam mode aktif (Active Mode)
 * - CPU berjalan pada 240MHz, toggle LED secara cepat
 * - Menampilkan statistik CPU: frekuensi, heap size, tick count
 * - Durasi pengukuran: 30 detik dengan statistik periodik
 * - Expected active mode consumption: ~120mA @ 240MHz
 * 
 * WIRING / KONEKSI:
 * ┌─────────────────────────────────────────────┐
 * │  ESP32          Komponen                     │
 * │  GPIO2  ──────► LED (+) ──► R(220Ω) ──► GND │
 * │  VIN    ──┬──► Multimeter COM               │
 * │           └──► (inline measurement)          │
 * │  5V PSU  ──►   Multimeter V/A               │
 * └─────────────────────────────────────────────┘
 * 
 * SETUP MULTIMETER (untuk pengukuran arus):
 * 1. Set multimeter ke mode DC Ampere (mA range)
 * 2. Hubungkan multimeter SERI (inline) dengan VCC/VIN
 * 3. Power Supply → Multimeter (+) → ESP32 VIN
 * 4. ESP32 GND → Power Supply GND
 * 5. Baca arus pada multimeter (~120mA active mode)
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * [POWER] CPU Frequency: 240 MHz
 * [POWER] Free Heap: 298456 bytes
 * [POWER] Tick Count: 1000
 * [POWER] Uptime: 1.00 s | LED toggles: 500
 * [POWER] Estimated Power: ~120mA @ 3.3V = 396mW
 * ========================================
 * 
 * CATATAN:
 * - Pengukuran arus sebenarnya memerlukan multimeter/INA219
 * - Program ini membuat ESP32 bekerja aktif penuh untuk baseline
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_clk_tree.h"
#include "esp_chip_info.h"

static const char *TAG = "POWER";

/* Konfigurasi pin LED */
#define LED_PIN         GPIO_NUM_2      // Onboard LED (kebanyakan board ESP32)
#define MEASUREMENT_DURATION_SEC  30    // Durasi pengukuran dalam detik
#define STATS_INTERVAL_MS         2000  // Interval cetak statistik (2 detik)

/* Variabel global untuk tracking */
static uint32_t led_toggle_count = 0;
static int64_t start_time_us = 0;

/**
 * Inisialisasi GPIO untuk LED
 * Konfigurasi pin sebagai output push-pull
 */
static void init_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "GPIO%d dikonfigurasi sebagai output LED", LED_PIN);
}

/**
 * Cetak informasi chip ESP32
 * Menampilkan model, cores, revision, dan fitur
 */
static void print_chip_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 CURRENT MEASUREMENT BASELINE");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Chip: ESP32 rev %d, %d core(s)", chip_info.revision, chip_info.cores);
    ESP_LOGI(TAG, "Features: WiFi%s%s",
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    ESP_LOGI(TAG, "Flash: %s",
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

/**
 * Cetak statistik sistem secara periodik
 * Termasuk frekuensi CPU, heap, uptime, dan estimasi daya
 */
static void print_system_stats(void)
{
    /* Dapatkan frekuensi CPU saat ini */
    uint32_t cpu_freq_mhz = 0;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &cpu_freq_mhz);
    cpu_freq_mhz /= 1000000; // Konversi Hz ke MHz

    /* Hitung uptime */
    int64_t now_us = esp_timer_get_time();
    float uptime_sec = (float)(now_us - start_time_us) / 1000000.0f;

    /* Estimasi daya berdasarkan mode aktif */
    float est_current_ma = 120.0f;  // Typical active mode @ 240MHz
    float est_power_mw = est_current_ma * 3.3f;

    ESP_LOGI(TAG, "----------------------------------------");
    ESP_LOGI(TAG, "CPU Frequency: %lu MHz", (unsigned long)cpu_freq_mhz);
    ESP_LOGI(TAG, "Free Heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "Min Free Heap: %lu bytes", (unsigned long)esp_get_minimum_free_heap_size());
    ESP_LOGI(TAG, "Tick Count: %lu", (unsigned long)xTaskGetTickCount());
    ESP_LOGI(TAG, "Uptime: %.2f s | LED toggles: %lu", uptime_sec, (unsigned long)led_toggle_count);
    ESP_LOGI(TAG, "Estimated Power: ~%.0fmA @ 3.3V = %.0fmW", est_current_ma, est_power_mw);
    ESP_LOGI(TAG, "DATA,%lu,%.2f,%lu,%lu,%.0f",
             (unsigned long)cpu_freq_mhz, uptime_sec,
             (unsigned long)esp_get_free_heap_size(),
             (unsigned long)led_toggle_count, est_current_ma);
}

/**
 * Task utama: Toggle LED cepat + cetak statistik
 * LED di-toggle terus menerus untuk memaksimalkan konsumsi arus
 */
static void measurement_task(void *pvParameter)
{
    int led_state = 0;
    TickType_t last_stats_time = xTaskGetTickCount();
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t end_tick = start_tick + pdMS_TO_TICKS(MEASUREMENT_DURATION_SEC * 1000);

    ESP_LOGI(TAG, "Memulai pengukuran selama %d detik...", MEASUREMENT_DURATION_SEC);
    ESP_LOGW(TAG, "Pasang multimeter SERI dengan VCC untuk mengukur arus!");

    while (xTaskGetTickCount() < end_tick) {
        /* Toggle LED secepat mungkin (maximize active current) */
        led_state = !led_state;
        gpio_set_level(LED_PIN, led_state);
        led_toggle_count++;

        /* Cetak statistik setiap STATS_INTERVAL_MS */
        if ((xTaskGetTickCount() - last_stats_time) >= pdMS_TO_TICKS(STATS_INTERVAL_MS)) {
            print_system_stats();
            last_stats_time = xTaskGetTickCount();
        }

        /* Delay singkat agar watchdog tidak trigger */
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Selesai - matikan LED */
    gpio_set_level(LED_PIN, 0);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  PENGUKURAN SELESAI");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Total LED toggles: %lu", (unsigned long)led_toggle_count);
    ESP_LOGI(TAG, "Catat nilai arus dari multimeter Anda!");
    ESP_LOGW(TAG, "Bandingkan dengan datasheet: ~120mA active mode");

    /* Loop idle setelah selesai */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * Entry point utama ESP-IDF
 */
void app_main(void)
{
    /* Catat waktu mulai */
    start_time_us = esp_timer_get_time();

    /* Cetak info chip */
    print_chip_info();

    /* Inisialisasi GPIO LED */
    init_gpio();

    /* Buat task pengukuran */
    xTaskCreate(measurement_task, "measure_task", 4096, NULL, 5, NULL);
}
