/*
 * ==========================================================
 *  MODUL 04 - ADC | Program 03: ADC Averaging
 *  Framework: ESP-IDF
 * ==========================================================
 *  Deskripsi:
 *    Membaca ADC1 dengan jumlah sampel yang dapat dikonfigurasi
 *    (16, 32, 64) dan menghitung statistik: rata-rata, minimum,
 *    maksimum, dan standar deviasi. Menggunakan esp_adc_cal
 *    untuk konversi tegangan terkalibrasi.
 *
 *  Hardware:
 *    - Potensiometer 10kΩ
 *      * Pin tengah  → GPIO ADC (ESP32: GPIO34, S2/S3: GPIO4)
 *      * Pin kiri    → GND
 *      * Pin kanan   → 3.3V
 *
 *  Board Support:
 *    - ESP32 DevKit   : ADC1_CHANNEL_6 (GPIO34)
 *    - ESP32-S2/S3    : ADC1_CHANNEL_3 (GPIO4)
 * ==========================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

static const char *TAG = "ADC_AVG";

/* ===================== Konfigurasi Board ===================== */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC1_CHANNEL_6      // GPIO34 pada ESP32
    #define ADC_GPIO_NUM    34
    #define BOARD_NAME      "ESP32"
#elif CONFIG_IDF_TARGET_ESP32S2
    #define ADC_CHANNEL     ADC1_CHANNEL_3      // GPIO4 pada ESP32-S2
    #define ADC_GPIO_NUM    4
    #define BOARD_NAME      "ESP32-S2"
#elif CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC1_CHANNEL_3      // GPIO4 pada ESP32-S3
    #define ADC_GPIO_NUM    4
    #define BOARD_NAME      "ESP32-S3"
#else
    #error "Board tidak didukung! Gunakan ESP32, ESP32-S2, atau ESP32-S3."
#endif

/* ===================== Konfigurasi ADC ===================== */
#define ADC_ATTEN       ADC_ATTEN_DB_12         // Range ~0-3.3V
#define ADC_WIDTH       ADC_WIDTH_BIT_12        // Resolusi 12-bit (0-4095)
#define DEFAULT_VREF    1100                    // Vref default (mV)

/* Jumlah sampel yang tersedia untuk averaging */
#define NUM_SAMPLES_16  16
#define NUM_SAMPLES_32  32
#define NUM_SAMPLES_64  64

/* Delay antar pengambilan sampel (mikrodetik) */
#define SAMPLE_DELAY_US 100

/* Struktur untuk menyimpan hasil statistik ADC */
typedef struct {
    int avg;        // Nilai rata-rata
    int min;        // Nilai minimum
    int max;        // Nilai maksimum
    float stddev;   // Standar deviasi
    int raw_last;   // Nilai raw terakhir
} adc_stats_t;

/* Variabel kalibrasi ADC */
static esp_adc_cal_characteristics_t adc_chars;

/**
 * @brief Membaca ADC dengan jumlah sampel tertentu dan menghitung statistik
 *
 * @param channel   Channel ADC1 yang akan dibaca
 * @param num_samples Jumlah sampel (16, 32, atau 64)
 * @param stats     Pointer ke struktur hasil statistik
 */
static void adc_read_with_stats(adc1_channel_t channel, int num_samples, adc_stats_t *stats)
{
    int samples[64];        // Buffer untuk menyimpan semua sampel
    long sum = 0;
    int min_val = 4095;     // Inisialisasi dengan nilai maksimum ADC 12-bit
    int max_val = 0;        // Inisialisasi dengan nilai minimum ADC 12-bit

    /* Ambil sejumlah sampel dari ADC */
    for (int i = 0; i < num_samples; i++) {
        samples[i] = adc1_get_raw(channel);
        sum += samples[i];

        /* Cari nilai minimum dan maksimum */
        if (samples[i] < min_val) min_val = samples[i];
        if (samples[i] > max_val) max_val = samples[i];

        /* Delay kecil antar pembacaan untuk stabilitas */
        ets_delay_us(SAMPLE_DELAY_US);
    }

    /* Hitung rata-rata */
    int avg = (int)(sum / num_samples);

    /* Hitung standar deviasi */
    float variance = 0.0f;
    for (int i = 0; i < num_samples; i++) {
        float diff = (float)(samples[i] - avg);
        variance += diff * diff;
    }
    variance /= num_samples;

    /* Simpan hasil statistik */
    stats->avg = avg;
    stats->min = min_val;
    stats->max = max_val;
    stats->stddev = sqrtf(variance);
    stats->raw_last = samples[num_samples - 1];
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " Modul 04 - ADC Averaging");
    ESP_LOGI(TAG, " Board: %s | GPIO: %d", BOARD_NAME, ADC_GPIO_NUM);
    ESP_LOGI(TAG, " Hardware: Potensiometer 10kΩ");
    ESP_LOGI(TAG, "========================================");

    /* --- Konfigurasi ADC1 --- */
    // Atur lebar bit ADC (resolusi 12-bit)
    adc1_config_width(ADC_WIDTH);

    // Atur atenuasi channel (range 0 - ~3.3V)
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    ESP_LOGI(TAG, "ADC dikonfigurasi: 12-bit, atenuasi 11dB");

    /* --- Kalibrasi ADC menggunakan esp_adc_cal --- */
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, DEFAULT_VREF, &adc_chars
    );

    // Tampilkan tipe kalibrasi yang digunakan
    switch (cal_type) {
        case ESP_ADC_CAL_VAL_EFUSE_VREF:
            ESP_LOGI(TAG, "Kalibrasi: eFuse Vref");
            break;
        case ESP_ADC_CAL_VAL_EFUSE_TP:
            ESP_LOGI(TAG, "Kalibrasi: eFuse Two Point");
            break;
        default:
            ESP_LOGW(TAG, "Kalibrasi: Default Vref (%d mV)", DEFAULT_VREF);
            break;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Mulai pembacaan ADC dengan averaging...");
    ESP_LOGI(TAG, "Putar potensiometer untuk melihat perubahan nilai");
    ESP_LOGI(TAG, "");

    /* Daftar jumlah sampel yang akan diuji */
    int sample_counts[] = { NUM_SAMPLES_16, NUM_SAMPLES_32, NUM_SAMPLES_64 };
    int num_configs = sizeof(sample_counts) / sizeof(sample_counts[0]);

    /* --- Loop Utama --- */
    while (1) {
        ESP_LOGI(TAG, "------------------------------------------------");

        for (int i = 0; i < num_configs; i++) {
            int n = sample_counts[i];
            adc_stats_t stats;

            /* Baca ADC dan hitung statistik */
            adc_read_with_stats(ADC_CHANNEL, n, &stats);

            /* Konversi rata-rata ke tegangan menggunakan kalibrasi */
            uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(stats.avg, &adc_chars);

            /* Tampilkan hasil statistik */
            ESP_LOGI(TAG, "[N=%2d] Raw: %4d | Avg: %4d | Min: %4d | Max: %4d | "
                     "StdDev: %5.1f | Tegangan: %lu mV",
                     n, stats.raw_last, stats.avg, stats.min, stats.max,
                     stats.stddev, (unsigned long)voltage_mv);
        }

        ESP_LOGI(TAG, "");

        /* Delay 1 detik sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
