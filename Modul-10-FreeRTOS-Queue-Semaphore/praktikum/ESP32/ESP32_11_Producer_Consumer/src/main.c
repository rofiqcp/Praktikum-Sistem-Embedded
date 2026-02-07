/*
 * ===========================================================================
 * MODUL 10 - Percobaan 11: Producer Consumer
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Classic producer-consumer with bounded buffer (queue).
 * Multiple producers, single consumer, rate monitoring.
 * Hardware: Serial Monitor + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#define LED_PIN         GPIO_NUM_2
#define BUFFER_SIZE     8
#define NUM_PRODUCERS   3

static const char *TAG = "PROD_CONS";
static QueueHandle_t xBuffer;

typedef struct {
    uint8_t producer_id;
    uint32_t seq;
    float data;
} item_t;

static void producer_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    item_t item;
    uint32_t seq = 0;
    
    while (1) {
        item.producer_id = id;
        item.seq = seq++;
        item.data = (float)(esp_random() % 10000) / 100.0f;
        
        ESP_LOGI(TAG, "[P%d] Producing item #%lu (%.2f) queue=%lu/%d",
                 id, (unsigned long)item.seq, item.data,
                 (unsigned long)(BUFFER_SIZE - uxQueueSpacesAvailable(xBuffer)),
                 BUFFER_SIZE);
        
        if (xQueueSend(xBuffer, &item, pdMS_TO_TICKS(2000)) == pdPASS) {
            ESP_LOGI(TAG, "[P%d] Item enqueued OK", id);
        } else {
            ESP_LOGW(TAG, "[P%d] BUFFER FULL! Item dropped!", id);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500 + (esp_random() % 1000)));
    }
}

static void consumer_task(void *pvParam)
{
    item_t item;
    uint32_t total = 0;
    
    while (1) {
        if (xQueueReceive(xBuffer, &item, portMAX_DELAY) == pdPASS) {
            total++;
            gpio_set_level(LED_PIN, total % 2);
            
            ESP_LOGI(TAG, "[CONSUMER] Item from P%d seq#%lu data=%.2f (total=%lu, pending=%lu)",
                     item.producer_id, (unsigned long)item.seq, item.data,
                     (unsigned long)total,
                     (unsigned long)(BUFFER_SIZE - uxQueueSpacesAvailable(xBuffer)));
            
            /* Simulate processing time */
            vTaskDelay(pdMS_TO_TICKS(200 + (esp_random() % 300)));
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== MODUL 10: Producer-Consumer Pattern ===");
    ESP_LOGI(TAG, "%d producers, 1 consumer, buffer=%d", NUM_PRODUCERS, BUFFER_SIZE);
    
    xBuffer = xQueueCreate(BUFFER_SIZE, sizeof(item_t));
    
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Producer%d", i);
        xTaskCreate(producer_task, name, 2048, (void*)(uintptr_t)i, 2, NULL);
    }
    xTaskCreate(consumer_task, "Consumer", 2048, NULL, 3, NULL);
}
