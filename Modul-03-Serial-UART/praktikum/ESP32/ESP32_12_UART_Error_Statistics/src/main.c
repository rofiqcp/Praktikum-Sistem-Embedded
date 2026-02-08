/**
 * @file main.c
 * @brief UART Error Statistics Monitor
 *
 * @details
 * Modul       : Modul 03 - Serial UART
 * Board       : ESP32 DevKit / ESP32-S2 / ESP32-S3
 * Framework   : ESP-IDF
 *
 * Koneksi:
 *   - USB Serial (UART0) untuk komunikasi terminal
 *
 * Cara Kerja:
 *   1. Mengaktifkan parity checking (UART_PARITY_EVEN) untuk deteksi error
 *   2. Menggunakan UART event queue untuk menangkap event error
 *   3. Memantau error: overrun, framing, parity, break
 *   4. Menyimpan counter untuk setiap jenis error
 *   5. Melaporkan statistik error secara periodik
 *   6. Menampilkan status "health" berdasarkan error rate:
 *      - GOOD    : error rate < 1%
 *      - WARNING : error rate < 5%
 *      - CRITICAL: error rate >= 5%
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_ERROR";

/* UART event queue */
static QueueHandle_t uart_event_queue;

/* Error counters */
static volatile uint32_t overrun_count  = 0;
static volatile uint32_t framing_count  = 0;
static volatile uint32_t parity_count   = 0;
static volatile uint32_t break_count    = 0;
static volatile uint32_t buffer_full_count = 0;
static volatile uint32_t data_count     = 0;     /**< Total data events */
static volatile uint32_t total_bytes_rx = 0;

/**
 * @brief Inisialisasi UART dengan parity checking dan event queue
 */
static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_EVEN,       /* Enable parity checking */
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE,
                        UART_EVENT_QUEUE_SIZE, &uart_event_queue, 0);

    ESP_LOGI(TAG, "UART%d diinisialisasi pada %d baud, parity=EVEN",
             UART_PORT, UART_BAUD_RATE);
}

/**
 * @brief Hitung total error
 */
static uint32_t get_total_errors(void)
{
    return overrun_count + framing_count + parity_count + break_count + buffer_full_count;
}

/**
 * @brief Tentukan status health berdasarkan error rate
 */
static const char *get_health_status(void)
{
    uint32_t total_events = data_count + get_total_errors();

    if (total_events == 0) {
        return "IDLE (no data)";
    }

    float error_rate = (float)get_total_errors() / (float)total_events;

    if (error_rate < HEALTH_GOOD_RATE) {
        return "✓ GOOD";
    } else if (error_rate < HEALTH_WARN_RATE) {
        return "⚠ WARNING";
    } else {
        return "✗ CRITICAL";
    }
}

/**
 * @brief Task untuk memproses UART events (termasuk error)
 */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t rx_buf[UART_BUF_SIZE];

    ESP_LOGI(TAG, "UART event task dimulai");

    while (1) {
        if (xQueueReceive(uart_event_queue, &event, pdMS_TO_TICKS(100))) {
            switch (event.type) {
                case UART_DATA:
                    /* Data diterima normal */
                    data_count++;
                    if (event.size > 0) {
                        int len = uart_read_bytes(UART_PORT, rx_buf,
                                                  event.size, pdMS_TO_TICKS(100));
                        if (len > 0) {
                            total_bytes_rx += len;
                        }
                    }
                    break;

                case UART_FIFO_OVF:
                    /* Hardware FIFO overflow */
                    overrun_count++;
                    ESP_LOGW(TAG, "!! FIFO Overrun (count=%lu)",
                             (unsigned long)overrun_count);
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_event_queue);
                    break;

                case UART_BUFFER_FULL:
                    /* Ring buffer full */
                    buffer_full_count++;
                    ESP_LOGW(TAG, "!! Buffer Full (count=%lu)",
                             (unsigned long)buffer_full_count);
                    uart_flush_input(UART_PORT);
                    xQueueReset(uart_event_queue);
                    break;

                case UART_BREAK:
                    /* Break condition detected */
                    break_count++;
                    ESP_LOGW(TAG, "!! Break Detected (count=%lu)",
                             (unsigned long)break_count);
                    break;

                case UART_PARITY_ERR:
                    /* Parity error */
                    parity_count++;
                    ESP_LOGW(TAG, "!! Parity Error (count=%lu)",
                             (unsigned long)parity_count);
                    break;

                case UART_FRAME_ERR:
                    /* Framing error */
                    framing_count++;
                    ESP_LOGW(TAG, "!! Framing Error (count=%lu)",
                             (unsigned long)framing_count);
                    break;

                default:
                    ESP_LOGI(TAG, "UART event type: %d", event.type);
                    break;
            }
        }
    }
}

/**
 * @brief Task untuk melaporkan statistik error secara periodik
 */
static void stats_report_task(void *pvParameters)
{
    uint32_t report_num = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(REPORT_INTERVAL_MS));
        report_num++;

        uint32_t total_errors = get_total_errors();
        uint32_t total_events = data_count + total_errors;
        float error_rate = 0.0f;
        if (total_events > 0) {
            error_rate = (float)total_errors / (float)total_events * 100.0f;
        }

        ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
        ESP_LOGI(TAG, "║   UART Error Report #%04lu            ║",
                 (unsigned long)report_num);
        ESP_LOGI(TAG, "╠══════════════════════════════════════╣");
        ESP_LOGI(TAG, "║ Data Events    : %8lu            ║",
                 (unsigned long)data_count);
        ESP_LOGI(TAG, "║ Total Bytes RX : %8lu            ║",
                 (unsigned long)total_bytes_rx);
        ESP_LOGI(TAG, "╠══════════════════════════════════════╣");
        ESP_LOGI(TAG, "║ Overrun Errors : %8lu            ║",
                 (unsigned long)overrun_count);
        ESP_LOGI(TAG, "║ Framing Errors : %8lu            ║",
                 (unsigned long)framing_count);
        ESP_LOGI(TAG, "║ Parity Errors  : %8lu            ║",
                 (unsigned long)parity_count);
        ESP_LOGI(TAG, "║ Break Detect   : %8lu            ║",
                 (unsigned long)break_count);
        ESP_LOGI(TAG, "║ Buffer Full    : %8lu            ║",
                 (unsigned long)buffer_full_count);
        ESP_LOGI(TAG, "╠══════════════════════════════════════╣");
        ESP_LOGI(TAG, "║ Total Errors   : %8lu            ║",
                 (unsigned long)total_errors);
        ESP_LOGI(TAG, "║ Error Rate     : %6.2f%%             ║", error_rate);
        ESP_LOGI(TAG, "║ Health Status  : %-20s ║", get_health_status());
        ESP_LOGI(TAG, "║ Free Heap      : %8lu bytes      ║",
                 (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Modul 03: UART Error Statistics ===");
    ESP_LOGI(TAG, "Parity: EVEN, Report interval: %d ms", REPORT_INTERVAL_MS);

    uart_init();

    xTaskCreate(uart_event_task, "uart_event", 4096, NULL, 6, NULL);
    xTaskCreate(stats_report_task, "stats", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Kirim data via serial untuk memulai monitoring");
    ESP_LOGI(TAG, "Catatan: terminal biasa tanpa parity akan menghasilkan error parity");
}
