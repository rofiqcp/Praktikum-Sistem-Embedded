/*
 * ESP32_01_Software_Timer_Basic
 * =============================
 * Demonstrates FreeRTOS software timers: one-shot and auto-reload.
 * - One-shot timer: turns LED ON after 3 seconds, fires once.
 * - Auto-reload timer: toggles LED every 1 second, repeats automatically.
 * - Shows that timer callbacks run in the timer daemon task context.
 *
 * Hardware: 1x LED on GPIO2 (built-in)
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "TIMER_BASIC";

#define LED_GPIO        GPIO_NUM_2
#define ONESHOT_PERIOD_MS   3000
#define AUTORELOAD_PERIOD_MS 1000

static TimerHandle_t xOneShotTimer = NULL;
static TimerHandle_t xAutoReloadTimer = NULL;
static bool led_state = false;
static uint32_t oneshot_count = 0;
static uint32_t autoreload_count = 0;

static void led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);
}

static void oneshot_timer_callback(TimerHandle_t xTimer)
{
    oneshot_count++;
    /* Turn LED ON */
    gpio_set_level(LED_GPIO, 1);
    led_state = true;

    /* Show that callback runs in timer daemon task */
    TaskHandle_t xCurrentTask = xTimerGetTimerDaemonTaskHandle();
    const char *task_name = pcTaskGetName(xCurrentTask);

    ESP_LOGI(TAG, "EVENT,ONE_SHOT,FIRED,count=%lu,led=ON,daemon_task=%s,tick=%lu",
             (unsigned long)oneshot_count, task_name,
             (unsigned long)xTaskGetTickCount());

    ESP_LOGI(TAG, "INFO,One-shot timer fired after %d ms - LED turned ON", ONESHOT_PERIOD_MS);
    ESP_LOGI(TAG, "INFO,Callback runs in task: '%s' (timer daemon)", task_name);
}

static void autoreload_timer_callback(TimerHandle_t xTimer)
{
    autoreload_count++;
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state ? 1 : 0);

    TaskHandle_t xCurrentTask = xTimerGetTimerDaemonTaskHandle();
    const char *task_name = pcTaskGetName(xCurrentTask);

    ESP_LOGI(TAG, "EVENT,AUTO_RELOAD,TOGGLE,count=%lu,led=%s,daemon_task=%s,tick=%lu",
             (unsigned long)autoreload_count,
             led_state ? "ON" : "OFF",
             task_name,
             (unsigned long)xTaskGetTickCount());
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Software Timer Basic Demo ===");
    ESP_LOGI(TAG, "INFO,One-shot timer period: %d ms", ONESHOT_PERIOD_MS);
    ESP_LOGI(TAG, "INFO,Auto-reload timer period: %d ms", AUTORELOAD_PERIOD_MS);
    ESP_LOGI(TAG, "INFO,LED GPIO: %d", LED_GPIO);

    led_init();

    /* Create one-shot timer (pdFALSE = one-shot) */
    xOneShotTimer = xTimerCreate(
        "OneShotTimer",
        pdMS_TO_TICKS(ONESHOT_PERIOD_MS),
        pdFALSE,       /* One-shot */
        (void *)0,
        oneshot_timer_callback
    );

    /* Create auto-reload timer (pdTRUE = auto-reload) */
    xAutoReloadTimer = xTimerCreate(
        "AutoReloadTimer",
        pdMS_TO_TICKS(AUTORELOAD_PERIOD_MS),
        pdTRUE,         /* Auto-reload */
        (void *)1,
        autoreload_timer_callback
    );

    if (xOneShotTimer == NULL || xAutoReloadTimer == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create timers!");
        return;
    }

    ESP_LOGI(TAG, "INFO,Timers created successfully");
    ESP_LOGI(TAG, "INFO,Starting one-shot timer (LED ON after %d ms)...", ONESHOT_PERIOD_MS);

    /* Start one-shot timer first */
    if (xTimerStart(xOneShotTimer, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "ERROR,Failed to start one-shot timer");
    }

    ESP_LOGI(TAG, "INFO,One-shot timer started at tick=%lu",
             (unsigned long)xTaskGetTickCount());

    /* Wait for one-shot to complete, then start auto-reload */
    vTaskDelay(pdMS_TO_TICKS(ONESHOT_PERIOD_MS + 500));

    ESP_LOGI(TAG, "INFO,Starting auto-reload timer (toggle every %d ms)...", AUTORELOAD_PERIOD_MS);

    if (xTimerStart(xAutoReloadTimer, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "ERROR,Failed to start auto-reload timer");
    }

    /* Main task monitors timer activity */
    uint32_t last_autoreload = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "STATUS,oneshot_fires=%lu,autoreload_fires=%lu,led=%s,tick=%lu",
                 (unsigned long)oneshot_count,
                 (unsigned long)autoreload_count,
                 led_state ? "ON" : "OFF",
                 (unsigned long)xTaskGetTickCount());

        /* Demonstrate stopping and restarting the auto-reload timer */
        if (autoreload_count >= 10 && autoreload_count < 15 && last_autoreload < 10) {
            ESP_LOGW(TAG, "ACTION,Stopping auto-reload timer for 3 seconds...");
            xTimerStop(xAutoReloadTimer, pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(3000));
            ESP_LOGI(TAG, "ACTION,Restarting auto-reload timer");
            xTimerStart(xAutoReloadTimer, pdMS_TO_TICKS(100));
        }

        /* Demonstrate restarting the one-shot timer */
        if (autoreload_count >= 20 && last_autoreload < 20) {
            ESP_LOGI(TAG, "ACTION,Restarting one-shot timer again");
            gpio_set_level(LED_GPIO, 0);
            led_state = false;
            xTimerStart(xOneShotTimer, pdMS_TO_TICKS(100));
        }

        last_autoreload = autoreload_count;
    }
}
