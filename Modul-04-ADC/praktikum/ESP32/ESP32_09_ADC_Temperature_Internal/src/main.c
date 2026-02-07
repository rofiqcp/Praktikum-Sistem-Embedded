/**
 * ==========================================================================
 * PROGRAM 09: ADC Temperature Internal (Sensor Suhu Internal)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca sensor suhu internal ESP32. ESP32 memiliki sensor
 *   suhu built-in yang mengukur suhu die (chip).
 * 
 *   Untuk ESP32-S2/S3: Menggunakan driver temp_sensor yang lebih baru
 *   Untuk ESP32 (original): Menggunakan driver temp_sensor legacy
 * 
 *   Catatan: Suhu yang diukur adalah suhu chip, bukan suhu lingkungan.
 *   Suhu chip biasanya 5-15°C lebih tinggi dari suhu lingkungan.
 * 
 * API yang digunakan:
 *   - temperature_sensor_install()   : Install driver sensor suhu
 *   - temperature_sensor_enable()    : Aktifkan sensor
 *   - temperature_sensor_get_celsius(): Baca suhu dalam Celsius
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* 
 * Include header berdasarkan target chip
 * ESP-IDF v5.x menggunakan driver/temperature_sensor.h
 * ESP-IDF v4.x menggunakan driver/temp_sensor.h
 */
#include "driver/temperature_sensor.h"

static const char *TAG = "TEMP_INT";

/* Interval pembacaan */
#define READ_INTERVAL_MS    1000

/* Jumlah sampel untuk rata-rata */
#define AVG_SAMPLES         5

/* Buffer circular untuk tracking suhu */
#define HISTORY_SIZE        60  /* 60 detik history */
static float temp_history[HISTORY_SIZE];
static int history_idx = 0;
static int history_count = 0;

/**
 * @brief Simpan suhu ke history buffer
 */
static void add_to_history(float temp)
{
    temp_history[history_idx] = temp;
    history_idx = (history_idx + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
}

/**
 * @brief Hitung rata-rata suhu dari history
 */
static float get_avg_temperature(void)
{
    if (history_count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < history_count; i++) {
        sum += temp_history[i];
    }
    return sum / history_count;
}

/**
 * @brief Dapatkan suhu minimum dari history
 */
static float get_min_temperature(void)
{
    if (history_count == 0) return 0.0f;
    float min_t = temp_history[0];
    for (int i = 1; i < history_count; i++) {
        if (temp_history[i] < min_t) min_t = temp_history[i];
    }
    return min_t;
}

/**
 * @brief Dapatkan suhu maksimum dari history
 */
static float get_max_temperature(void)
{
    if (history_count == 0) return 0.0f;
    float max_t = temp_history[0];
    for (int i = 1; i < history_count; i++) {
        if (temp_history[i] > max_t) max_t = temp_history[i];
    }
    return max_t;
}

/**
 * @brief Mengembalikan label status suhu
 */
static const char* get_temp_status(float temp_c)
{
    if (temp_c < 0)   return "BEKU     ";
    if (temp_c < 25)  return "DINGIN   ";
    if (temp_c < 45)  return "NORMAL   ";
    if (temp_c < 65)  return "HANGAT   ";
    if (temp_c < 85)  return "PANAS    ";
    return "OVERHEAT!";
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Internal Temperature Sensor");
    ESP_LOGI(TAG, "  Sensor: Built-in die temperature");
    ESP_LOGI(TAG, "  Interval: %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "========================================");

    /* ====== INISIALISASI SENSOR SUHU ====== */
    
    /* Konfigurasi sensor suhu */
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    
    esp_err_t ret = temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menginstall sensor suhu: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Sensor suhu internal mungkin tidak didukung pada chip ini.");
        return;
    }
    ESP_LOGI(TAG, "Driver sensor suhu berhasil di-install");

    /* Aktifkan sensor */
    ret = temperature_sensor_enable(temp_sensor);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal mengaktifkan sensor suhu: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Sensor suhu diaktifkan");

    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        float temp_celsius = 0.0f;

        /* Baca suhu dengan rata-rata multi-sampel */
        float temp_sum = 0.0f;
        int valid_reads = 0;

        for (int i = 0; i < AVG_SAMPLES; i++) {
            float t;
            ret = temperature_sensor_get_celsius(temp_sensor, &t);
            if (ret == ESP_OK) {
                temp_sum += t;
                valid_reads++;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (valid_reads > 0) {
            temp_celsius = temp_sum / valid_reads;
        } else {
            ESP_LOGW(TAG, "Tidak ada pembacaan valid");
            vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
            continue;
        }

        /* Konversi ke Fahrenheit */
        float temp_fahrenheit = (temp_celsius * 9.0f / 5.0f) + 32.0f;

        /* Simpan ke history */
        add_to_history(temp_celsius);

        /* Tampilkan hasil */
        counter++;
        printf("[%04d] Suhu: %6.2f °C | %6.2f °F | Status: %s",
               counter, temp_celsius, temp_fahrenheit,
               get_temp_status(temp_celsius));

        /* Tampilkan mini bar suhu (0-100°C range) */
        int bar_pos = (int)(temp_celsius * 30 / 100);
        if (bar_pos < 0) bar_pos = 0;
        if (bar_pos > 30) bar_pos = 30;
        printf(" [");
        for (int i = 0; i < 30; i++) {
            if (i < bar_pos) printf("▓");
            else printf("░");
        }
        printf("]\n");

        /* Tampilkan statistik setiap 10 detik */
        if (counter % 10 == 0) {
            printf("  [STATS] Min: %.2f°C | Max: %.2f°C | Avg: %.2f°C | "
                   "Sampel: %d\n",
                   get_min_temperature(),
                   get_max_temperature(),
                   get_avg_temperature(),
                   history_count);
        }

        /* Peringatan overheat */
        if (temp_celsius > 80) {
            printf("  ⚠️  PERINGATAN OVERHEAT! Suhu chip terlalu tinggi!\n");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }

    /* Cleanup (tidak akan pernah tercapai dalam loop infinite) */
    temperature_sensor_disable(temp_sensor);
    temperature_sensor_uninstall(temp_sensor);
}
