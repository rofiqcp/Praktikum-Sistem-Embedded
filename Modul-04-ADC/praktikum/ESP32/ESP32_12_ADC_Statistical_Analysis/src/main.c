/**
 * ==========================================================================
 * PROGRAM 12: ADC Statistical Analysis (Analisis Statistik ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini melakukan analisis statistik lengkap pada pembacaan ADC:
 *   1. Mengumpulkan N=100 sampel
 *   2. Menghitung: min, max, rata-rata, standar deviasi
 *   3. Menampilkan histogram (10 bin)
 *   4. Menghitung SNR (Signal-to-Noise Ratio)
 * 
 *   SNR = 20 × log10(rata-rata / standar_deviasi) [dB]
 * 
 *   Semakin tinggi SNR, semakin baik kualitas sinyal ADC.
 *   SNR ideal untuk ADC 12-bit: ~74 dB
 * 
 * Koneksi Hardware:
 *   - Potensiometer: Wiper → GPIO34
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "ADC_STATS";

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

/* Konfigurasi analisis */
#define NUM_SAMPLES     100     /* Jumlah sampel per analisis */
#define NUM_BINS        10      /* Jumlah bin histogram */
#define HISTOGRAM_WIDTH 40      /* Lebar visual histogram */

/* Interval antar batch analisis (ms) */
#define ANALYSIS_INTERVAL_MS    3000

/* Handle ADC dan kalibrasi */
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;
static bool cali_ok = false;

/**
 * @brief Inisialisasi kalibrasi ADC
 * 
 * Menggunakan curve fitting (ESP32, ESP32S2) atau line fitting (ESP32C3, dll.)
 * secara otomatis berdasarkan dukungan platform.
 * 
 * @return true jika kalibrasi berhasil
 */
static bool adc_calibration_init(void)
{
    esp_err_t ret;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Kalibrasi: menggunakan Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Kalibrasi: menggunakan Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
#else
    ESP_LOGW(TAG, "Kalibrasi tidak didukung pada platform ini");
    return false;
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi ADC berhasil");
        return true;
    } else {
        ESP_LOGW(TAG, "Kalibrasi ADC gagal: %s", esp_err_to_name(ret));
        return false;
    }
}

/**
 * @brief Struktur untuk menyimpan hasil analisis statistik
 */
typedef struct {
    int samples[NUM_SAMPLES];   /* Array sampel */
    int min_val;                /* Nilai minimum */
    int max_val;                /* Nilai maksimum */
    float mean;                 /* Rata-rata (mean) */
    float std_dev;              /* Standar deviasi */
    float variance;             /* Varians */
    float snr_db;               /* Signal-to-Noise Ratio (dB) */
    int range;                  /* Range (max - min) */
    float median;               /* Median */
    int histogram[NUM_BINS];    /* Histogram bins */
    int bin_min;                /* Nilai minimum bin */
    int bin_max;                /* Nilai maksimum bin */
    float bin_width;            /* Lebar setiap bin */
} stats_result_t;

/**
 * @brief Fungsi pembanding untuk qsort
 */
static int compare_int(const void *a, const void *b)
{
    return (*(int*)a - *(int*)b);
}

/**
 * @brief Kumpulkan sampel dan hitung statistik
 */
static void collect_and_analyze(stats_result_t *result)
{
    /* ====== PENGUMPULAN SAMPEL ====== */
    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &result->samples[i]);
        vTaskDelay(pdMS_TO_TICKS(2));  /* Delay kecil antar sampel */
    }

    /* ====== HITUNG MIN, MAX ====== */
    result->min_val = result->samples[0];
    result->max_val = result->samples[0];
    float sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        if (result->samples[i] < result->min_val) {
            result->min_val = result->samples[i];
        }
        if (result->samples[i] > result->max_val) {
            result->max_val = result->samples[i];
        }
        sum += result->samples[i];
    }

    /* ====== HITUNG RATA-RATA (MEAN) ====== */
    result->mean = sum / NUM_SAMPLES;
    result->range = result->max_val - result->min_val;

    /* ====== HITUNG VARIANS DAN STANDAR DEVIASI ====== */
    /* Varians = Σ(xi - mean)² / (N - 1)  [sample variance] */
    float var_sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float diff = result->samples[i] - result->mean;
        var_sum += diff * diff;
    }
    result->variance = var_sum / (NUM_SAMPLES - 1);
    result->std_dev = sqrtf(result->variance);

    /* ====== HITUNG MEDIAN ====== */
    /* Salin dan urutkan array untuk mencari median */
    int sorted[NUM_SAMPLES];
    memcpy(sorted, result->samples, sizeof(sorted));
    qsort(sorted, NUM_SAMPLES, sizeof(int), compare_int);

    if (NUM_SAMPLES % 2 == 0) {
        result->median = (sorted[NUM_SAMPLES/2 - 1] + sorted[NUM_SAMPLES/2]) / 2.0f;
    } else {
        result->median = sorted[NUM_SAMPLES/2];
    }

    /* ====== HITUNG SNR (Signal-to-Noise Ratio) ====== */
    /* SNR = 20 × log10(mean / std_dev) dalam dB */
    if (result->std_dev > 0 && result->mean > 0) {
        result->snr_db = 20.0f * log10f(result->mean / result->std_dev);
    } else {
        result->snr_db = 0.0f;
    }

    /* ====== BUAT HISTOGRAM ====== */
    memset(result->histogram, 0, sizeof(result->histogram));
    result->bin_min = result->min_val;
    result->bin_max = result->max_val;

    /* Hindari pembagian nol jika semua nilai sama */
    if (result->range == 0) {
        result->bin_width = 1.0f;
        result->histogram[NUM_BINS / 2] = NUM_SAMPLES;
    } else {
        result->bin_width = (float)result->range / NUM_BINS;

        for (int i = 0; i < NUM_SAMPLES; i++) {
            int bin = (int)((result->samples[i] - result->min_val) / result->bin_width);
            if (bin >= NUM_BINS) bin = NUM_BINS - 1;
            if (bin < 0) bin = 0;
            result->histogram[bin]++;
        }
    }
}

