/*
 * ===========================================================================
 * MODUL 10 - Percobaan 01: Queue Basic
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Mengirim integer via queue — producer-consumer pattern dasar.
 * Producer task mengirim counter ke queue, consumer task menerima & mencetak.
 * Hardware: Serial Monitor + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN     GPIO_NUM_2
#define QUEUE_LEN   5

static const char *TAG = "QUEUE_BASIC";
static QueueHandle_t xQueue;

static void producer_task(void *pvParam)
{
    int counter = 0;
    while (1) {
        counter++;
        if (xQueueSend(xQueue, &counter, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[PRODUCER] Sent: %d", counter);
        } else {
            ESP_LOGW(TAG, "[PRODUCER] Queue FULL, cannot send %d", counter);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void consumer_task(void *pvParam)
{
    int received;
    while (1) {
        if (xQueueReceive(xQueue, &received, pdMS_TO_TICKS(1000)) == pdPASS) {
            ESP_LOGI(TAG, "[CONSUMER] Received: %d", received);
            gpio_set_level(LED_PIN, received % 2);  /* Toggle LED */
        } else {
            ESP_LOGW(TAG, "[CONSUMER] Timeout - no data in queue");
        }
    }
}

void app_main(void)
{
    /* Configure LED */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "=== MODUL 10: Queue Basic - Producer Consumer ===");
    ESP_LOGI(TAG, "Queue length: %d, Item size: %d bytes", QUEUE_LEN, sizeof(int));

    /* Create queue */
    xQueue = xQueueCreate(QUEUE_LEN, sizeof(int));
    if (xQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue!");
        return;
    }

    /* Create tasks */
    xTaskCreate(producer_task, "Producer", 2048, NULL, 2, NULL);
    xTaskCreate(consumer_task, "Consumer", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "Tasks created. Scheduler running...");
}
