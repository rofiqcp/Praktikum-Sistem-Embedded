/**
 * @file    main.c
 * @brief   ESP32_01 – UART Echo (Terima byte → kirim balik)
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
 *   1. Inisialisasi UART0 dengan baud rate 115200, 8N1, tanpa flow control.
 *   2. Loop utama membaca data yang masuk via uart_read_bytes().
 *   3. Setiap byte yang diterima langsung dikirim kembali (echo) via
 *      uart_write_bytes().
 *   4. Jumlah byte yang di-echo ditampilkan melalui ESP_LOGI.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_ECHO";

/**
 * @brief Inisialisasi UART dengan konfigurasi dari config.h
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

    /* Konfigurasi parameter UART */
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));

    /* Gunakan pin default untuk UART0 */
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    /* Install driver dengan TX & RX buffer */
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE,
                                        0, NULL, 0));
}

/**
 * @brief Entry point aplikasi
 */
void app_main(void)
{
    uart_init();
    ESP_LOGI(TAG, "UART Echo siap pada UART%d, baud=%d", UART_PORT, UART_BAUD);

    uint8_t data[BUF_SIZE];

    while (1) {
        /* Baca data dari UART (timeout 20 ms ≈ 1 tick) */
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE,
                                  pdMS_TO_TICKS(20));
        if (len > 0) {
            /* Echo: kirim kembali data yang diterima */
            uart_write_bytes(UART_PORT, (const char *)data, len);
            ESP_LOGI(TAG, "Echo %d byte", len);
        }
    }
}
