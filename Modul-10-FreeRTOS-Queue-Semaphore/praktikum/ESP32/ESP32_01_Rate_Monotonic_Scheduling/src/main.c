/**
 * ESP32_01: Rate Monotonic Scheduling (RMS)
 * ===========================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 * Board: ESP32 DOIT DevKit V1
 *
 * Konsep: RMS memberikan prioritas lebih tinggi ke task
 *         dengan periode eksekusi lebih pendek.
 *
 * Hardware yang dibutuhkan:
 *   - ESP32 DevKit V1 (hanya board, tanpa komponen tambahan)
 *   - USB untuk Serial Monitor (115200 baud)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "RMS";

typedef struct {
    char     task_name[16];
    uint32_t count;
    int64_t  timestamp_us;
    uint32_t period_ms;
} TaskLog_t;

static QueueHandle_t      xLogQueue   = NULL;
static SemaphoreHandle_t  xPrintMutex = NULL;

/* Task periode 10ms -> Prioritas 5 (tertinggi) */
static void vFastTask(void *pv)
{
    uint32_t cnt = 0;
    TaskLog_t lg;
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        cnt++;
        for (volatile int i = 0; i < 100; i++); /* simulasi kerja */
        if (cnt % 100 == 0) {
            strncpy(lg.task_name, "FastTask", 16);
            lg.count = cnt; lg.timestamp_us = esp_timer_get_time(); lg.period_ms = 10;
            xQueueSend(xLogQueue, &lg, 0);
        }
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(10));
    }
}

/* Task periode 50ms -> Prioritas 3 */
static void vMediumTask(void *pv)
{
    uint32_t cnt = 0;
    TaskLog_t lg;
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        cnt++;
        for (volatile int i = 0; i < 500; i++);
        if (cnt % 20 == 0) {
            strncpy(lg.task_name, "MediumTask", 16);
            lg.count = cnt; lg.timestamp_us = esp_timer_get_time(); lg.period_ms = 50;
            xQueueSend(xLogQueue, &lg, 0);
        }
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(50));
    }
}

/* Task periode 200ms -> Prioritas 1 (terendah) */
static void vSlowTask(void *pv)
{
    uint32_t cnt = 0;
    TaskLog_t lg;
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        cnt++;
        for (volatile int i = 0; i < 2000; i++);
        strncpy(lg.task_name, "SlowTask", 16);
        lg.count = cnt; lg.timestamp_us = esp_timer_get_time(); lg.period_ms = 200;
        xQueueSend(xLogQueue, &lg, 0);
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(200));
    }
}

/* Monitor: terima log dari queue dan print */
static void vMonitorTask(void *pv)
{
    TaskLog_t lg;
    for (;;) {
        if (xQueueReceive(xLogQueue, &lg, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[%10lld us] %-12s | cnt=%5lu | T=%3lu ms\n",
                   lg.timestamp_us, lg.task_name,
                   (unsigned long)lg.count, (unsigned long)lg.period_ms);
            xSemaphoreGive(xPrintMutex);
        }
    }
}

static void vStatusTask(void *pv)
{
    for (;;) {
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("\n=== RMS Status ===\n");
        printf("Heap free: %lu bytes\n", (unsigned long)esp_get_free_heap_size());
        printf("Queue    : %lu/20 items\n", (unsigned long)uxQueueMessagesWaiting(xLogQueue));
        printf("==================\n\n");
        xSemaphoreGive(xPrintMutex);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_01: Rate Monotonic Scheduling\n");
    printf("  Modul 10 - FreeRTOS Queue & Semaphore\n");
    printf("  RMS: periode kecil = prioritas tinggi\n");
    printf("=========================================================\n\n");

    xLogQueue   = xQueueCreate(20, sizeof(TaskLog_t));
    xPrintMutex = xSemaphoreCreateMutex();
    if (!xLogQueue || !xPrintMutex) { ESP_LOGE(TAG, "Alloc fail!"); return; }

    xTaskCreate(vFastTask,    "Fast",    4096, NULL, 5, NULL);
    xTaskCreate(vMediumTask,  "Medium",  4096, NULL, 3, NULL);
    xTaskCreate(vSlowTask,    "Slow",    4096, NULL, 1, NULL);
    xTaskCreate(vMonitorTask, "Monitor", 4096, NULL, 2, NULL);
    xTaskCreate(vStatusTask,  "Status",  4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "All tasks created.");
}
