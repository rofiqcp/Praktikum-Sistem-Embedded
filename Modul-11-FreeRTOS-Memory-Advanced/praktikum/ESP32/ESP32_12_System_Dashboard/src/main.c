/*
 * ESP32_12_System_Dashboard
 * Capstone: combine ALL FreeRTOS concepts from Modul 09-12.
 * - Task list with vTaskList() + stack high-water marks
 * - Heap statistics (free, min-ever, largest block)
 * - Per-task CPU usage (vTaskGetRunTimeStats if configured)
 * - System uptime
 * - Formatted dashboard every 5 s
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "driver/gpio.h"

static const char *TAG = "DASHBOARD";

#define LED_PIN  GPIO_NUM_2

/* ------------------------------------------------------------------ */
/*  Shared resources for demo tasks                                   */
/* ------------------------------------------------------------------ */
static QueueHandle_t     xSensorQueue;
static SemaphoreHandle_t xMutex;
static TimerHandle_t     xHeartbeatTimer;
static volatile uint32_t heartbeat_count = 0;
static portMUX_TYPE      dash_spinlock = portMUX_INITIALIZER_UNLOCKED;

/* ------------------------------------------------------------------ */
/*  Print system dashboard                                            */
/* ------------------------------------------------------------------ */
static void print_dashboard(void)
{
    uint32_t uptime_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    uint32_t up_s  = uptime_ms / 1000;
    uint32_t up_m  = up_s / 60;
    uint32_t up_h  = up_m / 60;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║             ESP32 FreeRTOS SYSTEM DASHBOARD                 ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");

    /* Uptime */
    printf("║ Uptime: %02lu:%02lu:%02lu (%lu ms)         Heartbeats: %-8lu      ║\n",
           (unsigned long)up_h,
           (unsigned long)(up_m % 60),
           (unsigned long)(up_s % 60),
           (unsigned long)uptime_ms,
           (unsigned long)heartbeat_count);

    /* Chip info */
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    printf("║ Chip: %s  Cores: %d  Rev: %d  Features: 0x%lX               ║\n",
           CONFIG_IDF_TARGET, chip.cores, chip.revision,
           (unsigned long)chip.features);

    printf("╠══════════════════════════════════════════════════════════════╣\n");

    /* Heap */
    size_t free_heap   = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t min_ever    = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    size_t largest_blk = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    size_t total       = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    float  used_pct    = total > 0 ? (1.0f - (float)free_heap / (float)total) * 100.0f : 0;
    float  frag_pct    = free_heap > 0 ? (1.0f - (float)largest_blk / (float)free_heap) * 100.0f : 0;

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

    printf("║ HEAP MEMORY                                                 ║\n");
    printf("║   Total:  %6u B  Free: %6u B  Used: %5.1f%%              ║\n",
           (unsigned)total, (unsigned)free_heap, used_pct);
    printf("║   Min-ever free: %6u B  Largest block: %6u B            ║\n",
           (unsigned)min_ever, (unsigned)largest_blk);
    printf("║   Fragmentation: %5.1f%%  Blocks: %u alloc, %u free         ║\n",
           frag_pct, (unsigned)info.allocated_blocks, (unsigned)info.free_blocks);

    /* Internal vs PSRAM */
    size_t int_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t spi_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    printf("║   Internal: %6u B free   SPIRAM: %6u B free             ║\n",
           (unsigned)int_free, (unsigned)spi_free);

    printf("╠══════════════════════════════════════════════════════════════╣\n");

    /* Task list — use uxTaskGetSystemState (always available) */
    printf("║ TASK LIST                                                   ║\n");
    printf("║   Name              State  Prio  HWM(w)                     ║\n");
    printf("║   ──────────────────────────────────────                     ║\n");

    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *task_array = pvPortMalloc(num_tasks * sizeof(TaskStatus_t));
    if (task_array) {
        UBaseType_t got = uxTaskGetSystemState(task_array, num_tasks, NULL);
        const char *state_str[] = {"Run", "Rdy", "Blk", "Sus", "Del", "Inv"};
        for (UBaseType_t i = 0; i < got; i++) {
            int s = task_array[i].eCurrentState;
            if (s < 0 || s > 5) s = 5;
            char line_buf[60];
            snprintf(line_buf, sizeof(line_buf), "%-18s %3s    %2lu   %5u",
                     task_array[i].pcTaskName,
                     state_str[s],
                     (unsigned long)task_array[i].uxCurrentPriority,
                     (unsigned)task_array[i].usStackHighWaterMark);
            printf("║   %-56s  ║\n", line_buf);
        }
        vPortFree(task_array);
    }

    printf("╠══════════════════════════════════════════════════════════════╣\n");

    /* Runtime stats note */
    printf("║ CPU USAGE: enable trace facility in menuconfig for details  ║\n");

    printf("╠══════════════════════════════════════════════════════════════╣\n");

    /* Queue / semaphore status */
    UBaseType_t q_msgs = uxQueueMessagesWaiting(xSensorQueue);
    UBaseType_t q_space = uxQueueSpacesAvailable(xSensorQueue);
    printf("║ IPC STATUS                                                  ║\n");
    printf("║   Sensor Queue: %u msgs / %u free                           ║\n",
           (unsigned)q_msgs, (unsigned)q_space);
    printf("║   Mutex: available                                          ║\n");
    printf("║   Active tasks: %lu                                          ║\n",
           (unsigned long)uxTaskGetNumberOfTasks());

    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
}

