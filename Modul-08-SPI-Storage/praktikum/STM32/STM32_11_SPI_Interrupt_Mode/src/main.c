/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_11_SPI_Interrupt_Mode
 * Description : SPI interrupt (non-blocking) mode with blocking comparison
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA4  -> CS   (software)
 *   PA5  -> SCK  (SPI1)
 *   PA6  <- MISO (SPI1) - connect to PA7 for loopback
 *   PA7  -> MOSI (SPI1) - connect to PA6 for loopback
 *   PA9  -> USB-TTL RX  (USART1 TX)
 *   PA10 <- USB-TTL TX  (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * Features:
 *   - Blocking (polling) SPI transfer timing
 *   - Interrupt (non-blocking) SPI transfer timing
 *   - CPU utilization comparison
 *   - Full-duplex interrupt mode
 *   - Error handling callbacks
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ==================== Global Handles ==================== */
UART_HandleTypeDef huart1;
SPI_HandleTypeDef  hspi1;

/* ==================== Transfer Buffers ==================== */
static uint8_t tx_buffer[SPI_BUFFER_SIZE];
static uint8_t rx_buffer[SPI_BUFFER_SIZE];

/* ==================== Interrupt State ==================== */
static volatile uint8_t  spi_tx_complete   = 0;
static volatile uint8_t  spi_rx_complete   = 0;
static volatile uint8_t  spi_txrx_complete = 0;
static volatile uint8_t  spi_error_flag    = 0;
static volatile uint32_t spi_complete_tick  = 0;
static volatile uint32_t spi_error_code     = 0;

/* ==================== DWT Timing ==================== */
static volatile uint32_t dwt_start = 0;
static volatile uint32_t dwt_end   = 0;

/* ==================== CPU Work Counter ==================== */
static volatile uint32_t cpu_work_counter = 0;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void SPI1_Init(void);
void Error_Handler(void);

/* DWT */
static void DWT_Init(void);
static inline uint32_t DWT_GetCycles(void);
static uint32_t cycles_to_us(uint32_t cycles);

/* Test functions */
static void fill_test_pattern(uint8_t *buf, uint16_t len);
static void test_blocking_transmit(void);
static void test_interrupt_transmit(void);
static void test_interrupt_txrx(void);
static void test_cpu_utilization(void);
static void print_comparison_table(void);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ==================== System Clock: HSE 8MHz -> PLL x9 -> 72MHz ==================== */
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
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ==================== GPIO: LED PC13, CS PA4 ==================== */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);

    /* CS PA4 */
    GPIO_InitStruct.Pin   = SPI1_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
}

/* ==================== UART1: PA9/PA10, 115200 8N1 ==================== */
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
        Error_Handler();
}

/* ==================== SPI1 Init with Interrupt ==================== */
static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* SPI1 GPIO */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(SPI1_PORT, &GPIO_InitStruct);

    /* SPI1 config */
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
        Error_Handler();

    /* Enable SPI1 interrupt in NVIC */
    HAL_NVIC_SetPriority(SPI1_IRQn, SPI1_IRQ_PRIORITY, SPI1_IRQ_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

    printf("[SPI] SPI1 initialized with interrupt, clock=%lu Hz\r\n", (uint32_t)SPI_CLOCK_HZ);
}

/* ==================== SPI1 IRQ Handler ==================== */
void SPI1_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&hspi1);
}

/* ==================== SPI Callbacks ==================== */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        spi_tx_complete   = 1;
        spi_complete_tick = DWT->CYCCNT;
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET); /* CS high */
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        spi_rx_complete   = 1;
        spi_complete_tick = DWT->CYCCNT;
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        spi_txrx_complete = 1;
        spi_complete_tick = DWT->CYCCNT;
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        spi_error_flag = 1;
        spi_error_code = HAL_SPI_GetError(hspi);
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        printf("[SPI] ERROR callback! code=0x%08lX\r\n", spi_error_code);
    }
}

/* ==================== DWT Cycle Counter ==================== */
static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    printf("[DWT] Cycle counter enabled\r\n");
}

static inline uint32_t DWT_GetCycles(void)
{
    return DWT->CYCCNT;
}

static uint32_t cycles_to_us(uint32_t cycles)
{
    return (uint32_t)((uint64_t)cycles * 1000000ULL / SYSCLK_FREQ);
}

