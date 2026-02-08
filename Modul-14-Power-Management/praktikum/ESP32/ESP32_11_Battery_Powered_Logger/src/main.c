/**
 * ESP32_11_Battery_Powered_Logger
 * 
 * Konsep: Data logger bertenaga baterai lengkap dengan siklus sleep.
 * - Baca tegangan baterai via ADC (GPIO34, voltage divider)
 * - Baca data sensor (temp internal + ADC channel)
 * - Cetak data ke serial (simulasi penyimpanan)
 * - Hitung persentase baterai tersisa
 * - Masuk deep sleep selama 60 detik
 * - Saat bangun, cek baterai - jika terlalu rendah, masuk hibernasi
 * 
 * Wiring: Voltage divider baterai ke GPIO34, LED pada GPIO2
 * 
 * Framework: ESP-IDF (bukan Arduino)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

static const char *TAG = "BAT_LOGGER";

#define LED_PIN                 GPIO_NUM_2
#define BATTERY_ADC_CHANNEL     ADC_CHANNEL_6   // GPIO34
#define SENSOR_ADC_CHANNEL      ADC_CHANNEL_7   // GPIO35
#define ADC_ATTEN               ADC_ATTEN_DB_12

// Parameter baterai
#define BATTERY_FULL_MV         4200    // LiPo penuh (mV)
#define BATTERY_EMPTY_MV        3300    // LiPo kosong (mV)
#define BATTERY_CRITICAL_MV     3400    // Batas kritis untuk hibernasi (mV)
#define VOLTAGE_DIVIDER_RATIO   2.0f    // Rasio voltage divider (R1=R2)

// Parameter sleep
#define SLEEP_DURATION_SEC      60      // Durasi deep sleep
#define MAX_LOG_ENTRIES          1000    // Maks entri log

// Data RTC - bertahan selama deep sleep
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int log_count = 0;
RTC_DATA_ATTR static int total_readings = 0;
RTC_DATA_ATTR static int32_t min_battery_mv = 9999;
RTC_DATA_ATTR static int32_t max_battery_mv = 0;
RTC_DATA_ATTR static int64_t total_active_time_us = 0;
RTC_DATA_ATTR static int low_battery_warnings = 0;

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
 * Baca nilai ADC
 */
static int read_adc(adc_channel_t channel)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &chan_cfg));

    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &raw));
    adc_oneshot_del_unit(adc_handle);

    return raw;
}

/**
 * Konversi ADC raw ke tegangan baterai (mV)
 */
static int32_t get_battery_voltage_mv(void)
{
    int raw = read_adc(BATTERY_ADC_CHANNEL);
    // Konversi: raw -> mV (12-bit, 3300mV range with attenuation)
    float voltage_at_pin = ((float)raw / 4095.0f) * 3300.0f;
    // Kalikan dengan rasio voltage divider
    float battery_mv = voltage_at_pin * VOLTAGE_DIVIDER_RATIO;
    return (int32_t)battery_mv;
}

/**
 * Hitung persentase baterai
 */
static int get_battery_percentage(int32_t battery_mv)
{
    if (battery_mv >= BATTERY_FULL_MV) return 100;
    if (battery_mv <= BATTERY_EMPTY_MV) return 0;

    int pct = (int)((float)(battery_mv - BATTERY_EMPTY_MV) /
                     (float)(BATTERY_FULL_MV - BATTERY_EMPTY_MV) * 100.0f);
    return pct;
}

/**
 * Baca data sensor (simulasi)
 */
static int read_sensor_data(void)
{
    return read_adc(SENSOR_ADC_CHANNEL);
}

/**
 * Tampilkan bar baterai ASCII
 */
static void print_battery_bar(int percentage)
{
    int bar_len = percentage / 5;  // 20 karakter = 100%
    char bar[25];
    memset(bar, ' ', sizeof(bar));

    for (int i = 0; i < 20; i++) {
        bar[i] = (i < bar_len) ? '#' : '-';
    }
    bar[20] = '\0';

    if (percentage > 50) {
        ESP_LOGI(TAG, "  Baterai: [%s] %d%%", bar, percentage);
    } else if (percentage > 20) {
        ESP_LOGW(TAG, "  Baterai: [%s] %d%% (RENDAH)", bar, percentage);
    } else {
        ESP_LOGE(TAG, "  Baterai: [%s] %d%% (KRITIS!)", bar, percentage);
    }
}

/**
 * Log data ke serial (simulasi penyimpanan ke SD card/flash)
 */
static void log_data_entry(int32_t battery_mv, int battery_pct, int sensor_raw)
{
    log_count++;
    total_readings++;

    // Format: CSV-like untuk kemudahan parsing
    ESP_LOGI(TAG, "DATA,%d,%ld,%d,%d,%lld",
             log_count,
             (long)battery_mv,
             battery_pct,
             sensor_raw,
             esp_timer_get_time());

    ESP_LOGI(TAG, "  Log #%d: Baterai=%ld mV (%d%%), Sensor=%d",
             log_count, (long)battery_mv, battery_pct, sensor_raw);
}

/**
 * Tampilkan statistik akumulasi
 */
