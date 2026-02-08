/**
 * ==========================================================================
 * FILE        : main.c
 * PROJECT     : ESP32_06_SPI_DAC_MCP4921
 * MODUL       : 07 - SPI & Storage
 * BOARD       : ESP32 DevKit V1
 * FRAMEWORK   : ESP-IDF
 * 
 * DESCRIPTION : Generate waveforms (Sine, Sawtooth, Triangle, Square) using
 *               MCP4921 12-bit external DAC via SPI. The program cycles
 *               through each waveform type every 5 seconds and outputs
 *               the analog signal through VOUT.
 * 
 * HARDWARE CONNECTIONS:
 *   ESP32 GPIO23 (MOSI) --> MCP4921 Pin 4 (SDI)
 *   ESP32 GPIO18 (SCLK) --> MCP4921 Pin 3 (SCK)
 *   ESP32 GPIO5  (CS)   --> MCP4921 Pin 2 (CS)
 *   ESP32 GPIO21 (LDAC) --> MCP4921 Pin 5 (LDAC)
 *   MCP4921 VREF (Pin 6) --> 3.3V
 *   MCP4921 VDD  (Pin 1) --> 3.3V
 *   MCP4921 VSS  (Pin 7) --> GND
 *   MCP4921 VOUT (Pin 8) --> Oscilloscope / Load
 * 
 * MCP4921 16-BIT FRAME:
 *   Bit 15   : A/B   = 0 (DAC A)
 *   Bit 14   : BUF   = 0 (Unbuffered)
 *   Bit 13   : GA    = 1 (1x gain, VOUT = VREF * D/4096)
 *   Bit 12   : SHDN  = 1 (Active mode)
 *   Bits 11:0: D[11:0] = 12-bit data (0-4095)
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
#include "esp_timer.h"
#include "config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char *TAG = "MCP4921";

/* SPI device handle */
static spi_device_handle_t spi_handle;

/* Pre-calculated sine lookup table (256 points, 12-bit values) */
static uint16_t sine_lut[WAVEFORM_POINTS];

/* Waveform names for display */
static const char *waveform_names[] = {
    "SINE", "SAWTOOTH", "TRIANGLE", "SQUARE"
};

/**
 * @brief Pre-calculate sine wave lookup table
 * 
 * Generates 256 points of a sine wave scaled to 12-bit DAC range (0-4095).
 * Sine is shifted and scaled: value = (sin(x) + 1) / 2 * 4095
 */
static void generate_sine_lut(void)
{
    for (int i = 0; i < WAVEFORM_POINTS; i++) {
        double angle = (2.0 * M_PI * i) / WAVEFORM_POINTS;
        double sine_val = (sin(angle) + 1.0) / 2.0;   // Normalize to 0.0 - 1.0
        sine_lut[i] = (uint16_t)(sine_val * WAVEFORM_AMPLITUDE);
    }
    ESP_LOGI(TAG, "Sine LUT generated: %d points", WAVEFORM_POINTS);
}

/**
 * @brief Initialize SPI bus and add MCP4921 device
 */
