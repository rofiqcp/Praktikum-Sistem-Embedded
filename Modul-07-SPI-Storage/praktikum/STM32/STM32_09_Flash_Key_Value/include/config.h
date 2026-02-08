/**
 * ============================================================================
 * Program     : STM32_09_Flash_Key_Value
 * Description : Configuration for flash-based key-value store (NVS-like)
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
#define FLASH_PAGE_SIZE         1024U               /* 1KB per page (medium density) */
#endif
#define FLASH_PAGE_COUNT        64

/* ==================== KV Store Flash Pages ==================== */
/* Use last 2 pages of flash for key-value storage */
#define KV_PAGE_A_NUM           62
#define KV_PAGE_B_NUM           63
#define KV_PAGE_A_ADDR          (FLASH_BASE_ADDR + (KV_PAGE_A_NUM * FLASH_PAGE_SIZE))  /* 0x0800F800 */
#define KV_PAGE_B_ADDR          (FLASH_BASE_ADDR + (KV_PAGE_B_NUM * FLASH_PAGE_SIZE))  /* 0x0800FC00 */

/* ==================== KV Store Limits ==================== */
#define KV_MAX_KEYS             16          /* Maximum number of keys */
#define KV_KEY_MAX_LEN          16          /* Maximum key name length (including null) */
#define KV_VALUE_MAX_SIZE       64          /* Maximum value size in bytes */

/* ==================== KV Entry Magic ==================== */
#define KV_ENTRY_MAGIC          0xABCD      /* Valid entry marker */
#define KV_ENTRY_DELETED        0x0000      /* Deleted entry marker */
#define KV_ENTRY_EMPTY          0xFFFF      /* Erased (empty) marker */

/* ==================== KV Page Header ==================== */
#define KV_PAGE_MAGIC           0x4B56      /* Page header magic ('KV') */
#define KV_PAGE_ACTIVE          0xAC7F      /* Active page marker */
#define KV_PAGE_BACKUP          0xBF0E      /* Backup page marker */

/* ==================== KV Data Types ==================== */
#define KV_TYPE_INT             0           /* int32_t */
#define KV_TYPE_FLOAT           1           /* float */
#define KV_TYPE_STRING          2           /* null-terminated string */
#define KV_TYPE_BLOB            3           /* binary blob */

/* ==================== KV Entry Structure ==================== */
/* Total size: 2 + 16 + 1 + 1 + 64 = 84 bytes */
/* Padded to 84 bytes, ~12 entries per 1KB page */
typedef struct __attribute__((packed)) {
    uint16_t magic;                         /* KV_ENTRY_MAGIC, KV_ENTRY_DELETED, or KV_ENTRY_EMPTY */
    char     key[KV_KEY_MAX_LEN];           /* Key name (null-terminated) */
    uint8_t  type;                          /* Data type: KV_TYPE_INT/FLOAT/STRING/BLOB */
    uint8_t  len;                           /* Actual data length in value[] */
    uint8_t  value[KV_VALUE_MAX_SIZE];      /* Value data */
} kv_flash_entry_t;

/* Entries per page */
#define KV_ENTRIES_PER_PAGE     (FLASH_PAGE_SIZE / sizeof(kv_flash_entry_t))

/* ==================== UART Configuration ==================== */
#define UART_BAUDRATE           115200
#define UART_TX_PIN             GPIO_PIN_9
#define UART_RX_PIN             GPIO_PIN_10
#define UART_PORT               GPIOA

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

#endif /* CONFIG_H */
