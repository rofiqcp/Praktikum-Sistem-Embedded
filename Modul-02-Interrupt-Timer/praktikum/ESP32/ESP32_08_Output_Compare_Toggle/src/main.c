/**
 * @file main.c
 * @brief Program 08 - Output Compare Toggle
 * @details Timer alarm men-toggle GPIO secara presisi tanpa intervensi
 *          CPU di main loop. Menghasilkan sinyal square wave pada pin output.
 *          Frekuensi dapat diukur dengan oscilloscope.
 *
 * @modul    Modul 02 - Interrupt & Timer
 * @board    ESP32 DOIT DevKit V1 / Lolin S2 Mini / ESP32-S3
 * @framework ESP-IDF
 *
 * @koneksi Hardware:
 *   ESP32 GPIO2  --> LED (+) --> 220Ω --> GND
 *   (Opsional: hubungkan oscilloscope probe ke GPIO2)
 *
 * @cara_kerja:
 *   1. GPTimer dikonfigurasi untuk alarm periodik
 *   2. Pada setiap alarm, callback toggle GPIO langsung di ISR
 *   3. Main loop hanya melakukan monitoring (tidak mempengaruhi timing)
 *   4. Ini mensimulasikan Output Compare Toggle mode pada STM32
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "OC_TOGGLE";

static volatile bool output_state = false;
static volatile uint32_t toggle_count = 0;
static volatile int64_t last_toggle_us = 0;

/**
 * @brief Alarm callback - toggle output pin langsung di ISR
 * Ini mensimulasikan OC toggle mode: hardware timer men-toggle pin
 * tanpa software intervention di main loop.
 */
static bool IRAM_ATTR timer_alarm_cb(gptimer_handle_t timer,
                                      const gptimer_alarm_event_data_t *edata,
                                      void *user_ctx)
{
    output_state = !output_state;
    gpio_set_level(OUTPUT_PIN, output_state ? 1 : 0);
    toggle_count++;
    last_toggle_us = esp_timer_get_time();

    return false;
}

/**
 * @brief Inisialisasi GPIO output
 */
static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(OUTPUT_PIN, 0);
}

/**
 * @brief Inisialisasi GPTimer untuk output compare toggle
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

    /* Alarm period = half-period of desired frequency
     * Untuk 1 Hz toggle: period = 500000 us (0.5s) */
    uint64_t alarm_ticks = TIMER_RESOLUTION_HZ / (2 * TOGGLE_FREQ_HZ);

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = alarm_ticks,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

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
    ESP_LOGI(TAG, "=== Output Compare Toggle Demo ===");
    ESP_LOGI(TAG, "Target frequency: %d Hz (period: %d us)",
             TOGGLE_FREQ_HZ, 1000000 / TOGGLE_FREQ_HZ);
    ESP_LOGI(TAG, "Output pin: GPIO%d", OUTPUT_PIN);

    gpio_init();
    gptimer_handle_t timer = timer_init();

    ESP_LOGI(TAG, "Timer started - output toggling at %d Hz", TOGGLE_FREQ_HZ);
    ESP_LOGI(TAG, "Connect oscilloscope to GPIO%d to verify frequency", OUTPUT_PIN);

    /* Main loop: monitoring only - tidak mempengaruhi toggle timing */
    while (1) {
        ESP_LOGI(TAG, "Toggle count: %lu | Output: %s | Last toggle: %lld us",
                 (unsigned long)toggle_count,
                 output_state ? "HIGH" : "LOW",
                 (long long)last_toggle_us);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    /* Cleanup (unreachable) */
    gptimer_stop(timer);
    gptimer_disable(timer);
    gptimer_del_timer(timer);
}
