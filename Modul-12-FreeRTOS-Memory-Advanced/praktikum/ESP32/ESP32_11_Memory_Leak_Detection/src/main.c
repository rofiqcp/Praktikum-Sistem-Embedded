/*
 * ESP32_11_Memory_Leak_Detection
 * Periodic heap monitoring to detect memory leaks.
 * Allocate in a loop without free to simulate a leak.
 * Track watermark changes and print leak report.
 *
 * Note: Full esp_heap_trace requires CONFIG_HEAP_TRACING_STANDALONE
 * in sdkconfig. This demo uses periodic monitoring as a reliable
 * leak detection method that works without extra config.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "LEAK_DET";

/* ------------------------------------------------------------------ */
/*  Heap snapshot structure for tracking                              */
/* ------------------------------------------------------------------ */
#define MAX_SNAPSHOTS 60

typedef struct {
    uint32_t tick_ms;
    size_t   free_heap;
    size_t   min_ever;
    size_t   largest_block;
    size_t   alloc_blocks;
} heap_snapshot_t;

static heap_snapshot_t snapshots[MAX_SNAPSHOTS];
static int snapshot_count = 0;

static void take_snapshot(void)
{
    if (snapshot_count >= MAX_SNAPSHOTS) return;

    heap_snapshot_t *s = &snapshots[snapshot_count];
    s->tick_ms      = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    s->free_heap    = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    s->min_ever     = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    s->largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    s->alloc_blocks = info.allocated_blocks;

    snapshot_count++;
}

/* ------------------------------------------------------------------ */
/*  Leaky task — intentionally leaks memory                           */
/* ------------------------------------------------------------------ */
static volatile int leak_active = 0;
static int leak_count = 0;
static size_t total_leaked = 0;

static void leaky_task(void *pv)
{
    /* Wait before starting to leak */
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGW(TAG, ">>> Starting intentional memory leak <<<");
    leak_active = 1;

    while (1) {
        /* Allocate 128 bytes each iteration WITHOUT freeing */
        size_t leak_size = 128;
        void *leaked = heap_caps_malloc(leak_size, MALLOC_CAP_DEFAULT);
        if (leaked) {
            memset(leaked, 0xDE, leak_size);
            leak_count++;
            total_leaked += leak_size;
            printf("[LEAK] Allocated %u bytes @ %p (total leaked: %u bytes, %d blocks)\n",
                   (unsigned)leak_size, leaked,
                   (unsigned)total_leaked, leak_count);
        } else {
            printf("[LEAK] Allocation FAILED — heap exhausted after %d leaks (%u bytes)\n",
                   leak_count, (unsigned)total_leaked);
            leak_active = 0;
            break;
        }

        /* Intentionally NOT freeing — this is the leak! */

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Stay alive */
    while (1) vTaskDelay(pdMS_TO_TICKS(10000));
}

/* ------------------------------------------------------------------ */
/*  Monitor task — takes periodic snapshots and detects leaks         */
/* ------------------------------------------------------------------ */
static void monitor_task(void *pv)
{
    size_t prev_free = 0;
    int consecutive_drops = 0;
    bool leak_detected = false;

    while (1) {
        take_snapshot();

        if (snapshot_count < 1) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        heap_snapshot_t *cur = &snapshots[snapshot_count - 1];

        printf("\n[MON] t=%lu ms  free=%u  min_ever=%u  largest=%u  alloc_blocks=%u\n",
               (unsigned long)cur->tick_ms,
               (unsigned)cur->free_heap,
               (unsigned)cur->min_ever,
               (unsigned)cur->largest_block,
               (unsigned)cur->alloc_blocks);

        /* Leak detection: check for consistent decrease */
        if (prev_free > 0) {
            int delta = (int)prev_free - (int)cur->free_heap;
            printf("[MON] Heap delta: %+d bytes\n", delta);

            if (delta > 0) {
                consecutive_drops++;
                if (consecutive_drops >= 3 && !leak_detected) {
                    leak_detected = true;
                    printf("\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    printf("!!! MEMORY LEAK DETECTED !!!           \n");
                    printf("!!! %d consecutive drops in free heap  \n", consecutive_drops);
                    printf("!!! Consider reviewing recent allocations\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
                }
            } else {
                consecutive_drops = 0;
            }
        }
        prev_free = cur->free_heap;

        /* Print leak report every 20 seconds */
        if (snapshot_count % 10 == 0 && snapshot_count > 1) {
            printf("\n===== LEAK DETECTION REPORT =====\n");
            heap_snapshot_t *first = &snapshots[0];
            heap_snapshot_t *last  = &snapshots[snapshot_count - 1];

            int total_delta = (int)first->free_heap - (int)last->free_heap;
            float rate_bps  = (float)total_delta /
                              ((float)(last->tick_ms - first->tick_ms) / 1000.0f);

            printf("  Observation period : %lu ms\n",
                   (unsigned long)(last->tick_ms - first->tick_ms));
            printf("  Initial free heap  : %u bytes\n", (unsigned)first->free_heap);
            printf("  Current free heap  : %u bytes\n", (unsigned)last->free_heap);
            printf("  Total decrease     : %d bytes\n", total_delta);
            printf("  Leak rate          : %.1f bytes/sec\n", rate_bps);
            printf("  Alloc blocks: %u -> %u (delta=%d)\n",
                   (unsigned)first->alloc_blocks,
                   (unsigned)last->alloc_blocks,
                   (int)last->alloc_blocks - (int)first->alloc_blocks);
            printf("  Snapshots          : %d / %d\n", snapshot_count, MAX_SNAPSHOTS);

            if (total_delta > 500) {
                printf("  VERDICT: *** LEAK LIKELY (%d bytes lost) ***\n", total_delta);
            } else if (total_delta > 0) {
                printf("  VERDICT: Minor decrease, may be normal\n");
            } else {
                printf("  VERDICT: No leak detected\n");
            }
            printf("=================================\n\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Memory Leak Detection Demo ===");
    ESP_LOGI(TAG, "Monitor runs every 2s. Leak starts after 5s.");
    ESP_LOGI(TAG, "Watch for consistent free heap decrease.");

    /* Initial state */
    printf("[INIT] Free heap: %u bytes\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

    xTaskCreate(monitor_task, "monitor", 4096, NULL, 5, NULL);
    xTaskCreate(leaky_task,   "leaky",   2048, NULL, 4, NULL);
}
