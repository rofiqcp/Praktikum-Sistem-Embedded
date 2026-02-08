/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_08_Flash_Read_Write
 * Description : Internal flash read/write for STM32F103 (medium density)
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA9  -> USB-TTL RX  (USART1 TX)
 *   PA10 <- USB-TTL TX  (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * Flash Layout (STM32F103C8 - 64KB):
 *   0x08000000 - 0x0800F7FF : Application code (pages 0-61)
 *   0x0800F800 - 0x0800FBFF : Page 62 - Calibration data
 *   0x0800FC00 - 0x0800FFFF : Page 63 - Boot counter
 *
 * Notes:
 *   - STM32F103 medium density: 1KB page size
 *   - Flash programming granularity: halfword (16-bit)
 *   - Flash must be erased (0xFF) before writing
 *   - Erase sets all bits to 1, programming clears bits to 0
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Global Handles ==================== */
UART_HandleTypeDef huart1;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);

/* Flash operations */
HAL_StatusTypeDef flash_erase_page(uint32_t page_addr);
HAL_StatusTypeDef flash_write_halfword(uint32_t addr, uint16_t data);
HAL_StatusTypeDef flash_write_data(uint32_t addr, uint16_t *data_ptr, uint32_t len);
void              flash_read_data(uint32_t addr, uint16_t *buf, uint32_t len);

/* Calibration data operations */
HAL_StatusTypeDef write_calibration_data(const CalibrationData_t *calib);
HAL_StatusTypeDef read_calibration_data(CalibrationData_t *calib);
uint16_t          compute_checksum(const CalibrationData_t *calib);
void              print_calibration_data(const CalibrationData_t *calib);

/* Boot counter operations */
HAL_StatusTypeDef boot_counter_init(void);
uint32_t          boot_counter_read(void);
HAL_StatusTypeDef boot_counter_increment(void);
void              print_boot_counter(void);

/* Test and demo functions */
void demo_flash_info(void);
void demo_pattern_test(void);
void demo_calibration_data(void);
void demo_boot_counter(void);
void print_flash_page_hex(uint32_t addr, uint32_t len);

void Error_Handler(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== System Clock Configuration ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== GPIO Initialization ==================== */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ==================== UART1 Initialization ==================== */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = UART_RX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== Flash Erase Page ==================== */
/**
 * Erase a single flash page.
 * @param page_addr  Page-aligned address to erase (must be multiple of FLASH_PAGE_SIZE)
 * @return           HAL_OK on success
 */
HAL_StatusTypeDef flash_erase_page(uint32_t page_addr)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error = 0;

    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = page_addr;
    erase_init.NbPages     = 1;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        printf("[ERROR] Flash unlock failed!\r\n");
        return status;
    }

    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK)
    {
        printf("[ERROR] Flash erase failed at page 0x%08lX, error=0x%08lX\r\n",
               page_addr, page_error);
    }

    HAL_FLASH_Lock();
    return status;
}

/* ==================== Flash Write Halfword ==================== */
/**
 * Write a single halfword (16-bit) to flash.
 * @param addr  Halfword-aligned address
 * @param data  16-bit data to write
 * @return      HAL_OK on success
 */
HAL_StatusTypeDef flash_write_halfword(uint32_t addr, uint16_t data)
{
    HAL_StatusTypeDef status;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return status;

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, (uint64_t)data);
    if (status != HAL_OK)
    {
        printf("[ERROR] Flash program failed at 0x%08lX\r\n", addr);
    }

    HAL_FLASH_Lock();
    return status;
}

/* ==================== Flash Write Data Array ==================== */
/**
 * Write an array of halfwords to flash.
 * Flash must be erased first!
 *
 * @param addr      Start address (halfword-aligned)
 * @param data_ptr  Pointer to halfword array
 * @param len       Number of halfwords to write
 * @return          HAL_OK on success
 */
