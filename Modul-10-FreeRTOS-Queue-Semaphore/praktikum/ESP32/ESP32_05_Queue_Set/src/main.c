/*
 * ===========================================================================
 * MODUL 10 - Percobaan 05: Queue Set
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * xQueueCreateSet() — wait on multiple queues simultaneously.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "QUEUE_SET";

static QueueHandle_t xTempQueue;
static QueueHandle_t xHumiQueue;
static QueueSetHandle_t xQueueSet;

static void temp_sender(void *pvParam)
{
    float temp;
    while (1) {
        temp = 20.0f + (float)(esp_random() % 200) / 10.0f;
        xQueueSend(xTempQueue, &temp, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "[TEMP_TX] Sent: %.1f°C", temp);
        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

static void humi_sender(void *pvParam)
{
    float humi;
    while (1) {
        humi = 30.0f + (float)(esp_random() % 500) / 10.0f;
        xQueueSend(xHumiQueue, &humi, pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "[HUMI_TX] Sent: %.1f%%", humi);
        vTaskDelay(pdMS_TO_TICKS(1100));
    }
}

static void mux_receiver(void *pvParam)
{
    QueueSetMemberHandle_t xActiveMember;
    float value;
    
    while (1) {
        xActiveMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
        
        if (xActiveMember == xTempQueue) {
            xQueueReceive(xTempQueue, &value, 0);
            ESP_LOGI(TAG, "[MUX] Temperature: %.1f°C", value);
        } else if (xActiveMember == xHumiQueue) {
            xQueueReceive(xHumiQueue, &value, 0);
            ESP_LOGI(TAG, "[MUX] Humidity: %.1f%%", value);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Queue Set (Select/Multiplex) ===");
    
    xTempQueue = xQueueCreate(5, sizeof(float));
    xHumiQueue = xQueueCreate(5, sizeof(float));
    
    /* Queue set must be large enough: sum of all queue lengths */
    xQueueSet = xQueueCreateSet(5 + 5);
    xQueueAddToSet(xTempQueue, xQueueSet);
    xQueueAddToSet(xHumiQueue, xQueueSet);
    
    xTaskCreate(temp_sender, "TempTX", 2048, NULL, 2, NULL);
    xTaskCreate(humi_sender, "HumiTX", 2048, NULL, 2, NULL);
    xTaskCreate(mux_receiver, "MuxRX", 2048, NULL, 3, NULL);
}
