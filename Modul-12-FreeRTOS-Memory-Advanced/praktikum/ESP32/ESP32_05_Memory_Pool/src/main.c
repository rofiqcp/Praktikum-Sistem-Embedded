/*
 * ESP32_05_Memory_Pool
 * Fixed-size block allocator using a FreeRTOS queue of pointers.
 * Zero fragmentation. Compare with dynamic allocation.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "MEM_POOL";

/* ------------------------------------------------------------------ */
/*  Memory pool configuration                                        */
/* ------------------------------------------------------------------ */
#define POOL_BLOCK_SIZE   64      /* bytes per block */
#define POOL_BLOCK_COUNT  20      /* number of blocks */

static uint8_t pool_memory[POOL_BLOCK_COUNT][POOL_BLOCK_SIZE];
static QueueHandle_t pool_queue;  /* queue of free-block pointers */

/* ------------------------------------------------------------------ */
/*  Pool API                                                          */
/* ------------------------------------------------------------------ */
static int pool_init(void)
{
    pool_queue = xQueueCreate(POOL_BLOCK_COUNT, sizeof(void *));
    if (!pool_queue) return -1;

    for (int i = 0; i < POOL_BLOCK_COUNT; i++) {
        void *ptr = &pool_memory[i][0];
        xQueueSend(pool_queue, &ptr, 0);
    }
    return 0;
}

static void *pool_alloc(TickType_t wait)
{
    void *ptr = NULL;
    if (xQueueReceive(pool_queue, &ptr, wait) == pdTRUE)
        return ptr;
    return NULL;
}

static void pool_free(void *ptr)
{
    if (ptr) xQueueSend(pool_queue, &ptr, 0);
}

static UBaseType_t pool_available(void)
{
    return uxQueueMessagesWaiting(pool_queue);
}

/* ------------------------------------------------------------------ */
/*  Benchmark: pool vs dynamic allocation                             */
/* ------------------------------------------------------------------ */
static void benchmark_task(void *pv)
{
    const int ITERS = 100;
    void *ptrs[ITERS];

    while (1) {
        printf("\n===== POOL vs DYNAMIC BENCHMARK (%d iters) =====\n", ITERS);

        /* --- Pool allocations --- */
        int pool_ok = 0;
        int64_t t0 = esp_timer_get_time();
        for (int i = 0; i < ITERS; i++) {
            ptrs[i] = pool_alloc(0);
            if (ptrs[i]) {
                memset(ptrs[i], 0xAA, POOL_BLOCK_SIZE);
                pool_ok++;
            }
        }
        int64_t t1 = esp_timer_get_time();
        for (int i = 0; i < ITERS; i++) {
            if (ptrs[i]) pool_free(ptrs[i]);
            ptrs[i] = NULL;
        }
        int64_t t2 = esp_timer_get_time();
        printf("[POOL]    alloc=%lld us  free=%lld us  success=%d/%d\n",
               (long long)(t1 - t0), (long long)(t2 - t1), pool_ok, ITERS);

        /* --- Dynamic allocations --- */
        int dyn_ok = 0;
        t0 = esp_timer_get_time();
        for (int i = 0; i < ITERS; i++) {
            ptrs[i] = heap_caps_malloc(POOL_BLOCK_SIZE, MALLOC_CAP_DEFAULT);
            if (ptrs[i]) {
                memset(ptrs[i], 0xBB, POOL_BLOCK_SIZE);
                dyn_ok++;
            }
        }
        t1 = esp_timer_get_time();
        for (int i = 0; i < ITERS; i++) {
            if (ptrs[i]) heap_caps_free(ptrs[i]);
            ptrs[i] = NULL;
        }
        t2 = esp_timer_get_time();
        printf("[DYNAMIC] alloc=%lld us  free=%lld us  success=%d/%d\n",
               (long long)(t1 - t0), (long long)(t2 - t1), dyn_ok, ITERS);

        /* Fragmentation comparison */
        size_t free_heap  = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        size_t largest    = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        float  frag_pct   = (1.0f - (float)largest / (float)free_heap) * 100.0f;

        printf("[HEAP]    Free=%u  Largest=%u  Frag=%.1f%%\n",
               (unsigned)free_heap, (unsigned)largest, frag_pct);
        printf("[POOL]    Available blocks: %u / %d (zero fragmentation)\n",
               (unsigned)pool_available(), POOL_BLOCK_COUNT);

        printf("============================================\n");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ------------------------------------------------------------------ */
/*  Producer/consumer demo with pool                                  */
/* ------------------------------------------------------------------ */
static QueueHandle_t data_queue;

typedef struct {
    uint32_t id;
    uint8_t  data[POOL_BLOCK_SIZE - sizeof(uint32_t)];
} pool_msg_t;

static void producer_task(void *pv)
{
    uint32_t seq = 0;
    while (1) {
        pool_msg_t *msg = (pool_msg_t *)pool_alloc(pdMS_TO_TICKS(1000));
        if (msg) {
            msg->id = seq++;
            memset(msg->data, (uint8_t)seq, sizeof(msg->data));
            xQueueSend(data_queue, &msg, portMAX_DELAY);
            printf("[PROD] Sent msg id=%lu  pool_avail=%u\n",
                   (unsigned long)msg->id, (unsigned)pool_available());
        } else {
            printf("[PROD] Pool empty, waiting…\n");
        }
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

static void consumer_task(void *pv)
{
    pool_msg_t *msg;
    while (1) {
        if (xQueueReceive(data_queue, &msg, pdMS_TO_TICKS(2000)) == pdTRUE) {
            printf("[CONS] Got msg id=%lu  pool_avail=%u\n",
                   (unsigned long)msg->id, (unsigned)pool_available());
            pool_free(msg);
        }
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Memory Pool Demo ===");

    if (pool_init() != 0) {
        ESP_LOGE(TAG, "Pool init failed!");
        return;
    }
    ESP_LOGI(TAG, "Pool: %d blocks x %d bytes = %d bytes total",
             POOL_BLOCK_COUNT, POOL_BLOCK_SIZE,
             POOL_BLOCK_COUNT * POOL_BLOCK_SIZE);

    data_queue = xQueueCreate(10, sizeof(pool_msg_t *));

    xTaskCreate(benchmark_task,  "bench",    4096, NULL, 3, NULL);
    xTaskCreate(producer_task,   "producer",  2048, NULL, 4, NULL);
    xTaskCreate(consumer_task,   "consumer",  2048, NULL, 4, NULL);
}
