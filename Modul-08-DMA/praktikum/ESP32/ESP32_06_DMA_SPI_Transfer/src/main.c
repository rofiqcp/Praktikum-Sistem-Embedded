/**
 * ===========================================================================
 *  PROGRAM 6: ESP32 SPI Transfer with DMA
 * ===========================================================================
 *
 *  DESKRIPSI:
 *  SPI DMA di ESP32 terintegrasi di peripheral SPI. Driver ESP-IDF
 *  secara otomatis menggunakan DMA untuk transfer besar dan CPU copy
 *  untuk transfer kecil (< 32 bytes).
 *
 *  Program ini:
 *    1. Membandingkan transfer berbagai ukuran (64-4096 bytes)
 *    2. Mengukur timing dan throughput per ukuran
 *    3. Menunjukkan DMA advantage: di atas threshold, DMA lebih efisien
 *    4. Full-duplex transfer: TX dan RX simultan via DMA
 *    5. Analisis crossover point DMA vs CPU copy
 *
 *  CATATAN: Wire MOSI (GPIO23) ke MISO (GPIO19) untuk loopback test.
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
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "config.h"

static const char *TAG = "SPI_DMA";

static spi_device_handle_t spi_dev = NULL;

/* Main transfer sizes */
static const size_t transfer_sizes[NUM_TRANSFER_SIZES] = {
    TRANSFER_SIZE_0, TRANSFER_SIZE_1, TRANSFER_SIZE_2, TRANSFER_SIZE_3
};

/* Crossover analysis: finer grain sizes around threshold */
static const size_t crossover_sizes[NUM_CROSSOVER_SIZES] = {
    8, 16, 32, 48, 64, 128, 256, 512
};

/* Result storage */
typedef struct {
    size_t size;
    int64_t time_us;
    double throughput_mbps;
    double throughput_mhz;  /* in terms of SPI clock efficiency */
    bool valid;
} spi_result_t;

/* ========================= SPI Initialization ========================= */
static esp_err_t spi_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MAX_TRANSFER_SIZE,
    };

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,                          /* CPOL=0, CPHA=0 */
        .spics_io_num = PIN_CS,
        .queue_size = 1,
        .flags = 0,
    };

    esp_err_t ret;

    /* Initialize SPI bus with DMA */
    ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI initialized:");
    ESP_LOGI(TAG, "  Host: SPI2, Clock: %d MHz", SPI_CLOCK_HZ / 1000000);
    ESP_LOGI(TAG, "  MOSI=GPIO%d, MISO=GPIO%d, SCLK=GPIO%d, CS=GPIO%d",
             PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS);
    ESP_LOGI(TAG, "  DMA: AUTO (SPI_DMA_CH_AUTO)");
    ESP_LOGI(TAG, "  Max transfer: %d bytes", MAX_TRANSFER_SIZE);

    return ESP_OK;
}

/* ========================= Fill Pattern ========================= */
static void fill_pattern(uint8_t *buf, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        buf[i] = (uint8_t)((i * 7 + 0x5A) & 0xFF);
    }
}

