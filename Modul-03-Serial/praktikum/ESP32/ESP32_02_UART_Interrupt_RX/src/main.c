/**
 * @file    main.c
 * @brief   ESP32_02 – UART Interrupt-Driven RX via Event Queue
 *
 * @details
 * Modul       : Modul-03 Serial UART
 * Board       : ESP32 DevKit / Lolin S2 Mini / ESP32-S3 DevKitC
 * Framework   : ESP-IDF
 *
 * Koneksi     :
 *   - UART0 menggunakan pin default (TX=GPIO1, RX=GPIO3 pada ESP32 DevKit)
 *   - Hubungkan USB ke PC, gunakan serial monitor 115200 baud
 *
 * Cara Kerja  :
 *   1. UART0 diinisialisasi dengan event queue melalui uart_driver_install().
 *   2. Task khusus (uart_event_task) menunggu event UART via xQueueReceive().
 *   3. Saat event UART_DATA diterima, data dibaca dan di-echo kembali.
 *   4. Event lain (FIFO_OVF, BUFFER_FULL, BREAK, PARITY_ERR, FRAME_ERR)
 *      di-log sebagai peringatan (ESP_LOGW).
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_INT_RX";

/** @brief Queue untuk menerima event UART dari ISR */
static QueueHandle_t uart_queue;

/**
 * @brief Task yang memproses event UART dari queue
 * @param pvParameters Parameter (tidak digunakan)
 */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t data[BUF_SIZE];

    while (1) {
        /* Tunggu event dari UART driver (blocking) */
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            switch (event.type) {

            case UART_DATA:
                ESP_LOGI(TAG, "UART_DATA event: %d byte tersedia", (int)event.size);
                {
                    int len = uart_read_bytes(UART_PORT, data,
                                              event.size, pdMS_TO_TICKS(100));
                    if (len > 0) {
                        /* Null-terminate agar bisa di-print sebagai string */
                        data[len] = '\0';
                        ESP_LOGI(TAG, "Diterima: %s", data);
                        /* Echo kembali */
                        uart_write_bytes(UART_PORT, (const char *)data, len);
                    }
                }
                break;

            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "FIFO overflow!");
                uart_flush_input(UART_PORT);
                xQueueReset(uart_queue);
                break;

            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "Ring buffer penuh!");
                uart_flush_input(UART_PORT);
                xQueueReset(uart_queue);
                break;

            case UART_BREAK:
                ESP_LOGW(TAG, "UART break detected");
                break;

            case UART_PARITY_ERR:
                ESP_LOGW(TAG, "Parity error");
                break;

            case UART_FRAME_ERR:
                ESP_LOGW(TAG, "Frame error");
                break;

            default:
                ESP_LOGI(TAG, "Event type: %d", event.type);
                break;
            }
        }
    }
}

/**
 * @brief Inisialisasi UART dengan event queue
 */
static void uart_init(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* Install driver DENGAN event queue */
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE,
                                        UART_QUEUE_SIZE, &uart_queue, 0));
}

/**
 * @brief Entry point aplikasi
 */
void app_main(void)
{
    uart_init();
    ESP_LOGI(TAG, "UART Interrupt RX siap (UART%d, baud=%d)", UART_PORT, UART_BAUD);

    /* Buat task untuk memproses event UART */
    xTaskCreate(uart_event_task, "uart_event_task",
                4096, NULL, 12, NULL);
}
