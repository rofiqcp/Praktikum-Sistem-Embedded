/**
 * ==========================================================================
 * PROGRAM 02: ADC Voltage Display (Tampilan Tegangan ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca nilai ADC dan mengkonversi ke tegangan (mV).
 *   Dua metode konversi digunakan:
 *   1. Konversi manual: V = raw × 3300 / 4095 (mV)
 *   2. Konversi terkalibrasi menggunakan ADC Calibration API (esp_adc/adc_cali.h)
 *   Kedua hasil ditampilkan untuk perbandingan.
 * 
 * Koneksi Hardware:
 *   - Potensiometer: VCC → 3.3V, GND → GND, Wiper → GPIO34
 * 
 * API yang digunakan (ESP-IDF v5.x Oneshot + Calibration API):
 *   - adc_oneshot_new_unit()                    : Membuat unit handle ADC
 *   - adc_oneshot_config_channel()              : Konfigurasi channel
 *   - adc_oneshot_read()                        : Membaca nilai mentah ADC
 *   - adc_cali_create_scheme_curve_fitting()    : Buat handle kalibrasi (ESP32)
 *   - adc_cali_create_scheme_line_fitting()     : Buat handle kalibrasi (ESP32-S2/S3/C3)
 *   - adc_cali_raw_to_voltage()                 : Konversi raw ke tegangan terkalibrasi
 * ==========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "ADC_VOLTAGE";

/* Konfigurasi Channel ADC */
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

/* Interval pembacaan */
#define READ_INTERVAL_MS    500

/* Flag apakah kalibrasi berhasil */
static bool cali_enabled = false;

/**
 * @brief Inisialisasi kalibrasi ADC
 * 
 * ESP32 mendukung curve fitting, ESP32-S2/C3 mendukung line fitting.
 * Fungsi ini otomatis memilih skema yang sesuai berdasarkan target.
 * 
 * @param unit Unit ADC (ADC_UNIT_1)
 * @param atten Atenuasi yang digunakan
 * @param out_handle Pointer ke handle kalibrasi (output)
 * @return true jika kalibrasi berhasil
 */
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    /* Skema Curve Fitting (tersedia di ESP32, ESP32-S2) */
    ESP_LOGI(TAG, "Menggunakan skema kalibrasi: Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    /* Skema Line Fitting (tersedia di ESP32-C3, ESP32-S3, dll.) */
    ESP_LOGI(TAG, "Menggunakan skema kalibrasi: Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi ADC berhasil diinisialisasi");
        *out_handle = handle;
        return true;
    } else if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Skema kalibrasi tidak didukung, hasil tanpa kalibrasi");
    } else {
        ESP_LOGE(TAG, "Kalibrasi gagal: %s", esp_err_to_name(ret));
    }

    return false;
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

    /* ====== KALIBRASI ADC ====== */
    adc_cali_handle_t cali_handle = NULL;
    cali_enabled = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN, &cali_handle);

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Voltage Display");
    ESP_LOGI(TAG, "  Channel  : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Kalibrasi: %s", cali_enabled ? "TERSEDIA" : "TIDAK TERSEDIA");
    ESP_LOGI(TAG, "========================================");

    /* ====== LOOP PEMBACAAN ====== */
    int counter = 0;

    while (1) {
        /* Baca nilai mentah ADC */
        int raw_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));

        /* Metode 1: Konversi manual (tanpa kalibrasi) */
        /* Rumus: V(mV) = raw × 3300 / 4095 */
        uint32_t voltage_manual = (uint32_t)((raw_value * 3300.0) / 4095.0);

        /* Metode 2: Konversi terkalibrasi menggunakan ADC Calibration API */
        int voltage_calibrated = 0;
        if (cali_enabled) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage_calibrated));
        }

        /* Hitung selisih antara kedua metode */
        int32_t difference = (int32_t)voltage_calibrated - (int32_t)voltage_manual;

        /* Tampilkan hasil */
        counter++;
        printf("[%04d] Raw: %4d | Manual: %4lu mV (%5.3f V) | "
               "Kalibrasi: %4d mV (%5.3f V) | Selisih: %+ld mV\n",
               counter,
               raw_value,
               (unsigned long)voltage_manual,
               voltage_manual / 1000.0f,
               voltage_calibrated,
               voltage_calibrated / 1000.0f,
               (long)difference);

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
