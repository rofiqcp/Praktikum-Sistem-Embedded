/**
 * ESP32_12_Power_Budget_Analysis
 * 
 * Konsep: Profiling daya lengkap: analisis aktif/sleep/transisi.
 * - Ukur dan laporkan waktu di setiap state:
 *   - Active (sensor read, processing)
 *   - Sleep
 * - Hitung duty cycle dan arus rata-rata
 * - Estimasi battery life: hours = capacity(mAh) / avg_current(mA)
 * - Kapasitas baterai konfigurabel (default 2000mAh LiPo)
 * - Jalankan beberapa siklus, akumulasi statistik
 * - Cetak laporan power budget komprehensif
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

static const char *TAG = "PWR_BUDGET";

#define LED_PIN             GPIO_NUM_2
#define ADC_CHANNEL         ADC_CHANNEL_6       // GPIO34
#define ADC_ATTEN           ADC_ATTEN_DB_12

// Parameter konfigurabel
#define BATTERY_CAPACITY_MAH    2000    // Kapasitas baterai (mAh)
#define SLEEP_DURATION_SEC      10      // Durasi deep sleep (detik)
#define NUM_SENSOR_READS        5       // Jumlah pembacaan sensor per siklus
#define MAX_CYCLES              100     // Maks siklus sebelum berhenti

// Estimasi arus per state (mA) - dari datasheet ESP32
#define CURRENT_ACTIVE_MA       50.0f   // CPU aktif, radio off
#define CURRENT_WIFI_MA         120.0f  // WiFi TX aktif
#define CURRENT_LIGHT_SLEEP_MA  0.8f    // Light sleep
#define CURRENT_DEEP_SLEEP_MA   0.01f   // Deep sleep (10uA)
#define CURRENT_HIBERNATE_MA    0.005f  // Hibernate (5uA)
#define CURRENT_TRANSITION_MA   30.0f   // Transisi wakeup

// Data RTC - bertahan selama deep sleep
RTC_DATA_ATTR static int cycle_count = 0;
RTC_DATA_ATTR static int64_t total_active_us = 0;
RTC_DATA_ATTR static int64_t total_sensor_us = 0;
RTC_DATA_ATTR static int64_t total_process_us = 0;
RTC_DATA_ATTR static int64_t total_transition_us = 0;
RTC_DATA_ATTR static int64_t total_sleep_us = 0;
RTC_DATA_ATTR static int64_t min_cycle_us = INT64_MAX;
RTC_DATA_ATTR static int64_t max_cycle_us = 0;
RTC_DATA_ATTR static int total_sensor_readings = 0;

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
 * Baca ADC (simulasi sensor read)
 */
static int read_sensor(void)
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
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
    adc_oneshot_del_unit(adc_handle);

    return raw;
}

/**
 * Fase pembacaan sensor - ukur waktu
 */
static int64_t phase_sensor_read(void)
{
    int64_t start = esp_timer_get_time();

    ESP_LOGI(TAG, "  [SENSOR] Membaca %d sampel sensor...", NUM_SENSOR_READS);
    int sum = 0;
    for (int i = 0; i < NUM_SENSOR_READS; i++) {
        int val = read_sensor();
        sum += val;
        total_sensor_readings++;
    }
    int avg = sum / NUM_SENSOR_READS;
    ESP_LOGI(TAG, "  [SENSOR] Rata-rata ADC: %d", avg);

    int64_t elapsed = esp_timer_get_time() - start;
    total_sensor_us += elapsed;
    return elapsed;
}

/**
 * Fase processing - simulasi pengolahan data
 */
static int64_t phase_processing(void)
{
    int64_t start = esp_timer_get_time();

    ESP_LOGI(TAG, "  [PROSES] Mengolah data...");

    // Simulasi kalkulasi (checksum, format, dll)
    volatile float result = 0;
    for (int i = 0; i < 10000; i++) {
        result += (float)i * 0.001f;
    }

    // Simulasi formatting data
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "CYCLE:%d,READS:%d,VAL:%.2f",
             cycle_count, total_sensor_readings, result);

    ESP_LOGI(TAG, "  [PROSES] Data: %s", buffer);

    int64_t elapsed = esp_timer_get_time() - start;
    total_process_us += elapsed;
    return elapsed;
}

/**
 * Cetak laporan power budget komprehensif
 */
