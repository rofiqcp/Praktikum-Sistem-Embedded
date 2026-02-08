/**
 * ============================================================
 *  STM32 SPI SD CARD DRIVER (Bare-Metal, No FATFS)
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Deskripsi:
 *    Driver SD Card via SPI mode (bare-metal).
 *    Mendukung SDSC dan SDHC. Operasi raw block read/write.
 *
 *  Wiring (SD Card SPI Mode):
 *    PA5  -> SD CLK  (pin 5)
 *    PA7  -> SD MOSI / DI (pin 2)
 *    PA6  -> SD MISO / DO (pin 7)
 *    PA4  -> SD CS   (pin 1)
 *    3.3V -> SD VCC  (pin 4)
 *    GND  -> SD GND  (pin 3, 6)
 *
 *  Protokol SPI SD:
 *    - Init pada kecepatan lambat (~400kHz)
 *    - Operasi normal pada kecepatan tinggi (9MHz)
 *    - CMD0 -> CMD8 -> CMD55+ACMD41 -> CMD58
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "config.h"

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

/* ==================== SD Card State ==================== */
static uint8_t sd_type = SD_TYPE_UNKNOWN;
static uint8_t sd_initialized = 0;

/* ==================== Buffers ==================== */
static uint8_t block_buffer[SD_BLOCK_SIZE];
static uint8_t verify_buffer[SD_BLOCK_SIZE];

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void GPIO_Init(void);

/* SPI Speed Control */
static void spi_set_slow(void);
static void spi_set_fast(void);

/* SD Card Low-Level */
static void sd_cs_low(void);
static void sd_cs_high(void);
static uint8_t sd_spi_transfer(uint8_t data);
static void sd_send_dummy_clocks(uint8_t count);
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg);
static uint8_t sd_send_acmd(uint8_t cmd, uint32_t arg);
static uint8_t sd_wait_ready(uint16_t timeout_ms);

/* SD Card Operations */
static uint8_t sd_init(void);
static uint8_t sd_read_block(uint32_t block_addr, uint8_t *buf);
static uint8_t sd_write_block(uint32_t block_addr, const uint8_t *data);

/* Demo */
static void print_hex_dump(const char *label, const uint8_t *data, uint16_t len);
static void run_sd_demo(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 *  SystemClock_Config
 *  HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
 * ============================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        while (1);
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  UART1_Init - PA9(TX), PA10(RX), 115200 baud
 * ============================================================ */
static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = UART1_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART1_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = UART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(UART1_RX_PORT, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART1_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  SPI1_Init - Start with slow speed for SD init
 * ============================================================ */
static void SPI1_Init(void) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* SCK - PA5 */
    GPIO_InitStruct.Pin = SPI1_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_SCK_PORT, &GPIO_InitStruct);

    /* MOSI - PA7 */
    GPIO_InitStruct.Pin = SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_MOSI_PORT, &GPIO_InitStruct);

    /* MISO - PA6 (input with pull-up) */
    GPIO_InitStruct.Pin = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SPI1_MISO_PORT, &GPIO_InitStruct);

    /* CS - PA4 (manual GPIO) */
    GPIO_InitStruct.Pin = SD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SD_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);

    /* SPI Config - Start SLOW for SD init */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI1_DATA_SIZE;
    hspi1.Init.CLKPolarity = SPI1_CPOL;
    hspi1.Init.CLKPhase = SPI1_CPHA;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_PRESCALER_SLOW;
    hspi1.Init.FirstBit = SPI1_FIRST_BIT;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  GPIO_Init - LED PC13
 * ============================================================ */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ============================================================
 *  SPI Speed Control - Reinitialize SPI at different speed
 * ============================================================ */
static void spi_set_slow(void) {
    HAL_SPI_DeInit(&hspi1);
    hspi1.Init.BaudRatePrescaler = SPI_PRESCALER_SLOW;
    HAL_SPI_Init(&hspi1);
}

static void spi_set_fast(void) {
    HAL_SPI_DeInit(&hspi1);
    hspi1.Init.BaudRatePrescaler = SPI_PRESCALER_FAST;
    HAL_SPI_Init(&hspi1);
}

