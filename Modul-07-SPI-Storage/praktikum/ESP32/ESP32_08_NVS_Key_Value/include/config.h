/**
 * ==========================================================================
 * FILE        : config.h
 * PROJECT     : ESP32_08_NVS_Key_Value
 * MODUL       : 07 - SPI & Storage
 * DESCRIPTION : Configuration for Non-Volatile Storage (NVS) key-value
 *               operations using ESP-IDF NVS API.
 * 
 * NOTE: NVS uses internal flash partition, no external hardware needed.
 *       Make sure the partition table includes an "nvs" partition.
 *       Default partition table already includes NVS.
 * ==========================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== NVS Namespace ============================= */
#define NVS_NAMESPACE       "storage"       // NVS namespace for this app

/* ======================== NVS Keys ================================== */
// Integer keys
#define KEY_BOOT_COUNT      "boot_count"    // int32_t: boot counter
#define KEY_THRESHOLD       "threshold"     // int32_t: threshold * 100 (stored as int)

// String keys
#define KEY_DEVICE_NAME     "device_name"   // string: device name
#define KEY_WIFI_SSID       "wifi_ssid"     // string: WiFi SSID

// Blob keys
#define KEY_BLOB_DATA       "blob_data"     // blob: calibration data struct

/* ======================== Default Values ============================= */
#define DEFAULT_DEVICE_NAME "ESP32-Node-01"
#define DEFAULT_WIFI_SSID   "MyNetwork"
#define DEFAULT_THRESHOLD   25.50f          // Will be stored as 2550 (int)
#define DEFAULT_BOOT_COUNT  0

/* ======================== Storage Limits ============================= */
#define MAX_STRING_LENGTH   64              // Max string value length
#define MAX_BLOB_SIZE       256             // Max blob data size

/* ======================== Calibration Data Structure ================ */
typedef struct {
    float   offset;         // Calibration offset
    float   gain;           // Calibration gain
    uint8_t channel;        // Channel number
    uint8_t valid;          // Validity flag
    uint32_t timestamp;     // Calibration timestamp (epoch)
    char    operator[16];   // Operator name
} calibration_data_t;

#endif // CONFIG_H
