/*
 * ===========================================================================
 * MODUL 10 - Percobaan 10: Recursive Mutex
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Recursive mutex: same task can take the mutex multiple times.
 * Useful for nested function calls that each need the lock.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "REC_MUTEX";
static SemaphoreHandle_t xRecMutex;
static int shared_resource = 0;

/* Nested function that also needs the lock */
static void update_resource_inner(int delta)
{
    ESP_LOGI(TAG, "  [inner] Taking recursive mutex (2nd time)...");
    if (xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY) == pdTRUE) {
        shared_resource += delta;
        ESP_LOGI(TAG, "  [inner] Resource = %d (added %d)", shared_resource, delta);
        xSemaphoreGiveRecursive(xRecMutex);
        ESP_LOGI(TAG, "  [inner] Released (still held by outer)");
    }
}

/* Outer function that calls inner */
static void update_resource_outer(int value)
{
    ESP_LOGI(TAG, "[outer] Taking recursive mutex (1st time)...");
    if (xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "[outer] Got mutex. Current resource = %d", shared_resource);
        
        /* Call nested function — it will take mutex again */
        update_resource_inner(value);
        update_resource_inner(value * 2); /* Another nested call */
        
        ESP_LOGI(TAG, "[outer] After nested calls, resource = %d", shared_resource);
        xSemaphoreGiveRecursive(xRecMutex);
        ESP_LOGI(TAG, "[outer] Released mutex completely");
    }
}

static void task_func(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    int iter = 0;
    while (1) {
        iter++;
        ESP_LOGI(TAG, "\n=== Task %d, Iteration %d ===", id, iter);
        update_resource_outer(id * 10);
        vTaskDelay(pdMS_TO_TICKS(2000 + (id * 500)));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Recursive Mutex ===");
    ESP_LOGI(TAG, "Same task can lock mutex multiple times in nested calls");
    
    xRecMutex = xSemaphoreCreateRecursiveMutex();
    
    xTaskCreate(task_func, "Task1", 2048, (void*)1, 2, NULL);
    xTaskCreate(task_func, "Task2", 2048, (void*)2, 2, NULL);
}
