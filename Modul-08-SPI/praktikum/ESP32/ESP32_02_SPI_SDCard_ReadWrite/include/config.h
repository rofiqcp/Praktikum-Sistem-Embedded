#ifndef CONFIG_H
#define CONFIG_H

// SD Card Pin Definitions (SPI mode)
#define SD_MOSI_PIN     13
#define SD_MISO_PIN     12
#define SD_SCLK_PIN     14
#define SD_CS_PIN       15

// SPI Configuration
#define SD_SPI_HOST     SPI2_HOST
#define SD_SPI_CLOCK    20000000  // 20 MHz

// File paths
#define MOUNT_POINT     "/sdcard"
#define TEST_FILE       MOUNT_POINT"/test.txt"
#define DATA_FILE       MOUNT_POINT"/data.csv"

// Buffer sizes
#define READ_BUFFER_SIZE    512
#define WRITE_BUFFER_SIZE   512

#endif // CONFIG_H
