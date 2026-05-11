#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

// SPI Master Configuration
#define SPI_MASTER                  SPI1
#define SPI_MASTER_CLK_ENABLE()     __HAL_RCC_SPI1_CLK_ENABLE()
#define SPI_MASTER_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

// SPI Pins (SPI1)
#define SPI_SCK_PIN                 GPIO_PIN_5
#define SPI_MISO_PIN                GPIO_PIN_6
#define SPI_MOSI_PIN                GPIO_PIN_7
#define SPI_CS_PIN                  GPIO_PIN_4
#define SPI_GPIO_PORT               GPIOA

// Handshake pin (ESP32 ready signal)
#define HANDSHAKE_PIN               GPIO_PIN_3
#define HANDSHAKE_GPIO_PORT         GPIOA

// LED Pin
#define LED_PIN                     GPIO_PIN_13
#define LED_GPIO_PORT               GPIOC
#define LED_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOC_CLK_ENABLE()

// Buffer size
#define BUFFER_SIZE                 128

// UART for debugging
#define UART_INSTANCE               USART1
#define UART_BAUDRATE               115200

// Data exchange protocol
#define CMD_READ_SENSOR             0x01
#define CMD_WRITE_LED               0x02
#define CMD_GET_STATUS              0x03
#define CMD_ECHO                    0x04

#endif // CONFIG_H
