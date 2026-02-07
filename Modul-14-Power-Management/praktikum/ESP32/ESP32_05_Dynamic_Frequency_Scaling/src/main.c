/**
 * ============================================================================
 * PROJECT  : ESP32_05_Dynamic_Frequency_Scaling
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Dynamic Frequency Scaling (DFS) & Clock Gating
 *
 * DESKRIPSI:
 * Demonstrasi DFS pada ESP32 — CPU otomatis turun frekuensi saat idle
 * dan naik saat ada beban kerja. Juga mendemonstrasikan periph_module
 * enable/disable sebagai bentuk clock gating.
 *
 * Benchmark dilakukan pada frekuensi 240MHz, 80MHz, dan 10MHz
 * untuk menunjukkan trade-off antara performa dan daya.
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) DFS: === Frequency Scaling Benchmark ===
 *   I (xxx) DFS: [240 MHz] 1M iterations: 12345 us
 *   I (xxx) DFS: [ 80 MHz] 1M iterations: 37000 us
 *   I (xxx) DFS: Ratio 240/80: 3.0x
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "driver/periph_ctrl.h"

static const char *TAG = "DFS";

/**
 * Benchmark: hitung 1M iterasi, ukur waktu
 */
static int64_t run_benchmark(void)
{
    int64_t start = esp_timer_get_time();
    volatile long sum = 0;
    for (long i = 0; i < 1000000; i++) {
        sum += i;
    }
    int64_t elapsed = esp_timer_get_time() - start;
    (void)sum;  /* Hindari compiler optimize-out */
    return elapsed;
}

/**
 * Set CPU frequency menggunakan esp_pm_configure
 */
static esp_err_t set_cpu_freq(int mhz)
{
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = mhz,
        .min_freq_mhz = mhz,
        .light_sleep_enable = false
    };
    return esp_pm_configure(&pm_config);
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Dynamic Frequency Scaling    ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    /* ====================================================
     * BAGIAN 1: Benchmark pada frekuensi berbeda
     * ==================================================== */
    ESP_LOGI(TAG, "=== Frequency Scaling Benchmark ===");

    /* Test 240 MHz */
    set_cpu_freq(240);
    vTaskDelay(pdMS_TO_TICKS(100));
    int64_t t240 = run_benchmark();
    ESP_LOGI(TAG, "[240 MHz] 1M iterations: %lld us", t240);

    /* Test 160 MHz */
    set_cpu_freq(160);
    vTaskDelay(pdMS_TO_TICKS(100));
    int64_t t160 = run_benchmark();
    ESP_LOGI(TAG, "[160 MHz] 1M iterations: %lld us", t160);

    /* Test 80 MHz */
    set_cpu_freq(80);
    vTaskDelay(pdMS_TO_TICKS(100));
    int64_t t80 = run_benchmark();
    ESP_LOGI(TAG, "[ 80 MHz] 1M iterations: %lld us", t80);

    /* Kembali ke 240 MHz */
    set_cpu_freq(240);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Speed Ratios ===");
    if (t240 > 0) {
        ESP_LOGI(TAG, "160/240 ratio: %.1fx slower", (float)t160 / t240);
        ESP_LOGI(TAG, " 80/240 ratio: %.1fx slower", (float)t80 / t240);
    }

    /* ====================================================
     * BAGIAN 2: Clock Gating — disable peripheral
     * ==================================================== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Clock Gating Demo ===");

    ESP_LOGI(TAG, "Disabling I2C0 peripheral clock...");
    periph_module_disable(PERIPH_I2C0_MODULE);
    ESP_LOGI(TAG, "I2C0 clock disabled (saves power if not used)");

    ESP_LOGI(TAG, "Disabling SPI2 peripheral clock...");
    periph_module_disable(PERIPH_SPI2_MODULE);
    ESP_LOGI(TAG, "SPI2 clock disabled");

    /* Re-enable jika diperlukan */
    ESP_LOGI(TAG, "Re-enabling I2C0...");
    periph_module_enable(PERIPH_I2C0_MODULE);
    ESP_LOGI(TAG, "I2C0 re-enabled");

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Summary ===");
    ESP_LOGI(TAG, "- Lower frequency = lower power but slower execution");
    ESP_LOGI(TAG, "- Disable unused peripherals to save static power");
    ESP_LOGI(TAG, "- ESP-IDF DFS can auto-scale between min/max freq");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
