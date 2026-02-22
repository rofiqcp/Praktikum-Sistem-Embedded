/**
 * @file config.h
 * @brief Konfigurasi UART Timeout-based Packet Parser
 */

#ifndef CONFIG_H
#define CONFIG_H

/* UART Configuration */
#define UART_PORT           UART_NUM_0
#define UART_BAUD_RATE      115200
#define UART_TX_PIN         UART_PIN_NO_CHANGE
#define UART_RX_PIN         UART_PIN_NO_CHANGE
#define UART_BUF_SIZE       1024

/* Timeout Configuration */
#define TIMEOUT_MS          50      /**< Timeout antar byte untuk deteksi akhir paket (ms) */

/* Buffer Sizes */
#define MAX_PACKET_SIZE     256

/* Timing */
#define STATS_INTERVAL_MS   10000   /**< Interval tampilkan statistik (ms) */

#endif /* CONFIG_H */
