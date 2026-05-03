/**
 * ==========================================================================
 * FILE        : main.c
 * PROJECT     : ESP32_07_SPI_Multi_Slave
 * MODUL       : 07 - SPI & Storage
 * BOARD       : ESP32 DevKit V1
 * FRAMEWORK   : ESP-IDF
 * 
 * DESCRIPTION : Demonstrate multi-slave SPI communication on a shared bus.
 *               Two SPI slave devices share MOSI, MISO, and SCLK lines,
 *               with individual CS pins. The program initializes the SPI
 *               bus once, adds two devices, and alternately communicates
 *               with each slave, demonstrating bus arbitration and
 *               back-to-back transfers.
 * 
 * HARDWARE CONNECTIONS:
 *   ESP32 GPIO23 (MOSI) --> Shared MOSI to both slaves
 *   ESP32 GPIO19 (MISO) <-- Shared MISO from both slaves
 *   ESP32 GPIO18 (SCLK) --> Shared SCLK to both slaves
 *   ESP32 GPIO5  (CS1)  --> Slave 1 CS (ADC/Sensor)
 *   ESP32 GPIO17 (CS2)  --> Slave 2 CS (Flash/Memory)
 * 
 * KEY CONCEPTS:
 *   - Single SPI bus shared by multiple slaves
 *   - spi_bus_initialize() called once
 *   - spi_bus_add_device() called per slave with unique CS
 *   - CS pin toggles managed by ESP-IDF SPI driver
 *   - Back-to-back transfers demonstrate bus sharing
 * ==========================================================================
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

static const char *TAG = "MULTI_SPI";

/* SPI device handles for each slave */
static spi_device_handle_t slave1_handle;
static spi_device_handle_t slave2_handle;

/* Communication statistics */
typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    int64_t  total_time_us;
    int64_t  min_time_us;
    int64_t  max_time_us;
} comm_stats_t;

static comm_stats_t stats_slave1 = {0, 0, 0, 0, INT64_MAX, 0};
static comm_stats_t stats_slave2 = {0, 0, 0, 0, INT64_MAX, 0};

/**
 * @brief Initialize shared SPI bus and add both slave devices
 */
static esp_err_t spi_multi_init(void)
{
    esp_err_t ret;

    /* Configure shared SPI bus */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };

    /* Initialize SPI bus ONCE for all slaves */
    ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI bus initialized: MOSI=%d, MISO=%d, SCLK=%d",
             PIN_MOSI, PIN_MISO, PIN_SCLK);

    /* Add Slave 1 (ADC/Sensor) with CS1 */
    spi_device_interface_config_t dev1_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,
        .spics_io_num = PIN_CS_SLAVE1,
        .queue_size = 4,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev1_cfg, &slave1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add Slave 1: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Slave 1 (%s) added: CS=%d, ID=0x%02X",
             DEVICE_NAME_SLAVE1, PIN_CS_SLAVE1, DEVICE_ID_SLAVE1);

    /* Add Slave 2 (Flash/Memory) with CS2 */
    spi_device_interface_config_t dev2_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,
        .spics_io_num = PIN_CS_SLAVE2,
        .queue_size = 4,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev2_cfg, &slave2_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add Slave 2: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Slave 2 (%s) added: CS=%d, ID=0x%02X",
             DEVICE_NAME_SLAVE2, PIN_CS_SLAVE2, DEVICE_ID_SLAVE2);

    return ESP_OK;
}

/**
 * @brief Perform SPI transaction and measure timing
 * 
 * @param handle   SPI device handle
 * @param tx_data  Data to transmit
 * @param rx_data  Buffer for received data
 * @param len      Length in bytes
 * @param elapsed  Output: elapsed time in microseconds
 * @return ESP_OK on success
 */
static esp_err_t spi_transfer_timed(spi_device_handle_t handle,
                                     const uint8_t *tx_data,
                                     uint8_t *rx_data,
                                     size_t len,
                                     int64_t *elapsed)
{
    spi_transaction_t trans = {
        .length = len * 8,          // Length in bits
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
        .flags = 0,
    };

    int64_t start = esp_timer_get_time();
    esp_err_t ret = spi_device_transmit(handle, &trans);
    int64_t end = esp_timer_get_time();

    if (elapsed) {
        *elapsed = end - start;
    }

    return ret;
}

/**
 * @brief Update communication statistics
 */
static void update_stats(comm_stats_t *stats, int64_t elapsed_us, bool success)
{
    if (success) {
        stats->tx_count++;
        stats->rx_count++;
        stats->total_time_us += elapsed_us;
        if (elapsed_us < stats->min_time_us) stats->min_time_us = elapsed_us;
        if (elapsed_us > stats->max_time_us) stats->max_time_us = elapsed_us;
    } else {
        stats->error_count++;
    }
}