/* ============================================================
 *  SD Card CS Control
 * ============================================================ */
static void sd_cs_low(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
    /* Small delay for CS setup */
    sd_spi_transfer(0xFF);
}

static void sd_cs_high(void) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    /* Extra clock for SD card release */
    sd_spi_transfer(0xFF);
}

/* ============================================================
 *  SPI Single Byte Transfer
 * ============================================================ */
static uint8_t sd_spi_transfer(uint8_t data) {
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

/* ============================================================
 *  Send Dummy Clock Cycles (CS HIGH)
 * ============================================================ */
static void sd_send_dummy_clocks(uint8_t count) {
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    for (uint8_t i = 0; i < count; i++) {
        sd_spi_transfer(0xFF);
    }
}

/* ============================================================
 *  Wait for SD card ready (DO goes HIGH / 0xFF)
 * ============================================================ */
static uint8_t sd_wait_ready(uint16_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    uint8_t resp;

    do {
        resp = sd_spi_transfer(0xFF);
        if (resp == 0xFF) return 0;  /* Ready */
    } while ((HAL_GetTick() - start) < timeout_ms);

    return 1;  /* Timeout */
}

/* ============================================================
 *  Send SD Command (6-byte frame)
 *  Format: [0x40|cmd][arg3][arg2][arg1][arg0][crc|0x01]
 *  Returns: R1 response byte
 * ============================================================ */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t resp;
    uint8_t crc = 0xFF;
    uint8_t retry;

    /* Wait for card ready */
    if (cmd != SD_CMD0) {
        sd_wait_ready(500);
    }

    /* Compute CRC for critical commands */
    if (cmd == SD_CMD0) {
        crc = 0x95;  /* Valid CRC for CMD0(0) */
    } else if (cmd == SD_CMD8) {
        crc = 0x87;  /* Valid CRC for CMD8(0x1AA) */
    } else {
        crc = 0x01;  /* Dummy CRC (CRC disabled after init) */
    }

    /* Send 6-byte command frame */
    sd_spi_transfer(0x40 | cmd);                     /* Command index */
    sd_spi_transfer((uint8_t)(arg >> 24) & 0xFF);    /* Argument [31:24] */
    sd_spi_transfer((uint8_t)(arg >> 16) & 0xFF);    /* Argument [23:16] */
    sd_spi_transfer((uint8_t)(arg >> 8) & 0xFF);     /* Argument [15:8] */
    sd_spi_transfer((uint8_t)(arg) & 0xFF);          /* Argument [7:0] */
    sd_spi_transfer(crc);                             /* CRC + stop bit */

    /* Wait for response (bit 7 = 0 indicates valid response) */
    /* Skip a stuff byte for CMD12 (stop transmission) */
    if (cmd == SD_CMD12) {
        sd_spi_transfer(0xFF);
    }

    retry = 0;
    do {
        resp = sd_spi_transfer(0xFF);
        retry++;
    } while ((resp & 0x80) && retry < SD_TIMEOUT_CMD);

    return resp;
}

/* ============================================================
 *  Send Application-Specific Command (CMD55 + ACMDxx)
 * ============================================================ */
static uint8_t sd_send_acmd(uint8_t cmd, uint32_t arg) {
    sd_send_cmd(SD_CMD55, 0);
    return sd_send_cmd(cmd, arg);
}

/* ============================================================
 *  SD Card Initialization
 *  CMD0 -> CMD8 -> CMD55+ACMD41 -> CMD58
 *  Returns: 0=success, 1=fail
 * ============================================================ */
