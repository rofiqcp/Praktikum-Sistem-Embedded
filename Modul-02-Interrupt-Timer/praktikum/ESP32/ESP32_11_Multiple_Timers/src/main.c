/**
 * @file main.c
 * @brief Program 11 - Multiple Timers (3 Timer Beda Frekuensi)
 * @details Tiga timer berjalan bersamaan dengan frekuensi berbeda,
 *          masing-masing mengontrol LED sendiri. Demonstrasi
 *          concurrency hardware timer.
 *
 * @modul    Modul 02 - Interrupt & Timer
 * @board    ESP32 DOIT DevKit V1 / Lolin S2 Mini / ESP32-S3
 * @framework ESP-IDF
 *
 * @koneksi Hardware:
 *   ESP32 GPIO2  --> LED1 (+) --> 220Ω --> GND  (fast: 200ms)
 *   ESP32 GPIO4  --> LED2 (+) --> 220Ω --> GND  (medium: 500ms)
 *   ESP32 GPIO5  --> LED3 (+) --> 220Ω --> GND  (slow: 1000ms)
 *
 * @cara_kerja:
 *   1. Tiga GPTimer dibuat dengan alarm interval berbeda
 *   2. Setiap timer callback men-toggle LED masing-masing
 *   3. Ketiga timer berjalan secara independen dan bersamaan
 *   4. Main loop hanya monitoring counter dari setiap timer
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "MULTI_TIMER";

/* State untuk setiap timer */
static volatile uint32_t timer1_count = 0;
static volatile uint32_t timer2_count = 0;
static volatile uint32_t timer3_count = 0;
static volatile bool led1_state = false;
static volatile bool led2_state = false;
static volatile bool led3_state = false;

/**
 * @brief Callback timer 1 (fast)
 */
static bool IRAM_ATTR timer1_cb(gptimer_handle_t timer,
                                 const gptimer_alarm_event_data_t *edata,
                                 void *user_ctx)
{
    led1_state = !led1_state;
    gpio_set_level(LED1_PIN, led1_state ? 1 : 0);
    timer1_count++;
    return false;
}

/**
 * @brief Callback timer 2 (medium)
 */
static bool IRAM_ATTR timer2_cb(gptimer_handle_t timer,
                                 const gptimer_alarm_event_data_t *edata,
                                 void *user_ctx)
{
    led2_state = !led2_state;
    gpio_set_level(LED2_PIN, led2_state ? 1 : 0);
    timer2_count++;
    return false;
}

/**
 * @brief Callback timer 3 (slow)
 */
static bool IRAM_ATTR timer3_cb(gptimer_handle_t timer,
                                 const gptimer_alarm_event_data_t *edata,
                                 void *user_ctx)
{
    led3_state = !led3_state;
    gpio_set_level(LED3_PIN, led3_state ? 1 : 0);
    timer3_count++;
    return false;
}

/**
 * @brief Inisialisasi GPIO untuk 3 LED
 */
static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN) | (1ULL << LED3_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(LED1_PIN, 0);
    gpio_set_level(LED2_PIN, 0);
    gpio_set_level(LED3_PIN, 0);
}

/**
 * @brief Membuat dan start satu GPTimer
 */
static gptimer_handle_t create_timer(uint32_t interval_ms,
                                      gptimer_alarm_cb_t callback)
{
    gptimer_handle_t timer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = (uint64_t)interval_ms * 1000,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));

    return timer;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Multiple Timers Demo ===");
    ESP_LOGI(TAG, "Timer 1: %d ms (LED GPIO%d)", TIMER1_INTERVAL_MS, LED1_PIN);
    ESP_LOGI(TAG, "Timer 2: %d ms (LED GPIO%d)", TIMER2_INTERVAL_MS, LED2_PIN);
    ESP_LOGI(TAG, "Timer 3: %d ms (LED GPIO%d)", TIMER3_INTERVAL_MS, LED3_PIN);

    gpio_init();

    /* Buat 3 timer dengan interval berbeda */
    gptimer_handle_t t1 = create_timer(TIMER1_INTERVAL_MS, timer1_cb);
    gptimer_handle_t t2 = create_timer(TIMER2_INTERVAL_MS, timer2_cb);
    gptimer_handle_t t3 = create_timer(TIMER3_INTERVAL_MS, timer3_cb);

    ESP_LOGI(TAG, "All 3 timers started!");

    /* Main loop: monitoring */
    while (1) {
        ESP_LOGI(TAG, "T1(%dms): %lu [%s] | T2(%dms): %lu [%s] | T3(%dms): %lu [%s]",
                 TIMER1_INTERVAL_MS, (unsigned long)timer1_count,
                 led1_state ? "ON" : "OFF",
                 TIMER2_INTERVAL_MS, (unsigned long)timer2_count,
                 led2_state ? "ON" : "OFF",
                 TIMER3_INTERVAL_MS, (unsigned long)timer3_count,
                 led3_state ? "ON" : "OFF");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    /* Cleanup (unreachable) */
    gptimer_stop(t1); gptimer_disable(t1); gptimer_del_timer(t1);
    gptimer_stop(t2); gptimer_disable(t2); gptimer_del_timer(t2);
    gptimer_stop(t3); gptimer_disable(t3); gptimer_del_timer(t3);
}
