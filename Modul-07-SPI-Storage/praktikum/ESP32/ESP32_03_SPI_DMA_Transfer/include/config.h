#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// ESP32 SPI DMA Transfer - Configuration
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

// Konfigurasi SPI
#define SPI_CLOCK_SPEED_HZ  (10 * 1000 * 1000) // 10 MHz untuk tes DMA
#define SPI_MODE            0                    // CPOL=0, CPHA=0
#define DMA_CHANNEL         SPI_DMA_CH_AUTO      // Auto DMA channel

// Ukuran transfer untuk perbandingan DMA
// DMA sangat efektif untuk transfer data besar
#define SMALL_TRANSFER_SIZE     16      // 16 byte - transfer kecil
#define MEDIUM_TRANSFER_SIZE    256     // 256 byte - transfer medium
#define LARGE_TRANSFER_SIZE     1024    // 1024 byte - transfer besar
#define XLARGE_TRANSFER_SIZE    4096    // 4096 byte - transfer sangat besar

// Jumlah iterasi per tes (untuk rata-rata)
#define NUM_ITERATIONS          10

// Ukuran maksimum transfer (harus cukup untuk buffer terbesar)
#define MAX_TRANSFER_SIZE       4096

#endif // CONFIG_H
