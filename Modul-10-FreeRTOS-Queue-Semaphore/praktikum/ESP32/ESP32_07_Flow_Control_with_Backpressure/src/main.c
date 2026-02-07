/**
 * ESP32_07: Flow Control with Backpressure
 * ==========================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Producer memeriksa ruang queue tersisa. Jika queue
 *         hampir penuh, kurangi rate produksi (backpressure).
 *         Counting semaphore untuk membatasi concurrent producers.
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

static const char *TAG = "BACKPRES";
#define QUEUE_SIZE 10
#define MAX_PRODUCERS 3

static QueueHandle_t xDataQueue = NULL;
static SemaphoreHandle_t xPrintMutex = NULL;
static SemaphoreHandle_t xProducerSlots = NULL; /* counting semaphore */

typedef struct {
    uint8_t  producer_id;
    uint32_t seq_num;
    uint32_t delay_ms;
} DataItem_t;

static void vProducerTask(void *pv)
{
    int id = (int)(intptr_t)pv;
    uint32_t seq = 0;
    uint32_t base_delay = 100;
    DataItem_t item;

    for (;;) {
        /* Ambil slot producer (counting semaphore) */
        if (xSemaphoreTake(xProducerSlots, pdMS_TO_TICKS(1000)) == pdTRUE) {
            seq++;
            /* Backpressure: cek ruang tersisa */
            UBaseType_t spaces = uxQueueSpacesAvailable(xDataQueue);
            uint32_t delay = base_delay;
            if (spaces < 3) delay = base_delay * 4; /* slow down */
            else if (spaces < 5) delay = base_delay * 2;

            item.producer_id = id;
            item.seq_num = seq;
            item.delay_ms = delay;

            if (xQueueSend(xDataQueue, &item, pdMS_TO_TICKS(500)) == pdPASS) {
                xSemaphoreTake(xPrintMutex, portMAX_DELAY);
                printf("[P%d] Sent #%lu | spaces=%lu | delay=%lu ms\n",
                       id, (unsigned long)seq, (unsigned long)spaces,
                       (unsigned long)delay);
                xSemaphoreGive(xPrintMutex);
            }
            xSemaphoreGive(xProducerSlots);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

static void vConsumerTask(void *pv)
{
    DataItem_t item;
    for (;;) {
        if (xQueueReceive(xDataQueue, &item, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Consumer] P%d seq#%lu (produced at %lu ms delay)\n",
                   item.producer_id, (unsigned long)item.seq_num,
                   (unsigned long)item.delay_ms);
            xSemaphoreGive(xPrintMutex);
            /* Simulasi processing lambat */
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_07: Flow Control with Backpressure\n");
    printf("  Counting semaphore + queue space monitoring\n");
    printf("=========================================================\n\n");

    xDataQueue     = xQueueCreate(QUEUE_SIZE, sizeof(DataItem_t));
    xPrintMutex    = xSemaphoreCreateMutex();
    xProducerSlots = xSemaphoreCreateCounting(MAX_PRODUCERS, MAX_PRODUCERS);

    for (int i = 0; i < 4; i++) /* 4 producers, but max 3 concurrent */
        xTaskCreate(vProducerTask, "Producer", 4096, (void*)(intptr_t)i, 2, NULL);
    xTaskCreate(vConsumerTask, "Consumer", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Backpressure demo started.");
}
