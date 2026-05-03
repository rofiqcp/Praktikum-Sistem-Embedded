/**
 * @file config.h
 * @brief Configuration for SPI Speed Benchmark
 * 
 * Modul 07 - SPI & Storage
 * Program 10: SPI Speed Benchmark
 * 
 * Platform: ESP32 (ESP-IDF)
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI Pin Configuration ==================== */
#define PIN_MOSI        23          // Master Out Slave In
#define PIN_MISO        19          // Master In Slave Out
#define PIN_SCLK        18          // Serial Clock
#define PIN_CS          5           // Chip Select

/* ==================== SPI Host ==================== */
#define SPI_HOST_ID     HSPI_HOST   // Use HSPI (SPI2)

/* ==================== Benchmark Speeds (Hz) ==================== */
#define NUM_SPEEDS      7
static const int test_speeds[NUM_SPEEDS] = {
    1000000,    //  1 MHz
    2000000,    //  2 MHz
    4000000,    //  4 MHz
    8000000,    //  8 MHz
    10000000,   // 10 MHz
    20000000,   // 20 MHz
    40000000,   // 40 MHz
};

static const char *speed_labels[NUM_SPEEDS] = {
    " 1 MHz", " 2 MHz", " 4 MHz", " 8 MHz", "10 MHz", "20 MHz", "40 MHz"
};

/* ==================== Buffer Sizes ==================== */
#define NUM_BUFFER_SIZES    5
static const int buffer_sizes[NUM_BUFFER_SIZES] = {
    16, 64, 256, 1024, 4096
};

/* ==================== Benchmark Settings ==================== */
#define NUM_ITERATIONS      100         // Iterations per test
#define DMA_THRESHOLD       32          // Use DMA for transfers > 32 bytes
#define DMA_CHANNEL         SPI_DMA_CH_AUTO  // Auto-select DMA channel
#define SPI_QUEUE_SIZE      1           // Queue size for device

#endif /* CONFIG_H */
