/*
 * ===========================================================================
 * MODUL 10 - Percobaan 02: Queue Struct
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Mengirim struct (sensor data) via queue — type-safe messaging.
 * Demonstrasi pengiriman data kompleks antar task.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"

#define QUEUE_LEN   10

static const char *TAG = "QUEUE_STRUCT";

typedef enum {
    SENSOR_TEMPERATURE,
    SENSOR_HUMIDITY,
    SENSOR_PRESSURE
} sensor_type_t;

typedef struct {
    sensor_type_t type;
    float value;
    uint32_t timestamp;
    uint8_t sensor_id;
} sensor_data_t;

static QueueHandle_t xSensorQueue;

static const char* sensor_name(sensor_type_t t) {
    switch(t) {
        case SENSOR_TEMPERATURE: return "TEMP";
        case SENSOR_HUMIDITY:    return "HUMI";
        case SENSOR_PRESSURE:    return "PRES";
        default: return "UNKN";
    }
}

static void sensor_task(void *pvParam)
{
    uint8_t id = (uint8_t)(uintptr_t)pvParam;
    sensor_data_t data;
    
    while (1) {
        data.sensor_id = id;
        data.timestamp = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        
        switch (id % 3) {
            case 0:
                data.type = SENSOR_TEMPERATURE;
                data.value = 20.0f + (float)(esp_random() % 150) / 10.0f;
                break;
            case 1:
                data.type = SENSOR_HUMIDITY;
                data.value = 40.0f + (float)(esp_random() % 400) / 10.0f;
                break;
            case 2:
                data.type = SENSOR_PRESSURE;
                data.value = 1000.0f + (float)(esp_random() % 500) / 10.0f;
                break;
        }
        
        if (xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[SENSOR_%d] Sent: %s = %.1f @%lums",
                     id, sensor_name(data.type), data.value, (unsigned long)data.timestamp);
        } else {
            ESP_LOGW(TAG, "[SENSOR_%d] Queue FULL!", id);
        }
        vTaskDelay(pdMS_TO_TICKS(300 + (id * 200)));
    }
}

static void processor_task(void *pvParam)
{
    sensor_data_t data;
    uint32_t count = 0;
    
    while (1) {
        if (xQueueReceive(xSensorQueue, &data, pdMS_TO_TICKS(2000)) == pdPASS) {
            count++;
            ESP_LOGI(TAG, "[PROC] #%lu Sensor_%d %s=%.1f t=%lums",
                     (unsigned long)count, data.sensor_id,
                     sensor_name(data.type), data.value, (unsigned long)data.timestamp);
        } else {
            ESP_LOGW(TAG, "[PROC] No data for 2s!");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Queue Struct - Sensor Data ===");
    ESP_LOGI(TAG, "Struct size: %d bytes", sizeof(sensor_data_t));
    
    xSensorQueue = xQueueCreate(QUEUE_LEN, sizeof(sensor_data_t));
    if (xSensorQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue!");
        return;
    }
    
    xTaskCreate(sensor_task, "Sensor0", 2048, (void*)0, 2, NULL);
    xTaskCreate(sensor_task, "Sensor1", 2048, (void*)1, 2, NULL);
    xTaskCreate(sensor_task, "Sensor2", 2048, (void*)2, 2, NULL);
    xTaskCreate(processor_task, "Processor", 2048, NULL, 3, NULL);
}
