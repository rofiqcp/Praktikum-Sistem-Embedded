/**
 * ==========================================================================
 * PROGRAM 01: ADC Single Read (Pembacaan ADC Tunggal)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca nilai ADC dari potensiometer yang terhubung ke 
 *   ADC1_CHANNEL_6 (GPIO34 pada ESP32, GPIO4 pada S2/S3) menggunakan
 *   mode single-shot (pembacaan tunggal). Nilai mentah (raw) ditampilkan
 *   setiap 500ms melalui serial monitor.
 * 
 * Koneksi Hardware:
 *   - Potensiometer: VCC → 3.3V, GND → GND, Wiper → GPIO34 (ESP32)
 *   - Atau Wiper → GPIO4 (ESP32-S2/S3)
 * 
 * API yang digunakan:
 *   - adc1_config_width()        : Mengatur resolusi ADC
 *   - adc1_config_channel_atten(): Mengatur atenuasi channel
 *   - adc1_get_raw()             : Membaca nilai mentah ADC
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"

/* Tag untuk logging */
static const char *TAG = "ADC_SINGLE";

/* 
 * Konfigurasi Channel ADC
 * ESP32   : ADC1_CHANNEL_6 = GPIO34
 * ESP32-S2: ADC1_CHANNEL_3 = GPIO4
 * ESP32-S3: ADC1_CHANNEL_3 = GPIO4
 */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC1_CHANNEL_6   /* GPIO34 pada ESP32 */
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC1_CHANNEL_3   /* GPIO4 pada S2/S3 */
    #define ADC_GPIO_NUM    4
#else
    #define ADC_CHANNEL     ADC1_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

/* Resolusi ADC: 12-bit (0-4095) */
#define ADC_WIDTH       ADC_WIDTH_BIT_12

/* Atenuasi: 11dB untuk rentang penuh 0-3.3V */
#define ADC_ATTEN       ADC_ATTEN_DB_11

/* Interval pembacaan dalam milidetik */
#define READ_INTERVAL_MS    500

/**
 * @brief Fungsi utama untuk inisialisasi dan pembacaan ADC
 * 
 * Langkah-langkah:
 * 1. Konfigurasi lebar bit ADC (resolusi)
 * 2. Konfigurasi atenuasi channel
 * 3. Loop pembacaan nilai mentah setiap 500ms
 */
void app_main(void)
{
    /* ====== INISIALISASI ADC ====== */
    
    /* Langkah 1: Atur resolusi ADC ke 12-bit (0-4095) */
    esp_err_t ret = adc1_config_width(ADC_WIDTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal mengatur resolusi ADC: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Resolusi ADC diatur ke 12-bit (0-4095)");

    /* Langkah 2: Atur atenuasi channel ke 11dB (rentang 0-3.3V) */
    ret = adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal mengatur atenuasi channel: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Atenuasi channel diatur ke 11dB (0-3.3V)");

    /* Cetak informasi konfigurasi */
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Single Read - Pembacaan Tunggal");
    ESP_LOGI(TAG, "  Channel : ADC1_CHANNEL_%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Resolusi: 12-bit (0-4095)");
    ESP_LOGI(TAG, "  Atenuasi: 11dB (0-3.3V)");
    ESP_LOGI(TAG, "  Interval: %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "========================================");

    /* Variabel untuk menyimpan nilai pembacaan */
    int raw_value = 0;
    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca nilai mentah ADC */
        raw_value = adc1_get_raw(ADC_CHANNEL);

        /* Hitung persentase dari nilai maksimum (4095) */
        float percentage = (raw_value / 4095.0f) * 100.0f;

        /* Tampilkan hasil pembacaan */
        counter++;
        printf("[%04d] ADC Raw: %4d | Persentase: %6.2f%%\n", 
               counter, raw_value, percentage);

        /* Tunggu sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
