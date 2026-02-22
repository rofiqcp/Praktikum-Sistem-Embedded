/*
 * ===========================================================================
 * MODUL 10 - Percobaan 09: Mutex Priority Inversion
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Demo: Priority inversion problem & fix with priority inheritance.
 * Low priority holds mutex → high priority blocked → medium runs first!
 * Mutex has priority inheritance built-in on ESP32.
 * Hardware: Serial Monitor + 3x LED (GPIO2, GPIO4, GPIO5)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_LOW     GPIO_NUM_2
#define LED_MED     GPIO_NUM_4
#define LED_HIGH    GPIO_NUM_5

static const char *TAG = "PRI_INV";
static SemaphoreHandle_t xMutex;

static void busy_wait_ms(int ms)
{
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(ms)) {
        taskYIELD();
    }
}

/* LOW priority task — holds mutex for a long time */
static void low_task(void *pvParam)
{
    while (1) {
        ESP_LOGI(TAG, "[LOW  prio=1] Taking mutex...");
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            gpio_set_level(LED_LOW, 1);
            ESP_LOGW(TAG, "[LOW  prio=1] GOT mutex — working 3s...");
            busy_wait_ms(3000);
            gpio_set_level(LED_LOW, 0);
            ESP_LOGW(TAG, "[LOW  prio=1] Releasing mutex");
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* MEDIUM priority task — no mutex, just CPU-bound */
static void medium_task(void *pvParam)
{
    vTaskDelay(pdMS_TO_TICKS(500)); /* Start slightly after low */
    while (1) {
        gpio_set_level(LED_MED, 1);
        ESP_LOGI(TAG, "[MED  prio=2] Running (no mutex needed)...");
        busy_wait_ms(2000);
        gpio_set_level(LED_MED, 0);
        ESP_LOGI(TAG, "[MED  prio=2] Done");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* HIGH priority task — needs mutex */
static void high_task(void *pvParam)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); /* Start after low has mutex */
    while (1) {
        ESP_LOGE(TAG, "[HIGH prio=3] Need mutex! Taking...");
        TickType_t t0 = xTaskGetTickCount();
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            TickType_t waited = xTaskGetTickCount() - t0;
            gpio_set_level(LED_HIGH, 1);
            ESP_LOGE(TAG, "[HIGH prio=3] GOT mutex after %lu ms (priority inheritance!)",
                     (unsigned long)(waited * portTICK_PERIOD_MS));
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(LED_HIGH, 0);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    gpio_reset_pin(LED_LOW);  gpio_set_direction(LED_LOW, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_MED);  gpio_set_direction(LED_MED, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_HIGH); gpio_set_direction(LED_HIGH, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== MODUL 10: Priority Inversion Demo ===");
    ESP_LOGI(TAG, "Mutex includes priority inheritance (ESP32 default)");
    
    xMutex = xSemaphoreCreateMutex();
    
    xTaskCreate(low_task,    "Low",    2048, NULL, 1, NULL);
    xTaskCreate(medium_task, "Medium", 2048, NULL, 2, NULL);
    xTaskCreate(high_task,   "High",   2048, NULL, 3, NULL);
}
