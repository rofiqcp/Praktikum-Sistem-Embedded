#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// Konfigurasi ESP32_05_SD_Card_Mount
// Mount SD Card via SPI menggunakan VFS FAT Filesystem
// ============================================================================

// Konfigurasi UART
#define UART_BAUDRATE   115200

// ============================================================================
// Konfigurasi Pin SPI untuk SD Card Module
// Menggunakan VSPI (SPI3) pada ESP32
// ============================================================================
#define PIN_NUM_MOSI    23      // Master Out Slave In  - hubungkan ke MOSI/DI pada modul SD
#define PIN_NUM_MISO    19      // Master In Slave Out   - hubungkan ke MISO/DO pada modul SD
#define PIN_NUM_CLK     18      // Serial Clock          - hubungkan ke SCK/CLK pada modul SD
#define PIN_NUM_CS      5       // Chip Select           - hubungkan ke CS/SS pada modul SD

// ============================================================================
// Konfigurasi SD Card
// ============================================================================
#define MOUNT_POINT     "/sdcard"           // Titik mount filesystem di VFS
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO     // Channel DMA otomatis
#define SD_SPI_HOST     SPI2_HOST           // Menggunakan SPI2 (HSPI)
#define SPI_FREQUENCY   (20 * 1000)         // Frekuensi SPI 20 MHz (dalam kHz)
#define MAX_FILES       5                   // Jumlah maksimal file yang bisa dibuka bersamaan

#endif // CONFIG_H
