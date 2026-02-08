/**
 * ==========================================================================
 * ESP32_05_Deep_Sleep_RTC - Deep Sleep dengan RTC Alarm / Scheduled Wake
 * ==========================================================================
 * 
 * Modul 14 - Power Management
 * Program 5: Scheduled data logging with deep sleep
 * 
 * KONSEP:
 * - Implementasi data logging terjadwal menggunakan deep sleep
 * - Siklus: Wake → Baca sensor → Simpan data → Cetak → Sleep
 * - Interval sleep: 30 detik (bisa diubah)
 * - Track total runtime vs sleep time untuk menghitung duty cycle
 * - RTC_DATA_ATTR untuk menyimpan data lintas siklus sleep
 * 
 * APLIKASI NYATA:
 * - Weather station: baca sensor tiap 5 menit
 * - Soil moisture monitor: baca tiap 30 menit
 * - Asset tracker: log posisi tiap 1 jam
 * - Bisa bertahan berbulan-bulan dengan baterai!
 * 
 * WIRING / KONEKSI:
 * ┌─────────────────────────────────────────────────────┐
 * │  ESP32          Komponen                             │
 * │  GPIO2  ──────► LED (+) ──► R(220Ω) ──► GND         │
 * │  GPIO34 ──────► Potentiometer wiper (simulasi sensor)│
 * │                 Pot VCC → 3.3V, Pot GND → GND       │
 * │                                                      │
 * │  Jika tanpa pot: gunakan internal hall sensor        │
 * └─────────────────────────────────────────────────────┘
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * [SCHED] Boot #5 | Wakeup: TIMER
 * [SCHED] --- Sensor Reading ---
 * [SCHED] ADC Raw: 2048 | Voltage: 1.65V
 * [SCHED] Internal temp: 53°C
 * [SCHED] --- Timing Stats ---
 * [SCHED] Total runtime: 2340 ms
 * [SCHED] Total sleep: 120000 ms
 * [SCHED] Duty cycle: 1.91%
 * [SCHED] Entering deep sleep for 30 seconds...
 * ========================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "SCHED";

/* Konfigurasi */
#define LED_PIN             GPIO_NUM_2
#define SENSOR_ADC_CHANNEL  ADC_CHANNEL_6   // GPIO34
#define SLEEP_INTERVAL_US   (30ULL * 1000000ULL) // 30 detik
#define MAX_READINGS        20              // Maks data yang disimpan di RTC

/* RTC memory - data bertahan selama deep sleep */
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int32_t total_active_ms = 0;     // Kumulatif waktu aktif
RTC_DATA_ATTR static int32_t total_sleep_ms = 0;       // Kumulatif waktu tidur
RTC_DATA_ATTR static int adc_readings[MAX_READINGS];   // Buffer sensor readings
RTC_DATA_ATTR static int reading_index = 0;             // Index buffer saat ini
RTC_DATA_ATTR static int total_readings = 0;            // Total reading yang pernah diambil

/**
 * Inisialisasi LED
 */
static void init_led(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

/**
 * Baca sensor ADC (simulasi sensor lingkungan)
 * Menggunakan ADC1 Channel 6 (GPIO34)
 */
static int read_sensor_adc(void)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC init gagal, menggunakan nilai simulasi");
        return (boot_count * 137 + 1024) % 4096; // Pseudo-random simulasi
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc_handle, SENSOR_ADC_CHANNEL, &chan_cfg);

    int adc_value = 0;
    ret = adc_oneshot_read(adc_handle, SENSOR_ADC_CHANNEL, &adc_value);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC read gagal, menggunakan simulasi");
        adc_value = (boot_count * 137 + 1024) % 4096;
    }

    adc_oneshot_del_unit(adc_handle);
    return adc_value;
}

/**
 * Simpan reading ke circular buffer di RTC memory
 */
static void store_reading(int value)
{
    adc_readings[reading_index] = value;
    reading_index = (reading_index + 1) % MAX_READINGS;
    total_readings++;
}

/**
 * Cetak semua reading yang tersimpan
 */
static void print_stored_readings(void)
{
    int count = (total_readings < MAX_READINGS) ? total_readings : MAX_READINGS;
    ESP_LOGI(TAG, "--- Stored Readings (%d/%d) ---", count, MAX_READINGS);

    for (int i = 0; i < count; i++) {
        int idx = (total_readings < MAX_READINGS) ? i :
                  ((reading_index + i) % MAX_READINGS);
        float voltage = adc_readings[idx] * 3.3f / 4095.0f;
        ESP_LOGI(TAG, "  [%2d] ADC=%4d  Voltage=%.2fV", i + 1, adc_readings[idx], voltage);
    }
}

