/**
 * ============================================================================
 * Program     : STM32_08_Flash_Read_Write
 * Description : Configuration for STM32F103 internal flash read/write
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ==================== Flash Configuration for STM32F103C8 ==================== */
#define FLASH_BASE_ADDR         0x08000000U     /* Flash base address */
#define FLASH_TOTAL_SIZE        (64 * 1024)     /* 64KB flash (medium density) */
#define FLASH_PAGE_SIZE         1024U           /* 1KB per page (medium density) */
#define FLASH_PAGE_COUNT        64              /* Total pages: 64KB / 1KB */

/* ==================== Data Storage Area ==================== */
/* Use last 2 pages for data storage (pages 62-63) */
#define DATA_START_PAGE         62
#define DATA_START_ADDR         (FLASH_BASE_ADDR + (DATA_START_PAGE * FLASH_PAGE_SIZE))  /* 0x0800F800 */
#define DATA_PAGE1_ADDR         DATA_START_ADDR                         /* Page 62 - Calibration data */
#define DATA_PAGE2_ADDR         (DATA_START_ADDR + FLASH_PAGE_SIZE)     /* Page 63 - Boot counter */

/* ==================== Application End ==================== */
#define APP_END_ADDR            DATA_START_ADDR /* Application code must not exceed this */

/* ==================== Calibration Data Structure ==================== */
#define CALIB_MAGIC             0xCAFEBABEU     /* Magic number for validation */
#define CALIB_NAME_MAX          16              /* Max name string length (bytes) */

/* Calibration data structure (must be halfword aligned) */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* Magic number for data validation */
    int16_t  offset;            /* Calibration offset */
    uint16_t gain;              /* Calibration gain (x100) */
    char     name[CALIB_NAME_MAX]; /* Calibration name string */
    uint16_t checksum;          /* Simple checksum */
} CalibrationData_t;

/* ==================== Boot Counter ==================== */
#define BOOT_COUNTER_MAGIC      0xB007C0DEU     /* Boot counter magic */
#define BOOT_COUNTER_OFFSET     0               /* Offset within page 63 */

typedef struct __attribute__((packed)) {
    uint32_t magic;             /* Magic number */
    uint32_t count;             /* Boot count */
    uint32_t last_reset;        /* Last reset timestamp (systick) */
} BootCounter_t;

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE           115200
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PIN             GPIO_PIN_10
#define UART_PORT               GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

/* ==================== Test Configuration ==================== */
#define TEST_PATTERN_SIZE       16  /* Number of halfwords for pattern test */

#endif /* CONFIG_H */
