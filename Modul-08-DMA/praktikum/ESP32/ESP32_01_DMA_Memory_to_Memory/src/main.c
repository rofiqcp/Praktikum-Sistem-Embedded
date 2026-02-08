/**
 * ===========================================================================
 *  PROGRAM 1: ESP32 Memory Copy Benchmark
 * ===========================================================================
 *
 *  JUDUL : "ESP32 Memory Copy Benchmark — ESP32 tidak memiliki DMA M2M
 *           seperti STM32"
 *
 *  DESKRIPSI:
 *  ESP32 menggunakan DMA yang terintegrasi di peripheral, bukan DMA
 *  controller terpisah seperti pada STM32 (DMA1/DMA2). Oleh karena itu,
 *  untuk transfer Memory-to-Memory, tidak ada dedicated DMA channel.
 *
 *  Program ini membandingkan beberapa metode copy memory:
 *    1. Simple for-loop byte copy
 *    2. memcpy() — dioptimasi oleh compiler/libc
 *    3. memcpy dengan buffer IRAM (heap_caps_malloc + MALLOC_CAP_INTERNAL)
 *    4. memcpy dengan SPIRAM/PSRAM (jika tersedia)
 *    5. DMA via SPI loopback (MOSI→MISO di-wire, transfer DMA nyata)
 *
 *  Setiap metode diukur untuk berbagai ukuran buffer, dihitung throughput
 *  dalam MB/s, dan ditampilkan tabel perbandingan.
 *
 *  PLATFORM : ESP32 (ESP-IDF / PlatformIO)
 *  AUTHOR   : Praktikum Sistem Embedded
 * ===========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "config.h"

static const char *TAG = "MEM_BENCH";

/* Buffer sizes to test */
static const size_t buffer_sizes[NUM_BUFFER_SIZES] = {
    BUFFER_SIZE_0, BUFFER_SIZE_1, BUFFER_SIZE_2, BUFFER_SIZE_3, BUFFER_SIZE_4
};

/* Result storage */
typedef struct {
    double throughput_mbps;
    int64_t time_us;
    bool available;
} bench_result_t;

static bench_result_t results[5][NUM_BUFFER_SIZES]; /* 5 methods x NUM_BUFFER_SIZES */

/* Method names */
static const char *method_names[] = {
    "For-Loop Byte Copy",
    "memcpy (Default)",
    "memcpy (IRAM Buffer)",
    "memcpy (SPIRAM)",
    "SPI DMA Loopback"
};

/* ========================= METHOD 1: For-loop byte copy ========================= */
static void bench_forloop_copy(uint8_t *dst, const uint8_t *src, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
}

/* ========================= METHOD 5: SPI DMA Loopback ========================= */
static spi_device_handle_t spi_dev = NULL;

static esp_err_t spi_dma_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_TRANSFER,
    };

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };

    esp_err_t ret = spi_bus_initialize(SPI_DMA_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    ret = spi_bus_add_device(SPI_DMA_HOST, &dev_cfg, &spi_dev);
    return ret;
}

static esp_err_t spi_dma_transfer(uint8_t *tx_buf, uint8_t *rx_buf, size_t size)
{
    spi_transaction_t trans = {
        .length = size * 8,     /* bits */
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };
    return spi_device_transmit(spi_dev, &trans);
}

static void spi_dma_deinit(void)
{
    if (spi_dev) {
        spi_bus_remove_device(spi_dev);
        spi_dev = NULL;
    }
    spi_bus_free(SPI_DMA_HOST);
}

/* ========================= Fill buffer with pattern ========================= */
static void fill_pattern(uint8_t *buf, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        buf[i] = (uint8_t)(i & 0xFF);
    }
}

/* ========================= Verify copy correctness ========================= */
static bool verify_copy(const uint8_t *dst, const uint8_t *src, size_t size)
{
    return (memcmp(dst, src, size) == 0);
}

