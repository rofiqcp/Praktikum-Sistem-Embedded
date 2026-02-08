/**
 * @file config.h
 * @brief Configuration for SPI Interrupt Mode Demo
 * 
 * Modul 07 - SPI & Storage
 * Program 11: SPI Interrupt (Non-blocking) Mode
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

/* ==================== SPI Settings ==================== */
#define SPI_CLOCK_HZ    4000000     // 4 MHz SPI clock
#define QUEUE_SIZE      7           // Transaction queue size
#define NUM_TRANSACTIONS 10         // Number of transactions for demo

/* ==================== Buffer Settings ==================== */
#define TX_BUFFER_SIZE  64          // Transfer buffer size in bytes
#define DMA_CHANNEL     SPI_DMA_CH_AUTO

/* ==================== Timing ==================== */
#define DEMO_DELAY_MS   1000        // Delay between demos

#endif /* CONFIG_H */
