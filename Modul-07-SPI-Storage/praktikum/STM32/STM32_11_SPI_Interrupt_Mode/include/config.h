/**
 * ============================================================================
 * Program     : STM32_11_SPI_Interrupt_Mode
 * Description : Configuration for SPI interrupt (non-blocking) mode
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ==================== SPI1 Pin Configuration ==================== */
#define SPI1_SCK_PIN            GPIO_PIN_5      /* PA5 - SCK */
#define SPI1_MISO_PIN           GPIO_PIN_6      /* PA6 - MISO */
#define SPI1_MOSI_PIN           GPIO_PIN_7      /* PA7 - MOSI */
#define SPI1_CS_PIN             GPIO_PIN_4      /* PA4 - CS (software) */
#define SPI1_PORT               GPIOA

/* ==================== SPI Configuration ==================== */
#define SPI_PRESCALER           SPI_BAUDRATEPRESCALER_16    /* 72MHz/16 = 4.5MHz */
#define SPI_CLOCK_HZ            4500000U

/* ==================== Buffer Configuration ==================== */
#define SPI_BUFFER_SIZE         64              /* Transfer buffer size */
#define SPI_TEST_ITERATIONS     10              /* Repeat for averaging */

/* ==================== NVIC Priority ==================== */
#define SPI1_IRQ_PRIORITY       1               /* SPI1 interrupt priority */
#define SPI1_IRQ_SUB_PRIORITY   0               /* SPI1 interrupt sub-priority */

/* ==================== DWT for Timing ==================== */
#define SYSCLK_FREQ             72000000U       /* 72 MHz system clock */

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE           115200
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PIN             GPIO_PIN_10
#define UART_PORT               GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

#endif /* CONFIG_H */
