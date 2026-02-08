/**
 * @file main.c
 * @brief ESP32 SPI SD Card with FAT Filesystem
 * @details Mounts an SD card via SPI, performs file create/read/write/list
 *          operations using ESP-IDF VFS FAT filesystem driver.
 *
 * Program: ESP32_04_SPI_SD_Card
 * Module:  07 - SPI & Storage
 *
 * Hardware Connections:
 *   ESP32 GPIO23 (MOSI) --> SD Card Module MOSI
 *   ESP32 GPIO19 (MISO) --> SD Card Module MISO
 *   ESP32 GPIO18 (SCLK) --> SD Card Module SCK/CLK
 *   ESP32 GPIO5  (CS)   --> SD Card Module CS
 *   ESP32 3.3V          --> SD Card Module VCC
 *   ESP32 GND           --> SD Card Module GND
 *
 * Pin Mapping:
 *   MOSI = GPIO23
 *   MISO = GPIO19
 *   SCLK = GPIO18
 *   CS   = GPIO5
 *
 * Description:
 *   - Mounts an SD card via SPI using esp_vfs_fat_sdspi_mount()
 *   - Prints card information (name, type, capacity)
 *   - Creates and writes data to /sdcard/test.txt
 *   - Reads the file back and verifies contents
 *   - Lists directory contents of /sdcard/
 *   - Unmounts the SD card
 *
 * Framework: ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "config.h"

static const char *TAG = "SD_CARD";

static sdmmc_card_t *card = NULL;

/**
 * @brief Mount SD card via SPI
 */
static esp_err_t sd_card_mount(void)
{
    ESP_LOGI(TAG, "Initializing SD card in SPI mode...");

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // SD SPI device configuration
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI_HOST_ID;

    // FAT filesystem mount configuration
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = SD_MAX_OPEN_FILES,
        .allocation_unit_size = 16 * 1024,
    };

    // Host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI_HOST_ID;

    ESP_LOGI(TAG, "Mounting FAT filesystem at %s...", MOUNT_POINT);

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card is inserted and wired correctly.", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted successfully at %s", MOUNT_POINT);
    return ESP_OK;
}

/**
 * @brief Print SD card information
 */
static void sd_card_print_info(void)
{
    ESP_LOGI(TAG, "--- SD Card Information ---");
    sdmmc_card_print_info(stdout, card);

    ESP_LOGI(TAG, "Card name: %s", card->cid.name);

    if (card->is_sdio) {
        ESP_LOGI(TAG, "Type: SDIO");
    } else if (card->is_mmc) {
        ESP_LOGI(TAG, "Type: MMC");
    } else {
        ESP_LOGI(TAG, "Type: %s", (card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC");
    }

    // Calculate and print capacity
    uint64_t card_size = ((uint64_t)card->csd.capacity) * card->csd.sector_size;
    ESP_LOGI(TAG, "Capacity: %llu MB", (unsigned long long)(card_size / (1024 * 1024)));
    ESP_LOGI(TAG, "Sector size: %d bytes", card->csd.sector_size);
}

/**
 * @brief Create and write a test file
 */
static esp_err_t sd_write_file(const char *path, const char *data)
{
    ESP_LOGI(TAG, "Writing file: %s", path);

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return ESP_FAIL;
    }

    fprintf(f, "%s", data);
    fclose(f);

    ESP_LOGI(TAG, "File written successfully (%d bytes)", (int)strlen(data));
    return ESP_OK;
}

/**
 * @brief Read a file and return contents
 */
static esp_err_t sd_read_file(const char *path, char *buf, size_t buf_size)
{
    ESP_LOGI(TAG, "Reading file: %s", path);

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
        return ESP_FAIL;
    }

    size_t bytes_read = fread(buf, 1, buf_size - 1, f);
    buf[bytes_read] = '\0';
    fclose(f);

    ESP_LOGI(TAG, "File read successfully (%d bytes)", (int)bytes_read);
    return ESP_OK;
}

/**
 * @brief Append data to a file
 */
static esp_err_t sd_append_file(const char *path, const char *data)
{
    ESP_LOGI(TAG, "Appending to file: %s", path);

    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", path);
        return ESP_FAIL;
    }

    fprintf(f, "%s", data);
    fclose(f);

    ESP_LOGI(TAG, "Data appended successfully");
    return ESP_OK;
}

/**
 * @brief Get file size using stat
 */
static long sd_get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/**
 * @brief List directory contents
 */
