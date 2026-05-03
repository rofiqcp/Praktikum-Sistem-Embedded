/**
 * ============================================================
 *  KONFIGURASI - STM32 SPI Flash W25Q32
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Board : Blue Pill STM32F103C8T6
 *  Flash : W25Q32 (32Mbit / 4MByte)
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ==================== SPI1 Pin Configuration ==================== */
#define SPI1_SCK_PIN            GPIO_PIN_5
#define SPI1_SCK_PORT           GPIOA
#define SPI1_MOSI_PIN           GPIO_PIN_7
#define SPI1_MOSI_PORT          GPIOA
#define SPI1_MISO_PIN           GPIO_PIN_6
#define SPI1_MISO_PORT          GPIOA

/* ==================== Flash CS Pin ==================== */
#define FLASH_CS_PIN            GPIO_PIN_4
#define FLASH_CS_PORT           GPIOA

/* ==================== SPI1 Parameters ==================== */
#define SPI1_PRESCALER          SPI_BAUDRATEPRESCALER_8   /* 72MHz / 8 = 9MHz */
#define SPI1_CPOL               SPI_POLARITY_LOW
#define SPI1_CPHA               SPI_PHASE_1EDGE
#define SPI1_DATA_SIZE          SPI_DATASIZE_8BIT
#define SPI1_FIRST_BIT          SPI_FIRSTBIT_MSB

/* ==================== W25Q32 Commands ==================== */
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS_1      0x05
#define W25Q_CMD_READ_STATUS_2      0x35
#define W25Q_CMD_WRITE_STATUS       0x01
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_FAST_READ          0x0B
#define W25Q_CMD_PAGE_PROGRAM       0x02
#define W25Q_CMD_SECTOR_ERASE       0x20    /* 4KB */
#define W25Q_CMD_BLOCK_ERASE_32K    0x52    /* 32KB */
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8    /* 64KB */
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_POWER_DOWN         0xB9
#define W25Q_CMD_RELEASE_POWER      0xAB
#define W25Q_CMD_DEVICE_ID          0xAB
#define W25Q_CMD_JEDEC_ID           0x9F
#define W25Q_CMD_UNIQUE_ID          0x4B

/* ==================== W25Q32 Status Register Bits ==================== */
#define W25Q_STATUS_BUSY            0x01    /* Write In Progress */
#define W25Q_STATUS_WEL             0x02    /* Write Enable Latch */

/* ==================== W25Q32 Parameters ==================== */
#define W25Q_PAGE_SIZE              256
#define W25Q_SECTOR_SIZE            4096    /* 4KB */
#define W25Q_BLOCK_SIZE_32K         32768
#define W25Q_BLOCK_SIZE_64K         65536
#define W25Q_TOTAL_SIZE             (4 * 1024 * 1024)  /* 4MB */
#define W25Q_NUM_PAGES              (W25Q_TOTAL_SIZE / W25Q_PAGE_SIZE)
#define W25Q_NUM_SECTORS            (W25Q_TOTAL_SIZE / W25Q_SECTOR_SIZE)

/* ==================== W25Q32 Expected JEDEC ID ==================== */
#define W25Q32_MFG_ID               0xEF    /* Winbond */
#define W25Q32_MEM_TYPE             0x40
#define W25Q32_CAPACITY             0x16    /* 32Mbit */

/* ==================== Timeouts ==================== */
#define W25Q_TIMEOUT_PAGE_PROG      10      /* ms */
#define W25Q_TIMEOUT_SECTOR_ERASE   500     /* ms */
#define W25Q_TIMEOUT_CHIP_ERASE     100000  /* ms */

/* ==================== UART1 Configuration ==================== */
#define UART1_TX_PIN            GPIO_PIN_9
#define UART1_TX_PORT           GPIOA
#define UART1_RX_PIN            GPIO_PIN_10
#define UART1_RX_PORT           GPIOA
#define UART1_BAUDRATE          115200

/* ==================== LED Configuration ==================== */
#define LED_PIN                 GPIO_PIN_13
#define LED_PORT                GPIOC

/* ==================== Test Configuration ==================== */
#define TEST_ADDRESS            0x000000    /* Test at sector 0 */
#define TEST_DATA_SIZE          64
#define TEST_SECTOR_ADDR        0x001000    /* Sector 1 for write test */

#endif /* CONFIG_H */
