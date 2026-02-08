/*
 * ESP32_04_Static_Allocation
 * xTaskCreateStatic() — no dynamic allocation.
 * StaticTask_t + StackType_t array.  Also xQueueCreateStatic().
 * LED blink on GPIO2 created statically.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "STATIC_ALLOC";

#define LED_PIN          GPIO_NUM_2
#define BLINK_STACK_SIZE 2048
#define QUEUE_LENGTH     5
#define QUEUE_ITEM_SIZE  sizeof(uint32_t)

/* ------------------------------------------------------------------ */
/*  Static memory for the blink task                                  */
/* ------------------------------------------------------------------ */
static StackType_t  blink_stack[BLINK_STACK_SIZE];
static StaticTask_t blink_tcb;

/* ------------------------------------------------------------------ */
/*  Static memory for the monitor task                                */
/* ------------------------------------------------------------------ */
#define MON_STACK_SIZE 2048
static StackType_t  mon_stack[MON_STACK_SIZE];
static StaticTask_t mon_tcb;

/* ------------------------------------------------------------------ */
/*  Static memory for the queue                                       */
/* ------------------------------------------------------------------ */
static uint8_t       queue_storage[QUEUE_LENGTH * QUEUE_ITEM_SIZE];
static StaticQueue_t queue_cb;
static QueueHandle_t xQueue;

/* ------------------------------------------------------------------ */
/*  Blink task (statically allocated)                                 */
/* ------------------------------------------------------------------ */
static void blink_task(void *pv)
{
    bool led_on = false;
    uint32_t count = 0;

    while (1) {
        led_on = !led_on;
        gpio_set_level(LED_PIN, led_on ? 1 : 0);
        count++;

        /* Send count through static queue */
        xQueueSend(xQueue, &count, 0);

        printf("[BLINK] LED %s  count=%lu  HWM=%u\n",
               led_on ? "ON " : "OFF",
               (unsigned long)count,
               (unsigned)uxTaskGetStackHighWaterMark(NULL));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ------------------------------------------------------------------ */
/*  Monitor task (statically allocated)                               */
/* ------------------------------------------------------------------ */
static void monitor_task(void *pv)
{
    uint32_t recv;

    while (1) {
        if (xQueueReceive(xQueue, &recv, pdMS_TO_TICKS(2000)) == pdTRUE) {
            printf("[MONITOR] Received count=%lu from queue\n", (unsigned long)recv);
        }

        /* Show that no heap was consumed for task/queue creation */
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        printf("[MONITOR] Free heap: %u bytes  (no malloc for tasks/queue!)\n",
               (unsigned)free_heap);
        printf("[MONITOR] Monitor HWM: %u words\n",
               (unsigned)uxTaskGetStackHighWaterMark(NULL));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Static Allocation Demo ===");
    ESP_LOGI(TAG, "No pvPortMalloc / heap_caps_malloc used for tasks or queue!");

    /* Record heap BEFORE creating anything */
    size_t heap_before = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    ESP_LOGI(TAG, "Heap BEFORE static creates: %u bytes", (unsigned)heap_before);

    /* Configure LED */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(LED_PIN, 0);

    /* Create static queue */
    xQueue = xQueueCreateStatic(QUEUE_LENGTH, QUEUE_ITEM_SIZE,
                                queue_storage, &queue_cb);
    ESP_LOGI(TAG, "Static queue created @ %p", (void *)xQueue);

    /* Create static tasks */
    TaskHandle_t h_blink = xTaskCreateStatic(
        blink_task, "blink_st", BLINK_STACK_SIZE,
        NULL, 5, blink_stack, &blink_tcb);
    ESP_LOGI(TAG, "Static blink task created @ %p", (void *)h_blink);

    TaskHandle_t h_mon = xTaskCreateStatic(
        monitor_task, "mon_st", MON_STACK_SIZE,
        NULL, 4, mon_stack, &mon_tcb);
    ESP_LOGI(TAG, "Static monitor task created @ %p", (void *)h_mon);

    /* Record heap AFTER — should be essentially the same */
    size_t heap_after = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    ESP_LOGI(TAG, "Heap AFTER static creates : %u bytes", (unsigned)heap_after);
    ESP_LOGI(TAG, "Heap delta (should be ~0) : %d bytes",
             (int)(heap_before - heap_after));
}
