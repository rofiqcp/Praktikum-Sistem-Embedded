/**
 * ==========================================================================
 * FILE        : main.c
 * PROJECT     : ESP32_08_NVS_Key_Value
 * MODUL       : 07 - SPI & Storage
 * BOARD       : ESP32 DevKit V1
 * FRAMEWORK   : ESP-IDF
 * 
 * DESCRIPTION : Demonstrate Non-Volatile Storage (NVS) key-value operations
 *               using ESP-IDF NVS Flash API. Shows how to store and retrieve
 *               integers, strings, and blob data that persists across reboots.
 *               Includes a boot counter, key enumeration, and erase operations.
 * 
 * HARDWARE CONNECTIONS:
 *   None - NVS uses internal flash, no external hardware required.
 *   Connect ESP32 via USB for serial output.
 * 
 * NVS FEATURES DEMONSTRATED:
 *   1. Boot counter (int32) - persists across resets
 *   2. String storage (device name, WiFi SSID)
 *   3. Float as integer (threshold * 100)
 *   4. Blob storage (calibration struct)
 *   5. Key enumeration (list all keys)
 *   6. Erase specific key
 *   7. Erase entire namespace
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_system.h"
#include "config.h"

static const char *TAG = "NVS_KV";

/**
 * @brief Initialize NVS Flash with error recovery
 * 
 * If NVS partition is truncated or has changed format,
 * erase it and re-initialize.
 */
static esp_err_t nvs_init(void)
{
    ESP_LOGI(TAG, "Initializing NVS Flash...");

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition issue (%s), erasing and re-initializing...",
                 esp_err_to_name(ret));
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS Flash initialized successfully");
    } else {
        ESP_LOGE(TAG, "NVS Flash init failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

/**
 * @brief Demo 1: Boot counter (persists across resets)
 * 
 * Reads the current boot count from NVS, increments it,
 * and writes it back. This value survives power cycles.
 */
static void demo_boot_counter(nvs_handle_t handle)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 1: Boot Counter ===");

    int32_t boot_count = DEFAULT_BOOT_COUNT;
    esp_err_t ret;

    /* Read current boot count */
    ret = nvs_get_i32(handle, KEY_BOOT_COUNT, &boot_count);
    switch (ret) {
        case ESP_OK:
            ESP_LOGI(TAG, "Current boot count: %d", (int)boot_count);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "Boot count not found, starting from %d", (int)boot_count);
            break;
        default:
            ESP_LOGE(TAG, "Error reading boot count: %s", esp_err_to_name(ret));
            return;
    }

    /* Increment and store */
    boot_count++;
    ret = nvs_set_i32(handle, KEY_BOOT_COUNT, boot_count);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Boot count updated to: %d", (int)boot_count);
    } else {
        ESP_LOGE(TAG, "Failed to write boot count: %s", esp_err_to_name(ret));
    }

    /* Commit to flash */
    ret = nvs_commit(handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Boot count committed to NVS");
    } else {
        ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(ret));
    }

    printf("NVS:BOOT_COUNT=%d,STATUS=OK\n", (int)boot_count);
}

/**
 * @brief Demo 2: Store and read string values
 */
static void demo_string_storage(nvs_handle_t handle)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 2: String Storage ===");

    esp_err_t ret;
    char read_buf[MAX_STRING_LENGTH];
    size_t buf_len;

    /* --- Device Name --- */
    /* Try to read existing value */
    buf_len = sizeof(read_buf);
    ret = nvs_get_str(handle, KEY_DEVICE_NAME, read_buf, &buf_len);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* Key doesn't exist yet, store default */
        ESP_LOGW(TAG, "Device name not found, storing default: %s", DEFAULT_DEVICE_NAME);
        ret = nvs_set_str(handle, KEY_DEVICE_NAME, DEFAULT_DEVICE_NAME);
        if (ret == ESP_OK) {
            strncpy(read_buf, DEFAULT_DEVICE_NAME, sizeof(read_buf));
            ESP_LOGI(TAG, "Stored device name: %s", read_buf);
        }
    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Read device name: \"%s\" (len=%d)", read_buf, (int)buf_len);
    } else {
        ESP_LOGE(TAG, "Error reading device name: %s", esp_err_to_name(ret));
    }
    printf("NVS:DEVICE_NAME=%s,STATUS=%s\n", read_buf, esp_err_to_name(ret));

    /* --- WiFi SSID --- */
    buf_len = sizeof(read_buf);
    ret = nvs_get_str(handle, KEY_WIFI_SSID, read_buf, &buf_len);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "WiFi SSID not found, storing default: %s", DEFAULT_WIFI_SSID);
        ret = nvs_set_str(handle, KEY_WIFI_SSID, DEFAULT_WIFI_SSID);
        if (ret == ESP_OK) {
            strncpy(read_buf, DEFAULT_WIFI_SSID, sizeof(read_buf));
            ESP_LOGI(TAG, "Stored WiFi SSID: %s", read_buf);
        }
    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Read WiFi SSID: \"%s\" (len=%d)", read_buf, (int)buf_len);
    } else {
        ESP_LOGE(TAG, "Error reading WiFi SSID: %s", esp_err_to_name(ret));
    }
    printf("NVS:WIFI_SSID=%s,STATUS=%s\n", read_buf, esp_err_to_name(ret));

    nvs_commit(handle);
}

