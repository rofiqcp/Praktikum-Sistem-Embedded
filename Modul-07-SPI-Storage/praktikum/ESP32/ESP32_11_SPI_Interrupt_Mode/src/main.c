/**
 * ===========================================================================
 *  Program 11 : SPI Interrupt (Non-blocking) Mode
 *  Board      : ESP32 DevKit V1
 *  Framework  : ESP-IDF
 * ===========================================================================
 *
 *  Deskripsi:
 *  Program ini mendemonstrasikan berbagai mode transfer SPI pada ESP32:
 *  blocking, non-blocking (queued/interrupt), dan polling. Termasuk
 *  penggunaan callback pre/post transaksi.
 *
 *  Fitur:
 *  1. Non-blocking transfer: spi_device_queue_trans() + get_trans_result()
 *  2. Pre/post transaction callbacks
 *  3. Demo 1: Queue multiple transactions, process as they complete
 *  4. Demo 2: Timing comparison blocking vs non-blocking
 *  5. Demo 3: Polling mode comparison
 *  6. Callback execution tracking
 *  7. Performance comparison table
 *
 *  Koneksi Hardware:
 *  - MOSI : GPIO 23 (hubungkan ke MISO untuk loopback)
 *  - MISO : GPIO 19
 *  - SCLK : GPIO 18
 *  - CS   : GPIO 5
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "config.h"

static const char *TAG = "SPI_INT";

/* ==================== Callback Counters ==================== */
static volatile int pre_cb_count = 0;
static volatile int post_cb_count = 0;

/* ==================== Callback Functions ==================== */

/**
 * @brief Pre-transaction callback (called from ISR context)
 *        Executed before each SPI transaction starts
 */
static void IRAM_ATTR spi_pre_transfer_callback(spi_transaction_t *trans)
{
    pre_cb_count++;
    // Can be used to set DC pin for displays, etc.
    // NOTE: This runs in ISR context - keep it minimal!
}

/**
 * @brief Post-transaction callback (called from ISR context)
 *        Executed after each SPI transaction completes
 */
static void IRAM_ATTR spi_post_transfer_callback(spi_transaction_t *trans)
{
    post_cb_count++;
}

/* ==================== SPI Initialization ==================== */

static spi_device_handle_t spi_dev;

/**
 * @brief Initialize SPI bus and add device with callbacks
 */
static esp_err_t init_spi(void)
{
    // Bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = PIN_MISO,
        .sclk_io_num     = PIN_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = TX_BUFFER_SIZE,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Device configuration with callbacks
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode           = 0,
        .spics_io_num   = PIN_CS,
        .queue_size     = QUEUE_SIZE,
        .pre_cb         = spi_pre_transfer_callback,
        .post_cb        = spi_post_transfer_callback,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

/* ==================== Demo 1: Queued Transactions ==================== */

/**
 * @brief Demo 1: Queue multiple transactions and process results as they complete
 */
static void demo_queued_transactions(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 1: Queued (Non-blocking) Transfers");
    ESP_LOGI(TAG, "========================================");

    // Allocate DMA-capable buffers for all transactions
    uint8_t *tx_bufs[NUM_TRANSACTIONS];
    uint8_t *rx_bufs[NUM_TRANSACTIONS];
    spi_transaction_t transactions[NUM_TRANSACTIONS];

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        tx_bufs[i] = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
        rx_bufs[i] = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);

        if (!tx_bufs[i] || !rx_bufs[i]) {
            ESP_LOGE(TAG, "Failed to allocate buffer %d", i);
            // Cleanup
            for (int j = 0; j <= i; j++) {
                if (tx_bufs[j]) free(tx_bufs[j]);
                if (rx_bufs[j]) free(rx_bufs[j]);
            }
            return;
        }

        // Fill TX buffer with pattern
        memset(tx_bufs[i], 0xA0 + i, TX_BUFFER_SIZE);
        memset(rx_bufs[i], 0, TX_BUFFER_SIZE);

        memset(&transactions[i], 0, sizeof(spi_transaction_t));
        transactions[i].length    = TX_BUFFER_SIZE * 8;  // bits
        transactions[i].tx_buffer = tx_bufs[i];
        transactions[i].rx_buffer = rx_bufs[i];
        transactions[i].user      = (void *)(intptr_t)i;  // Store index
    }

    // Reset callback counters
    pre_cb_count = 0;
    post_cb_count = 0;

    int64_t start = esp_timer_get_time();

    // Queue all transactions (non-blocking)
    ESP_LOGI(TAG, "Queuing %d transactions...", NUM_TRANSACTIONS);
    int queued = 0;
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        esp_err_t ret = spi_device_queue_trans(spi_dev, &transactions[i], pdMS_TO_TICKS(1000));
        if (ret == ESP_OK) {
            queued++;
        } else {
            ESP_LOGE(TAG, "Failed to queue transaction %d: %s", i, esp_err_to_name(ret));
        }
    }

    int64_t queue_time = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "Queued %d/%d transactions in %lld us", queued, NUM_TRANSACTIONS, queue_time);

    // Collect results as they complete
    ESP_LOGI(TAG, "Collecting results...");
    int completed = 0;
    for (int i = 0; i < queued; i++) {
        spi_transaction_t *ret_trans;
        esp_err_t ret = spi_device_get_trans_result(spi_dev, &ret_trans, pdMS_TO_TICKS(5000));
        if (ret == ESP_OK) {
            int idx = (int)(intptr_t)ret_trans->user;
            completed++;
            ESP_LOGI(TAG, "  Transaction %d completed (first byte RX: 0x%02X)", idx,
                     ((uint8_t *)ret_trans->rx_buffer)[0]);
        } else {
            ESP_LOGE(TAG, "  Failed to get result %d: %s", i, esp_err_to_name(ret));
        }
    }

    int64_t total_time = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "All %d transactions completed in %lld us", completed, total_time);
    ESP_LOGI(TAG, "Callbacks - Pre: %d, Post: %d", pre_cb_count, post_cb_count);

    // Cleanup
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        free(tx_bufs[i]);
        free(rx_bufs[i]);
    }
}