static void print_power_budget_report(int64_t sensor_us, int64_t process_us,
                                       int64_t cycle_active_us)
{
    // Estimasi waktu transisi (wakeup)
    int64_t transition_us = 3000;  // ~3ms typical wakeup time
    total_transition_us += transition_us;

    // Akumulasi sleep time
    int64_t sleep_us = (int64_t)SLEEP_DURATION_SEC * 1000000LL;
    total_sleep_us += sleep_us;

    // Update min/max siklus
    if (cycle_active_us < min_cycle_us) min_cycle_us = cycle_active_us;
    if (cycle_active_us > max_cycle_us) max_cycle_us = cycle_active_us;

    // ===== LAPORAN SIKLUS INI =====
    ESP_LOGI(TAG, "\n============================================================");
    ESP_LOGI(TAG, "  LAPORAN POWER BUDGET - Siklus #%d", cycle_count);
    ESP_LOGI(TAG, "============================================================");

    ESP_LOGI(TAG, "\n  --- Waktu per Fase (Siklus Ini) ---");
    ESP_LOGI(TAG, "  Transisi (wakeup) : %5lld us (%5.1f ms)", transition_us, transition_us / 1000.0f);
    ESP_LOGI(TAG, "  Sensor Read       : %5lld us (%5.1f ms)", sensor_us, sensor_us / 1000.0f);
    ESP_LOGI(TAG, "  Processing        : %5lld us (%5.1f ms)", process_us, process_us / 1000.0f);
    ESP_LOGI(TAG, "  Total Aktif       : %5lld us (%5.1f ms)", cycle_active_us, cycle_active_us / 1000.0f);
    ESP_LOGI(TAG, "  Deep Sleep        : %d detik", SLEEP_DURATION_SEC);

    // Duty cycle siklus ini
    float total_cycle_us = (float)cycle_active_us + (float)sleep_us;
    float duty_cycle = (float)cycle_active_us / total_cycle_us * 100.0f;

    ESP_LOGI(TAG, "\n  --- Duty Cycle ---");
    ESP_LOGI(TAG, "  Aktif / Total     : %.1f ms / %.1f s",
             cycle_active_us / 1000.0f, total_cycle_us / 1000000.0f);
    ESP_LOGI(TAG, "  Duty Cycle        : %.4f%%", duty_cycle);

    // Arus rata-rata
    float active_fraction = (float)cycle_active_us / total_cycle_us;
    float sleep_fraction = (float)sleep_us / total_cycle_us;
    float avg_current = (CURRENT_ACTIVE_MA * active_fraction) +
                        (CURRENT_DEEP_SLEEP_MA * sleep_fraction);

    ESP_LOGI(TAG, "\n  --- Estimasi Arus ---");
    ESP_LOGI(TAG, "  Arus aktif        : %.1f mA (%.4f%% waktu)", CURRENT_ACTIVE_MA, active_fraction * 100);
    ESP_LOGI(TAG, "  Arus deep sleep   : %.3f mA (%.4f%% waktu)", CURRENT_DEEP_SLEEP_MA, sleep_fraction * 100);
    ESP_LOGI(TAG, "  Arus rata-rata    : %.4f mA", avg_current);

    // Battery life
    float battery_hours = (float)BATTERY_CAPACITY_MAH / avg_current;
    float battery_days = battery_hours / 24.0f;
    float battery_months = battery_days / 30.0f;
    float battery_years = battery_days / 365.0f;

    ESP_LOGI(TAG, "\n  --- Estimasi Battery Life (%d mAh) ---", BATTERY_CAPACITY_MAH);
    ESP_LOGI(TAG, "  Jam   : %.0f", battery_hours);
    ESP_LOGI(TAG, "  Hari  : %.1f", battery_days);
    ESP_LOGI(TAG, "  Bulan : %.1f", battery_months);
    if (battery_years > 1.0f) {
        ESP_LOGI(TAG, "  Tahun : %.1f", battery_years);
    }

    // ===== STATISTIK KUMULATIF =====
    if (cycle_count > 1) {
        ESP_LOGI(TAG, "\n  --- Statistik Kumulatif (%d siklus) ---", cycle_count);
        ESP_LOGI(TAG, "  Total sensor reads  : %d", total_sensor_readings);
        ESP_LOGI(TAG, "  Total waktu aktif   : %lld us (%.1f ms)",
                 total_active_us, total_active_us / 1000.0f);
        ESP_LOGI(TAG, "  Rata-rata per siklus: %.1f ms",
                 (float)total_active_us / cycle_count / 1000.0f);
        ESP_LOGI(TAG, "  Min siklus          : %.1f ms", min_cycle_us / 1000.0f);
        ESP_LOGI(TAG, "  Max siklus          : %.1f ms", max_cycle_us / 1000.0f);

        // Breakdown waktu kumulatif
        ESP_LOGI(TAG, "\n  Breakdown Waktu Kumulatif:");
        float total_time = (float)(total_active_us + total_sleep_us);
        ESP_LOGI(TAG, "  Sensor     : %6.1f ms (%5.2f%%)",
                 total_sensor_us / 1000.0f, total_sensor_us / total_time * 100);
        ESP_LOGI(TAG, "  Processing : %6.1f ms (%5.2f%%)",
                 total_process_us / 1000.0f, total_process_us / total_time * 100);
        ESP_LOGI(TAG, "  Transisi   : %6.1f ms (%5.2f%%)",
                 total_transition_us / 1000.0f, total_transition_us / total_time * 100);
        ESP_LOGI(TAG, "  Sleep      : %6.0f s  (%5.2f%%)",
                 total_sleep_us / 1000000.0f, total_sleep_us / total_time * 100);
    }

    // Tabel perbandingan battery life untuk berbagai skenario
    ESP_LOGI(TAG, "\n  --- Perbandingan Skenario Battery Life ---");
    ESP_LOGI(TAG, "  Skenario               | Arus(mA) | Hari  | Bulan");
    ESP_LOGI(TAG, "  -----------------------|----------|-------|------");

    struct {
        const char *name;
        float current;
    } scenarios[] = {
        {"Always Active (no sleep)", CURRENT_ACTIVE_MA},
        {"Current Config",           avg_current},
        {"With WiFi (duty cycle)",   avg_current + (CURRENT_WIFI_MA * active_fraction)},
        {"Light Sleep mode",        CURRENT_LIGHT_SLEEP_MA + (CURRENT_ACTIVE_MA * active_fraction)},
        {"Deep Sleep only",          CURRENT_DEEP_SLEEP_MA},
        {"Hibernate only",           CURRENT_HIBERNATE_MA},
    };

    for (int i = 0; i < 6; i++) {
        float hrs = BATTERY_CAPACITY_MAH / scenarios[i].current;
        float dys = hrs / 24.0f;
        float mos = dys / 30.0f;
        ESP_LOGI(TAG, "  %-23s| %8.4f | %5.1f | %5.1f",
                 scenarios[i].name, scenarios[i].current, dys, mos);
    }
    ESP_LOGI(TAG, "  -----------------------|----------|-------|------");

    // State timeline ASCII
    ESP_LOGI(TAG, "\n  --- State Timeline (1 siklus) ---");
    int total_width = 50;
    int active_chars = (int)(duty_cycle / 100.0f * total_width);
    if (active_chars < 1) active_chars = 1;
    int sleep_chars = total_width - active_chars;

    char timeline[55];
    memset(timeline, 'Z', total_width);
    for (int i = 0; i < active_chars; i++) timeline[i] = 'A';
    timeline[total_width] = '\0';

    ESP_LOGI(TAG, "  [%s]", timeline);
    ESP_LOGI(TAG, "   A=Aktif, Z=Sleep | Duty: %.4f%%", duty_cycle);

    ESP_LOGI(TAG, "\n============================================================");
}

