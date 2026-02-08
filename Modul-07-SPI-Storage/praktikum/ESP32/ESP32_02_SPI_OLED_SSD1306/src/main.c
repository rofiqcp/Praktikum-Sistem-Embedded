/**
 * @file main.c
 * @brief ESP32 SPI OLED SSD1306 Display Driver
 * @details Drives a 128x64 SSD1306 OLED display via SPI interface using
 *          ESP-IDF SPI master driver. Implements basic graphics primitives
 *          and a 5x7 font for text rendering.
 *
 * Program: ESP32_02_SPI_OLED_SSD1306
 * Module:  07 - SPI & Storage
 *
 * Hardware Connections:
 *   ESP32 GPIO23 (MOSI) --> OLED DIN/SDA
 *   ESP32 GPIO18 (SCLK) --> OLED CLK
 *   ESP32 GPIO5  (CS)   --> OLED CS
 *   ESP32 GPIO21 (DC)   --> OLED DC
 *   ESP32 GPIO22 (RST)  --> OLED RST
 *   ESP32 3.3V          --> OLED VCC
 *   ESP32 GND            --> OLED GND
 *
 * Pin Mapping:
 *   MOSI = GPIO23
 *   SCLK = GPIO18
 *   CS   = GPIO5
 *   DC   = GPIO21
 *   RST  = GPIO22
 *
 * Description:
 *   Initializes the SSD1306 OLED via SPI, provides drawing primitives
 *   (pixel, char, string), and displays "ESP32 SPI" on line 1,
 *   "Modul 07" on line 2, with a counter incrementing every second.
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
#include "config.h"

static const char *TAG = "SPI_OLED";

static spi_device_handle_t spi_handle;

// Frame buffer: 128 x 8 pages = 1024 bytes
static uint8_t frame_buffer[OLED_WIDTH * OLED_PAGES];

// ============================================================================
// 5x7 Font - Characters ' ' (0x20) through 'Z' (0x5A), plus lowercase mapped
// Each character is 5 bytes wide (columns), each byte represents 7-bit column
// ============================================================================
static const uint8_t font_5x7[][5] = {
    // ' ' (space)
    {0x00, 0x00, 0x00, 0x00, 0x00},
    // '!' 
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    // '"'
    {0x00, 0x07, 0x00, 0x07, 0x00},
    // '#'
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    // '$'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    // '%'
    {0x23, 0x13, 0x08, 0x64, 0x62},
    // '&'
    {0x36, 0x49, 0x55, 0x22, 0x50},
    // '''
    {0x00, 0x05, 0x03, 0x00, 0x00},
    // '('
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    // ')'
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    // '*'
    {0x14, 0x08, 0x3E, 0x08, 0x14},
    // '+'
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    // ','
    {0x00, 0x50, 0x30, 0x00, 0x00},
    // '-'
    {0x08, 0x08, 0x08, 0x08, 0x08},
    // '.'
    {0x00, 0x60, 0x60, 0x00, 0x00},
    // '/'
    {0x20, 0x10, 0x08, 0x04, 0x02},
    // '0'
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    // '1'
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    // '2'
    {0x42, 0x61, 0x51, 0x49, 0x46},
    // '3'
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    // '4'
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    // '5'
    {0x27, 0x45, 0x45, 0x45, 0x39},
    // '6'
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    // '7'
    {0x01, 0x71, 0x09, 0x05, 0x03},
    // '8'
    {0x36, 0x49, 0x49, 0x49, 0x36},
    // '9'
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    // ':'
    {0x00, 0x36, 0x36, 0x00, 0x00},
    // ';'
    {0x00, 0x56, 0x36, 0x00, 0x00},
    // '<'
    {0x08, 0x14, 0x22, 0x41, 0x00},
    // '='
    {0x14, 0x14, 0x14, 0x14, 0x14},
    // '>'
    {0x00, 0x41, 0x22, 0x14, 0x08},
    // '?'
    {0x02, 0x01, 0x51, 0x09, 0x06},
    // '@'
    {0x32, 0x49, 0x79, 0x41, 0x3E},
    // 'A'
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    // 'B'
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    // 'C'
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    // 'D'
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    // 'E'
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    // 'F'
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    // 'G'
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    // 'H'
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    // 'I'
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    // 'J'
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    // 'K'
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    // 'L'
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    // 'M'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    // 'N'
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    // 'O'
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    // 'P'
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    // 'Q'
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    // 'R'
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    // 'S'
    {0x46, 0x49, 0x49, 0x49, 0x31},
    // 'T'
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    // 'U'
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    // 'V'
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    // 'W'
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    // 'X'
    {0x63, 0x14, 0x08, 0x14, 0x63},
    // 'Y'
    {0x07, 0x08, 0x70, 0x08, 0x07},
    // 'Z'
    {0x61, 0x51, 0x49, 0x45, 0x43},
};

/**
 * @brief Get font data for a character
 * @return Pointer to 5-byte font data, or space for unsupported chars
 */
