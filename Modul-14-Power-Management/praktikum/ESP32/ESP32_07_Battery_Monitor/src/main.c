/**
 * ============================================================================
 * PROJECT  : ESP32_07_Battery_Monitor
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Battery Voltage Monitor via ADC dengan Kalibrasi eFuse
 *
 * DESKRIPSI:
 * Membaca tegangan baterai Li-Ion melalui voltage divider + ADC ESP32.
 * Menggunakan esp_adc_cal untuk kalibrasi yang akurat berdasarkan eFuse.
 * Konversi tegangan ke persentase menggunakan lookup table kurva discharge.
 *
 * ============================================================================
 * WIRING
 * ============================================================================
 * Baterai 3.7V → Voltage Divider (R1=R2=100K) → GPIO34 (ADC1_CH6)
 *   VBAT ─── R1(100K) ─┬── GPIO34
 *                       R2(100K)
 *                       │
 *                      GND
 *   Rasio: VADC = VBAT / 2
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) BATT: Battery: 3.85V (50%) [GOOD]
 *   I (xxx) BATT: [██████████░░░░░░░░░░] 50%
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define BATT_ADC_CH      ADC1_CHANNEL_6    /* GPIO34 */
#define BATT_ADC_ATTEN   ADC_ATTEN_DB_11   /* 0-3.3V range */
#define BATT_ADC_WIDTH   ADC_WIDTH_BIT_12  /* 0-4095 */
#define V_DIV_RATIO      2.0f              /* Voltage divider ratio */
#define NUM_SAMPLES      64                /* Averaging samples */
#define DEFAULT_VREF     1100              /* mV, fallback jika eFuse kosong */

static const char *TAG = "BATT";
static esp_adc_cal_characteristics_t adc_chars;

/* Li-Ion discharge curve lookup table */
typedef struct {
    float voltage;
    int percentage;
} batt_level_t;

static const batt_level_t batt_table[] = {
    {4.20f, 100}, {4.15f, 95}, {4.11f, 90}, {4.08f, 85},
    {4.02f, 80},  {3.98f, 75}, {3.95f, 70}, {3.91f, 65},
    {3.87f, 60},  {3.85f, 55}, {3.84f, 50}, {3.82f, 45},
    {3.80f, 40},  {3.79f, 35}, {3.77f, 30}, {3.75f, 25},
    {3.73f, 20},  {3.71f, 15}, {3.69f, 10}, {3.61f, 5},
    {3.27f, 0}
};
#define BATT_TABLE_SIZE  (sizeof(batt_table) / sizeof(batt_table[0]))

/**
 * Konversi tegangan ke persentase menggunakan interpolasi linear
 */
static int voltage_to_percentage(float voltage)
{
    if (voltage >= 4.20f) return 100;
    if (voltage <= 3.27f) return 0;

    for (int i = 0; i < (int)BATT_TABLE_SIZE - 1; i++) {
        if (voltage >= batt_table[i + 1].voltage) {
            float v_range = batt_table[i].voltage - batt_table[i + 1].voltage;
            float v_diff = voltage - batt_table[i + 1].voltage;
            int pct_range = batt_table[i].percentage - batt_table[i + 1].percentage;
            return batt_table[i + 1].percentage + (int)(v_diff / v_range * pct_range);
        }
    }
    return 0;
}

/**
 * Baca tegangan baterai dengan averaging dan kalibrasi
 */
static float read_battery_voltage(void)
{
    uint32_t adc_sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_sum += adc1_get_raw(BATT_ADC_CH);
    }
    uint32_t adc_avg = adc_sum / NUM_SAMPLES;

    /* Konversi ke mV menggunakan kalibrasi */
    uint32_t mv = esp_adc_cal_raw_to_voltage(adc_avg, &adc_chars);

    /* Kalikan dengan rasio voltage divider */
    return (mv / 1000.0f) * V_DIV_RATIO;
}

/**
 * Cetak battery bar visual
 */
static void print_battery_bar(int percentage)
{
    printf("  [");
    int bars = percentage / 5;
    for (int i = 0; i < 20; i++) {
        printf("%s", (i < bars) ? "█" : "░");
    }
    printf("] %d%%\n", percentage);
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Battery Voltage Monitor      ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");

    /* Konfigurasi ADC */
    adc1_config_width(BATT_ADC_WIDTH);
    adc1_config_channel_atten(BATT_ADC_CH, BATT_ADC_ATTEN);

    /* Kalibrasi ADC menggunakan eFuse (jika tersedia) */
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1, BATT_ADC_ATTEN, BATT_ADC_WIDTH,
        DEFAULT_VREF, &adc_chars
    );

    ESP_LOGI(TAG, "ADC Calibration: %s",
             (cal_type == ESP_ADC_CAL_VAL_EFUSE_VREF) ? "eFuse Vref" :
             (cal_type == ESP_ADC_CAL_VAL_EFUSE_TP) ? "eFuse Two Point" :
             "Default Vref");

    /* Loop pembacaan */
    while (1) {
        float voltage = read_battery_voltage();
        int percentage = voltage_to_percentage(voltage);

        const char *status;
        if (percentage > 75) status = "FULL";
        else if (percentage > 50) status = "GOOD";
        else if (percentage > 25) status = "LOW";
        else if (percentage > 10) status = "CRITICAL";
        else status = "SHUTDOWN!";

        ESP_LOGI(TAG, "Battery: %.2fV (%d%%) [%s]", voltage, percentage, status);
        print_battery_bar(percentage);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
