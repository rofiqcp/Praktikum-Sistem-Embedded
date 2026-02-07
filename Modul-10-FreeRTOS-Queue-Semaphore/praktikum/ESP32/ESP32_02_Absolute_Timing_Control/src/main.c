/**
 * ESP32_02: Absolute Timing Control (vTaskDelayUntil)
 * ====================================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 * Board: ESP32 DOIT DevKit V1
 *
 * Konsep: vTaskDelayUntil() menjaga timing absolut, berbeda
 *         dengan vTaskDelay() yang relatif. Data timing dikirim
 *         via Queue untuk analisis jitter.
 *
 * Hardware yang dibutuhkan:
 *   - ESP32 DevKit V1 (hanya board)
 *   - USB Serial Monitor (115200 baud)
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ABS_TIME";

typedef struct {
    int64_t  expected_us;
    int64_t  actual_us;
    int64_t  jitter_us;
    uint32_t iteration;
    bool     use_delay_until;
} TimingData_t;

static QueueHandle_t     xTimingQueue = NULL;
static SemaphoreHandle_t xPrintMutex  = NULL;

/* Task menggunakan vTaskDelayUntil (timing absolut) */
static void vAbsoluteTimingTask(void *pv)
{
    TickType_t xLastWake = xTaskGetTickCount();
    int64_t last_time = esp_timer_get_time();
    const uint32_t period_ms = 100;
    TimingData_t td;
    uint32_t iter = 0;

    for (;;) {
        int64_t now = esp_timer_get_time();
        int64_t expected = last_time + (period_ms * 1000);
        iter++;
        td.expected_us = expected;
        td.actual_us   = now;
        td.jitter_us   = now - expected;
        td.iteration   = iter;
        td.use_delay_until = true;
        last_time = now;
        xQueueSend(xTimingQueue, &td, 0);

        /* Simulasi carga variabel */
        for (volatile int i = 0; i < (iter % 500); i++);
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(period_ms));
    }
}

/* Task menggunakan vTaskDelay (timing relatif) */
static void vRelativeTimingTask(void *pv)
{
    int64_t last_time = esp_timer_get_time();
    const uint32_t period_ms = 100;
    TimingData_t td;
    uint32_t iter = 0;

    for (;;) {
        int64_t now = esp_timer_get_time();
        int64_t expected = last_time + (period_ms * 1000);
        iter++;
        td.expected_us = expected;
        td.actual_us   = now;
        td.jitter_us   = now - expected;
        td.iteration   = iter;
        td.use_delay_until = false;
        last_time = now;
        xQueueSend(xTimingQueue, &td, 0);

        for (volatile int i = 0; i < (iter % 500); i++);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}

/* Analisis dan tampilkan data timing */
static void vAnalysisTask(void *pv)
{
    TimingData_t td;
    int64_t abs_sum = 0, rel_sum = 0;
    int64_t abs_max = 0, rel_max = 0;
    uint32_t abs_cnt = 0, rel_cnt = 0;

    for (;;) {
        if (xQueueReceive(xTimingQueue, &td, portMAX_DELAY) == pdPASS) {
            int64_t j = td.jitter_us < 0 ? -td.jitter_us : td.jitter_us;
            if (td.use_delay_until) {
                abs_sum += j; abs_cnt++;
                if (j > abs_max) abs_max = j;
            } else {
                rel_sum += j; rel_cnt++;
                if (j > rel_max) rel_max = j;
            }
            if ((abs_cnt + rel_cnt) % 50 == 0) {
                xSemaphoreTake(xPrintMutex, portMAX_DELAY);
                printf("\n=== Timing Analysis (iter %lu) ===\n", (unsigned long)(abs_cnt+rel_cnt));
                if (abs_cnt > 0) printf("DelayUntil: avg_jitter=%lld us, max=%lld us\n", abs_sum/abs_cnt, abs_max);
                if (rel_cnt > 0) printf("Delay     : avg_jitter=%lld us, max=%lld us\n", rel_sum/rel_cnt, rel_max);
                printf("==================================\n");
                xSemaphoreGive(xPrintMutex);
            }
        }
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_02: Absolute Timing Control\n");
    printf("  Membandingkan jitter: vTaskDelayUntil vs vTaskDelay\n");
    printf("=========================================================\n\n");

    xTimingQueue = xQueueCreate(30, sizeof(TimingData_t));
    xPrintMutex  = xSemaphoreCreateMutex();
    if (!xTimingQueue || !xPrintMutex) { ESP_LOGE(TAG, "Alloc fail"); return; }

    xTaskCreate(vAbsoluteTimingTask, "AbsTime",  4096, NULL, 3, NULL);
    xTaskCreate(vRelativeTimingTask, "RelTime",  4096, NULL, 3, NULL);
    xTaskCreate(vAnalysisTask,       "Analysis", 4096, NULL, 2, NULL);
    ESP_LOGI(TAG, "Tasks created.");
}
