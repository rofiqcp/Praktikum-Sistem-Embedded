/**
 * ===========================================================================
 *  ESP32 UART RX Event-Driven — Configuration
 * ===========================================================================
 *
 *  CATATAN ARSITEKTUR:
 *  ESP-IDF UART driver menggunakan interrupt-driven ring buffer untuk
 *  penerimaan data. Event queue memungkinkan pemrosesan asinkron yang
 *  efisien, mirip DMA-backed reception pada STM32.
 *
 *  File ini mendefinisikan parameter UART RX dan event handling.
 * ===========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---------- UART Configuration ---------- */
#define UART_PORT_NUM       UART_NUM_1
#define UART_TX_PIN         17
#define UART_RX_PIN         16
#define UART_BAUD_RATE      115200

/* ---------- Buffer Sizes ---------- */
#define UART_RX_BUF_SIZE    2048    /* RX ring buffer size */
#define UART_TX_BUF_SIZE    256     /* TX ring buffer (minimal, not used much) */

/* ---------- Event Queue ---------- */
#define UART_EVENT_QUEUE_SIZE   20
#define UART_READ_TIMEOUT_MS    100

/* ---------- Pattern Detection ---------- */
#define PATTERN_CHR         0x0D    /* Carriage Return '\r' */
#define PATTERN_CHR_NUM     1       /* Detect single pattern character */
#define PATTERN_TIMEOUT     9       /* Pattern detection timeout (UART clocks) */
#define PATTERN_POST_IDLE   0       /* Post-idle time (UART clocks) */
#define PATTERN_PRE_IDLE    0       /* Pre-idle time (UART clocks) */

/* ---------- Hex Dump ---------- */
#define HEX_DUMP_BYTES_PER_LINE     16
#define MAX_READ_BUF_SIZE           512

/* ---------- Stats Reporting ---------- */
#define STATS_REPORT_INTERVAL_MS    5000

/* ---------- Task Configuration ---------- */
#define RX_TASK_STACK_SIZE  4096
#define RX_TASK_PRIORITY    12

#endif /* CONFIG_H */
