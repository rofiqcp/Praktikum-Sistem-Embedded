/**
 * @file config.h
 * @brief Configuration for SPI Flash W25Q32 Driver
 *
 * Hardware: ESP32 DevKit V1 + W25Q32 SPI Flash (32Mbit / 4MB)
 *
 * Pin Mapping:
 *   MOSI -> GPIO23
 *   MISO -> GPIO19
 *   SCLK -> GPIO18
 *   CS   -> GPIO5
 */

#ifndef CONFIG_H
#define CONFIG_H

// SPI Host selection
#define SPI_HOST_ID         SPI2_HOST       // HSPI

// SPI Pin definitions
#define PIN_NUM_MOSI        23
#define PIN_NUM_MISO        19
#define PIN_NUM_SCLK        18
#define PIN_NUM_CS          5

// SPI Configuration
#define SPI_CLOCK_SPEED     (10 * 1000 * 1000)  // 10 MHz

// W25Q32 Flash Commands
#define W25Q_CMD_WRITE_ENABLE       0x06    // WREN - Write Enable
#define W25Q_CMD_WRITE_DISABLE      0x04    // WRDI - Write Disable
#define W25Q_CMD_READ_STATUS        0x05    // RDSR - Read Status Register
#define W25Q_CMD_WRITE_STATUS       0x01    // WRSR - Write Status Register
#define W25Q_CMD_READ_DATA          0x03    // READ - Read Data
#define W25Q_CMD_PAGE_PROGRAM       0x02    // PP   - Page Program (write)
#define W25Q_CMD_SECTOR_ERASE       0x20    // SE   - Sector Erase (4KB)
#define W25Q_CMD_BLOCK_ERASE_32K    0x52    // BE32 - Block Erase (32KB)
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8    // BE64 - Block Erase (64KB)
#define W25Q_CMD_CHIP_ERASE         0xC7    // CE   - Chip Erase
#define W25Q_CMD_READ_JEDEC_ID      0x9F    // RDID - Read JEDEC ID
#define W25Q_CMD_POWER_DOWN         0xB9    // DP   - Power Down
#define W25Q_CMD_RELEASE_PD         0xAB    // RES  - Release Power Down

// W25Q32 Status Register Bits
#define W25Q_STATUS_WIP             0x01    // Write In Progress
#define W25Q_STATUS_WEL             0x02    // Write Enable Latch

// W25Q32 Expected JEDEC ID
#define W25Q_MANUFACTURER_ID        0xEF    // Winbond
#define W25Q_DEVICE_ID_MSB          0x40    // W25Q series
#define W25Q_DEVICE_ID_LSB          0x16    // W25Q32 (32Mbit)

// Flash characteristics
#define W25Q_PAGE_SIZE              256     // 256 bytes per page
#define W25Q_SECTOR_SIZE            4096    // 4KB per sector
#define W25Q_BLOCK_SIZE_32K         32768   // 32KB per block
#define W25Q_BLOCK_SIZE_64K         65536   // 64KB per block
#define W25Q_TOTAL_SIZE             (4 * 1024 * 1024)  // 4MB total

// Timeout
#define W25Q_TIMEOUT_MS             3000    // Max wait for operations

// Test configuration
#define TEST_SECTOR_ADDR            0x000000    // Test at sector 0
#define TEST_DATA_SIZE              64          // Bytes to test

#endif // CONFIG_H
