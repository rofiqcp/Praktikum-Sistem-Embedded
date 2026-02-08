/**
 * ============================================================================
 * Program     : STM32_10_SPI_Speed_Benchmark
 * Description : Configuration for SPI speed and throughput benchmarking
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

/* ==================== System Clock ==================== */
#define SYSCLK_FREQ             72000000U       /* 72 MHz */
#define APB2_FREQ               72000000U       /* APB2 = 72 MHz (SPI1 on APB2) */

/* ==================== SPI Prescaler Test Configuration ==================== */
/* SPI1 is on APB2 (72 MHz). SPI clock = APB2 / prescaler */
#define NUM_PRESCALERS          8

/* Prescaler values and resulting SPI clock frequencies */
static const struct {
    uint32_t hal_prescaler;     /* HAL prescaler constant */
    uint32_t divider;           /* Numeric divider */
    uint32_t spi_clock_hz;     /* Resulting SPI clock in Hz */
    const char *label;          /* Human-readable label */
} spi_prescalers[NUM_PRESCALERS] = {
    { SPI_BAUDRATEPRESCALER_2,   2,   36000000, "36.000 MHz" },
    { SPI_BAUDRATEPRESCALER_4,   4,   18000000, "18.000 MHz" },
    { SPI_BAUDRATEPRESCALER_8,   8,    9000000,  "9.000 MHz" },
    { SPI_BAUDRATEPRESCALER_16,  16,   4500000,  "4.500 MHz" },
    { SPI_BAUDRATEPRESCALER_32,  32,   2250000,  "2.250 MHz" },
    { SPI_BAUDRATEPRESCALER_64,  64,   1125000,  "1.125 MHz" },
    { SPI_BAUDRATEPRESCALER_128, 128,   562500, "562.5 kHz"  },
    { SPI_BAUDRATEPRESCALER_256, 256,   281250, "281.3 kHz"  },
};

/* ==================== Buffer Size Test Configuration ==================== */
#define NUM_BUFFER_SIZES        7
static const uint16_t buffer_sizes[NUM_BUFFER_SIZES] = {
    8, 16, 32, 64, 128, 256, 512
};

#define MAX_BUFFER_SIZE         512     /* Largest buffer for allocation */

/* ==================== Benchmark Parameters ==================== */
#define NUM_ITERATIONS          100     /* Iterations per test */
#define WARMUP_ITERATIONS       5       /* Warmup before measurement */

/* ==================== DWT (Data Watchpoint & Trace) ==================== */
/* Used for precise cycle counting */
#define DWT_CYCCNT_ADDR         ((volatile uint32_t *)0xE0001004)
#define DWT_CONTROL_ADDR        ((volatile uint32_t *)0xE0001000)
#define DEMCR_ADDR              ((volatile uint32_t *)0xE000EDFC)
#define DWT_LAR_ADDR            ((volatile uint32_t *)0xE0001FB0)

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE           115200
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PIN             GPIO_PIN_10
#define UART_PORT               GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

#endif /* CONFIG_H */
