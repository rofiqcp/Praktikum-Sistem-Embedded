/*
 * ESP32_11_Event_Group_Sync
 * ===========================
 * Barrier synchronization with xEventGroupSync().
 * 3 tasks must ALL reach barrier point before any can continue.
 * Each task lights its LED when reaching barrier.
 * All proceed together after synchronization.
 *
 * Hardware: 3x LED on GPIO2, GPIO4, GPIO5
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

static const char *TAG = "EVTGRP_SYNC";

#define LED1_GPIO   GPIO_NUM_2
#define LED2_GPIO   GPIO_NUM_4
#define LED3_GPIO   GPIO_NUM_5

/* Sync bits for each task */
#define TASK0_BIT   (1 << 0)
#define TASK1_BIT   (1 << 1)
#define TASK2_BIT   (1 << 2)
#define ALL_SYNC    (TASK0_BIT | TASK1_BIT | TASK2_BIT)

static EventGroupHandle_t xSyncEventGroup = NULL;
static volatile uint32_t sync_count = 0;
static volatile uint32_t phase = 0;

static const gpio_num_t led_gpios[] = { LED1_GPIO, LED2_GPIO, LED3_GPIO };
static const EventBits_t task_bits[] = { TASK0_BIT, TASK1_BIT, TASK2_BIT };
static const char *task_names[] = { "Task_A", "Task_B", "Task_C" };

typedef struct {
    int task_id;
    uint32_t work_time_min_ms;
    uint32_t work_time_max_ms;
} TaskConfig;

static void sync_task(void *pvParameters)
{
    TaskConfig *config = (TaskConfig *)pvParameters;
    int id = config->task_id;
    uint32_t my_sync_count = 0;

    ESP_LOGI(TAG, "INFO,%s started, work time: %lu-%lu ms",
             task_names[id],
             (unsigned long)config->work_time_min_ms,
             (unsigned long)config->work_time_max_ms);

    while (1) {
        phase++;

        /* Phase 1: Do some work (variable time per task) */
        uint32_t work_range = config->work_time_max_ms - config->work_time_min_ms;
        uint32_t work_time = config->work_time_min_ms + (esp_random() % (work_range + 1));

        ESP_LOGI(TAG, "EVENT,WORK_START,task=%s,id=%d,work_time=%lu_ms,tick=%lu",
                 task_names[id], id,
                 (unsigned long)work_time,
                 (unsigned long)xTaskGetTickCount());

        vTaskDelay(pdMS_TO_TICKS(work_time));

        ESP_LOGI(TAG, "EVENT,WORK_DONE,task=%s,id=%d,completed_in=%lu_ms,tick=%lu",
                 task_names[id], id,
                 (unsigned long)work_time,
                 (unsigned long)xTaskGetTickCount());

        /* Light LED to show this task reached the barrier */
        gpio_set_level(led_gpios[id], 1);

        ESP_LOGW(TAG, "EVENT,BARRIER_REACHED,task=%s,id=%d,my_bit=0x%02X,waiting_for=0x%02X,tick=%lu",
                 task_names[id], id,
                 (int)task_bits[id],
                 (int)ALL_SYNC,
                 (unsigned long)xTaskGetTickCount());

        int64_t wait_start = esp_timer_get_time();

        /*
         * xEventGroupSync():
         * - Sets uxBitsToSet (our bit) AND waits for uxBitsToWaitFor (all bits)
         * - Atomically sets and waits, ensuring proper barrier sync
         * - All bits are cleared when all tasks have set their bits
         *
         * This is the key function for barrier synchronization.
         */
        EventBits_t result = xEventGroupSync(
            xSyncEventGroup,
            task_bits[id],     /* Set our bit */
            ALL_SYNC,          /* Wait for ALL bits */
            pdMS_TO_TICKS(10000)  /* Timeout */
        );

        int64_t wait_time = esp_timer_get_time() - wait_start;

        if ((result & ALL_SYNC) == ALL_SYNC) {
            my_sync_count++;
            if (id == 0) sync_count++;  /* Only one task counts */

            ESP_LOGI(TAG, "EVENT,BARRIER_PASSED,task=%s,id=%d,sync_count=%lu,wait_us=%lld,result=0x%02lX,tick=%lu",
                     task_names[id], id,
                     (unsigned long)my_sync_count,
                     (long long)wait_time,
                     (unsigned long)result,
                     (unsigned long)xTaskGetTickCount());

            /* All LEDs on briefly to show synchronization */
            vTaskDelay(pdMS_TO_TICKS(500));

            /* Turn off our LED */
            gpio_set_level(led_gpios[id], 0);
        } else {
            ESP_LOGE(TAG, "EVENT,BARRIER_TIMEOUT,task=%s,id=%d,result=0x%02lX,tick=%lu",
                     task_names[id], id,
                     (unsigned long)result,
                     (unsigned long)xTaskGetTickCount());
            gpio_set_level(led_gpios[id], 0);
        }

        /* Small delay before next cycle */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Static task configs */
static TaskConfig configs[3] = {
    { .task_id = 0, .work_time_min_ms = 500,  .work_time_max_ms = 1500 },
    { .task_id = 1, .work_time_min_ms = 1000, .work_time_max_ms = 3000 },
    { .task_id = 2, .work_time_min_ms = 2000, .work_time_max_ms = 4000 },
};

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Event Group Sync (Barrier) Demo ===");
    ESP_LOGI(TAG, "INFO,3 tasks synchronize at a barrier using xEventGroupSync()");
    ESP_LOGI(TAG, "INFO,LED1(GPIO%d)=Task_A, LED2(GPIO%d)=Task_B, LED3(GPIO%d)=Task_C",
             LED1_GPIO, LED2_GPIO, LED3_GPIO);
    ESP_LOGI(TAG, "INFO,Each LED lights when its task reaches the barrier");
    ESP_LOGI(TAG, "INFO,All 3 LEDs ON simultaneously = barrier sync achieved");

    /* Configure LEDs */
    for (int i = 0; i < 3; i++) {
        gpio_config_t led_conf = {
            .pin_bit_mask = (1ULL << led_gpios[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&led_conf);
        gpio_set_level(led_gpios[i], 0);
    }

    /* Create event group for sync */
    xSyncEventGroup = xEventGroupCreate();
    if (xSyncEventGroup == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create sync event group!");
        return;
    }

    /* Create 3 sync tasks */
    for (int i = 0; i < 3; i++) {
        char name[20];
        snprintf(name, sizeof(name), "sync_%d", i);
        xTaskCreate(sync_task, name, 4096, &configs[i], 5, NULL);
    }

    /* Monitor */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        EventBits_t current = xEventGroupGetBits(xSyncEventGroup);
        ESP_LOGI(TAG, "STATUS,sync_count=%lu,current_bits=0x%02lX,phase=%lu,tick=%lu",
                 (unsigned long)sync_count,
                 (unsigned long)current,
                 (unsigned long)phase,
                 (unsigned long)xTaskGetTickCount());
    }
}
