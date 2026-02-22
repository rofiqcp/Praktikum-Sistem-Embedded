/*
 * ===========================================================================
 * MODUL 10 - Percobaan 03: Queue Multiple
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Menggunakan multiple queues untuk routing pesan ke handler berbeda.
 * Hardware: Serial Monitor + 2x LED (GPIO2, GPIO4)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#define LED1_PIN    GPIO_NUM_2
#define LED2_PIN    GPIO_NUM_4

static const char *TAG = "QUEUE_MULTI";
static QueueHandle_t xCommandQueue;
static QueueHandle_t xStatusQueue;

typedef struct {
    uint8_t cmd_id;
    uint16_t param;
} command_t;

typedef struct {
    uint8_t cmd_id;
    uint8_t status;  /* 0=OK, 1=ERROR */
    uint32_t tick;
} status_t;

static void commander_task(void *pvParam)
{
    command_t cmd;
    uint8_t id = 0;
    while (1) {
        cmd.cmd_id = id++;
        cmd.param = esp_random() % 1000;
        
        if (xQueueSend(xCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[CMD] Sent cmd #%d param=%d", cmd.cmd_id, cmd.param);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void executor_task(void *pvParam)
{
    command_t cmd;
    status_t sts;
    while (1) {
        if (xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "[EXEC] Processing cmd #%d param=%d", cmd.cmd_id, cmd.param);
            
            /* Toggle LEDs based on command */
            gpio_set_level(LED1_PIN, cmd.param % 2);
            gpio_set_level(LED2_PIN, (cmd.param / 2) % 2);
            
            vTaskDelay(pdMS_TO_TICKS(200)); /* Simulate work */
            
            /* Send status back */
            sts.cmd_id = cmd.cmd_id;
            sts.status = (cmd.param > 500) ? 1 : 0;
            sts.tick = xTaskGetTickCount();
            xQueueSend(xStatusQueue, &sts, pdMS_TO_TICKS(100));
        }
    }
}

static void monitor_task(void *pvParam)
{
    status_t sts;
    while (1) {
        if (xQueueReceive(xStatusQueue, &sts, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "[MON] Cmd #%d %s @tick=%lu",
                     sts.cmd_id,
                     sts.status == 0 ? "OK" : "ERROR",
                     (unsigned long)sts.tick);
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(LED1_PIN); gpio_set_direction(LED1_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED2_PIN); gpio_set_direction(LED2_PIN, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== MODUL 10: Multiple Queues ===");
    
    xCommandQueue = xQueueCreate(5, sizeof(command_t));
    xStatusQueue  = xQueueCreate(5, sizeof(status_t));
    
    xTaskCreate(commander_task, "Commander", 2048, NULL, 2, NULL);
    xTaskCreate(executor_task,  "Executor",  2048, NULL, 3, NULL);
    xTaskCreate(monitor_task,   "Monitor",   2048, NULL, 1, NULL);
}
