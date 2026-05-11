/**
 * ESP32_02_SPI_SDCard_ReadWrite
 * SD Card read/write operations via SPI
 * 
 * Features:
 * - Initialize SD Card via SPI interface
 * - Create and write files
 * - Read file contents
 * - List directory contents
 * - File size and info display
 */

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "config.h"

static const char *TAG = "SD_CARD";

static sdmmc_card_t *card;

// Initialize SD Card
static esp_err_t sd_card_init(void) {
    ESP_LOGI(TAG, "Initializing SD card...");
    
    esp_err_t ret;
    
    // Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    // Initialize SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize SD card using SPI peripheral
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SD_SPI_HOST;
    
    ESP_LOGI(TAG, "Mounting filesystem...");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }
    
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    
    ESP_LOGI(TAG, "SD card mounted successfully");
    return ESP_OK;
}

// Write data to file
static esp_err_t write_file(const char *path, const char *data) {
    ESP_LOGI(TAG, "Writing to file: %s", path);
    
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    
    fprintf(f, "%s", data);
    fclose(f);
    
    ESP_LOGI(TAG, "File written successfully");
    return ESP_OK;
}

// Append data to file
static esp_err_t append_file(const char *path, const char *data) {
    ESP_LOGI(TAG, "Appending to file: %s", path);
    
    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending");
        return ESP_FAIL;
    }
    
    fprintf(f, "%s", data);
    fclose(f);
    
    ESP_LOGI(TAG, "Data appended successfully");
    return ESP_OK;
}

// Read file contents
static esp_err_t read_file(const char *path) {
    ESP_LOGI(TAG, "Reading file: %s", path);
    
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    
    char line[READ_BUFFER_SIZE];
    ESP_LOGI(TAG, "File contents:");
    printf("========================================\n");
    while (fgets(line, sizeof(line), f) != NULL) {
        // Strip newline
        char *pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        printf("%s\n", line);
    }
    printf("========================================\n");
    
    fclose(f);
    ESP_LOGI(TAG, "File read successfully");
    return ESP_OK;
}

// Get file size
static long get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

// Delete file
static esp_err_t delete_file(const char *path) {
    ESP_LOGI(TAG, "Deleting file: %s", path);
    
    if (unlink(path) == 0) {
        ESP_LOGI(TAG, "File deleted successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to delete file");
        return ESP_FAIL;
    }
}

// Rename file
static esp_err_t rename_file(const char *old_path, const char *new_path) {
    ESP_LOGI(TAG, "Renaming file from %s to %s", old_path, new_path);
    
    if (rename(old_path, new_path) == 0) {
        ESP_LOGI(TAG, "File renamed successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to rename file");
        return ESP_FAIL;
    }
}

// Test SD card operations
static void test_sd_operations(void) {
    ESP_LOGI(TAG, "Starting SD card operations test...");
    
    // Test 1: Write to file
    ESP_LOGI(TAG, "\n=== Test 1: Write to file ===");
    const char *test_data = "Hello, ESP32!\nThis is a test file.\nSD Card via SPI works!\n";
    write_file(TEST_FILE, test_data);
    
    // Test 2: Read file
    ESP_LOGI(TAG, "\n=== Test 2: Read file ===");
    read_file(TEST_FILE);
    
    // Test 3: Get file size
    ESP_LOGI(TAG, "\n=== Test 3: Get file size ===");
    long size = get_file_size(TEST_FILE);
    if (size >= 0) {
        ESP_LOGI(TAG, "File size: %ld bytes", size);
    }
    
    // Test 4: Append to file
    ESP_LOGI(TAG, "\n=== Test 4: Append to file ===");
    append_file(TEST_FILE, "Appended line 1\n");
    append_file(TEST_FILE, "Appended line 2\n");
    read_file(TEST_FILE);
    
    // Test 5: Create CSV file
    ESP_LOGI(TAG, "\n=== Test 5: Create CSV file ===");
    write_file(DATA_FILE, "Timestamp,Temperature,Humidity\n");
    append_file(DATA_FILE, "2024-01-01 10:00:00,25.5,60.2\n");
    append_file(DATA_FILE, "2024-01-01 10:01:00,25.7,59.8\n");
    append_file(DATA_FILE, "2024-01-01 10:02:00,25.6,60.0\n");
    read_file(DATA_FILE);
    
    // Test 6: Rename file
    ESP_LOGI(TAG, "\n=== Test 6: Rename file ===");
    const char *new_name = MOUNT_POINT"/renamed.txt";
    rename_file(TEST_FILE, new_name);
    read_file(new_name);
    
    // Test 7: Delete file
    ESP_LOGI(TAG, "\n=== Test 7: Delete file ===");
    delete_file(new_name);
    
    ESP_LOGI(TAG, "\nAll tests completed!");
}

// Continuous write test
static void continuous_write_test(void) {
    ESP_LOGI(TAG, "Starting continuous write test...");
    
    const char *log_file = MOUNT_POINT"/log.txt";
    int counter = 0;
    
    while (1) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Log entry %d - Timestamp: %lu\n", 
                 counter++, (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        
        append_file(log_file, buffer);
        ESP_LOGI(TAG, "Written: %s", buffer);
        
        // Display file size every 10 entries
        if (counter % 10 == 0) {
            long size = get_file_size(log_file);
            ESP_LOGI(TAG, "Current log file size: %ld bytes", size);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SPI SD Card Read/Write Demo");
    
    // Initialize SD card
    esp_err_t ret = sd_card_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card initialization failed!");
        return;
    }
    
    // Run tests
    test_sd_operations();
    
    // Wait before starting continuous test
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Start continuous write test
    continuous_write_test();
    
    // Unmount filesystem (never reached in this example)
    // esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    // spi_bus_free(SD_SPI_HOST);
}
