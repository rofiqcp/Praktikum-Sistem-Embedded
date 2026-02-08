/**
 * @file config.h
 * @brief Configuration for SPIFFS File System Demo
 * 
 * Modul 07 - SPI & Storage
 * Program 09: SPIFFS File System Operations
 * 
 * Platform: ESP32 (ESP-IDF)
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPIFFS Configuration ==================== */
#define SPIFFS_PARTITION_LABEL   "storage"        // Partition label in partition table
#define SPIFFS_BASE_PATH         "/spiffs"         // VFS mount point
#define SPIFFS_MAX_FILES         5                 // Maximum number of open files
#define SPIFFS_FORMAT_IF_FAILED  true              // Format partition if mount fails

/* ==================== Test File Paths ==================== */
#define TEST_TEXT_FILE           "/spiffs/test.txt"
#define TEST_BINARY_FILE        "/spiffs/data.bin"
#define TEST_APPEND_FILE        "/spiffs/test.txt"

/* ==================== Binary Data Struct ==================== */
typedef struct {
    uint32_t id;
    float    temperature;
    float    humidity;
    uint32_t timestamp;
    char     label[16];
} sensor_record_t;

/* ==================== Demo Settings ==================== */
#define NUM_BINARY_RECORDS       5                 // Number of binary records to write
#define APPEND_ITERATIONS        3                 // Number of append operations

#endif /* CONFIG_H */
