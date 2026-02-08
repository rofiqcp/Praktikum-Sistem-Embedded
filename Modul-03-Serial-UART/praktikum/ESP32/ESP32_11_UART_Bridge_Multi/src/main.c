/**
 * @file main.c
 * @brief UART Bridge Multi - Bridge antara UART0 dan UART1
 *
 * @details
 * Modul       : Modul 03 - Serial UART
 * Board       : ESP32 DevKit / ESP32-S2 / ESP32-S3
 * Framework   : ESP-IDF
 *
 * Koneksi:
 *   - UART0: USB Serial (monitor/terminal)
 *   - UART1 TX: GPIO17 -> hubungkan ke perangkat external RX
 *   - UART1 RX: GPIO16 -> hubungkan ke perangkat external TX
 *   - GND bersama antara ESP32 dan perangkat external
 *
 * Cara Kerja:
 *   1. Inisialisasi UART0 (115200 baud) dan UART1 (9600 baud)
 *   2. UART1 menggunakan pin GPIO17 (TX) dan GPIO16 (RX) via uart_set_pin()
 *   3. Data dari UART0 diteruskan ke UART1 (forward direction)
 *   4. Data dari UART1 diteruskan ke UART0 (reverse direction)
 *   5. Dua task terpisah menangani masing-masing arah
 *   6. Statistik: byte count per arah, ditampilkan periodik
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_BRIDGE";

/* Bridge statistics */
static volatile uint32_t uart0_to_uart1_bytes = 0;
static volatile uint32_t uart1_to_uart0_bytes = 0;
static volatile uint32_t uart0_to_uart1_packets = 0;
static volatile uint32_t uart1_to_uart0_packets = 0;

/**
 * @brief Inisialisasi UART0 (USB Serial)
 */
static void uart0_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = UART0_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART0_PORT, &uart_config);
    uart_driver_install(UART0_PORT, UART0_BUF_SIZE, UART0_BUF_SIZE, 0, NULL, 0);

    ESP_LOGI(TAG, "UART0 diinisialisasi pada %d baud (USB Serial)", UART0_BAUD_RATE);
}

/**
 * @brief Inisialisasi UART1 (External, custom pins)
 */
static void uart1_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = BRIDGE_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART1_PORT, &uart_config);
    uart_driver_install(UART1_PORT, UART1_BUF_SIZE, UART1_BUF_SIZE, 0, NULL, 0);

    /* Set custom pins for UART1 */
    uart_set_pin(UART1_PORT, UART1_TX_PIN, UART1_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "UART1 diinisialisasi pada %d baud (TX=GPIO%d, RX=GPIO%d)",
             BRIDGE_BAUD, UART1_TX_PIN, UART1_RX_PIN);
}

/**
 * @brief Task bridge: UART0 -> UART1
 *
 * Meneruskan data dari USB Serial ke perangkat external
 */
static void uart0_to_uart1_task(void *pvParameters)
{
    uint8_t buf[BRIDGE_BUF_SIZE];

    ESP_LOGI(TAG, "Bridge UART0 -> UART1 dimulai");

    while (1) {
        int len = uart_read_bytes(UART0_PORT, buf, BRIDGE_BUF_SIZE, pdMS_TO_TICKS(50));
        if (len > 0) {
            uart_write_bytes(UART1_PORT, buf, len);
            uart0_to_uart1_bytes += len;
            uart0_to_uart1_packets++;

            ESP_LOGI(TAG, "UART0->UART1: %d bytes forwarded", len);
        }
    }
}

/**
 * @brief Task bridge: UART1 -> UART0
 *
 * Meneruskan data dari perangkat external ke USB Serial
 */
static void uart1_to_uart0_task(void *pvParameters)
{
    uint8_t buf[BRIDGE_BUF_SIZE];

    ESP_LOGI(TAG, "Bridge UART1 -> UART0 dimulai");

    while (1) {
        int len = uart_read_bytes(UART1_PORT, buf, BRIDGE_BUF_SIZE, pdMS_TO_TICKS(50));
        if (len > 0) {
            uart_write_bytes(UART0_PORT, buf, len);
            uart1_to_uart0_bytes += len;
            uart1_to_uart0_packets++;

            ESP_LOGI(TAG, "UART1->UART0: %d bytes forwarded", len);
        }
    }
}

/**
 * @brief Task statistik bridge
 */
static void stats_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STATS_INTERVAL_MS));

        ESP_LOGI(TAG, "=== Bridge Statistics ===");
        ESP_LOGI(TAG, "  UART0 -> UART1: %lu bytes, %lu packets",
                 (unsigned long)uart0_to_uart1_bytes,
                 (unsigned long)uart0_to_uart1_packets);
        ESP_LOGI(TAG, "  UART1 -> UART0: %lu bytes, %lu packets",
                 (unsigned long)uart1_to_uart0_bytes,
                 (unsigned long)uart1_to_uart0_packets);
        ESP_LOGI(TAG, "  Total forwarded: %lu bytes",
                 (unsigned long)(uart0_to_uart1_bytes + uart1_to_uart0_bytes));
        ESP_LOGI(TAG, "  Free heap: %lu bytes",
                 (unsigned long)esp_get_free_heap_size());
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Modul 03: UART Bridge Multi ===");
    ESP_LOGI(TAG, "UART0 (%d baud) <--> UART1 (%d baud)",
             UART0_BAUD_RATE, BRIDGE_BAUD);
    ESP_LOGI(TAG, "UART1 pins: TX=GPIO%d, RX=GPIO%d", UART1_TX_PIN, UART1_RX_PIN);

    uart0_init();
    uart1_init();

    xTaskCreate(uart0_to_uart1_task, "u0_to_u1", 4096, NULL, 6, NULL);
    xTaskCreate(uart1_to_uart0_task, "u1_to_u0", 4096, NULL, 6, NULL);
    xTaskCreate(stats_task, "stats", 4096, NULL, 3, NULL);
}