/**
 * Dapatkan string wakeup cause
 */
static const char* get_wakeup_str(esp_sleep_wakeup_cause_t cause)
{
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER: return "TIMER";
        case ESP_SLEEP_WAKEUP_EXT0:  return "EXT0";
        case ESP_SLEEP_WAKEUP_EXT1:  return "EXT1";
        default:                     return "POWER_ON";
    }
}

/**
 * Entry point - dipanggil setiap wakeup dari deep sleep
 */
void app_main(void)
{
    int64_t start_us = esp_timer_get_time();

    boot_count++;

    /* Dapatkan wakeup cause */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    init_led();
    gpio_set_level(LED_PIN, 1); // LED ON = sedang bekerja

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "  SCHEDULED DATA LOGGER - Boot #%d", boot_count);
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "Wakeup cause: %s", get_wakeup_str(cause));

    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        total_sleep_ms += (int32_t)(SLEEP_INTERVAL_US / 1000ULL);
    } else {
        /* First boot - reset counters */
        total_active_ms = 0;
        total_sleep_ms = 0;
        reading_index = 0;
        total_readings = 0;
        memset(adc_readings, 0, sizeof(adc_readings));
        ESP_LOGI(TAG, "First boot - counters reset");
    }

    /* Baca sensor */
    ESP_LOGI(TAG, "--- Sensor Reading ---");
    int adc_raw = read_sensor_adc();
    float voltage = adc_raw * 3.3f / 4095.0f;
    ESP_LOGI(TAG, "ADC Raw: %d | Voltage: %.2fV", adc_raw, voltage);

    /* Simpan reading ke RTC buffer */
    store_reading(adc_raw);

    /* Tampilkan semua data tersimpan */
    print_stored_readings();

    /* Hitung timing statistics */
    int64_t active_us = esp_timer_get_time() - start_us;
    int active_ms = (int)(active_us / 1000);
    total_active_ms += active_ms;

    float total_time_ms = (float)(total_active_ms + total_sleep_ms);
    float duty_cycle = (total_time_ms > 0) ?
                       ((float)total_active_ms / total_time_ms * 100.0f) : 100.0f;

    ESP_LOGI(TAG, "--- Timing Stats ---");
    ESP_LOGI(TAG, "This boot active time : %d ms", active_ms);
    ESP_LOGI(TAG, "Total active time     : %d ms (%.1f s)", total_active_ms, total_active_ms / 1000.0f);
    ESP_LOGI(TAG, "Total sleep time      : %d ms (%.1f s)", total_sleep_ms, total_sleep_ms / 1000.0f);
    ESP_LOGI(TAG, "Effective duty cycle  : %.2f%%", duty_cycle);
    ESP_LOGI(TAG, "Total readings taken  : %d", total_readings);

    /* Estimasi battery life */
    float avg_current_ua = (duty_cycle / 100.0f) * 120000.0f +
                           (1.0f - duty_cycle / 100.0f) * 10.0f;
    float battery_hours = 2000000.0f / avg_current_ua; // 2000mAh battery
    ESP_LOGI(TAG, "Est. avg current      : %.1f µA (%.3f mA)", avg_current_ua, avg_current_ua / 1000.0f);
    ESP_LOGI(TAG, "Est. battery life     : %.0f hours (%.0f days)", battery_hours, battery_hours / 24.0f);

    /* Data terformat untuk Python */
    ESP_LOGI(TAG, "DATA,%d,%d,%d,%.2f,%d,%d,%d,%.2f,%.1f",
             boot_count, adc_raw, active_ms,
             voltage, total_active_ms, total_sleep_ms,
             total_readings, duty_cycle, avg_current_ua);

    /* LED OFF sebelum sleep */
    gpio_set_level(LED_PIN, 0);

    /* Konfigurasi dan masuk deep sleep */
    ESP_LOGI(TAG, "Entering deep sleep for %lld seconds...",
             SLEEP_INTERVAL_US / 1000000ULL);
    ESP_LOGI(TAG, "================================================");

    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_deep_sleep(SLEEP_INTERVAL_US);
}
