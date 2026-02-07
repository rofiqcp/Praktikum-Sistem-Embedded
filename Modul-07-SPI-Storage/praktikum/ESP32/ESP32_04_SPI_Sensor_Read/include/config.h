#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// ESP32 SPI Sensor Read - Configuration
// Simulasi pembacaan sensor SPI (protokol mirip BME280)
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

#define SPI_HOST_USED       SPI2_HOST       // HSPI
#define PIN_NUM_MOSI        13
#define PIN_NUM_MISO        12
#define PIN_NUM_CLK         14
#define PIN_NUM_CS          15

// Konfigurasi SPI untuk sensor
#define SPI_CLOCK_SPEED_HZ  (1 * 1000 * 1000)  // 1 MHz (kecepatan umum sensor)
#define SPI_MODE            0                    // Mode 0 (umum untuk BME280)
#define DMA_CHANNEL         SPI_DMA_CH_AUTO
#define MAX_TRANSFER_SIZE   64

// ============================================================
// Simulasi Register Map Sensor (mirip BME280)
// Pada sensor SPI sesungguhnya, setiap register memiliki alamat
// dan bit R/W yang menentukan operasi baca/tulis
// ============================================================

// Bit R/W pada alamat register (MSB)
// BME280: bit 7 = 1 untuk READ, bit 7 = 0 untuk WRITE
#define SPI_READ_BIT        0x80    // OR dengan alamat untuk operasi baca
#define SPI_WRITE_BIT       0x00    // Mask untuk operasi tulis (bit 7 = 0)

// Alamat register sensor (simulasi BME280)
#define REG_CHIP_ID         0xD0    // Chip ID register
#define REG_CTRL_MEAS       0xF4    // Control measurement register
#define REG_CONFIG          0xF5    // Config register
#define REG_STATUS          0xF3    // Status register
#define REG_TEMP_MSB        0xFA    // Temperature MSB
#define REG_TEMP_LSB        0xFB    // Temperature LSB
#define REG_TEMP_XLSB       0xFC    // Temperature XLSB
#define REG_PRESS_MSB       0xF7    // Pressure MSB
#define REG_PRESS_LSB       0xF8    // Pressure LSB
#define REG_PRESS_XLSB      0xF9    // Pressure XLSB
#define REG_HUM_MSB         0xFD    // Humidity MSB
#define REG_HUM_LSB         0xFE    // Humidity LSB
#define REG_RESET           0xE0    // Reset register

// Nilai simulasi sensor
#define SIMULATED_CHIP_ID   0x60    // BME280 chip ID
#define RESET_VALUE         0xB6    // Soft reset value

// Jumlah pembacaan sensor berulang
#define SENSOR_READ_COUNT   5
#define SENSOR_READ_INTERVAL_MS  2000  // Interval antar pembacaan (ms)

#endif // CONFIG_H