HAL_StatusTypeDef flash_write_data(uint32_t addr, uint16_t *data_ptr, uint32_t len)
{
    HAL_StatusTypeDef status;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        printf("[ERROR] Flash unlock failed!\r\n");
        return status;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                   addr + (i * 2), (uint64_t)data_ptr[i]);
        if (status != HAL_OK)
        {
            printf("[ERROR] Flash write failed at offset %lu (addr 0x%08lX)\r\n",
                   i, addr + (i * 2));
            HAL_FLASH_Lock();
            return status;
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

/* ==================== Flash Read Data ==================== */
/**
 * Read halfwords directly from flash memory.
 * Flash is memory-mapped, so we can read directly.
 *
 * @param addr  Start address
 * @param buf   Destination buffer (halfword array)
 * @param len   Number of halfwords to read
 */
void flash_read_data(uint32_t addr, uint16_t *buf, uint32_t len)
{
    volatile uint16_t *flash_ptr = (volatile uint16_t *)addr;

    for (uint32_t i = 0; i < len; i++)
    {
        buf[i] = flash_ptr[i];
    }
}

/* ==================== Compute Checksum ==================== */
uint16_t compute_checksum(const CalibrationData_t *calib)
{
    const uint8_t *data = (const uint8_t *)calib;
    uint16_t sum = 0;

    /* Sum all bytes except the checksum field itself */
    for (uint32_t i = 0; i < offsetof(CalibrationData_t, checksum); i++)
    {
        sum += data[i];
    }
    return sum;
}

/* ==================== Write Calibration Data ==================== */
HAL_StatusTypeDef write_calibration_data(const CalibrationData_t *calib)
{
    HAL_StatusTypeDef status;
    uint32_t num_halfwords = (sizeof(CalibrationData_t) + 1) / 2;

    printf("[FLASH] Writing calibration data to page 62 (0x%08lX)...\r\n",
           (uint32_t)DATA_PAGE1_ADDR);

    /* Step 1: Erase the page */
    status = flash_erase_page(DATA_PAGE1_ADDR);
    if (status != HAL_OK)
    {
        printf("[ERROR] Page erase failed!\r\n");
        return status;
    }
    printf("  Page erased OK\r\n");

    /* Step 2: Write calibration data as halfwords */
    status = flash_write_data(DATA_PAGE1_ADDR, (uint16_t *)calib, num_halfwords);
    if (status != HAL_OK)
    {
        printf("[ERROR] Write calibration failed!\r\n");
        return status;
    }
    printf("  %lu halfwords written OK\r\n", num_halfwords);

    return HAL_OK;
}

/* ==================== Read Calibration Data ==================== */
HAL_StatusTypeDef read_calibration_data(CalibrationData_t *calib)
{
    uint32_t num_halfwords = (sizeof(CalibrationData_t) + 1) / 2;

    printf("[FLASH] Reading calibration data from 0x%08lX...\r\n",
           (uint32_t)DATA_PAGE1_ADDR);

    flash_read_data(DATA_PAGE1_ADDR, (uint16_t *)calib, num_halfwords);

    /* Validate magic number */
    if (calib->magic != CALIB_MAGIC)
    {
        printf("[ERROR] Invalid magic: 0x%08lX (expected 0x%08lX)\r\n",
               (uint32_t)calib->magic, (uint32_t)CALIB_MAGIC);
        return HAL_ERROR;
    }

    /* Validate checksum */
    uint16_t calc_checksum = compute_checksum(calib);
    if (calib->checksum != calc_checksum)
    {
        printf("[ERROR] Checksum mismatch: stored=0x%04X, calculated=0x%04X\r\n",
               calib->checksum, calc_checksum);
        return HAL_ERROR;
    }

    printf("  Magic and checksum valid\r\n");
    return HAL_OK;
}

/* ==================== Print Calibration Data ==================== */
void print_calibration_data(const CalibrationData_t *calib)
{
    printf("  +--------------------------+\r\n");
    printf("  | Calibration Data         |\r\n");
    printf("  +--------------------------+\r\n");
    printf("  | Magic    : 0x%08lX    |\r\n", (uint32_t)calib->magic);
    printf("  | Offset   : %d            \r\n", calib->offset);
    printf("  | Gain     : %u (x%.2f)    \r\n", calib->gain, calib->gain / 100.0f);
    printf("  | Name     : \"%s\"         \r\n", calib->name);
    printf("  | Checksum : 0x%04X        \r\n", calib->checksum);
    printf("  +--------------------------+\r\n");
}

/* ==================== Boot Counter Init ==================== */
/**
 * Initialize boot counter if not already present.
 */
HAL_StatusTypeDef boot_counter_init(void)
{
    BootCounter_t bc;
    uint32_t num_halfwords = (sizeof(BootCounter_t) + 1) / 2;

    flash_read_data(DATA_PAGE2_ADDR, (uint16_t *)&bc, num_halfwords);

    if (bc.magic != BOOT_COUNTER_MAGIC)
    {
        /* First boot or corrupted - initialize */
        printf("[BOOT] Initializing boot counter (first boot or corrupted)...\r\n");

        bc.magic      = BOOT_COUNTER_MAGIC;
        bc.count      = 1;
        bc.last_reset = HAL_GetTick();

        HAL_StatusTypeDef status = flash_erase_page(DATA_PAGE2_ADDR);
        if (status != HAL_OK) return status;

        return flash_write_data(DATA_PAGE2_ADDR, (uint16_t *)&bc, num_halfwords);
    }

    return HAL_OK;
}

/* ==================== Boot Counter Read ==================== */
uint32_t boot_counter_read(void)
{
    BootCounter_t bc;
    uint32_t num_halfwords = (sizeof(BootCounter_t) + 1) / 2;

    flash_read_data(DATA_PAGE2_ADDR, (uint16_t *)&bc, num_halfwords);

    if (bc.magic == BOOT_COUNTER_MAGIC)
        return bc.count;
    else
        return 0;
}

/* ==================== Boot Counter Increment ==================== */
HAL_StatusTypeDef boot_counter_increment(void)
{
    BootCounter_t bc;
    uint32_t num_halfwords = (sizeof(BootCounter_t) + 1) / 2;

    flash_read_data(DATA_PAGE2_ADDR, (uint16_t *)&bc, num_halfwords);

    if (bc.magic == BOOT_COUNTER_MAGIC)
    {
        bc.count++;
    }
    else
    {
        bc.magic = BOOT_COUNTER_MAGIC;
        bc.count = 1;
    }
    bc.last_reset = HAL_GetTick();

    HAL_StatusTypeDef status = flash_erase_page(DATA_PAGE2_ADDR);
    if (status != HAL_OK) return status;

    return flash_write_data(DATA_PAGE2_ADDR, (uint16_t *)&bc, num_halfwords);
}

/* ==================== Print Boot Counter ==================== */
void print_boot_counter(void)
{
    BootCounter_t bc;
    uint32_t num_halfwords = (sizeof(BootCounter_t) + 1) / 2;

    flash_read_data(DATA_PAGE2_ADDR, (uint16_t *)&bc, num_halfwords);

    printf("  +-----------------------------+\r\n");
    printf("  | Boot Counter                |\r\n");
    printf("  +-----------------------------+\r\n");
    if (bc.magic == BOOT_COUNTER_MAGIC)
    {
        printf("  | Magic      : 0x%08lX     |\r\n", (uint32_t)bc.magic);
        printf("  | Boot Count : %lu             \r\n", (uint32_t)bc.count);
        printf("  | Last Reset : %lu ms          \r\n", (uint32_t)bc.last_reset);
    }
    else
    {
        printf("  | Status     : NOT INITIALIZED |\r\n");
    }
    printf("  +-----------------------------+\r\n");
}

/* ==================== Print Flash Page Hex Dump ==================== */
void print_flash_page_hex(uint32_t addr, uint32_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)addr;

    printf("  Flash hex dump at 0x%08lX (%lu bytes):\r\n", addr, len);
    for (uint32_t i = 0; i < len; i++)
    {
        if (i % 16 == 0)
            printf("  %08lX: ", addr + i);

        printf("%02X ", p[i]);

        if (i % 16 == 15 || i == len - 1)
            printf("\r\n");
    }
}

