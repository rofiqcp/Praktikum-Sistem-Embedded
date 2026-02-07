/**
 * ==========================================================================
 * PROGRAM 05: ADC Calibration (Kalibrasi ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan penggunaan skema kalibrasi ADC pada ESP32.
 *   Program membandingkan pembacaan mentah vs terkalibrasi, menampilkan
 *   tipe kalibrasi yang digunakan (eFuse Vref, eFuse Two Point, atau
 *   Default Vref), dan menghitung persentase error.
 * 
 * Teori Kalibrasi ADC ESP32:
 *   - ADC ESP32 memiliki non-linearitas bawaan
 *   - Kalibrasi menggunakan titik referensi yang disimpan di eFuse
 *   - eFuse Two Point: paling akurat, menggunakan dua titik kalibrasi
 *   - eFuse Vref: menggunakan tegangan referensi dari eFuse
 *   - Default Vref: menggunakan nilai default 1100mV (kurang akurat)
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
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char *TAG = "ADC_CAL";

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
#define ADC_UNIT        ADC_UNIT_1
#define DEFAULT_VREF    1100
#define READ_INTERVAL_MS    1000

/* Daftar atenuasi yang akan diuji */
#define NUM_ATTEN_LEVELS    4

/* Struktur untuk menyimpan data kalibrasi per atenuasi */
typedef struct {
    adc_atten_t atten;
    const char *label;
    int voltage_range;              /* Rentang tegangan maks (mV) */
    esp_adc_cal_characteristics_t chars;
    esp_adc_cal_value_t cal_type;
} atten_config_t;

/**
 * @brief Mengembalikan string nama tipe kalibrasi
 */
static const char* get_cal_type_name(esp_adc_cal_value_t cal_type)
{
    switch (cal_type) {
        case ESP_ADC_CAL_VAL_EFUSE_TP:   return "eFuse Two Point";
        case ESP_ADC_CAL_VAL_EFUSE_VREF: return "eFuse Vref";
        case ESP_ADC_CAL_VAL_DEFAULT_VREF: return "Default Vref";
        default: return "Unknown";
    }
}

void app_main(void)
{
    /* ====== KONFIGURASI ATENUASI ====== */
    atten_config_t atten_configs[NUM_ATTEN_LEVELS] = {
        { ADC_ATTEN_DB_0,   "0dB",   1100 },    /* ~100-950 mV */
        { ADC_ATTEN_DB_2_5, "2.5dB", 1500 },    /* ~100-1250 mV */
        { ADC_ATTEN_DB_6,   "6dB",   2200 },    /* ~150-1750 mV */
        { ADC_ATTEN_DB_11,  "11dB",  3300 }     /* ~150-2450 mV (efektif ~3100mV) */
    };

    /* ====== INISIALISASI ADC ====== */
    adc1_config_width(ADC_WIDTH);

    /* Karakterisasi untuk setiap level atenuasi */
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Calibration Analysis");
    ESP_LOGI(TAG, "  Channel: ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Informasi Kalibrasi per Atenuasi ---");

    for (int i = 0; i < NUM_ATTEN_LEVELS; i++) {
        atten_configs[i].cal_type = esp_adc_cal_characterize(
            ADC_UNIT,
            atten_configs[i].atten,
            ADC_WIDTH,
            DEFAULT_VREF,
            &atten_configs[i].chars
        );

        ESP_LOGI(TAG, "  Atten %5s: Tipe=%s, Range=0-%dmV",
                 atten_configs[i].label,
                 get_cal_type_name(atten_configs[i].cal_type),
                 atten_configs[i].voltage_range);
    }

    /* ====== CEK eFUSE ====== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Status eFuse ---");

#if CONFIG_IDF_TARGET_ESP32
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "  eFuse Two Point : TERSEDIA (paling akurat)");
    } else {
        ESP_LOGW(TAG, "  eFuse Two Point : TIDAK TERSEDIA");
    }

    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "  eFuse Vref      : TERSEDIA");
    } else {
        ESP_LOGW(TAG, "  eFuse Vref      : TIDAK TERSEDIA");
    }
#else
    ESP_LOGI(TAG, "  Target non-ESP32: menggunakan kalibrasi bawaan");
#endif

    ESP_LOGI(TAG, "");

    /* Gunakan atenuasi 11dB untuk pembacaan utama */
    int main_atten_idx = 3;  /* 11dB */
    adc1_config_channel_atten(ADC_CHANNEL, atten_configs[main_atten_idx].atten);

    /* Cetak header */
    printf("\n%-6s | %-6s | %-10s | %-10s | %-8s | %-6s\n",
           "No", "Raw", "Manual(mV)", "Calib(mV)", "Error(%)", "Atten");
    printf("-------+--------+------------+------------+----------+--------\n");

    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca nilai mentah */
        int raw = adc1_get_raw(ADC_CHANNEL);

        /* Konversi manual: V = raw * Vmax / 4095 */
        float voltage_manual = raw * 3300.0f / 4095.0f;

        /* Konversi terkalibrasi */
        uint32_t voltage_cal = esp_adc_cal_raw_to_voltage(
            raw, &atten_configs[main_atten_idx].chars);

        /* Hitung persentase error */
        float error_pct = 0.0f;
        if (voltage_cal > 0) {
            error_pct = fabsf((voltage_manual - voltage_cal) / voltage_cal) * 100.0f;
        }

        counter++;
        printf("[%04d] | %4d   | %7.1f    | %7lu    | %5.2f%%   | %s\n",
               counter, raw,
               voltage_manual,
               (unsigned long)voltage_cal,
               error_pct,
               atten_configs[main_atten_idx].label);

        /* Setiap 20 pembacaan, tampilkan perbandingan semua atenuasi */
        if (counter % 20 == 0) {
            printf("\n--- Perbandingan semua atenuasi (Raw=%d) ---\n", raw);
            for (int i = 0; i < NUM_ATTEN_LEVELS; i++) {
                uint32_t v = esp_adc_cal_raw_to_voltage(
                    raw, &atten_configs[i].chars);
                printf("  Atten %5s: Kalibrasi=%4lu mV (tipe: %s)\n",
                       atten_configs[i].label,
                       (unsigned long)v,
                       get_cal_type_name(atten_configs[i].cal_type));
            }
            printf("---\n\n");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
