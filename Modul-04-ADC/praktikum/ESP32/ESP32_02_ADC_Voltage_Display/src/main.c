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
 *   2. Konversi terkalibrasi menggunakan esp_adc_cal
 *   Kedua hasil ditampilkan untuk perbandingan.
 * 
 * Koneksi Hardware:
 *   - Potensiometer: VCC → 3.3V, GND → GND, Wiper → GPIO34
 * 
 * API yang digunakan:
 *   - esp_adc_cal_characterize()  : Kalibrasi karakteristik ADC
 *   - esp_adc_cal_raw_to_voltage(): Konversi raw ke tegangan terkalibrasi
 * ==========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char *TAG = "ADC_VOLTAGE";

/* Konfigurasi Channel ADC */
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
#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_UNIT        ADC_UNIT_1

/* Tegangan referensi default (mV) */
#define DEFAULT_VREF    1100

/* Interval pembacaan */
#define READ_INTERVAL_MS    500

/* Variabel global untuk kalibrasi */
static esp_adc_cal_characteristics_t *adc_chars;

/**
 * @brief Mengecek tipe kalibrasi eFuse yang tersedia
 * 
 * ESP32 mendukung beberapa tipe kalibrasi:
 * - Two Point: Kalibrasi dua titik (paling akurat)
 * - Vref: Tegangan referensi dari eFuse
 * - Default Vref: Menggunakan nilai default (kurang akurat)
 */
static void check_efuse_calibration(void)
{
#if CONFIG_IDF_TARGET_ESP32
    /* Cek apakah TP (Two Point) tersedia di eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi eFuse Two Point: TERSEDIA");
    } else {
        ESP_LOGW(TAG, "Kalibrasi eFuse Two Point: TIDAK TERSEDIA");
    }

    /* Cek apakah Vref tersedia di eFuse */
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi eFuse Vref: TERSEDIA");
    } else {
        ESP_LOGW(TAG, "Kalibrasi eFuse Vref: TIDAK TERSEDIA");
    }
#else
    ESP_LOGI(TAG, "Menggunakan kalibrasi default untuk target ini");
#endif
}

/**
 * @brief Mengembalikan string nama tipe kalibrasi
 */
static const char* get_cal_type_name(esp_adc_cal_value_t cal_type)
{
    switch (cal_type) {
        case ESP_ADC_CAL_VAL_EFUSE_TP:
            return "eFuse Two Point";
        case ESP_ADC_CAL_VAL_EFUSE_VREF:
            return "eFuse Vref";
        case ESP_ADC_CAL_VAL_DEFAULT_VREF:
            return "Default Vref";
        default:
            return "Unknown";
    }
}

void app_main(void)
{
    /* ====== CEK KALIBRASI eFUSE ====== */
    check_efuse_calibration();

    /* ====== KONFIGURASI ADC ====== */
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    /* ====== KARAKTERISASI ADC (KALIBRASI) ====== */
    /* Alokasi memori untuk struktur kalibrasi */
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    
    /* Lakukan kalibrasi dan dapatkan tipe yang digunakan */
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT,           /* Unit ADC (ADC1) */
        ADC_ATTEN,          /* Atenuasi */
        ADC_WIDTH,          /* Resolusi */
        DEFAULT_VREF,       /* Vref default jika eFuse tidak tersedia */
        adc_chars           /* Struktur output kalibrasi */
    );

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Voltage Display");
    ESP_LOGI(TAG, "  Channel  : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Kalibrasi: %s", get_cal_type_name(cal_type));
    ESP_LOGI(TAG, "========================================");

    /* ====== LOOP PEMBACAAN ====== */
    int counter = 0;

    while (1) {
        /* Baca nilai mentah ADC */
        int raw_value = adc1_get_raw(ADC_CHANNEL);

        /* Metode 1: Konversi manual (tanpa kalibrasi) */
        /* Rumus: V(mV) = raw × 3300 / 4095 */
        uint32_t voltage_manual = (uint32_t)((raw_value * 3300.0) / 4095.0);

        /* Metode 2: Konversi terkalibrasi menggunakan esp_adc_cal */
        uint32_t voltage_calibrated = 0;
        voltage_calibrated = esp_adc_cal_raw_to_voltage(raw_value, adc_chars);

        /* Hitung selisih antara kedua metode */
        int32_t difference = (int32_t)voltage_calibrated - (int32_t)voltage_manual;

        /* Tampilkan hasil */
        counter++;
        printf("[%04d] Raw: %4d | Manual: %4lu mV (%5.3f V) | "
               "Kalibrasi: %4lu mV (%5.3f V) | Selisih: %+ld mV\n",
               counter,
               raw_value,
               (unsigned long)voltage_manual,
               voltage_manual / 1000.0f,
               (unsigned long)voltage_calibrated,
               voltage_calibrated / 1000.0f,
               (long)difference);

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
