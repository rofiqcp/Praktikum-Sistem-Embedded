/**
 * ==========================================================================
 * PROGRAM 03: ADC Moving Average (Filter Rata-Rata Bergerak)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini mengimplementasikan filter moving average menggunakan
 *   circular buffer untuk meredam noise pada pembacaan ADC.
 *   Dua ukuran window digunakan: N=16 dan N=32 untuk perbandingan.
 *   Program juga menghitung persentase pengurangan noise.
 * 
 * Konsep Moving Average:
 *   - Circular buffer menyimpan N sampel terakhir
 *   - Rata-rata dihitung dari seluruh buffer
 *   - Semakin besar N, semakin halus sinyal tetapi semakin lambat respons
 * 
 * Koneksi Hardware:
 *   - Potensiometer: VCC → 3.3V, GND → GND, Wiper → GPIO34
 * 
 * API yang digunakan (ESP-IDF v5.x Oneshot API):
 *   - adc_oneshot_new_unit()       : Membuat unit handle ADC
 *   - adc_oneshot_config_channel() : Mengatur konfigurasi channel
 *   - adc_oneshot_read()           : Membaca nilai mentah ADC (single-shot)
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "ADC_MAVG";

/* Konfigurasi ADC */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC_CHANNEL_6
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC_CHANNEL_3
    #define ADC_GPIO_NUM    4
#else
    #define ADC_CHANNEL     ADC_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

#define ADC_ATTEN       ADC_ATTEN_DB_12
#define READ_INTERVAL_MS    50  /* Pembacaan cepat untuk melihat efek filter */

/* Ukuran window moving average */
#define WINDOW_SIZE_16  16
#define WINDOW_SIZE_32  32
#define MAX_WINDOW_SIZE 32  /* Ukuran maksimum buffer */

/* Jumlah sampel untuk kalkulasi noise */
#define NOISE_CALC_SAMPLES  64

/**
 * @brief Struktur circular buffer untuk moving average
 * 
 * Circular buffer adalah array yang digunakan secara melingkar.
 * Ketika index mencapai akhir array, ia kembali ke awal.
 */
typedef struct {
    int buffer[MAX_WINDOW_SIZE];    /* Array penyimpan sampel */
    int head;                       /* Posisi penulisan berikutnya */
    int sum;                        /* Jumlah total semua elemen dalam buffer */
    int count;                      /* Jumlah elemen yang sudah diisi */
    int window_size;                /* Ukuran window yang digunakan */
} moving_avg_t;

/**
 * @brief Inisialisasi struktur moving average
 * @param ma Pointer ke struktur moving average
 * @param window_size Ukuran window filter
 */
static void moving_avg_init(moving_avg_t *ma, int window_size)
{
    memset(ma->buffer, 0, sizeof(ma->buffer));
    ma->head = 0;
    ma->sum = 0;
    ma->count = 0;
    ma->window_size = (window_size > MAX_WINDOW_SIZE) ? MAX_WINDOW_SIZE : window_size;
}

/**
 * @brief Tambahkan sampel baru dan hitung rata-rata
 * 
 * Algoritma:
 * 1. Kurangi nilai lama dari sum (jika buffer sudah penuh)
 * 2. Tambahkan nilai baru ke buffer dan sum
 * 3. Geser head ke posisi berikutnya (melingkar)
 * 4. Hitung rata-rata = sum / count
 * 
 * @param ma Pointer ke struktur moving average
 * @param new_value Nilai sampel baru
 * @return Nilai rata-rata bergerak
 */
static int moving_avg_add(moving_avg_t *ma, int new_value)
{
    /* Jika buffer sudah penuh, kurangi nilai lama dari sum */
    if (ma->count >= ma->window_size) {
        ma->sum -= ma->buffer[ma->head];
    } else {
        ma->count++;
    }

    /* Simpan nilai baru di posisi head */
    ma->buffer[ma->head] = new_value;
    ma->sum += new_value;

    /* Geser head ke posisi berikutnya (melingkar) */
    ma->head = (ma->head + 1) % ma->window_size;

    /* Kembalikan rata-rata */
    return ma->sum / ma->count;
}

/**
 * @brief Hitung standar deviasi dari array nilai
 * @param values Array nilai
 * @param count Jumlah elemen
 * @return Standar deviasi
 */
static float calculate_std_dev(int *values, int count)
{
    if (count <= 1) return 0.0f;

    /* Hitung rata-rata */
    float mean = 0.0f;
    for (int i = 0; i < count; i++) {
        mean += values[i];
    }
    mean /= count;

    /* Hitung varians */
    float variance = 0.0f;
    for (int i = 0; i < count; i++) {
        float diff = values[i] - mean;
        variance += diff * diff;
    }
    variance /= (count - 1);

    return sqrtf(variance);
}

void app_main(void)
{
    /* ====== KONFIGURASI ADC (Oneshot) ====== */
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Moving Average Filter");
    ESP_LOGI(TAG, "  Channel: ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Window : N=16 dan N=32");
    ESP_LOGI(TAG, "========================================");

    /* ====== INISIALISASI FILTER ====== */
    moving_avg_t filter_16, filter_32;
    moving_avg_init(&filter_16, WINDOW_SIZE_16);
    moving_avg_init(&filter_32, WINDOW_SIZE_32);

    /* Buffer untuk kalkulasi noise */
    int raw_samples[NOISE_CALC_SAMPLES];
    int avg16_samples[NOISE_CALC_SAMPLES];
    int avg32_samples[NOISE_CALC_SAMPLES];
    int noise_idx = 0;
    int noise_ready = 0;

    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca nilai mentah ADC */
        int raw_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));

        /* Terapkan filter moving average */
        int avg_16 = moving_avg_add(&filter_16, raw_value);
        int avg_32 = moving_avg_add(&filter_32, raw_value);

        /* Simpan sampel untuk kalkulasi noise */
        raw_samples[noise_idx] = raw_value;
        avg16_samples[noise_idx] = avg_16;
        avg32_samples[noise_idx] = avg_32;
        noise_idx = (noise_idx + 1) % NOISE_CALC_SAMPLES;
        if (noise_idx == 0) noise_ready = 1;

        /* Tampilkan hasil setiap pembacaan */
        counter++;
        printf("[%04d] Raw: %4d | MA-16: %4d | MA-32: %4d",
               counter, raw_value, avg_16, avg_32);

        /* Hitung dan tampilkan noise reduction setiap siklus penuh */
        if (noise_ready && (counter % NOISE_CALC_SAMPLES == 0)) {
            float std_raw = calculate_std_dev(raw_samples, NOISE_CALC_SAMPLES);
            float std_16 = calculate_std_dev(avg16_samples, NOISE_CALC_SAMPLES);
            float std_32 = calculate_std_dev(avg32_samples, NOISE_CALC_SAMPLES);

            float reduction_16 = (std_raw > 0) ? 
                ((std_raw - std_16) / std_raw * 100.0f) : 0.0f;
            float reduction_32 = (std_raw > 0) ? 
                ((std_raw - std_32) / std_raw * 100.0f) : 0.0f;

            printf(" | Noise: Raw=%.1f, N16=%.1f (-%0.1f%%), N32=%.1f (-%0.1f%%)",
                   std_raw, std_16, reduction_16, std_32, reduction_32);
        }

        printf("\n");

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
