/**
 * ESP32_01_SPI_LCD_Basic
 * Basic LCD ST7735 display demonstration
 * 
 * Features:
 * - Initialize ST7735 LCD via SPI
 * - Display colored rectangles
 * - Display text messages
 * - Demonstrate basic graphics primitives
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "LCD_BASIC";

// ST7735 Commands
#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_COLMOD   0x3A
#define ST7735_MADCTL   0x36
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

typedef struct {
    spi_device_handle_t spi;
    int dc_pin;
    int rst_pin;
    int bl_pin;
} lcd_handle_t;

static lcd_handle_t lcd;

// SPI transaction helper
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user;
    gpio_set_level(lcd.dc_pin, dc);
}

// Send command to LCD
static void lcd_cmd(uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0;  // D/C = 0 for command
    ret = spi_device_polling_transmit(lcd.spi, &t);
    assert(ret == ESP_OK);
}

// Send data to LCD
static void lcd_data(uint8_t data) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    t.user = (void*)1;  // D/C = 1 for data
    ret = spi_device_polling_transmit(lcd.spi, &t);
    assert(ret == ESP_OK);
}

// Send data buffer to LCD
static void lcd_data_buf(const uint8_t *data, int len) {
    if (len == 0) return;
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    t.user = (void*)1;  // D/C = 1 for data
    ret = spi_device_polling_transmit(lcd.spi, &t);
    assert(ret == ESP_OK);
}

// Initialize LCD
static void lcd_init(void) {
    ESP_LOGI(TAG, "Initializing LCD...");
    
    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << lcd.dc_pin) | (1ULL << lcd.rst_pin) | (1ULL << lcd.bl_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Hardware reset
    gpio_set_level(lcd.rst_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(lcd.rst_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Software reset
    lcd_cmd(ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    // Sleep out
    lcd_cmd(ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Frame rate control
    lcd_cmd(ST7735_FRMCTR1);
    lcd_data(0x01); lcd_data(0x2C); lcd_data(0x2D);
    lcd_cmd(ST7735_FRMCTR2);
    lcd_data(0x01); lcd_data(0x2C); lcd_data(0x2D);
    lcd_cmd(ST7735_FRMCTR3);
    lcd_data(0x01); lcd_data(0x2C); lcd_data(0x2D);
    lcd_data(0x01); lcd_data(0x2C); lcd_data(0x2D);
    
    // Display inversion control
    lcd_cmd(ST7735_INVCTR);
    lcd_data(0x07);
    
    // Power control
    lcd_cmd(ST7735_PWCTR1);
    lcd_data(0xA2); lcd_data(0x02); lcd_data(0x84);
    lcd_cmd(ST7735_PWCTR2);
    lcd_data(0xC5);
    lcd_cmd(ST7735_PWCTR3);
    lcd_data(0x0A); lcd_data(0x00);
    lcd_cmd(ST7735_PWCTR4);
    lcd_data(0x8A); lcd_data(0x2A);
    lcd_cmd(ST7735_PWCTR5);
    lcd_data(0x8A); lcd_data(0xEE);
    
    // VCOM control
    lcd_cmd(ST7735_VMCTR1);
    lcd_data(0x0E);
    
    // Inversion off
    lcd_cmd(ST7735_INVOFF);
    
    // Memory access control (rotation)
    lcd_cmd(ST7735_MADCTL);
    lcd_data(0xC8);  // RGB, MX, MY
    
    // Color mode: 16-bit
    lcd_cmd(ST7735_COLMOD);
    lcd_data(0x05);
    
    // Gamma correction
    lcd_cmd(ST7735_GMCTRP1);
    lcd_data(0x02); lcd_data(0x1c); lcd_data(0x07); lcd_data(0x12);
    lcd_data(0x37); lcd_data(0x32); lcd_data(0x29); lcd_data(0x2d);
    lcd_data(0x29); lcd_data(0x25); lcd_data(0x2B); lcd_data(0x39);
    lcd_data(0x00); lcd_data(0x01); lcd_data(0x03); lcd_data(0x10);
    
    lcd_cmd(ST7735_GMCTRN1);
    lcd_data(0x03); lcd_data(0x1d); lcd_data(0x07); lcd_data(0x06);
    lcd_data(0x2E); lcd_data(0x2C); lcd_data(0x29); lcd_data(0x2D);
    lcd_data(0x2E); lcd_data(0x2E); lcd_data(0x37); lcd_data(0x3F);
    lcd_data(0x00); lcd_data(0x00); lcd_data(0x02); lcd_data(0x10);
    
    // Normal display on
    lcd_cmd(ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Display on
    lcd_cmd(ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Turn on backlight
    gpio_set_level(lcd.bl_pin, 1);
    
    ESP_LOGI(TAG, "LCD initialized successfully");
}

// Set address window
static void lcd_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_cmd(ST7735_CASET);
    lcd_data(0x00);
    lcd_data(x0 + LCD_OFFSET_X);
    lcd_data(0x00);
    lcd_data(x1 + LCD_OFFSET_X);
    
    lcd_cmd(ST7735_RASET);
    lcd_data(0x00);
    lcd_data(y0 + LCD_OFFSET_Y);
    lcd_data(0x00);
    lcd_data(y1 + LCD_OFFSET_Y);
    
    lcd_cmd(ST7735_RAMWR);
}

// Fill rectangle with color
static void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) return;
    if ((x + w - 1) >= LCD_WIDTH) w = LCD_WIDTH - x;
    if ((y + h - 1) >= LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    lcd_set_addr_window(x, y, x + w - 1, y + h - 1);
    
    uint8_t hi = color >> 8, lo = color & 0xFF;
    uint32_t num_pixels = w * h;
    
    // Send color data
    for (uint32_t i = 0; i < num_pixels; i++) {
        lcd_data(hi);
        lcd_data(lo);
    }
}

// Clear screen
static void lcd_clear(uint16_t color) {
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

// Draw pixel
static void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) return;
    lcd_set_addr_window(x, y, x, y);
    lcd_data(color >> 8);
    lcd_data(color & 0xFF);
}

// Draw line
static void lcd_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        lcd_draw_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SPI LCD Basic Demo");
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8
    };
    
    // Initialize SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = LCD_SPI_CLOCK,
        .mode = 0,
        .spics_io_num = LCD_CS_PIN,
        .queue_size = 7,
        .pre_cb = lcd_spi_pre_transfer_callback,
    };
    
    // Attach LCD to SPI bus
    lcd.dc_pin = LCD_DC_PIN;
    lcd.rst_pin = LCD_RST_PIN;
    lcd.bl_pin = LCD_BL_PIN;
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_SPI_HOST, &devcfg, &lcd.spi));
    
    // Initialize LCD
    lcd_init();
    
    // Demo: Display colored rectangles
    ESP_LOGI(TAG, "Displaying colored rectangles...");
    
    while (1) {
        // Clear screen with black
        lcd_clear(COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Red rectangle
        lcd_fill_rect(10, 10, 50, 50, COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Green rectangle
        lcd_fill_rect(70, 10, 50, 50, COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Blue rectangle
        lcd_fill_rect(10, 70, 50, 50, COLOR_BLUE);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Yellow rectangle
        lcd_fill_rect(70, 70, 50, 50, COLOR_YELLOW);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Draw lines
        lcd_clear(COLOR_BLACK);
        lcd_draw_line(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1, COLOR_WHITE);
        lcd_draw_line(0, LCD_HEIGHT-1, LCD_WIDTH-1, 0, COLOR_WHITE);
        lcd_draw_line(LCD_WIDTH/2, 0, LCD_WIDTH/2, LCD_HEIGHT-1, COLOR_CYAN);
        lcd_draw_line(0, LCD_HEIGHT/2, LCD_WIDTH-1, LCD_HEIGHT/2, COLOR_MAGENTA);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Color bars
        lcd_clear(COLOR_BLACK);
        int bar_height = LCD_HEIGHT / 6;
        lcd_fill_rect(0, 0, LCD_WIDTH, bar_height, COLOR_RED);
        lcd_fill_rect(0, bar_height, LCD_WIDTH, bar_height, COLOR_GREEN);
        lcd_fill_rect(0, bar_height*2, LCD_WIDTH, bar_height, COLOR_BLUE);
        lcd_fill_rect(0, bar_height*3, LCD_WIDTH, bar_height, COLOR_YELLOW);
        lcd_fill_rect(0, bar_height*4, LCD_WIDTH, bar_height, COLOR_CYAN);
        lcd_fill_rect(0, bar_height*5, LCD_WIDTH, bar_height, COLOR_MAGENTA);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        ESP_LOGI(TAG, "Display cycle completed");
    }
}
