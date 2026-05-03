/**
 * ===========================================================================
 *  Program 09 : SPIFFS File System Operations
 *  Board      : ESP32 DevKit V1
 *  Framework  : ESP-IDF
 * ===========================================================================
 *
 *  Deskripsi:
 *  Program ini mendemonstrasikan penggunaan SPIFFS (SPI Flash File System)
 *  pada ESP32 menggunakan ESP-IDF framework. SPIFFS memungkinkan penyimpanan
 *  data secara persisten di flash internal ESP32.
 *
 *  Fitur yang didemonstrasikan:
 *  1. Mount/register SPIFFS partition
 *  2. Membuat dan menulis file teks
 *  3. Membaca file teks
 *  4. Append data ke file yang sudah ada
 *  5. Membuat file biner dan menulis struct data
 *  6. Membaca file biner (struct data)
 *  7. Listing semua file di SPIFFS (opendir/readdir)
 *  8. Menghapus file (unlink)
 *  9. Cek ruang penyimpanan (total/used/free)
 *  10. Unregister SPIFFS saat selesai
 *
 *  Koneksi Hardware:
 *  - Tidak ada koneksi eksternal (menggunakan flash internal)
 *
 *  Catatan:
 *  - Pastikan partition table memiliki partisi SPIFFS berlabel "storage"
 *  - default.csv sudah mencakup partisi SPIFFS
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_timer.h"

#include "config.h"

static const char *TAG = "SPIFFS_DEMO";

/* ==================== Helper Functions ==================== */

/**
 * @brief Print SPIFFS partition info (total and used bytes)
 */
static void print_spiffs_info(const char *label)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "=== Storage Info ===");
        ESP_LOGI(TAG, "  Total: %d bytes (%d KB)", total, total / 1024);
        ESP_LOGI(TAG, "  Used : %d bytes (%d KB)", used, used / 1024);
        ESP_LOGI(TAG, "  Free : %d bytes (%d KB)", total - used, (total - used) / 1024);
        ESP_LOGI(TAG, "  Usage: %.1f%%", (float)used / total * 100.0f);
        ESP_LOGI(TAG, "====================");
    } else {
        ESP_LOGE(TAG, "Failed to get SPIFFS info: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief Initialize and mount SPIFFS
 * @return ESP_OK on success
 */
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path              = SPIFFS_BASE_PATH,
        .partition_label        = SPIFFS_PARTITION_LABEL,
        .max_files              = SPIFFS_MAX_FILES,
        .format_if_mount_failed = SPIFFS_FORMAT_IF_FAILED
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'", SPIFFS_PARTITION_LABEL);
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // Check if SPIFFS was mounted successfully
    if (!esp_spiffs_mounted(SPIFFS_PARTITION_LABEL)) {
        ESP_LOGE(TAG, "SPIFFS is not mounted!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SPIFFS mounted successfully at '%s'", SPIFFS_BASE_PATH);
    return ESP_OK;
}

/**
 * @brief Demo 1: Create and write a text file
 */
static void demo_write_text_file(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 1: Write Text File");
    ESP_LOGI(TAG, "========================================");

    int64_t start = esp_timer_get_time();

    FILE *f = fopen(TEST_TEXT_FILE, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", strerror(errno));
        return;
    }

    // Write multiple lines
    fprintf(f, "=== SPIFFS Demo File ===\n");
    fprintf(f, "ESP32 SPIFFS File System Test\n");
    fprintf(f, "Framework: ESP-IDF\n");
    fprintf(f, "Modul 07: SPI & Storage\n");
    fprintf(f, "Line 5: Hello from ESP32!\n");

    fclose(f);

    int64_t elapsed = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "File '%s' written successfully", TEST_TEXT_FILE);
    ESP_LOGI(TAG, "Write time: %lld us", elapsed);
}

/**
 * @brief Demo 2: Read text file and print contents
 */
static void demo_read_text_file(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 2: Read Text File");
    ESP_LOGI(TAG, "========================================");

    int64_t start = esp_timer_get_time();

    FILE *f = fopen(TEST_TEXT_FILE, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", strerror(errno));
        return;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    ESP_LOGI(TAG, "File size: %ld bytes", fsize);
    ESP_LOGI(TAG, "--- File Contents ---");

    char line[128];
    int line_num = 0;
    while (fgets(line, sizeof(line), f) != NULL) {
        line_num++;
        // Remove trailing newline for cleaner output
        size_t len = strlen(line);
        if (len > 0 && line[line_num - 1 >= 0 ? len - 1 : 0] == '\n') {
            line[len - 1] = '\0';
        }
        ESP_LOGI(TAG, "  [%d] %s", line_num, line);
    }

    fclose(f);

    int64_t elapsed = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "--- End of File ---");
    ESP_LOGI(TAG, "Total lines: %d, Read time: %lld us", line_num, elapsed);
}

/**
 * @brief Demo 3: Append data to existing file
 */
static void demo_append_file(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 3: Append Data to File");
    ESP_LOGI(TAG, "========================================");

    for (int i = 0; i < APPEND_ITERATIONS; i++) {
        FILE *f = fopen(TEST_APPEND_FILE, "a");
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for appending");
            return;
        }
        fprintf(f, "Appended line %d - tick: %lu\n", i + 1, (unsigned long)xTaskGetTickCount());
        fclose(f);
        ESP_LOGI(TAG, "Appended line %d to file", i + 1);
    }

    // Verify by reading
    ESP_LOGI(TAG, "Verifying appended content...");
    demo_read_text_file();
}

