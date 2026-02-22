/*
 * ESP32_04_Timer_Debounce
 * ========================
 * Software debounce using one-shot timer.
 * Button ISR resets the timer on each edge. Timer callback
 * fires only after button is stable for 50ms.
 * Much cleaner than delay-based debounce.
 *
 * Hardware: 1x Button on GPIO0 (BOOT), 1x LED on GPIO2
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "TIMER_DEBOUNCE";

#define LED_GPIO        GPIO_NUM_2
#define BTN_GPIO        GPIO_NUM_0
#define DEBOUNCE_MS     50

static TimerHandle_t xDebounceTimer = NULL;
static volatile uint32_t isr_trigger_count = 0;
static volatile uint32_t debounced_press_count = 0;
static volatile uint32_t debounced_release_count = 0;
static volatile uint32_t raw_edge_count = 0;
static bool led_state = false;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    raw_edge_count++;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /*
     * Reset (restart) the debounce timer on EVERY edge.
     * The timer callback only fires when no more edges occur
     * for DEBOUNCE_MS milliseconds. This is the key to debouncing.
     */
    xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
    isr_trigger_count++;

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void debounce_timer_callback(TimerHandle_t xTimer)
{
    /* Timer expired = button has been stable for DEBOUNCE_MS */
    int btn_level = gpio_get_level(BTN_GPIO);

    if (btn_level == 0) {
        /* Button pressed (active low) */
        debounced_press_count++;
        led_state = !led_state;
        gpio_set_level(LED_GPIO, led_state ? 1 : 0);

        ESP_LOGI(TAG, "EVENT,DEBOUNCED_PRESS,count=%lu,led=%s,raw_edges=%lu,isr_triggers=%lu,tick=%lu",
                 (unsigned long)debounced_press_count,
                 led_state ? "ON" : "OFF",
                 (unsigned long)raw_edge_count,
                 (unsigned long)isr_trigger_count,
                 (unsigned long)xTaskGetTickCount());

        /* Show how many raw edges were filtered */
        uint32_t filtered = isr_trigger_count - debounced_press_count - debounced_release_count;
        ESP_LOGI(TAG, "FILTER,total_edges=%lu,valid_presses=%lu,valid_releases=%lu,filtered_bounces=%lu",
                 (unsigned long)raw_edge_count,
                 (unsigned long)debounced_press_count,
                 (unsigned long)debounced_release_count,
                 (unsigned long)filtered);
    } else {
        /* Button released */
        debounced_release_count++;
        ESP_LOGI(TAG, "EVENT,DEBOUNCED_RELEASE,count=%lu,tick=%lu",
                 (unsigned long)debounced_release_count,
                 (unsigned long)xTaskGetTickCount());
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Timer Debounce Demo ===");
    ESP_LOGI(TAG, "INFO,Button GPIO: %d (BOOT button)", BTN_GPIO);
    ESP_LOGI(TAG, "INFO,LED GPIO: %d", LED_GPIO);
    ESP_LOGI(TAG, "INFO,Debounce time: %d ms", DEBOUNCE_MS);
    ESP_LOGI(TAG, "INFO,Press BOOT button - LED toggles with clean debounce");
    ESP_LOGI(TAG, "INFO,Watch how many raw ISR edges get filtered vs clean presses");

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

    /* Configure button - interrupt on BOTH edges to catch bouncing */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&btn_conf);

    /* Create one-shot debounce timer */
    xDebounceTimer = xTimerCreate(
        "DebounceTimer",
        pdMS_TO_TICKS(DEBOUNCE_MS),
        pdFALSE,        /* One-shot: fires once after stable period */
        NULL,
        debounce_timer_callback
    );

    if (xDebounceTimer == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create debounce timer!");
        return;
    }

    ESP_LOGI(TAG, "INFO,Debounce timer created (one-shot, %d ms)", DEBOUNCE_MS);

    /* Install ISR */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, button_isr_handler, NULL);

    ESP_LOGI(TAG, "INFO,ISR installed. System ready. Press the BOOT button!");

    /* Monitor task */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        float efficiency = 0;
        if (raw_edge_count > 0) {
            efficiency = ((float)(debounced_press_count + debounced_release_count) /
                         (float)raw_edge_count) * 100.0f;
        }
        ESP_LOGI(TAG, "STATUS,presses=%lu,releases=%lu,raw_edges=%lu,isr_triggers=%lu,filter_efficiency=%.1f%%,tick=%lu",
                 (unsigned long)debounced_press_count,
                 (unsigned long)debounced_release_count,
                 (unsigned long)raw_edge_count,
                 (unsigned long)isr_trigger_count,
                 efficiency,
                 (unsigned long)xTaskGetTickCount());
    }
}