/**
 * @brief Communicate with Slave 1 (Simulated ADC read)
 * 
 * Sends a read command and receives simulated ADC data.
 * In a real scenario, this would be an MCP3208 or similar.
 */
static esp_err_t communicate_slave1(int transfer_num)
{
    /* Simulated ADC read command: [CMD, CHANNEL, 0x00, 0x00] */
    uint8_t tx_data[TRANSFER_SIZE] = {0x06, (uint8_t)(transfer_num & 0x07), 0x00, 0x00};
    uint8_t rx_data[TRANSFER_SIZE] = {0};
    int64_t elapsed = 0;

    ESP_LOGI(TAG, "[SLAVE1] Sending ADC read CH%d: [0x%02X 0x%02X 0x%02X 0x%02X]",
             transfer_num % 8, tx_data[0], tx_data[1], tx_data[2], tx_data[3]);

    esp_err_t ret = spi_transfer_timed(slave1_handle, tx_data, rx_data,
                                        TRANSFER_SIZE, &elapsed);

    if (ret == ESP_OK) {
        printf("SLAVE:1,TX:%02X%02X%02X%02X,RX:%02X%02X%02X%02X,TIME:%lld\n",
               tx_data[0], tx_data[1], tx_data[2], tx_data[3],
               rx_data[0], rx_data[1], rx_data[2], rx_data[3],
               (long long)elapsed);
        ESP_LOGI(TAG, "[SLAVE1] Received: [0x%02X 0x%02X 0x%02X 0x%02X] in %lld us",
                 rx_data[0], rx_data[1], rx_data[2], rx_data[3], (long long)elapsed);
    } else {
        ESP_LOGE(TAG, "[SLAVE1] Transfer failed: %s", esp_err_to_name(ret));
    }

    update_stats(&stats_slave1, elapsed, ret == ESP_OK);
    return ret;
}

/**
 * @brief Communicate with Slave 2 (Simulated Flash read)
 * 
 * Sends a read command and receives simulated flash data.
 * In a real scenario, this would be a W25Q or similar flash chip.
 */
static esp_err_t communicate_slave2(int transfer_num)
{
    /* Simulated flash read command: [READ_CMD, ADDR_H, ADDR_L, 0x00] */
    uint8_t tx_data[TRANSFER_SIZE] = {0x03, 0x00, (uint8_t)(transfer_num & 0xFF), 0x00};
    uint8_t rx_data[TRANSFER_SIZE] = {0};
    int64_t elapsed = 0;

    ESP_LOGI(TAG, "[SLAVE2] Sending Flash read addr 0x%04X: [0x%02X 0x%02X 0x%02X 0x%02X]",
             transfer_num & 0xFF, tx_data[0], tx_data[1], tx_data[2], tx_data[3]);

    esp_err_t ret = spi_transfer_timed(slave2_handle, tx_data, rx_data,
                                        TRANSFER_SIZE, &elapsed);

    if (ret == ESP_OK) {
        printf("SLAVE:2,TX:%02X%02X%02X%02X,RX:%02X%02X%02X%02X,TIME:%lld\n",
               tx_data[0], tx_data[1], tx_data[2], tx_data[3],
               rx_data[0], rx_data[1], rx_data[2], rx_data[3],
               (long long)elapsed);
        ESP_LOGI(TAG, "[SLAVE2] Received: [0x%02X 0x%02X 0x%02X 0x%02X] in %lld us",
                 rx_data[0], rx_data[1], rx_data[2], rx_data[3], (long long)elapsed);
    } else {
        ESP_LOGE(TAG, "[SLAVE2] Transfer failed: %s", esp_err_to_name(ret));
    }

    update_stats(&stats_slave2, elapsed, ret == ESP_OK);
    return ret;
}

/**
 * @brief Print communication statistics for both slaves
 */
