/**
 * ============================================================================
 * File        : main.c
 * Program     : STM32_10_SPI_Speed_Benchmark
 * Description : SPI speed and throughput benchmark across prescalers & buffer sizes
 *
 * Board       : STM32F103C8 (Blue Pill)
 * Framework   : STM32Cube HAL
 *
 * Wiring:
 *   PA4  -> CS   (software, directly to slave or loopback)
 *   PA5  -> SCK  (SPI1)
 *   PA6  <- MISO (SPI1)
 *   PA7  -> MOSI (SPI1)
 *   PA9  -> USB-TTL RX  (USART1 TX)
 *   PA10 <- USB-TTL TX  (USART1 RX)
 *   PC13 -> Onboard LED
 *
 * Notes:
 *   - Uses DWT cycle counter for precise microsecond timing
 *   - SPI1 on APB2 (72 MHz), prescalers /2../256
 *   - Tests HAL_SPI_Transmit and HAL_SPI_TransmitReceive
 *   - MOSI looped to MISO for self-test (connect PA6 to PA7)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ==================== Global Handles ==================== */
UART_HandleTypeDef huart1;
SPI_HandleTypeDef  hspi1;

/* ==================== Buffers ==================== */
static uint8_t tx_buffer[MAX_BUFFER_SIZE];
static uint8_t rx_buffer[MAX_BUFFER_SIZE];

/* ==================== Benchmark Results ==================== */
typedef struct {
    uint32_t prescaler_div;
    uint32_t spi_clock_hz;
    uint16_t buffer_size;
    uint32_t total_cycles;
    uint32_t total_time_us;
    uint32_t throughput_bps;    /* bytes per second */
    uint32_t efficiency_pct;    /* actual/theoretical x 100 */
} benchmark_result_t;

static benchmark_result_t results[NUM_PRESCALERS * NUM_BUFFER_SIZES];
static uint32_t result_count = 0;

/* Best result tracking */
static uint32_t best_throughput = 0;
static uint32_t best_prescaler  = 0;
static uint16_t best_bufsize    = 0;

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void UART1_Init(void);
static void SPI1_Init(uint32_t prescaler);
static void SPI1_DeInit(void);
void Error_Handler(void);

/* DWT timing */
static void DWT_Init(void);
static inline uint32_t DWT_GetCycles(void);
static uint32_t cycles_to_us(uint32_t cycles);

/* Benchmark functions */
static void fill_test_pattern(uint8_t *buf, uint16_t len);
static void run_prescaler_benchmark(int prescaler_idx);
static void run_tx_vs_txrx_comparison(void);
static void print_summary(void);

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

    /* CS PA4 - software managed */
    GPIO_InitStruct.Pin   = SPI1_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET); /* CS high (idle) */
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

/* ==================== SPI1 Init with Variable Prescaler ==================== */
static void SPI1_Init(uint32_t prescaler)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* SPI1 GPIO: SCK=PA5, MISO=PA6, MOSI=PA7 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* SCK + MOSI: AF push-pull */
    GPIO_InitStruct.Pin   = SPI1_SCK_PIN | SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_PORT, &GPIO_InitStruct);

    /* MISO: input floating */
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
    hspi1.Init.BaudRatePrescaler = prescaler;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial     = 10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
        Error_Handler();
}

/* ==================== SPI1 DeInit ==================== */
static void SPI1_DeInit(void)
{
    HAL_SPI_DeInit(&hspi1);
    __HAL_RCC_SPI1_CLK_DISABLE();
}

