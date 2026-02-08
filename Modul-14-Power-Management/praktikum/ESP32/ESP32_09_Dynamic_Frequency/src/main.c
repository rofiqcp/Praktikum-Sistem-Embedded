/**
 * ESP32_09_Dynamic_Frequency
 * 
 * Konsep: CPU frequency scaling, trade-off performa vs daya.
 * - Gunakan esp_pm_configure() untuk mengatur frekuensi CPU
 * - Siklus melalui frekuensi: 240, 160, 80, 40, 20, 10 MHz
 * - Di setiap frekuensi: jalankan benchmark, ukur waktu, estimasi MIPS
 * - Tampilkan tabel trade-off performa/daya
 * 
 * Framework: ESP-IDF (bukan Arduino)
 * Catatan: Perlu CONFIG_PM_ENABLE=y di sdkconfig
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_clk_tree.h"
#include "esp_sleep.h"

static const char *TAG = "DYN_FREQ";

// Frekuensi CPU yang akan diuji (dalam MHz)
static const int test_frequencies[] = {240, 160, 80, 40, 20, 10};
static const int num_frequencies = sizeof(test_frequencies) / sizeof(test_frequencies[0]);

// Estimasi arus per frekuensi (mA, typical dari datasheet)
static const float estimated_current[] = {68.0, 50.0, 32.0, 20.0, 13.0, 8.0};

// Hasil benchmark
typedef struct {
    int freq_mhz;
    int64_t benchmark_time_us;
    uint32_t loop_count;
    float mips_estimate;
    float current_ma;
    float efficiency;  // MIPS per mA
} freq_result_t;

static freq_result_t results[6];

/**
 * Jalankan benchmark sederhana - loop counting dan kalkulasi
 * Mengembalikan jumlah iterasi yang selesai
 */
static uint32_t run_benchmark(int64_t *elapsed_us)
{
    volatile uint32_t counter = 0;
    volatile float accumulator = 1.0f;
    const uint32_t iterations = 500000;

    int64_t start = esp_timer_get_time();

    for (uint32_t i = 0; i < iterations; i++) {
        counter++;
        accumulator = accumulator * 1.00001f + 0.00001f;
        // Operasi tambahan untuk benchmark realistis
        if (i % 1000 == 0) {
            accumulator = accumulator / 1.001f;
        }
    }

    int64_t end = esp_timer_get_time();
    *elapsed_us = end - start;

    // Gunakan accumulator agar compiler tidak mengoptimasi
    if (accumulator < 0) counter = 0;

    return counter;
}

/**
 * Set frekuensi CPU menggunakan power management
 */
static esp_err_t set_cpu_frequency(int freq_mhz)
{
    esp_pm_config_t pm_config = {
        .max_freq_mhz = freq_mhz,
        .min_freq_mhz = freq_mhz,
        .light_sleep_enable = false,
    };

    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal set frekuensi %d MHz: %s", freq_mhz, esp_err_to_name(ret));
        return ret;
    }

    // Tunggu frekuensi stabil
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

/**
 * Verifikasi frekuensi CPU aktual
 */
static uint32_t get_actual_frequency(void)
{
    uint32_t freq_hz = 0;
    esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &freq_hz);
    return freq_hz / 1000000;  // Konversi ke MHz
}

/**
 * Jalankan benchmark di semua frekuensi
 */