/* ==================== Demo 2: Blocking vs Non-blocking ==================== */

/**
 * @brief Demo 2: Compare blocking vs non-blocking timing
 */
static void demo_blocking_vs_nonblocking(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 2: Blocking vs Non-blocking");
    ESP_LOGI(TAG, "========================================");

    uint8_t *tx_buf = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);

    if (!tx_buf || !rx_buf) {
        ESP_LOGE(TAG, "Buffer allocation failed");
        if (tx_buf) free(tx_buf);
        if (rx_buf) free(rx_buf);
        return;
    }

    memset(tx_buf, 0xBB, TX_BUFFER_SIZE);

    spi_transaction_t trans = {
        .length    = TX_BUFFER_SIZE * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    // --- Test 1: Blocking mode (spi_device_transmit) ---
    pre_cb_count = 0;
    post_cb_count = 0;

    int64_t start = esp_timer_get_time();
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        memset(rx_buf, 0, TX_BUFFER_SIZE);
        spi_device_transmit(spi_dev, &trans);
    }
    int64_t blocking_time = esp_timer_get_time() - start;
    int blocking_pre = pre_cb_count;
    int blocking_post = post_cb_count;

    ESP_LOGI(TAG, "BLOCKING: %d transfers in %lld us (avg: %lld us/transfer)",
             NUM_TRANSACTIONS, blocking_time, blocking_time / NUM_TRANSACTIONS);

    // --- Test 2: Non-blocking mode (queue + get_result) ---
    pre_cb_count = 0;
    post_cb_count = 0;

    // Allocate separate buffers for queued transactions
    uint8_t *qtx_bufs[NUM_TRANSACTIONS];
    uint8_t *qrx_bufs[NUM_TRANSACTIONS];
    spi_transaction_t qtrans[NUM_TRANSACTIONS];

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        qtx_bufs[i] = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
        qrx_bufs[i] = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
        memset(qtx_bufs[i], 0xCC, TX_BUFFER_SIZE);
        memset(qrx_bufs[i], 0, TX_BUFFER_SIZE);
        memset(&qtrans[i], 0, sizeof(spi_transaction_t));
        qtrans[i].length    = TX_BUFFER_SIZE * 8;
        qtrans[i].tx_buffer = qtx_bufs[i];
        qtrans[i].rx_buffer = qrx_bufs[i];
    }

    start = esp_timer_get_time();

    // Queue all
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        spi_device_queue_trans(spi_dev, &qtrans[i], pdMS_TO_TICKS(1000));
    }
    int64_t queue_only_time = esp_timer_get_time() - start;

    // Collect all
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        spi_transaction_t *ret_trans;
        spi_device_get_trans_result(spi_dev, &ret_trans, pdMS_TO_TICKS(5000));
    }
    int64_t nonblocking_time = esp_timer_get_time() - start;
    int nonblocking_pre = pre_cb_count;
    int nonblocking_post = post_cb_count;

    ESP_LOGI(TAG, "NON-BLOCKING: %d transfers in %lld us (queue: %lld us, total: %lld us)",
             NUM_TRANSACTIONS, nonblocking_time, queue_only_time, nonblocking_time);

    // Cleanup queued buffers
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        free(qtx_bufs[i]);
        free(qrx_bufs[i]);
    }

    // --- Print Comparison ---
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════╦════════════╦════════════╦═══════════╗");
    ESP_LOGI(TAG, "║ Mode             ║ Total(us)  ║ Avg(us)    ║ Callbacks ║");
    ESP_LOGI(TAG, "╠══════════════════╬════════════╬════════════╬═══════════╣");
    ESP_LOGI(TAG, "║ Blocking         ║ %8lld   ║ %8lld   ║ %3d / %3d ║",
             blocking_time, blocking_time / NUM_TRANSACTIONS,
             blocking_pre, blocking_post);
    ESP_LOGI(TAG, "║ Non-blocking     ║ %8lld   ║ %8lld   ║ %3d / %3d ║",
             nonblocking_time, nonblocking_time / NUM_TRANSACTIONS,
             nonblocking_pre, nonblocking_post);
    ESP_LOGI(TAG, "╚══════════════════╩════════════╩════════════╩═══════════╝");

    double speedup = (double)blocking_time / (double)nonblocking_time;
    ESP_LOGI(TAG, "Speedup: %.2fx %s", speedup > 1.0 ? speedup : 1.0 / speedup,
             speedup > 1.0 ? "(non-blocking faster)" : "(blocking faster)");

    free(tx_buf);
    free(rx_buf);
}