/* ==================== Demo: Flash Info ==================== */
void demo_flash_info(void)
{
    printf("\r\n");
    printf("==================== FLASH INFORMATION ====================\r\n");
    printf("  Flash Base Address : 0x%08lX\r\n", (uint32_t)FLASH_BASE_ADDR);
    printf("  Flash Total Size   : %d KB (%d bytes)\r\n",
           FLASH_TOTAL_SIZE / 1024, FLASH_TOTAL_SIZE);
    printf("  Page Size          : %lu bytes\r\n", (uint32_t)FLASH_PAGE_SIZE);
    printf("  Total Pages        : %d\r\n", FLASH_PAGE_COUNT);
    printf("  Programming Unit   : Halfword (16-bit)\r\n");
    printf("  ---\r\n");
    printf("  App Code Area      : 0x%08lX - 0x%08lX (pages 0-%d)\r\n",
           (uint32_t)FLASH_BASE_ADDR, (uint32_t)(APP_END_ADDR - 1), DATA_START_PAGE - 1);
    printf("  Data Storage Area  : 0x%08lX - 0x%08lX (pages %d-%d)\r\n",
           (uint32_t)DATA_START_ADDR,
           (uint32_t)(FLASH_BASE_ADDR + FLASH_TOTAL_SIZE - 1),
           DATA_START_PAGE, FLASH_PAGE_COUNT - 1);
    printf("  Calib Data Page    : 0x%08lX (page %d)\r\n",
           (uint32_t)DATA_PAGE1_ADDR, DATA_START_PAGE);
    printf("  Boot Counter Page  : 0x%08lX (page %d)\r\n",
           (uint32_t)DATA_PAGE2_ADDR, DATA_START_PAGE + 1);
    printf("============================================================\r\n");
}

