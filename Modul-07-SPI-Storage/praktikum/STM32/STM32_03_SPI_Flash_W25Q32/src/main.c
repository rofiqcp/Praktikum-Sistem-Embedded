/**
 * ============================================================
 *  STM32 SPI FLASH W25Q32 DRIVER
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Deskripsi:
 *    Driver lengkap untuk W25Q32 SPI Flash Memory.
 *    Operasi: Read JEDEC ID, Erase, Write, Read, Verify.
 *
 *  Wiring:
 *    PA5  -> W25Q32 CLK
 *    PA7  -> W25Q32 DI (MOSI)
 *    PA6  -> W25Q32 DO (MISO)
 *    PA4  -> W25Q32 CS
 *    3.3V -> W25Q32 VCC, WP, HOLD
 *    GND  -> W25Q32 GND
 *
 *  Kapasitas: 32Mbit = 4MByte
 *  Page Size: 256 bytes
 *  Sector Size: 4KB (smallest erase unit)
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "config.h"

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

/* ==================== Buffers ==================== */
static uint8_t write_buffer[W25Q_PAGE_SIZE];
static uint8_t read_buffer[W25Q_PAGE_SIZE];

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void GPIO_Init(void);

/* W25Q32 Driver Functions */
static void w25q_cs_low(void);
static void w25q_cs_high(void);
static uint8_t w25q_spi_transfer(uint8_t data);
static void w25q_read_jedec_id(uint8_t *mfg, uint8_t *mem_type, uint8_t *capacity);
static uint8_t w25q_read_status(void);
static void w25q_write_enable(void);
static void w25q_write_disable(void);
static void w25q_wait_busy(uint32_t timeout_ms);
static void w25q_sector_erase(uint32_t addr);
static void w25q_page_program(uint32_t addr, const uint8_t *data, uint16_t len);
static void w25q_read_data(uint32_t addr, uint8_t *buf, uint16_t len);
static void w25q_power_down(void);
static void w25q_release_power(void);

static void print_hex_dump(const char *label, const uint8_t *data, uint16_t len);
static void run_flash_demo(void);

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
 *  SPI1_Init - Master, Mode 0, 9MHz, 8-bit
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

    /* MISO - PA6 */
    GPIO_InitStruct.Pin = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SPI1_MISO_PORT, &GPIO_InitStruct);

    /* CS - PA4 (manual GPIO) */
    GPIO_InitStruct.Pin = FLASH_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(FLASH_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET);

    /* SPI Configuration */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI1_DATA_SIZE;
    hspi1.Init.CLKPolarity = SPI1_CPOL;
    hspi1.Init.CLKPhase = SPI1_CPHA;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI1_PRESCALER;
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
 *  W25Q32 CS Control
 * ============================================================ */
static void w25q_cs_low(void) {
    HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_RESET);
}

static void w25q_cs_high(void) {
    HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET);
}

/* ============================================================
 *  SPI Single Byte Transfer (transmit + receive)
 * ============================================================ */
static uint8_t w25q_spi_transfer(uint8_t data) {
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

/* ============================================================
 *  Read JEDEC ID (0x9F)
 *  Returns: Manufacturer ID, Memory Type, Capacity
 * ============================================================ */
static void w25q_read_jedec_id(uint8_t *mfg, uint8_t *mem_type, uint8_t *capacity) {
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_JEDEC_ID);   /* 0x9F */
    *mfg = w25q_spi_transfer(0xFF);          /* Manufacturer */
    *mem_type = w25q_spi_transfer(0xFF);     /* Memory Type */
    *capacity = w25q_spi_transfer(0xFF);     /* Capacity */
    w25q_cs_high();
}

/* ============================================================
 *  Read Status Register 1 (0x05)
 * ============================================================ */
static uint8_t w25q_read_status(void) {
    uint8_t status;
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_READ_STATUS_1);  /* 0x05 */
    status = w25q_spi_transfer(0xFF);
    w25q_cs_high();
    return status;
}

/* ============================================================
 *  Write Enable (0x06)
 * ============================================================ */