/* ==================== Demo 3: Polling Mode ==================== */

/**
 * @brief Demo 3: Polling mode comparison
 */
static void demo_polling_mode(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 3: Polling Mode Transfer");
    ESP_LOGI(TAG, "========================================");

    uint8_t *tx_buf = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);

    if (!tx_buf || !rx_buf) {
        ESP_LOGE(TAG, "Buffer allocation failed");
        if (tx_buf) free(tx_buf);
        if (rx_buf) free(rx_buf);
        return;
    }

    memset(tx_buf, 0xDD, TX_BUFFER_SIZE);

    spi_transaction_t trans = {
        .length    = TX_BUFFER_SIZE * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    // Reset counters
    pre_cb_count = 0;
    post_cb_count = 0;

    // --- Polling mode ---
    int64_t start = esp_timer_get_time();
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        memset(rx_buf, 0, TX_BUFFER_SIZE);
        spi_device_polling_transmit(spi_dev, &trans);
    }
    int64_t polling_time = esp_timer_get_time() - start;
    int polling_pre = pre_cb_count;
    int polling_post = post_cb_count;

    ESP_LOGI(TAG, "POLLING: %d transfers in %lld us (avg: %lld us/transfer)",
             NUM_TRANSACTIONS, polling_time, polling_time / NUM_TRANSACTIONS);
    ESP_LOGI(TAG, "Polling callbacks - Pre: %d, Post: %d", polling_pre, polling_post);

    // --- Blocking mode for comparison ---
    pre_cb_count = 0;
    post_cb_count = 0;

    start = esp_timer_get_time();
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        memset(rx_buf, 0, TX_BUFFER_SIZE);
        spi_device_transmit(spi_dev, &trans);
    }
    int64_t blocking_time = esp_timer_get_time() - start;

    // --- Queued mode for comparison ---
    // Need separate buffers for queued transactions
    uint8_t *qtx_bufs[NUM_TRANSACTIONS];
    uint8_t *qrx_bufs[NUM_TRANSACTIONS];
    spi_transaction_t qtrans[NUM_TRANSACTIONS];

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        qtx_bufs[i] = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
        qrx_bufs[i] = heap_caps_malloc(TX_BUFFER_SIZE, MALLOC_CAP_DMA);
        memset(qtx_bufs[i], 0xEE, TX_BUFFER_SIZE);
        memset(qrx_bufs[i], 0, TX_BUFFER_SIZE);
        memset(&qtrans[i], 0, sizeof(spi_transaction_t));
        qtrans[i].length    = TX_BUFFER_SIZE * 8;
        qtrans[i].tx_buffer = qtx_bufs[i];
        qtrans[i].rx_buffer = qrx_bufs[i];
    }

    pre_cb_count = 0;
    post_cb_count = 0;

    start = esp_timer_get_time();
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        spi_device_queue_trans(spi_dev, &qtrans[i], pdMS_TO_TICKS(1000));
    }
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        spi_transaction_t *ret_trans;
        spi_device_get_trans_result(spi_dev, &ret_trans, pdMS_TO_TICKS(5000));
    }
    int64_t queued_time = esp_timer_get_time() - start;

    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        free(qtx_bufs[i]);
        free(qrx_bufs[i]);
    }

    // --- Final comparison table ---
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════╦════════════╦════════════╦═════════════════╗");
    ESP_LOGI(TAG, "║ Transfer Mode    ║ Total(us)  ║ Avg(us)    ║ Notes           ║");
    ESP_LOGI(TAG, "╠══════════════════╬════════════╬════════════╬═════════════════╣");
    ESP_LOGI(TAG, "║ Blocking         ║ %8lld   ║ %8lld   ║ Interrupt-based ║",
             blocking_time, blocking_time / NUM_TRANSACTIONS);
    ESP_LOGI(TAG, "║ Queued (async)   ║ %8lld   ║ %8lld   ║ Interrupt-based ║",
             queued_time, queued_time / NUM_TRANSACTIONS);
    ESP_LOGI(TAG, "║ Polling          ║ %8lld   ║ %8lld   ║ CPU busy-wait   ║",
             polling_time, polling_time / NUM_TRANSACTIONS);
    ESP_LOGI(TAG, "╚══════════════════╩════════════╩════════════╩═════════════════╝");

    // Determine fastest
    int64_t fastest = polling_time;
    const char *fastest_mode = "Polling";
    if (blocking_time < fastest) { fastest = blocking_time; fastest_mode = "Blocking"; }
    if (queued_time < fastest) { fastest = queued_time; fastest_mode = "Queued"; }

    ESP_LOGI(TAG, "Fastest mode: %s (%lld us)", fastest_mode, fastest);
    ESP_LOGI(TAG, "Note: Polling is fastest for small, frequent transfers");
    ESP_LOGI(TAG, "Note: Queued is best when CPU needs to do other work during transfer");

    free(tx_buf);
    free(rx_buf);
}