/* ========================= SPI Transfer Benchmark ========================= */
static spi_result_t benchmark_spi_transfer(size_t size, bool full_duplex)
{
    spi_result_t result = {0};
    result.size = size;
    result.valid = false;

    /* Allocate DMA-capable buffers */
    uint8_t *tx_buf = heap_caps_malloc(size, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_malloc(size, MALLOC_CAP_DMA);

    if (!tx_buf || !rx_buf) {
        ESP_LOGW(TAG, "DMA malloc failed for size %u", (unsigned)size);
        heap_caps_free(tx_buf);
        heap_caps_free(rx_buf);
        return result;
    }

    fill_pattern(tx_buf, size);
    memset(rx_buf, 0, size);

    /* Warm up */
    spi_transaction_t warmup = {
        .length = size * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = full_duplex ? rx_buf : NULL,
    };
    spi_device_transmit(spi_dev, &warmup);

    /* Benchmark */
    int64_t start = esp_timer_get_time();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        spi_transaction_t trans = {
            .length = size * 8,             /* Transfer length in bits */
            .tx_buffer = tx_buf,
            .rx_buffer = full_duplex ? rx_buf : NULL,
        };

        esp_err_t ret = spi_device_transmit(spi_dev, &trans);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
            heap_caps_free(tx_buf);
            heap_caps_free(rx_buf);
            return result;
        }
    }

    int64_t elapsed = esp_timer_get_time() - start;

    /* Calculate throughput */
    double total_bytes = (double)size * NUM_ITERATIONS;
    if (full_duplex) total_bytes *= 2; /* TX + RX */
    double elapsed_s = elapsed / 1e6;
    double mbps = (total_bytes / (1024.0 * 1024.0)) / elapsed_s;

    /* SPI clock efficiency */
    double theoretical_mbps = SPI_CLOCK_HZ / (8.0 * 1024 * 1024);
    double efficiency = (mbps / theoretical_mbps) * 100.0;

    /* Verify loopback (only valid if MOSI wired to MISO) */
    bool loopback_ok = (memcmp(tx_buf, rx_buf, size) == 0);

    result.size = size;
    result.time_us = elapsed;
    result.throughput_mbps = mbps;
    result.throughput_mhz = efficiency;
    result.valid = true;

    const char *mode = full_duplex ? "FullDuplex" : "TX_Only";
    const char *dma_note = (size <= CPU_COPY_THRESHOLD) ? "CPU_COPY" : "DMA";

    printf("[SPI_BENCH] Mode=%s Size=%u DMA=%s Time=%lld us Throughput=%.2f MB/s "
           "Efficiency=%.1f%% Loopback=%s\n",
           mode, (unsigned)size, dma_note, (long long)elapsed,
           mbps, efficiency,
           full_duplex ? (loopback_ok ? "OK" : "N/A(no wire?)") : "N/A");

    heap_caps_free(tx_buf);
    heap_caps_free(rx_buf);

    return result;
}

