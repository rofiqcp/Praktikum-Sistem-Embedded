/*
 * ==========================================================
 * Program 01: ADC Basic - ESP32 (ESP-IDF)
 * Modul 04 - Praktikum Sistem Embedded
 * ==========================================================
 * Deskripsi:
 *   Membaca nilai ADC dari potensiometer pada kanal ADC1,
 *   mengkonversi nilai mentah ke tegangan (mV), dan
 *   menampilkan hasilnya melalui UART setiap 500ms.
 *
 * Hardware:
 *   - Potensiometer 10kΩ
 *   - Kabel jumper
 *
 * Koneksi Pin:
 *   - ESP32    : GPIO34 (ADC1_CHANNEL_6)
 *   - ESP32-S2 : GPIO4  (ADC1_CHANNEL_3)
 *   - ESP32-S3 : GPIO4  (ADC1_CHANNEL_3)
 *
 *   Potensiometer:
 *     Pin 1 -> 3.3V
 *     Pin 2 -> GPIO ADC (lihat di atas)
 *     Pin 3 -> GND
 * ==========================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

/* Tag untuk logging ESP-IDF */
static const char *TAG = "ADC_BASIC";

/* --------------------------------------------------------
 * Pemilihan kanal ADC berdasarkan varian board ESP32
 * Setiap varian memiliki mapping GPIO-ke-kanal yang berbeda
 * -------------------------------------------------------- */
#if CONFIG_IDF_TARGET_ESP32
    /* ESP32 klasik: GPIO34 = ADC1 Channel 6 (input-only pin) */
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2
    /* ESP32-S2: GPIO4 = ADC1 Channel 3 */
    #define ADC_CHANNEL     ADC1_CHANNEL_3
    #define ADC_GPIO_NUM    4
#elif CONFIG_IDF_TARGET_ESP32S3
    /* ESP32-S3: GPIO4 = ADC1 Channel 3 */
    #define ADC_CHANNEL     ADC1_CHANNEL_3
    #define ADC_GPIO_NUM    4
#else
    /* Default: gunakan channel 6 untuk board lain */
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

/* Konfigurasi ADC */
#define ADC_WIDTH       ADC_WIDTH_BIT_12      /* Resolusi 12-bit (0-4095) */
#define ADC_ATTEN       ADC_ATTEN_DB_11       /* Atenuasi 11dB -> range ~0-3.3V */
#define DEFAULT_VREF    1100                  /* Tegangan referensi default (mV) */
#define NUM_SAMPLES     64                    /* Jumlah sampel untuk multisampling */

void app_main(void)
{
    /* ====================================================
     * 1. Konfigurasi lebar bit ADC1
     *    ADC_WIDTH_BIT_12 -> resolusi 12-bit (0 s.d. 4095)
     * ==================================================== */
    adc1_config_width(ADC_WIDTH);

    /* ====================================================
     * 2. Konfigurasi atenuasi pada kanal ADC
     *    ADC_ATTEN_DB_11 -> rentang pengukuran ~0-3.3V
     *    Pilihan lain:
     *      ADC_ATTEN_DB_0   -> ~0-1.1V
     *      ADC_ATTEN_DB_2_5 -> ~0-1.5V
     *      ADC_ATTEN_DB_6   -> ~0-2.2V
     * ==================================================== */
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    /* ====================================================
     * 3. Karakterisasi ADC untuk kalibrasi
     *    Menggunakan esp_adc_cal untuk konversi yang lebih
     *    akurat dari nilai mentah ke tegangan (mV)
     * ==================================================== */
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1,         /* Menggunakan ADC1 */
        ADC_ATTEN,          /* Atenuasi yang sudah dikonfigurasi */
        ADC_WIDTH,          /* Lebar bit yang sudah dikonfigurasi */
        DEFAULT_VREF,       /* Tegangan referensi default */
        &adc_chars          /* Struct untuk menyimpan hasil karakterisasi */
    );

    /* Tampilkan jenis kalibrasi yang digunakan */
    if (cal_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "Kalibrasi: Two Point (eFuse)");
    } else if (cal_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "Kalibrasi: Vref dari eFuse");
    } else {
        ESP_LOGI(TAG, "Kalibrasi: Default Vref (%d mV)", DEFAULT_VREF);
    }

    ESP_LOGI(TAG, "Program 01: ADC Basic - ESP32 (ESP-IDF)");
    ESP_LOGI(TAG, "ADC1 Channel: %d | GPIO: %d", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "Resolusi: 12-bit | Atenuasi: 11dB (~0-3.3V)");
    ESP_LOGI(TAG, "=========================================\n");

    /* ====================================================
     * 4. Loop utama: baca ADC dan tampilkan hasilnya
     * ==================================================== */
    while (1) {
        /* --- Multisampling untuk mengurangi noise --- */
        uint32_t adc_reading = 0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            adc_reading += adc1_get_raw(ADC_CHANNEL);
        }
        adc_reading /= NUM_SAMPLES; /* Rata-rata dari NUM_SAMPLES pembacaan */

        /* --- Konversi nilai mentah ke tegangan (mV) menggunakan kalibrasi --- */
        uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);

        /* --- Tampilkan hasil pembacaan --- */
        ESP_LOGI(TAG, "ADC Raw: %4lu | Tegangan: %lu mV (%.2f V)",
                 (unsigned long)adc_reading,
                 (unsigned long)voltage_mv,
                 voltage_mv / 1000.0);

        /* --- Tunda 500ms sebelum pembacaan berikutnya --- */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