/**
 * @brief Demo 3: Store float as integer (threshold * 100)
 */
static void demo_integer_storage(nvs_handle_t handle)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 3: Integer/Float Storage ===");

    esp_err_t ret;
    int32_t stored_threshold = 0;

    /* Read threshold (stored as int * 100) */
    ret = nvs_get_i32(handle, KEY_THRESHOLD, &stored_threshold);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* Store default threshold */
        stored_threshold = (int32_t)(DEFAULT_THRESHOLD * 100.0f);
        ESP_LOGW(TAG, "Threshold not found, storing default: %.2f (as %d)",
                 DEFAULT_THRESHOLD, (int)stored_threshold);
        ret = nvs_set_i32(handle, KEY_THRESHOLD, stored_threshold);
    } else if (ret == ESP_OK) {
        float actual = (float)stored_threshold / 100.0f;
        ESP_LOGI(TAG, "Read threshold: stored=%d, actual=%.2f",
                 (int)stored_threshold, actual);
    } else {
        ESP_LOGE(TAG, "Error reading threshold: %s", esp_err_to_name(ret));
        return;
    }

    /* Modify threshold: increase by 0.5 */
    stored_threshold += 50;  // Add 0.50
    ret = nvs_set_i32(handle, KEY_THRESHOLD, stored_threshold);
    if (ret == ESP_OK) {
        float actual = (float)stored_threshold / 100.0f;
        ESP_LOGI(TAG, "Updated threshold: stored=%d, actual=%.2f",
                 (int)stored_threshold, actual);
    }

    nvs_commit(handle);
    printf("NVS:THRESHOLD=%.2f,RAW=%d,STATUS=%s\n",
           (float)stored_threshold / 100.0f, (int)stored_threshold, esp_err_to_name(ret));
}

/**
 * @brief Demo 4: Store and read blob data (struct)
 */
static void demo_blob_storage(nvs_handle_t handle)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 4: Blob Storage (Calibration Data) ===");

    esp_err_t ret;
    calibration_data_t cal_data;
    size_t blob_size = sizeof(calibration_data_t);

    /* Try to read existing calibration data */
    ret = nvs_get_blob(handle, KEY_BLOB_DATA, &cal_data, &blob_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        /* Create and store default calibration data */
        ESP_LOGW(TAG, "Calibration data not found, creating defaults...");

        cal_data.offset = 0.125f;
        cal_data.gain = 1.005f;
        cal_data.channel = 0;
        cal_data.valid = 1;
        cal_data.timestamp = 1738886400;  // Example epoch timestamp
        strncpy(cal_data.operator, "Technician-A", sizeof(cal_data.operator));

        ret = nvs_set_blob(handle, KEY_BLOB_DATA, &cal_data, sizeof(calibration_data_t));
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Calibration data stored successfully");
        } else {
            ESP_LOGE(TAG, "Failed to store calibration data: %s", esp_err_to_name(ret));
            return;
        }
    } else if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration data loaded from NVS (size=%d bytes)", (int)blob_size);
    } else {
        ESP_LOGE(TAG, "Error reading blob: %s", esp_err_to_name(ret));
        return;
    }

    /* Display calibration data */
    ESP_LOGI(TAG, "  Offset    : %.4f", cal_data.offset);
    ESP_LOGI(TAG, "  Gain      : %.4f", cal_data.gain);
    ESP_LOGI(TAG, "  Channel   : %d", cal_data.channel);
    ESP_LOGI(TAG, "  Valid     : %s", cal_data.valid ? "YES" : "NO");
    ESP_LOGI(TAG, "  Timestamp : %u", (unsigned)cal_data.timestamp);
    ESP_LOGI(TAG, "  Operator  : %s", cal_data.operator);

    printf("NVS:BLOB=CAL,OFFSET=%.4f,GAIN=%.4f,CH=%d,VALID=%d,STATUS=%s\n",
           cal_data.offset, cal_data.gain, cal_data.channel,
           cal_data.valid, esp_err_to_name(ret));

    nvs_commit(handle);
}

/**
 * @brief Demo 5: List all keys in namespace using nvs_entry_find/info
 */
