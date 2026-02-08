/**
 * ==========================================================================
 * ESP32_03_Deep_Sleep_Timer - Deep Sleep dengan Timer Wakeup (10 detik)
 * ==========================================================================
 * 
 * Modul 14 - Power Management
 * Program 3: Deep sleep with timer wakeup, measure µA
 * 
 * KONSEP DEEP SLEEP:
 * - Hanya RTC controller + ULP coprocessor yang aktif
 * - Konsumsi arus: ~10µA (sangat rendah!)
 * - Pada wakeup, ESP32 melakukan COLD BOOT (restart dari awal)
 * - Semua variabel RAM hilang, KECUALI yang disimpan di RTC memory
 * - RTC_DATA_ATTR: variabel yang bertahan selama deep sleep
 * 
 * PERBEDAAN Light Sleep vs Deep Sleep:
 * ┌────────────────┬─────────────────┬─────────────────┐
 * │ Aspek          │ Light Sleep     │ Deep Sleep      │
 * ├────────────────┼─────────────────┼─────────────────┤
 * │ Arus           │ ~0.8 mA         │ ~10 µA          │
 * │ CPU            │ Paused          │ OFF             │
 * │ RAM            │ Preserved       │ LOST            │
 * │ Wakeup         │ Resume          │ Cold Boot       │
 * │ Wakeup time    │ ~1 ms           │ ~200 ms         │
 * │ RTC Memory     │ Available       │ Available       │
 * └────────────────┴─────────────────┴─────────────────┘
 * 
 * WIRING / KONEKSI:
 * ┌─────────────────────────────────────────────┐
 * │  ESP32          Komponen                     │
 * │  GPIO2  ──────► LED (+) ──► R(220Ω) ──► GND │
 * │                                              │
 * │  Opsional: µA meter SERI dengan VCC          │
 * │  untuk mengukur arus saat deep sleep (~10µA) │
 * └─────────────────────────────────────────────┘
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * [DEEP] ==============================
 * [DEEP] Boot count: 3
 * [DEEP] Wakeup cause: TIMER
 * [DEEP] LED blinking before deep sleep...
 * [DEEP] Entering deep sleep for 10 seconds...
 * [DEEP] (ESP32 will restart after wakeup)
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

static const char *TAG = "DEEP";

/* Konfigurasi */
#define LED_PIN             GPIO_NUM_2      // Onboard LED
#define DEEP_SLEEP_SEC      10              // Durasi deep sleep (10 detik)
#define BLINK_COUNT         5               // Jumlah blink sebelum sleep
#define BLINK_DELAY_MS      200             // Delay per blink

/**
 * RTC_DATA_ATTR: Variabel ini BERTAHAN selama deep sleep!
 * Disimpan di RTC slow memory (8KB available)
 * Tidak hilang saat deep sleep, tapi hilang saat power off
 */
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int64_t last_sleep_time_us = 0;
RTC_DATA_ATTR static int total_awake_time_ms = 0;

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
 * Dapatkan string penyebab wakeup
 */
static const char* get_wakeup_reason_str(esp_sleep_wakeup_cause_t cause)
{
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:     return "TIMER";
        case ESP_SLEEP_WAKEUP_EXT0:      return "EXT0 (single GPIO)";
        case ESP_SLEEP_WAKEUP_EXT1:      return "EXT1 (multi GPIO)";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:  return "TOUCHPAD";
        case ESP_SLEEP_WAKEUP_ULP:       return "ULP";
        case ESP_SLEEP_WAKEUP_UNDEFINED: return "UNDEFINED (first boot / power on)";
        default:                         return "UNKNOWN";
    }
}

/**
 * LED blink pattern untuk indikasi visual
 * Jumlah blink = boot_count (max 5)
 */
static void blink_led(int count)
{
    int blinks = (count > BLINK_COUNT) ? BLINK_COUNT : count;
    ESP_LOGI(TAG, "LED blinking %d kali (boot count indicator)...", blinks);

    for (int i = 0; i < blinks; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY_MS));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY_MS));
    }
}

/**
 * Entry point utama
 * Setiap kali bangun dari deep sleep, app_main dipanggil ulang (cold boot)
 */
void app_main(void)
{
    int64_t wake_time = esp_timer_get_time();

    /* Increment boot counter (RTC memory - survives deep sleep) */
    boot_count++;

    /* Inisialisasi GPIO */
    init_gpio();

    /* Dapatkan penyebab wakeup */
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, "  ESP32 DEEP SLEEP - Timer Wakeup");
    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, "Boot count: %d", boot_count);
    ESP_LOGI(TAG, "Wakeup cause: %s", get_wakeup_reason_str(wakeup_cause));

    /* Jika bukan boot pertama, hitung waktu sleep sebenarnya */
    if (wakeup_cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI(TAG, "Successfully woke up from deep sleep!");
        ESP_LOGI(TAG, "Configured sleep: %d seconds", DEEP_SLEEP_SEC);

        /* Variabel biasa (non-RTC) akan selalu 0 setelah deep sleep */
        int regular_variable = 0; // Ini selalu 0 setelah deep sleep!
        ESP_LOGW(TAG, "Regular variable (non-RTC): %d (always 0 after deep sleep)", regular_variable);
        ESP_LOGI(TAG, "RTC boot_count: %d (survives deep sleep!)", boot_count);
    } else {
        ESP_LOGI(TAG, "First boot (power on / reset)");
        last_sleep_time_us = 0;
        total_awake_time_ms = 0;
    }

    /* Informasi sistem */
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "Total awake time accumulated: %d ms", total_awake_time_ms);

    /* Blink LED sesuai boot count */
    blink_led(boot_count);

    /* Hitung waktu aktif di boot ini */
    int64_t active_duration = esp_timer_get_time() - wake_time;
    int active_ms = (int)(active_duration / 1000);
    total_awake_time_ms += active_ms;

    /* Data terformat untuk Python parser */
    ESP_LOGI(TAG, "DATA,%d,%d,%s,%lu,%d",
             boot_count, DEEP_SLEEP_SEC,
             get_wakeup_reason_str(wakeup_cause),
             (unsigned long)esp_get_free_heap_size(),
             active_ms);

    /* Tampilkan info sebelum masuk deep sleep */
    ESP_LOGI(TAG, "--------------------------------------");
    ESP_LOGI(TAG, "Active time this boot: %d ms", active_ms);
    ESP_LOGI(TAG, "Entering deep sleep for %d seconds...", DEEP_SLEEP_SEC);
    ESP_LOGW(TAG, "Deep sleep: ~10µA, RTC+ULP only");
    ESP_LOGW(TAG, "ESP32 will RESTART after wakeup (cold boot)");
    ESP_LOGI(TAG, "======================================");

    /* Flush semua log sebelum sleep */
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Simpan timestamp sebelum tidur */
    last_sleep_time_us = esp_timer_get_time();

    /* ===== MASUK DEEP SLEEP ===== */
    /* esp_deep_sleep() tidak pernah return - ESP32 akan restart */
    esp_deep_sleep(DEEP_SLEEP_SEC * 1000000ULL);

    /* Kode di bawah ini TIDAK PERNAH dieksekusi */
    ESP_LOGE(TAG, "This should never be printed!");
}
