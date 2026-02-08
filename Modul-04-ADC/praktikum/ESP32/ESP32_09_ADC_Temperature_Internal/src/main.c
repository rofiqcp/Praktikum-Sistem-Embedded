/**
 * ==========================================================================
 * PROGRAM 09: ADC Temperature Internal (Sensor Suhu Internal)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca sensor suhu internal ESP32.
 *
 *   Untuk ESP32-S2/S3: Menggunakan driver temperature_sensor (hardware)
 *   Untuk ESP32 (original): ESP32 klasik tidak memiliki driver temp sensor
 *     yang stabil di ESP-IDF v5.x, sehingga menggunakan estimasi suhu
 *     berbasis ADC internal.
 *
 *   Catatan: Suhu yang diukur adalah suhu chip, bukan suhu lingkungan.
 *   Suhu chip biasanya 5-15 C lebih tinggi dari suhu lingkungan.
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#if CONFIG_IDF_TARGET_ESP32
    #include "esp_adc/adc_oneshot.h"
    #include "esp_adc/adc_cali.h"
    #include "esp_adc/adc_cali_scheme.h"
    #define USE_ADC_TEMP  1
#else
    #include "driver/temperature_sensor.h"
    #define USE_ADC_TEMP  0
#endif

static const char *TAG = "TEMP_INT";
#define READ_INTERVAL_MS    1000
#define AVG_SAMPLES         5
#define HISTORY_SIZE        60

static float temp_history[HISTORY_SIZE];
static int history_idx = 0;
static int history_count = 0;

static void add_to_history(float temp) {
    temp_history[history_idx] = temp;
    history_idx = (history_idx + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

static float get_avg_temperature(void) {
    if (history_count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < history_count; i++) sum += temp_history[i];
    return sum / history_count;
}

static float get_min_temperature(void) {
    if (history_count == 0) return 0.0f;
    float m = temp_history[0];
    for (int i = 1; i < history_count; i++) if (temp_history[i] < m) m = temp_history[i];
    return m;
}

static float get_max_temperature(void) {
    if (history_count == 0) return 0.0f;
    float m = temp_history[0];
    for (int i = 1; i < history_count; i++) if (temp_history[i] > m) m = temp_history[i];
    return m;
}

static const char* get_temp_status(float temp_c) {
    if (temp_c < 0)   return "BEKU     ";
    if (temp_c < 25)  return "DINGIN   ";
    if (temp_c < 45)  return "NORMAL   ";
    if (temp_c < 65)  return "HANGAT   ";
    if (temp_c < 85)  return "PANAS    ";
    return "OVERHEAT!";
}

#if USE_ADC_TEMP
static float estimate_chip_temp(adc_oneshot_unit_handle_t adc_h, adc_cali_handle_t cal_h) {
    int raw = 0;
    float temp = 35.0f;
    esp_err_t ret = adc_oneshot_read(adc_h, ADC_CHANNEL_0, &raw);
    if (ret == ESP_OK) {
        if (cal_h != NULL) {
            int mv = 0;
            adc_cali_raw_to_voltage(cal_h, raw, &mv);
            temp = 25.0f + (float)(mv - 1100) * 0.03f;
        } else {
            temp = 25.0f + (float)(raw - 2048) * 0.015f;
        }
        if (temp < -10.0f) temp = -10.0f;
        if (temp > 120.0f) temp = 120.0f;
    }
    return temp;
}
#endif

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Internal Temperature Sensor");
    ESP_LOGI(TAG, "  Interval: %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "========================================");

#if USE_ADC_TEMP
    ESP_LOGI(TAG, "Platform: ESP32 Classic (ADC estimation, +/-5C)");
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));
    adc_oneshot_chan_cfg_t ch_cfg = { .bitwidth = ADC_BITWIDTH_DEFAULT, .atten = ADC_ATTEN_DB_12 };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &ch_cfg));
    adc_cali_handle_t cali_handle = NULL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cal_cfg = { .unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    adc_cali_create_scheme_curve_fitting(&cal_cfg, &cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cal_cfg = { .unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    adc_cali_create_scheme_line_fitting(&cal_cfg, &cali_handle);
#endif
#else
    ESP_LOGI(TAG, "Platform: ESP32-S2/S3 (Hardware temp sensor)");
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t ts_cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&ts_cfg, &temp_sensor));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
    ESP_LOGI(TAG, "Hardware temp sensor aktif");
#endif

    int counter = 0;
    while (1) {
        float temp_sum = 0.0f;
        int valid = 0;
        for (int i = 0; i < AVG_SAMPLES; i++) {
            float t = 0.0f;
            esp_err_t r;
#if USE_ADC_TEMP
            t = estimate_chip_temp(adc_handle, cali_handle);
            r = ESP_OK;
#else
            r = temperature_sensor_get_celsius(temp_sensor, &t);
#endif
            if (r == ESP_OK) { temp_sum += t; valid++; }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (valid == 0) { vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS)); continue; }
        
        float tc = temp_sum / valid;
        float tf = tc * 9.0f / 5.0f + 32.0f;
        add_to_history(tc);
        counter++;
        
        printf("[%04d] Suhu: %6.2f C | %6.2f F | %s", counter, tc, tf, get_temp_status(tc));
        int bar = (int)(tc * 30 / 100);
        if (bar < 0) bar = 0;
        if (bar > 30) bar = 30;
        printf(" [");
        for (int i = 0; i < 30; i++) printf(i < bar ? "#" : ".");
        printf("]\n");
        
        if (counter % 10 == 0) {
            printf("  [STATS] Min:%.2fC Max:%.2fC Avg:%.2fC N:%d\n",
                   get_min_temperature(), get_max_temperature(), get_avg_temperature(), history_count);
        }
        if (tc > 80) printf("  WARNING: OVERHEAT!\n");
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
