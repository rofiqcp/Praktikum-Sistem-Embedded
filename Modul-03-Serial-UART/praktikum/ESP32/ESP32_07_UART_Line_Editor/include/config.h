/**
 * @file config.h
 * @brief Konfigurasi UART Line Editor
 */

#ifndef CONFIG_H
#define CONFIG_H

/* UART Configuration */
#define UART_PORT           UART_NUM_0
#define UART_BAUD_RATE      115200
#define UART_TX_PIN         UART_PIN_NO_CHANGE
#define UART_RX_PIN         UART_PIN_NO_CHANGE
#define UART_BUF_SIZE       1024

/* Line Editor Configuration */
#define MAX_LINE_LEN        128
#define HISTORY_SIZE        5

/* Special Characters */
#define CHAR_BACKSPACE_1    0x08
#define CHAR_BACKSPACE_2    0x7F
#define CHAR_CR             0x0D
#define CHAR_LF             0x0A
#define CHAR_ESC            0x1B

/* Prompt */
#define PROMPT              "> "

#endif /* CONFIG_H */
