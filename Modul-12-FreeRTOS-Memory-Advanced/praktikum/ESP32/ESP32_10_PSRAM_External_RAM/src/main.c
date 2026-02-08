/*
 * ESP32_10_PSRAM_External_RAM
 * heap_caps_malloc(size, MALLOC_CAP_SPIRAM) for external PSRAM.
 * Even without PSRAM hardware, shows the API and fallback.
 * Compare DRAM vs SPIRAM speeds. Conditional code with CONFIG_SPIRAM.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "PSRAM";

/* ------------------------------------------------------------------ */
static void print_memory_regions(void)
{
    printf("\n===== MEMORY REGIONS =====\n");

    size_t dram_free   = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t dram_total  = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t dram_large  = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    printf("[DRAM / Internal]\n");
    printf("  Total    : %u bytes\n", (unsigned)dram_total);
    printf("  Free     : %u bytes\n", (unsigned)dram_free);
    printf("  Largest  : %u bytes\n", (unsigned)dram_large);

    size_t def_free  = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t def_total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    printf("\n[DEFAULT]\n");
    printf("  Total    : %u bytes\n", (unsigned)def_total);
    printf("  Free     : %u bytes\n", (unsigned)def_free);

    size_t dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
    printf("\n[DMA-capable]\n");
    printf("  Free     : %u bytes\n", (unsigned)dma_free);

    /* PSRAM / SPIRAM */
    size_t spiram_free  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t spiram_large = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    printf("\n[SPIRAM / PSRAM]\n");
#if CONFIG_SPIRAM
    printf("  CONFIG_SPIRAM: ENABLED\n");
#else
    printf("  CONFIG_SPIRAM: DISABLED (no PSRAM hardware or not configured)\n");
#endif
    printf("  Total    : %u bytes\n", (unsigned)spiram_total);
    printf("  Free     : %u bytes\n", (unsigned)spiram_free);
    printf("  Largest  : %u bytes\n", (unsigned)spiram_large);

    if (spiram_free == 0) {
        printf("  >>> No PSRAM available. SPIRAM allocations will fail.\n");
        printf("  >>> On ESP32-WROVER, enable CONFIG_SPIRAM in menuconfig.\n");
    }

    printf("===========================\n\n");
}

/* ------------------------------------------------------------------ */
/*  Speed benchmark: fill + read a buffer in DRAM vs SPIRAM           */
/* ------------------------------------------------------------------ */
static void benchmark_region(const char *name, uint32_t caps, size_t size)
{
    void *buf = heap_caps_malloc(size, caps);
    if (!buf) {
        printf("[BENCH] %s: allocation of %u bytes FAILED\n", name, (unsigned)size);
        return;
    }

    /* Write speed */
    int64_t t0 = esp_timer_get_time();
    memset(buf, 0xAA, size);
    int64_t t1 = esp_timer_get_time();

    /* Read speed (sum to prevent optimizer removal) */
    volatile uint32_t sum = 0;
    uint8_t *p = (uint8_t *)buf;
    int64_t t2 = esp_timer_get_time();
    for (size_t i = 0; i < size; i += 4) {
        sum += p[i];
    }
    int64_t t3 = esp_timer_get_time();

    double write_mbps = (double)size / (double)(t1 - t0);  /* bytes/us = MB/s */
    double read_mbps  = (double)size / (double)(t3 - t2);

    printf("[BENCH] %s %u B: write=%lld us (%.2f MB/s)  read=%lld us (%.2f MB/s)  sum=%lu\n",
           name, (unsigned)size,
           (long long)(t1 - t0), write_mbps,
           (long long)(t3 - t2), read_mbps,
           (unsigned long)sum);

    heap_caps_free(buf);
}

/* ------------------------------------------------------------------ */
static void psram_task(void *pv)
{
    while (1) {
        print_memory_regions();

        printf("===== SPEED BENCHMARKS =====\n");
        size_t test_sizes[] = {1024, 4096, 16384, 65536};
        int n = sizeof(test_sizes) / sizeof(test_sizes[0]);

        for (int i = 0; i < n; i++) {
            printf("\n--- %u bytes ---\n", (unsigned)test_sizes[i]);
            benchmark_region("DRAM   ", MALLOC_CAP_INTERNAL, test_sizes[i]);
            benchmark_region("SPIRAM ", MALLOC_CAP_SPIRAM,   test_sizes[i]);
        }

        /* Demonstrate PSRAM API even if it fails */
        printf("\n--- PSRAM-specific API demo ---\n");
        void *psram_buf = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
        if (psram_buf) {
            printf("  SPIRAM alloc 1024 B: SUCCESS @ %p\n", psram_buf);
            /* Check which capabilities the pointer has */
            printf("  heap_caps_check_integrity: %s\n",
                   heap_caps_check_integrity_all(true) ? "PASS" : "FAIL");
            heap_caps_free(psram_buf);
        } else {
            printf("  SPIRAM alloc 1024 B: FAILED (expected without PSRAM)\n");
            printf("  Fallback: using DRAM instead\n");
            void *dram_buf = heap_caps_malloc(1024, MALLOC_CAP_DEFAULT);
            if (dram_buf) {
                printf("  DRAM fallback alloc: SUCCESS @ %p\n", dram_buf);
                heap_caps_free(dram_buf);
            }
        }

        printf("============================\n\n");

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== PSRAM / External RAM Demo ===");
#if CONFIG_SPIRAM
    ESP_LOGI(TAG, "SPIRAM is ENABLED in config");
#else
    ESP_LOGW(TAG, "SPIRAM is NOT enabled — PSRAM allocations will fail");
    ESP_LOGW(TAG, "For ESP32-WROVER: enable CONFIG_SPIRAM in menuconfig");
#endif

    xTaskCreate(psram_task, "psram_demo", 4096, NULL, 5, NULL);
}
