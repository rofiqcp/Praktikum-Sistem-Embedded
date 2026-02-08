/*
 * ESP32_02_Timer_Period_Change
 * ============================
 * Dynamic timer period change with xTimerChangePeriod().
 * Button press cycles through different blink speeds.
 * Period change takes effect immediately.
 *
 * Hardware: 1x LED on GPIO2, 1x Button on GPIO0 (BOOT)
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "TIMER_PERIOD";

#define LED_GPIO    GPIO_NUM_2
#define BTN_GPIO    GPIO_NUM_0

/* Speed presets in milliseconds */
static const uint32_t speeds_ms[] = { 100, 500, 1000, 2000 };
static const char *speed_names[] = { "FAST(100ms)", "MEDIUM(500ms)", "NORMAL(1000ms)", "SLOW(2000ms)" };
#define NUM_SPEEDS  (sizeof(speeds_ms) / sizeof(speeds_ms[0]))

static TimerHandle_t xBlinkTimer = NULL;
static volatile uint32_t speed_index = 2;  /* Start at NORMAL */
static volatile bool led_state = false;
static volatile uint32_t toggle_count = 0;
static volatile uint32_t button_press_count = 0;
static SemaphoreHandle_t xButtonSemaphore = NULL;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void blink_timer_callback(TimerHandle_t xTimer)
{
    toggle_count++;
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state ? 1 : 0);

    ESP_LOGI(TAG, "EVENT,BLINK,led=%s,speed=%s,period=%lu_ms,toggle_count=%lu,tick=%lu",
             led_state ? "ON" : "OFF",
             speed_names[speed_index],
             (unsigned long)speeds_ms[speed_index],
             (unsigned long)toggle_count,
             (unsigned long)xTaskGetTickCount());
}

static void button_task(void *pvParameters)
{
    while (1) {
        if (xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE) {
            /* Simple debounce */
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BTN_GPIO) == 0) {
                button_press_count++;
                uint32_t old_index = speed_index;
                speed_index = (speed_index + 1) % NUM_SPEEDS;

                ESP_LOGW(TAG, "EVENT,BUTTON_PRESS,count=%lu,old_speed=%s,new_speed=%s",
                         (unsigned long)button_press_count,
                         speed_names[old_index],
                         speed_names[speed_index]);

                /* Change timer period - takes effect immediately */
                TickType_t old_tick = xTaskGetTickCount();
                if (xTimerChangePeriod(xBlinkTimer,
                                       pdMS_TO_TICKS(speeds_ms[speed_index]),
                                       pdMS_TO_TICKS(100)) == pdPASS) {
                    ESP_LOGI(TAG, "EVENT,PERIOD_CHANGED,new_period=%lu_ms,applied_at_tick=%lu,latency=%lu_ticks",
                             (unsigned long)speeds_ms[speed_index],
                             (unsigned long)xTaskGetTickCount(),
                             (unsigned long)(xTaskGetTickCount() - old_tick));
                } else {
                    ESP_LOGE(TAG, "ERROR,Failed to change timer period");
                }

                /* Wait for button release */
                while (gpio_get_level(BTN_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Timer Period Change Demo ===");
    ESP_LOGI(TAG, "INFO,LED GPIO: %d, Button GPIO: %d", LED_GPIO, BTN_GPIO);
    ESP_LOGI(TAG, "INFO,Press BOOT button to cycle blink speeds");
    ESP_LOGI(TAG, "INFO,Speeds: 100ms -> 500ms -> 1000ms -> 2000ms -> (repeat)");

    /* Configure LED */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_GPIO, 0);

    /* Configure button with interrupt */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    xButtonSemaphore = xSemaphoreCreateBinary();

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, button_isr_handler, NULL);

    /* Create blink timer */
    xBlinkTimer = xTimerCreate(
        "BlinkTimer",
        pdMS_TO_TICKS(speeds_ms[speed_index]),
        pdTRUE,
        NULL,
        blink_timer_callback
    );

    if (xBlinkTimer == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create blink timer!");
        return;
    }

    /* Start blink timer */
    xTimerStart(xBlinkTimer, pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "INFO,Blink timer started with period=%lu ms", (unsigned long)speeds_ms[speed_index]);

    /* Create button handler task */
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);

    /* Monitor task */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "STATUS,speed=%s,period=%lu_ms,toggles=%lu,presses=%lu,tick=%lu",
                 speed_names[speed_index],
                 (unsigned long)speeds_ms[speed_index],
                 (unsigned long)toggle_count,
                 (unsigned long)button_press_count,
                 (unsigned long)xTaskGetTickCount());
    }
}
