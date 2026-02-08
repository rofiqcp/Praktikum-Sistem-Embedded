/**
 * ===========================================================================
 *  Program 10 : SPI Speed Benchmark
 *  Board      : ESP32 DevKit V1
 *  Framework  : ESP-IDF
 * ===========================================================================
 *
 *  Deskripsi:
 *  Program ini melakukan benchmark kecepatan SPI pada ESP32. Mengukur
 *  throughput aktual untuk berbagai kombinasi clock speed dan buffer size.
 *
 *  Fitur:
 *  1. Test 7 kecepatan SPI (1-40 MHz)
 *  2. Test 5 ukuran buffer (16-4096 bytes)
 *  3. Pengukuran throughput aktual (bytes/sec)
 *  4. Perhitungan efisiensi vs kecepatan teoretis
 *  5. DMA otomatis untuk transfer besar (>32 bytes)
 *  6. Tabel hasil terformat
 *  7. Ringkasan konfigurasi terbaik
 *
 *  Koneksi Hardware:
 *  - MOSI : GPIO 23
 *  - MISO : GPIO 19
 *  - SCLK : GPIO 18
 *  - CS   : GPIO 5
 *  (Hubungkan MOSI ke MISO untuk loopback test, atau gunakan slave device)
 *
 *  Catatan:
 *  - Untuk loopback test, hubungkan MOSI <-> MISO
 *  - Throughput aktual selalu < teoretis karena overhead software
 *  - DMA memberikan keuntungan signifikan untuk buffer besar
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "config.h"

static const char *TAG = "SPI_BENCH";

/* ==================== Result Storage ==================== */
typedef struct {
    int     speed_hz;
    int     buffer_size;
    int     iterations;
    int64_t total_time_us;
    double  throughput_bps;     // bytes per second
    double  throughput_mbps;    // megabits per second
    double  efficiency_pct;    // effective vs theoretical
    bool    dma_used;
} benchmark_result_t;

static benchmark_result_t results[NUM_SPEEDS * NUM_BUFFER_SIZES];
static int result_count = 0;

/* ==================== SPI Bus Management ==================== */

/**
 * @brief Initialize SPI bus
 */
static esp_err_t init_spi_bus(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = PIN_MISO,
        .sclk_io_num     = PIN_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

/**
 * @brief Add SPI device with specified clock speed
 */
static esp_err_t add_spi_device(int clock_hz, spi_device_handle_t *handle)
{
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = clock_hz,
        .mode           = 0,               // SPI mode 0 (CPOL=0, CPHA=0)
        .spics_io_num   = PIN_CS,
        .queue_size     = SPI_QUEUE_SIZE,
        .flags          = 0,
    };

    return spi_bus_add_device(SPI_HOST_ID, &dev_cfg, handle);
}

/**
 * @brief Run benchmark for a specific speed and buffer size
 */
static benchmark_result_t run_single_benchmark(spi_device_handle_t handle,
                                                int speed_hz, int buf_size)
{
    benchmark_result_t res = {
        .speed_hz    = speed_hz,
        .buffer_size = buf_size,
        .iterations  = NUM_ITERATIONS,
        .dma_used    = (buf_size > DMA_THRESHOLD),
    };

    // Allocate DMA-capable buffers
    uint8_t *tx_buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);

    if (!tx_buf || !rx_buf) {
        ESP_LOGE(TAG, "Failed to allocate DMA buffers for size %d", buf_size);
        if (tx_buf) free(tx_buf);
        if (rx_buf) free(rx_buf);
        return res;
    }

    // Fill TX buffer with test pattern
    for (int i = 0; i < buf_size; i++) {
        tx_buf[i] = (uint8_t)(i & 0xFF);
    }

    // Prepare transaction
    spi_transaction_t trans = {
        .length    = buf_size * 8,   // length in bits
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    // Warm up: a few transactions before timing
    for (int i = 0; i < 3; i++) {
        spi_device_transmit(handle, &trans);
    }

    // Timed benchmark
    int64_t start = esp_timer_get_time();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        esp_err_t ret = spi_device_transmit(handle, &trans);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmit failed at iter %d: %s", i, esp_err_to_name(ret));
            break;
        }
    }

    int64_t end = esp_timer_get_time();
    res.total_time_us = end - start;

    // Calculate throughput
    double total_bytes = (double)buf_size * NUM_ITERATIONS;
    double time_sec = (double)res.total_time_us / 1000000.0;

    res.throughput_bps  = total_bytes / time_sec;
    res.throughput_mbps = (res.throughput_bps * 8.0) / 1000000.0;

    // Theoretical max: speed_hz bits/sec -> speed_hz/8 bytes/sec
    double theoretical_bps = (double)speed_hz / 8.0;
    res.efficiency_pct = (res.throughput_bps / theoretical_bps) * 100.0;

    free(tx_buf);
    free(rx_buf);

    return res;
}

/**
 * @brief Print benchmark results table
 */
