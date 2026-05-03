/**
 * ============================================================================
 * Program     : STM32_12_Storage_Data_Logger
 * Description : Configuration for internal flash data logger
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ==================== Flash Configuration ==================== */
#define FLASH_BASE_ADDR         0x08000000U
#define FLASH_TOTAL_SIZE        (64 * 1024)         /* 64KB */
#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE         1024U               /* 1KB per page */
#endif
#define FLASH_PAGE_COUNT        64

/* ==================== Data Logger Flash Area ==================== */
/* Pages 56-63 (8 pages = 8KB) for data storage */
#define LOG_START_PAGE          56
#define LOG_END_PAGE            63
#define LOG_NUM_PAGES           (LOG_END_PAGE - LOG_START_PAGE + 1)  /* 8 pages */
#define LOG_START_ADDR          (FLASH_BASE_ADDR + (LOG_START_PAGE * FLASH_PAGE_SIZE))  /* 0x0800E000 */
#define LOG_END_ADDR            (FLASH_BASE_ADDR + ((LOG_END_PAGE + 1) * FLASH_PAGE_SIZE))  /* 0x08010000 */
#define LOG_TOTAL_SIZE          (LOG_NUM_PAGES * FLASH_PAGE_SIZE)    /* 8192 bytes */

/* ==================== Log Entry Structure ==================== */
/* Total size: 4 + 2 + 2 + 2 + 2 = 12 bytes */
/* Entries per page: 1024 / 12 = 85 (with 4 bytes unused) */
#define LOG_ENTRY_MAGIC         0xDA7A              /* Valid entry marker */

typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms;      /* Millisecond timestamp from SysTick */
    int16_t  temperature;       /* Temperature x100 (e.g., 2550 = 25.50°C) */
    int16_t  humidity;          /* Humidity x100 (e.g., 6500 = 65.00%) */
    uint16_t adc_raw;           /* ADC raw value (0-4095) */
    uint16_t magic;             /* Entry magic marker */
} log_entry_t;

#define LOG_ENTRY_SIZE          sizeof(log_entry_t)                     /* 12 bytes */
#define LOG_ENTRIES_PER_PAGE    (FLASH_PAGE_SIZE / LOG_ENTRY_SIZE)      /* 85 entries */
#define LOG_MAX_ENTRIES         (LOG_ENTRIES_PER_PAGE * LOG_NUM_PAGES)  /* 680 entries */

/* ==================== Circular Buffer (RAM) ==================== */
#define RAM_BUFFER_SIZE         16              /* Entries buffered in RAM before flash write */

/* ==================== Simulation Parameters ==================== */
#define TEMP_BASE               2500            /* Base temperature: 25.00°C */
#define TEMP_AMPLITUDE          500             /* Sine amplitude: ±5.00°C */
#define HUMIDITY_BASE           6500            /* Base humidity: 65.00% */
#define HUMIDITY_NOISE          300             /* Noise range: ±3.00% */
#define ADC_SAWTOOTH_MAX        4095            /* ADC max value */
#define ADC_SAWTOOTH_STEP       64              /* ADC increment per sample */

#define LOG_INTERVAL_MS         500             /* Log every 500ms */
#define PRINT_INTERVAL_ENTRIES  50              /* Print stats every 50 entries */

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE           115200
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PIN             GPIO_PIN_10
#define UART_PORT               GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

#endif /* CONFIG_H */
