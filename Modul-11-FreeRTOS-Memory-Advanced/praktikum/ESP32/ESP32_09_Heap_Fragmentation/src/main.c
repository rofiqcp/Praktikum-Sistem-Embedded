/*
 * ESP32_09_Heap_Fragmentation
 * Demonstrate heap fragmentation on ESP32.
 * Phase 1: allocate many small blocks.
 * Phase 2: free every other block (create holes).
 * Phase 3: try to allocate large block — may fail due to fragmentation.
 * Show heap_caps_get_largest_free_block() vs total free.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "HEAP_FRAG";

#define NUM_BLOCKS      50
#define SMALL_BLOCK     256
#define LARGE_BLOCK     8192

/* ------------------------------------------------------------------ */
static void print_frag_status(const char *label)
{
    size_t free_total = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t largest    = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    size_t min_ever   = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    float  frag_pct   = free_total > 0 ? (1.0f - (float)largest / (float)free_total) * 100.0f : 0;

    printf("[%s]\n", label);
    printf("  Total free     : %6u bytes\n", (unsigned)free_total);
    printf("  Largest block  : %6u bytes\n", (unsigned)largest);
    printf("  Min-ever free  : %6u bytes\n", (unsigned)min_ever);
    printf("  Fragmentation  : %5.1f %%\n", frag_pct);

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    printf("  Free blocks    : %u\n", (unsigned)info.free_blocks);
    printf("  Alloc blocks   : %u\n", (unsigned)info.allocated_blocks);
    printf("  Total blocks   : %u\n\n", (unsigned)info.total_blocks);
}

/* ------------------------------------------------------------------ */
static void fragmentation_task(void *pv)
{
    uint32_t round = 0;

    while (1) {
        round++;
        void *blocks[NUM_BLOCKS] = {NULL};

        printf("\n############## FRAGMENTATION ROUND %lu ##############\n\n",
               (unsigned long)round);

        /* ---- PHASE 0: Initial state ---- */
        print_frag_status("PHASE 0 — Initial");

        /* ---- PHASE 1: Allocate many small blocks ---- */
        printf(">>> PHASE 1: Allocating %d x %d byte blocks\n", NUM_BLOCKS, SMALL_BLOCK);
        int allocated = 0;
        for (int i = 0; i < NUM_BLOCKS; i++) {
            blocks[i] = heap_caps_malloc(SMALL_BLOCK, MALLOC_CAP_DEFAULT);
            if (blocks[i]) {
                memset(blocks[i], (uint8_t)i, SMALL_BLOCK);
                allocated++;
            }
        }
        printf("  Allocated: %d / %d blocks\n", allocated, NUM_BLOCKS);
        print_frag_status("PHASE 1 — After small allocs");

        vTaskDelay(pdMS_TO_TICKS(2000));

        /* ---- PHASE 2: Free every other block (create holes) ---- */
        printf(">>> PHASE 2: Freeing EVEN-indexed blocks (0,2,4,…)\n");
        int freed = 0;
        for (int i = 0; i < NUM_BLOCKS; i += 2) {
            if (blocks[i]) {
                heap_caps_free(blocks[i]);
                blocks[i] = NULL;
                freed++;
            }
        }
        printf("  Freed: %d blocks\n", freed);
        print_frag_status("PHASE 2 — After creating holes");

        vTaskDelay(pdMS_TO_TICKS(2000));

        /* ---- PHASE 3: Try large allocation ---- */
        size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        size_t total_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

        printf(">>> PHASE 3: Attempting large allocation of %d bytes\n", LARGE_BLOCK);
        printf("  Total free = %u  but largest contiguous = %u\n",
               (unsigned)total_free, (unsigned)largest);

        void *big = heap_caps_malloc(LARGE_BLOCK, MALLOC_CAP_DEFAULT);
        if (big) {
            printf("  RESULT: SUCCESS — large block allocated at %p\n", big);
            heap_caps_free(big);
        } else {
            printf("  RESULT: FAILED — fragmentation prevents allocation!\n");
            printf("  >>> This demonstrates the fragmentation problem <<<\n");
        }

        print_frag_status("PHASE 3 — After large alloc attempt");

        /* ---- Cleanup ---- */
        printf(">>> CLEANUP: Freeing remaining blocks\n");
        for (int i = 0; i < NUM_BLOCKS; i++) {
            if (blocks[i]) {
                heap_caps_free(blocks[i]);
                blocks[i] = NULL;
            }
        }
        print_frag_status("CLEANUP — All freed");

        printf("##############################################\n\n");

        vTaskDelay(pdMS_TO_TICKS(8000));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Heap Fragmentation Demo ===");
    ESP_LOGI(TAG, "Small block = %d B, Large block = %d B, Count = %d",
             SMALL_BLOCK, LARGE_BLOCK, NUM_BLOCKS);

    xTaskCreate(fragmentation_task, "frag_demo", 4096, NULL, 5, NULL);
}
