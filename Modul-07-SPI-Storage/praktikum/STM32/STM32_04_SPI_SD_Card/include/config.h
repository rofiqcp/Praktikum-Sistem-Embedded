/**
 * ============================================================
 *  KONFIGURASI - STM32 SPI SD CARD
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Board : Blue Pill STM32F103C8T6
 *  SD    : MicroSD via SPI mode (bare-metal, no FATFS)
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

/* ==================== SD Card CS Pin ==================== */
#define SD_CS_PIN               GPIO_PIN_4
#define SD_CS_PORT              GPIOA

/* ==================== SPI Prescalers ==================== */
#define SPI_PRESCALER_SLOW      SPI_BAUDRATEPRESCALER_256  /* 72MHz/256 = ~281kHz (init) */
#define SPI_PRESCALER_FAST      SPI_BAUDRATEPRESCALER_8    /* 72MHz/8 = 9MHz (normal) */

/* ==================== SPI1 Parameters ==================== */
#define SPI1_CPOL               SPI_POLARITY_LOW
#define SPI1_CPHA               SPI_PHASE_1EDGE
#define SPI1_DATA_SIZE          SPI_DATASIZE_8BIT
#define SPI1_FIRST_BIT          SPI_FIRSTBIT_MSB

/* ==================== SD Card SPI Commands ==================== */
#define SD_CMD0     0   /* GO_IDLE_STATE - Reset to idle */
#define SD_CMD1     1   /* SEND_OP_COND - Init (MMC) */
#define SD_CMD8     8   /* SEND_IF_COND - Check voltage range */
#define SD_CMD9     9   /* SEND_CSD - Read CSD register */
#define SD_CMD10    10  /* SEND_CID - Read CID register */
#define SD_CMD12    12  /* STOP_TRANSMISSION */
#define SD_CMD16    16  /* SET_BLOCKLEN - Set block length */
#define SD_CMD17    17  /* READ_SINGLE_BLOCK */
#define SD_CMD18    18  /* READ_MULTIPLE_BLOCK */
#define SD_CMD24    24  /* WRITE_BLOCK - Write single block */
#define SD_CMD25    25  /* WRITE_MULTIPLE_BLOCK */
#define SD_CMD55    55  /* APP_CMD - Prefix for ACMD */
#define SD_CMD58    58  /* READ_OCR - Read OCR register */
#define SD_ACMD41   41  /* SD_SEND_OP_COND - Init (SD) */

/* ==================== SD Card Response Tokens ==================== */
#define SD_R1_IDLE          0x01
#define SD_R1_ERASE_RESET   0x02
#define SD_R1_ILLEGAL_CMD   0x04
#define SD_R1_CRC_ERROR     0x08
#define SD_R1_ERASE_SEQ     0x10
#define SD_R1_ADDR_ERROR    0x20
#define SD_R1_PARAM_ERROR   0x40

/* ==================== SD Card Data Tokens ==================== */
#define SD_TOKEN_START_BLOCK        0xFE
#define SD_TOKEN_START_MULTI_WRITE  0xFC
#define SD_TOKEN_STOP_MULTI_WRITE   0xFD

/* ==================== SD Card Data Response ==================== */
#define SD_DATA_ACCEPTED    0x05
#define SD_DATA_CRC_ERROR   0x0B
#define SD_DATA_WRITE_ERROR 0x0D

/* ==================== SD Card Types ==================== */
#define SD_TYPE_UNKNOWN     0
#define SD_TYPE_MMC         1
#define SD_TYPE_SDSC        2   /* Standard Capacity (up to 2GB) */
#define SD_TYPE_SDHC        3   /* High Capacity (up to 32GB) */

/* ==================== Block Size ==================== */
#define SD_BLOCK_SIZE       512

/* ==================== Timeouts ==================== */
#define SD_TIMEOUT_CMD      100     /* Command response timeout (cycles) */
#define SD_TIMEOUT_READ     2000    /* Read timeout (ms) */
#define SD_TIMEOUT_WRITE    2500    /* Write timeout (ms) */
#define SD_TIMEOUT_INIT     5000    /* Init timeout (ms) */

/* ==================== CMD8 Check Pattern ==================== */
#define SD_CMD8_VHS         0x01    /* 2.7-3.6V */
#define SD_CMD8_CHECK       0xAA    /* Check pattern */

/* ==================== Test Configuration ==================== */
#define TEST_BLOCK_ADDR     0x1000  /* Test at block 4096 (safe area) */
#define TEST_DATA_SIZE      SD_BLOCK_SIZE

/* ==================== UART1 Configuration ==================== */
#define UART1_TX_PIN        GPIO_PIN_9
#define UART1_TX_PORT       GPIOA
#define UART1_RX_PIN        GPIO_PIN_10
#define UART1_RX_PORT       GPIOA
#define UART1_BAUDRATE      115200

/* ==================== LED Configuration ==================== */
#define LED_PIN             GPIO_PIN_13
#define LED_PORT            GPIOC

#endif /* CONFIG_H */