/* ==================== DWT Cycle Counter Init ==================== */
static void DWT_Init(void)
{
    /* Enable TRC (trace) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Reset cycle counter */
    DWT->CYCCNT = 0;
    /* Enable cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    printf("[DWT] Cycle counter enabled, SYSCLK=%lu Hz\r\n", (uint32_t)SYSCLK_FREQ);
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

/* ==================== Run Benchmark for One Prescaler ==================== */
static void run_prescaler_benchmark(int prescaler_idx)
{
    uint32_t prescaler_val = spi_prescalers[prescaler_idx].hal_prescaler;
    uint32_t divider       = spi_prescalers[prescaler_idx].divider;
    uint32_t spi_clock     = spi_prescalers[prescaler_idx].spi_clock_hz;

    printf("\r\n╔══════════════════════════════════════════════════════════════╗\r\n");
    printf("║ Prescaler: /%lu  SPI Clock: %s                      ║\r\n",
           divider, spi_prescalers[prescaler_idx].label);
    printf("╠══════════════════════════════════════════════════════════════╣\r\n");
    printf("║ BufSize │ Time(us)  │ Throughput(KB/s) │ Efficiency(%%)    ║\r\n");
    printf("╠═════════╪═══════════╪══════════════════╪══════════════════╣\r\n");

    /* Reinit SPI with this prescaler */
    SPI1_DeInit();
    SPI1_Init(prescaler_val);

    for (int bi = 0; bi < NUM_BUFFER_SIZES; bi++) {
        uint16_t bufsize = buffer_sizes[bi];
        fill_test_pattern(tx_buffer, bufsize);

        /* Warmup */
        for (int w = 0; w < WARMUP_ITERATIONS; w++) {
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, tx_buffer, bufsize, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        }

        /* Measure */
        uint32_t start_cycles = DWT_GetCycles();

        for (int it = 0; it < NUM_ITERATIONS; it++) {
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, tx_buffer, bufsize, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        }

        uint32_t end_cycles   = DWT_GetCycles();
        uint32_t total_cycles = end_cycles - start_cycles;
        uint32_t total_us     = cycles_to_us(total_cycles);

        /* Calculate throughput */
        uint32_t total_bytes = (uint32_t)bufsize * NUM_ITERATIONS;
        uint32_t throughput  = 0;
        if (total_us > 0) {
            throughput = (uint32_t)((uint64_t)total_bytes * 1000000ULL / total_us);
        }

        /* Calculate theoretical max: SPI_clock / 8 bits = bytes/sec */
        uint32_t theoretical_bps = spi_clock / 8;
        uint32_t efficiency = 0;
        if (theoretical_bps > 0) {
            efficiency = (uint32_t)((uint64_t)throughput * 100ULL / theoretical_bps);
        }

        /* Store result */
        benchmark_result_t *r = &results[result_count++];
        r->prescaler_div  = divider;
        r->spi_clock_hz   = spi_clock;
        r->buffer_size    = bufsize;
        r->total_cycles   = total_cycles;
        r->total_time_us  = total_us;
        r->throughput_bps = throughput;
        r->efficiency_pct = efficiency;

        /* Track best */
        if (throughput > best_throughput) {
            best_throughput = throughput;
            best_prescaler  = divider;
            best_bufsize    = bufsize;
        }

        printf("║ %3u B   │ %7lu   │ %8lu.%01lu      │ %3lu%%             ║\r\n",
               bufsize, total_us,
               throughput / 1024, (throughput % 1024) * 10 / 1024,
               efficiency);

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    }

    printf("╚══════════════════════════════════════════════════════════════╝\r\n");
}

/* ==================== Compare Transmit vs TransmitReceive ==================== */
static void run_tx_vs_txrx_comparison(void)
{
    printf("\r\n╔══════════════════════════════════════════════════════════════╗\r\n");
    printf("║  HAL_SPI_Transmit vs HAL_SPI_TransmitReceive Comparison     ║\r\n");
    printf("╠══════════════════════════════════════════════════════════════╣\r\n");
    printf("║ BufSize │ TX Time(us) │ TXRX Time(us) │ Overhead(%%)       ║\r\n");
    printf("╠═════════╪═════════════╪═══════════════╪═══════════════════╣\r\n");

    /* Use prescaler /16 (4.5 MHz) for comparison */
    SPI1_DeInit();
    SPI1_Init(SPI_BAUDRATEPRESCALER_16);

    for (int bi = 0; bi < NUM_BUFFER_SIZES; bi++) {
        uint16_t bufsize = buffer_sizes[bi];
        fill_test_pattern(tx_buffer, bufsize);
        memset(rx_buffer, 0, bufsize);

        /* Warmup */
        for (int w = 0; w < WARMUP_ITERATIONS; w++) {
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, tx_buffer, bufsize, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        }

        /* Measure Transmit */
        uint32_t start = DWT_GetCycles();
        for (int it = 0; it < NUM_ITERATIONS; it++) {
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, tx_buffer, bufsize, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        }
        uint32_t tx_us = cycles_to_us(DWT_GetCycles() - start);

        /* Warmup TransmitReceive */
        for (int w = 0; w < WARMUP_ITERATIONS; w++) {
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, bufsize, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        }

        /* Measure TransmitReceive */
        start = DWT_GetCycles();
        for (int it = 0; it < NUM_ITERATIONS; it++) {
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, tx_buffer, rx_buffer, bufsize, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(SPI1_PORT, SPI1_CS_PIN, GPIO_PIN_SET);
        }
        uint32_t txrx_us = cycles_to_us(DWT_GetCycles() - start);

        /* Overhead */
        int32_t overhead = 0;
        if (tx_us > 0) {
            overhead = (int32_t)(((int64_t)txrx_us - (int64_t)tx_us) * 100 / (int64_t)tx_us);
        }

        printf("║ %3u B   │ %7lu     │ %7lu       │ %+4ld%%             ║\r\n",
               bufsize, tx_us, txrx_us, overhead);
    }

    printf("╚══════════════════════════════════════════════════════════════╝\r\n");
}

/* ==================== Print Summary ==================== */
static void print_summary(void)
{
    printf("\r\n╔══════════════════════════════════════════════════════════════╗\r\n");
    printf("║                   BENCHMARK SUMMARY                         ║\r\n");
    printf("╠══════════════════════════════════════════════════════════════╣\r\n");
    printf("║ Total tests run     : %lu\r\n", result_count);
    printf("║ Best throughput     : %lu bytes/sec (%lu KB/s)\r\n",
           best_throughput, best_throughput / 1024);
    printf("║ Best prescaler      : /%lu\r\n", best_prescaler);
    printf("║ Best buffer size    : %u bytes\r\n", best_bufsize);
    printf("╠══════════════════════════════════════════════════════════════╣\r\n");
    printf("║ Prescaler │ Max Throughput (KB/s) │ Best Buf Size           ║\r\n");
    printf("╠═══════════╪═══════════════════════╪═════════════════════════╣\r\n");

    /* Find max throughput per prescaler */
    for (int pi = 0; pi < NUM_PRESCALERS; pi++) {
        uint32_t max_tp = 0;
        uint16_t max_bs = 0;
        for (uint32_t ri = 0; ri < result_count; ri++) {
            if (results[ri].prescaler_div == spi_prescalers[pi].divider) {
                if (results[ri].throughput_bps > max_tp) {
                    max_tp = results[ri].throughput_bps;
                    max_bs = results[ri].buffer_size;
                }
            }
        }
        printf("║ /%3lu      │ %8lu.%01lu           │ %3u B                   ║\r\n",
               spi_prescalers[pi].divider,
               max_tp / 1024, (max_tp % 1024) * 10 / 1024,
               max_bs);
    }
    printf("╚══════════════════════════════════════════════════════════════╝\r\n");

    /* CSV output for debug script */
    printf("\r\n[CSV_START]\r\n");
    printf("prescaler_div,spi_clock_hz,buffer_size,time_us,throughput_bps,efficiency_pct\r\n");
    for (uint32_t ri = 0; ri < result_count; ri++) {
        printf("%lu,%lu,%u,%lu,%lu,%lu\r\n",
               results[ri].prescaler_div,
               results[ri].spi_clock_hz,
               results[ri].buffer_size,
               results[ri].total_time_us,
               results[ri].throughput_bps,
               results[ri].efficiency_pct);
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
    printf("  STM32_10: SPI Speed Benchmark\r\n");
    printf("  Board : Blue Pill STM32F103C8\r\n");
    printf("  Clock : 72 MHz (HSE + PLL)\r\n");
    printf("  SPI1  : APB2 = 72 MHz\r\n");
    printf("========================================\r\n");

    /* Init DWT cycle counter */
    DWT_Init();

    /* Fill default test pattern */
    fill_test_pattern(tx_buffer, MAX_BUFFER_SIZE);

    printf("\r\n[BENCH] Starting SPI benchmark...\r\n");
    printf("[BENCH] Prescalers: %d, Buffer sizes: %d, Iterations: %d\r\n",
           NUM_PRESCALERS, NUM_BUFFER_SIZES, NUM_ITERATIONS);
    printf("[BENCH] Total tests: %d\r\n\r\n",
           NUM_PRESCALERS * NUM_BUFFER_SIZES);

    /* Run benchmark for each prescaler */
    for (int pi = 0; pi < NUM_PRESCALERS; pi++) {
        run_prescaler_benchmark(pi);
        HAL_Delay(100);
    }

    /* TX vs TXRX comparison */
    run_tx_vs_txrx_comparison();

    /* Summary */
    print_summary();

    printf("\r\n[BENCH] Benchmark complete!\r\n");

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
