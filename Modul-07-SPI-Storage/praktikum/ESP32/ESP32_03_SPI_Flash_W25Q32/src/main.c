/**
 * @file main.c
 * @brief ESP32 SPI Flash W25Q32 Driver
 * @details Complete driver for W25Q32 SPI NOR flash memory using ESP-IDF
 *          SPI master. Implements read, write, erase operations with
 *          JEDEC ID verification.
 *
 * Program: ESP32_03_SPI_Flash_W25Q32
 * Module:  07 - SPI & Storage
 *
 * Hardware Connections:
 *   ESP32 GPIO23 (MOSI) --> W25Q32 DI   (pin 5)
 *   ESP32 GPIO19 (MISO) --> W25Q32 DO   (pin 2)
 *   ESP32 GPIO18 (SCLK) --> W25Q32 CLK  (pin 6)
 *   ESP32 GPIO5  (CS)   --> W25Q32 /CS  (pin 1)
 *   3.3V                --> W25Q32 VCC  (pin 8)
 *   3.3V                --> W25Q32 /WP  (pin 3)
 *   3.3V                --> W25Q32 /HOLD(pin 7)
 *   GND                 --> W25Q32 GND  (pin 4)
 *
 * Pin Mapping:
 *   MOSI = GPIO23
 *   MISO = GPIO19
 *   SCLK = GPIO18
 *   CS   = GPIO5
 *
 * Description:
 *   Implements a W25Q32 flash driver with functions for:
 *   - JEDEC ID read and verification
 *   - Status register read (WIP/WEL bits)
 *   - Write enable/disable
 *   - Sector erase (4KB)
 *   - Page program (up to 256 bytes)
 *   - Data read (arbitrary length)
 *   Demo sequence: Read ID → Erase → Write → Read back → Verify
 *
 * Framework: ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "SPI_FLASH";

static spi_device_handle_t spi_handle;

// ============================================================================
// SPI Initialization
// ============================================================================

/**
 * @brief Initialize SPI bus and add W25Q32 device
 */
static esp_err_t w25q_spi_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = W25Q_PAGE_SIZE + 4,  // Page + command/address
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,                      // SPI Mode 0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI bus initialized for W25Q32");
    return ESP_OK;
}

// ============================================================================
// W25Q32 Low-level Functions
// ============================================================================

/**
 * @brief Send a simple command (no data phase)
 */
static esp_err_t w25q_send_cmd(uint8_t cmd)
{
    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    return spi_device_transmit(spi_handle, &trans);
}

/**
 * @brief Read JEDEC ID (Manufacturer + Device ID)
 * @param id Output array of 3 bytes [manufacturer, mem_type, capacity]
 */
static esp_err_t w25q_read_jedec_id(uint8_t *id)
{
    uint8_t tx_buf[4] = {W25Q_CMD_READ_JEDEC_ID, 0, 0, 0};
    uint8_t rx_buf[4] = {0};

    spi_transaction_t trans = {
        .length = 32,               // 8 cmd + 24 data bits
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret == ESP_OK) {
        id[0] = rx_buf[1];          // Manufacturer ID
        id[1] = rx_buf[2];          // Memory Type
        id[2] = rx_buf[3];          // Capacity
    }
    return ret;
}

/**
 * @brief Read status register
 * @return Status register value, or -1 on error
 */
static int w25q_read_status(void)
{
    uint8_t tx_buf[2] = {W25Q_CMD_READ_STATUS, 0};
    uint8_t rx_buf[2] = {0};

    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        return -1;
    }
    return rx_buf[1];
}

/**
 * @brief Wait for Write In Progress (WIP) bit to clear
 */