/**
 * @brief Demo 4: Write binary data (struct) to file
 */
static void demo_write_binary_file(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 4: Write Binary File (Struct)");
    ESP_LOGI(TAG, "========================================");

    int64_t start = esp_timer_get_time();

    FILE *f = fopen(TEST_BINARY_FILE, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open binary file for writing");
        return;
    }

    // Write header: number of records
    uint32_t num_records = NUM_BINARY_RECORDS;
    fwrite(&num_records, sizeof(uint32_t), 1, f);

    // Write sensor records
    for (uint32_t i = 0; i < NUM_BINARY_RECORDS; i++) {
        sensor_record_t record = {
            .id          = i,
            .temperature = 20.0f + (float)i * 1.5f,
            .humidity    = 45.0f + (float)i * 2.0f,
            .timestamp   = (uint32_t)(esp_timer_get_time() / 1000),
        };
        snprintf(record.label, sizeof(record.label), "Sensor_%lu", (unsigned long)i);
        fwrite(&record, sizeof(sensor_record_t), 1, f);
        ESP_LOGI(TAG, "Written record %lu: T=%.1f°C, H=%.1f%%, label=%s",
                 (unsigned long)record.id, record.temperature, record.humidity, record.label);
    }

    fclose(f);

    int64_t elapsed = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "Binary file written: %d records (%d bytes), time: %lld us",
             NUM_BINARY_RECORDS,
             (int)(sizeof(uint32_t) + NUM_BINARY_RECORDS * sizeof(sensor_record_t)),
             elapsed);
}

/**
 * @brief Demo 5: Read binary data (struct) from file
 */
static void demo_read_binary_file(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 5: Read Binary File (Struct)");
    ESP_LOGI(TAG, "========================================");

    int64_t start = esp_timer_get_time();

    FILE *f = fopen(TEST_BINARY_FILE, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open binary file for reading");
        return;
    }

    // Read header
    uint32_t num_records = 0;
    fread(&num_records, sizeof(uint32_t), 1, f);
    ESP_LOGI(TAG, "Number of records in file: %lu", (unsigned long)num_records);

    // Read records
    ESP_LOGI(TAG, "%-4s %-10s %-10s %-12s %-12s", "ID", "Temp(°C)", "Hum(%)", "Timestamp", "Label");
    ESP_LOGI(TAG, "---- ---------- ---------- ------------ ------------");

    for (uint32_t i = 0; i < num_records; i++) {
        sensor_record_t record;
        size_t read_count = fread(&record, sizeof(sensor_record_t), 1, f);
        if (read_count != 1) {
            ESP_LOGE(TAG, "Failed to read record %lu", (unsigned long)i);
            break;
        }
        ESP_LOGI(TAG, "%-4lu %-10.1f %-10.1f %-12lu %-12s",
                 (unsigned long)record.id, record.temperature, record.humidity,
                 (unsigned long)record.timestamp, record.label);
    }

    fclose(f);

    int64_t elapsed = esp_timer_get_time() - start;
    ESP_LOGI(TAG, "Binary read complete. Time: %lld us", elapsed);
}

