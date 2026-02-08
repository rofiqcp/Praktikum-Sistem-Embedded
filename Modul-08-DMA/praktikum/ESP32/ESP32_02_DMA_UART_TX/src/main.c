/**
 * ===========================================================================
 *  PROGRAM 2: ESP32 UART TX with Driver-Buffered Transmission
 * ===========================================================================
 *
 *  DESKRIPSI:
 *  ESP-IDF UART driver menggunakan interrupt+FIFO, bukan DMA channel
 *  terpisah. Driver secara internal mengelola ring buffer dan interrupt
 *  untuk transfer data yang efisien.
 *
 *  Program ini membandingkan metode transmisi:
 *    1. uart_write_bytes() — driver-buffered (internal DMA/FIFO)
 *    2. uart_tx_chars() — byte-by-byte via FIFO
 *    3. printf via UART0 (baseline comparison)
 *
 *  Setiap metode diukur untuk beberapa ukuran data, throughput dihitung,
 *  dan ditampilkan tabel perbandingan.
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
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_TX";

/* Test data sizes */
static const size_t test_sizes[NUM_TEST_SIZES] = {
    TEST_SIZE_SMALL, TEST_SIZE_MEDIUM, TEST_SIZE_LARGE
};

/* Result storage */
typedef struct {
    double throughput_bps;    /* bits per second */
    double throughput_kbps;   /* kilobits per second */
    int64_t time_us;
    size_t data_size;
    bool valid;
} tx_result_t;

static tx_result_t results[3][NUM_TEST_SIZES]; /* 3 methods x 3 sizes */

static const char *method_names[] = {
    "uart_write_bytes",
    "uart_tx_chars",
    "printf (UART0)"
};

/* ========================= UART Initialization ========================= */
static esp_err_t uart1_init(void)
{
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret;

    ret = uart_driver_install(UART_PORT_NUM, UART_RX_BUF_SIZE * 2,
                              UART_TX_BUF_SIZE * 2, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(UART_PORT_NUM, &uart_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "UART%d initialized: TX=GPIO%d, RX=GPIO%d, Baud=%d",
             UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);

    return ESP_OK;
}

/* ========================= Generate Test Data ========================= */
static uint8_t *generate_test_data(size_t size)
{
    uint8_t *data = malloc(size);
    if (!data) return NULL;

    for (size_t i = 0; i < size; i++) {
        /* Printable ASCII pattern for easy debug */
        data[i] = 0x30 + (i % 10); /* '0'-'9' repeating */
    }
    return data;
}

/* ========================= Method 1: uart_write_bytes ========================= */
static void bench_write_bytes(const uint8_t *data, size_t size, int size_idx)
{
    ESP_LOGI(TAG, "  Method 1: uart_write_bytes() — %u bytes x %d iter",
             (unsigned)size, NUM_ITERATIONS);

    int64_t total_time = 0;
    size_t total_sent = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int64_t start = esp_timer_get_time();
        int sent = uart_write_bytes(UART_PORT_NUM, data, size);
        /* Wait for TX FIFO to complete */
        uart_wait_tx_done(UART_PORT_NUM, pdMS_TO_TICKS(1000));
        int64_t elapsed = esp_timer_get_time() - start;

        total_time += elapsed;
        if (sent > 0) total_sent += sent;
    }

    double total_bits = (double)total_sent * 8.0;
    double time_sec = total_time / 1e6;
    double bps = total_bits / time_sec;

    results[0][size_idx].throughput_bps = bps;
    results[0][size_idx].throughput_kbps = bps / 1000.0;
    results[0][size_idx].time_us = total_time;
    results[0][size_idx].data_size = size;
    results[0][size_idx].valid = true;

    printf("[BENCH] Method=write_bytes Size=%u TotalTime=%lld us Throughput=%.2f kbps\n",
           (unsigned)size, (long long)total_time, bps / 1000.0);
}

/* ========================= Method 2: uart_tx_chars ========================= */
static void bench_tx_chars(const uint8_t *data, size_t size, int size_idx)
{
    ESP_LOGI(TAG, "  Method 2: uart_tx_chars() — %u bytes x %d iter",
             (unsigned)size, NUM_ITERATIONS);

    int64_t total_time = 0;
    size_t total_sent = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int64_t start = esp_timer_get_time();

        size_t offset = 0;
        while (offset < size) {
            /* uart_tx_chars sends what fits in FIFO, returns count */
            int sent = uart_tx_chars(UART_PORT_NUM,
                                     (const char *)(data + offset),
                                     size - offset);
            if (sent > 0) {
                offset += sent;
                total_sent += sent;
            } else {
                /* FIFO full, wait briefly */
                vTaskDelay(1);
            }
        }
        uart_wait_tx_done(UART_PORT_NUM, pdMS_TO_TICKS(1000));

        int64_t elapsed = esp_timer_get_time() - start;
        total_time += elapsed;
    }

    double total_bits = (double)total_sent * 8.0;
    double time_sec = total_time / 1e6;
    double bps = total_bits / time_sec;

    results[1][size_idx].throughput_bps = bps;
    results[1][size_idx].throughput_kbps = bps / 1000.0;
    results[1][size_idx].time_us = total_time;
    results[1][size_idx].data_size = size;
    results[1][size_idx].valid = true;

    printf("[BENCH] Method=tx_chars Size=%u TotalTime=%lld us Throughput=%.2f kbps\n",
           (unsigned)size, (long long)total_time, bps / 1000.0);
}

