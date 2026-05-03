/**
 * ============================================================================
 * Program     : STM32_05_SPI_MCP3208_ADC
 * Description : Configuration for MCP3208 8-channel 12-bit ADC via SPI
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI1 Pin Configuration ==================== */
#define SPI1_SCK_PIN        GPIO_PIN_5      /* PA5 - SPI1 SCK  */
#define SPI1_SCK_PORT       GPIOA
#define SPI1_MOSI_PIN       GPIO_PIN_7      /* PA7 - SPI1 MOSI */
#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MISO_PIN       GPIO_PIN_6      /* PA6 - SPI1 MISO */
#define SPI1_MISO_PORT      GPIOA

/* ==================== Chip Select Pin ==================== */
#define MCP3208_CS_PIN      GPIO_PIN_4      /* PA4 - CS (active low) */
#define MCP3208_CS_PORT     GPIOA

/* ==================== MCP3208 Configuration ==================== */
#define MCP3208_VREF        3.3f            /* Reference voltage (V) */
#define MCP3208_CHANNELS    8               /* Number of ADC channels (0-7) */
#define MCP3208_RESOLUTION  4095            /* 12-bit max value (2^12 - 1) */

/* ==================== SPI Configuration ==================== */
#define SPI_PRESCALER       SPI_BAUDRATEPRESCALER_16  /* 72MHz/16 = 4.5MHz */

/* ==================== Sampling Configuration ==================== */
#define STATS_SAMPLES       10              /* Samples per channel for statistics */
#define READ_INTERVAL_MS    2000            /* Reading interval in ms */

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE       115200
#define UART_TX_PIN         GPIO_PIN_9      /* PA9 - USART1 TX */
#define UART_RX_PIN         GPIO_PIN_10     /* PA10 - USART1 RX */
#define UART_PORT           GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN             GPIO_PIN_13     /* PC13 - Onboard LED */
#define LED_PORT            GPIOC

/* ==================== Bar Graph Configuration ==================== */
#define BAR_MAX_WIDTH       30              /* Max bar graph characters */

#endif /* CONFIG_H */
