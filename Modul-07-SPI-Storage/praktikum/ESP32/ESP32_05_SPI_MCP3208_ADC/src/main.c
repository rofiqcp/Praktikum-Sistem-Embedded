/**
 * ==========================================================================
 * FILE        : main.c
 * PROJECT     : ESP32_05_SPI_MCP3208_ADC
 * MODUL       : 07 - SPI & Storage
 * BOARD       : ESP32 DevKit V1
 * FRAMEWORK   : ESP-IDF
 * 
 * DESCRIPTION : Read 8-channel 12-bit ADC values from MCP3208 via SPI.
 *               The MCP3208 is an 8-channel, 12-bit successive approximation
 *               ADC with SPI interface. This program reads all 8 channels
 *               continuously, converts raw values to voltage, and computes
 *               min/max/average statistics over multiple samples.
 * 
 * HARDWARE CONNECTIONS:
 *   ESP32 GPIO23 (MOSI) --> MCP3208 Pin 11 (DIN)
 *   ESP32 GPIO19 (MISO) <-- MCP3208 Pin 12 (DOUT)
 *   ESP32 GPIO18 (SCLK) --> MCP3208 Pin 13 (CLK)
 *   ESP32 GPIO5  (CS)   --> MCP3208 Pin 10 (CS/SHDN)
 *   MCP3208 VREF (Pin 15) --> 3.3V
 *   MCP3208 VDD  (Pin 16) --> 3.3V
 *   MCP3208 AGND (Pin 14) --> GND
 *   MCP3208 DGND (Pin 9)  --> GND
 *   CH0-CH7: Connect analog signals to pins 1-8
 * 
 * SPI PROTOCOL (MCP3208):
 *   TX Byte 0: 0000 0 1 S D2  (Start=1, S=Single/Diff, D2=channel bit2)
 *   TX Byte 1: D1 D0 x x x x x x  (D1,D0=channel bits 1,0)
 *   TX Byte 2: 0x00 (don't care, clock out data)
 *   RX: 3 bytes, result in bits [13:2] of combined 24-bit response
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "MCP3208";

/* SPI device handle */
static spi_device_handle_t spi_handle;

/* Statistics storage per channel */
typedef struct {
    uint16_t samples[SAMPLE_COUNT];
    uint16_t min_val;
    uint16_t max_val;
    float    average;
    int      sample_index;
} channel_stats_t;

static channel_stats_t ch_stats[MCP3208_NUM_CHANNELS];

/**
 * @brief Initialize SPI bus and add MCP3208 device
 */
static esp_err_t spi_init(void)
{
    esp_err_t ret;

    /* SPI bus configuration */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    /* Initialize SPI bus */
    ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    /* MCP3208 SPI device configuration */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,                      // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = PIN_CS,
        .queue_size = 1,
        .flags = 0,
    };

    /* Add MCP3208 to SPI bus */
    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI initialized: MOSI=%d, MISO=%d, SCLK=%d, CS=%d, Clock=%d Hz",
             PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS, SPI_CLOCK_SPEED);

    return ESP_OK;
}

/**
 * @brief Read a single channel from MCP3208
 * 
 * MCP3208 SPI communication protocol:
 * - Send 3 bytes: Start bit, Single/Diff mode, Channel select
 * - Receive 3 bytes: Contains 12-bit ADC result
 * 
 * TX Frame:
 *   Byte 0: 0000 0 1 1 D2    (0x06 | (channel >> 2))
 *            Start=1, Single-ended=1, D2=MSB of channel
 *   Byte 1: D1 D0 0 0 0 0 0 0  ((channel & 0x03) << 6)
 *   Byte 2: 0x00               (don't care)
 * 
 * RX Frame:
 *   24 bits total, ADC data in bits [13:2] (zero-indexed from LSB)
 * 
 * @param channel  Channel number (0-7)
 * @param raw_out  Pointer to store 12-bit ADC result
 * @return ESP_OK on success
 */
