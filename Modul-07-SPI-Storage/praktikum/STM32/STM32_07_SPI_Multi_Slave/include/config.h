/**
 * ============================================================================
 * Program     : STM32_07_SPI_Multi_Slave
 * Description : Configuration for multiple SPI slaves on shared bus
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI1 Shared Bus Pin Configuration ==================== */
#define SPI1_SCK_PIN        GPIO_PIN_5      /* PA5 - SPI1 SCK  */
#define SPI1_SCK_PORT       GPIOA
#define SPI1_MOSI_PIN       GPIO_PIN_7      /* PA7 - SPI1 MOSI */
#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MISO_PIN       GPIO_PIN_6      /* PA6 - SPI1 MISO */
#define SPI1_MISO_PORT      GPIOA

/* ==================== Chip Select Pins ==================== */
#define CS1_PIN             GPIO_PIN_4      /* PA4 - Slave 1 CS */
#define CS1_PORT            GPIOA
#define CS2_PIN             GPIO_PIN_0      /* PB0 - Slave 2 CS */
#define CS2_PORT            GPIOB

/* ==================== Device Configurations ==================== */
/* Slave 1 configuration */
#define SLAVE1_NAME         "Slave-1 (PA4)"
#define SLAVE1_TX_SIZE      4               /* Bytes per transfer */
#define SLAVE1_CMD_READ     0xA1
#define SLAVE1_CMD_WRITE    0xA2
#define SLAVE1_CMD_STATUS   0xA3

/* Slave 2 configuration */
#define SLAVE2_NAME         "Slave-2 (PB0)"
#define SLAVE2_TX_SIZE      4               /* Bytes per transfer */
#define SLAVE2_CMD_READ     0xB1
#define SLAVE2_CMD_WRITE    0xB2
#define SLAVE2_CMD_STATUS   0xB3

/* ==================== SPI Configuration ==================== */
#define SPI_PRESCALER       SPI_BAUDRATEPRESCALER_16  /* 72MHz/16 = 4.5MHz */
#define SPI_TIMEOUT_MS      100

/* ==================== Timing / Statistics ==================== */
#define DEMO_INTERVAL_MS    1000            /* Demo cycle interval */
#define MAX_TRANSFER_LOG    100             /* Max transfers to log */

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE       115200
#define UART_TX_PIN         GPIO_PIN_9
#define UART_RX_PIN         GPIO_PIN_10
#define UART_PORT           GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN             GPIO_PIN_13
#define LED_PORT            GPIOC

#endif /* CONFIG_H */