/* ==================== Demo: Pattern Test ==================== */
void demo_pattern_test(void)
{
    uint16_t write_buf[TEST_PATTERN_SIZE];
    uint16_t read_buf[TEST_PATTERN_SIZE];
    uint32_t errors = 0;

    printf("\r\n==================== PATTERN WRITE/READ TEST ====================\r\n");

    /* Generate test pattern */
    printf("  Generating test pattern (%d halfwords)...\r\n", TEST_PATTERN_SIZE);
    for (int i = 0; i < TEST_PATTERN_SIZE; i++)
    {
        write_buf[i] = (uint16_t)(0xA000 + i);
    }

    /* Erase page 62 */
    printf("  Erasing page 62...\r\n");
    if (flash_erase_page(DATA_PAGE1_ADDR) != HAL_OK)
    {
        printf("  [FAIL] Erase failed!\r\n");
        return;
    }
    printf("  Erase OK\r\n");

    /* Verify page is erased (all 0xFFFF) */
    flash_read_data(DATA_PAGE1_ADDR, read_buf, TEST_PATTERN_SIZE);
    for (int i = 0; i < TEST_PATTERN_SIZE; i++)
    {
        if (read_buf[i] != 0xFFFF)
        {
            printf("  [FAIL] Erase verify failed at offset %d: 0x%04X\r\n",
                   i, read_buf[i]);
            return;
        }
    }
    printf("  Erase verified (all 0xFFFF)\r\n");

    /* Write test pattern */
    printf("  Writing test pattern...\r\n");
    if (flash_write_data(DATA_PAGE1_ADDR, write_buf, TEST_PATTERN_SIZE) != HAL_OK)
    {
        printf("  [FAIL] Write failed!\r\n");
        return;
    }
    printf("  Write OK\r\n");

    /* Read back and verify */
    printf("  Reading back and verifying...\r\n");
    memset(read_buf, 0, sizeof(read_buf));
    flash_read_data(DATA_PAGE1_ADDR, read_buf, TEST_PATTERN_SIZE);

    for (int i = 0; i < TEST_PATTERN_SIZE; i++)
    {
        if (read_buf[i] != write_buf[i])
        {
            printf("  [FAIL] Mismatch at offset %d: wrote 0x%04X, read 0x%04X\r\n",
                   i, write_buf[i], read_buf[i]);
            errors++;
        }
    }

    if (errors == 0)
    {
        printf("  [PASS] All %d halfwords verified correctly!\r\n", TEST_PATTERN_SIZE);
    }
    else
    {
        printf("  [FAIL] %lu errors detected!\r\n", errors);
    }

    /* Hex dump of written data */
    print_flash_page_hex(DATA_PAGE1_ADDR, TEST_PATTERN_SIZE * 2);
    printf("================================================================\r\n");
}

