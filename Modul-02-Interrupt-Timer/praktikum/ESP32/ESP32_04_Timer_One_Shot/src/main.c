/**
 * ============================================================
 *  ESP32_04_Timer_One_Shot
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi one-shot timer menggunakan esp_timer API.
 *    Tekan tombol → LED menyala langsung → setelah 3 detik
 *    timer callback mematikan LED otomatis (auto-shutoff).
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up aktif)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *
 *  Cara Kerja:
 *    1. GPIO4 dikonfigurasi dengan falling-edge interrupt
 *    2. ISR meng-set flag → main loop menyalakan LED
 *    3. Main loop start one-shot timer (3 detik)
 *    4. Setelah 3 detik, timer callback mematikan LED
 *    5. Jika tombol ditekan lagi sebelum timeout, timer di-restart
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

static const char *TAG = "TIMER_ONESHOT";

/* ---- ISR flag ---- */
static volatile bool button_pressed = false;

/* ---- ESP Timer handle ---- */
static esp_timer_handle_t oneshot_timer = NULL;

/* ---- Press counter ---- */
static uint32_t press_count = 0;

/**
 * GPIO ISR Handler - set flag saat tombol ditekan.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    button_pressed = true;
}

/**
 * One-shot timer callback - dipanggil setelah DELAY_MS.
 * Mematikan LED (auto-shutoff).
 * Dispatch method: ESP_TIMER_TASK → berjalan di timer task context.
 */
static void oneshot_timer_cb(void *arg)
{
    gpio_set_level(LED_PIN, 0);
    ESP_LOGI(TAG, "Timer expired! LED OFF (auto-shutoff setelah %d ms)", DELAY_MS);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 One-Shot Timer Demo ===");
    ESP_LOGI(TAG, "Button pin : GPIO%d", BUTTON_PIN);
    ESP_LOGI(TAG, "LED pin    : GPIO%d", LED_PIN);
    ESP_LOGI(TAG, "Delay      : %d ms", DELAY_MS);

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

    /* ---- Buat one-shot timer (belum distart) ---- */
    esp_timer_create_args_t timer_args = {
        .callback        = oneshot_timer_cb,
        .arg             = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "led_shutoff",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &oneshot_timer));

    ESP_LOGI(TAG, "Sistem siap. Tekan tombol untuk menyalakan LED...");

    /* ---- Konversi delay ke microseconds ---- */
    const int64_t delay_us = (int64_t)DELAY_MS * 1000;

    /* ---- Main Loop ---- */
    while (1) {
        if (button_pressed) {
            button_pressed = false;
            press_count++;

            /* Nyalakan LED langsung */
            gpio_set_level(LED_PIN, 1);
            ESP_LOGI(TAG, "Button #%lu → LED ON | Timer %d ms dimulai...",
                     (unsigned long)press_count, DELAY_MS);

            /*
             * Stop timer dulu (jika masih berjalan dari press sebelumnya),
             * lalu start ulang. esp_timer_stop() return error jika timer
             * tidak aktif — kita abaikan error tersebut.
             */
            esp_timer_stop(oneshot_timer);
            ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, delay_us));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