static esp_err_t spi_init(void)
{
    esp_err_t ret;

    /* SPI bus configuration (no MISO needed for DAC) */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,          // MCP4921 has no data output
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    /* MCP4921 SPI device configuration */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,                      // SPI Mode 0 (CPOL=0, CPHA=0)
        .spics_io_num = PIN_CS,
        .queue_size = 1,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure LDAC pin as output, hold LOW for immediate update */
    gpio_config_t ldac_cfg = {
        .pin_bit_mask = (1ULL << PIN_LDAC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ldac_cfg);
    gpio_set_level(PIN_LDAC, 0);   // LDAC LOW = immediate latch on CS rising edge

    ESP_LOGI(TAG, "SPI initialized: MOSI=%d, SCLK=%d, CS=%d, LDAC=%d, Clock=%d Hz",
             PIN_MOSI, PIN_SCLK, PIN_CS, PIN_LDAC, SPI_CLOCK_SPEED);

    return ESP_OK;
}

/**
 * @brief Write a 12-bit value to MCP4921
 * 
 * Sends a 16-bit frame over SPI:
 *   Bit 15   : 0 (DAC A select)
 *   Bit 14   : 0 (Unbuffered VREF input)
 *   Bit 13   : 1 (1x output gain)
 *   Bit 12   : 1 (Output power-down control, active)
 *   Bits 11:0: 12-bit data value
 * 
 * @param value  12-bit DAC value (0-4095)
 * @return ESP_OK on success
 */
static esp_err_t mcp4921_write(uint16_t value)
{
    /* Clamp to 12-bit range */
    if (value > MCP4921_RESOLUTION) {
        value = MCP4921_RESOLUTION;
    }

    /* Build 16-bit command frame */
    uint16_t frame = MCP4921_CONFIG_BITS | (value & 0x0FFF);

    /* SPI sends MSB first: split into 2 bytes */
    uint8_t tx_data[2];
    tx_data[0] = (frame >> 8) & 0xFF;   // High byte
    tx_data[1] = frame & 0xFF;          // Low byte

    spi_transaction_t trans = {
        .length = 16,                   // 16 bits
        .tx_buffer = tx_data,
        .rx_buffer = NULL,
        .flags = 0,
    };

    return spi_device_transmit(spi_handle, &trans);
}

/**
 * @brief Get DAC value for current waveform at given index
 * 
 * @param type   Waveform type
 * @param index  Current point index (0 to WAVEFORM_POINTS-1)
 * @return 12-bit DAC value
 */
static uint16_t get_waveform_value(waveform_type_t type, int index)
{
    switch (type) {
        case WAVEFORM_SINE:
            return sine_lut[index];

        case WAVEFORM_SAWTOOTH:
            /* Linear ramp from 0 to 4095 */
            return (uint16_t)((uint32_t)index * WAVEFORM_AMPLITUDE / (WAVEFORM_POINTS - 1));

        case WAVEFORM_TRIANGLE:
            /* Ramp up first half, ramp down second half */
            if (index < WAVEFORM_POINTS / 2) {
                return (uint16_t)((uint32_t)index * 2 * WAVEFORM_AMPLITUDE / (WAVEFORM_POINTS - 1));
            } else {
                return (uint16_t)((uint32_t)(WAVEFORM_POINTS - 1 - index) * 2 * WAVEFORM_AMPLITUDE / (WAVEFORM_POINTS - 1));
            }

        case WAVEFORM_SQUARE:
            /* First half = full scale, second half = 0 */
            return (index < WAVEFORM_POINTS / 2) ? WAVEFORM_AMPLITUDE : 0;

        default:
            return 0;
    }
}

/**
 * @brief Convert DAC value to expected output voltage
 */
static float dac_to_voltage(uint16_t value)
{
    return ((float)value / 4096.0f) * MCP4921_VREF;
}

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 MCP4921 DAC Waveform Generator");
    ESP_LOGI(TAG, "========================================");

    /* Initialize SPI */
    esp_err_t ret = spi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed! Halting.");
        return;
    }

    /* Generate sine lookup table */
    generate_sine_lut();

    ESP_LOGI(TAG, "VREF = %.2f V, Resolution = 12-bit (%d levels)",
             MCP4921_VREF, MCP4921_RESOLUTION + 1);
    ESP_LOGI(TAG, "Waveform points: %d, Step interval: %d us",
             WAVEFORM_POINTS, WAVEFORM_STEP_US);
    ESP_LOGI(TAG, "Starting waveform generation...");

    waveform_type_t current_waveform = WAVEFORM_SINE;
    int64_t switch_time = esp_timer_get_time();
    int point_index = 0;
    int print_counter = 0;

    while (1) {
        /* Get current waveform value */
        uint16_t dac_value = get_waveform_value(current_waveform, point_index);

        /* Write to DAC */
        ret = mcp4921_write(dac_value);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "DAC write failed: %s", esp_err_to_name(ret));
        }

        /* Print status periodically (every 256 points = once per cycle) */
        if (print_counter % WAVEFORM_POINTS == 0) {
            float voltage = dac_to_voltage(dac_value);
            printf("WAVE:%s,DAC:%d,VOLT:%.4f\n",
                   waveform_names[current_waveform], dac_value, voltage);
            ESP_LOGI(TAG, "Waveform: %s | Point: %d/%d | DAC: %d | Voltage: %.4f V",
                     waveform_names[current_waveform], point_index,
                     WAVEFORM_POINTS, dac_value, voltage);
        }
        print_counter++;

        /* Advance to next point */
        point_index = (point_index + 1) % WAVEFORM_POINTS;

        /* Check if it's time to switch waveform */
        int64_t now = esp_timer_get_time();
        if ((now - switch_time) >= (WAVEFORM_SWITCH_SEC * 1000000LL)) {
            current_waveform = (waveform_type_t)((current_waveform + 1) % WAVEFORM_COUNT);
            switch_time = now;
            point_index = 0;
            ESP_LOGI(TAG, ">>> Switching to waveform: %s <<<",
                     waveform_names[current_waveform]);
        }

        /* Delay for waveform timing */
        esp_rom_delay_us(WAVEFORM_STEP_US);
    }
}