/* ------------------------------------------------------------------ */
/*  Heartbeat timer callback                                          */
/* ------------------------------------------------------------------ */
static void heartbeat_callback(TimerHandle_t xTimer)
{
    portENTER_CRITICAL(&dash_spinlock);
    heartbeat_count++;
    portEXIT_CRITICAL(&dash_spinlock);

    static bool led_on = false;
    led_on = !led_on;
    gpio_set_level(LED_PIN, led_on ? 1 : 0);
}

/* ------------------------------------------------------------------ */
/*  Simulated sensor task                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t timestamp;
    int16_t  temperature;  /* x10 */
    uint16_t humidity;     /* x10 */
} sensor_data_t;

static void sensor_task(void *pv)
{
    uint32_t seq = 0;
    while (1) {
        sensor_data_t data;
        data.timestamp   = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        data.temperature = 250 + (seq % 50) - 25;   /* 22.5–27.4°C */
        data.humidity    = 500 + (seq * 3) % 200;    /* 50.0–69.9% */

        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(50));
            xSemaphoreGive(xMutex);
        }
        seq++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ------------------------------------------------------------------ */
/*  Data processing task                                              */
/* ------------------------------------------------------------------ */
static void processing_task(void *pv)
{
    sensor_data_t data;
    uint32_t processed = 0;
    int32_t temp_sum = 0;

    while (1) {
        if (xQueueReceive(xSensorQueue, &data, pdMS_TO_TICKS(2000)) == pdTRUE) {
            processed++;
            temp_sum += data.temperature;

            if (processed % 5 == 0) {
                int32_t avg_temp = temp_sum / 5;
                printf("[PROC] %lu readings  avg_temp=%.1f°C  last_hum=%.1f%%\n",
                       (unsigned long)processed,
                       avg_temp / 10.0f,
                       data.humidity / 10.0f);
                temp_sum = 0;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  CPU load simulation task                                          */
/* ------------------------------------------------------------------ */
static void cpu_load_task(void *pv)
{
    while (1) {
        /* Simulate varying CPU load */
        volatile uint32_t sum = 0;
        int load_cycles = 10000 + (xTaskGetTickCount() % 20000);
        for (int i = 0; i < load_cycles; i++) {
            sum += i;
        }
        (void)sum;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ------------------------------------------------------------------ */
/*  Dashboard task — main display loop                                */
/* ------------------------------------------------------------------ */
static void dashboard_task(void *pv)
{
    while (1) {
        print_dashboard();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 FreeRTOS System Dashboard ===");
    ESP_LOGI(TAG, "Capstone project — Modul 09-12 combined");

    /* LED */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);

    /* IPC primitives */
    xSensorQueue = xQueueCreate(10, sizeof(sensor_data_t));
    xMutex = xSemaphoreCreateMutex();

    /* Software timer for heartbeat */
    xHeartbeatTimer = xTimerCreate("heartbeat", pdMS_TO_TICKS(500),
                                    pdTRUE, NULL, heartbeat_callback);
    xTimerStart(xHeartbeatTimer, 0);

    /* Create all tasks */
    xTaskCreate(sensor_task,     "sensor",     2048, NULL, 5, NULL);
    xTaskCreate(processing_task, "processing", 2048, NULL, 4, NULL);
    xTaskCreate(cpu_load_task,   "cpu_load",   2048, NULL, 2, NULL);
    xTaskCreate(dashboard_task,  "dashboard",  4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "All tasks started. Dashboard refreshes every 5s.");
}
