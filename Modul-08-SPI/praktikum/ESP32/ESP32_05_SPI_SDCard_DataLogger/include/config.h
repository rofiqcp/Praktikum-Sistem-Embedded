#ifndef CONFIG_H
#define CONFIG_H

// SPI Pin Definitions
#define PIN_NUM_MOSI    13
#define PIN_NUM_MISO    12
#define PIN_NUM_CLK     14

// SD Card Pin Definitions - aliases
#define SD_MOSI_PIN     PIN_NUM_MOSI
#define SD_MISO_PIN     PIN_NUM_MISO
#define SD_SCLK_PIN     PIN_NUM_CLK

// SD Card Pin Definitions
#define PIN_NUM_SD_CS   5
#define SD_CS_PIN       PIN_NUM_SD_CS

// SD Card Configuration
#define SD_SPI_CLOCK    20000000  // 20 MHz
#define MOUNT_POINT     "/sdcard"

// Data Logger Configuration
#define LOG_FILE_PATH   "/sdcard/datalog.csv"
#define LOG_FILE        LOG_FILE_PATH
#define LOG_INTERVAL_MS 5000      // Log every 5 seconds
#define MAX_LOG_SIZE    1048576   // 1 MB max file size

// Sensor Simulation
#define TEMP_MIN        20.0f
#define TEMP_MAX        35.0f
#define HUMIDITY_MIN    40.0f
#define HUMIDITY_MAX    80.0f
#define PRESSURE_MIN    980.0f
#define PRESSURE_MAX    1020.0f

#endif // CONFIG_H