static const uint8_t *get_font_char(char c)
{
    // Map lowercase to uppercase
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }

    if (c >= ' ' && c <= 'Z') {
        return font_5x7[c - ' '];
    }
    // Return space for unsupported characters
    return font_5x7[0];
}

// ============================================================================
// SSD1306 Low-level SPI Functions
// ============================================================================

/**
 * @brief Send a command byte to SSD1306
 */
static void ssd1306_send_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_OLED_DC, 0);  // DC=0 for command

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send command 0x%02X: %s", cmd, esp_err_to_name(ret));
    }
}

/**
 * @brief Send data buffer to SSD1306
 */
static void ssd1306_send_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;

    gpio_set_level(PIN_OLED_DC, 1);  // DC=1 for data

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send data: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief Initialize SSD1306 OLED display
 */
static void ssd1306_init(void)
{
    ESP_LOGI(TAG, "Initializing SSD1306 OLED display...");

    // Hardware reset
    gpio_set_level(PIN_OLED_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_OLED_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Initialization sequence
    ssd1306_send_cmd(0xAE);  // Display OFF
    ESP_LOGI(TAG, "  CMD: 0xAE - Display OFF");

    ssd1306_send_cmd(0xD5);  // Set display clock divide ratio/oscillator frequency
    ssd1306_send_cmd(0x80);  // Default value
    ESP_LOGI(TAG, "  CMD: 0xD5,0x80 - Clock Divide");

    ssd1306_send_cmd(0xA8);  // Set multiplex ratio
    ssd1306_send_cmd(0x3F);  // 1/64 duty (64 lines)
    ESP_LOGI(TAG, "  CMD: 0xA8,0x3F - Mux Ratio 64");

    ssd1306_send_cmd(0xD3);  // Set display offset
    ssd1306_send_cmd(0x00);  // No offset
    ESP_LOGI(TAG, "  CMD: 0xD3,0x00 - Display Offset 0");

    ssd1306_send_cmd(0x40);  // Set display start line to 0
    ESP_LOGI(TAG, "  CMD: 0x40 - Start Line 0");

    ssd1306_send_cmd(0x8D);  // Charge pump setting
    ssd1306_send_cmd(0x14);  // Enable charge pump
    ESP_LOGI(TAG, "  CMD: 0x8D,0x14 - Charge Pump ON");

    ssd1306_send_cmd(0x20);  // Set memory addressing mode
    ssd1306_send_cmd(0x00);  // Horizontal addressing mode
    ESP_LOGI(TAG, "  CMD: 0x20,0x00 - Horizontal Addressing");

    ssd1306_send_cmd(0xA1);  // Set segment re-map (column 127 mapped to SEG0)
    ESP_LOGI(TAG, "  CMD: 0xA1 - Segment Remap");

    ssd1306_send_cmd(0xC8);  // Set COM output scan direction (remapped)
    ESP_LOGI(TAG, "  CMD: 0xC8 - COM Scan Remapped");

    ssd1306_send_cmd(0xDA);  // Set COM pins hardware configuration
    ssd1306_send_cmd(0x12);  // Alternative COM pin config
    ESP_LOGI(TAG, "  CMD: 0xDA,0x12 - COM Pins Config");

    ssd1306_send_cmd(0x81);  // Set contrast control
    ssd1306_send_cmd(0xCF);  // Contrast value
    ESP_LOGI(TAG, "  CMD: 0x81,0xCF - Contrast");

    ssd1306_send_cmd(0xD9);  // Set pre-charge period
    ssd1306_send_cmd(0xF1);
    ESP_LOGI(TAG, "  CMD: 0xD9,0xF1 - Pre-charge Period");

    ssd1306_send_cmd(0xDB);  // Set VCOMH deselect level
    ssd1306_send_cmd(0x40);
    ESP_LOGI(TAG, "  CMD: 0xDB,0x40 - VCOMH Level");

    ssd1306_send_cmd(0xA4);  // Entire display ON (resume to RAM content)
    ESP_LOGI(TAG, "  CMD: 0xA4 - Display from RAM");

    ssd1306_send_cmd(0xA6);  // Set normal display (not inverted)
    ESP_LOGI(TAG, "  CMD: 0xA6 - Normal Display");

    ssd1306_send_cmd(0xAF);  // Display ON
    ESP_LOGI(TAG, "  CMD: 0xAF - Display ON");

    ESP_LOGI(TAG, "SSD1306 initialization complete");
}

// ============================================================================
// Drawing Functions
// ============================================================================

/**
 * @brief Clear the frame buffer
 */
static void ssd1306_clear(void)
{
    memset(frame_buffer, 0x00, sizeof(frame_buffer));
}

/**
 * @brief Update the OLED display from frame buffer
 */
static void ssd1306_update(void)
{
    // Set column address range 0-127
    ssd1306_send_cmd(0x21);
    ssd1306_send_cmd(0x00);
    ssd1306_send_cmd(0x7F);

    // Set page address range 0-7
    ssd1306_send_cmd(0x22);
    ssd1306_send_cmd(0x00);
    ssd1306_send_cmd(0x07);

    // Send frame buffer data
    ssd1306_send_data(frame_buffer, sizeof(frame_buffer));
}

/**
 * @brief Set a pixel in the frame buffer
 */
static void ssd1306_set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }

    int page = y / 8;
    int bit = y % 8;
    int idx = page * OLED_WIDTH + x;

    if (on) {
        frame_buffer[idx] |= (1 << bit);
    } else {
        frame_buffer[idx] &= ~(1 << bit);
    }
}

