#ifndef CONFIG_H
#define CONFIG_H

// SPI1 Pins (Hardware SPI)
#define SPI_SCK_PIN   PA5
#define SPI_MISO_PIN  PA6
#define SPI_MOSI_PIN  PA7

// ST7735 LCD Pins
#define LCD_CS_PIN    PA4
#define LCD_DC_PIN    PA3
#define LCD_RST_PIN   PA2

// LCD Configuration
#define LCD_WIDTH     128
#define LCD_HEIGHT    160

// Animation settings
#define BALL_RADIUS   5
#define BALL_SPEED    2
#define NUM_STARS     30

#endif // CONFIG_H