static void print_results_table(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║                              SPI SPEED BENCHMARK RESULTS                                     ║");
    ESP_LOGI(TAG, "╠════════╦════════╦═══════╦═════════════╦══════════════╦════════════╦══════════╦═══════════════╣");
    ESP_LOGI(TAG, "║ Speed  ║ BufSz  ║ Iters ║ Total(us)   ║ Tput(KB/s)   ║ Tput(Mbps) ║ Eff(%%)   ║ DMA          ║");
    ESP_LOGI(TAG, "╠════════╬════════╬═══════╬═════════════╬══════════════╬════════════╬══════════╬═══════════════╣");

    for (int i = 0; i < result_count; i++) {
        benchmark_result_t *r = &results[i];

        // Find speed label
        const char *slabel = "??";
        for (int s = 0; s < NUM_SPEEDS; s++) {
            if (test_speeds[s] == r->speed_hz) {
                slabel = speed_labels[s];
                break;
            }
        }

        ESP_LOGI(TAG, "║ %s ║ %5d  ║  %3d  ║ %9lld   ║ %10.1f   ║ %8.2f   ║ %6.1f   ║ %s           ║",
                 slabel, r->buffer_size, r->iterations,
                 r->total_time_us, r->throughput_bps / 1024.0,
                 r->throughput_mbps, r->efficiency_pct,
                 r->dma_used ? "Yes" : "No ");
    }

    ESP_LOGI(TAG, "╚════════╩════════╩═══════╩═════════════╩══════════════╩════════════╩══════════╩═══════════════╝");
}

/**
 * @brief Print summary with best configuration
 */
static void print_summary(void)
{
    double max_throughput = 0;
    int best_idx = 0;
    double max_efficiency = 0;
    int eff_idx = 0;

    for (int i = 0; i < result_count; i++) {
        if (results[i].throughput_bps > max_throughput) {
            max_throughput = results[i].throughput_bps;
            best_idx = i;
        }
        if (results[i].efficiency_pct > max_efficiency) {
            max_efficiency = results[i].efficiency_pct;
            eff_idx = i;
        }
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  BENCHMARK SUMMARY");
    ESP_LOGI(TAG, "========================================");

    if (result_count > 0) {
        ESP_LOGI(TAG, "  Total configurations tested: %d", result_count);
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "  BEST THROUGHPUT:");
        ESP_LOGI(TAG, "    Speed: %d Hz, Buffer: %d bytes",
                 results[best_idx].speed_hz, results[best_idx].buffer_size);
        ESP_LOGI(TAG, "    Throughput: %.1f KB/s (%.2f Mbps)",
                 max_throughput / 1024.0, results[best_idx].throughput_mbps);
        ESP_LOGI(TAG, "    Efficiency: %.1f%%", results[best_idx].efficiency_pct);
        ESP_LOGI(TAG, "    DMA: %s", results[best_idx].dma_used ? "Yes" : "No");
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "  BEST EFFICIENCY:");
        ESP_LOGI(TAG, "    Speed: %d Hz, Buffer: %d bytes",
                 results[eff_idx].speed_hz, results[eff_idx].buffer_size);
        ESP_LOGI(TAG, "    Efficiency: %.1f%%", max_efficiency);
        ESP_LOGI(TAG, "    Throughput: %.1f KB/s",
                 results[eff_idx].throughput_bps / 1024.0);
    }

    ESP_LOGI(TAG, "========================================");
}

/* ==================== Main Entry Point ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Program 10: SPI Speed Benchmark        ║");
    ESP_LOGI(TAG, "║   Modul 07 - SPI & Storage               ║");
    ESP_LOGI(TAG, "║   Framework: ESP-IDF                      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MOSI: GPIO%d, MISO: GPIO%d", PIN_MOSI, PIN_MISO);
    ESP_LOGI(TAG, "  SCLK: GPIO%d, CS: GPIO%d", PIN_SCLK, PIN_CS);
    ESP_LOGI(TAG, "  Speeds to test: %d", NUM_SPEEDS);
    ESP_LOGI(TAG, "  Buffer sizes: %d", NUM_BUFFER_SIZES);
    ESP_LOGI(TAG, "  Iterations per test: %d", NUM_ITERATIONS);
    ESP_LOGI(TAG, "  DMA threshold: %d bytes", DMA_THRESHOLD);
    ESP_LOGI(TAG, "");

    // Initialize SPI bus
    esp_err_t ret = init_spi_bus();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed! Halting.");
        return;
    }
    ESP_LOGI(TAG, "SPI bus initialized successfully");

    result_count = 0;

    // Run benchmarks for each speed
    for (int s = 0; s < NUM_SPEEDS; s++) {
        int speed = test_speeds[s];
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, ">>> Testing speed: %s (%d Hz) <<<", speed_labels[s], speed);

        // Add device with this speed
        spi_device_handle_t dev;
        ret = add_spi_device(speed, &dev);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add SPI device at %s: %s",
                     speed_labels[s], esp_err_to_name(ret));
            continue;
        }

        // Test each buffer size
        for (int b = 0; b < NUM_BUFFER_SIZES; b++) {
            int buf_size = buffer_sizes[b];
            ESP_LOGI(TAG, "  Buffer: %d bytes, DMA: %s ...",
                     buf_size, buf_size > DMA_THRESHOLD ? "Yes" : "No");

            benchmark_result_t res = run_single_benchmark(dev, speed, buf_size);
            results[result_count++] = res;

            ESP_LOGI(TAG, "    -> %.1f KB/s (%.2f Mbps), efficiency: %.1f%%, time: %lld us",
                     res.throughput_bps / 1024.0, res.throughput_mbps,
                     res.efficiency_pct, res.total_time_us);
        }

        // Remove device before reconfiguring
        spi_bus_remove_device(dev);
        ESP_LOGI(TAG, "  Device removed for reconfiguration");

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Print results
    print_results_table();
    print_summary();

    // Free SPI bus
    spi_bus_free(SPI_HOST_ID);
    ESP_LOGI(TAG, "SPI bus freed. Benchmark complete.");
}
