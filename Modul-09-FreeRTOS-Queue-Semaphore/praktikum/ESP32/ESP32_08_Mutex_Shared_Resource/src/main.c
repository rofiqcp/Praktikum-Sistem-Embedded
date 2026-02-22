/*
 * ===========================================================================
 * MODUL 10 - Percobaan 08: Mutex Shared Resource
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Melindungi shared variable dengan mutex.
 * Demonstrasi: TANPA mutex (race condition) vs DENGAN mutex (aman).
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "MUTEX";
static SemaphoreHandle_t xMutex;

/* Shared resource */
static volatile int shared_counter = 0;
static volatile int unsafe_counter = 0;

static void safe_increment_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    for (int i = 0; i < 1000; i++) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            int temp = shared_counter;
            temp++;
            /* Small delay to increase chance of race condition */
            for (volatile int j = 0; j < 10; j++) {}
            shared_counter = temp;
            xSemaphoreGive(xMutex);
        }
    }
    ESP_LOGI(TAG, "[SAFE_%d] Finished 1000 increments. Counter=%d", id, shared_counter);
    vTaskDelete(NULL);
}

static void unsafe_increment_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    for (int i = 0; i < 1000; i++) {
        int temp = unsafe_counter;
        temp++;
        for (volatile int j = 0; j < 10; j++) {}
        unsafe_counter = temp;
    }
    ESP_LOGI(TAG, "[UNSAFE_%d] Finished 1000 increments. Counter=%d", id, unsafe_counter);
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Mutex — Shared Resource Protection ===");
    
    xMutex = xSemaphoreCreateMutex();
    
    /* Phase 1: UNSAFE — no mutex */
    ESP_LOGW(TAG, "--- Phase 1: WITHOUT MUTEX (expect race condition) ---");
    unsafe_counter = 0;
    
    TaskHandle_t h1, h2;
    xTaskCreate(unsafe_increment_task, "Unsafe1", 2048, (void*)1, 2, &h1);
    xTaskCreate(unsafe_increment_task, "Unsafe2", 2048, (void*)2, 2, &h2);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGW(TAG, "UNSAFE result: %d (expected 2000)", unsafe_counter);
    
    /* Phase 2: SAFE — with mutex */
    ESP_LOGI(TAG, "--- Phase 2: WITH MUTEX (correct result) ---");
    shared_counter = 0;
    
    xTaskCreate(safe_increment_task, "Safe1", 2048, (void*)1, 2, NULL);
    xTaskCreate(safe_increment_task, "Safe2", 2048, (void*)2, 2, NULL);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "SAFE result: %d (expected 2000)", shared_counter);
    
    ESP_LOGI(TAG, "=== COMPARISON: unsafe=%d vs safe=%d ===",
             unsafe_counter, shared_counter);
}
