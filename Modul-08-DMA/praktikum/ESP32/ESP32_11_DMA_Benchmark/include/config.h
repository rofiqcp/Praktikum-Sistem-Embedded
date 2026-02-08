/*
 * ==========================================================================
 *  ESP32 DMA Benchmark - Configuration
 * ==========================================================================
 *  Modul 08 - Program 11: Comprehensive DMA vs CPU Benchmark
 *
 *  Benchmark ini membandingkan transfer dengan dan tanpa DMA
 *  di berbagai peripheral ESP32.
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Test Sizes ---- */
#define NUM_TEST_SIZES          5
static const int TEST_SIZES[NUM_TEST_SIZES] = { 64, 256, 1024, 4096, 16384 };

/* ---- Test Iterations ---- */
#define TEST_ITERATIONS         100

/* ---- SPI Configuration (for DMA vs non-DMA test) ---- */
#define SPI_HOST_ID             SPI2_HOST
#define SPI_MOSI_PIN            23
#define SPI_MISO_PIN            19
#define SPI_CLK_PIN             18
#define SPI_CS_PIN              5
#define SPI_CLOCK_HZ            10000000    /* 10 MHz */
#define SPI_MAX_TRANSFER_SIZE   16384

/* ---- UART Configuration ---- */
#define UART_TEST_NUM           UART_NUM_1
#define UART_TX_PIN             17
#define UART_RX_PIN             16
#define UART_BAUD_RATE          921600

/* ---- ADC Configuration ---- */
#define ADC_TEST_CHANNEL        ADC_CHANNEL_0   /* GPIO36 */
#define ADC_TEST_ATTEN          ADC_ATTEN_DB_12
#define ADC_SAMPLE_RATE         20000   /* 20 kHz for continuous mode */

/* ---- CPU Load Estimation ---- */
#define CPU_LOAD_COUNT_PERIOD   1000    /* Count loop iterations in 1ms */

/* ---- Memory Test ---- */
#define MEM_ALIGNMENT           4       /* 4-byte alignment */

/* ---- Results Table ---- */
#define MAX_RESULTS             30      /* Maximum benchmark entries */

#endif /* CONFIG_H */
