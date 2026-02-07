#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// ESP32 SPI Clock Modes - Configuration
// Hardware: ESP32 DOIT DevKit V1 / Wemos Lolin S2 / ESP32-S3
// ============================================================

// UART Configuration
#define UART_BAUDRATE       115200

// SPI Configuration - Menggunakan HSPI (SPI2)
// Definisi pin SPI pada ESP32:
//   MOSI = GPIO 13
//   MISO = GPIO 12
//   SCLK = GPIO 14
//   CS   = GPIO 15
// Untuk loopback: hubungkan MOSI (GPIO13) ke MISO (GPIO12)

#define SPI_HOST_USED       SPI2_HOST       // HSPI
#define PIN_NUM_MOSI        13
#define PIN_NUM_MISO        12
#define PIN_NUM_CLK         14
#define PIN_NUM_CS          15

// Konfigurasi SPI Transfer
#define SPI_CLOCK_SPEED_HZ  (1 * 1000 * 1000)  // 1 MHz
#define DMA_CHANNEL         SPI_DMA_CH_AUTO       // Auto DMA channel
#define MAX_TRANSFER_SIZE   64                    // Max bytes per transfer
#define TEST_DATA_LEN       8                     // Jumlah byte test data

// Definisi 4 Mode SPI (CPOL | CPHA)
// Mode 0: CPOL=0, CPHA=0 - Clock idle LOW,  data pada rising edge
// Mode 1: CPOL=0, CPHA=1 - Clock idle LOW,  data pada falling edge
// Mode 2: CPOL=1, CPHA=0 - Clock idle HIGH, data pada rising edge
// Mode 3: CPOL=1, CPHA=1 - Clock idle HIGH, data pada falling edge
#define SPI_MODE_0          0
#define SPI_MODE_1          1
#define SPI_MODE_2          2
#define SPI_MODE_3          3
#define NUM_SPI_MODES       4

#endif // CONFIG_H