static esp_err_t sd_list_directory(const char *path)
{
    ESP_LOGI(TAG, "Listing directory: %s", path);

    DIR *dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return ESP_FAIL;
    }

    struct dirent *entry;
    int file_count = 0;

    printf("  %-30s %-10s %s\n", "Name", "Type", "Size");
    printf("  %-30s %-10s %s\n", "----", "----", "----");

    while ((entry = readdir(dir)) != NULL) {
        const char *type_str;
        switch (entry->d_type) {
            case DT_REG:
                type_str = "FILE";
                break;
            case DT_DIR:
                type_str = "DIR";
                break;
            default:
                type_str = "OTHER";
                break;
        }

        // Get file size
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        long size = sd_get_file_size(full_path);

        if (size >= 0) {
            printf("  %-30s %-10s %ld bytes\n", entry->d_name, type_str, size);
        } else {
            printf("  %-30s %-10s %s\n", entry->d_name, type_str, "-");
        }

        file_count++;
    }

    closedir(dir);
    ESP_LOGI(TAG, "Total entries: %d", file_count);
    return ESP_OK;
}

/**
 * @brief Unmount SD card
 */
static void sd_card_unmount(void)
{
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SD card unmounted successfully");
    } else {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
    }

    spi_bus_free(SPI_HOST_ID);
    ESP_LOGI(TAG, "SPI bus freed");
}

// ============================================================================
// Main Application
// ============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 SPI SD Card - FAT Filesystem");
    ESP_LOGI(TAG, "  Module 07 - SPI & Storage");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MOSI = GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  MISO = GPIO%d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  SCLK = GPIO%d", PIN_NUM_SCLK);
    ESP_LOGI(TAG, "  CS   = GPIO%d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  Mount Point: %s", MOUNT_POINT);

    // ====== Step 1: Mount SD Card ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 1: Mount SD Card ======");

    esp_err_t ret = sd_card_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed. Halting.");
        return;
    }

    // ====== Step 2: Print Card Info ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 2: Card Information ======");
    sd_card_print_info();

    // ====== Step 3: Write Test File ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 3: Write Test File ======");

    const char *test_data =
        "ESP32 SPI SD Card Test\n"
        "Module 07 - SPI & Storage\n"
        "Praktikum Sistem Embedded\n"
        "================================\n"
        "Line 1: Hello from ESP32!\n"
        "Line 2: SPI SD Card works!\n"
        "Line 3: FAT filesystem test\n"
        "Line 4: Data integrity check\n";

    ret = sd_write_file(TEST_FILENAME, test_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "File write failed");
        sd_card_unmount();
        return;
    }

    // Get file size
    long file_size = sd_get_file_size(TEST_FILENAME);
    ESP_LOGI(TAG, "File size after write: %ld bytes", file_size);

    // ====== Step 4: Append to File ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 4: Append Data ======");

    const char *append_data = "Line 5: Appended data!\n";
    ret = sd_append_file(TEST_FILENAME, append_data);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "File append failed (non-critical)");
    }

    file_size = sd_get_file_size(TEST_FILENAME);
    ESP_LOGI(TAG, "File size after append: %ld bytes", file_size);

    // ====== Step 5: Read File Back ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 5: Read File Back ======");

    char read_buf[1024] = {0};
    ret = sd_read_file(TEST_FILENAME, read_buf, sizeof(read_buf));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "File contents:");
        printf("--- BEGIN FILE ---\n%s--- END FILE ---\n", read_buf);

        // Verify content
        char expected[1024];
        snprintf(expected, sizeof(expected), "%s%s", test_data, append_data);

        if (strcmp(read_buf, expected) == 0) {
            ESP_LOGI(TAG, "File content verification: PASS");
        } else {
            ESP_LOGW(TAG, "File content verification: MISMATCH");
            ESP_LOGW(TAG, "Expected length: %d, Got: %d",
                     (int)strlen(expected), (int)strlen(read_buf));
        }
    }

    // ====== Step 6: List Directory ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 6: List Directory Contents ======");

    ret = sd_list_directory(TEST_DIR);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Directory listing failed");
    }

    // ====== Step 7: Create Additional File ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 7: Create Additional Files ======");

    const char *log_file = MOUNT_POINT "/log.csv";
    const char *csv_data = "timestamp,sensor,value\n"
                           "1000,temperature,25.5\n"
                           "2000,humidity,60.2\n"
                           "3000,temperature,26.1\n"
                           "4000,humidity,58.7\n";

    ret = sd_write_file(log_file, csv_data);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CSV log file created successfully");
    }

    // List directory again to show new file
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Updated directory listing:");
    sd_list_directory(TEST_DIR);

    // ====== Summary ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  SD CARD TEST COMPLETE");
    ESP_LOGI(TAG, "  Files created: test.txt, log.csv");
    ESP_LOGI(TAG, "  All operations successful");
    ESP_LOGI(TAG, "========================================");

    // ====== Step 8: Unmount ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 8: Unmount SD Card ======");
    sd_card_unmount();

    ESP_LOGI(TAG, "Program complete.");
}
