/**
 * @file config.h
 * @brief Configuration for SPI SD Card with FAT Filesystem
 *
 * Hardware: ESP32 DevKit V1 + SD Card Module (SPI mode)
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

// SPI Pin definitions for SD Card
#define PIN_NUM_MOSI        23
#define PIN_NUM_MISO        19
#define PIN_NUM_SCLK        18
#define PIN_NUM_CS          5

// SD Card mount configuration
#define MOUNT_POINT         "/sdcard"

// File test configuration
#define TEST_FILENAME       MOUNT_POINT "/test.txt"
#define TEST_DIR            MOUNT_POINT

// SD Card SPI configuration
#define SD_SPI_MAX_FREQ     (400)           // Max frequency in kHz during init
#define SD_MAX_OPEN_FILES   5               // Max simultaneously open files

#endif // CONFIG_H