/* ==================== Demo: Calibration Data ==================== */
void demo_calibration_data(void)
{
    CalibrationData_t calib_write;
    CalibrationData_t calib_read;

    printf("\r\n==================== CALIBRATION DATA TEST ====================\r\n");

    /* Prepare calibration data */
    memset(&calib_write, 0, sizeof(CalibrationData_t));
    calib_write.magic  = CALIB_MAGIC;
    calib_write.offset = -42;
    calib_write.gain   = 150;   /* 1.50x */
    strncpy(calib_write.name, "SensorCal-v1", CALIB_NAME_MAX - 1);
    calib_write.name[CALIB_NAME_MAX - 1] = '\0';
    calib_write.checksum = compute_checksum(&calib_write);

    printf("  Data to write:\r\n");
    print_calibration_data(&calib_write);

    /* Write to flash */
    if (write_calibration_data(&calib_write) != HAL_OK)
    {
        printf("  [FAIL] Write failed!\r\n");
        return;
    }

    /* Read back */
    memset(&calib_read, 0, sizeof(CalibrationData_t));
    if (read_calibration_data(&calib_read) != HAL_OK)
    {
        printf("  [FAIL] Read/verify failed!\r\n");
        return;
    }

    printf("\r\n  Data read back:\r\n");
    print_calibration_data(&calib_read);

    /* Compare */
    if (memcmp(&calib_write, &calib_read, sizeof(CalibrationData_t)) == 0)
    {
        printf("  [PASS] Calibration data write/read verified!\r\n");
    }
    else
    {
        printf("  [FAIL] Data mismatch!\r\n");
    }

    /* Hex dump */
    print_flash_page_hex(DATA_PAGE1_ADDR, sizeof(CalibrationData_t));
    printf("================================================================\r\n");
}

/* ==================== Demo: Boot Counter ==================== */
void demo_boot_counter(void)
{
    printf("\r\n==================== BOOT COUNTER ====================\r\n");

    /* Read current boot counter */
    uint32_t current = boot_counter_read();
    printf("  Current boot count: %lu\r\n", current);

    /* Increment boot counter */
    printf("  Incrementing boot counter...\r\n");
    if (boot_counter_increment() != HAL_OK)
    {
        printf("  [FAIL] Boot counter increment failed!\r\n");
        return;
    }

    /* Read back */
    uint32_t updated = boot_counter_read();
    printf("  Updated boot count: %lu\r\n", updated);

    if (updated == current + 1 || (current == 0 && updated == 1))
    {
        printf("  [PASS] Boot counter increment verified!\r\n");
    }
    else
    {
        printf("  [WARN] Unexpected counter value (expected %lu)\r\n", current + 1);
    }

    print_boot_counter();

    /* Hex dump */
    print_flash_page_hex(DATA_PAGE2_ADDR, sizeof(BootCounter_t));
    printf("========================================================\r\n");
}

/* ==================== Error Handler ==================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

/* ==================== SysTick Handler ==================== */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* ==================== Main Function ==================== */
int main(void)
{
    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART1_Init();

    /* Startup banner */
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32F103 Internal Flash Read/Write Demo\r\n");
    printf("============================================================\r\n");
    printf("  MCU          : STM32F103C8 (Medium Density)\r\n");
    printf("  Flash Size   : %d KB\r\n", FLASH_TOTAL_SIZE / 1024);
    printf("  Page Size    : %lu bytes\r\n", (uint32_t)FLASH_PAGE_SIZE);
    printf("  UART         : PA9(TX)/PA10(RX) @ %d baud\r\n", UART_BAUDRATE);
    printf("============================================================\r\n");

    /* LED on to indicate running */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /* ---- Demo 1: Flash Information ---- */
    demo_flash_info();
    HAL_Delay(500);

    /* ---- Demo 2: Pattern Write/Read Test ---- */
    demo_pattern_test();
    HAL_Delay(500);

    /* ---- Demo 3: Calibration Data ---- */
    demo_calibration_data();
    HAL_Delay(500);

    /* ---- Demo 4: Boot Counter ---- */
    demo_boot_counter();
    HAL_Delay(500);

    /* Summary */
    printf("\r\n");
    printf("==================== ALL DEMOS COMPLETE ====================\r\n");
    printf("  Boot count: %lu\r\n", boot_counter_read());
    printf("  Uptime    : %lu ms\r\n", HAL_GetTick());
    printf("============================================================\r\n");

    /* Main loop - periodic display */
    uint32_t cycle = 0;
    while (1)
    {
        cycle++;

        printf("\r\n>>> Status Cycle #%lu | Uptime: %lu sec <<<\r\n",
               cycle, HAL_GetTick() / 1000);

        /* Re-read and display calibration data */
        {
            CalibrationData_t calib;
            if (read_calibration_data(&calib) == HAL_OK)
            {
                printf("  Calibration: offset=%d, gain=%.2f, name=\"%s\"\r\n",
                       calib.offset, calib.gain / 100.0f, calib.name);
            }
            else
            {
                printf("  Calibration: INVALID or EMPTY\r\n");
            }
        }

        /* Display boot counter */
        printf("  Boot count: %lu\r\n", boot_counter_read());

        /* Toggle LED */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        HAL_Delay(5000);
    }
}
