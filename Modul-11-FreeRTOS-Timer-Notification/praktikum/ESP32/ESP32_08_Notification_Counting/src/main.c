/*
 * ESP32_08_Notification_Counting
 * ================================
 * Task notification as counting semaphore.
 * Multiple events increment notification value.
 * Receiver task processes events one at a time
 * using ulTaskNotifyTake(pdTRUE, ...) to decrement.
 *
 * Hardware: Serial (UART0), 1x LED on GPIO2
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "NOTIFY_COUNT";

#define LED_GPIO    GPIO_NUM_2

static TaskHandle_t xProcessorTaskHandle = NULL;
static volatile uint32_t events_generated = 0;
static volatile uint32_t events_processed = 0;
static volatile uint32_t batch_count = 0;
static bool led_state = false;

/* Simulated event producer tasks */
static void event_producer_task(void *pvParameters)
{
    int producer_id = (int)(uintptr_t)pvParameters;
    uint32_t my_events = 0;

    ESP_LOGI(TAG, "INFO,Producer %d started", producer_id);

    while (1) {
        /* Each producer generates events at different rates */
        uint32_t delay_ms = 500 + (producer_id * 300);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        /* Generate 1-3 events at once */
        int num_events = (producer_id % 3) + 1;

        for (int i = 0; i < num_events; i++) {
            my_events++;
            events_generated++;

            /*
             * xTaskNotifyGive() increments the notification value.
             * Each call adds 1 to the receiver's notification count,
             * acting like a counting semaphore.
             */
            xTaskNotifyGive(xProcessorTaskHandle);
        }

        ESP_LOGI(TAG, "EVENT,PRODUCED,producer=%d,batch=%d,total_produced=%lu,my_events=%lu,tick=%lu",
                 producer_id, num_events,
                 (unsigned long)events_generated,
                 (unsigned long)my_events,
                 (unsigned long)xTaskGetTickCount());
    }
}

static void processor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "INFO,Processor task started, waiting for events...");

    while (1) {
        /*
         * ulTaskNotifyTake():
         * - pdTRUE: Decrement by 1 (counting semaphore behavior)
         *   Returns the count BEFORE decrementing
         * - pdFALSE: Clear to zero (binary semaphore behavior)
         *   Returns the count before clearing
         *
         * We use pdTRUE to process events one at a time.
         */
        uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (count > 0) {
            events_processed++;
            batch_count++;

            /* Toggle LED to show processing */
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state ? 1 : 0);

            ESP_LOGI(TAG, "EVENT,PROCESSED,count=%lu,pending_before=%lu,generated=%lu,processed=%lu,backlog=%lu,tick=%lu",
                     (unsigned long)events_processed,
                     (unsigned long)count,
                     (unsigned long)events_generated,
                     (unsigned long)events_processed,
                     (unsigned long)(events_generated - events_processed),
                     (unsigned long)xTaskGetTickCount());

            /* Simulate processing time */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/* Burst generator - generates many events at once to show counting behavior */
static void burst_generator_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(10000));  /* Wait 10s before first burst */

    while (1) {
        int burst_size = 10;
        batch_count = 0;

        ESP_LOGW(TAG, "EVENT,BURST_START,size=%d,generating %d events rapidly", burst_size, burst_size);

        int64_t start = esp_timer_get_time();

        /* Generate burst of events */
        for (int i = 0; i < burst_size; i++) {
            events_generated++;
            xTaskNotifyGive(xProcessorTaskHandle);
        }

        int64_t elapsed = esp_timer_get_time() - start;

        ESP_LOGW(TAG, "EVENT,BURST_GENERATED,size=%d,generation_time=%lld_us,backlog=%lu",
                 burst_size,
                 (long long)elapsed,
                 (unsigned long)(events_generated - events_processed));

        /* Wait for all burst events to be processed */
        vTaskDelay(pdMS_TO_TICKS(burst_size * 150 + 1000));

        ESP_LOGI(TAG, "EVENT,BURST_COMPLETE,processed=%lu,total_gen=%lu,total_proc=%lu,backlog=%lu",
                 (unsigned long)batch_count,
                 (unsigned long)events_generated,
                 (unsigned long)events_processed,
                 (unsigned long)(events_generated - events_processed));

        vTaskDelay(pdMS_TO_TICKS(15000));  /* Wait before next burst */
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Notification Counting Demo ===");
    ESP_LOGI(TAG, "INFO,Task notification as counting semaphore");
    ESP_LOGI(TAG, "INFO,Multiple producers, single consumer");
    ESP_LOGI(TAG, "INFO,LED GPIO: %d (toggles on each event processed)", LED_GPIO);

    /* Configure LED */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_GPIO, 0);

    /* Create processor task first (needs handle for producers) */
    xTaskCreate(processor_task, "processor", 4096, NULL, 10, &xProcessorTaskHandle);

    /* Create 3 event producer tasks */
    for (int i = 0; i < 3; i++) {
        char name[20];
        snprintf(name, sizeof(name), "producer_%d", i);
        xTaskCreate(event_producer_task, name, 4096, (void *)(uintptr_t)i, 5, NULL);
    }

    /* Create burst generator */
    xTaskCreate(burst_generator_task, "burst_gen", 4096, NULL, 5, NULL);

    /* Monitor */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "STATUS,generated=%lu,processed=%lu,backlog=%lu,led=%s,tick=%lu",
                 (unsigned long)events_generated,
                 (unsigned long)events_processed,
                 (unsigned long)(events_generated - events_processed),
                 led_state ? "ON" : "OFF",
                 (unsigned long)xTaskGetTickCount());
    }
}
