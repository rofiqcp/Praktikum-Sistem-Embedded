/*
 * ===========================================================================
 * MODUL 10 - Percobaan 04: Queue ISR
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * xQueueSendFromISR() — button interrupt sends event via queue to task.
 * Hardware: 1x Push button (GPIO0) + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN_PIN     GPIO_NUM_0
#define LED_PIN     GPIO_NUM_2
#define QUEUE_LEN   10

static const char *TAG = "QUEUE_ISR";
static QueueHandle_t xButtonQueue;

typedef struct {
    uint32_t tick;
    int level;
} button_event_t;

static void IRAM_ATTR button_isr(void *arg)
{
    button_event_t evt;
    evt.tick = xTaskGetTickCountFromISR();
    evt.level = gpio_get_level(BTN_PIN);
    
    BaseType_t xHigherPrioWoken = pdFALSE;
    xQueueSendFromISR(xButtonQueue, &evt, &xHigherPrioWoken);
    if (xHigherPrioWoken) {
        portYIELD_FROM_ISR();
    }
}

static void led_handler_task(void *pvParam)
{
    button_event_t evt;
    int led_state = 0;
    uint32_t press_count = 0;
    
    while (1) {
        if (xQueueReceive(xButtonQueue, &evt, portMAX_DELAY) == pdPASS) {
            if (evt.level == 0) {  /* Button pressed (active low) */
                press_count++;
                led_state = !led_state;
                gpio_set_level(LED_PIN, led_state);
                ESP_LOGI(TAG, "[ISR→TASK] Button #%lu @tick=%lu LED=%s",
                         (unsigned long)press_count, (unsigned long)evt.tick,
                         led_state ? "ON" : "OFF");
            }
        }
    }
}

void app_main(void)
{
    /* LED setup */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    /* Button setup with interrupt */
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_ANYEDGE);
    
    ESP_LOGI(TAG, "=== MODUL 10: Queue from ISR ===");
    
    xButtonQueue = xQueueCreate(QUEUE_LEN, sizeof(button_event_t));
    
    xTaskCreate(led_handler_task, "LEDHandler", 2048, NULL, 3, NULL);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, button_isr, NULL);
    
    ESP_LOGI(TAG, "Press BOOT button (GPIO0) to toggle LED");
}
