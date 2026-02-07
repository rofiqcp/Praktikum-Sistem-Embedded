/**
 * ESP32_03: Idle Task Hook Implementation
 * ========================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Idle hook berjalan saat tidak ada task aktif.
 *         Digunakan untuk monitoring idle time via Queue.
 *
 * Hardware: ESP32 DevKit V1 saja (serial via USB 115200)
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "IDLE_HOOK";
static volatile uint32_t idle_counter_core0 = 0;
static volatile uint32_t idle_counter_core1 = 0;
static SemaphoreHandle_t xPrintMutex = NULL;

/* Idle hook - called by ESP-IDF idle hook mechanism */
static bool idle_hook_core0(void)
{
    idle_counter_core0++;
    return true;
}

static bool idle_hook_core1(void)
{
    idle_counter_core1++;
    return true;
}

/* Work task - simulasi beban CPU variabel */
static void vWorkTask(void *pv)
{
    int load_percent = (int)(intptr_t)pv;
    for (;;) {
        int64_t start = esp_timer_get_time();
        while ((esp_timer_get_time() - start) < (load_percent * 100)) {
            /* busy */
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Monitor task - track idle rates */
static void vMonitorTask(void *pv)
{
    uint32_t last0 = 0, last1 = 0;
    for (;;) {
        uint32_t cur0 = idle_counter_core0;
        uint32_t cur1 = idle_counter_core1;
        uint32_t d0 = cur0 - last0;
        uint32_t d1 = cur1 - last1;
        last0 = cur0; last1 = cur1;

        xSemaphoreTake(xPrintMutex, portMAX_DELAY);
        printf("[Monitor] Core0 idle: %lu/s | Core1 idle: %lu/s | heap=%lu\n",
               (unsigned long)d0, (unsigned long)d1,
               (unsigned long)esp_get_free_heap_size());
        xSemaphoreGive(xPrintMutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_03: Idle Task Hook Implementation\n");
    printf("  Monitor idle cycles & CPU load per core\n");
    printf("=========================================================\n\n");

    xPrintMutex = xSemaphoreCreateMutex();

    /* Register idle hooks per core */
    esp_register_freertos_idle_hook_for_cpu(idle_hook_core0, 0);
    esp_register_freertos_idle_hook_for_cpu(idle_hook_core1, 1);

    /* Work tasks with different loads, pinned to different cores */
    xTaskCreatePinnedToCore(vWorkTask, "Work30", 4096, (void*)30, 1, NULL, 0);
    xTaskCreatePinnedToCore(vWorkTask, "Work50", 4096, (void*)50, 1, NULL, 1);
    xTaskCreate(vMonitorTask, "Monitor", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "Idle hooks registered. Tasks created.");
}
