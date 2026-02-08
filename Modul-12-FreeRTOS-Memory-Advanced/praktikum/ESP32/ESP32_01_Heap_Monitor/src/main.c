/*
 * ESP32_01_Heap_Monitor
 * Monitor heap memory using FreeRTOS and ESP-IDF heap APIs.
 * Reports: free heap, minimum-ever free heap, internal/default caps, multi-heap info.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "HEAP_MON";

/* ------------------------------------------------------------------ */
/*  Print a detailed heap report                                      */
/* ------------------------------------------------------------------ */
static void print_heap_report(void)
{
    printf("\n========== HEAP MONITOR REPORT ==========\n");

    /* FreeRTOS portable API */
    size_t free_heap   = xPortGetFreeHeapSize();
    size_t min_ever    = xPortGetMinimumEverFreeHeapSize();

    printf("[FreeRTOS] Free heap            : %u bytes\n", (unsigned)free_heap);
    printf("[FreeRTOS] Min-ever free heap    : %u bytes\n", (unsigned)min_ever);

    /* ESP-IDF heap_caps API */
    size_t default_free  = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t dma_free      = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t largest_blk   = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    printf("[heap_caps] DEFAULT  free        : %u bytes\n", (unsigned)default_free);
    printf("[heap_caps] INTERNAL free        : %u bytes\n", (unsigned)internal_free);
    printf("[heap_caps] DMA      free        : %u bytes\n", (unsigned)dma_free);
    printf("[heap_caps] Largest free block    : %u bytes\n", (unsigned)largest_blk);

    /* ESP-IDF convenience */
    size_t esp_free = esp_get_free_heap_size();
    size_t esp_min  = esp_get_minimum_free_heap_size();

    printf("[esp_sys]  esp_get_free_heap     : %u bytes\n", (unsigned)esp_free);
    printf("[esp_sys]  esp_get_min_free_heap : %u bytes\n", (unsigned)esp_min);

    /* Multi-heap info */
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

    printf("\n--- Multi-heap capability info (DEFAULT) ---\n");
    printf("  Total free bytes           : %u\n", (unsigned)info.total_free_bytes);
    printf("  Total allocated bytes      : %u\n", (unsigned)info.total_allocated_bytes);
    printf("  Largest free block         : %u\n", (unsigned)info.largest_free_block);
    printf("  Minimum ever free bytes    : %u\n", (unsigned)info.minimum_free_bytes);
    printf("  Allocated blocks           : %u\n", (unsigned)info.allocated_blocks);
    printf("  Free blocks                : %u\n", (unsigned)info.free_blocks);
    printf("  Total blocks               : %u\n", (unsigned)info.total_blocks);

    /* SPIRAM / PSRAM check */
    size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (spiram_free > 0) {
        printf("\n[PSRAM] SPIRAM free            : %u bytes\n", (unsigned)spiram_free);
    } else {
        printf("\n[PSRAM] No SPIRAM detected.\n");
    }

    printf("==========================================\n\n");
}

/* ------------------------------------------------------------------ */
/*  Heap monitor task — runs every 2 s                                */
/* ------------------------------------------------------------------ */
static void heap_monitor_task(void *pvParam)
{
    uint32_t iteration = 0;
    void *test_allocs[5] = {NULL};

    while (1) {
        iteration++;
        printf(">>> Iteration %lu\n", (unsigned long)iteration);

        print_heap_report();

        /* Every 4th iteration allocate a small block to show changes */
        if (iteration % 4 == 0 && iteration <= 16) {
            int idx = (iteration / 4) - 1;
            if (idx < 5) {
                test_allocs[idx] = heap_caps_malloc(1024, MALLOC_CAP_DEFAULT);
                if (test_allocs[idx]) {
                    ESP_LOGI(TAG, "Allocated 1024 B test block #%d @ %p", idx, test_allocs[idx]);
                }
            }
        }
        /* Free them after iteration 20 */
        if (iteration == 20) {
            for (int i = 0; i < 5; i++) {
                if (test_allocs[i]) {
                    heap_caps_free(test_allocs[i]);
                    test_allocs[i] = NULL;
                    ESP_LOGI(TAG, "Freed test block #%d", i);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Heap Monitor Demo ===");
    ESP_LOGI(TAG, "Reporting heap statistics every 2 seconds");

    print_heap_report();  /* initial snapshot */

    xTaskCreate(heap_monitor_task, "heap_mon", 4096, NULL, 5, NULL);
}
