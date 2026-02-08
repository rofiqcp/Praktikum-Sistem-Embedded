/**
 * ============================================================
 *  ESP32_02_EXTI_Debounce
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    External Interrupt dengan software debounce.
 *    ISR menyimpan timestamp menggunakan esp_timer_get_time().
 *    Main loop memvalidasi apakah cukup waktu telah berlalu
 *    sebelum memproses penekanan tombol (200 ms debounce).
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up aktif)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *
 *  Cara Kerja:
 *    1. ISR dipicu oleh falling edge pada GPIO4
 *    2. ISR menyimpan timestamp (microseconds) ke variabel volatile
 *    3. Main loop membandingkan timestamp dengan waktu terakhir valid
 *    4. Jika interval >= DEBOUNCE_MS → toggle LED (valid press)
 *    5. Jika interval < DEBOUNCE_MS → abaikan (bounce)
 * ============================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "EXTI_DEB";

/* ---- Shared variables (ISR ↔ main) ---- */
static volatile bool    isr_flag       = false;
static volatile int64_t isr_timestamp  = 0;    // Timestamp saat ISR (us)

/* ---- Debounce tracking (main loop only) ---- */
static int64_t  last_valid_time = 0;            // Timestamp terakhir valid (us)
static uint32_t valid_count     = 0;            // Jumlah press valid
static uint32_t bounce_count    = 0;            // Jumlah press diabaikan (bounce)

/* ---- LED state ---- */
static bool led_state = false;

/**
 * ISR Handler - simpan timestamp, set flag.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    isr_timestamp = esp_timer_get_time();   // Microseconds sejak boot
    isr_flag = true;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 EXTI Debounce Demo ===");
    ESP_LOGI(TAG, "Button     : GPIO%d", BUTTON_PIN);
    ESP_LOGI(TAG, "LED        : GPIO%d", LED_PIN);
    ESP_LOGI(TAG, "Debounce   : %d ms", DEBOUNCE_MS);

    /* ---- Konfigurasi LED (output) ---- */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

    /* ---- Konfigurasi Button (input + pull-up + falling edge) ---- */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    /* ---- Install ISR service & attach handler ---- */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, NULL);

    ESP_LOGI(TAG, "Sistem siap. Tekan tombol...");

    /* ---- Konversi debounce threshold ke microseconds ---- */
    const int64_t debounce_us = (int64_t)DEBOUNCE_MS * 1000;

    /* ---- Main Loop ---- */
    while (1) {
        if (isr_flag) {
            isr_flag = false;

            /* Ambil timestamp dari ISR */
            int64_t current_time = isr_timestamp;
            int64_t elapsed_us   = current_time - last_valid_time;

            if (elapsed_us >= debounce_us) {
                /* ---- Valid press (debounced) ---- */
                last_valid_time = current_time;
                valid_count++;

                /* Toggle LED */
                led_state = !led_state;
                gpio_set_level(LED_PIN, led_state ? 1 : 0);

                ESP_LOGI(TAG, "[VALID  ] Press #%lu | LED = %s | dt = %lld ms",
                         (unsigned long)valid_count,
                         led_state ? "ON" : "OFF",
                         (long long)(elapsed_us / 1000));
            } else {
                /* ---- Bounce detected → abaikan ---- */
                bounce_count++;

                ESP_LOGW(TAG, "[BOUNCE ] Diabaikan #%lu | dt = %lld ms (< %d ms)",
                         (unsigned long)bounce_count,
                         (long long)(elapsed_us / 1000),
                         DEBOUNCE_MS);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
