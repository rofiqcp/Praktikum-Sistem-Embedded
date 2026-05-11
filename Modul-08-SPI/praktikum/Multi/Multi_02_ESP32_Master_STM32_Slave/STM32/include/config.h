#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"

// SPI Slave Configuration
#define SPI_SLAVE                   SPI1
#define SPI_SLAVE_CLK_ENABLE()      __HAL_RCC_SPI1_CLK_ENABLE()
#define SPI_SLAVE_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

// SPI Pins (SPI1)
#define SPI_SCK_PIN                 GPIO_PIN_5
#define SPI_MISO_PIN                GPIO_PIN_6
#define SPI_MOSI_PIN                GPIO_PIN_7
#define SPI_CS_PIN                  GPIO_PIN_4
#define SPI_GPIO_PORT               GPIOA

// LED Pin
#define LED_PIN                     GPIO_PIN_13
#define LED_GPIO_PORT               GPIOC
#define LED_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOC_CLK_ENABLE()

// Buffer size
#define BUFFER_SIZE                 64

// UART for debugging
#define UART_INSTANCE               USART1
#define UART_BAUDRATE               115200

#endif // CONFIG_H