/* ========================= Run Benchmark ========================= */
static void run_benchmarks(void)
{
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, " ESP32 Memory Copy Benchmark");
    ESP_LOGI(TAG, " ESP32 tidak memiliki DMA M2M seperti STM32");
    ESP_LOGI(TAG, " ESP32 menggunakan DMA yang terintegrasi di peripheral,");
    ESP_LOGI(TAG, " bukan DMA controller terpisah.");
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, " Iterations per test: %d", NUM_ITERATIONS);
    ESP_LOGI(TAG, "============================================================");

    /* Initialize SPI DMA */
    bool spi_available = (spi_dma_init() == ESP_OK);
    if (spi_available) {
        ESP_LOGI(TAG, "SPI DMA loopback initialized (wire MOSI→MISO for actual DMA)");
    } else {
        ESP_LOGW(TAG, "SPI DMA init failed — skipping SPI loopback benchmark");
    }

    /* Check SPIRAM availability */
    bool spiram_available = (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0);
    ESP_LOGI(TAG, "SPIRAM available: %s", spiram_available ? "YES" : "NO");

    printf("\n");

    for (int s = 0; s < NUM_BUFFER_SIZES; s++) {
        size_t size = buffer_sizes[s];
        ESP_LOGI(TAG, "--- Buffer Size: %u bytes ---", (unsigned)size);

        /* ---- Method 1: For-loop byte copy ---- */
        {
            uint8_t *src = malloc(size);
            uint8_t *dst = malloc(size);
            if (src && dst) {
                fill_pattern(src, size);
                memset(dst, 0, size);

                int64_t start = esp_timer_get_time();
                for (int i = 0; i < NUM_ITERATIONS; i++) {
                    bench_forloop_copy(dst, src, size);
                }
                int64_t elapsed = esp_timer_get_time() - start;

                bool ok = verify_copy(dst, src, size);
                double total_bytes = (double)size * NUM_ITERATIONS;
                double mbps = (total_bytes / (1024.0 * 1024.0)) / (elapsed / 1e6);

                results[0][s].throughput_mbps = mbps;
                results[0][s].time_us = elapsed;
                results[0][s].available = true;

                printf("[BENCH] Method=ForLoop Size=%u Time=%lld us Throughput=%.2f MB/s Verify=%s\n",
                       (unsigned)size, (long long)elapsed, mbps, ok ? "OK" : "FAIL");
            } else {
                results[0][s].available = false;
                ESP_LOGW(TAG, "ForLoop: malloc failed for size %u", (unsigned)size);
            }
            free(src);
            free(dst);
        }

        /* ---- Method 2: memcpy (default heap) ---- */
        {
            uint8_t *src = malloc(size);
            uint8_t *dst = malloc(size);
            if (src && dst) {
                fill_pattern(src, size);
                memset(dst, 0, size);

                int64_t start = esp_timer_get_time();
                for (int i = 0; i < NUM_ITERATIONS; i++) {
                    memcpy(dst, src, size);
                }
                int64_t elapsed = esp_timer_get_time() - start;

                bool ok = verify_copy(dst, src, size);
                double total_bytes = (double)size * NUM_ITERATIONS;
                double mbps = (total_bytes / (1024.0 * 1024.0)) / (elapsed / 1e6);

                results[1][s].throughput_mbps = mbps;
                results[1][s].time_us = elapsed;
                results[1][s].available = true;

                printf("[BENCH] Method=Memcpy Size=%u Time=%lld us Throughput=%.2f MB/s Verify=%s\n",
                       (unsigned)size, (long long)elapsed, mbps, ok ? "OK" : "FAIL");
            } else {
                results[1][s].available = false;
            }
            free(src);
            free(dst);
        }

        /* ---- Method 3: memcpy with IRAM buffers ---- */
        {
            uint8_t *src = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            uint8_t *dst = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            if (src && dst) {
                fill_pattern(src, size);
                memset(dst, 0, size);

                int64_t start = esp_timer_get_time();
                for (int i = 0; i < NUM_ITERATIONS; i++) {
                    memcpy(dst, src, size);
                }
                int64_t elapsed = esp_timer_get_time() - start;

                bool ok = verify_copy(dst, src, size);
                double total_bytes = (double)size * NUM_ITERATIONS;
                double mbps = (total_bytes / (1024.0 * 1024.0)) / (elapsed / 1e6);

                results[2][s].throughput_mbps = mbps;
                results[2][s].time_us = elapsed;
                results[2][s].available = true;

                printf("[BENCH] Method=IRAM Size=%u Time=%lld us Throughput=%.2f MB/s Verify=%s\n",
                       (unsigned)size, (long long)elapsed, mbps, ok ? "OK" : "FAIL");
            } else {
                results[2][s].available = false;
                ESP_LOGW(TAG, "IRAM: heap_caps_malloc failed for size %u", (unsigned)size);
            }
            heap_caps_free(src);
            heap_caps_free(dst);
        }

        /* ---- Method 4: memcpy with SPIRAM ---- */
        {
            if (spiram_available) {
                uint8_t *src = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
                uint8_t *dst = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
                if (src && dst) {
                    fill_pattern(src, size);
                    memset(dst, 0, size);

                    int64_t start = esp_timer_get_time();
                    for (int i = 0; i < NUM_ITERATIONS; i++) {
                        memcpy(dst, src, size);
                    }
                    int64_t elapsed = esp_timer_get_time() - start;

                    bool ok = verify_copy(dst, src, size);
                    double total_bytes = (double)size * NUM_ITERATIONS;
                    double mbps = (total_bytes / (1024.0 * 1024.0)) / (elapsed / 1e6);

                    results[3][s].throughput_mbps = mbps;
                    results[3][s].time_us = elapsed;
                    results[3][s].available = true;

                    printf("[BENCH] Method=SPIRAM Size=%u Time=%lld us Throughput=%.2f MB/s Verify=%s\n",
                           (unsigned)size, (long long)elapsed, mbps, ok ? "OK" : "FAIL");
                } else {
                    results[3][s].available = false;
                    ESP_LOGW(TAG, "SPIRAM: heap_caps_malloc failed for size %u", (unsigned)size);
                }
                heap_caps_free(src);
                heap_caps_free(dst);
            } else {
                results[3][s].available = false;
                printf("[BENCH] Method=SPIRAM Size=%u SKIPPED (no PSRAM)\n", (unsigned)size);
            }
        }

        /* ---- Method 5: SPI DMA loopback ---- */
        {
            if (spi_available && size <= SPI_MAX_TRANSFER) {
                /* DMA-capable buffers required */
                uint8_t *tx = heap_caps_malloc(size, MALLOC_CAP_DMA);
                uint8_t *rx = heap_caps_malloc(size, MALLOC_CAP_DMA);
                if (tx && rx) {
                    fill_pattern(tx, size);
                    memset(rx, 0, size);

                    int64_t start = esp_timer_get_time();
                    for (int i = 0; i < NUM_ITERATIONS; i++) {
                        spi_dma_transfer(tx, rx, size);
                    }
                    int64_t elapsed = esp_timer_get_time() - start;

                    /*
                     * CATATAN: Verifikasi hanya valid jika MOSI dan MISO di-wire
                     * (loopback). Tanpa loopback, rx_buf akan berisi 0x00/0xFF.
                     */
                    bool ok = verify_copy(rx, tx, size);
                    double total_bytes = (double)size * NUM_ITERATIONS;
                    double mbps = (total_bytes / (1024.0 * 1024.0)) / (elapsed / 1e6);

                    results[4][s].throughput_mbps = mbps;
                    results[4][s].time_us = elapsed;
                    results[4][s].available = true;

                    printf("[BENCH] Method=SPI_DMA Size=%u Time=%lld us Throughput=%.2f MB/s Verify=%s\n",
                           (unsigned)size, (long long)elapsed, mbps,
                           ok ? "OK" : "N/A(no loopback?)");
                } else {
                    results[4][s].available = false;
                    ESP_LOGW(TAG, "SPI_DMA: DMA malloc failed for size %u", (unsigned)size);
                }
                heap_caps_free(tx);
                heap_caps_free(rx);
            } else if (!spi_available) {
                results[4][s].available = false;
                printf("[BENCH] Method=SPI_DMA Size=%u SKIPPED (SPI init failed)\n",
                       (unsigned)size);
            } else {
                results[4][s].available = false;
                printf("[BENCH] Method=SPI_DMA Size=%u SKIPPED (exceeds max %d)\n",
                       (unsigned)size, SPI_MAX_TRANSFER);
            }
        }

        printf("\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* ========================= Print Summary Table ========================= */
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║              ESP32 Memory Copy Benchmark — Summary (MB/s)              ║\n");
    printf("╠═══════════════════╦══════════╦══════════╦══════════╦══════════╦═════════╣\n");
    printf("║ Method            ║  256 B   ║  1 KB    ║  4 KB    ║  16 KB   ║ 64 KB   ║\n");
    printf("╠═══════════════════╬══════════╬══════════╬══════════╬══════════╬═════════╣\n");

    for (int m = 0; m < 5; m++) {
        printf("║ %-17s ║", method_names[m]);
        for (int s = 0; s < NUM_BUFFER_SIZES; s++) {
            if (results[m][s].available) {
                printf(" %7.2f ║", results[m][s].throughput_mbps);
            } else {
                printf("   N/A   ║");
            }
        }
        printf("\n");
    }

    printf("╚═══════════════════╩══════════╩══════════╩══════════╩══════════╩═════════╝\n");
    printf("\n");
    printf("[INFO] ESP32 DMA Architecture:\n");
    printf("  - ESP32 menggunakan DMA yang terintegrasi di peripheral\n");
    printf("  - Tidak ada DMA controller terpisah seperti STM32 DMA1/DMA2\n");
    printf("  - Untuk M2M copy: gunakan CPU (memcpy) yang sudah dioptimasi\n");
    printf("  - DMA hanya tersedia melalui SPI, I2S, UART, ADC peripheral\n");
    printf("  - SPI DMA loopback menunjukkan overhead DMA setup vs CPU copy\n");

    /* Cleanup */
    if (spi_available) {
        spi_dma_deinit();
    }
}

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 Memory Copy Benchmark");
    ESP_LOGI(TAG, " Modul 08 — DMA (Memory-to-Memory)");
    ESP_LOGI(TAG, "========================================");

    /* Print memory info */
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "Internal free: %lu bytes",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "SPIRAM free: %lu bytes",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    printf("\n");
    run_benchmarks();

    ESP_LOGI(TAG, "Benchmark complete. Restarting in 30 seconds...");
    vTaskDelay(pdMS_TO_TICKS(30000));
    esp_restart();
}
