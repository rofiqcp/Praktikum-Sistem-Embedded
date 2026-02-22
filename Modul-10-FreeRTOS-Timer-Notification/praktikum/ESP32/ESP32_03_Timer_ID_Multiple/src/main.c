/*
 * ESP32_03_Timer_ID_Multiple
 * ===========================
 * Single callback function controlling 3 timers with different IDs.
 * pvTimerGetTimerID() distinguishes which timer fired.
 * Each timer has a different period and controls a different LED.
 *
 * Hardware: 3x LED on GPIO2, GPIO4, GPIO5
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "TIMER_MULTI";

#define LED1_GPIO   GPIO_NUM_2
#define LED2_GPIO   GPIO_NUM_4
#define LED3_GPIO   GPIO_NUM_5

/* Timer IDs */
#define TIMER_ID_LED1   0
#define TIMER_ID_LED2   1
#define TIMER_ID_LED3   2

/* Timer periods */
#define TIMER1_PERIOD_MS    500
#define TIMER2_PERIOD_MS    1000
#define TIMER3_PERIOD_MS    2000

static TimerHandle_t xTimers[3] = { NULL };
static bool led_states[3] = { false, false, false };
static uint32_t timer_counts[3] = { 0, 0, 0 };
static const gpio_num_t led_gpios[3] = { LED1_GPIO, LED2_GPIO, LED3_GPIO };
static const uint32_t timer_periods[3] = { TIMER1_PERIOD_MS, TIMER2_PERIOD_MS, TIMER3_PERIOD_MS };
static const char *timer_names[3] = { "LED1(GPIO2)", "LED2(GPIO4)", "LED3(GPIO5)" };

/*
 * Single callback for all 3 timers.
 * Uses pvTimerGetTimerID() to determine which timer fired.
 */
static void common_timer_callback(TimerHandle_t xTimer)
{
    uint32_t timer_id = (uint32_t)(uintptr_t)pvTimerGetTimerID(xTimer);

    if (timer_id >= 3) {
        ESP_LOGE(TAG, "ERROR,Invalid timer ID: %lu", (unsigned long)timer_id);
        return;
    }

    timer_counts[timer_id]++;
    led_states[timer_id] = !led_states[timer_id];
    gpio_set_level(led_gpios[timer_id], led_states[timer_id] ? 1 : 0);

    const char *timer_pcName = pcTimerGetName(xTimer);

    ESP_LOGI(TAG, "EVENT,TIMER_FIRE,id=%lu,name=%s,led=%s,gpio=%d,count=%lu,period=%lu_ms,tick=%lu",
             (unsigned long)timer_id,
             timer_pcName,
             led_states[timer_id] ? "ON" : "OFF",
             led_gpios[timer_id],
             (unsigned long)timer_counts[timer_id],
             (unsigned long)timer_periods[timer_id],
             (unsigned long)xTaskGetTickCount());
}

static void leds_init(void)
{
    for (int i = 0; i < 3; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << led_gpios[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(led_gpios[i], 0);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Timer ID Multiple Demo ===");
    ESP_LOGI(TAG, "INFO,3 timers sharing 1 callback, each controlling different LED");
    ESP_LOGI(TAG, "INFO,Timer0: GPIO%d @ %d ms", LED1_GPIO, TIMER1_PERIOD_MS);
    ESP_LOGI(TAG, "INFO,Timer1: GPIO%d @ %d ms", LED2_GPIO, TIMER2_PERIOD_MS);
    ESP_LOGI(TAG, "INFO,Timer2: GPIO%d @ %d ms", LED3_GPIO, TIMER3_PERIOD_MS);

    leds_init();

    /* Create 3 timers with different IDs, same callback */
    const char *names[3] = { "Timer_LED1", "Timer_LED2", "Timer_LED3" };

    for (int i = 0; i < 3; i++) {
        xTimers[i] = xTimerCreate(
            names[i],
            pdMS_TO_TICKS(timer_periods[i]),
            pdTRUE,                         /* Auto-reload */
            (void *)(uintptr_t)i,           /* Timer ID = index */
            common_timer_callback           /* Same callback for all */
        );

        if (xTimers[i] == NULL) {
            ESP_LOGE(TAG, "ERROR,Failed to create timer %d", i);
            return;
        }

        /* Verify the ID was set correctly */
        uint32_t check_id = (uint32_t)(uintptr_t)pvTimerGetTimerID(xTimers[i]);
        ESP_LOGI(TAG, "INFO,Created timer '%s' with ID=%lu, period=%lu ms",
                 names[i], (unsigned long)check_id, (unsigned long)timer_periods[i]);
    }

    /* Start all timers */
    for (int i = 0; i < 3; i++) {
        if (xTimerStart(xTimers[i], pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGE(TAG, "ERROR,Failed to start timer %d", i);
        } else {
            ESP_LOGI(TAG, "INFO,Timer %d started", i);
        }
    }

    /* Monitor task - periodic status */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "STATUS,t0_count=%lu,t1_count=%lu,t2_count=%lu,"
                 "led0=%s,led1=%s,led2=%s,tick=%lu",
                 (unsigned long)timer_counts[0],
                 (unsigned long)timer_counts[1],
                 (unsigned long)timer_counts[2],
                 led_states[0] ? "ON" : "OFF",
                 led_states[1] ? "ON" : "OFF",
                 led_states[2] ? "ON" : "OFF",
                 (unsigned long)xTaskGetTickCount());

        /* Verify expected ratios: Timer0 should fire ~4x, Timer1 ~2x, Timer2 ~1x in 2s */
        if (timer_counts[0] > 0 && timer_counts[2] > 0) {
            float ratio = (float)timer_counts[0] / (float)timer_counts[2];
            ESP_LOGI(TAG, "RATIO,t0/t2=%.2f (expected ~4.0)", ratio);
        }
    }
}
