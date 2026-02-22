/**
 * @file config.h
 * @brief Konfigurasi UART Error Statistics Monitor
 */

#ifndef CONFIG_H
#define CONFIG_H

/* UART Configuration */
#define UART_PORT           UART_NUM_0
#define UART_BAUD_RATE      115200
#define UART_TX_PIN         UART_PIN_NO_CHANGE
#define UART_RX_PIN         UART_PIN_NO_CHANGE
#define UART_BUF_SIZE       1024

/* Event Queue */
#define UART_EVENT_QUEUE_SIZE   20

/* Timing */
#define REPORT_INTERVAL_MS  5000    /**< Interval laporan statistik error (ms) */

/* Health Thresholds */
#define HEALTH_GOOD_RATE    0.01f   /**< < 1% error = GOOD */
#define HEALTH_WARN_RATE    0.05f   /**< < 5% error = WARNING */
                                    /**< >= 5% error = CRITICAL */

#endif /* CONFIG_H */
