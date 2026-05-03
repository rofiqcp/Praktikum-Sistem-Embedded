/**
 * @file config.h
 * @brief Configuration for SPI OLED SSD1306 Display
 *
 * Hardware: ESP32 DevKit V1 + SSD1306 128x64 OLED (SPI interface)
 *
 * Pin Mapping:
 *   MOSI (DIN/SDA) -> GPIO23
 *   SCLK (CLK)     -> GPIO18
 *   CS             -> GPIO5
 *   DC             -> GPIO21
 *   RST (RESET)    -> GPIO22
 */

#ifndef CONFIG_H
#define CONFIG_H

// SPI Host selection
#define SPI_HOST_ID         SPI2_HOST       // HSPI

// SPI Pin definitions for OLED
#define PIN_NUM_MOSI        23              // DIN / SDA
#define PIN_NUM_SCLK        18              // CLK
#define PIN_NUM_CS          5               // Chip Select
#define PIN_OLED_DC         21              // Data/Command select
#define PIN_OLED_RST        22              // Reset

// SPI Configuration
#define SPI_CLOCK_SPEED     (10 * 1000 * 1000)  // 10 MHz

// OLED Display Configuration
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)   // 8 pages for 64px height

// Font Configuration
#define FONT_WIDTH          5
#define FONT_HEIGHT         7
#define FONT_CHAR_SPACING   1               // 1 pixel between characters

// Update interval
#define COUNTER_DELAY_MS    1000

#endif // CONFIG_H