static uint8_t sd_init(void) {
    uint8_t resp;
    uint8_t ocr[4];
    uint32_t start;

    sd_type = SD_TYPE_UNKNOWN;
    sd_initialized = 0;

    printf("[SD] Starting initialization...\n");

    /* Step 1: Power up - send 80+ clock cycles with CS HIGH */
    printf("[SD] Step 1: Power up (80 clocks, CS HIGH)\n");
    spi_set_slow();
    sd_send_dummy_clocks(10);  /* 10 bytes = 80 clocks */

    /* Step 2: CMD0 - GO_IDLE_STATE (software reset) */
    printf("[SD] Step 2: CMD0 - GO_IDLE_STATE\n");
    sd_cs_low();
    resp = sd_send_cmd(SD_CMD0, 0);
    sd_cs_high();

    printf("[SD]   CMD0 response: 0x%02X\n", resp);
    if (resp != SD_R1_IDLE) {
        printf("[SD]   ERROR: Expected 0x01 (idle), got 0x%02X\n", resp);
        return 1;
    }
    printf("[SD]   Card in IDLE state - OK\n");

    /* Step 3: CMD8 - SEND_IF_COND (check voltage, SDv2 detection) */
    printf("[SD] Step 3: CMD8 - SEND_IF_COND\n");
    sd_cs_low();
    resp = sd_send_cmd(SD_CMD8, ((uint32_t)SD_CMD8_VHS << 8) | SD_CMD8_CHECK);

    if (resp == SD_R1_IDLE) {
        /* SDv2 card - read 4-byte R7 response */
        ocr[0] = sd_spi_transfer(0xFF);
        ocr[1] = sd_spi_transfer(0xFF);
        ocr[2] = sd_spi_transfer(0xFF);
        ocr[3] = sd_spi_transfer(0xFF);
        sd_cs_high();

        printf("[SD]   CMD8 R7: %02X %02X %02X %02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);

        if (ocr[2] != SD_CMD8_VHS || ocr[3] != SD_CMD8_CHECK) {
            printf("[SD]   ERROR: Voltage/check pattern mismatch!\n");
            return 1;
        }
        printf("[SD]   SDv2 card detected\n");

        /* Step 4: CMD55 + ACMD41 - Initialize SDv2 card */
        printf("[SD] Step 4: ACMD41 - SD_SEND_OP_COND (with HCS)\n");
        start = HAL_GetTick();
        do {
            sd_cs_low();
            resp = sd_send_acmd(SD_ACMD41, 0x40000000);  /* HCS bit set */
            sd_cs_high();

            if ((HAL_GetTick() - start) > SD_TIMEOUT_INIT) {
                printf("[SD]   ERROR: ACMD41 timeout!\n");
                return 1;
            }
        } while (resp != 0x00);

        printf("[SD]   ACMD41 complete, card ready\n");

        /* Step 5: CMD58 - Read OCR to check CCS (SDHC flag) */
        printf("[SD] Step 5: CMD58 - READ_OCR\n");
        sd_cs_low();
        resp = sd_send_cmd(SD_CMD58, 0);
        if (resp == 0x00) {
            ocr[0] = sd_spi_transfer(0xFF);
            ocr[1] = sd_spi_transfer(0xFF);
            ocr[2] = sd_spi_transfer(0xFF);
            ocr[3] = sd_spi_transfer(0xFF);

            printf("[SD]   OCR: %02X %02X %02X %02X\n", ocr[0], ocr[1], ocr[2], ocr[3]);

            if (ocr[0] & 0x40) {
                sd_type = SD_TYPE_SDHC;
                printf("[SD]   Card type: SDHC (block addressing)\n");
            } else {
                sd_type = SD_TYPE_SDSC;
                printf("[SD]   Card type: SDSC (byte addressing)\n");
            }
        }
        sd_cs_high();

    } else if (resp == (SD_R1_IDLE | SD_R1_ILLEGAL_CMD)) {
        /* SDv1 or MMC card */
        sd_cs_high();
        printf("[SD]   CMD8 illegal - SDv1/MMC card\n");

        /* Try ACMD41 for SDv1 */
        printf("[SD] Step 4: ACMD41 - SD_SEND_OP_COND (SDv1)\n");
        start = HAL_GetTick();
        do {
            sd_cs_low();
            resp = sd_send_acmd(SD_ACMD41, 0);
            sd_cs_high();

            if ((HAL_GetTick() - start) > SD_TIMEOUT_INIT) {
                /* Not SD, try MMC with CMD1 */
                printf("[SD]   ACMD41 failed, trying CMD1 (MMC)\n");
                start = HAL_GetTick();
                do {
                    sd_cs_low();
                    resp = sd_send_cmd(SD_CMD1, 0);
                    sd_cs_high();

                    if ((HAL_GetTick() - start) > SD_TIMEOUT_INIT) {
                        printf("[SD]   ERROR: CMD1 timeout - no card?\n");
                        return 1;
                    }
                } while (resp != 0x00);

                sd_type = SD_TYPE_MMC;
                printf("[SD]   Card type: MMC\n");
                break;
            }
        } while (resp != 0x00);

        if (sd_type == SD_TYPE_UNKNOWN) {
            sd_type = SD_TYPE_SDSC;
            printf("[SD]   Card type: SDSC (SDv1)\n");
        }

        /* Set block length to 512 for SDSC */
        sd_cs_low();
        resp = sd_send_cmd(SD_CMD16, SD_BLOCK_SIZE);
        sd_cs_high();
        if (resp != 0x00) {
            printf("[SD]   WARNING: CMD16 response: 0x%02X\n", resp);
        }

    } else {
        sd_cs_high();
        printf("[SD]   ERROR: CMD8 unexpected response: 0x%02X\n", resp);
        return 1;
    }

    /* Switch to fast SPI speed */
    printf("[SD] Switching to fast SPI speed (9 MHz)\n");
    spi_set_fast();

    sd_initialized = 1;
    printf("[SD] Initialization complete!\n");

    return 0;
}

/* ============================================================
 *  SD Read Single Block (CMD17)
 *  block_addr: block number (SDHC) or byte address (SDSC)
 *  buf: 512-byte receive buffer
 *  Returns: 0=success, 1=fail
 * ============================================================ */
static uint8_t sd_read_block(uint32_t block_addr, uint8_t *buf) {
    uint8_t resp;
    uint32_t addr;
    uint32_t start;

    /* Convert block address to byte address for SDSC */
    if (sd_type == SD_TYPE_SDHC) {
        addr = block_addr;          /* Block addressing */
    } else {
        addr = block_addr * SD_BLOCK_SIZE;  /* Byte addressing */
    }

    sd_cs_low();

    /* Send CMD17 */
    resp = sd_send_cmd(SD_CMD17, addr);
    if (resp != 0x00) {
        sd_cs_high();
        printf("[SD] CMD17 error: 0x%02X\n", resp);
        return 1;
    }

    /* Wait for data start token (0xFE) */
    start = HAL_GetTick();
    do {
        resp = sd_spi_transfer(0xFF);
        if ((HAL_GetTick() - start) > SD_TIMEOUT_READ) {
            sd_cs_high();
            printf("[SD] Read timeout!\n");
            return 1;
        }
    } while (resp != SD_TOKEN_START_BLOCK);

    /* Read 512 bytes of data */
    HAL_SPI_Receive(&hspi1, buf, SD_BLOCK_SIZE, HAL_MAX_DELAY);

    /* Read and discard 2-byte CRC */
    sd_spi_transfer(0xFF);
    sd_spi_transfer(0xFF);

    sd_cs_high();

    return 0;
}

/* ============================================================
 *  SD Write Single Block (CMD24)
 *  block_addr: block number (SDHC) or byte address (SDSC)
 *  data: 512-byte data buffer
 *  Returns: 0=success, 1=fail
 * ============================================================ */
static uint8_t sd_write_block(uint32_t block_addr, const uint8_t *data) {
    uint8_t resp;
    uint32_t addr;
    uint32_t start;

    /* Convert block address */
    if (sd_type == SD_TYPE_SDHC) {
        addr = block_addr;
    } else {
        addr = block_addr * SD_BLOCK_SIZE;
    }

    sd_cs_low();

    /* Send CMD24 */
    resp = sd_send_cmd(SD_CMD24, addr);
    if (resp != 0x00) {
        sd_cs_high();
        printf("[SD] CMD24 error: 0x%02X\n", resp);
        return 1;
    }

    /* Send dummy byte before data */
    sd_spi_transfer(0xFF);

    /* Send data start token */
    sd_spi_transfer(SD_TOKEN_START_BLOCK);

    /* Send 512 bytes of data */
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, SD_BLOCK_SIZE, HAL_MAX_DELAY);

    /* Send dummy 2-byte CRC */
    sd_spi_transfer(0xFF);
    sd_spi_transfer(0xFF);

    /* Read data response token */
    resp = sd_spi_transfer(0xFF);
    if ((resp & 0x1F) != SD_DATA_ACCEPTED) {
        sd_cs_high();
        printf("[SD] Write data response error: 0x%02X\n", resp);
        return 1;
    }

    /* Wait for write to complete (card busy = DO low) */
    start = HAL_GetTick();
    while (sd_spi_transfer(0xFF) == 0x00) {
        if ((HAL_GetTick() - start) > SD_TIMEOUT_WRITE) {
            sd_cs_high();
            printf("[SD] Write busy timeout!\n");
            return 1;
        }
    }

    sd_cs_high();

    return 0;
}

