/**
 * ESP32_08: Queue Peek Operations
 * =================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: xQueuePeek membaca tanpa menghapus item dari queue.
 *         Berguna untuk status sharing ke banyak reader.
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

static const char *TAG = "Q_PEEK";

typedef struct {
    float    cpu_temp;
    uint32_t uptime_sec;
    uint32_t free_heap;
    uint8_t  status_code;
} SystemStatus_t;

static QueueHandle_t xStatusQueue = NULL;      /* depth=1, overwrite */
static QueueHandle_t xEventQueue  = NULL;       /* normal queue */
static SemaphoreHandle_t xPrintMutex = NULL;

/* Updater: tulis status terbaru (overwrite) */
static void vStatusUpdater(void *pv)
{
    SystemStatus_t st;
    uint32_t uptime = 0;
    for (;;) {
        uptime++;
        st.cpu_temp    = 40.0f + (float)(esp_random() % 200) / 10.0f;
        st.uptime_sec  = uptime;
        st.free_heap   = (uint32_t)esp_get_free_heap_size();
        st.status_code = (uptime % 10 == 0) ? 2 : 1; /* warning tiap 10s */
        xQueueOverwrite(xStatusQueue, &st);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Display reader: peek status tanpa consume */
static void vDisplayReader(void *pv)
{
    int id = (int)(intptr_t)pv;
    SystemStatus_t st;
    for (;;) {
        if (xQueuePeek(xStatusQueue, &st, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Display%d] T=%.1fC up=%lus heap=%lu st=%d\n",
                   id, st.cpu_temp, (unsigned long)st.uptime_sec,
                   (unsigned long)st.free_heap, st.status_code);
            xSemaphoreGive(xPrintMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1500 + id * 500));
    }
}

/* Alert checker: peek dan kirim event jika warning */
static void vAlertChecker(void *pv)
{
    SystemStatus_t st;
    for (;;) {
        if (xQueuePeek(xStatusQueue, &st, portMAX_DELAY) == pdPASS) {
            if (st.status_code >= 2) {
                uint32_t evt = st.uptime_sec;
                xQueueSend(xEventQueue, &evt, 0);
                xSemaphoreTake(xPrintMutex, portMAX_DELAY);
                printf("[Alert] WARNING at uptime %lu s!\n", (unsigned long)evt);
                xSemaphoreGive(xPrintMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_08: Queue Peek Operations\n");
    printf("  Status sharing via xQueuePeek (non-destructive read)\n");
    printf("=========================================================\n\n");

    xStatusQueue = xQueueCreate(1, sizeof(SystemStatus_t));
    xEventQueue  = xQueueCreate(10, sizeof(uint32_t));
    xPrintMutex  = xSemaphoreCreateMutex();

    xTaskCreate(vStatusUpdater, "Updater",  4096, NULL, 3, NULL);
    xTaskCreate(vDisplayReader, "Disp1",    4096, (void*)1, 1, NULL);
    xTaskCreate(vDisplayReader, "Disp2",    4096, (void*)2, 1, NULL);
    xTaskCreate(vAlertChecker,  "Alert",    4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "Queue peek demo started.");
}
