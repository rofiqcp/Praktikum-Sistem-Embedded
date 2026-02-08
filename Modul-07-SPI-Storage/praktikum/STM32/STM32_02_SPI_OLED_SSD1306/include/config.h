/**
 * ============================================================
 *  KONFIGURASI - STM32 SPI OLED SSD1306
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Board : Blue Pill STM32F103C8T6
 *  OLED  : SSD1306 128x64, SPI Interface
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI1 Pin Configuration ==================== */
#define SPI1_SCK_PIN            GPIO_PIN_5
#define SPI1_SCK_PORT           GPIOA
#define SPI1_MOSI_PIN           GPIO_PIN_7
#define SPI1_MOSI_PORT          GPIOA
#define SPI1_MISO_PIN           GPIO_PIN_6
#define SPI1_MISO_PORT          GPIOA
#define SPI1_CS_PIN             GPIO_PIN_4
#define SPI1_CS_PORT            GPIOA

/* ==================== OLED Control Pins ==================== */
#define OLED_DC_PIN             GPIO_PIN_0
#define OLED_DC_PORT            GPIOB
#define OLED_RST_PIN            GPIO_PIN_1
#define OLED_RST_PORT           GPIOB

/* ==================== SPI1 Parameters ==================== */
#define SPI1_PRESCALER          SPI_BAUDRATEPRESCALER_4   /* 72MHz / 4 = 18MHz */
#define SPI1_CPOL               SPI_POLARITY_LOW
#define SPI1_CPHA               SPI_PHASE_1EDGE
#define SPI1_DATA_SIZE          SPI_DATASIZE_8BIT
#define SPI1_FIRST_BIT          SPI_FIRSTBIT_MSB

/* ==================== SSD1306 Display ==================== */
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_PAGES           (SSD1306_HEIGHT / 8)
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_PAGES)

/* ==================== SSD1306 Commands ==================== */
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CLOCK_DIV       0xD5
#define SSD1306_CMD_SET_MUX_RATIO       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_SET_MEMORY_MODE     0x20
#define SSD1306_CMD_SEG_REMAP           0xA1
#define SSD1306_CMD_COM_SCAN_DEC        0xC8
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOMH           0xDB
#define SSD1306_CMD_ENTIRE_DISPLAY_ON   0xA4
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_SET_COL_ADDR        0x21
#define SSD1306_CMD_SET_PAGE_ADDR       0x22

/* ==================== UART1 Configuration ==================== */
#define UART1_TX_PIN            GPIO_PIN_9
#define UART1_TX_PORT           GPIOA
#define UART1_RX_PIN            GPIO_PIN_10
#define UART1_RX_PORT           GPIOA
#define UART1_BAUDRATE          115200

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

/* ==================== Font Configuration ==================== */
#define FONT_WIDTH              5
#define FONT_HEIGHT             7
#define FONT_FIRST_CHAR         0x20  /* Space */
#define FONT_LAST_CHAR          0x5A  /* 'Z' */

#endif /* CONFIG_H */