static esp_err_t w25q_wait_busy(void)
{
    int64_t start = esp_timer_get_time();
    int64_t timeout_us = (int64_t)W25Q_TIMEOUT_MS * 1000;

    while (1) {
        int status = w25q_read_status();
        if (status < 0) {
            return ESP_FAIL;
        }

        if (!(status & W25Q_STATUS_WIP)) {
            return ESP_OK;
        }

        if ((esp_timer_get_time() - start) > timeout_us) {
            ESP_LOGE(TAG, "Timeout waiting for flash busy");
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief Send Write Enable (WREN) command
 */
static esp_err_t w25q_write_enable(void)
{
    esp_err_t ret = w25q_send_cmd(W25Q_CMD_WRITE_ENABLE);
    if (ret != ESP_OK) {
        return ret;
    }

    // Verify WEL bit is set
    int status = w25q_read_status();
    if (status < 0 || !(status & W25Q_STATUS_WEL)) {
        ESP_LOGE(TAG, "Write Enable failed, status=0x%02X", status);
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Erase a 4KB sector
 * @param addr Any address within the sector to erase
 */
static esp_err_t w25q_sector_erase(uint32_t addr)
{
    ESP_LOGI(TAG, "Erasing sector at address 0x%06lX...", (unsigned long)addr);

    esp_err_t ret = w25q_write_enable();
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t tx_buf[4] = {
        W25Q_CMD_SECTOR_ERASE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr),
    };

    spi_transaction_t trans = {
        .length = 32,
        .tx_buffer = tx_buf,
    };

    ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = w25q_wait_busy();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Sector erase complete");
    }
    return ret;
}

/**
 * @brief Program a page (up to 256 bytes)
 * @param addr Start address (must be page-aligned for full page writes)
 * @param data Data buffer to write
 * @param len Number of bytes to write (max 256)
 */
static esp_err_t w25q_page_program(uint32_t addr, const uint8_t *data, size_t len)
{
    if (len == 0 || len > W25Q_PAGE_SIZE) {
        ESP_LOGE(TAG, "Invalid page program length: %d", (int)len);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Programming %d bytes at address 0x%06lX...", (int)len, (unsigned long)addr);

    esp_err_t ret = w25q_write_enable();
    if (ret != ESP_OK) {
        return ret;
    }

    // Build command + address + data buffer
    uint8_t *tx_buf = heap_caps_malloc(4 + len, MALLOC_CAP_DMA);
    if (!tx_buf) {
        ESP_LOGE(TAG, "Failed to allocate TX buffer");
        return ESP_ERR_NO_MEM;
    }

    tx_buf[0] = W25Q_CMD_PAGE_PROGRAM;
    tx_buf[1] = (uint8_t)(addr >> 16);
    tx_buf[2] = (uint8_t)(addr >> 8);
    tx_buf[3] = (uint8_t)(addr);
    memcpy(&tx_buf[4], data, len);

    spi_transaction_t trans = {
        .length = (4 + len) * 8,
        .tx_buffer = tx_buf,
    };

    ret = spi_device_transmit(spi_handle, &trans);
    free(tx_buf);

    if (ret != ESP_OK) {
        return ret;
    }

    ret = w25q_wait_busy();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Page program complete");
    }
    return ret;
}

/**
 * @brief Read data from flash
 * @param addr Start address
 * @param buf Output buffer
 * @param len Number of bytes to read
 */
static esp_err_t w25q_read_data(uint32_t addr, uint8_t *buf, size_t len)
{
    ESP_LOGI(TAG, "Reading %d bytes from address 0x%06lX...", (int)len, (unsigned long)addr);

    // Allocate DMA-capable buffers
    uint8_t *tx_buf = heap_caps_calloc(1, 4 + len, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_calloc(1, 4 + len, MALLOC_CAP_DMA);
    if (!tx_buf || !rx_buf) {
        ESP_LOGE(TAG, "Failed to allocate buffers");
        free(tx_buf);
        free(rx_buf);
        return ESP_ERR_NO_MEM;
    }

    tx_buf[0] = W25Q_CMD_READ_DATA;
    tx_buf[1] = (uint8_t)(addr >> 16);
    tx_buf[2] = (uint8_t)(addr >> 8);
    tx_buf[3] = (uint8_t)(addr);

    spi_transaction_t trans = {
        .length = (4 + len) * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret == ESP_OK) {
        memcpy(buf, &rx_buf[4], len);
    }

    free(tx_buf);
    free(rx_buf);
    return ret;
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Print hex dump of a buffer
 */
static void print_hex_dump(const char *label, const uint8_t *buf, size_t len)
{
    printf("  %s (%d bytes):\n", label, (int)len);
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("    %04X: ", (unsigned int)i);
        }
        printf("%02X ", buf[i]);
        if ((i + 1) % 16 == 0 || i == len - 1) {
            // Print ASCII representation
            size_t line_start = (i / 16) * 16;
            size_t line_end = i + 1;
            // Pad if needed
            for (size_t j = line_end; j % 16 != 0 && j != line_start; j++) {
                printf("   ");
            }
            printf(" |");
            for (size_t j = line_start; j < line_end; j++) {
                char c = (buf[j] >= 0x20 && buf[j] <= 0x7E) ? (char)buf[j] : '.';
                printf("%c", c);
            }
            printf("|\n");
        }
    }
}

// ============================================================================
// Main Application
// ============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 SPI Flash W25Q32 Driver");
    ESP_LOGI(TAG, "  Module 07 - SPI & Storage");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MOSI = GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  MISO = GPIO%d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  SCLK = GPIO%d", PIN_NUM_SCLK);
    ESP_LOGI(TAG, "  CS   = GPIO%d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  SPI Clock: %d Hz", SPI_CLOCK_SPEED);

    // Initialize SPI
    esp_err_t ret = w25q_spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed. Halting.");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // ====== Step 1: Read JEDEC ID ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 1: Read JEDEC ID ======");

    uint8_t jedec_id[3] = {0};
    ret = w25q_read_jedec_id(jedec_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read JEDEC ID");
        return;
    }

    ESP_LOGI(TAG, "JEDEC ID: Manufacturer=0x%02X, MemType=0x%02X, Capacity=0x%02X",
             jedec_id[0], jedec_id[1], jedec_id[2]);

    bool id_valid = (jedec_id[0] == W25Q_MANUFACTURER_ID &&
                     jedec_id[1] == W25Q_DEVICE_ID_MSB &&
                     jedec_id[2] == W25Q_DEVICE_ID_LSB);

    if (id_valid) {
        ESP_LOGI(TAG, "JEDEC ID verification: PASS (W25Q32 detected)");
    } else {
        ESP_LOGW(TAG, "JEDEC ID verification: MISMATCH (expected EF,40,16)");
        ESP_LOGW(TAG, "Continuing anyway - chip may be compatible");
    }

    // ====== Step 2: Read Status Register ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 2: Read Status Register ======");

    int status = w25q_read_status();
    if (status >= 0) {
        ESP_LOGI(TAG, "Status Register: 0x%02X", status);
        ESP_LOGI(TAG, "  WIP (Write In Progress): %s", (status & W25Q_STATUS_WIP) ? "YES" : "NO");
        ESP_LOGI(TAG, "  WEL (Write Enable Latch): %s", (status & W25Q_STATUS_WEL) ? "YES" : "NO");
    } else {
        ESP_LOGE(TAG, "Failed to read status register");
    }

    // ====== Step 3: Erase Sector 0 ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 3: Erase Sector at 0x%06lX ======", (unsigned long)TEST_SECTOR_ADDR);

    ret = w25q_sector_erase(TEST_SECTOR_ADDR);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sector erase failed: %s", esp_err_to_name(ret));
        return;
    }

    // Verify erase (should read all 0xFF)
    uint8_t verify_buf[TEST_DATA_SIZE];
    ret = w25q_read_data(TEST_SECTOR_ADDR, verify_buf, TEST_DATA_SIZE);
    if (ret == ESP_OK) {
        bool erase_ok = true;
        for (int i = 0; i < TEST_DATA_SIZE; i++) {
            if (verify_buf[i] != 0xFF) {
                erase_ok = false;
                break;
            }
        }
        ESP_LOGI(TAG, "Erase verification: %s", erase_ok ? "PASS (all 0xFF)" : "FAIL");
        if (!erase_ok) {
            print_hex_dump("After erase", verify_buf, TEST_DATA_SIZE);
        }
    }

    // ====== Step 4: Write Test Data ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 4: Write Test Data ======");

    uint8_t write_data[TEST_DATA_SIZE];
    // Fill with test pattern: "Hello W25Q32 Flash!" + sequential bytes
    const char *test_msg = "Hello W25Q32 Flash!";
    size_t msg_len = strlen(test_msg);
    memcpy(write_data, test_msg, msg_len);
    for (size_t i = msg_len; i < TEST_DATA_SIZE; i++) {
        write_data[i] = (uint8_t)(i & 0xFF);
    }

    print_hex_dump("Write data", write_data, TEST_DATA_SIZE);

    ret = w25q_page_program(TEST_SECTOR_ADDR, write_data, TEST_DATA_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Page program failed: %s", esp_err_to_name(ret));
        return;
    }

    // ====== Step 5: Read Back and Verify ======
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "====== Step 5: Read Back and Verify ======");

    uint8_t read_data[TEST_DATA_SIZE] = {0};
    ret = w25q_read_data(TEST_SECTOR_ADDR, read_data, TEST_DATA_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(ret));
        return;
    }

    print_hex_dump("Read data", read_data, TEST_DATA_SIZE);

    // Compare
    int errors = 0;
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        if (write_data[i] != read_data[i]) {
            errors++;
            if (errors <= 5) {
                ESP_LOGW(TAG, "  Mismatch at byte %d: wrote=0x%02X, read=0x%02X",
                         i, write_data[i], read_data[i]);
            }
        }
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    if (errors == 0) {
        ESP_LOGI(TAG, "  DATA VERIFICATION: PASS");
        ESP_LOGI(TAG, "  All %d bytes match!", TEST_DATA_SIZE);
    } else {
        ESP_LOGE(TAG, "  DATA VERIFICATION: FAIL");
        ESP_LOGE(TAG, "  %d / %d bytes mismatched", errors, TEST_DATA_SIZE);
    }
    ESP_LOGI(TAG, "========================================");

    // Cleanup
    spi_bus_remove_device(spi_handle);
    spi_bus_free(SPI_HOST_ID);
    ESP_LOGI(TAG, "SPI bus freed. Test complete.");
}
