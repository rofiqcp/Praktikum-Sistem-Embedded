/**
 * ============================================================================
 * PROJECT  : ESP32_11_Solar_Weather_Station
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Solar-Powered Weather Station (Integrasi Lengkap)
 *
 * DESKRIPSI:
 * Program ini mengintegrasikan semua konsep power management:
 * 1. Deep sleep dengan timer wake-up
 * 2. RTC memory untuk menyimpan data antar siklus
 * 3. Adaptive duty cycling berdasarkan level baterai
 * 4. Data batching (kumpulkan data sebelum kirim)
 * 5. Battery monitoring dengan ADC
 *
 * Alur kerja:
 * - Bangun → Baca sensor → Simpan ke RTC buffer
 * - Jika buffer penuh atau interval tertentu → Kirim semua data
 * - Sesuaikan sleep duration berdasarkan baterai
 * - Masuk deep sleep
 *
 * HARDWARE:
 * - ESP32 DevKit V1
 * - Sensor suhu internal (simulasi)
 * - ADC pada GPIO34 untuk battery monitor
 * - LED pada GPIO2
 *
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/temp_sensor.h"

static const char *TAG = "SOLAR_WX";

#define LED_PIN             GPIO_NUM_2
#define BAT_ADC_CHANNEL     ADC1_CHANNEL_6  /* GPIO34 */
#define VREF_MV             1100

/* Data batching configuration */
#define MAX_READINGS        10
#define SEND_INTERVAL       5   /* Kirim setiap 5 siklus */

/* Threshold baterai (setelah voltage divider /2) */
#define BAT_FULL_MV         2100
#define BAT_EMPTY_MV        1500

/* ========================== RTC MEMORY DATA ========================== */

typedef struct {
    float temperature;
    uint32_t battery_mv;
    uint32_t timestamp;     /* Waktu sejak pertama boot (detik) */
} sensor_reading_t;

RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int reading_index = 0;
RTC_DATA_ATTR static sensor_reading_t readings[MAX_READINGS];
RTC_DATA_ATTR static uint32_t total_uptime_sec = 0;
RTC_DATA_ATTR static uint32_t total_sleep_sec = 0;
RTC_DATA_ATTR static uint32_t last_send_cycle = 0;

/* ===================================================================== */

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

static uint8_t battery_percent(uint32_t mv)
{
    if (mv >= BAT_FULL_MV) return 100;
    if (mv <= BAT_EMPTY_MV) return 0;
    return (uint8_t)((mv - BAT_EMPTY_MV) * 100 / (BAT_FULL_MV - BAT_EMPTY_MV));
}

static uint32_t adaptive_sleep_sec(uint8_t bat_pct)
{
    if (bat_pct > 80) return 15;    /* Frequent sampling */
    if (bat_pct > 50) return 30;
    if (bat_pct > 20) return 60;
    return 120;                      /* Critical: 2 min */
}

static float read_temperature(void)
{
    /* Simulasi pembacaan sensor */
    /* Pada hardware nyata, gunakan I2C sensor seperti BMP280 */
    uint32_t raw = esp_random();
    return 20.0f + (float)(raw % 150) / 10.0f;  /* 20.0-35.0°C */
}

static void transmit_data(void)
{
    /* Simulasi pengiriman data (WiFi/LoRa/BLE) */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "📡 TRANSMITTING BATCHED DATA (%d readings)", reading_index);
    ESP_LOGI(TAG, "┌──────┬──────────┬────────────┬──────────┐");
    ESP_LOGI(TAG, "│  #   │ Temp(°C) │ Battery(mV)│ Time(s)  │");
    ESP_LOGI(TAG, "├──────┼──────────┼────────────┼──────────┤");

    for (int i = 0; i < reading_index; i++) {
        ESP_LOGI(TAG, "│  %2d  │  %5.1f   │   %4lu     │  %5lu   │",
                 i + 1,
                 readings[i].temperature,
                 readings[i].battery_mv * 2,  /* Actual voltage */
                 readings[i].timestamp);
    }
    ESP_LOGI(TAG, "└──────┴──────────┴────────────┴──────────┘");

    /* Reset buffer setelah transmit */
    reading_index = 0;
    last_send_cycle = boot_count;
    ESP_LOGI(TAG, "Buffer cleared. Next transmit in %d cycles.", SEND_INTERVAL);
}

void app_main(void)
{
    boot_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ☀️  Solar Weather Station                 ║");
    ESP_LOGI(TAG, "║  Modul 14: Power Management (Integrasi)   ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    /* Wake-up cause */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Boot #%d | Wake: %s",
             boot_count,
             (cause == ESP_SLEEP_WAKEUP_TIMER) ? "TIMER" :
             (cause == ESP_SLEEP_WAKEUP_EXT0) ? "EXT0" : "POWER_ON");

    /* LED quick flash = alive indicator */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);

    /* Init ADC */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BAT_ADC_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
                             VREF_MV, &adc_chars);

    /* ===== FASE 1: Baca Sensor ===== */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "📊 SENSOR READING...");

    float temp = read_temperature();
    uint32_t bat_mv = read_battery_mv();
    uint8_t bat_pct = battery_percent(bat_mv);

    ESP_LOGI(TAG, "  Temperature : %.1f °C", temp);
    ESP_LOGI(TAG, "  Battery     : %lumV (%d%%)", bat_mv * 2, bat_pct);

    /* ===== FASE 2: Simpan ke RTC Buffer ===== */
    if (reading_index < MAX_READINGS) {
        readings[reading_index].temperature = temp;
        readings[reading_index].battery_mv = bat_mv;
        readings[reading_index].timestamp = total_uptime_sec + total_sleep_sec;
        reading_index++;
        ESP_LOGI(TAG, "  Buffer      : %d/%d readings", reading_index, MAX_READINGS);
    } else {
        ESP_LOGW(TAG, "  Buffer FULL! Forcing transmit...");
    }

    /* ===== FASE 3: Transmit Jika Perlu ===== */
    bool should_transmit = (reading_index >= MAX_READINGS) ||
                           (boot_count - last_send_cycle >= SEND_INTERVAL);

    if (should_transmit && reading_index > 0) {
        transmit_data();
    }

    /* ===== FASE 4: Adaptive Sleep ===== */
    uint32_t sleep_sec = adaptive_sleep_sec(bat_pct);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "⚡ POWER STATUS:");
    ESP_LOGI(TAG, "  Battery Level : %d%% (%s)",
             bat_pct,
             (bat_pct > 80) ? "FULL" :
             (bat_pct > 50) ? "GOOD" :
             (bat_pct > 20) ? "LOW" : "CRITICAL");
    ESP_LOGI(TAG, "  Sleep Duration: %lu seconds", sleep_sec);
    ESP_LOGI(TAG, "  Total Boots   : %d", boot_count);
    ESP_LOGI(TAG, "  Total Uptime  : ~%lu seconds", total_uptime_sec);
    ESP_LOGI(TAG, "  Total Sleep   : ~%lu seconds", total_sleep_sec);

    /* Update statistics */
    total_uptime_sec += 1;  /* Approx 1s active per cycle */
    total_sleep_sec += sleep_sec;

    /* LED off before sleep */
    gpio_set_level(LED_PIN, 0);

    /* Configure deep sleep */
    esp_sleep_enable_timer_wakeup(sleep_sec * 1000000ULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "💤 Entering deep sleep for %lu seconds...", sleep_sec);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_deep_sleep_start();
}