static void w25q_write_enable(void) {
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_WRITE_ENABLE);   /* 0x06 */
    w25q_cs_high();

    /* Verify WEL bit is set */
    uint8_t status = w25q_read_status();
    if (!(status & W25Q_STATUS_WEL)) {
        printf("[FLASH] WARNING: Write Enable failed! SR=0x%02X\n", status);
    }
}

/* ============================================================
 *  Write Disable (0x04)
 * ============================================================ */
static void w25q_write_disable(void) {
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_WRITE_DISABLE);  /* 0x04 */
    w25q_cs_high();
}

/* ============================================================
 *  Wait for Busy (poll WIP bit in Status Register)
 * ============================================================ */
static void w25q_wait_busy(uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    while ((w25q_read_status() & W25Q_STATUS_BUSY)) {
        if ((HAL_GetTick() - start) > timeout_ms) {
            printf("[FLASH] ERROR: Timeout waiting for busy!\n");
            return;
        }
        HAL_Delay(1);
    }
}

/* ============================================================
 *  Sector Erase (0x20) - Erase 4KB sector
 *  addr: 24-bit address (any address within the sector)
 * ============================================================ */
static void w25q_sector_erase(uint32_t addr) {
    printf("[FLASH] Erasing sector at 0x%06lX...\n", addr);

    w25q_write_enable();

    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_SECTOR_ERASE);           /* 0x20 */
    w25q_spi_transfer((uint8_t)(addr >> 16) & 0xFF);    /* A23-A16 */
    w25q_spi_transfer((uint8_t)(addr >> 8) & 0xFF);     /* A15-A8 */
    w25q_spi_transfer((uint8_t)(addr) & 0xFF);          /* A7-A0 */
    w25q_cs_high();

    w25q_wait_busy(W25Q_TIMEOUT_SECTOR_ERASE);

    printf("[FLASH] Sector erase complete\n");
}

/* ============================================================
 *  Page Program (0x02) - Write up to 256 bytes
 *  addr: 24-bit start address (must be page-aligned for full page)
 *  data: pointer to data buffer
 *  len: number of bytes (max 256)
 * ============================================================ */
static void w25q_page_program(uint32_t addr, const uint8_t *data, uint16_t len) {
    if (len > W25Q_PAGE_SIZE) {
        len = W25Q_PAGE_SIZE;
    }

    printf("[FLASH] Programming %d bytes at 0x%06lX...\n", len, addr);

    w25q_write_enable();

    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_PAGE_PROGRAM);            /* 0x02 */
    w25q_spi_transfer((uint8_t)(addr >> 16) & 0xFF);     /* A23-A16 */
    w25q_spi_transfer((uint8_t)(addr >> 8) & 0xFF);      /* A15-A8 */
    w25q_spi_transfer((uint8_t)(addr) & 0xFF);           /* A7-A0 */

    /* Send data bytes */
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, HAL_MAX_DELAY);

    w25q_cs_high();

    w25q_wait_busy(W25Q_TIMEOUT_PAGE_PROG);

    printf("[FLASH] Page program complete\n");
}

/* ============================================================
 *  Read Data (0x03) - Read arbitrary number of bytes
 *  addr: 24-bit start address
 *  buf: pointer to receive buffer
 *  len: number of bytes to read
 * ============================================================ */
static void w25q_read_data(uint32_t addr, uint8_t *buf, uint16_t len) {
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_READ_DATA);               /* 0x03 */
    w25q_spi_transfer((uint8_t)(addr >> 16) & 0xFF);     /* A23-A16 */
    w25q_spi_transfer((uint8_t)(addr >> 8) & 0xFF);      /* A15-A8 */
    w25q_spi_transfer((uint8_t)(addr) & 0xFF);           /* A7-A0 */

    /* Read data bytes */
    HAL_SPI_Receive(&hspi1, buf, len, HAL_MAX_DELAY);

    w25q_cs_high();
}

/* ============================================================
 *  Power Down (0xB9) - Enter low power mode
 * ============================================================ */
static void w25q_power_down(void) {
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_POWER_DOWN);
    w25q_cs_high();
    HAL_Delay(1);
}