/**
 * Masuk deep sleep
 */
static void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "Masuk deep sleep selama %d detik...", SLEEP_DURATION_SEC);
    ESP_LOGI(TAG, "Siklus berikutnya: #%d", cycle_count + 1);

    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_DURATION_SEC * 1000000ULL);

    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}

void app_main(void)
{
    int64_t cycle_start = esp_timer_get_time();
    cycle_count++;

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 Power Budget Analysis");
    ESP_LOGI(TAG, "  Siklus #%d / %d", cycle_count, MAX_CYCLES);
    ESP_LOGI(TAG, "============================================");

    // Analisis wakeup
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wakeup: Timer");
    } else {
        ESP_LOGI(TAG, "Wakeup: Power-on/Reset");
        // Reset semua statistik
        cycle_count = 1;
        total_active_us = 0;
        total_sensor_us = 0;
        total_process_us = 0;
        total_transition_us = 0;
        total_sleep_us = 0;
        min_cycle_us = INT64_MAX;
        max_cycle_us = 0;
        total_sensor_readings = 0;
    }

    init_led();
    gpio_set_level(LED_PIN, 1);

    // === Fase aktif: baca sensor ===
    int64_t sensor_us = phase_sensor_read();

    // === Fase aktif: processing ===
    int64_t process_us = phase_processing();

    // Hitung total waktu aktif siklus ini
    int64_t cycle_end = esp_timer_get_time();
    int64_t cycle_active_us = cycle_end - cycle_start;
    total_active_us += cycle_active_us;

    // === Laporan ===
    print_power_budget_report(sensor_us, process_us, cycle_active_us);

    gpio_set_level(LED_PIN, 0);

    // Cek apakah sudah maks siklus
    if (cycle_count >= MAX_CYCLES) {
        ESP_LOGW(TAG, "\nMaksimum siklus tercapai (%d). Berhenti.", MAX_CYCLES);
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }

    // Masuk deep sleep
    enter_deep_sleep();
}
