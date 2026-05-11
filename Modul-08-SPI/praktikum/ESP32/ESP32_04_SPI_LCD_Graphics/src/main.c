#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "LCD_Graphics";

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
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

spi_device_handle_t spi;

// Bitmap: 16x16 heart icon
static const uint16_t heart_bitmap[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0},
    {0,0,1,1,1,1,1,0,1,1,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0},
    {0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0},
    {0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0},
    {0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

void lcd_cmd(uint8_t cmd) {
    gpio_set_level(PIN_NUM_LCD_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_data(uint8_t data) {
    gpio_set_level(PIN_NUM_LCD_DC, 1);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_data_buf(uint8_t *data, int len) {
    gpio_set_level(PIN_NUM_LCD_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_init(void) {
    gpio_set_level(PIN_NUM_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_cmd(ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    lcd_cmd(ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    lcd_cmd(ST7735_FRMCTR1);
    lcd_data(0x01); lcd_data(0x2C); lcd_data(0x2D);
    
    lcd_cmd(ST7735_INVCTR);
    lcd_data(0x07);
    
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
    
    lcd_cmd(ST7735_VMCTR1);
    lcd_data(0x0E);
    
    lcd_cmd(ST7735_INVOFF);
    
    lcd_cmd(ST7735_MADCTL);
    lcd_data(0xC8);
    
    lcd_cmd(ST7735_COLMOD);
    lcd_data(0x05);
    
    lcd_cmd(ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    lcd_cmd(ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "LCD initialized");
}

void lcd_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_cmd(ST7735_CASET);
    lcd_data(x0 >> 8);
    lcd_data(x0 & 0xFF);
    lcd_data(x1 >> 8);
    lcd_data(x1 & 0xFF);
    
    lcd_cmd(ST7735_RASET);
    lcd_data(y0 >> 8);
    lcd_data(y0 & 0xFF);
    lcd_data(y1 >> 8);
    lcd_data(y1 & 0xFF);
    
    lcd_cmd(ST7735_RAMWR);
}

void lcd_fill_screen(uint16_t color) {
    lcd_set_addr_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    uint8_t data[2] = {color >> 8, color & 0xFF};
    gpio_set_level(PIN_NUM_LCD_DC, 1);
    
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        lcd_data_buf(data, 2);
    }
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    lcd_set_addr_window(x, y, x, y);
    uint8_t data[2] = {color >> 8, color & 0xFF};
    lcd_data_buf(data, 2);
}

void lcd_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1) {
        lcd_draw_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
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

void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    lcd_draw_line(x, y, x + w - 1, y, color);
    lcd_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    lcd_draw_line(x + w - 1, y + h - 1, x, y + h - 1, color);
    lcd_draw_line(x, y + h - 1, x, y, color);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    
    lcd_set_addr_window(x, y, x + w - 1, y + h - 1);
    
    uint8_t data[2] = {color >> 8, color & 0xFF};
    gpio_set_level(PIN_NUM_LCD_DC, 1);
    
    for (int i = 0; i < w * h; i++) {
        lcd_data_buf(data, 2);
    }
}

void lcd_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    lcd_draw_pixel(x0, y0 + r, color);
    lcd_draw_pixel(x0, y0 - r, color);
    lcd_draw_pixel(x0 + r, y0, color);
    lcd_draw_pixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        lcd_draw_pixel(x0 + x, y0 + y, color);
        lcd_draw_pixel(x0 - x, y0 + y, color);
        lcd_draw_pixel(x0 + x, y0 - y, color);
        lcd_draw_pixel(x0 - x, y0 - y, color);
        lcd_draw_pixel(x0 + y, y0 + x, color);
        lcd_draw_pixel(x0 - y, y0 + x, color);
        lcd_draw_pixel(x0 + y, y0 - x, color);
        lcd_draw_pixel(x0 - y, y0 - x, color);
    }
}

void lcd_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    lcd_draw_line(x0, y0 - r, x0, y0 + r, color);
    
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        lcd_draw_line(x0 + x, y0 - y, x0 + x, y0 + y, color);
        lcd_draw_line(x0 - x, y0 - y, x0 - x, y0 + y, color);
        lcd_draw_line(x0 + y, y0 - x, x0 + y, y0 + x, color);
        lcd_draw_line(x0 - y, y0 - x, x0 - y, y0 + x, color);
    }
}

void lcd_draw_bitmap(uint16_t x, uint16_t y, const uint16_t bitmap[][16], uint16_t w, uint16_t h, uint16_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (bitmap[j][i]) {
                lcd_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

void demo_shapes(void) {
    ESP_LOGI(TAG, "Drawing shapes...");
    lcd_fill_screen(COLOR_BLACK);
    
    // Draw rectangles
    lcd_draw_rect(10, 10, 40, 30, COLOR_RED);
    lcd_fill_rect(60, 10, 40, 30, COLOR_GREEN);
    
    // Draw circles
    lcd_draw_circle(30, 70, 15, COLOR_BLUE);
    lcd_fill_circle(80, 70, 15, COLOR_YELLOW);
    
    // Draw lines
    lcd_draw_line(10, 110, 110, 110, COLOR_CYAN);
    lcd_draw_line(10, 120, 110, 140, COLOR_MAGENTA);
    
    vTaskDelay(pdMS_TO_TICKS(3000));
}

void demo_bitmap(void) {
    ESP_LOGI(TAG, "Drawing bitmap...");
    lcd_fill_screen(COLOR_BLACK);
    
    // Draw multiple hearts
    lcd_draw_bitmap(20, 20, heart_bitmap, 16, 16, COLOR_RED);
    lcd_draw_bitmap(50, 20, heart_bitmap, 16, 16, COLOR_MAGENTA);
    lcd_draw_bitmap(80, 20, heart_bitmap, 16, 16, COLOR_ORANGE);
    
    lcd_draw_bitmap(35, 50, heart_bitmap, 16, 16, COLOR_YELLOW);
    lcd_draw_bitmap(65, 50, heart_bitmap, 16, 16, COLOR_CYAN);
    
    vTaskDelay(pdMS_TO_TICKS(3000));
}

void demo_animation(void) {
    ESP_LOGI(TAG, "Running animation...");
    
    // Bouncing ball animation
    int16_t x = 64, y = 80;
    int16_t dx = 2, dy = 2;
    int16_t radius = 8;
    
    for (int i = 0; i < 200; i++) {
        lcd_fill_screen(COLOR_BLACK);
        
        // Draw boundaries
        lcd_draw_rect(5, 5, LCD_WIDTH - 10, LCD_HEIGHT - 10, COLOR_WHITE);
        
        // Draw ball
        lcd_fill_circle(x, y, radius, COLOR_RED);
        
        // Update position
        x += dx;
        y += dy;
        
        // Bounce off walls
        if (x - radius <= 5 || x + radius >= LCD_WIDTH - 5) {
            dx = -dx;
        }
        if (y - radius <= 5 || y + radius >= LCD_HEIGHT - 5) {
            dy = -dy;
        }
        
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void demo_gradient(void) {
    ESP_LOGI(TAG, "Drawing gradient...");
    
    for (int y = 0; y < LCD_HEIGHT; y++) {
        uint8_t r = (y * 31) / LCD_HEIGHT;
        uint8_t g = ((LCD_HEIGHT - y) * 63) / LCD_HEIGHT;
        uint8_t b = 15;
        uint16_t color = (r << 11) | (g << 5) | b;
        
        lcd_draw_line(0, y, LCD_WIDTH - 1, y, color);
    }
    
    vTaskDelay(pdMS_TO_TICKS(3000));
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SPI LCD Graphics Demo");
    
    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_LCD_DC) | (1ULL << PIN_NUM_LCD_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = LCD_SPI_CLOCK,
        .mode = 0,
        .spics_io_num = PIN_NUM_LCD_CS,
        .queue_size = 7,
        .pre_cb = NULL,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
    
    lcd_init();
    
    while (1) {
        demo_shapes();
        demo_bitmap();
        demo_animation();
        demo_gradient();
    }
}
