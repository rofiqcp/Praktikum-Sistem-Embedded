#ifndef CONFIG_H
#define CONFIG_H

// SPI Pin Definitions
#define PIN_NUM_MOSI    13
#define PIN_NUM_MISO    12
#define PIN_NUM_CLK     14

// ST7735 LCD Pin Definitions
#define PIN_NUM_LCD_CS  15
#define PIN_NUM_LCD_DC  2
#define PIN_NUM_LCD_RST 4

// LCD Configuration
#define LCD_WIDTH       128
#define LCD_HEIGHT      160
#define LCD_SPI_CLOCK   40000000  // 40 MHz

// Color Definitions (RGB565)
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_ORANGE    0xFD20
#define COLOR_PURPLE    0x8010

#endif // CONFIG_H