/* ==================== Main Entry Point ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Program 11: SPI Interrupt Mode         ║");
    ESP_LOGI(TAG, "║   Modul 07 - SPI & Storage               ║");
    ESP_LOGI(TAG, "║   Framework: ESP-IDF                      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "SPI Configuration:");
    ESP_LOGI(TAG, "  MOSI: GPIO%d, MISO: GPIO%d", PIN_MOSI, PIN_MISO);
    ESP_LOGI(TAG, "  SCLK: GPIO%d, CS: GPIO%d", PIN_SCLK, PIN_CS);
    ESP_LOGI(TAG, "  Clock: %d Hz (%d MHz)", SPI_CLOCK_HZ, SPI_CLOCK_HZ / 1000000);
    ESP_LOGI(TAG, "  Queue: %d, Transactions: %d", QUEUE_SIZE, NUM_TRANSACTIONS);
    ESP_LOGI(TAG, "  Buffer: %d bytes", TX_BUFFER_SIZE);

    // Initialize SPI
    esp_err_t ret = init_spi();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed! Halting.");
        return;
    }
    ESP_LOGI(TAG, "SPI initialized successfully with callbacks.");

    vTaskDelay(pdMS_TO_TICKS(DEMO_DELAY_MS));

    // Demo 1: Queued transactions
    demo_queued_transactions();
    vTaskDelay(pdMS_TO_TICKS(DEMO_DELAY_MS));

    // Demo 2: Blocking vs Non-blocking
    demo_blocking_vs_nonblocking();
    vTaskDelay(pdMS_TO_TICKS(DEMO_DELAY_MS));

    // Demo 3: Polling mode
    demo_polling_mode();

    // Final callback summary
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Total Callback Executions");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Pre-transfer callbacks:  %d", pre_cb_count);
    ESP_LOGI(TAG, "  Post-transfer callbacks: %d", post_cb_count);

    // Cleanup
    spi_bus_remove_device(spi_dev);
    spi_bus_free(SPI_HOST_ID);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  All SPI Interrupt demos completed!");
    ESP_LOGI(TAG, "========================================");
}