/* ============================================================
 *  Print Hex Dump (first N bytes of 512)
 * ============================================================ */
static void print_hex_dump(const char *label, const uint8_t *data, uint16_t len) {
    uint16_t print_len = (len > 64) ? 64 : len;

    printf("  %s [showing %d of %d bytes]:\n", label, print_len, len);
    for (uint16_t i = 0; i < print_len; i++) {
        if (i % 16 == 0) {
            printf("    %04X: ", i);
        }
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf(" |");
            for (uint16_t j = i - 15; j <= i; j++) {
                printf("%c", (data[j] >= 32 && data[j] < 127) ? data[j] : '.');
            }
            printf("|\n");
        }
    }
    if (print_len % 16 != 0) {
        uint16_t rem = print_len % 16;
        for (uint16_t i = 0; i < (16 - rem); i++) printf("   ");
        printf(" |");
        for (uint16_t j = print_len - rem; j < print_len; j++) {
            printf("%c", (data[j] >= 32 && data[j] < 127) ? data[j] : '.');
        }
        printf("|\n");
    }
    if (len > 64) {
        printf("    ... (%d more bytes)\n", len - 64);
    }
}

/* ============================================================
 *  Run SD Card Demo
 * ============================================================ */
static void run_sd_demo(void) {
    uint8_t result;
    uint32_t errors = 0;

    printf("\n========================================\n");
    printf("  SD Card SPI Mode Test\n");
    printf("========================================\n");

    /* Step 1: Initialize */
    printf("\n[Step 1] Initializing SD Card...\n");
    result = sd_init();
    if (result != 0) {
        printf("[SD] INITIALIZATION FAILED!\n");
        printf("[SD] Check wiring and card insertion.\n");
        printf("[SD] Retrying in 5 seconds...\n");
        return;
    }

    printf("\n[SD] Card Type: ");
    switch (sd_type) {
        case SD_TYPE_SDSC: printf("SDSC (Standard Capacity)\n"); break;
        case SD_TYPE_SDHC: printf("SDHC (High Capacity)\n"); break;
        case SD_TYPE_MMC:  printf("MMC\n"); break;
        default:           printf("Unknown\n"); break;
    }

    /* Step 2: Read existing data at test block */
    printf("\n[Step 2] Reading block %lu (0x%04lX)...\n",
           (unsigned long)TEST_BLOCK_ADDR, (unsigned long)TEST_BLOCK_ADDR);
    memset(block_buffer, 0, SD_BLOCK_SIZE);
    uint32_t start_tick = HAL_GetTick();
    result = sd_read_block(TEST_BLOCK_ADDR, block_buffer);
    uint32_t read_time = HAL_GetTick() - start_tick;

    if (result == 0) {
        printf("[SD]   Read OK (%lu ms)\n", read_time);
        print_hex_dump("Current Data", block_buffer, SD_BLOCK_SIZE);
    } else {
        printf("[SD]   Read FAILED!\n");
    }

    /* Step 3: Prepare and write test data */
    printf("\n[Step 3] Writing test data to block %lu...\n",
           (unsigned long)TEST_BLOCK_ADDR);

    /* Fill with test pattern */
    memset(block_buffer, 0x00, SD_BLOCK_SIZE);
    const char *header = "STM32F103-SD-SPI-TEST";
    memcpy(block_buffer, header, strlen(header));

    /* Fill rest with pattern */
    for (uint16_t i = strlen(header); i < SD_BLOCK_SIZE; i++) {
        block_buffer[i] = (uint8_t)(i & 0xFF);
    }

    /* Add timestamp */
    uint32_t ticks = HAL_GetTick();
    block_buffer[SD_BLOCK_SIZE - 4] = (uint8_t)(ticks >> 24);
    block_buffer[SD_BLOCK_SIZE - 3] = (uint8_t)(ticks >> 16);
    block_buffer[SD_BLOCK_SIZE - 2] = (uint8_t)(ticks >> 8);
    block_buffer[SD_BLOCK_SIZE - 1] = (uint8_t)(ticks);

    print_hex_dump("Write Data", block_buffer, SD_BLOCK_SIZE);

    start_tick = HAL_GetTick();
    result = sd_write_block(TEST_BLOCK_ADDR, block_buffer);
    uint32_t write_time = HAL_GetTick() - start_tick;

    if (result == 0) {
        printf("[SD]   Write OK (%lu ms)\n", write_time);
    } else {
        printf("[SD]   Write FAILED!\n");
        return;
    }

    /* Step 4: Read back and verify */
    printf("\n[Step 4] Reading back and verifying...\n");
    memset(verify_buffer, 0, SD_BLOCK_SIZE);
    start_tick = HAL_GetTick();
    result = sd_read_block(TEST_BLOCK_ADDR, verify_buffer);
    uint32_t verify_read_time = HAL_GetTick() - start_tick;

    if (result != 0) {
        printf("[SD]   Verify read FAILED!\n");
        return;
    }

    printf("[SD]   Read back OK (%lu ms)\n", verify_read_time);
    print_hex_dump("Verify Data", verify_buffer, SD_BLOCK_SIZE);

    /* Compare */
    errors = 0;
    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++) {
        if (block_buffer[i] != verify_buffer[i]) {
            errors++;
            if (errors <= 5) {
                printf("[SD]   MISMATCH at [%d]: wrote=0x%02X, read=0x%02X\n",
                       i, block_buffer[i], verify_buffer[i]);
            }
        }
    }

    /* Summary */
    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("  Card Type     : %s\n",
           sd_type == SD_TYPE_SDHC ? "SDHC" :
           sd_type == SD_TYPE_SDSC ? "SDSC" :
           sd_type == SD_TYPE_MMC  ? "MMC" : "Unknown");
    printf("  Init          : %s\n", sd_initialized ? "PASS" : "FAIL");
    printf("  Block Read    : %lu ms\n", read_time);
    printf("  Block Write   : %lu ms\n", write_time);
    printf("  Verify Read   : %lu ms\n", verify_read_time);
    printf("  Data Verify   : %s (%lu errors in %d bytes)\n",
           errors == 0 ? "PASS" : "FAIL", errors, SD_BLOCK_SIZE);
    printf("  Test Block    : %lu (0x%04lX)\n",
           (unsigned long)TEST_BLOCK_ADDR, (unsigned long)TEST_BLOCK_ADDR);
    printf("========================================\n");

    if (errors == 0 && sd_initialized) {
        printf("\n  >>> ALL TESTS PASSED <<<\n");
        for (int i = 0; i < 6; i++) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            HAL_Delay(100);
        }
    } else {
        printf("\n  >>> SOME TESTS FAILED <<<\n");
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    }
}

/* ============================================================
 *  Main Entry Point
 * ============================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    SPI1_Init();
    GPIO_Init();

    printf("\n\n");
    printf("============================================\n");
    printf("  STM32F103 SPI SD Card (Bare-Metal)\n");
    printf("  Modul 07 - SPI & Storage\n");
    printf("============================================\n");
    printf("  SYSCLK    : 72 MHz\n");
    printf("  SPI Init  : ~281 kHz (slow)\n");
    printf("  SPI Normal: 9 MHz (fast)\n");
    printf("  CS Pin    : PA4\n");
    printf("============================================\n");

    HAL_Delay(500);

    while (1) {
        run_sd_demo();

        printf("\nMenunggu 5 detik...\n");
        HAL_Delay(5000);
    }
}
