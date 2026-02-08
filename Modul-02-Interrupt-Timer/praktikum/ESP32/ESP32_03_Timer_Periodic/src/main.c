/**
 * ============================================================
 *  ESP32_03_Timer_Periodic
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi Hardware General Purpose Timer (GPTimer)
 *    dengan alarm periodik. Timer menghasilkan interrupt
 *    setiap 1 detik, callback toggle LED dan increment counter.
 *
 *  Koneksi Hardware:
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *
 *  Cara Kerja:
 *    1. GPTimer dikonfigurasi: 1 MHz resolution (1 µs tick)
 *    2. Alarm diset pada 1.000.000 count (= 1 detik)
 *    3. Auto-reload aktif → alarm berulang secara periodik
 *    4. Callback toggle LED dan increment counter
 *    5. Main loop menampilkan counter secara berkala
 * ============================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "TIMER_PERIODIC";

/* ---- Shared variables (callback ↔ main) ---- */
static volatile uint32_t alarm_count = 0;
static volatile bool     led_state   = false;

/**
 * Timer alarm callback - dipanggil dari ISR context.
 * Toggle LED dan increment counter.
 *
 * Return true  → high-priority task woken (request context switch)
 * Return false → no context switch needed
 */
static bool IRAM_ATTR timer_alarm_cb(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t *edata,
                                     void *user_ctx)
{
    /* Toggle LED */
    led_state = !led_state;
    gpio_set_level(LED_PIN, led_state ? 1 : 0);

    /* Increment counter */
    alarm_count++;

    return false;   // Tidak perlu context switch
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 GP Timer Periodic Demo ===");
    ESP_LOGI(TAG, "LED pin        : GPIO%d", LED_PIN);
    ESP_LOGI(TAG, "Timer interval : %d ms", TIMER_INTERVAL_MS);

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

    /* ---- Konfigurasi GP Timer ---- */
    gptimer_handle_t gptimer = NULL;

    gptimer_config_t timer_config = {
        .clk_src    = GPTIMER_CLK_SRC_DEFAULT,
        .direction  = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,   // 1 MHz → 1 µs per tick
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    /* ---- Konfigurasi Alarm (periodik) ---- */
    uint64_t alarm_ticks = (uint64_t)TIMER_INTERVAL_MS * 1000;  // ms → µs (ticks)

    gptimer_alarm_config_t alarm_config = {
        .alarm_count  = alarm_ticks,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,     // Auto-reload → periodik
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    /* ---- Register callback ---- */
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    /* ---- Enable & Start timer ---- */
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    ESP_LOGI(TAG, "Timer berjalan. LED toggle setiap %d ms.", TIMER_INTERVAL_MS);

    /* ---- Main Loop: tampilkan counter ---- */
    uint32_t last_count = 0;
    while (1) {
        uint32_t current = alarm_count;
        if (current != last_count) {
            ESP_LOGI(TAG, "Alarm #%lu | LED = %s",
                     (unsigned long)current,
                     led_state ? "ON" : "OFF");
            last_count = current;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
