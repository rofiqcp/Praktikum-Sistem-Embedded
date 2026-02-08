/**
 * @file config.h
 * @brief Konfigurasi UART Binary Framing (STX/ETX)
 */

#ifndef CONFIG_H
#define CONFIG_H

/* UART Configuration */
#define UART_PORT           UART_NUM_0
#define UART_BAUD_RATE      115200
#define UART_TX_PIN         UART_PIN_NO_CHANGE
#define UART_RX_PIN         UART_PIN_NO_CHANGE
#define UART_BUF_SIZE       1024

/* Framing Protocol Constants */
#define STX                 0x02    /**< Start of Text */
#define ETX                 0x03    /**< End of Text */
#define DLE                 0x10    /**< Data Link Escape */

/* Buffer Sizes */
#define MAX_FRAME_SIZE      256
#define MAX_DATA_SIZE       128

/* Timing */
#define SEND_INTERVAL_MS    3000    /**< Interval kirim frame test (ms) */

#endif /* CONFIG_H */
