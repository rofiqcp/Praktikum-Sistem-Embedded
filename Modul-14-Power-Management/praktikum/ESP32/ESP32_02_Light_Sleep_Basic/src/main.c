/**
 * ==========================================================================
 * ESP32_02_Light_Sleep_Basic - Light Sleep dengan Timer Auto-Wakeup
 * ==========================================================================
 * 
 * Modul 14 - Power Management
 * Program 2: Enter light sleep mode with timer auto-wakeup
 * 
 * KONSEP LIGHT SLEEP:
 * - CPU di-pause, clock dihentikan sementara
 * - Konsumsi arus: ~0.8mA (jauh lebih rendah dari active ~120mA)
 * - Wi-Fi/BT dimatikan, RTC & peripheral tetap ON
 * - PERBEDAAN KUNCI dari Deep Sleep:
 *   → Eksekusi DILANJUTKAN dari titik terakhir (tidak restart)
 *   → Semua variabel RAM tetap tersimpan
 *   → Wakeup lebih cepat (~1ms vs ~200ms deep sleep)
 * 
 * WIRING / KONEKSI:
 * ┌─────────────────────────────────────────────┐
 * │  ESP32          Komponen                     │
 * │  GPIO2  ──────► LED (+) ──► R(220Ω) ──► GND │
 * │                                              │
 * │  Opsional: Multimeter SERI dengan VCC        │
 * │  untuk mengukur arus saat sleep (~0.8mA)     │
 * └─────────────────────────────────────────────┘
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * [SLEEP] === Cycle 1 ===
 * [SLEEP] LED ON - Preparing for light sleep...
 * [SLEEP] Time before sleep: 1234567 us
 * [SLEEP] Entering light sleep for 5 seconds...
 * [SLEEP] --- Woke up from light sleep! ---
 * [SLEEP] Time after sleep: 6234890 us
 * [SLEEP] Actual sleep duration: 5000.32 ms
 * [SLEEP] Variables preserved! counter=1, sum=42
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

static const char *TAG = "SLEEP";

/* Konfigurasi */
#define LED_PIN             GPIO_NUM_2      // Onboard LED
#define SLEEP_DURATION_US   (5 * 1000000)   // 5 detik dalam mikro-detik
#define TOTAL_CYCLES        10              // Jumlah siklus sleep/wake
#define ACTIVE_TIME_MS      3000            // Waktu aktif sebelum sleep (3 detik)

/* Variabel yang akan bertahan selama light sleep (RAM preserved) */
static int cycle_count = 0;
static float cumulative_sleep_ms = 0;
static float cumulative_active_ms = 0;
static int test_variable = 42;  // Untuk membuktikan variabel preserved

/**
 * Inisialisasi GPIO LED
 */
static void init_gpio(void)
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
 * Cetak ringkasan siklus sleep/wake
 */
static void print_cycle_summary(void)
{
    float total_time = cumulative_sleep_ms + cumulative_active_ms;
    float sleep_pct = (total_time > 0) ? (cumulative_sleep_ms / total_time * 100.0f) : 0;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RINGKASAN LIGHT SLEEP");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Total Cycles       : %d", cycle_count);
    ESP_LOGI(TAG, "Cumulative Sleep   : %.1f ms", cumulative_sleep_ms);
    ESP_LOGI(TAG, "Cumulative Active  : %.1f ms", cumulative_active_ms);
    ESP_LOGI(TAG, "Sleep Ratio        : %.1f%%", sleep_pct);
    ESP_LOGI(TAG, "Est. Avg Current   : ~%.1f mA (blended)",
             0.8f * (sleep_pct / 100.0f) + 120.0f * (1.0f - sleep_pct / 100.0f));
}

/**
 * Entry point utama
 * Light sleep loop: LED ON → cetak info → sleep → bangun → ulangi
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 LIGHT SLEEP - Timer Wakeup");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Sleep duration: %d seconds", SLEEP_DURATION_US / 1000000);
    ESP_LOGI(TAG, "Total cycles: %d", TOTAL_CYCLES);
    ESP_LOGI(TAG, "Light Sleep: CPU paused, ~0.8mA, RAM preserved");

    /* Inisialisasi GPIO */
    init_gpio();

    /* Konfigurasi timer wakeup untuk light sleep */
    esp_err_t ret = esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal konfigurasi timer wakeup: %s", esp_err_to_name(ret));
        return;
    }

    /* Loop utama: siklus sleep/wake */
    while (cycle_count < TOTAL_CYCLES) {
        cycle_count++;
        int64_t active_start = esp_timer_get_time();

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "=== Cycle %d / %d ===", cycle_count, TOTAL_CYCLES);

        /* LED ON - menandakan sedang aktif */
        gpio_set_level(LED_PIN, 1);
        ESP_LOGI(TAG, "LED ON - Aktif selama %d ms sebelum sleep...", ACTIVE_TIME_MS);

        /* Modifikasi variabel untuk membuktikan preservation */
        test_variable += cycle_count;

        /* Tampilkan status sebelum tidur */
        int64_t time_before = esp_timer_get_time();
        ESP_LOGI(TAG, "Time before sleep: %lld us", time_before);
        ESP_LOGI(TAG, "test_variable = %d (akan preserved setelah sleep)", test_variable);
        ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());

        /* Tunggu sebentar dalam mode aktif */
        vTaskDelay(pdMS_TO_TICKS(ACTIVE_TIME_MS));

        int64_t active_end = esp_timer_get_time();
        float active_ms = (float)(active_end - active_start) / 1000.0f;
        cumulative_active_ms += active_ms;

        /* LED OFF - menandakan akan tidur */
        gpio_set_level(LED_PIN, 0);
        ESP_LOGI(TAG, "LED OFF - Entering light sleep for %d seconds...", SLEEP_DURATION_US / 1000000);

        /* Flush log sebelum sleep */
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(100));

        /* ===== MASUK LIGHT SLEEP ===== */
        int64_t sleep_start = esp_timer_get_time();
        esp_err_t sleep_ret = esp_light_sleep_start();
        int64_t sleep_end = esp_timer_get_time();
        /* ===== BANGUN DARI LIGHT SLEEP ===== */

        float actual_sleep_ms = (float)(sleep_end - sleep_start) / 1000.0f;
        cumulative_sleep_ms += actual_sleep_ms;

        /* Cek penyebab wakeup */
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

        ESP_LOGI(TAG, "--- Woke up from light sleep! ---");
        ESP_LOGI(TAG, "Sleep return code: %s", esp_err_to_name(sleep_ret));
        ESP_LOGI(TAG, "Wakeup cause: %d (%s)", cause,
                 cause == ESP_SLEEP_WAKEUP_TIMER ? "TIMER" : "OTHER");
        ESP_LOGI(TAG, "Actual sleep duration: %.2f ms", actual_sleep_ms);
        ESP_LOGI(TAG, "Variables PRESERVED! test_variable=%d, cycle_count=%d",
                 test_variable, cycle_count);

        /* Data terformat untuk Python parser */
        ESP_LOGI(TAG, "DATA,%d,%.2f,%.2f,%.2f,%d",
                 cycle_count, actual_sleep_ms, active_ms,
                 cumulative_sleep_ms, test_variable);
    }

    /* Cetak ringkasan akhir */
    print_cycle_summary();

    ESP_LOGI(TAG, "Light sleep demo selesai. Idle...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
