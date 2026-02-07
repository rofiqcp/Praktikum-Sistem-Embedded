/**
 * ESP32_11: Recursive Mutex Locking
 * ====================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Recursive mutex memungkinkan task yang sama melakukan
 *         lock berulang kali tanpa deadlock. Setiap take harus
 *         diimbangi give. Juga demo priority inheritance.
 *
 * Hardware: ESP32 DevKit V1 saja (serial via USB 115200)
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

static const char *TAG = "REC_MUTEX";

static SemaphoreHandle_t xRecMutex   = NULL;
static SemaphoreHandle_t xNormalMutex = NULL;
static SemaphoreHandle_t xPrintMutex = NULL;
static QueueHandle_t     xLogQueue   = NULL;

typedef struct {
    char msg[80];
} LogEntry_t;

static void safe_log(const char *fmt, ...)
{
    LogEntry_t entry;
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.msg, sizeof(entry.msg), fmt, args);
    va_end(args);
    xQueueSend(xLogQueue, &entry, pdMS_TO_TICKS(100));
}

/* Fungsi yang butuh mutex - bisa dipanggil nested */
static void innerFunction(void)
{
    /* Take recursive mutex (level 2) */
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    safe_log("  innerFunction: lock level 2");

    /* Do work */
    for (volatile int i = 0; i < 1000; i++);

    safe_log("  innerFunction: unlock level 2");
    xSemaphoreGiveRecursive(xRecMutex);
}

static void outerFunction(void)
{
    /* Take recursive mutex (level 1) */
    xSemaphoreTakeRecursive(xRecMutex, portMAX_DELAY);
    safe_log("outerFunction: lock level 1");

    /* Panggil inner yang juga lock - OK karena recursive! */
    innerFunction();

    safe_log("outerFunction: unlock level 1");
    xSemaphoreGiveRecursive(xRecMutex);
}

static void vRecursiveTask(void *pv)
{
    int id = (int)(intptr_t)pv;
    for (;;) {
        safe_log("[Task%d] Calling outerFunction...", id);
        outerFunction();
        safe_log("[Task%d] Done\n", id);
        vTaskDelay(pdMS_TO_TICKS(1000 + id * 500));
    }
}

/* Priority Inheritance Demo */
static void vLowPrioTask(void *pv)
{
    for (;;) {
        xSemaphoreTake(xNormalMutex, portMAX_DELAY);
        UBaseType_t prio = uxTaskPriorityGet(NULL);
        safe_log("[LowPrio] Got mutex (prio=%lu)", (unsigned long)prio);
        vTaskDelay(pdMS_TO_TICKS(500)); /* Hold mutex lama */
        prio = uxTaskPriorityGet(NULL);
        safe_log("[LowPrio] Releasing mutex (prio=%lu, may be inherited)", (unsigned long)prio);
        xSemaphoreGive(xNormalMutex);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void vHighPrioTask(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(200)); /* Let low-prio get mutex first */
    for (;;) {
        safe_log("[HighPrio] Waiting for mutex...");
        int64_t start = esp_timer_get_time();
        xSemaphoreTake(xNormalMutex, portMAX_DELAY);
        int64_t wait_ms = (esp_timer_get_time() - start) / 1000;
        safe_log("[HighPrio] Got mutex! (waited %lld ms)", wait_ms);
        xSemaphoreGive(xNormalMutex);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* Logger task: terima log dari queue */
static void vLoggerTask(void *pv)
{
    LogEntry_t entry;
    for (;;) {
        if (xQueueReceive(xLogQueue, &entry, portMAX_DELAY) == pdPASS) {
            printf("%s\n", entry.msg);
        }
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_11: Recursive Mutex Locking\n");
    printf("  Nested locking + Priority Inheritance demo\n");
    printf("=========================================================\n\n");

    xRecMutex    = xSemaphoreCreateRecursiveMutex();
    xNormalMutex = xSemaphoreCreateMutex();
    xPrintMutex  = xSemaphoreCreateMutex();
    xLogQueue    = xQueueCreate(30, sizeof(LogEntry_t));

    xTaskCreate(vLoggerTask,    "Logger",   4096, NULL, 5, NULL);
    xTaskCreate(vRecursiveTask, "RecTask1", 4096, (void*)1, 2, NULL);
    xTaskCreate(vRecursiveTask, "RecTask2", 4096, (void*)2, 2, NULL);
    xTaskCreate(vLowPrioTask,   "LowPrio",  4096, NULL, 1, NULL);
    xTaskCreate(vHighPrioTask,  "HighPrio", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Recursive mutex + priority inheritance demo started.");
}
