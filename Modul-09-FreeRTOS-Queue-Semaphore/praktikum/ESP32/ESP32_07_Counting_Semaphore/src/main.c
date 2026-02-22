/*
 * ===========================================================================
 * MODUL 10 - Percobaan 07: Counting Semaphore
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Resource pool management: 3 resources shared by 5 tasks.
 * Parking lot analogy — counting semaphore limits concurrent access.
 * Hardware: Serial Monitor + 3x LED (GPIO2, GPIO4, GPIO5)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#define NUM_RESOURCES   3
#define NUM_WORKERS     5

static const gpio_num_t led_pins[3] = {GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5};
static const char *TAG = "COUNT_SEM";
static SemaphoreHandle_t xResourceSem;

static void worker_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    
    while (1) {
        ESP_LOGI(TAG, "[W%d] Waiting for resource... (avail=%lu)",
                 id, (unsigned long)uxSemaphoreGetCount(xResourceSem));
        
        if (xSemaphoreTake(xResourceSem, portMAX_DELAY) == pdTRUE) {
            UBaseType_t avail = uxSemaphoreGetCount(xResourceSem);
            int res_idx = NUM_RESOURCES - 1 - (int)avail;
            if (res_idx < 0) res_idx = 0;
            if (res_idx >= NUM_RESOURCES) res_idx = NUM_RESOURCES - 1;
            
            gpio_set_level(led_pins[res_idx], 1);
            ESP_LOGI(TAG, "[W%d] ACQUIRED resource (LED%d ON, avail=%lu)",
                     id, res_idx, (unsigned long)avail);
            
            /* Use resource */
            vTaskDelay(pdMS_TO_TICKS(1000 + (esp_random() % 2000)));
            
            gpio_set_level(led_pins[res_idx], 0);
            xSemaphoreGive(xResourceSem);
            ESP_LOGI(TAG, "[W%d] RELEASED resource (LED%d OFF)", id, res_idx);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    for (int i = 0; i < NUM_RESOURCES; i++) {
        gpio_reset_pin(led_pins[i]);
        gpio_set_direction(led_pins[i], GPIO_MODE_OUTPUT);
    }
    
    ESP_LOGI(TAG, "=== MODUL 10: Counting Semaphore ===");
    ESP_LOGI(TAG, "%d resources, %d workers", NUM_RESOURCES, NUM_WORKERS);
    
    xResourceSem = xSemaphoreCreateCounting(NUM_RESOURCES, NUM_RESOURCES);
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Worker%d", i);
        xTaskCreate(worker_task, name, 2048, (void*)(uintptr_t)i, 2, NULL);
    }
}