static esp_err_t mcp3208_read_channel(uint8_t channel, uint16_t *raw_out)
{
    if (channel >= MCP3208_NUM_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel: %d (must be 0-%d)", channel, MCP3208_NUM_CHANNELS - 1);
        return ESP_ERR_INVALID_ARG;
    }

    /* Build TX data for MCP3208 */
    uint8_t tx_data[3];
    uint8_t rx_data[3];

    // Byte 0: Start bit (1) + Single-ended (1) + D2 (channel bit 2)
    tx_data[0] = 0x06 | (channel >> 2);
    // Byte 1: D1, D0 (channel bits 1,0) shifted to MSB positions
    tx_data[1] = (channel & 0x03) << 6;
    // Byte 2: Don't care (just clocking out data)
    tx_data[2] = 0x00;

    /* Configure SPI transaction */
    spi_transaction_t trans = {
        .length = 24,               // 3 bytes = 24 bits
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
        .flags = 0,
    };

    /* Execute SPI transaction */
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Extract 12-bit ADC value from response
     * RX data format (24 bits):
     * Byte 0: x x x x x x x x  (don't care)
     * Byte 1: x x 0 B11 B10 B9 B8 B7
     * Byte 2: B6 B5 B4 B3 B2 B1 B0 x
     * 
     * Result = ((rx[1] & 0x1F) << 7) | (rx[2] >> 1)
     * Or equivalently: bits [13:2] of the 24-bit combined value
     */
    uint32_t combined = ((uint32_t)rx_data[0] << 16) | 
                        ((uint32_t)rx_data[1] << 8)  | 
                        (uint32_t)rx_data[2];
    *raw_out = (combined >> 2) & 0x0FFF;  // Extract bits [13:2], mask 12 bits

    return ESP_OK;
}

/**
 * @brief Convert raw ADC value to voltage
 */
static float raw_to_voltage(uint16_t raw)
{
    return ((float)raw / (float)MCP3208_RESOLUTION) * MCP3208_VREF;
}

/**
 * @brief Initialize statistics for all channels
 */
static void stats_init(void)
{
    for (int ch = 0; ch < MCP3208_NUM_CHANNELS; ch++) {
        ch_stats[ch].min_val = 0xFFFF;
        ch_stats[ch].max_val = 0;
        ch_stats[ch].average = 0.0f;
        ch_stats[ch].sample_index = 0;
        memset(ch_stats[ch].samples, 0, sizeof(ch_stats[ch].samples));
    }
}

/**
 * @brief Update statistics for a channel with a new sample
 */
static void stats_update(int ch, uint16_t raw)
{
    channel_stats_t *s = &ch_stats[ch];

    /* Store sample in circular buffer */
    s->samples[s->sample_index % SAMPLE_COUNT] = raw;
    s->sample_index++;

    /* Calculate min, max, average over stored samples */
    int count = (s->sample_index < SAMPLE_COUNT) ? s->sample_index : SAMPLE_COUNT;
    s->min_val = 0xFFFF;
    s->max_val = 0;
    uint32_t sum = 0;

    for (int i = 0; i < count; i++) {
        uint16_t val = s->samples[i];
        if (val < s->min_val) s->min_val = val;
        if (val > s->max_val) s->max_val = val;
        sum += val;
    }
    s->average = (float)sum / (float)count;
}

/**
 * @brief Print formatted ADC readings table
 */
static void print_adc_table(uint16_t *raw_values)
{
    printf("\n");
    printf("╔═══════════╦═══════════╦══════════════╗\n");
    printf("║  Channel  ║  Raw ADC  ║  Voltage (V) ║\n");
    printf("╠═══════════╬═══════════╬══════════════╣\n");

    for (int ch = 0; ch < MCP3208_NUM_CHANNELS; ch++) {
        float voltage = raw_to_voltage(raw_values[ch]);
        printf("║   CH%d     ║   %4d    ║    %6.4f    ║\n",
               ch, raw_values[ch], voltage);
    }

    printf("╚═══════════╩═══════════╩══════════════╝\n");
}

/**
 * @brief Print statistics table for all channels
 */
static void print_stats_table(void)
{
    printf("\n--- Statistics (last %d samples) ---\n", SAMPLE_COUNT);
    printf("╔═══════════╦══════════╦══════════╦══════════════╗\n");
    printf("║  Channel  ║  Min (V) ║  Max (V) ║  Avg (V)     ║\n");
    printf("╠═══════════╬══════════╬══════════╬══════════════╣\n");

    for (int ch = 0; ch < MCP3208_NUM_CHANNELS; ch++) {
        channel_stats_t *s = &ch_stats[ch];
        if (s->sample_index > 0) {
            printf("║   CH%d     ║ %6.4f   ║ %6.4f   ║ %6.4f       ║\n",
                   ch,
                   raw_to_voltage(s->min_val),
                   raw_to_voltage(s->max_val),
                   raw_to_voltage((uint16_t)s->average));
        }
    }

    printf("╚═══════════╩══════════╩══════════╩══════════════╝\n");
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 MCP3208 8-Channel ADC via SPI");
    ESP_LOGI(TAG, "========================================");

    /* Initialize SPI */
    esp_err_t ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed! Halting.");
        return;
    }

    /* Initialize statistics */
    stats_init();

    ESP_LOGI(TAG, "Starting continuous ADC reading...");
    ESP_LOGI(TAG, "VREF = %.2f V, Resolution = %d (12-bit)", MCP3208_VREF, MCP3208_RESOLUTION);

    uint16_t raw_values[MCP3208_NUM_CHANNELS];
    int cycle = 0;

    while (1) {
        cycle++;
        ESP_LOGI(TAG, "--- Read Cycle #%d ---", cycle);

        /* Read all 8 channels */
        bool read_ok = true;
        for (int ch = 0; ch < MCP3208_NUM_CHANNELS; ch++) {
            ret = mcp3208_read_channel(ch, &raw_values[ch]);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to read CH%d", ch);
                raw_values[ch] = 0;
                read_ok = false;
            }
            /* Update statistics */
            stats_update(ch, raw_values[ch]);

            vTaskDelay(pdMS_TO_TICKS(CHANNEL_DELAY_MS));
        }

        if (read_ok) {
            /* Print current readings */
            print_adc_table(raw_values);

            /* Print statistics every SAMPLE_COUNT cycles */
            if (cycle % SAMPLE_COUNT == 0) {
                print_stats_table();
            }

            /* Print machine-readable output for debug script */
            printf("DATA:");
            for (int ch = 0; ch < MCP3208_NUM_CHANNELS; ch++) {
                printf("%d", raw_values[ch]);
                if (ch < MCP3208_NUM_CHANNELS - 1) printf(",");
            }
            printf("\n");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
