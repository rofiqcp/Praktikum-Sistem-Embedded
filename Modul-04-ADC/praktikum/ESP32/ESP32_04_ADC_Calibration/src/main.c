/*
 * ==========================================================
 *  MODUL 04 - ADC | Program 04: ADC Calibration
 *  Framework: ESP-IDF
 * ==========================================================
 *  Deskripsi:
 *    Mengkonfigurasi ADC dengan esp_adc_cal_characterize()
 *    untuk membandingkan nilai raw vs nilai terkalibrasi.
 *    Menampilkan tipe kalibrasi (eFuse Vref, eFuse TP, Default)
 *    dan perbandingan konversi tegangan manual vs terkalibrasi.
 *
 *  Hardware:
 *    - Potensiometer 10kΩ
 *      * Pin tengah  → GPIO ADC (ESP32: GPIO34, S2/S3: GPIO4)
 *      * Pin kiri    → GND
 *      * Pin kanan   → 3.3V
 *    - Multimeter (untuk verifikasi tegangan aktual)
 *
 *  Board Support:
 *    - ESP32 DevKit   : ADC1_CHANNEL_6 (GPIO34)
 *    - ESP32-S2/S3    : ADC1_CHANNEL_3 (GPIO4)
 * ==========================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

static const char *TAG = "ADC_CAL";

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
#define ADC_MAX_VALUE   4095                    // Nilai maksimum ADC 12-bit
#define VREF_MV         3300                    // Tegangan referensi 3.3V dalam mV

/* Jumlah sampel untuk multi-sampling */
#define NUM_SAMPLES     32

/* Variabel kalibrasi ADC */
static esp_adc_cal_characteristics_t adc_chars;

/**
 * @brief Mendapatkan string deskripsi tipe kalibrasi
 *
 * @param cal_type Tipe kalibrasi dari esp_adc_cal_characterize()
 * @return String deskripsi tipe kalibrasi
 */
static const char* get_cal_type_str(esp_adc_cal_value_t cal_type)
{
    switch (cal_type) {
        case ESP_ADC_CAL_VAL_EFUSE_VREF:
            return "eFuse Vref";
        case ESP_ADC_CAL_VAL_EFUSE_TP:
            return "eFuse Two Point";
        default:
            return "Default Vref";
    }
}

/**
 * @brief Membaca ADC dengan multi-sampling untuk stabilitas
 *
 * @param channel Channel ADC1 yang akan dibaca
 * @param num_samples Jumlah sampel
 * @return Nilai rata-rata ADC raw
 */
static int adc_read_multisampled(adc1_channel_t channel, int num_samples)
{
    long sum = 0;
    for (int i = 0; i < num_samples; i++) {
        sum += adc1_get_raw(channel);
        ets_delay_us(100);  // Delay kecil antar pembacaan
    }
    return (int)(sum / num_samples);
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " Modul 04 - ADC Calibration");
    ESP_LOGI(TAG, " Board: %s | GPIO: %d", BOARD_NAME, ADC_GPIO_NUM);
    ESP_LOGI(TAG, " Hardware: Potensiometer 10kΩ + Multimeter");
    ESP_LOGI(TAG, "========================================");

    /* --- Konfigurasi ADC1 --- */
    // Atur lebar bit ADC (resolusi 12-bit)
    adc1_config_width(ADC_WIDTH);

    // Atur atenuasi channel (range 0 - ~3.3V)
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    ESP_LOGI(TAG, "ADC dikonfigurasi:");
    ESP_LOGI(TAG, "  - Resolusi    : 12-bit (0-4095)");
    ESP_LOGI(TAG, "  - Atenuasi    : 11 dB (~0-3.3V)");
    ESP_LOGI(TAG, "  - Channel     : ADC1_CH%d (GPIO%d)", (int)ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  - Multi-sample: %d sampel", NUM_SAMPLES);

    /* --- Kalibrasi ADC menggunakan esp_adc_cal --- */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Informasi Kalibrasi ---");

    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, DEFAULT_VREF, &adc_chars
    );

    const char *cal_str = get_cal_type_str(cal_type);
    ESP_LOGI(TAG, "Tipe kalibrasi : %s", cal_str);

    /* Tampilkan detail berdasarkan tipe kalibrasi */
    if (cal_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "Status         : eFuse Vref tersedia (akurasi tinggi)");
        ESP_LOGI(TAG, "Sumber         : Nilai Vref disimpan di eFuse saat produksi");
    } else if (cal_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "Status         : eFuse Two Point tersedia (akurasi tinggi)");
        ESP_LOGI(TAG, "Sumber         : Dua titik kalibrasi disimpan di eFuse");
    } else {
        ESP_LOGW(TAG, "Status         : Menggunakan Vref default (%d mV)", DEFAULT_VREF);
        ESP_LOGW(TAG, "Catatan        : Akurasi lebih rendah, pertimbangkan kalibrasi manual");
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Perbandingan Konversi Raw vs Kalibrasi ---");
    ESP_LOGI(TAG, "Konversi manual : V = (raw / %d) x %d mV", ADC_MAX_VALUE, VREF_MV);
    ESP_LOGI(TAG, "Konversi kalibrasi: esp_adc_cal_raw_to_voltage()");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Gunakan multimeter untuk verifikasi tegangan aktual");
    ESP_LOGI(TAG, "Putar potensiometer untuk melihat perubahan nilai");
    ESP_LOGI(TAG, "");

    /* --- Loop Utama --- */
    int reading_num = 0;

    while (1) {
        reading_num++;

        /* Baca nilai raw ADC (single sample) */
        int raw_single = adc1_get_raw(ADC_CHANNEL);

        /* Baca nilai raw ADC (multi-sample untuk stabilitas) */
        int raw_avg = adc_read_multisampled(ADC_CHANNEL, NUM_SAMPLES);

        /* Konversi manual: rumus linier sederhana */
        uint32_t voltage_manual = (uint32_t)((raw_avg * VREF_MV) / ADC_MAX_VALUE);

        /* Konversi terkalibrasi: menggunakan esp_adc_cal */
        uint32_t voltage_cal = esp_adc_cal_raw_to_voltage(raw_avg, &adc_chars);

        /* Hitung perbedaan antara manual dan kalibrasi */
        int diff = (int)voltage_cal - (int)voltage_manual;

        /* Tampilkan hasil perbandingan */
        ESP_LOGI(TAG, "=== Pembacaan #%d ===", reading_num);
        ESP_LOGI(TAG, "  Raw (single)    : %4d", raw_single);
        ESP_LOGI(TAG, "  Raw (avg x%d)  : %4d", NUM_SAMPLES, raw_avg);
        ESP_LOGI(TAG, "  Konversi manual : %lu mV  [V = (%d/%d) x %d]",
                 (unsigned long)voltage_manual, raw_avg, ADC_MAX_VALUE, VREF_MV);
        ESP_LOGI(TAG, "  Konversi kalib. : %lu mV  [esp_adc_cal, tipe: %s]",
                 (unsigned long)voltage_cal, cal_str);
        ESP_LOGI(TAG, "  Selisih         : %+d mV  (kalibrasi - manual)", diff);
        ESP_LOGI(TAG, "  >>> Ukur dengan multimeter untuk verifikasi <<<");
        ESP_LOGI(TAG, "");

        /* Delay 2 detik sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
