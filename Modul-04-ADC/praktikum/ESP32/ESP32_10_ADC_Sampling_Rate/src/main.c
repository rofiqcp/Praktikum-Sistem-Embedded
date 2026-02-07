/**
 * ==========================================================================
 * PROGRAM 10: ADC Sampling Rate (Pengukuran Kecepatan Sampling ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini mengukur kecepatan sampling (sampling rate) ADC ESP32
 *   dengan menghitung waktu yang dibutuhkan untuk N konversi.
 *   Pengukuran dilakukan dengan esp_timer_get_time() yang memberikan
 *   resolusi mikro-detik.
 * 
 *   Program menguji sampling rate pada setiap level atenuasi:
 *   - 0dB, 2.5dB, 6dB, 11dB
 * 
 * Rumus Sampling Rate:
 *   Rate (samples/second) = N / (waktu_total_us / 1000000)
 * 
 * Koneksi Hardware:
 *   - Potensiometer: Wiper → GPIO34 (opsional, bisa tanpa input)
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "ADC_RATE";

/* Konfigurasi ADC */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC1_CHANNEL_3
    #define ADC_GPIO_NUM    4
#else
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

#define ADC_WIDTH       ADC_WIDTH_BIT_12

/* Jumlah konversi untuk pengukuran */
#define NUM_CONVERSIONS     10000
#define TEST_REPEAT         5   /* Ulangi setiap test N kali */

/* Daftar atenuasi yang akan diuji */
typedef struct {
    adc_atten_t atten;
    const char *label;
} atten_test_t;

static const atten_test_t atten_list[] = {
    { ADC_ATTEN_DB_0,   "0 dB   " },
    { ADC_ATTEN_DB_2_5, "2.5 dB " },
    { ADC_ATTEN_DB_6,   "6 dB   " },
    { ADC_ATTEN_DB_11,  "11 dB  " }
};
#define NUM_ATTEN (sizeof(atten_list) / sizeof(atten_list[0]))

/**
 * @brief Ukur sampling rate untuk atenuasi tertentu
 * 
 * @param atten Level atenuasi
 * @param num_samples Jumlah sampel yang diambil
 * @param out_rate_hz Output: sampling rate (Hz)
 * @param out_time_us Output: total waktu (mikro-detik)
 * @param out_avg_val Output: nilai rata-rata ADC
 */
static void measure_sampling_rate(adc_atten_t atten, int num_samples,
                                   float *out_rate_hz, int64_t *out_time_us,
                                   float *out_avg_val)
{
    /* Konfigurasi atenuasi */
    adc1_config_channel_atten(ADC_CHANNEL, atten);

    /* Variabel untuk akumulasi */
    int64_t sum = 0;
    volatile int raw;  /* volatile agar compiler tidak mengoptimasi pembacaan */

    /* Catat waktu mulai (mikro-detik) */
    int64_t start_time = esp_timer_get_time();

    /* Lakukan N konversi berturut-turut */
    for (int i = 0; i < num_samples; i++) {
        raw = adc1_get_raw(ADC_CHANNEL);
        sum += raw;
    }

    /* Catat waktu selesai */
    int64_t end_time = esp_timer_get_time();

    /* Hitung hasil */
    *out_time_us = end_time - start_time;
    *out_rate_hz = (float)num_samples / (*out_time_us / 1000000.0f);
    *out_avg_val = (float)sum / num_samples;
}

void app_main(void)
{
    /* ====== INISIALISASI ADC ====== */
    adc1_config_width(ADC_WIDTH);

    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "  ADC Sampling Rate Measurement");
    ESP_LOGI(TAG, "  Channel   : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Resolusi  : 12-bit");
    ESP_LOGI(TAG, "  Konversi  : %d per pengujian", NUM_CONVERSIONS);
    ESP_LOGI(TAG, "  Pengulangan: %d kali per atenuasi", TEST_REPEAT);
    ESP_LOGI(TAG, "================================================");

    int round = 0;

    /* ====== LOOP PENGUJIAN ====== */
    while (1) {
        round++;
        printf("\n========== ROUND %d ==========\n", round);
        printf("%-10s | %-12s | %-14s | %-10s | %-8s\n",
               "Atten", "Waktu (us)", "Rate (sps)", "us/sample", "Avg ADC");
        printf("-----------+--------------+----------------+------------+----------\n");

        /* Array untuk menyimpan rata-rata rate per atenuasi */
        float avg_rates[NUM_ATTEN];

        /* Uji setiap level atenuasi */
        for (int a = 0; a < NUM_ATTEN; a++) {
            float total_rate = 0;
            float best_rate = 0;
            float worst_rate = 999999999.0f;

            /* Ulangi pengujian beberapa kali */
            for (int r = 0; r < TEST_REPEAT; r++) {
                float rate_hz;
                int64_t time_us;
                float avg_val;

                measure_sampling_rate(atten_list[a].atten, NUM_CONVERSIONS,
                                      &rate_hz, &time_us, &avg_val);

                total_rate += rate_hz;
                if (rate_hz > best_rate) best_rate = rate_hz;
                if (rate_hz < worst_rate) worst_rate = rate_hz;

                /* Tampilkan setiap percobaan */
                float us_per_sample = (float)time_us / NUM_CONVERSIONS;
                printf("  %s | %10lld   | %12.0f   | %8.2f   | %6.1f\n",
                       atten_list[a].label,
                       (long long)time_us,
                       rate_hz,
                       us_per_sample,
                       avg_val);

                vTaskDelay(pdMS_TO_TICKS(10));  /* Jeda kecil antar test */
            }

            avg_rates[a] = total_rate / TEST_REPEAT;

            /* Tampilkan ringkasan untuk atenuasi ini */
            printf("  >> %s Rata-rata: %.0f sps | Terbaik: %.0f sps | "
                   "Terburuk: %.0f sps\n",
                   atten_list[a].label, avg_rates[a], best_rate, worst_rate);
        }

        /* Ringkasan keseluruhan */
        printf("\n--- RINGKASAN ROUND %d ---\n", round);
        for (int a = 0; a < NUM_ATTEN; a++) {
            int bar_len = (int)(avg_rates[a] / 5000);  /* Skala bar */
            if (bar_len > 40) bar_len = 40;

            printf("  %s: %8.0f sps [", atten_list[a].label, avg_rates[a]);
            for (int i = 0; i < 40; i++) {
                printf(i < bar_len ? "█" : "░");
            }
            printf("]\n");
        }
        printf("---\n");

        /* Tunggu sebelum round berikutnya */
        printf("\n[INFO] Round berikutnya dalam 5 detik...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