static void print_statistics(int32_t battery_mv)
{
    // Update min/max
    if (battery_mv < min_battery_mv) min_battery_mv = battery_mv;
    if (battery_mv > max_battery_mv) max_battery_mv = battery_mv;

    ESP_LOGI(TAG, "\n--- Statistik Logger ---");
    ESP_LOGI(TAG, "  Boot ke-%d", boot_count);
    ESP_LOGI(TAG, "  Total log entries   : %d", log_count);
    ESP_LOGI(TAG, "  Total readings      : %d", total_readings);
    ESP_LOGI(TAG, "  Battery min         : %ld mV", (long)min_battery_mv);
    ESP_LOGI(TAG, "  Battery max         : %ld mV", (long)max_battery_mv);
    ESP_LOGI(TAG, "  Low battery warnings: %d", low_battery_warnings);

    // Estimasi uptime
    float total_sleep_sec = (float)(boot_count - 1) * SLEEP_DURATION_SEC;
    float total_active_sec = (float)total_active_time_us / 1000000.0f;
    float total_uptime = total_sleep_sec + total_active_sec;

    ESP_LOGI(TAG, "  Total active time   : %.1f detik", total_active_sec);
    ESP_LOGI(TAG, "  Total sleep time    : %.0f detik", total_sleep_sec);
    ESP_LOGI(TAG, "  Total uptime        : %.0f detik (%.1f menit)",
             total_uptime, total_uptime / 60.0f);

    if (total_uptime > 0) {
        float duty_cycle = total_active_sec / total_uptime * 100.0f;
        ESP_LOGI(TAG, "  Duty cycle          : %.2f%%", duty_cycle);
    }
}

/**
 * Masuk hibernasi (baterai terlalu rendah)
 */
static void enter_hibernation(void)
{
    ESP_LOGE(TAG, "============================================");
    ESP_LOGE(TAG, "  BATERAI KRITIS - MASUK HIBERNASI!");
    ESP_LOGE(TAG, "  Hubungkan charger untuk melanjutkan.");
    ESP_LOGE(TAG, "============================================");

    // Matikan semua peripheral
    gpio_set_level(LED_PIN, 0);

    // Disable semua wakeup source kecuali reset
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // Masuk deep sleep tanpa wakeup (hibernasi)
    // Hanya bisa bangun dengan reset atau power cycle
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_deep_sleep_start();
}

/**
 * Masuk deep sleep untuk siklus berikutnya
 */
static void enter_sleep_cycle(void)
{
    ESP_LOGI(TAG, "--------------------------------------------");
    ESP_LOGI(TAG, "Masuk deep sleep selama %d detik...", SLEEP_DURATION_SEC);
    ESP_LOGI(TAG, "Siklus berikutnya: log #%d", log_count + 1);
    ESP_LOGI(TAG, "--------------------------------------------");

    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_SEC * 1000000ULL);

    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}

void app_main(void)
{
    int64_t cycle_start = esp_timer_get_time();
    boot_count++;

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 Battery-Powered Data Logger");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Boot ke-%d | Log entries: %d", boot_count, log_count);

    // Analisis penyebab wakeup
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wakeup: Timer (siklus logging rutin)");
    } else {
        ESP_LOGI(TAG, "Wakeup: Power-on/Reset (boot awal)");
        // Reset counter pada boot pertama
        log_count = 0;
        total_readings = 0;
        min_battery_mv = 9999;
        max_battery_mv = 0;
        total_active_time_us = 0;
        low_battery_warnings = 0;
    }

    init_led();
    gpio_set_level(LED_PIN, 1);  // LED ON saat aktif

    // === FASE 1: Baca tegangan baterai ===
    ESP_LOGI(TAG, "\n--- Cek Baterai ---");
    int32_t battery_mv = get_battery_voltage_mv();
    int battery_pct = get_battery_percentage(battery_mv);

    print_battery_bar(battery_pct);
    ESP_LOGI(TAG, "  Tegangan: %ld mV", (long)battery_mv);

    // Cek apakah baterai kritis
    if (battery_mv < BATTERY_CRITICAL_MV && battery_mv > 500) {
        // battery > 500 untuk menghindari false positive saat tidak ada baterai
        low_battery_warnings++;
        ESP_LOGE(TAG, "PERINGATAN: Baterai rendah! (%ld mV < %d mV)",
                 (long)battery_mv, BATTERY_CRITICAL_MV);

        if (low_battery_warnings >= 3) {
            enter_hibernation();  // Tidak akan kembali
        }
    }

    // === FASE 2: Baca sensor data ===
    ESP_LOGI(TAG, "\n--- Baca Sensor ---");
    int sensor_raw = read_sensor_data();

    // Konversi ke nilai bermakna (simulasi)
    float sensor_voltage = ((float)sensor_raw / 4095.0f) * 3.3f;

    ESP_LOGI(TAG, "  Sensor RAW  : %d", sensor_raw);
    ESP_LOGI(TAG, "  Sensor Volt : %.2f V", sensor_voltage);

    // === FASE 3: Log data ===
    ESP_LOGI(TAG, "\n--- Log Data ---");
    log_data_entry(battery_mv, battery_pct, sensor_raw);

    // === FASE 4: Statistik ===
    print_statistics(battery_mv);

    // Hitung waktu aktif siklus ini
    int64_t cycle_end = esp_timer_get_time();
    int64_t cycle_duration = cycle_end - cycle_start;
    total_active_time_us += cycle_duration;

    ESP_LOGI(TAG, "\n  Durasi siklus ini: %lld us (%.1f ms)",
             cycle_duration, (float)cycle_duration / 1000.0f);

    // LED OFF sebelum tidur
    gpio_set_level(LED_PIN, 0);

    // Cek apakah sudah mencapai maks log
    if (log_count >= MAX_LOG_ENTRIES) {
        ESP_LOGW(TAG, "Maksimum log entries tercapai (%d). Berhenti logging.",
                 MAX_LOG_ENTRIES);
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }

    // Masuk deep sleep
    enter_sleep_cycle();
}
