/**
 * ESP32_06: Mailbox Pattern (Queue depth=1 + xQueueOverwrite)
 * =============================================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Mailbox = Queue berukuran 1. Writer selalu overwrite
 *         data terbaru. Reader bisa peek tanpa menghapus.
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
#include "esp_random.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MAILBOX";

typedef struct {
    float    temperature;
    float    humidity;
    uint32_t update_count;
    int64_t  timestamp_us;
} MailboxData_t;

static QueueHandle_t xMailbox = NULL;
static SemaphoreHandle_t xPrintMutex = NULL;

/* Writer task - selalu overwrite dengan data terbaru */
static void vWriterTask(void *pv)
{
    MailboxData_t data;
    uint32_t cnt = 0;
    for (;;) {
        cnt++;
        data.temperature = 20.0f + (float)(esp_random() % 200) / 10.0f;
        data.humidity    = 40.0f + (float)(esp_random() % 400) / 10.0f;
        data.update_count = cnt;
        data.timestamp_us = esp_timer_get_time();

        /* xQueueOverwrite: selalu berhasil, overwrite data lama */
        xQueueOverwrite(xMailbox, &data);

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[Writer] Update #%lu: T=%.1fC H=%.1f%%\n",
               (unsigned long)cnt, data.temperature, data.humidity);
        xSemaphoreGive(xPrintMutex);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* Reader task - peek data tanpa menghapus dari mailbox */
static void vReaderTask(void *pv)
{
    int id = (int)(intptr_t)pv;
    MailboxData_t data;
    for (;;) {
        /* xQueuePeek: baca tanpa hapus, beberapa reader bisa baca data sama */
        if (xQueuePeek(xMailbox, &data, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Reader%d] Read update #%lu: T=%.1fC H=%.1f%%\n",
                   id, (unsigned long)data.update_count,
                   data.temperature, data.humidity);
            xSemaphoreGive(xPrintMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500 + id * 300));
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_06: Mailbox Pattern\n");
    printf("  Queue(1) + xQueueOverwrite + xQueuePeek\n");
    printf("=========================================================\n\n");

    xMailbox    = xQueueCreate(1, sizeof(MailboxData_t));
    xPrintMutex = xSemaphoreCreateMutex();

    xTaskCreate(vWriterTask, "Writer",  4096, NULL, 2, NULL);
    xTaskCreate(vReaderTask, "Reader1", 4096, (void*)1, 1, NULL);
    xTaskCreate(vReaderTask, "Reader2", 4096, (void*)2, 1, NULL);

    ESP_LOGI(TAG, "Mailbox pattern demo started.");
}