/**
 * @brief Draw a single character at position (x, y)
 * @param x X coordinate (pixel)
 * @param y Y coordinate (pixel, top of character)
 * @param c Character to draw
 */
static void ssd1306_draw_char(int x, int y, char c)
{
    const uint8_t *font_data = get_font_char(c);

    for (int col = 0; col < FONT_WIDTH; col++) {
        uint8_t column_byte = font_data[col];
        for (int row = 0; row < FONT_HEIGHT; row++) {
            bool pixel_on = (column_byte >> row) & 0x01;
            ssd1306_set_pixel(x + col, y + row, pixel_on);
        }
    }
}

/**
 * @brief Draw a string at position (x, y)
 * @param x X coordinate (pixel)
 * @param y Y coordinate (pixel, top of text)
 * @param str Null-terminated string
 */
static void ssd1306_draw_string(int x, int y, const char *str)
{
    int cursor_x = x;
    while (*str) {
        if (cursor_x + FONT_WIDTH > OLED_WIDTH) {
            break;  // Stop if text exceeds display width
        }
        ssd1306_draw_char(cursor_x, y, *str);
        cursor_x += FONT_WIDTH + FONT_CHAR_SPACING;
        str++;
    }
}

// ============================================================================
// SPI Initialization
// ============================================================================

/**
 * @brief Initialize SPI bus and configure GPIO pins
 */
static esp_err_t spi_oled_init(void)
{
    // Configure DC and RST pins as GPIO output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_OLED_DC) | (1ULL << PIN_OLED_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_OLED_RST, 1);
    gpio_set_level(PIN_OLED_DC, 0);

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,              // OLED has no MISO
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = OLED_WIDTH * OLED_PAGES,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // SPI device configuration
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,                      // SPI Mode 0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI bus initialized for OLED");
    return ESP_OK;
}

// ============================================================================
// Main Application
// ============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 SPI OLED SSD1306 Display");
    ESP_LOGI(TAG, "  Module 07 - SPI & Storage");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MOSI = GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  SCLK = GPIO%d", PIN_NUM_SCLK);
    ESP_LOGI(TAG, "  CS   = GPIO%d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  DC   = GPIO%d", PIN_OLED_DC);
    ESP_LOGI(TAG, "  RST  = GPIO%d", PIN_OLED_RST);
    ESP_LOGI(TAG, "  Display: %dx%d", OLED_WIDTH, OLED_HEIGHT);
    ESP_LOGI(TAG, "  SPI Clock: %d Hz", SPI_CLOCK_SPEED);

    // Initialize SPI
    esp_err_t ret = spi_oled_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed. Halting.");
        return;
    }

    // Initialize OLED
    ssd1306_init();

    // Clear display
    ssd1306_clear();
    ssd1306_update();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Draw static text
    ESP_LOGI(TAG, "Drawing text on OLED...");

    uint32_t counter = 0;
    char counter_str[32];

    while (1) {
        ssd1306_clear();

        // Line 1: "ESP32 SPI" at page 0 (y=0)
        ssd1306_draw_string(10, 0, "ESP32 SPI");

        // Line 2: "Modul 07" at page 2 (y=16)
        ssd1306_draw_string(10, 16, "MODUL 07");

        // Line 3: Counter at page 4 (y=32)
        snprintf(counter_str, sizeof(counter_str), "COUNT: %lu", (unsigned long)counter);
        ssd1306_draw_string(10, 32, counter_str);

        // Line 4: Separator line at page 6 (y=48)
        ssd1306_draw_string(0, 48, "SPI OLED DEMO");

        // Update display
        ssd1306_update();

        ESP_LOGI(TAG, "Display updated - Counter: %lu", (unsigned long)counter);
        counter++;

        vTaskDelay(pdMS_TO_TICKS(COUNTER_DELAY_MS));
    }
}
