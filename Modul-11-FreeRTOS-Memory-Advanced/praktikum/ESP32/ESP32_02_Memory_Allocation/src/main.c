/*
 * ESP32_02_Memory_Allocation
 * Compare pvPortMalloc/vPortFree with heap_caps_malloc/free.
 * Measure allocation time, show fragmentation demo.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MEM_ALLOC";

#define NUM_BLOCKS   10
#define BLOCK_SIZE   512
#define LARGE_BLOCK  4096

/* ------------------------------------------------------------------ */
static void print_heap_status(const char *label)
{
    printf("[%s] Free: %u  Min-ever: %u  Largest-blk: %u\n",
           label,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
           (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
}

/* ------------------------------------------------------------------ */
/*  Benchmark a single allocation + free cycle                        */
/* ------------------------------------------------------------------ */
static void benchmark_alloc(const char *method, size_t size, bool use_caps)
{
    int64_t t0, t1, t2;
    void *p;

    t0 = esp_timer_get_time();
    if (use_caps) {
        p = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
    } else {
        p = pvPortMalloc(size);
    }
    t1 = esp_timer_get_time();

    if (!p) {
        printf("  %s(%u) -> FAILED\n", method, (unsigned)size);
        return;
    }

    memset(p, 0xAA, size);  /* touch memory */

    if (use_caps) {
        heap_caps_free(p);
    } else {
        vPortFree(p);
    }
    t2 = esp_timer_get_time();

    printf("  %s(%u) alloc=%lld us  free=%lld us  total=%lld us\n",
           method, (unsigned)size,
           (long long)(t1 - t0), (long long)(t2 - t1), (long long)(t2 - t0));
}

/* ------------------------------------------------------------------ */
/*  Fragmentation demo                                                */
/* ------------------------------------------------------------------ */
static void fragmentation_demo(void)
{
    void *blocks[NUM_BLOCKS] = {NULL};

    printf("\n===== FRAGMENTATION DEMO =====\n");
    print_heap_status("BEFORE alloc");

    /* Phase 1 — allocate 10 blocks */
    printf("\n[Phase 1] Allocating %d blocks of %d bytes\n", NUM_BLOCKS, BLOCK_SIZE);
    for (int i = 0; i < NUM_BLOCKS; i++) {
        blocks[i] = heap_caps_malloc(BLOCK_SIZE, MALLOC_CAP_DEFAULT);
        printf("  Block[%d] @ %p  %s\n", i, blocks[i], blocks[i] ? "OK" : "FAIL");
    }
    print_heap_status("AFTER alloc");

    /* Phase 2 — free odd-indexed blocks → create holes */
    printf("\n[Phase 2] Freeing ODD blocks (1,3,5,7,9) → creating holes\n");
    for (int i = 1; i < NUM_BLOCKS; i += 2) {
        heap_caps_free(blocks[i]);
        blocks[i] = NULL;
        printf("  Freed block[%d]\n", i);
    }
    print_heap_status("AFTER free-odd");

    /* Phase 3 — try to allocate large block */
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    printf("\n[Phase 3] Largest contiguous block: %u bytes\n", (unsigned)largest);
    printf("  Attempting to allocate %d bytes … ", LARGE_BLOCK);

    void *big = heap_caps_malloc(LARGE_BLOCK, MALLOC_CAP_DEFAULT);
    printf("%s\n", big ? "SUCCESS" : "FAILED (fragmentation!)");
    if (big) heap_caps_free(big);

    /* Cleanup */
    for (int i = 0; i < NUM_BLOCKS; i++) {
        if (blocks[i]) heap_caps_free(blocks[i]);
    }
    print_heap_status("AFTER cleanup");
    printf("==============================\n");
}

/* ------------------------------------------------------------------ */
/*  Main task                                                         */
/* ------------------------------------------------------------------ */
static void alloc_task(void *pv)
{
    /* Benchmark various sizes */
    size_t sizes[] = {32, 128, 512, 1024, 4096};
    int n = sizeof(sizes) / sizeof(sizes[0]);

    printf("\n===== ALLOCATION BENCHMARKS =====\n");
    for (int i = 0; i < n; i++) {
        printf("\n--- Size %u bytes ---\n", (unsigned)sizes[i]);
        benchmark_alloc("pvPortMalloc ", sizes[i], false);
        benchmark_alloc("heap_caps_mal", sizes[i], true);
    }
    printf("=================================\n");

    fragmentation_demo();

    /* Periodic repeat */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        fragmentation_demo();
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Memory Allocation Comparison ===");
    print_heap_status("INIT");

    xTaskCreate(alloc_task, "alloc_task", 4096, NULL, 5, NULL);
}
