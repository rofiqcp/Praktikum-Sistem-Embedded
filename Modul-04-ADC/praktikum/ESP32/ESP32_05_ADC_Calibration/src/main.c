/**
 * ==========================================================================
 * PROGRAM 05: ADC Calibration (Kalibrasi ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan penggunaan skema kalibrasi ADC pada ESP32.
 *   Program membandingkan pembacaan mentah vs terkalibrasi, menampilkan
 *   tipe kalibrasi yang digunakan (Curve Fitting atau Line Fitting),
 *   dan menghitung persentase error.
 * 
 * Teori Kalibrasi ADC ESP32:
 *   - ADC ESP32 memiliki non-linearitas bawaan
 *   - Kalibrasi menggunakan titik referensi yang disimpan di eFuse
 *   - Curve Fitting: paling akurat, menggunakan kurva polinomial
 *   - Line Fitting: menggunakan regresi linear (untuk chip yang tidak
 *     mendukung curve fitting)
 * 
 * Koneksi Hardware:
 *   - Potensiometer: VCC → 3.3V, GND → GND, Wiper → GPIO34
 * ==========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "ADC_CAL";

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

#define ADC_UNIT        ADC_UNIT_1
#define READ_INTERVAL_MS    1000

/* Daftar atenuasi yang akan diuji */
#define NUM_ATTEN_LEVELS    4

/* Struktur untuk menyimpan data kalibrasi per atenuasi */
typedef struct {
    adc_atten_t atten;
    const char *label;
    int voltage_range;              /* Rentang tegangan maks (mV) */
    adc_cali_handle_t cali_handle;  /* Handle kalibrasi */
    bool calibrated;                /* Status kalibrasi berhasil */
    const char *cal_scheme;         /* Nama skema kalibrasi */
} atten_config_t;

/**
 * @brief Membuat handle kalibrasi ADC untuk atenuasi tertentu
 * 
 * @param unit     Unit ADC
 * @param atten    Level atenuasi
 * @param out_handle  Pointer ke handle kalibrasi (output)
 * @param out_scheme  Pointer ke nama skema (output)
 * @return true jika kalibrasi berhasil
 */
static bool create_calibration(adc_unit_t unit, adc_atten_t atten,
                               adc_cali_handle_t *out_handle,
                               const char **out_scheme)
{
    adc_cali_handle_t handle = NULL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &handle) == ESP_OK) {
        calibrated = true;
        *out_scheme = "Curve Fitting";
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if (adc_cali_create_scheme_line_fitting(&cali_config, &handle) == ESP_OK) {
            calibrated = true;
            *out_scheme = "Line Fitting";
        }
    }
#endif

    *out_handle = handle;
    return calibrated;
}

void app_main(void)
{
    /* ====== KONFIGURASI ATENUASI ====== */
    atten_config_t atten_configs[NUM_ATTEN_LEVELS] = {
        { ADC_ATTEN_DB_0,   "0dB",   1100, NULL, false, "N/A" },  /* ~100-950 mV */
        { ADC_ATTEN_DB_2_5, "2.5dB", 1500, NULL, false, "N/A" },  /* ~100-1250 mV */
        { ADC_ATTEN_DB_6,   "6dB",   2200, NULL, false, "N/A" },  /* ~150-1750 mV */
        { ADC_ATTEN_DB_12,  "12dB",  3300, NULL, false, "N/A" }   /* ~150-2450 mV (efektif ~3100mV) */
    };

    /* ====== INISIALISASI ADC ONESHOT ====== */
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    /* Konfigurasi channel dengan atenuasi 12dB untuk pembacaan utama */
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    /* Karakterisasi (kalibrasi) untuk setiap level atenuasi */
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Calibration Analysis");
    ESP_LOGI(TAG, "  Channel: ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Informasi Kalibrasi per Atenuasi ---");

    for (int i = 0; i < NUM_ATTEN_LEVELS; i++) {
        atten_configs[i].calibrated = create_calibration(
            ADC_UNIT,
            atten_configs[i].atten,
            &atten_configs[i].cali_handle,
            &atten_configs[i].cal_scheme
        );

        ESP_LOGI(TAG, "  Atten %5s: Skema=%s, Range=0-%dmV, Status=%s",
                 atten_configs[i].label,
                 atten_configs[i].cal_scheme,
                 atten_configs[i].voltage_range,
                 atten_configs[i].calibrated ? "OK" : "GAGAL");
    }

    /* ====== CEK SKEMA KALIBRASI ====== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Status Skema Kalibrasi ---");

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "  Curve Fitting : DIDUKUNG (paling akurat)");
#else
    ESP_LOGW(TAG, "  Curve Fitting : TIDAK DIDUKUNG");
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "  Line Fitting  : DIDUKUNG");
#else
    ESP_LOGW(TAG, "  Line Fitting  : TIDAK DIDUKUNG");
#endif

    ESP_LOGI(TAG, "");

    /* Gunakan atenuasi 12dB untuk pembacaan utama */
    int main_atten_idx = 3;  /* 12dB */

    /* Cetak header */
    printf("\n%-6s | %-6s | %-10s | %-10s | %-8s | %-6s\n",
           "No", "Raw", "Manual(mV)", "Calib(mV)", "Error(%)", "Atten");
    printf("-------+--------+------------+------------+----------+--------\n");

    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca nilai mentah */
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));

        /* Konversi manual: V = raw * Vmax / 4095 */
        float voltage_manual = raw * 3300.0f / 4095.0f;

        /* Konversi terkalibrasi */
        int voltage_cal = 0;
        if (atten_configs[main_atten_idx].calibrated) {
            adc_cali_raw_to_voltage(atten_configs[main_atten_idx].cali_handle,
                                    raw, &voltage_cal);
        }

        /* Hitung persentase error */
        float error_pct = 0.0f;
        if (voltage_cal > 0) {
            error_pct = fabsf((voltage_manual - voltage_cal) / voltage_cal) * 100.0f;
        }

        counter++;
        printf("[%04d] | %4d   | %7.1f    | %7d    | %5.2f%%   | %s\n",
               counter, raw,
               voltage_manual,
               voltage_cal,
               error_pct,
               atten_configs[main_atten_idx].label);

        /* Setiap 20 pembacaan, tampilkan perbandingan semua atenuasi */
        if (counter % 20 == 0) {
            printf("\n--- Perbandingan semua atenuasi (Raw=%d) ---\n", raw);
            for (int i = 0; i < NUM_ATTEN_LEVELS; i++) {
                int v = 0;
                if (atten_configs[i].calibrated) {
                    adc_cali_raw_to_voltage(atten_configs[i].cali_handle,
                                            raw, &v);
                }
                printf("  Atten %5s: Kalibrasi=%4d mV (skema: %s)\n",
                       atten_configs[i].label,
                       v,
                       atten_configs[i].cal_scheme);
            }
            printf("---\n\n");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