/**
 * @brief Demo 6: List all files in SPIFFS directory
 */
static void demo_list_files(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 6: List All Files in SPIFFS");
    ESP_LOGI(TAG, "========================================");

    DIR *dir = opendir(SPIFFS_BASE_PATH);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory '%s': %s", SPIFFS_BASE_PATH, strerror(errno));
        return;
    }

    int file_count = 0;
    size_t total_size = 0;
    struct dirent *entry;
    struct stat st;
    char filepath[512];

    ESP_LOGI(TAG, "%-30s %-10s", "Filename", "Size (bytes)");
    ESP_LOGI(TAG, "------------------------------ ----------");

    while ((entry = readdir(dir)) != NULL) {
        snprintf(filepath, sizeof(filepath), "%s/%s", SPIFFS_BASE_PATH, entry->d_name);

        if (stat(filepath, &st) == 0) {
            ESP_LOGI(TAG, "%-30s %-10ld", entry->d_name, (long)st.st_size);
            total_size += st.st_size;
        } else {
            ESP_LOGI(TAG, "%-30s %-10s", entry->d_name, "(unknown)");
        }
        file_count++;
    }

    closedir(dir);

    ESP_LOGI(TAG, "------------------------------ ----------");
    ESP_LOGI(TAG, "Total: %d files, %d bytes", file_count, total_size);
}

/**
 * @brief Demo 7: Delete a file
 */
static void demo_delete_file(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Demo 7: Delete File");
    ESP_LOGI(TAG, "========================================");

    ESP_LOGI(TAG, "Space before delete:");
    print_spiffs_info(SPIFFS_PARTITION_LABEL);

    // Check if file exists
    struct stat st;
    if (stat(TEST_BINARY_FILE, &st) == 0) {
        ESP_LOGI(TAG, "Deleting file: %s (size: %ld bytes)", TEST_BINARY_FILE, (long)st.st_size);

        if (unlink(TEST_BINARY_FILE) == 0) {
            ESP_LOGI(TAG, "File deleted successfully!");
        } else {
            ESP_LOGE(TAG, "Failed to delete file: %s", strerror(errno));
        }
    } else {
        ESP_LOGW(TAG, "File '%s' does not exist", TEST_BINARY_FILE);
    }

    ESP_LOGI(TAG, "Space after delete:");
    print_spiffs_info(SPIFFS_PARTITION_LABEL);

    // List remaining files
    ESP_LOGI(TAG, "Remaining files:");
    demo_list_files();
}

/* ==================== Main Entry Point ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Program 09: SPIFFS File System Demo    ║");
    ESP_LOGI(TAG, "║   Modul 07 - SPI & Storage               ║");
    ESP_LOGI(TAG, "║   Framework: ESP-IDF                      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    // Initialize SPIFFS
    esp_err_t ret = init_spiffs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS initialization failed! Halting.");
        return;
    }

    // Print initial storage info
    ESP_LOGI(TAG, "--- Initial Storage Status ---");
    print_spiffs_info(SPIFFS_PARTITION_LABEL);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 1: Write text file
    demo_write_text_file();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 2: Read text file
    demo_read_text_file();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 3: Append to file
    demo_append_file();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 4: Write binary file
    demo_write_binary_file();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 5: Read binary file
    demo_read_binary_file();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 6: List all files
    demo_list_files();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Print storage status after all writes
    ESP_LOGI(TAG, "--- Storage After All Operations ---");
    print_spiffs_info(SPIFFS_PARTITION_LABEL);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Demo 7: Delete a file and check space
    demo_delete_file();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Final storage status
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Final Storage Status ---");
    print_spiffs_info(SPIFFS_PARTITION_LABEL);

    // Unregister SPIFFS
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Unregistering SPIFFS...");
    ret = esp_vfs_spiffs_unregister(SPIFFS_PARTITION_LABEL);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS unregistered successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to unregister SPIFFS: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  All SPIFFS demos completed!");
    ESP_LOGI(TAG, "========================================");
}
