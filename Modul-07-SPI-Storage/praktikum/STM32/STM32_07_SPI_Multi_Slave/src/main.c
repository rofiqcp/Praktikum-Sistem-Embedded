/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_07_SPI_Multi_Slave
 * Description : Multiple SPI slaves on shared SPI1 bus with independent CS
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA5  -> Shared SCK  (SPI1 SCK to both slaves)
 *   PA7  -> Shared MOSI (SPI1 MOSI to both slaves)
 *   PA6  <- Shared MISO (SPI1 MISO from both slaves, directly or via mux)
 *   PA4  -> Slave 1 CS  (GPIO output, active low)
 *   PB0  -> Slave 2 CS  (GPIO output, active low)
 *   PA9  -> USB-TTL RX  (USART1 TX)
 *   PA10 <- USB-TTL TX  (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * Demonstrates:
 *   - Shared SPI bus with multiple chip selects
 *   - Independent slave communication
 *   - Transfer timing measurement
 *   - Error tracking per slave
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart1;

/* ==================== Per-Slave Statistics ==================== */
typedef struct {
    const char *name;
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t total_bytes;
    uint32_t last_transfer_us;
    uint32_t min_transfer_us;
    uint32_t max_transfer_us;
    uint32_t total_transfer_us;
    uint8_t  last_tx[SLAVE1_TX_SIZE];
    uint8_t  last_rx[SLAVE1_TX_SIZE];
} SlaveStats_t;

static SlaveStats_t slave1_stats;
static SlaveStats_t slave2_stats;

/* ==================== Timing ==================== */
static volatile uint32_t us_counter = 0;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void CS_GPIO_Init(void);
static void Stats_Init(void);

/* CS control functions */
static void cs1_select(void);
static void cs1_deselect(void);
static void cs2_select(void);
static void cs2_deselect(void);

/* Communication functions */
HAL_StatusTypeDef communicate_slave1(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
HAL_StatusTypeDef communicate_slave2(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/* Demo and display functions */
void demo_individual_transfers(uint32_t cycle);
void demo_back_to_back_transfers(uint32_t cycle);
void print_slave_stats(void);
void print_transfer_result(const char *name, uint8_t *tx, uint8_t *rx, uint16_t size,
                           uint32_t time_us, HAL_StatusTypeDef status);

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

    /* PC13 LED */
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

/* ==================== SPI1 Initialization ==================== */
static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_PRESCALER;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==================== CS GPIO Initialization ==================== */
static void CS_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* CS1 - PA4 */
    GPIO_InitStruct.Pin   = CS1_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CS1_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(CS1_PORT, CS1_PIN, GPIO_PIN_SET);

    /* CS2 - PB0 */
    GPIO_InitStruct.Pin   = CS2_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CS2_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(CS2_PORT, CS2_PIN, GPIO_PIN_SET);
}

/* ==================== Statistics Init ==================== */
static void Stats_Init(void)
{
    memset(&slave1_stats, 0, sizeof(SlaveStats_t));
    memset(&slave2_stats, 0, sizeof(SlaveStats_t));

    slave1_stats.name = SLAVE1_NAME;
    slave1_stats.min_transfer_us = 0xFFFFFFFF;

    slave2_stats.name = SLAVE2_NAME;
    slave2_stats.min_transfer_us = 0xFFFFFFFF;
}

/* ==================== Chip Select Control ==================== */
static void cs1_select(void)
{
    /* Ensure CS2 is deselected first */
    HAL_GPIO_WritePin(CS2_PORT, CS2_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CS1_PORT, CS1_PIN, GPIO_PIN_RESET);
}

static void cs1_deselect(void)
{
    HAL_GPIO_WritePin(CS1_PORT, CS1_PIN, GPIO_PIN_SET);
}

static void cs2_select(void)
{
    /* Ensure CS1 is deselected first */
    HAL_GPIO_WritePin(CS1_PORT, CS1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CS2_PORT, CS2_PIN, GPIO_PIN_RESET);
}

static void cs2_deselect(void)
{
    HAL_GPIO_WritePin(CS2_PORT, CS2_PIN, GPIO_PIN_SET);
}

/* ==================== DWT Cycle Counter for Microsecond Timing ==================== */
static void DWT_Init(void)
{
    /* Enable DWT cycle counter for precise timing */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t DWT_GetMicroseconds(void)
{
    return DWT->CYCCNT / (SystemCoreClock / 1000000);
}

/* ==================== Communicate with Slave 1 ==================== */
HAL_StatusTypeDef communicate_slave1(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t start_us, end_us, elapsed;

    /* Record TX data */
    memcpy(slave1_stats.last_tx, tx_data, size);

    /* Measure transfer time */
    start_us = DWT_GetMicroseconds();

    cs1_select();
    status = HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, size, SPI_TIMEOUT_MS);
    cs1_deselect();

    end_us = DWT_GetMicroseconds();
    elapsed = end_us - start_us;

    /* Update statistics */
    slave1_stats.tx_count++;
    if (status == HAL_OK)
    {
        slave1_stats.rx_count++;
        memcpy(slave1_stats.last_rx, rx_data, size);
    }
    else
    {
        slave1_stats.error_count++;
    }

    slave1_stats.total_bytes      += size;
    slave1_stats.last_transfer_us  = elapsed;
    slave1_stats.total_transfer_us += elapsed;

    if (elapsed < slave1_stats.min_transfer_us)
        slave1_stats.min_transfer_us = elapsed;
    if (elapsed > slave1_stats.max_transfer_us)
        slave1_stats.max_transfer_us = elapsed;

    return status;
}

/* ==================== Communicate with Slave 2 ==================== */
HAL_StatusTypeDef communicate_slave2(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    HAL_StatusTypeDef status;
    uint32_t start_us, end_us, elapsed;

    memcpy(slave2_stats.last_tx, tx_data, size);

    start_us = DWT_GetMicroseconds();

    cs2_select();
    status = HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, size, SPI_TIMEOUT_MS);
    cs2_deselect();

    end_us = DWT_GetMicroseconds();
    elapsed = end_us - start_us;

    slave2_stats.tx_count++;
    if (status == HAL_OK)
    {
        slave2_stats.rx_count++;
        memcpy(slave2_stats.last_rx, rx_data, size);
    }
    else
    {
        slave2_stats.error_count++;
    }

    slave2_stats.total_bytes      += size;
    slave2_stats.last_transfer_us  = elapsed;
    slave2_stats.total_transfer_us += elapsed;

    if (elapsed < slave2_stats.min_transfer_us)
        slave2_stats.min_transfer_us = elapsed;
    if (elapsed > slave2_stats.max_transfer_us)
        slave2_stats.max_transfer_us = elapsed;

    return status;
}

/* ==================== Print Transfer Result ==================== */
void print_transfer_result(const char *name, uint8_t *tx, uint8_t *rx,
                           uint16_t size, uint32_t time_us,
                           HAL_StatusTypeDef status)
{
    printf("  %s: ", name);
    printf("TX=[");
    for (uint16_t i = 0; i < size; i++)
    {
        printf("0x%02X", tx[i]);
        if (i < size - 1) printf(",");
    }
    printf("] RX=[");
    for (uint16_t i = 0; i < size; i++)
    {
        printf("0x%02X", rx[i]);
        if (i < size - 1) printf(",");
    }
    printf("] %luus %s\r\n", time_us,
           (status == HAL_OK) ? "OK" : "ERR");
}

/* ==================== Demo: Individual Transfers ==================== */
void demo_individual_transfers(uint32_t cycle)
{
    uint8_t tx1[SLAVE1_TX_SIZE];
    uint8_t rx1[SLAVE1_TX_SIZE];
    uint8_t tx2[SLAVE2_TX_SIZE];
    uint8_t rx2[SLAVE2_TX_SIZE];
    HAL_StatusTypeDef s1, s2;

    /* Prepare different data for each slave */
    tx1[0] = SLAVE1_CMD_READ;
    tx1[1] = (uint8_t)(cycle & 0xFF);
    tx1[2] = (uint8_t)((cycle >> 8) & 0xFF);
    tx1[3] = 0xAA;  /* Pattern byte */

    tx2[0] = SLAVE2_CMD_STATUS;
    tx2[1] = (uint8_t)(~cycle & 0xFF);
    tx2[2] = (uint8_t)((~cycle >> 8) & 0xFF);
    tx2[3] = 0x55;  /* Pattern byte */

    printf("\r\n--- Individual Transfers (Cycle %lu) ---\r\n", cycle);

    /* Transfer to Slave 1 */
    s1 = communicate_slave1(tx1, rx1, SLAVE1_TX_SIZE);
    print_transfer_result(SLAVE1_NAME, tx1, rx1, SLAVE1_TX_SIZE,
                          slave1_stats.last_transfer_us, s1);

    /* Small gap between slaves */
    __NOP(); __NOP(); __NOP(); __NOP();

    /* Transfer to Slave 2 */
    s2 = communicate_slave2(tx2, rx2, SLAVE2_TX_SIZE);
    print_transfer_result(SLAVE2_NAME, tx2, rx2, SLAVE2_TX_SIZE,
                          slave2_stats.last_transfer_us, s2);
}

/* ==================== Demo: Back-to-Back Transfers ==================== */
void demo_back_to_back_transfers(uint32_t cycle)
{
    uint8_t tx1[SLAVE1_TX_SIZE] = {SLAVE1_CMD_WRITE, 0x11, 0x22, 0x33};
    uint8_t rx1[SLAVE1_TX_SIZE];
    uint8_t tx2[SLAVE2_TX_SIZE] = {SLAVE2_CMD_WRITE, 0x44, 0x55, 0x66};
    uint8_t rx2[SLAVE2_TX_SIZE];
    HAL_StatusTypeDef s1, s2;
    uint32_t total_start, total_end;

    printf("\r\n--- Back-to-Back Transfers (Cycle %lu) ---\r\n", cycle);

    total_start = DWT_GetMicroseconds();

    /* Rapid sequence: Slave1 -> Slave2 -> Slave1 -> Slave2 */
    s1 = communicate_slave1(tx1, rx1, SLAVE1_TX_SIZE);
    s2 = communicate_slave2(tx2, rx2, SLAVE2_TX_SIZE);

    /* Second round with modified data */
    tx1[1] = 0xAA; tx1[2] = 0xBB;
    s1 = communicate_slave1(tx1, rx1, SLAVE1_TX_SIZE);

    tx2[1] = 0xCC; tx2[2] = 0xDD;
    s2 = communicate_slave2(tx2, rx2, SLAVE2_TX_SIZE);

    total_end = DWT_GetMicroseconds();

    printf("  4 transfers completed in %lu us\r\n", total_end - total_start);
    printf("  S1 last: TX=[0x%02X,0x%02X,0x%02X,0x%02X] RX=[0x%02X,0x%02X,0x%02X,0x%02X] %s\r\n",
           tx1[0], tx1[1], tx1[2], tx1[3],
           rx1[0], rx1[1], rx1[2], rx1[3],
           (s1 == HAL_OK) ? "OK" : "ERR");
    printf("  S2 last: TX=[0x%02X,0x%02X,0x%02X,0x%02X] RX=[0x%02X,0x%02X,0x%02X,0x%02X] %s\r\n",
           tx2[0], tx2[1], tx2[2], tx2[3],
           rx2[0], rx2[1], rx2[2], rx2[3],
           (s2 == HAL_OK) ? "OK" : "ERR");
}

/* ==================== Print Slave Statistics ==================== */
void print_slave_stats(void)
{
    uint32_t avg1 = 0, avg2 = 0;

    if (slave1_stats.tx_count > 0)
        avg1 = slave1_stats.total_transfer_us / slave1_stats.tx_count;
    if (slave2_stats.tx_count > 0)
        avg2 = slave2_stats.total_transfer_us / slave2_stats.tx_count;

    printf("\r\n");
    printf("======================== SPI BUS STATISTICS ========================\r\n");
    printf("              | %-18s | %-18s\r\n", slave1_stats.name, slave2_stats.name);
    printf("--------------+--------------------+--------------------\r\n");
    printf(" TX Count     | %18lu | %18lu\r\n", slave1_stats.tx_count, slave2_stats.tx_count);
    printf(" RX Count     | %18lu | %18lu\r\n", slave1_stats.rx_count, slave2_stats.rx_count);
    printf(" Errors       | %18lu | %18lu\r\n", slave1_stats.error_count, slave2_stats.error_count);
    printf(" Total Bytes  | %18lu | %18lu\r\n", slave1_stats.total_bytes, slave2_stats.total_bytes);
    printf(" Last Time    | %15lu us | %15lu us\r\n",
           slave1_stats.last_transfer_us, slave2_stats.last_transfer_us);
    printf(" Min Time     | %15lu us | %15lu us\r\n",
           (slave1_stats.min_transfer_us == 0xFFFFFFFF) ? 0 : slave1_stats.min_transfer_us,
           (slave2_stats.min_transfer_us == 0xFFFFFFFF) ? 0 : slave2_stats.min_transfer_us);
    printf(" Max Time     | %15lu us | %15lu us\r\n",
           slave1_stats.max_transfer_us, slave2_stats.max_transfer_us);
    printf(" Avg Time     | %15lu us | %15lu us\r\n", avg1, avg2);
    printf("====================================================================\r\n");

    /* Error rate */
    float err1 = (slave1_stats.tx_count > 0) ?
                 (float)slave1_stats.error_count / slave1_stats.tx_count * 100.0f : 0.0f;
    float err2 = (slave2_stats.tx_count > 0) ?
                 (float)slave2_stats.error_count / slave2_stats.tx_count * 100.0f : 0.0f;
    printf(" Error Rate   | %14.2f %%  | %14.2f %%\r\n", err1, err2);
    printf("====================================================================\r\n");
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
    uint32_t cycle_count = 0;

    /* Initialize HAL */
    HAL_Init();

    /* Configure system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    GPIO_Init();
    UART1_Init();
    SPI1_Init();
    CS_GPIO_Init();
    Stats_Init();

    /* Enable DWT cycle counter for microsecond timing */
    DWT_Init();

    /* Startup banner */
    printf("\r\n");
    printf("============================================================\r\n");
    printf("  STM32F103 SPI Multi-Slave Communication Demo\r\n");
    printf("============================================================\r\n");
    printf("  Shared SPI1 Bus:\r\n");
    printf("    SCK  : PA5\r\n");
    printf("    MOSI : PA7\r\n");
    printf("    MISO : PA6\r\n");
    printf("  Slave 1: CS = PA4 (%s)\r\n", SLAVE1_NAME);
    printf("  Slave 2: CS = PB0 (%s)\r\n", SLAVE2_NAME);
    printf("  SPI Speed: 72MHz/16 = 4.5MHz\r\n");
    printf("  Transfer Size: %d bytes\r\n", SLAVE1_TX_SIZE);
    printf("  Interval: %d ms\r\n", DEMO_INTERVAL_MS);
    printf("============================================================\r\n");

    /* Verify both CS lines are HIGH (deselected) */
    printf("\r\n[INIT] CS1 (PA4) = %s\r\n",
           (HAL_GPIO_ReadPin(CS1_PORT, CS1_PIN) == GPIO_PIN_SET) ? "HIGH (OK)" : "LOW (ERR)");
    printf("[INIT] CS2 (PB0) = %s\r\n",
           (HAL_GPIO_ReadPin(CS2_PORT, CS2_PIN) == GPIO_PIN_SET) ? "HIGH (OK)" : "LOW (ERR)");

    /* Quick test transfers */
    printf("\r\n[TEST] Quick connectivity test...\r\n");
    {
        uint8_t test_tx[4] = {0xFF, 0x00, 0xFF, 0x00};
        uint8_t test_rx[4] = {0};
        HAL_StatusTypeDef s;

        s = communicate_slave1(test_tx, test_rx, 4);
        printf("  Slave1 test: %s (RX: 0x%02X 0x%02X 0x%02X 0x%02X)\r\n",
               (s == HAL_OK) ? "OK" : "FAIL",
               test_rx[0], test_rx[1], test_rx[2], test_rx[3]);

        s = communicate_slave2(test_tx, test_rx, 4);
        printf("  Slave2 test: %s (RX: 0x%02X 0x%02X 0x%02X 0x%02X)\r\n",
               (s == HAL_OK) ? "OK" : "FAIL",
               test_rx[0], test_rx[1], test_rx[2], test_rx[3]);
    }

    printf("\r\n[START] Multi-slave demo running...\r\n");

    /* Toggle LED to indicate running */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /* Main loop */
    while (1)
    {
        cycle_count++;

        printf("\r\n======== Cycle #%lu | Uptime: %lu sec ========\r\n",
               cycle_count, HAL_GetTick() / 1000);

        /* Alternate between individual and back-to-back demos */
        if (cycle_count % 2 == 1)
        {
            demo_individual_transfers(cycle_count);
        }
        else
        {
            demo_back_to_back_transfers(cycle_count);
        }

        /* Print full statistics every 5th cycle */
        if (cycle_count % 5 == 0)
        {
            print_slave_stats();
        }

        /* Toggle LED */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        HAL_Delay(DEMO_INTERVAL_MS);
    }
}
