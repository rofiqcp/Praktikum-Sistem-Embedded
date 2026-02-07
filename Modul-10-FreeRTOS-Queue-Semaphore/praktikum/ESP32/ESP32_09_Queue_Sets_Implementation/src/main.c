/**
 * ESP32_09: Queue Sets Implementation
 * =====================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Queue Set memungkinkan satu task menunggu data
 *         dari multiple queue/semaphore sekaligus (multiplexing).
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

static const char *TAG = "Q_SET";

static QueueHandle_t xTempQueue = NULL;
static QueueHandle_t xHumQueue  = NULL;
static SemaphoreHandle_t xAlertSem  = NULL;
static QueueSetHandle_t  xQueueSet  = NULL;
static SemaphoreHandle_t xPrintMutex = NULL;

static void vTempSensorTask(void *pv)
{
    float temp;
    for (;;) {
        temp = 20.0f + (float)(esp_random() % 200) / 10.0f;
        xQueueSend(xTempQueue, &temp, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vHumSensorTask(void *pv)
{
    float hum;
    for (;;) {
        hum = 30.0f + (float)(esp_random() % 500) / 10.0f;
        xQueueSend(xHumQueue, &hum, 0);
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

static void vAlertTask(void *pv)
{
    for (;;) {
        xSemaphoreGive(xAlertSem);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* Multiplexer: satu task menunggu data dari queue set */
static void vMultiplexerTask(void *pv)
{
    QueueSetMemberHandle_t xActivated;
    uint32_t temp_cnt = 0, hum_cnt = 0, alert_cnt = 0;

    for (;;) {
        xActivated = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        if (xActivated == (QueueSetMemberHandle_t)xTempQueue) {
            float temp;
            xQueueReceive(xTempQueue, &temp, 0);
            temp_cnt++;
            printf("[Mux] TEMP=%.1fC  (#%lu)\n", temp, (unsigned long)temp_cnt);
        }
        else if (xActivated == (QueueSetMemberHandle_t)xHumQueue) {
            float hum;
            xQueueReceive(xHumQueue, &hum, 0);
            hum_cnt++;
            printf("[Mux] HUM=%.1f%%  (#%lu)\n", hum, (unsigned long)hum_cnt);
        }
        else if (xActivated == (QueueSetMemberHandle_t)xAlertSem) {
            xSemaphoreTake(xAlertSem, 0);
            alert_cnt++;
            printf("[Mux] ALERT!  (#%lu)\n", (unsigned long)alert_cnt);
        }
        xSemaphoreGive(xPrintMutex);
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_09: Queue Sets Implementation\n");
    printf("  Satu task multiplexing dari 2 queue + 1 semaphore\n");
    printf("=========================================================\n\n");

    xTempQueue = xQueueCreate(5, sizeof(float));
    xHumQueue  = xQueueCreate(5, sizeof(float));
    xAlertSem  = xSemaphoreCreateBinary();
    xPrintMutex = xSemaphoreCreateMutex();

    /* Queue set harus cukup besar: 5 + 5 + 1 = 11 */
    xQueueSet = xQueueCreateSet(11);
    xQueueAddToSet(xTempQueue, xQueueSet);
    xQueueAddToSet(xHumQueue, xQueueSet);
    xQueueAddToSet(xAlertSem, xQueueSet);

    xTaskCreate(vTempSensorTask,  "TempSens", 4096, NULL, 2, NULL);
    xTaskCreate(vHumSensorTask,   "HumSens",  4096, NULL, 2, NULL);
    xTaskCreate(vAlertTask,       "Alert",    4096, NULL, 2, NULL);
    xTaskCreate(vMultiplexerTask, "Mux",      4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Queue set multiplexer started.");
}
