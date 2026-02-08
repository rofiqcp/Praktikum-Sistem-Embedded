/**
 * ESP32_08_ULP_Coprocessor
 * 
 * Konsep: Simulasi perilaku ULP coprocessor selama deep sleep.
 * - Gunakan timer wakeup periodik (interval pendek)
 * - Baca ADC saat bangun, simpan di RTC memory
 * - Jika nilai melebihi threshold, bangunkan CPU utama (simulasi ULP)
 * - Demonstrasi konsep ULP yang berjalan mandiri selama deep sleep
 * 
 * Wiring: Potensiometer pada GPIO34 (ADC1_CH6), LED pada GPIO2
 * 
 * Framework: ESP-IDF (bukan Arduino)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static const char *TAG = "ULP_SIM";

#define LED_PIN             GPIO_NUM_2
#define ADC_CHANNEL         ADC_CHANNEL_6       // GPIO34
#define ADC_ATTEN           ADC_ATTEN_DB_12     // Full range 0-3.3V
#define ULP_CHECK_INTERVAL  5                   // Interval cek ULP (detik)
#define ADC_THRESHOLD       2000                // Threshold ADC (0-4095)
#define MAX_SAMPLES         20                  // Maksimum sampel yang disimpan

// Data RTC - bertahan selama deep sleep
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int ulp_check_count = 0;
RTC_DATA_ATTR static int threshold_exceeded_count = 0;
RTC_DATA_ATTR static int adc_samples[MAX_SAMPLES];
RTC_DATA_ATTR static int sample_index = 0;
RTC_DATA_ATTR static int accumulated_sum = 0;
RTC_DATA_ATTR static bool main_cpu_wakeup = false;

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
static int read_adc_value(void)
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

    int adc_raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw));
    adc_oneshot_del_unit(adc_handle);

    return adc_raw;
}

/**
 * Simulasi pengecekan ULP - baca ADC dan cek threshold
 */
static bool ulp_simulate_check(void)
{
    int adc_value = read_adc_value();
    ulp_check_count++;

    // Simpan sampel di buffer circular RTC
    adc_samples[sample_index % MAX_SAMPLES] = adc_value;
    sample_index++;
    accumulated_sum += adc_value;

    ESP_LOGI(TAG, "ULP checked ADC: value=%d, threshold=%d (cek ke-%d)",
             adc_value, ADC_THRESHOLD, ulp_check_count);

    // Cek apakah melebihi threshold
    if (adc_value > ADC_THRESHOLD) {
        threshold_exceeded_count++;
        ESP_LOGW(TAG, ">>> THRESHOLD EXCEEDED! value=%d > %d (exceed ke-%d)",
                 adc_value, ADC_THRESHOLD, threshold_exceeded_count);
        ESP_LOGW(TAG, ">>> ULP akan membangunkan CPU utama!");
        main_cpu_wakeup = true;
        return true;
    }

    ESP_LOGI(TAG, "ULP: Nilai di bawah threshold, kembali tidur...");
    main_cpu_wakeup = false;
    return false;
}

/**
 * Tampilkan semua sampel yang tersimpan
 */
static void print_accumulated_samples(void)
{
    int count = (sample_index < MAX_SAMPLES) ? sample_index : MAX_SAMPLES;
    int start = (sample_index < MAX_SAMPLES) ? 0 : (sample_index % MAX_SAMPLES);

    ESP_LOGI(TAG, "\n--- Sampel ADC Tersimpan di RTC Memory (%d sampel) ---", count);
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % MAX_SAMPLES;
        char marker = (adc_samples[idx] > ADC_THRESHOLD) ? '!' : ' ';
        ESP_LOGI(TAG, "  [%2d] ADC=%4d %c", i + 1, adc_samples[idx], marker);
    }

    if (ulp_check_count > 0) {
        int avg = accumulated_sum / ulp_check_count;
        ESP_LOGI(TAG, "  Rata-rata ADC: %d", avg);
    }
}

/**
 * Tampilkan statistik lengkap
 */
static void print_statistics(void)
{
    ESP_LOGI(TAG, "\n--- Statistik ULP Simulator ---");
    ESP_LOGI(TAG, "Total boot           : %d", boot_count);
    ESP_LOGI(TAG, "Total pengecekan ULP : %d", ulp_check_count);
    ESP_LOGI(TAG, "Threshold exceeded   : %d", threshold_exceeded_count);
    ESP_LOGI(TAG, "Interval cek         : %d detik", ULP_CHECK_INTERVAL);
    ESP_LOGI(TAG, "ADC threshold        : %d", ADC_THRESHOLD);

    if (ulp_check_count > 0) {
        float exceed_pct = (float)threshold_exceeded_count / ulp_check_count * 100.0f;
        ESP_LOGI(TAG, "Persentase exceed    : %.1f%%", exceed_pct);
    }
}

/**
 * Masuk deep sleep untuk simulasi siklus ULP berikutnya
 */
static void enter_ulp_sleep(void)
{
    ESP_LOGI(TAG, "--------------------------------------------");
    ESP_LOGI(TAG, "Masuk deep sleep (simulasi ULP monitoring)...");
    ESP_LOGI(TAG, "Akan bangun dalam %d detik untuk cek ADC", ULP_CHECK_INTERVAL);
    ESP_LOGI(TAG, "--------------------------------------------");

    esp_sleep_enable_timer_wakeup((uint64_t)ULP_CHECK_INTERVAL * 1000000ULL);

    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}

void app_main(void)
{
    boot_count++;

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 ULP Coprocessor Simulator");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Boot ke-%d", boot_count);

    // Analisis penyebab wakeup
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Wakeup: Timer (simulasi periode ULP)");
    } else {
        ESP_LOGI(TAG, "Wakeup: Power-on/Reset (boot awal)");
        // Reset statistik pada boot pertama
        ulp_check_count = 0;
        threshold_exceeded_count = 0;
        sample_index = 0;
        accumulated_sum = 0;
        memset(adc_samples, 0, sizeof(adc_samples));
    }

    init_led();

    // Simulasi pengecekan ULP
    bool threshold_hit = ulp_simulate_check();

    if (threshold_hit) {
        // CPU utama bangun - lakukan aksi lengkap
        ESP_LOGW(TAG, "\n=== CPU UTAMA AKTIF - Threshold Exceeded ===");
        gpio_set_level(LED_PIN, 1);  // LED ON

        // Tampilkan semua data
        print_accumulated_samples();
        print_statistics();

        ESP_LOGW(TAG, "CPU utama selesai memproses, kembali ke mode ULP...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        gpio_set_level(LED_PIN, 0);
    } else {
        // ULP mode - hanya cek singkat
        ESP_LOGI(TAG, "ULP mode: Pengecekan singkat selesai");
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LED_PIN, 0);

        // Setiap 10 cek, tampilkan ringkasan
        if (ulp_check_count % 10 == 0) {
            print_accumulated_samples();
            print_statistics();
        }
    }

    // Kembali tidur
    enter_ulp_sleep();
}
