#ifndef CONFIG_H
#define CONFIG_H

// ST7735 LCD Pin Definitions
#define LCD_MOSI_PIN    13
#define LCD_SCLK_PIN    14
#define LCD_CS_PIN      15
#define LCD_DC_PIN      2
#define LCD_RST_PIN     4
#define LCD_BL_PIN      5  // Backlight

// LCD Specifications
#define LCD_WIDTH       128
#define LCD_HEIGHT      160
#define LCD_OFFSET_X    0
#define LCD_OFFSET_Y    0

// SPI Configuration
#define LCD_SPI_HOST    SPI2_HOST
#define LCD_SPI_CLOCK   40000000  // 40 MHz

// Colors (RGB565 format)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F

#endif // CONFIG_H
