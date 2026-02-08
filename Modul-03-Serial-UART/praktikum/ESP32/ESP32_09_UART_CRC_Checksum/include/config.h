/**
 * @file config.h
 * @brief Konfigurasi UART CRC-8 Checksum
 */

#ifndef CONFIG_H
#define CONFIG_H

/* UART Configuration */
#define UART_PORT           UART_NUM_0
#define UART_BAUD_RATE      115200
#define UART_TX_PIN         UART_PIN_NO_CHANGE
#define UART_RX_PIN         UART_PIN_NO_CHANGE
#define UART_BUF_SIZE       1024

/* CRC-8 Configuration */
#define CRC_POLY            0x07    /**< Polynomial: x^8 + x^2 + x + 1 */
#define CRC_INIT            0x00    /**< Nilai awal CRC */

/* Buffer Sizes */
#define MAX_DATA_SIZE       128