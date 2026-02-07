/**
 * ============================================================================
 * PROJECT  : ESP32_09_Adaptive_Duty_Cycling
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Adaptive Duty Cycling (Sleep Berdasarkan Level Baterai)
 *
 * DESKRIPSI:
 * Program mengatur durasi deep sleep secara dinamis berdasarkan level baterai.
 * - Baterai penuh (>80%)   : sleep 30 detik (sampling sering)
 * - Baterai sedang (40-80%): sleep 60 detik
 * - Baterai rendah (20-40%): sleep 120 detik (hemat daya)
 * - Baterai kritis (<20%)  : sleep 300 detik (5 menit)
 *
 * Konsep ini sangat penting untuk perangkat IoT bertenaga baterai/solar.
 *
 * HARDWARE:
 * - ESP32 DevKit V1
 * - Voltage divider pada GPIO34 (ADC1_CH6)
 * - LED pada GPIO2 (indikator)
 *
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "driver/gpio.h"

static const char *TAG = "ADAPTIVE";

#define BAT_ADC_CHANNEL     ADC1_CHANNEL_6  /* GPIO34 */
#define LED_PIN             GPIO_NUM_2
#define VREF_MV             1100

/* Durasi sleep berdasarkan level baterai (dalam detik) */
#define SLEEP_FULL          30      /* >80% */
#define SLEEP_MEDIUM        60      /* 40-80% */
#define SLEEP_LOW           120     /* 20-40% */
#define SLEEP_CRITICAL      300     /* <20% */

/* Threshold baterai Li-Ion 3.7V via voltage divider (R1=100k, R2=100k → /2) */
#define BAT_FULL_MV         2100    /* 4.2V / 2 */
#define BAT_EMPTY_MV        1500    /* 3.0V / 2 */

/* RTC data untuk tracking across reboot */
RTC_DATA_ATTR static int cycle_count = 0;
RTC_DATA_ATTR static uint32_t last_voltage_mv = 0;
RTC_DATA_ATTR static uint32_t last_sleep_sec = 0;

static esp_adc_cal_characteristics_t adc_chars;

static uint32_t read_battery_mv(void)
{
    uint32_t raw = 0;
    for (int i = 0; i < 16; i++) {
        raw += adc1_get_raw(BAT_ADC_CHANNEL);
    }
    raw /= 16;
    return esp_adc_cal_raw_to_voltage(raw, &adc_chars);
}

static uint8_t voltage_to_percent(uint32_t mv)
{
    if (mv >= BAT_FULL_MV) return 100;
    if (mv <= BAT_EMPTY_MV) return 0;
    return (uint8_t)((mv - BAT_EMPTY_MV) * 100 / (BAT_FULL_MV - BAT_EMPTY_MV));
}

static uint32_t get_sleep_duration(uint8_t percent)
{
    if (percent > 80) return SLEEP_FULL;
    if (percent > 40) return SLEEP_MEDIUM;
    if (percent > 20) return SLEEP_LOW;
    return SLEEP_CRITICAL;
}

static const char* get_level_string(uint8_t percent)
{
    if (percent > 80) return "FULL";
    if (percent > 40) return "MEDIUM";
    if (percent > 20) return "LOW";
    return "CRITICAL";
}

void app_main(void)
{
    cycle_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  Adaptive Duty Cycling - Power Mgmt      ║");
    ESP_LOGI(TAG, "║  Modul 14: Power Management               ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    /* Wake-up cause */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Cycle #%d | Wake cause: %s",
             cycle_count,
             (cause == ESP_SLEEP_WAKEUP_TIMER) ? "TIMER" : "POWER_ON");

    if (cycle_count > 1) {
        ESP_LOGI(TAG, "Previous: voltage=%lumV, sleep=%lus",
                 last_voltage_mv, last_sleep_sec);
    }

    /* ADC init */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BAT_ADC_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
                             VREF_MV, &adc_chars);

    /* Baca tegangan baterai */
    uint32_t bat_mv = read_battery_mv();
    uint32_t actual_mv = bat_mv * 2;  /* Kompensasi voltage divider */
    uint8_t percent = voltage_to_percent(bat_mv);

    ESP_LOGI(TAG, "┌─────────────────────────────────────┐");
    ESP_LOGI(TAG, "│ Battery: %lumV (ADC: %lumV)        ", actual_mv, bat_mv);
    ESP_LOGI(TAG, "│ Level  : %d%% [%s]", percent, get_level_string(percent));

    /* Tentukan durasi sleep */
    uint32_t sleep_sec = get_sleep_duration(percent);

    ESP_LOGI(TAG, "│ Sleep  : %lu seconds", sleep_sec);
    ESP_LOGI(TAG, "└─────────────────────────────────────┘");

    /* Tampilkan tabel policy */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Adaptive Sleep Policy:");
    ESP_LOGI(TAG, "  >80%%  → %3d sec %s", SLEEP_FULL,
             (percent > 80) ? "◄ CURRENT" : "");
    ESP_LOGI(TAG, "  40-80%% → %3d sec %s", SLEEP_MEDIUM,
             (percent > 40 && percent <= 80) ? "◄ CURRENT" : "");
    ESP_LOGI(TAG, "  20-40%% → %3d sec %s", SLEEP_LOW,
             (percent > 20 && percent <= 40) ? "◄ CURRENT" : "");
    ESP_LOGI(TAG, "  <20%%  → %3d sec %s", SLEEP_CRITICAL,
             (percent <= 20) ? "◄ CURRENT" : "");

    /* LED indicator berdasarkan level */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    int blink_count = (percent > 80) ? 4 : (percent > 40) ? 3 :
                      (percent > 20) ? 2 : 1;

    for (int i = 0; i < blink_count; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* Simpan state ke RTC memory */
    last_voltage_mv = bat_mv;
    last_sleep_sec = sleep_sec;

    /* Konfigurasi deep sleep */
    esp_sleep_enable_timer_wakeup(sleep_sec * 1000000ULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Entering deep sleep for %lu seconds...", sleep_sec);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_deep_sleep_start();
}
