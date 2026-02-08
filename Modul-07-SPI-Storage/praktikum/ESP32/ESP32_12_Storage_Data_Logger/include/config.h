/**
 * @file config.h
 * @brief Configuration for Storage Data Logger
 * 
 * Modul 07 - SPI & Storage
 * Program 12: Storage Data Logger
 * 
 * Platform: ESP32 (ESP-IDF)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ==================== SPIFFS Configuration ==================== */
#define SPIFFS_PARTITION_LABEL   "storage"
#define SPIFFS_BASE_PATH         "/spiffs"
#define SPIFFS_MAX_FILES         5
#define SPIFFS_FORMAT_IF_FAILED  true

/* ==================== Log File Configuration ==================== */
#define LOG_FILE_PATH            "/spiffs/datalog.csv"
#define LOG_FILE_BACKUP          "/spiffs/datalog.bak"
#define LOG_FILE_HEADER          "timestamp,temperature,humidity,adc_value\n"
#define MAX_FILE_SIZE            (100 * 1024)   // 100 KB max file size

/* ==================== Circular Buffer ==================== */
#define CIRCULAR_BUFFER_SIZE     64             // Number of entries in ring buffer
#define FLUSH_THRESHOLD          10             // Flush to file every N entries

/* ==================== Logging Intervals ==================== */
#define LOG_INTERVAL_MS          1000           // Data collection interval
#define STATS_INTERVAL_MS        10000          // Statistics print interval
#define PRINT_LAST_N_ENTRIES     5              // Print last N entries from file

/* ==================== Simulation Parameters ==================== */
#define TEMP_BASE                25.0f          // Base temperature (°C)
#define TEMP_AMPLITUDE           10.0f          // Temperature sine wave amplitude
#define TEMP_PERIOD_MS           60000          // Temperature cycle period (60s)
#define HUMIDITY_MIN             30.0f          // Min humidity (%)
#define HUMIDITY_MAX             80.0f          // Max humidity (%)
#define ADC_MAX_VALUE            4095           // 12-bit ADC max
#define ADC_SAW_PERIOD_MS        30000          // Sawtooth period (30s)

/* ==================== Data Entry Struct ==================== */
typedef struct {
    uint32_t timestamp;          // Milliseconds since boot
    float    temperature;        // Temperature in °C (simulated sine wave)
    float    humidity;           // Humidity in % (simulated random)
    uint16_t adc_value;          // ADC value 0-4095 (simulated sawtooth)
} log_entry_t;

/* ==================== Task Configuration ==================== */
#define COLLECTOR_TASK_STACK     4096
#define COLLECTOR_TASK_PRIORITY  5
#define WRITER_TASK_STACK        4096
#define WRITER_TASK_PRIORITY     4

#endif /* CONFIG_H */
