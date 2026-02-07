#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// ESP32 SPI Loopback - Configuration
// Hardware: ESP32 DOIT DevKit V1 / Wemos Lolin S2 / ESP32-S3
// ============================================================

// UART Configuration
#define UART_BAUDRATE       115200

// SPI Configuration - Using HSPI (SPI2)
// ESP32 DOIT DevKit V1 Default HSPI Pins:
//   MOSI = GPIO 13
//   MISO = GPIO 12
//   SCLK = GPIO 14
//   CS   = GPIO 15
// Untuk loopback: hubungkan MOSI (GPIO13) ke MISO (GPIO12) dengan jumper wire

#define SPI_HOST_USED       SPI2_HOST       // HSPI
#define PIN_NUM_MOSI        13
#define PIN_NUM_MISO        12
#define PIN_NUM_CLK         14
#define PIN_NUM_CS          15

// SPI Transfer Configuration
#define SPI_CLOCK_SPEED_HZ  (1 * 1000 * 1000)  // 1 MHz
#define SPI_MODE            0                     // CPOL=0, CPHA=0
#define DMA_CHANNEL         SPI_DMA_CH_AUTO       // Auto DMA channel
#define MAX_TRANSFER_SIZE   64                    // Max bytes per transfer
#define TEST_DATA_LEN       16                    // Jumlah byte test data

#endif // CONFIG_H