/* ==================== Fill Test Pattern ==================== */
static void fill_test_pattern(uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(i & 0xFF);
    }
}

/* ==================== Timing storage for comparison ==================== */
static uint32_t blocking_tx_us[SPI_TEST_ITERATIONS];
static uint32_t interrupt_tx_us[SPI_TEST_ITERATIONS];
static uint32_t interrupt_txrx_us[SPI_TEST_ITERATIONS];
static uint32_t cpu_ops_during_blocking[SPI_TEST_ITERATIONS];
static uint32_t cpu_ops_during_interrupt[SPI_TEST_ITERATIONS];

/* ==================== Test 1: Blocking Transmit ==================== */
static void test_blocking_transmit(void)
{
    printf("\r\n=== Test 1: Blocking (Polling) SPI Transmit ===\r\n");
    printf("Buffer: %d bytes, Iterations: %d\r\n", SPI_BUFFER_SIZE, SPI_TEST_ITERATIONS);

    fill_test_pattern(tx_buffer, SPI_BUFFER_SIZE);

    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) {
        /* Count CPU ops during blocking (should be 0, CPU is stuck) */
        cpu_work_counter = 0;

        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
        uint32_t start = DWT_GetCycles();

        HAL_SPI_Transmit(&hspi1, tx_buffer, SPI_BUFFER_SIZE, HAL_MAX_DELAY);

        uint32_t end = DWT_GetCycles();
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);

        blocking_tx_us[i] = cycles_to_us(end - start);
        cpu_ops_during_blocking[i] = cpu_work_counter;

        printf("[BLOCK] Iter %2d: %lu us (CPU ops: %lu)\r\n",
               i, blocking_tx_us[i], cpu_ops_during_blocking[i]);
    }

    /* Average */
    uint32_t sum = 0;
    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) sum += blocking_tx_us[i];
    printf("[BLOCK] Average: %lu us\r\n", sum / SPI_TEST_ITERATIONS);
}

/* ==================== Test 2: Interrupt Transmit ==================== */
static void test_interrupt_transmit(void)
{
    printf("\r\n=== Test 2: Interrupt (Non-blocking) SPI Transmit ===\r\n");
    printf("Buffer: %d bytes, Iterations: %d\r\n", SPI_BUFFER_SIZE, SPI_TEST_ITERATIONS);

    fill_test_pattern(tx_buffer, SPI_BUFFER_SIZE);

    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) {
        spi_tx_complete  = 0;
        spi_error_flag   = 0;
        cpu_work_counter = 0;

        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
        uint32_t start = DWT_GetCycles();

        /* Start non-blocking transmit */
        HAL_SPI_Transmit_IT(&hspi1, tx_buffer, SPI_BUFFER_SIZE);

        /* CPU is free! Do work while transfer happens */
        while (!spi_tx_complete && !spi_error_flag) {
            cpu_work_counter++;
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN); /* Simulated CPU work */
        }

        uint32_t end = DWT_GetCycles();
        interrupt_tx_us[i] = cycles_to_us(end - start);
        cpu_ops_during_interrupt[i] = cpu_work_counter;

        if (spi_error_flag) {
            printf("[INT] Iter %2d: ERROR! code=0x%08lX\r\n", i, spi_error_code);
        } else {
            printf("[INT] Iter %2d: %lu us (CPU ops: %lu)\r\n",
                   i, interrupt_tx_us[i], cpu_ops_during_interrupt[i]);
        }
    }

    uint32_t sum = 0;
    uint32_t ops_sum = 0;
    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) {
        sum += interrupt_tx_us[i];
        ops_sum += cpu_ops_during_interrupt[i];
    }
    printf("[INT] Average: %lu us, Avg CPU ops: %lu\r\n",
           sum / SPI_TEST_ITERATIONS, ops_sum / SPI_TEST_ITERATIONS);
}

