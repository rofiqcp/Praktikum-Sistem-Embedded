/**
 * @file config.h
 * @brief Konfigurasi UART Bridge Multi (UART0 <-> UART1)
 */

#ifndef CONFIG_H
#define CONFIG_H

/* UART0 Configuration (USB Serial) */
#define UART0_PORT          UART_NUM_0
#define UART0_BAUD_RATE     115200
#define UART0_BUF_SIZE      1024

/* UART1 Configuration (External) */
#define UART1_PORT          UART_NUM_1
#define UART1_TX_PIN        GPIO_NUM_17
#define UART1_RX_PIN        GPIO_NUM_16
#define UART1_BUF_SIZE      1024

/* Bridge Configuration */
#define BRIDGE_BAUD         9600    /**< Baud rate UART1 */

/* Bridge Buffer */
#define BRIDGE_BUF_SIZE     256

/* Statistics */
#define STATS_INTERVAL_MS   5000    /**< Interval tampilkan statistik (ms) */

#endif /* CONFIG_H */