/* ============================================================
 *  Release Power Down (0xAB) - Wake up from power down
 * ============================================================ */
static void w25q_release_power(void) {
    w25q_cs_low();
    w25q_spi_transfer(W25Q_CMD_RELEASE_POWER);
    w25q_cs_high();
    HAL_Delay(1);
}

/* ============================================================
 *  Print Hex Dump
 * ============================================================ */
static void print_hex_dump(const char *label, const uint8_t *data, uint16_t len) {
    printf("  %s [%d bytes]:\n", label, len);
    for (uint16_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
            printf("    %04X: ", i);
        }
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            /* Print ASCII */
            printf(" |");
            for (uint16_t j = i - 15; j <= i; j++) {
                printf("%c", (data[j] >= 32 && data[j] < 127) ? data[j] : '.');
            }
            printf("|\n");
        }
    }
    /* Handle last partial line */
    uint16_t rem = len % 16;
    if (rem != 0) {
        for (uint16_t i = 0; i < (16 - rem); i++) {
            printf("   ");
        }
        printf(" |");
        for (uint16_t j = len - rem; j < len; j++) {
            printf("%c", (data[j] >= 32 && data[j] < 127) ? data[j] : '.');
        }
        printf("|\n");
    }
}

/* ============================================================
 *  Run Flash Demo
 * ============================================================ */
