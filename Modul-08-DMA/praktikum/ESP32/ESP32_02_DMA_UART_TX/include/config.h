/**
 * ===========================================================================
 *  ESP32 UART TX Benchmark — Configuration
 * ===========================================================================
 *
 *  CATATAN ARSITEKTUR:
 *  ESP-IDF UART driver menggunakan interrupt + FIFO hardware, bukan DMA
 *  channel terpisah seperti STM32. Namun driver secara internal menggunakan
 *  ring buffer dan interrupt-driven transfer yang efisien, mirip dengan
 *  DMA-backed transfer.
 *
 *  File ini mendefinisikan parameter UART dan ukuran buffer untuk benchmark.
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
#define UART_TX_BUF_SIZE    1024    /* TX ring buffer size */
#define UART_RX_BUF_SIZE    1024    /* RX ring buffer size (needed for driver) */

/* ---------- Test Data Sizes ---------- */
#define TEST_SIZE_SMALL     256
#define TEST_SIZE_MEDIUM    1024
#define TEST_SIZE_LARGE     4096
#define NUM_TEST_SIZES      3

/* ---------- Benchmark Parameters ---------- */
#define NUM_ITERATIONS      50
#define INTER_TEST_DELAY_MS 200

#endif /* CONFIG_H */