static void demo_list_keys(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 5: List All Keys in Namespace ===");

    nvs_iterator_t it = NULL;
    esp_err_t ret = nvs_entry_find("nvs", NVS_NAMESPACE, NVS_TYPE_ANY, &it);

    int key_count = 0;

    printf("\n");
    printf("╔═══════════════════╦══════════════╗\n");
    printf("║       Key         ║     Type     ║\n");
    printf("╠═══════════════════╬══════════════╣\n");

    while (ret == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        const char *type_str = "UNKNOWN";
        switch (info.type) {
            case NVS_TYPE_U8:   type_str = "UINT8";  break;
            case NVS_TYPE_I8:   type_str = "INT8";   break;
            case NVS_TYPE_U16:  type_str = "UINT16"; break;
            case NVS_TYPE_I16:  type_str = "INT16";  break;
            case NVS_TYPE_U32:  type_str = "UINT32"; break;
            case NVS_TYPE_I32:  type_str = "INT32";  break;
            case NVS_TYPE_U64:  type_str = "UINT64"; break;
            case NVS_TYPE_I64:  type_str = "INT64";  break;
            case NVS_TYPE_STR:  type_str = "STRING"; break;
            case NVS_TYPE_BLOB: type_str = "BLOB";   break;
            default: break;
        }

        printf("║ %-17s ║ %-12s ║\n", info.key, type_str);
        key_count++;

        printf("NVS:KEY=%s,TYPE=%s\n", info.key, type_str);

        ret = nvs_entry_next(&it);
    }

    printf("╚═══════════════════╩══════════════╝\n");
    ESP_LOGI(TAG, "Total keys in namespace \"%s\": %d", NVS_NAMESPACE, key_count);

    nvs_release_iterator(it);
}

/**
 * @brief Demo 6: Erase a specific key
 */
static void demo_erase_key(nvs_handle_t handle, const char *key)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 6: Erase Specific Key ===");

    esp_err_t ret = nvs_erase_key(handle, key);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Key \"%s\" erased successfully", key);
        nvs_commit(handle);
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Key \"%s\" not found (already erased?)", key);
    } else {
        ESP_LOGE(TAG, "Failed to erase key \"%s\": %s", key, esp_err_to_name(ret));
    }

    printf("NVS:ERASE_KEY=%s,STATUS=%s\n", key, esp_err_to_name(ret));
}

/**
 * @brief Demo 7: Erase all keys in namespace
 */
static void demo_erase_all(nvs_handle_t handle)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Demo 7: Erase All Keys in Namespace ===");

    esp_err_t ret = nvs_erase_all(handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "All keys in namespace \"%s\" erased", NVS_NAMESPACE);
        nvs_commit(handle);
    } else {
        ESP_LOGE(TAG, "Failed to erase all keys: %s", esp_err_to_name(ret));
    }

    printf("NVS:ERASE_ALL=%s,STATUS=%s\n", NVS_NAMESPACE, esp_err_to_name(ret));
}

/**
 * @brief Print NVS partition statistics
 */
static void print_nvs_stats(void)
{
    nvs_stats_t nvs_stats;
    esp_err_t ret = nvs_get_stats("nvs", &nvs_stats);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "=== NVS Partition Statistics ===");
        ESP_LOGI(TAG, "  Used entries  : %d", (int)nvs_stats.used_entries);
        ESP_LOGI(TAG, "  Free entries  : %d", (int)nvs_stats.free_entries);
        ESP_LOGI(TAG, "  Total entries : %d", (int)nvs_stats.total_entries);
        ESP_LOGI(TAG, "  Namespace count: %d", (int)nvs_stats.namespace_count);

        printf("NVS:STATS,USED=%d,FREE=%d,TOTAL=%d,NS=%d\n",
               (int)nvs_stats.used_entries, (int)nvs_stats.free_entries,
               (int)nvs_stats.total_entries, (int)nvs_stats.namespace_count);
    }
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 Non-Volatile Storage (NVS) Demo");
    ESP_LOGI(TAG, "========================================");

    /* Initialize NVS Flash */
    esp_err_t ret = nvs_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed! Halting.");
        return;
    }

    /* Open NVS handle with namespace */
    nvs_handle_t nvs_handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "NVS handle opened for namespace: \"%s\"", NVS_NAMESPACE);

    /* ===== Run all demos ===== */

    /* Demo 1: Boot counter */
    demo_boot_counter(nvs_handle);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Demo 2: String storage */
    demo_string_storage(nvs_handle);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Demo 3: Integer/Float storage */
    demo_integer_storage(nvs_handle);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Demo 4: Blob storage */
    demo_blob_storage(nvs_handle);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Demo 5: List all keys */
    demo_list_keys();
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Print NVS statistics */
    print_nvs_stats();
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Demo 6: Erase a specific key (WiFi SSID as example) */
    demo_erase_key(nvs_handle, KEY_WIFI_SSID);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* List keys after erase to verify */
    ESP_LOGI(TAG, "--- Keys after erasing \"%s\" ---", KEY_WIFI_SSID);
    demo_list_keys();

    /* Note: Demo 7 (erase all) is commented out to preserve data */
    /* Uncomment the line below to erase all keys: */
    // demo_erase_all(nvs_handle);

    /* Close NVS handle */
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "NVS handle closed");

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " NVS Demo Complete");
    ESP_LOGI(TAG, " Reset the board to see boot count increment!");
    ESP_LOGI(TAG, "========================================");

    /* Keep main task alive */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
