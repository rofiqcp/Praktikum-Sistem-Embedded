/**
 * ESP32_10: Gatekeeper Task Pattern
 * ====================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Gatekeeper task = satu-satunya task yang akses resource
 *         (UART). Task lain mengirim request via Queue. Menghindari
 *         kebutuhan mutex untuk shared resource.
 *
 * Hardware: ESP32 DevKit V1 saja (serial via USB 115200)
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "GATEKEEPER";

typedef struct {
    char    message[64];
    uint8_t priority; /* 0=normal, 1=important, 2=critical */
} PrintRequest_t;

static QueueHandle_t xPrintQueue = NULL;

/* Gatekeeper: satu-satunya task yang print ke serial */
static void vGatekeeperTask(void *pv)
{
    PrintRequest_t req;
    const char *prio_labels[] = {"NORMAL", "IMPORTANT", "CRITICAL"};

    for (;;) {
        if (xQueueReceive(xPrintQueue, &req, portMAX_DELAY) == pdPASS) {
            /* Tidak perlu mutex! Hanya gatekeeper yang akses UART */
            printf("[%s] %s\n", prio_labels[req.priority], req.message);
        }
    }
}

/* Worker tasks: kirim print request via queue */
static void vWorkerTask(void *pv)
{
    int id = (int)(intptr_t)pv;
    PrintRequest_t req;
    uint32_t cnt = 0;

    for (;;) {
        cnt++;
        req.priority = (cnt % 5 == 0) ? 2 : (cnt % 3 == 0) ? 1 : 0;
        snprintf(req.message, sizeof(req.message),
                 "Worker%d iteration %lu (heap=%lu)",
                 id, (unsigned long)cnt,
                 (unsigned long)esp_get_free_heap_size());

        /* Kirim ke front jika critical, back jika normal */
        if (req.priority == 2)
            xQueueSendToFront(xPrintQueue, &req, pdMS_TO_TICKS(100));
        else
            xQueueSend(xPrintQueue, &req, pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(300 + id * 100));
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_10: Gatekeeper Task Pattern\n");
    printf("  Satu task = satu-satunya yang akses UART\n");
    printf("  Tasks lain kirim request via Queue\n");
    printf("=========================================================\n\n");

    xPrintQueue = xQueueCreate(20, sizeof(PrintRequest_t));

    /* Gatekeeper punya prioritas tinggi agar segera print */
    xTaskCreate(vGatekeeperTask, "Gatekeeper", 4096, NULL, 4, NULL);
    for (int i = 0; i < 4; i++)
        xTaskCreate(vWorkerTask, "Worker", 4096, (void*)(intptr_t)i, 2, NULL);

    ESP_LOGI(TAG, "Gatekeeper pattern started.");
}
