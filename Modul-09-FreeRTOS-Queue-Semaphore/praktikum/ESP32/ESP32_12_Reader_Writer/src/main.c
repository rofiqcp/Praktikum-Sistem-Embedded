/*
 * ===========================================================================
 * MODUL 10 - Percobaan 12: Reader-Writer Lock
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Multiple readers can read concurrently, but writer needs exclusive access.
 * Implemented with a mutex + counting semaphore.
 * Hardware: Serial Monitor
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_random.h"

#define NUM_READERS 4
#define NUM_WRITERS 2

static const char *TAG = "RW_LOCK";

static SemaphoreHandle_t xWriteMutex;    /* Exclusive write access */
static SemaphoreHandle_t xReaderMutex;   /* Protect reader_count */
static int reader_count = 0;

/* Shared data */
static int shared_data = 0;
static int write_count = 0;

static void reader_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    int local_data;
    
    while (1) {
        /* Reader entry */
        xSemaphoreTake(xReaderMutex, portMAX_DELAY);
        reader_count++;
        if (reader_count == 1) {
            xSemaphoreTake(xWriteMutex, portMAX_DELAY); /* First reader blocks writers */
        }
        xSemaphoreGive(xReaderMutex);
        
        /* Critical section — READ */
        local_data = shared_data;
        ESP_LOGI(TAG, "[R%d] Read: %d (readers=%d)", id, local_data, reader_count);
        vTaskDelay(pdMS_TO_TICKS(200)); /* Simulate reading time */
        
        /* Reader exit */
        xSemaphoreTake(xReaderMutex, portMAX_DELAY);
        reader_count--;
        if (reader_count == 0) {
            xSemaphoreGive(xWriteMutex); /* Last reader unblocks writers */
        }
        xSemaphoreGive(xReaderMutex);
        
        vTaskDelay(pdMS_TO_TICKS(500 + (esp_random() % 1000)));
    }
}

static void writer_task(void *pvParam)
{
    int id = (int)(uintptr_t)pvParam;
    
    while (1) {
        xSemaphoreTake(xWriteMutex, portMAX_DELAY);
        
        /* Critical section — WRITE */
        write_count++;
        shared_data = write_count * 100 + id;
        ESP_LOGW(TAG, "[W%d] WRITE: %d (exclusive access)", id, shared_data);
        vTaskDelay(pdMS_TO_TICKS(500)); /* Simulate write time */
        
        xSemaphoreGive(xWriteMutex);
        
        vTaskDelay(pdMS_TO_TICKS(2000 + (esp_random() % 2000)));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== MODUL 10: Reader-Writer Lock ===");
    ESP_LOGI(TAG, "%d readers, %d writers", NUM_READERS, NUM_WRITERS);
    
    xWriteMutex  = xSemaphoreCreateMutex();
    xReaderMutex = xSemaphoreCreateMutex();
    
    for (int i = 0; i < NUM_READERS; i++) {
        char n[16]; snprintf(n, sizeof(n), "Reader%d", i);
        xTaskCreate(reader_task, n, 2048, (void*)(uintptr_t)i, 2, NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        char n[16]; snprintf(n, sizeof(n), "Writer%d", i);
        xTaskCreate(writer_task, n, 2048, (void*)(uintptr_t)i, 3, NULL);
    }
}
