#ifndef CONFIG_H
#define CONFIG_H

// SPI1 Pins (Hardware SPI)
#define SPI_SCK_PIN   PA5
#define SPI_MISO_PIN  PA6
#define SPI_MOSI_PIN  PA7

// SD Card Pins
#define SD_CS_PIN     PA4

// Sensor pins (simulated)
#define TEMP_SENSOR_PIN   PA0  // ADC
#define LIGHT_SENSOR_PIN  PA1  // ADC
#define BUTTON_PIN        PB0  // Digital input

// Data logging configuration
#define LOG_INTERVAL      5000  // 5 seconds
#define MAX_LOG_SIZE      1000  // Maximum entries per file
#define DATA_DIR          "/logs"
#define CONFIG_FILE       "/config.txt"

// Sensor calibration
#define TEMP_OFFSET       0.0
#define TEMP_SCALE        100.0
#define LIGHT_SCALE       1023.0

#endif // CONFIG_H
