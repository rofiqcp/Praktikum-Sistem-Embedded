/**
 * ===========================================================================
 *  PROGRAM 3: ESP32 UART RX Event-Driven with Driver Buffering
 * ===========================================================================
 *
 *  DESKRIPSI:
 *  UART receive menggunakan event-driven approach dari ESP-IDF UART driver.
 *  Driver mengelola ring buffer secara internal dengan interrupt, mirip
 *  DMA-backed reception pada STM32.
 *
 *  Fitur:
 *    - Event-driven RX: UART_DATA, UART_FIFO_OVF, UART_BUFFER_FULL,
 *      UART_PATTERN_DET
 *    - Pattern detection: deteksi karakter akhir baris (CR)
 *    - Hex dump data yang diterima
 *    - Statistik: bytes received, events, overflow, pattern matches
 *    - Circular processing: proses data secara ring-buffer fashion
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
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_RX";

/* Event queue handle */
static QueueHandle_t uart_event_queue = NULL;

/* Statistics */
typedef struct {
    uint32_t total_bytes_received;
    uint32_t total_events;
    uint32_t data_events;
    uint32_t fifo_overflow_count;
    uint32_t buffer_full_count;
    uint32_t pattern_match_count;
    uint32_t break_count;
    uint32_t parity_error_count;
    uint32_t frame_error_count;
    uint32_t other_events;
    int64_t  start_time_us;
} uart_stats_t;

static uart_stats_t stats = {0};

/* ========================= Hex Dump Utility ========================= */
static void hex_dump(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i += HEX_DUMP_BYTES_PER_LINE) {
        printf("  %04X: ", (unsigned)i);

        /* Hex values */
        for (size_t j = 0; j < HEX_DUMP_BYTES_PER_LINE; j++) {
            if (i + j < len) {
                printf("%02X ", data[i + j]);
            } else {
                printf("   ");
            }
            if (j == 7) printf(" ");
        }

        printf(" |");

        /* ASCII representation */
        for (size_t j = 0; j < HEX_DUMP_BYTES_PER_LINE && (i + j) < len; j++) {
            uint8_t c = data[i + j];
            printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }

        printf("|\n");
    }
}

/* ========================= Print Statistics ========================= */
static void print_stats(void)
{
    int64_t elapsed_us = esp_timer_get_time() - stats.start_time_us;
    double elapsed_s = elapsed_us / 1e6;
    double avg_bps = (stats.total_bytes_received * 8.0) / elapsed_s;

    printf("\n");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║          UART RX Statistics                     ║\n");
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║ Runtime:          %10.1f s                  ║\n", elapsed_s);
    printf("║ Total Bytes:      %10u                     ║\n", (unsigned)stats.total_bytes_received);
    printf("║ Avg Throughput:   %10.2f bps                ║\n", avg_bps);
    printf("╠══════════════════════════════════════════════════╣\n");
    printf("║ Total Events:     %10u                     ║\n", (unsigned)stats.total_events);
    printf("║ Data Events:      %10u                     ║\n", (unsigned)stats.data_events);
    printf("║ FIFO Overflow:    %10u                     ║\n", (unsigned)stats.fifo_overflow_count);
    printf("║ Buffer Full:      %10u                     ║\n", (unsigned)stats.buffer_full_count);
    printf("║ Pattern Match:    %10u                     ║\n", (unsigned)stats.pattern_match_count);
    printf("║ Break:            %10u                     ║\n", (unsigned)stats.break_count);
    printf("║ Parity Error:     %10u                     ║\n", (unsigned)stats.parity_error_count);
    printf("║ Frame Error:      %10u                     ║\n", (unsigned)stats.frame_error_count);
    printf("║ Other:            %10u                     ║\n", (unsigned)stats.other_events);
    printf("╚══════════════════════════════════════════════════╝\n\n");
}