/* ==================== Test 3: Interrupt Full-Duplex ==================== */
static void test_interrupt_txrx(void)
{
    printf("\r\n=== Test 3: Interrupt Full-Duplex (TransmitReceive) ===\r\n");
    printf("Buffer: %d bytes, Iterations: %d\r\n", SPI_BUFFER_SIZE, SPI_TEST_ITERATIONS);

    fill_test_pattern(tx_buffer, SPI_BUFFER_SIZE);

    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) {
        spi_txrx_complete = 0;
        spi_error_flag    = 0;
        cpu_work_counter  = 0;
        memset(rx_buffer, 0, SPI_BUFFER_SIZE);

        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
        uint32_t start = DWT_GetCycles();

        /* Start non-blocking full-duplex transfer */
        HAL_SPI_TransmitReceive_IT(&hspi1, tx_buffer, rx_buffer, SPI_BUFFER_SIZE);

        /* CPU work while transfer happens */
        while (!spi_txrx_complete && !spi_error_flag) {
            cpu_work_counter++;
        }

        uint32_t end = DWT_GetCycles();
        interrupt_txrx_us[i] = cycles_to_us(end - start);

        if (spi_error_flag) {
            printf("[TXRX] Iter %2d: ERROR! code=0x%08lX\r\n", i, spi_error_code);
        } else {
            /* Verify loopback data (if MOSI connected to MISO) */
            int match = (memcmp(tx_buffer, rx_buffer, SPI_BUFFER_SIZE) == 0);
            printf("[TXRX] Iter %2d: %lu us, CPU ops: %lu, data %s\r\n",
                   i, interrupt_txrx_us[i], cpu_work_counter,
                   match ? "MATCH" : "MISMATCH");
        }
    }

    uint32_t sum = 0;
    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) sum += interrupt_txrx_us[i];
    printf("[TXRX] Average: %lu us\r\n", sum / SPI_TEST_ITERATIONS);
}

/* ==================== Test 4: CPU Utilization Comparison ==================== */
static void test_cpu_utilization(void)
{
    printf("\r\n=== Test 4: CPU Utilization During Transfer ===\r\n");

    /* Measure how many ops the CPU can do in a fixed time */
    uint32_t baseline_ops = 0;
    uint32_t test_cycles  = SYSCLK_FREQ / 100; /* ~10ms worth of cycles */

    /* Baseline: no SPI, just CPU work */
    uint32_t start = DWT_GetCycles();
    while ((DWT_GetCycles() - start) < test_cycles) {
        baseline_ops++;
    }
    printf("[CPU] Baseline ops (no SPI): %lu in ~10ms\r\n", baseline_ops);

    /* Blocking SPI: CPU can't do anything */
    uint32_t blocking_ops = 0;
    start = DWT_GetCycles();
    while ((DWT_GetCycles() - start) < test_cycles) {
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi1, tx_buffer, SPI_BUFFER_SIZE, HAL_MAX_DELAY);
        HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        blocking_ops++;
    }
    uint32_t blocking_transfers = blocking_ops;
    printf("[CPU] Blocking mode : %lu transfers (CPU 100%% busy with SPI)\r\n",
           blocking_transfers);

    /* Interrupt SPI: CPU can do work between interrupts */
    uint32_t interrupt_ops = 0;
    uint32_t interrupt_transfers = 0;
    spi_tx_complete = 1; /* Ready for first transfer */

    start = DWT_GetCycles();
    while ((DWT_GetCycles() - start) < test_cycles) {
        if (spi_tx_complete) {
            spi_tx_complete = 0;
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_Transmit_IT(&hspi1, tx_buffer, SPI_BUFFER_SIZE);
            interrupt_transfers++;
        }
        interrupt_ops++; /* CPU work while SPI transfers */
    }
    /* Wait for last transfer */
    while (!spi_tx_complete);

    printf("[CPU] Interrupt mode: %lu transfers + %lu CPU ops\r\n",
           interrupt_transfers, interrupt_ops);

    /* CPU utilization */
    uint32_t cpu_avail_pct = 0;
    if (baseline_ops > 0) {
        cpu_avail_pct = (uint32_t)((uint64_t)interrupt_ops * 100ULL / baseline_ops);
    }
    printf("[CPU] CPU availability: ~%lu%% (vs 0%% in blocking mode)\r\n", cpu_avail_pct);
    printf("[CPU] Transfers: blocking=%lu, interrupt=%lu\r\n",
           blocking_transfers, interrupt_transfers);
}