static void print_statistics(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║             SPI Multi-Slave Communication Stats          ║\n");
    printf("╠════════════════╦══════════╦══════════╦════════╦══════════╣\n");
    printf("║    Device      ║   TX     ║   RX     ║ Errors ║ Avg (us) ║\n");
    printf("╠════════════════╬══════════╬══════════╬════════╬══════════╣\n");

    if (stats_slave1.tx_count > 0) {
        int64_t avg1 = stats_slave1.total_time_us / stats_slave1.tx_count;
        printf("║ %-14s ║ %8u ║ %8u ║ %6u ║ %8lld ║\n",
               DEVICE_NAME_SLAVE1,
               (unsigned)stats_slave1.tx_count,
               (unsigned)stats_slave1.rx_count,
               (unsigned)stats_slave1.error_count,
               (long long)avg1);
    }
    if (stats_slave2.tx_count > 0) {
        int64_t avg2 = stats_slave2.total_time_us / stats_slave2.tx_count;
        printf("║ %-14s ║ %8u ║ %8u ║ %6u ║ %8lld ║\n",
               DEVICE_NAME_SLAVE2,
               (unsigned)stats_slave2.tx_count,
               (unsigned)stats_slave2.rx_count,
               (unsigned)stats_slave2.error_count,
               (long long)avg2);
    }

    printf("╠════════════════╬══════════╬══════════╬════════╬══════════╣\n");
    printf("║ Min/Max Timing ║ Slave 1: %lld/%lld us   ║ Slave 2: %lld/%lld us ║\n",
           (long long)stats_slave1.min_time_us,
           (long long)stats_slave1.max_time_us,
           (long long)stats_slave2.min_time_us,
           (long long)stats_slave2.max_time_us);
    printf("╚════════════════╩══════════╩══════════╩════════╩══════════╝\n");
}

/**
 * @brief Demonstrate back-to-back transfers between slaves
 * 
 * Shows rapid switching between CS lines to verify bus sharing
 * works correctly without interference.
 */
static void demo_back_to_back(void)
{
    ESP_LOGI(TAG, "=== Back-to-Back Transfer Demo ===");

    uint8_t tx1[TRANSFER_SIZE] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t tx2[TRANSFER_SIZE] = {0x11, 0x22, 0x33, 0x44};
    uint8_t rx1[TRANSFER_SIZE] = {0};
    uint8_t rx2[TRANSFER_SIZE] = {0};
    int64_t elapsed1 = 0, elapsed2 = 0;

    /* Rapid back-to-back: Slave1 then immediately Slave2 */
    int64_t total_start = esp_timer_get_time();

    esp_err_t ret1 = spi_transfer_timed(slave1_handle, tx1, rx1, TRANSFER_SIZE, &elapsed1);
    /* No delay between slaves — test bus arbitration */
    esp_err_t ret2 = spi_transfer_timed(slave2_handle, tx2, rx2, TRANSFER_SIZE, &elapsed2);

    int64_t total_elapsed = esp_timer_get_time() - total_start;

    ESP_LOGI(TAG, "Back-to-back result:");
    ESP_LOGI(TAG, "  Slave1: %s (%lld us)", ret1 == ESP_OK ? "OK" : "FAIL", (long long)elapsed1);
    ESP_LOGI(TAG, "  Slave2: %s (%lld us)", ret2 == ESP_OK ? "OK" : "FAIL", (long long)elapsed2);
    ESP_LOGI(TAG, "  Total:  %lld us (overhead: %lld us)",
             (long long)total_elapsed, (long long)(total_elapsed - elapsed1 - elapsed2));

    printf("B2B:S1=%lld,S2=%lld,TOTAL=%lld,OVERHEAD=%lld\n",
           (long long)elapsed1, (long long)elapsed2,
           (long long)total_elapsed,
           (long long)(total_elapsed - elapsed1 - elapsed2));
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 Multi-Slave SPI Communication");
    ESP_LOGI(TAG, "========================================");

    /* Initialize SPI bus and both slave devices */
    esp_err_t ret = spi_multi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI multi-slave initialization failed! Halting.");
        return;
    }

    ESP_LOGI(TAG, "Starting multi-slave communication demo...");
    int cycle = 0;

    while (1) {
        cycle++;
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "============ Cycle #%d ============", cycle);

        /* --- Phase 1: Alternate between slaves --- */
        ESP_LOGI(TAG, "--- Phase 1: Alternating Communication ---");
        for (int i = 0; i < NUM_TRANSFERS; i++) {
            ESP_LOGI(TAG, ">> Transfer %d/%d <<", i + 1, NUM_TRANSFERS);

            /* Communicate with Slave 1 */
            communicate_slave1(i);

            /* Brief delay between slave switches */
            vTaskDelay(pdMS_TO_TICKS(INTER_SLAVE_DELAY_MS));

            /* Communicate with Slave 2 */
            communicate_slave2(i);

            vTaskDelay(pdMS_TO_TICKS(INTER_SLAVE_DELAY_MS));
        }

        /* --- Phase 2: Back-to-back transfers --- */
        ESP_LOGI(TAG, "--- Phase 2: Back-to-Back Transfers ---");
        demo_back_to_back();

        /* --- Print statistics --- */
        print_statistics();

        /* Wait before next cycle */
        ESP_LOGI(TAG, "Waiting %d ms before next cycle...", CYCLE_DELAY_MS);
        vTaskDelay(pdMS_TO_TICKS(CYCLE_DELAY_MS));
    }
}