/* ========================= Run All Benchmarks ========================= */
static void run_benchmarks(void)
{
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, " ESP32 SPI Transfer with DMA — Benchmark");
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, " SPI Clock: %d MHz", SPI_CLOCK_HZ / 1000000);
    ESP_LOGI(TAG, " DMA: SPI_DMA_CH_AUTO (terintegrasi di SPI peripheral)");
    ESP_LOGI(TAG, " Iterations: %d per test", NUM_ITERATIONS);
    ESP_LOGI(TAG, " CPU copy threshold: ~%d bytes", CPU_COPY_THRESHOLD);
    ESP_LOGI(TAG, " Theoretical max: %.2f MB/s (full clock)",
             (double)SPI_CLOCK_HZ / (8.0 * 1024 * 1024));
    ESP_LOGI(TAG, "============================================================");

    spi_result_t main_results[NUM_TRANSFER_SIZES];
    spi_result_t duplex_results[NUM_TRANSFER_SIZES];

    printf("\n=== TX-Only Transfers ===\n");
    for (int i = 0; i < NUM_TRANSFER_SIZES; i++) {
        main_results[i] = benchmark_spi_transfer(transfer_sizes[i], false);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    printf("\n=== Full-Duplex Transfers (TX + RX via DMA) ===\n");
    for (int i = 0; i < NUM_TRANSFER_SIZES; i++) {
        duplex_results[i] = benchmark_spi_transfer(transfer_sizes[i], true);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* Crossover analysis */
    printf("\n=== DMA Crossover Analysis ===\n");
    printf("(Fine-grained sizes around CPU/DMA threshold)\n\n");
    spi_result_t crossover_results[NUM_CROSSOVER_SIZES];
    for (int i = 0; i < NUM_CROSSOVER_SIZES; i++) {
        crossover_results[i] = benchmark_spi_transfer(crossover_sizes[i], true);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* ========================= Summary Table ========================= */
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║            ESP32 SPI DMA Benchmark — Summary                           ║\n");
    printf("╠══════════╦═══════════════╦═══════════════╦══════════════════════════════╣\n");
    printf("║ Size (B) ║  TX-Only MB/s ║ Full-Dup MB/s ║ DMA Mode                     ║\n");
    printf("╠══════════╬═══════════════╬═══════════════╬══════════════════════════════╣\n");

    for (int i = 0; i < NUM_TRANSFER_SIZES; i++) {
        const char *mode = (transfer_sizes[i] <= CPU_COPY_THRESHOLD) ? "CPU Copy" : "DMA";
        printf("║ %8u ║", (unsigned)transfer_sizes[i]);
        if (main_results[i].valid) {
            printf(" %11.2f   ║", main_results[i].throughput_mbps);
        } else {
            printf("     N/A       ║");
        }
        if (duplex_results[i].valid) {
            printf(" %11.2f   ║", duplex_results[i].throughput_mbps);
        } else {
            printf("     N/A       ║");
        }
        printf(" %-28s ║\n", mode);
    }

    printf("╚══════════╩═══════════════╩═══════════════╩══════════════════════════════╝\n");

    /* Crossover summary */
    printf("\n  DMA Crossover Analysis:\n");
    printf("  %-10s  %-12s  %-12s\n", "Size", "Throughput", "DMA/CPU");
    printf("  %-10s  %-12s  %-12s\n", "--------", "----------", "--------");
    for (int i = 0; i < NUM_CROSSOVER_SIZES; i++) {
        if (crossover_results[i].valid) {
            const char *mode = (crossover_sizes[i] <= CPU_COPY_THRESHOLD) ? "CPU" : "DMA";
            printf("  %-10u  %8.2f MB/s  %s\n",
                   (unsigned)crossover_sizes[i],
                   crossover_results[i].throughput_mbps,
                   mode);
        }
    }

    printf("\n");
    printf("[INFO] ESP32 SPI DMA Architecture:\n");
    printf("  - DMA terintegrasi di SPI peripheral (bukan DMA controller terpisah)\n");
    printf("  - Transfer < %d bytes: CPU copy (lebih cepat untuk data kecil)\n",
           CPU_COPY_THRESHOLD);
    printf("  - Transfer >= %d bytes: DMA (lebih efisien untuk data besar)\n",
           CPU_COPY_THRESHOLD);
    printf("  - SPI_DMA_CH_AUTO: driver otomatis memilih DMA channel\n");
    printf("  - Buffer harus DMA-capable: heap_caps_malloc(size, MALLOC_CAP_DMA)\n");
    printf("  - Full-duplex: TX dan RX simultan melalui DMA\n");
}

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 SPI Transfer with DMA");
    ESP_LOGI(TAG, " Modul 08 — DMA (SPI)");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Perbandingan DMA ESP32 vs STM32:");
    ESP_LOGI(TAG, "  STM32: SPI → DMA Channel → Memory (konfigurasi manual)");
    ESP_LOGI(TAG, "  ESP32: SPI + DMA terintegrasi (otomatis via driver)");
    ESP_LOGI(TAG, "  Kedua approach mencapai hasil yang sama:");
    ESP_LOGI(TAG, "  transfer data tanpa CPU intervention.");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "CATATAN: Wire MOSI (GPIO%d) ke MISO (GPIO%d) untuk loopback!",
             PIN_MOSI, PIN_MISO);

    esp_err_t ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI init failed! Aborting.");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    run_benchmarks();

    ESP_LOGI(TAG, "Benchmark complete.");

    /* Cleanup */
    if (spi_dev) {
        spi_bus_remove_device(spi_dev);
        spi_dev = NULL;
    }
    spi_bus_free(SPI_HOST_ID);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