/* ==================== Print Comparison Table ==================== */
static void print_comparison_table(void)
{
    /* Calculate averages */
    uint32_t avg_blocking = 0, avg_interrupt = 0, avg_txrx = 0;
    uint32_t avg_blocking_ops = 0, avg_interrupt_ops = 0;

    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) {
        avg_blocking     += blocking_tx_us[i];
        avg_interrupt    += interrupt_tx_us[i];
        avg_txrx         += interrupt_txrx_us[i];
        avg_blocking_ops += cpu_ops_during_blocking[i];
        avg_interrupt_ops += cpu_ops_during_interrupt[i];
    }
    avg_blocking     /= SPI_TEST_ITERATIONS;
    avg_interrupt    /= SPI_TEST_ITERATIONS;
    avg_txrx         /= SPI_TEST_ITERATIONS;
    avg_blocking_ops /= SPI_TEST_ITERATIONS;
    avg_interrupt_ops /= SPI_TEST_ITERATIONS;

    printf("\r\n╔══════════════════════════════════════════════════════════════╗\r\n");
    printf("║           SPI Transfer Mode Comparison Table                ║\r\n");
    printf("╠══════════════════════════════════════════════════════════════╣\r\n");
    printf("║ Mode              │ Avg Time(us) │ CPU Ops │ CPU Free      ║\r\n");
    printf("╠═══════════════════╪══════════════╪═════════╪═══════════════╣\r\n");
    printf("║ Blocking TX       │ %7lu      │ %5lu   │ No            ║\r\n",
           avg_blocking, avg_blocking_ops);
    printf("║ Interrupt TX      │ %7lu      │ %5lu   │ Yes           ║\r\n",
           avg_interrupt, avg_interrupt_ops);
    printf("║ Interrupt TX+RX   │ %7lu      │   -     │ Yes           ║\r\n",
           avg_txrx);
    printf("╠══════════════════════════════════════════════════════════════╣\r\n");

    int32_t overhead_pct = 0;
    if (avg_blocking > 0) {
        overhead_pct = (int32_t)(((int64_t)avg_interrupt - (int64_t)avg_blocking) * 100
                                / (int64_t)avg_blocking);
    }
    printf("║ Interrupt overhead vs blocking: %+ld%%                       ║\r\n",
           overhead_pct);
    printf("║ Buffer size: %d bytes, SPI clock: %lu Hz              ║\r\n",
           SPI_BUFFER_SIZE, (uint32_t)SPI_CLOCK_HZ);
    printf("╚══════════════════════════════════════════════════════════════╝\r\n");

    /* CSV for debug script */
    printf("\r\n[CSV_START]\r\n");
    printf("iter,blocking_us,interrupt_us,txrx_us,blocking_ops,interrupt_ops\r\n");
    for (int i = 0; i < SPI_TEST_ITERATIONS; i++) {
        printf("%d,%lu,%lu,%lu,%lu,%lu\r\n",
               i, blocking_tx_us[i], interrupt_tx_us[i],
               interrupt_txrx_us[i],
               cpu_ops_during_blocking[i], cpu_ops_during_interrupt[i]);
    }
    printf("[CSV_END]\r\n");
}

/* ==================== Main ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    printf("\r\n");
    printf("========================================\r\n");
    printf("  STM32_11: SPI Interrupt Mode\r\n");
    printf("  Board : Blue Pill STM32F103C8\r\n");
    printf("  Clock : 72 MHz (HSE + PLL)\r\n");
    printf("  SPI1  : %lu Hz (/%d)\r\n",
           (uint32_t)SPI_CLOCK_HZ, 72000000 / SPI_CLOCK_HZ);
    printf("  Buffer: %d bytes\r\n", SPI_BUFFER_SIZE);
    printf("========================================\r\n");

    DWT_Init();
    SPI1_Init();

    /* Fill test buffer */
    fill_test_pattern(tx_buffer, SPI_BUFFER_SIZE);

    /* Run all tests */
    test_blocking_transmit();
    HAL_Delay(200);

    test_interrupt_transmit();
    HAL_Delay(200);

    test_interrupt_txrx();
    HAL_Delay(200);

    test_cpu_utilization();
    HAL_Delay(200);

    /* Print comparison */
    print_comparison_table();

    printf("\r\n========================================\r\n");
    printf("  All SPI interrupt tests complete!\r\n");
    printf("========================================\r\n");

    /* Main loop */
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(500);
    }
}

/* ==================== Error Handler ==================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
