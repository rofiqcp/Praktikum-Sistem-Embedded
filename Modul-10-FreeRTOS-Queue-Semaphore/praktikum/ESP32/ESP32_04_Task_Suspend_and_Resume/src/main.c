/**
 * ESP32_04: Task Suspend and Resume with Queue
 * ==============================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Suspend/resume task berdasarkan command yang
 *         dikirim via Queue. Binary semaphore untuk sync.
 *
 * Hardware: ESP32 DevKit V1 + USB Serial (115200)
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
#include "driver/gpio.h"

static const char *TAG = "SUSP_RES";

typedef enum { CMD_SUSPEND, CMD_RESUME, CMD_STATUS } CmdType_t;
typedef struct { CmdType_t cmd; uint8_t target_task; } Command_t;

static QueueHandle_t xCmdQueue = NULL;
static SemaphoreHandle_t xPrintMutex = NULL;
static TaskHandle_t xWorkerHandles[3] = {NULL};
static const char *worker_names[] = {"WorkerA", "WorkerB", "WorkerC"};

static void vWorkerTask(void *pv)
{
    int id = (int)(intptr_t)pv;
    uint32_t cnt = 0;
    for (;;) {
        cnt++;
        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[%s] running #%lu\n", worker_names[id], (unsigned long)cnt);
        xSemaphoreGive(xPrintMutex);
        vTaskDelay(pdMS_TO_TICKS(500 + id * 200));
    }
}

static void vControlTask(void *pv)
{
    /* Auto-generate commands to demo suspend/resume */
    Command_t cmds[] = {
        {CMD_STATUS, 0}, {CMD_SUSPEND, 0}, {CMD_STATUS, 0},
        {CMD_SUSPEND, 1}, {CMD_STATUS, 0}, {CMD_RESUME, 0},
        {CMD_STATUS, 0}, {CMD_RESUME, 1}, {CMD_STATUS, 0}
    };
    int idx = 0;
    for (;;) {
        Command_t c = cmds[idx % (sizeof(cmds)/sizeof(cmds[0]))];
        xQueueSend(xCmdQueue, &c, portMAX_DELAY);
        idx++;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void vCommandHandler(void *pv)
{
    Command_t c;
    for (;;) {
        if (xQueueReceive(xCmdQueue, &c, portMAX_DELAY) == pdPASS) {
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            switch (c.cmd) {
                case CMD_SUSPEND:
                    if (c.target_task < 3 && xWorkerHandles[c.target_task]) {
                        vTaskSuspend(xWorkerHandles[c.target_task]);
                        printf(">>> SUSPENDED %s\n", worker_names[c.target_task]);
                    }
                    break;
                case CMD_RESUME:
                    if (c.target_task < 3 && xWorkerHandles[c.target_task]) {
                        vTaskResume(xWorkerHandles[c.target_task]);
                        printf(">>> RESUMED %s\n", worker_names[c.target_task]);
                    }
                    break;
                case CMD_STATUS:
                    printf("\n--- Task Status ---\n");
                    for (int i = 0; i < 3; i++) {
                        eTaskState st = eTaskGetState(xWorkerHandles[i]);
                        printf("  %s: %s\n", worker_names[i],
                               st == eSuspended ? "SUSPENDED" : "RUNNING/READY");
                    }
                    printf("-------------------\n");
                    break;
            }
            xSemaphoreGive(xPrintMutex);
        }
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_04: Task Suspend and Resume\n");
    printf("  Queue-based command dispatch for suspend/resume\n");
    printf("=========================================================\n\n");

    xCmdQueue = xQueueCreate(10, sizeof(Command_t));
    xPrintMutex = xSemaphoreCreateMutex();

    for (int i = 0; i < 3; i++)
        xTaskCreate(vWorkerTask, worker_names[i], 4096, (void*)(intptr_t)i, 1, &xWorkerHandles[i]);
    xTaskCreate(vControlTask, "Control", 4096, NULL, 3, NULL);
    xTaskCreate(vCommandHandler, "CmdHandler", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "All tasks created.");
}