/* ========================= UART Event Task ========================= */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *read_buf = malloc(MAX_READ_BUF_SIZE);
    if (!read_buf) {
        ESP_LOGE(TAG, "Failed to allocate read buffer!");
        vTaskDelete(NULL);
        return;
    }

    int64_t last_stats_time = esp_timer_get_time();

    ESP_LOGI(TAG, "UART RX event task started. Waiting for data...");
    ESP_LOGI(TAG, "Send data to UART%d (GPIO%d RX) to test.", UART_PORT_NUM, UART_RX_PIN);
    ESP_LOGI(TAG, "Pattern detection: 0x%02X (CR/Enter)", PATTERN_CHR);

    while (1) {
        /* Wait for UART event */
        if (xQueueReceive(uart_event_queue, &event, pdMS_TO_TICKS(UART_READ_TIMEOUT_MS))) {
            stats.total_events++;

            switch (event.type) {
                case UART_DATA:
                {
                    stats.data_events++;
                    ESP_LOGD(TAG, "[UART_DATA] Size: %d", event.size);

                    /* Read data from ring buffer */
                    int len = uart_read_bytes(UART_PORT_NUM, read_buf,
                                              (event.size < MAX_READ_BUF_SIZE)
                                              ? event.size : MAX_READ_BUF_SIZE,
                                              pdMS_TO_TICKS(100));

                    if (len > 0) {
                        stats.total_bytes_received += len;

                        printf("[RX] Received %d bytes:\n", len);
                        hex_dump(read_buf, len);

                        /* Echo back for debug (circular processing) */
                        uart_write_bytes(UART_PORT_NUM, read_buf, len);
                    }
                    break;
                }

                case UART_FIFO_OVF:
                {
                    stats.fifo_overflow_count++;
                    ESP_LOGW(TAG, "[UART_FIFO_OVF] FIFO overflow detected!");
                    /* Flush input to recover */
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_event_queue);
                    break;
                }

                case UART_BUFFER_FULL:
                {
                    stats.buffer_full_count++;
                    ESP_LOGW(TAG, "[UART_BUFFER_FULL] Ring buffer full!");
                    /* Read all available data to free buffer */
                    int len;
                    do {
                        len = uart_read_bytes(UART_PORT_NUM, read_buf,
                                              MAX_READ_BUF_SIZE,
                                              pdMS_TO_TICKS(10));
                        if (len > 0) {
                            stats.total_bytes_received += len;
                            printf("[RX-DRAIN] Drained %d bytes\n", len);
                        }
                    } while (len > 0);
                    break;
                }

                case UART_PATTERN_DET:
                {
                    stats.pattern_match_count++;
                    /* Get pattern position in buffer */
                    int pos = uart_pattern_pop_pos(UART_PORT_NUM);
                    ESP_LOGI(TAG, "[PATTERN] Detected at position: %d", pos);

                    if (pos >= 0) {
                        /* Read up to and including the pattern */
                        int read_len = pos + PATTERN_CHR_NUM;
                        if (read_len > MAX_READ_BUF_SIZE) {
                            read_len = MAX_READ_BUF_SIZE;
                        }
                        int len = uart_read_bytes(UART_PORT_NUM, read_buf,
                                                  read_len, pdMS_TO_TICKS(100));
                        if (len > 0) {
                            stats.total_bytes_received += len;
                            printf("[PATTERN] Complete message (%d bytes):\n", len);
                            hex_dump(read_buf, len);
                        }
                    } else {
                        ESP_LOGW(TAG, "Pattern pos queue full, flushing");
                        uart_flush_input(UART_PORT_NUM);
                    }
                    break;
                }

                case UART_BREAK:
                    stats.break_count++;
                    ESP_LOGW(TAG, "[UART_BREAK] Break condition detected");
                    break;

                case UART_PARITY_ERR:
                    stats.parity_error_count++;
                    ESP_LOGW(TAG, "[UART_PARITY_ERR] Parity error");
                    break;

                case UART_FRAME_ERR:
                    stats.frame_error_count++;
                    ESP_LOGW(TAG, "[UART_FRAME_ERR] Frame error");
                    break;

                default:
                    stats.other_events++;
                    ESP_LOGD(TAG, "[EVENT] Type: %d", event.type);
                    break;
            }
        }

        /* Periodic statistics report */
        int64_t now = esp_timer_get_time();
        if ((now - last_stats_time) >= (STATS_REPORT_INTERVAL_MS * 1000LL)) {
            print_stats();
            last_stats_time = now;
        }
    }

    free(read_buf);
    vTaskDelete(NULL);
}

/* ========================= UART Initialization ========================= */
static esp_err_t uart_init(void)
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

    /* Install driver with event queue */
    ret = uart_driver_install(UART_PORT_NUM, UART_RX_BUF_SIZE,
                              UART_TX_BUF_SIZE, UART_EVENT_QUEUE_SIZE,
                              &uart_event_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(UART_PORT_NUM, &uart_cfg);
    if (ret != ESP_OK) return ret;

    ret = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) return ret;

    /* Enable pattern detection for end-of-line (CR) */
    ret = uart_enable_pattern_det_baud_intr(UART_PORT_NUM, PATTERN_CHR,
                                            PATTERN_CHR_NUM,
                                            PATTERN_TIMEOUT,
                                            PATTERN_POST_IDLE,
                                            PATTERN_PRE_IDLE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Pattern detection setup failed: %s", esp_err_to_name(ret));
        /* Non-fatal — continue without pattern detection */
    }

    /* Allocate pattern position queue */
    ret = uart_pattern_queue_reset(UART_PORT_NUM, UART_EVENT_QUEUE_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Pattern queue reset failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "UART%d initialized: TX=GPIO%d, RX=GPIO%d, Baud=%d",
             UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
    ESP_LOGI(TAG, "RX buffer: %d bytes, Event queue: %d entries",
             UART_RX_BUF_SIZE, UART_EVENT_QUEUE_SIZE);
    ESP_LOGI(TAG, "Pattern detection: char=0x%02X", PATTERN_CHR);

    return ESP_OK;
}

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 UART RX Event-Driven");
    ESP_LOGI(TAG, " Modul 08 — DMA (UART RX)");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP-IDF UART driver menggunakan interrupt-driven");
    ESP_LOGI(TAG, "ring buffer untuk penerimaan data. Tidak ada DMA");
    ESP_LOGI(TAG, "channel terpisah seperti STM32 — driver mengelola");
    ESP_LOGI(TAG, "FIFO hardware + interrupt secara internal.");

    esp_err_t ret = uart_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART init failed! Aborting.");
        return;
    }

    /* Initialize stats */
    memset(&stats, 0, sizeof(stats));
    stats.start_time_us = esp_timer_get_time();

    /* Create RX event task */
    BaseType_t task_ret = xTaskCreate(uart_event_task, "uart_rx_task",
                                      RX_TASK_STACK_SIZE, NULL,
                                      RX_TASK_PRIORITY, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART RX task!");
        return;
    }

    ESP_LOGI(TAG, "UART RX task started. Send data to test reception.");
    ESP_LOGI(TAG, "Use a serial terminal or debug_uart_rx.py to send data.");

    /* Main loop — just keep alive, all work in RX task */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
