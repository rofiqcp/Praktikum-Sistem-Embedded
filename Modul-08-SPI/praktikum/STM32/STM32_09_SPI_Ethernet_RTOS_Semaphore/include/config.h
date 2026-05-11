#ifndef CONFIG_H
#define CONFIG_H

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#elif defined(STM32F401xC) || defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

// W5500 on SPI1: SCK=PA5, MISO=PA6, MOSI=PA7
// CS=PB0, RST=PB1, INT=PB5
#define ETH_CS_PORT         GPIOB
#define ETH_CS_PIN          GPIO_PIN_0
#define ETH_RST_PORT        GPIOB
#define ETH_RST_PIN         GPIO_PIN_1
#define ETH_INT_PORT        GPIOB
#define ETH_INT_PIN         GPIO_PIN_5

// SPI1 pins (shared)
#define SPI1_SCK_PORT       GPIOA
#define SPI1_SCK_PIN        GPIO_PIN_5
#define SPI1_MISO_PORT      GPIOA
#define SPI1_MISO_PIN       GPIO_PIN_6
#define SPI1_MOSI_PORT      GPIOA
#define SPI1_MOSI_PIN       GPIO_PIN_7

// LED (active LOW on bluepill)
#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

// UART
#define UART_BAUDRATE       115200

// FreeRTOS task priorities
#define ETH_TX_TASK_PRIORITY    2
#define ETH_RX_TASK_PRIORITY    2
#define APP_TASK_PRIORITY       1

// FreeRTOS task stack sizes (words)
#define ETH_TX_TASK_STACK       512
#define ETH_RX_TASK_STACK       512
#define APP_TASK_STACK          256

// Network config (static)
#define ETH_MAC_0   0xDE
#define ETH_MAC_1   0xAD
#define ETH_MAC_2   0xBE
#define ETH_MAC_3   0xEF
#define ETH_MAC_4   0xFE
#define ETH_MAC_5   0xED

// W5500 register addresses (common block)
#define W5500_MR            0x0000  // Mode Register
#define W5500_GAR0          0x0001  // Gateway IP
#define W5500_SUBR0         0x0005  // Subnet Mask
#define W5500_SHAR0         0x0009  // MAC Address
#define W5500_SIPR0         0x000F  // Source IP

// Block select bytes
#define W5500_BSB_COMMON    0x00    // Common register block
#define W5500_RW_WRITE      0x04
#define W5500_RW_READ       0x00

// Timing (ms)
#define TX_INTERVAL_MS      1000
#define RX_CHECK_MS         100
#define APP_UPDATE_MS       2000

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* CONFIG_H */