static void run_frequency_sweep(void)
{
    ESP_LOGI(TAG, "\n--- Memulai Frequency Sweep Benchmark ---\n");

    for (int i = 0; i < num_frequencies; i++) {
        int target_freq = test_frequencies[i];
        ESP_LOGI(TAG, "Testing frekuensi: %d MHz...", target_freq);

        // Set frekuensi
        esp_err_t ret = set_cpu_frequency(target_freq);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Skip %d MHz (tidak didukung)", target_freq);
            results[i].freq_mhz = target_freq;
            results[i].benchmark_time_us = 0;
            results[i].loop_count = 0;
            results[i].mips_estimate = 0;
            results[i].current_ma = estimated_current[i];
            results[i].efficiency = 0;
            continue;
        }

        // Verifikasi frekuensi
        uint32_t actual_freq = get_actual_frequency();
        ESP_LOGI(TAG, "  Frekuensi aktual: %lu MHz", (unsigned long)actual_freq);

        // Jalankan benchmark
        int64_t elapsed_us = 0;
        uint32_t count = run_benchmark(&elapsed_us);

        // Hitung metrik
        float seconds = (float)elapsed_us / 1000000.0f;
        float mips = ((float)count * 3.0f) / (seconds * 1000000.0f); // ~3 operasi per iterasi

        results[i].freq_mhz = target_freq;
        results[i].benchmark_time_us = elapsed_us;
        results[i].loop_count = count;
        results[i].mips_estimate = mips;
        results[i].current_ma = estimated_current[i];
        results[i].efficiency = (estimated_current[i] > 0) ? mips / estimated_current[i] : 0;

        ESP_LOGI(TAG, "  Waktu: %lld us | Iterasi: %lu | MIPS: %.2f",
                 elapsed_us, (unsigned long)count, mips);
    }
}

/**
 * Tampilkan tabel hasil
 */
static void print_results_table(void)
{
    ESP_LOGI(TAG, "\n============================================================");
    ESP_LOGI(TAG, "  TABEL PERFORMA vs DAYA - ESP32 Dynamic Frequency Scaling");
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, " Freq(MHz) | Waktu(ms) |   MIPS   | Arus(mA) | Efisiensi");
    ESP_LOGI(TAG, "-----------|-----------|----------|----------|----------");

    for (int i = 0; i < num_frequencies; i++) {
        if (results[i].benchmark_time_us > 0) {
            float time_ms = (float)results[i].benchmark_time_us / 1000.0f;
            ESP_LOGI(TAG, "   %3d     | %7.1f   | %6.2f   | %6.1f   | %6.4f",
                     results[i].freq_mhz,
                     time_ms,
                     results[i].mips_estimate,
                     results[i].current_ma,
                     results[i].efficiency);
        } else {
            ESP_LOGW(TAG, "   %3d     |   N/A     |   N/A    | %6.1f   |   N/A",
                     results[i].freq_mhz,
                     results[i].current_ma);
        }
    }
    ESP_LOGI(TAG, "-----------|-----------|----------|----------|----------");
    ESP_LOGI(TAG, "");

    // Analisis
    float best_efficiency = 0;
    int best_idx = 0;
    float fastest_time = 1e9;
    int fastest_idx = 0;

    for (int i = 0; i < num_frequencies; i++) {
        if (results[i].efficiency > best_efficiency) {
            best_efficiency = results[i].efficiency;
            best_idx = i;
        }
        if (results[i].benchmark_time_us > 0 &&
            (float)results[i].benchmark_time_us < fastest_time) {
            fastest_time = (float)results[i].benchmark_time_us;
            fastest_idx = i;
        }
    }

    ESP_LOGI(TAG, "Analisis:");
    ESP_LOGI(TAG, "  Tercepat    : %d MHz (%.1f ms)",
             results[fastest_idx].freq_mhz,
             fastest_time / 1000.0f);
    ESP_LOGI(TAG, "  Ter-efisien : %d MHz (%.4f MIPS/mA)",
             results[best_idx].freq_mhz, best_efficiency);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "  * Efisiensi = MIPS / Arus (MIPS per mA)");
    ESP_LOGI(TAG, "  * Frekuensi rendah lebih efisien untuk tugas ringan");
    ESP_LOGI(TAG, "  * Frekuensi tinggi lebih baik untuk tugas berat (selesai cepat)");
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 Dynamic Frequency Scaling");
    ESP_LOGI(TAG, "============================================");

    // Jalankan sweep benchmark
    run_frequency_sweep();

    // Tampilkan hasil
    print_results_table();

    // Kembalikan ke frekuensi default
    set_cpu_frequency(160);
    ESP_LOGI(TAG, "Frekuensi dikembalikan ke 160 MHz");

    ESP_LOGI(TAG, "\nProgram selesai. Restart untuk mengulangi.");
    ESP_LOGI(TAG, "============================================");

    // Loop idle
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
