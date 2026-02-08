/**
 * @file main.c
 * @brief Program 07 - Timer Cascade / Multi-Timer Chaining
 * @details Demonstrasi timer chaining: Timer cepat (100ms) men-trigger
 *          counter, setiap N counter tercapai toggle LED lambat.
 *          Simulasi master-slave timer untuk precise long delay.
 *
 * @modul    Modul 02 - Interrupt & Timer
 * @board    ESP32 DOIT DevKit V1 / Lolin S2 Mini / ESP32-S3
 * @framework ESP-IDF
 *
 * @koneksi Hardware:
 *   ESP32 GPIO2  --> LED1 (+) --> 220Ω --> GND  (fast blink)
 *   ESP32 GPIO4  --> LED2 (+) --> 220Ω --> GND  (slow blink, cascaded)
 *
 * @cara_kerja:
 *   1. GPTimer dikonfigurasi dengan alarm periodik setiap TIMER_FAST_MS
 *   2. Setiap alarm callback, counter internal di-increment
 *   3. LED fast di-toggle setiap alarm
 *   4. Saat counter mencapai CASCADE_COUNT, LED slow di-toggle (cascaded event)
 *   5. Ini mensimulasikan timer cascade tanpa hardware timer chaining
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "TIMER_CASCADE";

/* Volatile counters shared with ISR */
static volatile uint32_t fast_tick_count = 0;
static volatile uint32_t cascade_event_count = 0;
static volatile bool fast_led_state = false;
static volatile bool slow_led_state = false;

/**
 * @brief Callback alarm timer - IRAM_ATTR untuk keamanan ISR
 */
static bool IRAM_ATTR timer_alarm_cb(gptimer_handle_t timer,
                                      const gptimer_alarm_event_data_t *edata,
                                      void *user_ctx)
{
    fast_tick_count++;

    /* Toggle LED fast setiap alarm */
    fast_led_state = !fast_led_state;
    gpio_set_level(LED_FAST_PIN, fast_led_state ? 1 : 0);

    /* Cascade: setiap CASCADE_COUNT tick, toggle LED slow */
    if ((fast_tick_count % CASCADE_COUNT) == 0) {
        slow_led_state = !slow_led_state;
        gpio_set_level(LED_SLOW_PIN, slow_led_state ? 1 : 0);
        cascade_event_count++;
    }

    return false; /* no high-priority task woken */
}

/**
 * @brief Inisialisasi GPIO untuk 2 LED
 */
static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_FAST_PIN) | (1ULL << LED_SLOW_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(LED_FAST_PIN, 0);
    gpio_set_level(LED_SLOW_PIN, 0);
}

/**
 * @brief Inisialisasi GPTimer dengan alarm periodik
 */
static gptimer_handle_t timer_init(void)
{
    gptimer_handle_t timer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

    /* Alarm periodik setiap TIMER_FAST_MS */
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = TIMER_FAST_MS * 1000, /* ms -> us (1MHz resolution) */
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    /* Register callback */
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));

    return timer;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Timer Cascade Demo ===");
    ESP_LOGI(TAG, "Fast timer: %d ms interval", TIMER_FAST_MS);
    ESP_LOGI(TAG, "Cascade every: %d ticks = %d ms",
             CASCADE_COUNT, TIMER_FAST_MS * CASCADE_COUNT);

    gpio_init();
    gptimer_handle_t timer = timer_init();

    ESP_LOGI(TAG, "Timer started. LED fast = GPIO%d, LED slow = GPIO%d",
             LED_FAST_PIN, LED_SLOW_PIN);

    /* Main loop: hanya monitoring dan logging */
    uint32_t last_fast = 0;
    uint32_t last_cascade = 0;

    while (1) {
        uint32_t current_fast = fast_tick_count;
        uint32_t current_cascade = cascade_event_count;

        if (current_fast != last_fast || current_cascade != last_cascade) {
            ESP_LOGI(TAG, "Fast ticks: %lu | Cascade events: %lu | "
                     "Fast LED: %s | Slow LED: %s",
                     (unsigned long)current_fast,
                     (unsigned long)current_cascade,
                     fast_led_state ? "ON" : "OFF",
                     slow_led_state ? "ON" : "OFF");
            last_fast = current_fast;
            last_cascade = current_cascade;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* Cleanup (unreachable) */
    gptimer_stop(timer);
    gptimer_disable(timer);
    gptimer_del_timer(timer);
}