/**
 * @brief Tampilkan hasil analisis
 */
static void print_analysis(stats_result_t *result, int batch_num)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║         ANALISIS STATISTIK ADC - Batch #%04d           ║\n", batch_num);
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Jumlah Sampel  : %-6d                                ║\n", NUM_SAMPLES);
    printf("║ Nilai Minimum  : %-6d                                ║\n", result->min_val);
    printf("║ Nilai Maksimum : %-6d                                ║\n", result->max_val);
    printf("║ Range          : %-6d                                ║\n", result->range);
    printf("║ Rata-rata      : %-10.2f                            ║\n", result->mean);
    printf("║ Median         : %-10.2f                            ║\n", result->median);
    printf("║ Varians        : %-10.2f                            ║\n", result->variance);
    printf("║ Standar Deviasi: %-10.4f                            ║\n", result->std_dev);
    printf("║ SNR            : %-10.2f dB                         ║\n", result->snr_db);
    printf("╠══════════════════════════════════════════════════════════╣\n");

    /* Konversi tegangan menggunakan kalibrasi baru */
    int v_mean = 0, v_min = 0, v_max = 0;
    if (cali_ok) {
        adc_cali_raw_to_voltage(cali_handle, (int)result->mean, &v_mean);
        adc_cali_raw_to_voltage(cali_handle, result->min_val, &v_min);
        adc_cali_raw_to_voltage(cali_handle, result->max_val, &v_max);
    }
    printf("║ Tegangan Mean  : %-6d mV                             ║\n", v_mean);
    printf("║ Tegangan Min   : %-6d mV                             ║\n", v_min);
    printf("║ Tegangan Max   : %-6d mV                             ║\n", v_max);
    printf("╠══════════════════════════════════════════════════════════╣\n");

    /* Histogram */
    printf("║                    HISTOGRAM                           ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");

    /* Cari frekuensi maksimum untuk normalisasi visual */
    int max_freq = 0;
    for (int i = 0; i < NUM_BINS; i++) {
        if (result->histogram[i] > max_freq) {
            max_freq = result->histogram[i];
        }
    }

    for (int i = 0; i < NUM_BINS; i++) {
        int bin_start = result->bin_min + (int)(i * result->bin_width);
        int bin_end = result->bin_min + (int)((i + 1) * result->bin_width);

        /* Normalisasi panjang bar */
        int bar_len = 0;
        if (max_freq > 0) {
            bar_len = result->histogram[i] * HISTOGRAM_WIDTH / max_freq;
        }

        printf("║ %4d-%4d |", bin_start, bin_end);
        for (int j = 0; j < HISTOGRAM_WIDTH; j++) {
            printf(j < bar_len ? "█" : " ");
        }
        printf("| %2d ║\n", result->histogram[i]);
    }

    printf("╠══════════════════════════════════════════════════════════╣\n");

    /* Evaluasi kualitas */
    const char *quality;
    if (result->snr_db > 60)      quality = "SANGAT BAIK";
    else if (result->snr_db > 40) quality = "BAIK";
    else if (result->snr_db > 20) quality = "CUKUP";
    else                          quality = "BURUK";

    printf("║ Kualitas Sinyal: %-15s                       ║\n", quality);
    printf("║ (SNR ideal 12-bit: ~74 dB)                             ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    /* Cetak data mentah untuk parsing oleh Python */
    printf("DATA_RAW:");
    for (int i = 0; i < NUM_SAMPLES; i++) {
        printf("%d", result->samples[i]);
        if (i < NUM_SAMPLES - 1) printf(",");
    }
    printf("\n");

    printf("STATS:min=%d,max=%d,mean=%.2f,std=%.4f,snr=%.2f,median=%.2f\n",
           result->min_val, result->max_val, result->mean,
           result->std_dev, result->snr_db, result->median);
}

void app_main(void)
{
    /* ====== INISIALISASI ADC ONESHOT ====== */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    /* Inisialisasi kalibrasi */
    cali_ok = adc_calibration_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Statistical Analysis");
    ESP_LOGI(TAG, "  Channel : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Sampel  : %d per batch", NUM_SAMPLES);
    ESP_LOGI(TAG, "  Bins    : %d", NUM_BINS);
    ESP_LOGI(TAG, "  Kalibrasi: %s", cali_ok ? "Ya" : "Tidak");
    ESP_LOGI(TAG, "========================================");

    stats_result_t result;
    int batch = 0;

    /* ====== LOOP ANALISIS ====== */
    while (1) {
        batch++;

        ESP_LOGI(TAG, "Mengumpulkan %d sampel untuk batch #%d...", NUM_SAMPLES, batch);

        /* Kumpulkan dan analisis */
        collect_and_analyze(&result);

        /* Tampilkan hasil */
        print_analysis(&result, batch);

        /* Tunggu sebelum batch berikutnya */
        ESP_LOGI(TAG, "Batch berikutnya dalam %d detik...", ANALYSIS_INTERVAL_MS / 1000);
        vTaskDelay(pdMS_TO_TICKS(ANALYSIS_INTERVAL_MS));
    }
}