static void run_flash_demo(void) {
    uint8_t mfg_id, mem_type, capacity;
    uint32_t test_addr = TEST_SECTOR_ADDR;
    uint32_t errors = 0;

    printf("\n========================================\n");
    printf("  W25Q32 Flash Memory Test\n");
    printf("========================================\n");

    /* Step 1: Release from power down */
    printf("\n[Step 1] Release from power down\n");
    w25q_release_power();

    /* Step 2: Read JEDEC ID */
    printf("\n[Step 2] Reading JEDEC ID...\n");
    w25q_read_jedec_id(&mfg_id, &mem_type, &capacity);
    printf("  JEDEC ID: 0x%02X 0x%02X 0x%02X\n", mfg_id, mem_type, capacity);
    printf("  Manufacturer: ");
    if (mfg_id == W25Q32_MFG_ID) {
        printf("Winbond (0x%02X) - OK\n", mfg_id);
    } else {
        printf("Unknown (0x%02X) - Expected 0x%02X\n", mfg_id, W25Q32_MFG_ID);
    }
    printf("  Memory Type : 0x%02X\n", mem_type);
    printf("  Capacity    : 0x%02X", capacity);
    if (capacity == W25Q32_CAPACITY) {
        printf(" (32Mbit = 4MB)\n");
    } else {
        printf(" (Unknown)\n");
    }

    /* Step 3: Read Status Register */
    printf("\n[Step 3] Status Register\n");
    uint8_t status = w25q_read_status();
    printf("  Status Reg 1: 0x%02X\n", status);
    printf("  BUSY (WIP)  : %s\n", (status & W25Q_STATUS_BUSY) ? "YES" : "NO");
    printf("  WEL         : %s\n", (status & W25Q_STATUS_WEL) ? "YES" : "NO");

    /* Step 4: Read current data before erase */
    printf("\n[Step 4] Reading data BEFORE erase at 0x%06lX\n", test_addr);
    memset(read_buffer, 0, TEST_DATA_SIZE);
    w25q_read_data(test_addr, read_buffer, TEST_DATA_SIZE);
    print_hex_dump("Before Erase", read_buffer, TEST_DATA_SIZE);

    /* Step 5: Sector Erase */
    printf("\n[Step 5] Erasing sector at 0x%06lX (4KB)\n", test_addr);
    uint32_t start_tick = HAL_GetTick();
    w25q_sector_erase(test_addr);
    uint32_t erase_time = HAL_GetTick() - start_tick;
    printf("  Erase time: %lu ms\n", erase_time);

    /* Verify erase (all bytes should be 0xFF) */
    w25q_read_data(test_addr, read_buffer, TEST_DATA_SIZE);
    uint8_t erase_ok = 1;
    for (uint16_t i = 0; i < TEST_DATA_SIZE; i++) {
        if (read_buffer[i] != 0xFF) {
            erase_ok = 0;
            break;
        }
    }
    printf("  Erase verify: %s\n", erase_ok ? "PASS (all 0xFF)" : "FAIL");
    print_hex_dump("After Erase", read_buffer, TEST_DATA_SIZE);

    /* Step 6: Prepare test data and write */
    printf("\n[Step 6] Writing test data at 0x%06lX\n", test_addr);
    for (uint16_t i = 0; i < TEST_DATA_SIZE; i++) {
        write_buffer[i] = (uint8_t)(i & 0xFF);
    }
    /* Add ASCII signature */
    const char *sig = "STM32-W25Q32";
    memcpy(&write_buffer[0], sig, strlen(sig));

    print_hex_dump("Write Data", write_buffer, TEST_DATA_SIZE);

    start_tick = HAL_GetTick();
    w25q_page_program(test_addr, write_buffer, TEST_DATA_SIZE);
    uint32_t write_time = HAL_GetTick() - start_tick;
    printf("  Write time: %lu ms\n", write_time);

    /* Step 7: Read back and verify */
    printf("\n[Step 7] Reading back and verifying...\n");
    memset(read_buffer, 0, TEST_DATA_SIZE);
    w25q_read_data(test_addr, read_buffer, TEST_DATA_SIZE);
    print_hex_dump("Read Back", read_buffer, TEST_DATA_SIZE);

    errors = 0;
    for (uint16_t i = 0; i < TEST_DATA_SIZE; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            errors++;
            if (errors <= 5) {
                printf("  MISMATCH [%d]: wrote=0x%02X, read=0x%02X\n",
                       i, write_buffer[i], read_buffer[i]);
            }
        }
    }

    printf("\n  Verification: ");
    if (errors == 0) {
        printf("PASS (%d bytes match)\n", TEST_DATA_SIZE);
    } else {
        printf("FAIL (%lu errors in %d bytes)\n", errors, TEST_DATA_SIZE);
    }

    /* Step 8: Power down test */
    printf("\n[Step 8] Power down / wake up test\n");
    w25q_power_down();
    printf("  Flash powered down\n");
    HAL_Delay(100);
    w25q_release_power();
    printf("  Flash woken up\n");

    /* Verify data persists after power cycle */
    memset(read_buffer, 0, TEST_DATA_SIZE);
    w25q_read_data(test_addr, read_buffer, TEST_DATA_SIZE);
    uint8_t persist_ok = 1;
    for (uint16_t i = 0; i < TEST_DATA_SIZE; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            persist_ok = 0;
            break;
        }
    }
    printf("  Data persistence: %s\n", persist_ok ? "PASS" : "FAIL");

    /* Summary */
    printf("\n========================================\n");
    printf("  TEST SUMMARY\n");
    printf("========================================\n");
    printf("  JEDEC ID      : %s\n", (mfg_id == W25Q32_MFG_ID) ? "PASS" : "FAIL");
    printf("  Sector Erase  : %s (%lu ms)\n", erase_ok ? "PASS" : "FAIL", erase_time);
    printf("  Page Write    : %s (%lu ms)\n", (errors == 0) ? "PASS" : "FAIL", write_time);
    printf("  Read Verify   : %s\n", (errors == 0) ? "PASS" : "FAIL");
    printf("  Power Cycle   : %s\n", persist_ok ? "PASS" : "FAIL");
    printf("========================================\n");

    /* LED indication */
    if (errors == 0 && erase_ok && persist_ok) {
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
    printf("  STM32F103 SPI Flash W25Q32 Driver\n");
    printf("  Modul 07 - SPI & Storage\n");
    printf("============================================\n");
    printf("  SYSCLK  : 72 MHz\n");
    printf("  SPI1    : 9 MHz (Prescaler /8)\n");
    printf("  Flash   : W25Q32 (4MB)\n");
    printf("  CS Pin  : PA4\n");
    printf("============================================\n");

    HAL_Delay(500);

    while (1) {
        run_flash_demo();

        printf("\nMenunggu 5 detik...\n");
        HAL_Delay(5000);
    }
}