/* ========================= Method 3: printf via UART0 ========================= */
static void bench_printf_uart0(const uint8_t *data, size_t size, int size_idx)
{
    ESP_LOGI(TAG, "  Method 3: printf via UART0 — %u bytes x %d iter",
             (unsigned)size, NUM_ITERATIONS);

    /* Prepare a printable string */
    char *str = malloc(size + 1);
    if (!str) {
        results[2][size_idx].valid = false;
        return;
    }
    memcpy(str, data, size);
    str[size] = '\0';

    int64_t total_time = 0;

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int64_t start = esp_timer_get_time();
        /* Write to UART0 (console) using low-level write */
        uart_write_bytes(UART_NUM_0, str, size);
        uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(1000));
        int64_t elapsed = esp_timer_get_time() - start;
        total_time += elapsed;
    }

    double total_bits = (double)size * NUM_ITERATIONS * 8.0;
    double time_sec = total_time / 1e6;
    double bps = total_bits / time_sec;

    results[2][size_idx].throughput_bps = bps;
    results[2][size_idx].throughput_kbps = bps / 1000.0;
    results[2][size_idx].time_us = total_time;
    results[2][size_idx].data_size = size;
    results[2][size_idx].valid = true;

    free(str);

    printf("\n[BENCH] Method=printf_uart0 Size=%u TotalTime=%lld us Throughput=%.2f kbps\n",
           (unsigned)size, (long long)total_time, bps / 1000.0);
}

/* ========================= Run All Benchmarks ========================= */
static void run_benchmarks(void)
{
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, " ESP32 UART TX Benchmark");
    ESP_LOGI(TAG, " ESP-IDF UART driver menggunakan interrupt+FIFO,");
    ESP_LOGI(TAG, " bukan DMA channel terpisah seperti STM32");
    ESP_LOGI(TAG, "============================================================");
    ESP_LOGI(TAG, " Baud Rate: %d", UART_BAUD_RATE);
    ESP_LOGI(TAG, " Iterations: %d", NUM_ITERATIONS);
    ESP_LOGI(TAG, " TX Buffer: %d bytes", UART_TX_BUF_SIZE);
    ESP_LOGI(TAG, "============================================================\n");

    for (int s = 0; s < NUM_TEST_SIZES; s++) {
        size_t size = test_sizes[s];
        uint8_t *data = generate_test_data(size);
        if (!data) {
            ESP_LOGE(TAG, "Failed to allocate %u bytes test data", (unsigned)size);
            continue;
        }

        ESP_LOGI(TAG, "=== Test Data Size: %u bytes ===", (unsigned)size);

        bench_write_bytes(data, size, s);
        vTaskDelay(pdMS_TO_TICKS(INTER_TEST_DELAY_MS));

        bench_tx_chars(data, size, s);
        vTaskDelay(pdMS_TO_TICKS(INTER_TEST_DELAY_MS));

        bench_printf_uart0(data, size, s);
        vTaskDelay(pdMS_TO_TICKS(INTER_TEST_DELAY_MS));

        free(data);
        printf("\n");
    }

    /* ========================= Print Summary Table ========================= */
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║           ESP32 UART TX Benchmark — Summary (kbps)              ║\n");
    printf("╠═══════════════════╦═════════════╦═════════════╦═════════════════╣\n");
    printf("║ Method            ║   256 B     ║   1024 B    ║   4096 B        ║\n");
    printf("╠═══════════════════╬═════════════╬═════════════╬═════════════════╣\n");

    for (int m = 0; m < 3; m++) {
        printf("║ %-17s ║", method_names[m]);
        for (int s = 0; s < NUM_TEST_SIZES; s++) {
            if (results[m][s].valid) {
                printf(" %9.2f   ║", results[m][s].throughput_kbps);
            } else {
                printf("    N/A      ║");
            }
        }
        printf("\n");
    }

    printf("╚═══════════════════╩═════════════╩═════════════╩═════════════════╝\n");
    printf("\n");
    printf("[INFO] Theoretical max at %d baud: %.2f kbps\n",
           UART_BAUD_RATE, UART_BAUD_RATE / 1000.0);
    printf("[INFO] ESP-IDF UART driver uses interrupt+FIFO internally\n");
    printf("[INFO] uart_write_bytes() should approach theoretical max\n");
    printf("[INFO] uart_tx_chars() has more overhead per call\n");
}

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 UART TX Benchmark");
    ESP_LOGI(TAG, " Modul 08 — DMA (UART TX)");
    ESP_LOGI(TAG, "========================================");

    esp_err_t ret = uart1_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART1 init failed! Aborting.");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); /* Settle time */

    run_benchmarks();

    ESP_LOGI(TAG, "Benchmark complete.");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
