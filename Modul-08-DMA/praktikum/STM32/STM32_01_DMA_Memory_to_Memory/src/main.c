/**
 * ============================================================
 * STM32_01_DMA_Memory_to_Memory
 * DMA1 Channel 1 - Memory-to-Memory Transfer Benchmark
 * Target: STM32F103C8T6 Blue Pill (72MHz, DMA1 x7 channels)
 * Framework: STM32Cube HAL
 * ============================================================
 *
 * Demonstrates DMA Memory-to-Memory transfer on STM32F103.
 * Compares DMA transfer vs CPU memcpy using DWT cycle counter.
 * Shows crossover point where DMA becomes faster than CPU copy.
 *
 * Hardware: No external hardware needed (internal memory test)
 * UART1 PA9/PA10 for debug output at 115200 baud
 * LED PC13 indicates activity
 *
 * DMA1 Channel 1 configured for M2M with word-aligned transfers.
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ---- Handle Declarations ---- */
UART_HandleTypeDef huart1;
DMA_HandleTypeDef  hdma_m2m;

/* ---- Buffers (word-aligned) ---- */
static uint32_t src_buffer[BUFFER_SIZE_WORDS] __attribute__((aligned(4)));
static uint32_t dst_buffer_dma[BUFFER_SIZE_WORDS] __attribute__((aligned(4)));
static uint32_t dst_buffer_cpu[BUFFER_SIZE_WORDS] __attribute__((aligned(4)));

/* ---- Test sizes array ---- */
static const uint32_t test_sizes[NUM_TEST_SIZES] = {
    TEST_SIZE_1, TEST_SIZE_2, TEST_SIZE_3, TEST_SIZE_4, TEST_SIZE_5
};

/* ---- Timing results ---- */
typedef struct {
    uint32_t size_words;
    uint32_t dma_cycles;
    uint32_t cpu_cycles;
    float    speedup;
    uint8_t  data_ok;
} transfer_result_t;

static transfer_result_t results[NUM_TEST_SIZES];
static volatile uint8_t dma_complete_flag = 0;
static uint32_t test_iteration = 0;

/* ---- Forward Declarations ---- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);
static void MX_DMA_Init(void);
static void DWT_Init(void);
static uint32_t DWT_GetCycles(void);
static void DWT_Reset(void);
static void init_source_data(void);
static void run_dma_transfer(uint32_t num_words, uint32_t *dma_cyc);
static void run_cpu_copy(uint32_t num_words, uint32_t *cpu_cyc);
static uint8_t verify_transfer(const uint32_t *src, const uint32_t *dst, uint32_t num_words);
static void print_results(void);
static void print_separator(void);
void Error_Handler(void);

/* ---- printf retarget ---- */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_Init();
    MX_DMA_Init();
    DWT_Init();

    /* Startup banner */
    print_separator();
    printf("STM32F103 DMA Memory-to-Memory Benchmark\r\n");
    printf("System Clock: %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("DMA1 Channel 1, Word-aligned transfers\r\n");
    print_separator();

    /* Initialize source data with known pattern */
    init_source_data();

    while (1) {
        test_iteration++;
        printf("\r\n=== Test Iteration %lu ===\r\n", test_iteration);
        printf("%-12s %-12s %-12s %-10s %-8s\r\n",
               "Size(words)", "DMA(cyc)", "CPU(cyc)", "Speedup", "Verify");
        printf("--------------------------------------------------------------\r\n");

        for (int i = 0; i < NUM_TEST_SIZES; i++) {
            uint32_t num_words = test_sizes[i];
            uint32_t dma_cyc = 0, cpu_cyc = 0;

            /* Clear destination buffers */
            memset(dst_buffer_dma, 0, sizeof(dst_buffer_dma));
            memset(dst_buffer_cpu, 0, sizeof(dst_buffer_cpu));

            /* --- DMA Transfer (averaged) --- */
            uint32_t dma_total = 0;
            for (int j = 0; j < NUM_ITERATIONS; j++) {
                memset(dst_buffer_dma, 0, num_words * 4);
                run_dma_transfer(num_words, &dma_cyc);
                dma_total += dma_cyc;
            }
            dma_cyc = dma_total / NUM_ITERATIONS;

            /* --- CPU memcpy (averaged) --- */
            uint32_t cpu_total = 0;
            for (int j = 0; j < NUM_ITERATIONS; j++) {
                memset(dst_buffer_cpu, 0, num_words * 4);
                run_cpu_copy(num_words, &cpu_cyc);
                cpu_total += cpu_cyc;
            }
            cpu_cyc = cpu_total / NUM_ITERATIONS;

            /* Verify final transfer */
            uint8_t dma_ok = verify_transfer(src_buffer, dst_buffer_dma, num_words);
            uint8_t cpu_ok = verify_transfer(src_buffer, dst_buffer_cpu, num_words);

            /* Calculate speedup ratio */
            float speedup = (dma_cyc > 0) ? (float)cpu_cyc / (float)dma_cyc : 0.0f;

            /* Store result */
            results[i].size_words = num_words;
            results[i].dma_cycles = dma_cyc;
            results[i].cpu_cycles = cpu_cyc;
            results[i].speedup = speedup;
            results[i].data_ok = (dma_ok && cpu_ok);

            printf("%-12lu %-12lu %-12lu %-10.2f %s\r\n",
                   num_words, dma_cyc, cpu_cyc, speedup,
                   (dma_ok && cpu_ok) ? "PASS" : "FAIL");
        }

        /* Print analysis */
        print_results();

        LED_TOGGLE();
        HAL_Delay(PRINT_INTERVAL_MS);
    }
}

/* ============================================================
 * DMA Transfer: configure and start M2M
 * ============================================================ */
static void run_dma_transfer(uint32_t num_words, uint32_t *dma_cyc) {
    /* De-init and re-init for fresh transfer */
    HAL_DMA_DeInit(&hdma_m2m);

    hdma_m2m.Instance                 = DMA_CHANNEL;
    hdma_m2m.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    hdma_m2m.Init.PeriphInc           = DMA_PINC_ENABLE;
    hdma_m2m.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_m2m.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_m2m.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_m2m.Init.Mode                = DMA_NORMAL;
    hdma_m2m.Init.Priority            = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(&hdma_m2m) != HAL_OK) {
        Error_Handler();
    }

    DWT_Reset();
    uint32_t start = DWT_GetCycles();

    /* Start DMA transfer: src → dst */
    HAL_DMA_Start(&hdma_m2m, (uint32_t)src_buffer, (uint32_t)dst_buffer_dma, num_words);
    HAL_DMA_PollForTransfer(&hdma_m2m, HAL_DMA_FULL_TRANSFER, DMA_TIMEOUT_MS);

    uint32_t end = DWT_GetCycles();
    *dma_cyc = end - start;
}

/* ============================================================
 * CPU memcpy for comparison
 * ============================================================ */
static void run_cpu_copy(uint32_t num_words, uint32_t *cpu_cyc) {
    DWT_Reset();
    uint32_t start = DWT_GetCycles();

    memcpy(dst_buffer_cpu, src_buffer, num_words * sizeof(uint32_t));

    uint32_t end = DWT_GetCycles();
    *cpu_cyc = end - start;
}

/* ============================================================
 * Verify data integrity
 * ============================================================ */
static uint8_t verify_transfer(const uint32_t *src, const uint32_t *dst, uint32_t num_words) {
    for (uint32_t i = 0; i < num_words; i++) {
        if (src[i] != dst[i]) {
            printf("  MISMATCH at word %lu: src=0x%08lX dst=0x%08lX\r\n",
                   i, src[i], dst[i]);
            return 0;
        }
    }
    return 1;
}

/* ============================================================
 * Initialize source buffer with test pattern
 * ============================================================ */
static void init_source_data(void) {
    for (uint32_t i = 0; i < BUFFER_SIZE_WORDS; i++) {
        /* Mixed pattern: incrementing + bit pattern */
        src_buffer[i] = (i << 16) | (0xA5A5 ^ i);
    }
    printf("Source buffer initialized: %lu words (%lu bytes)\r\n",
           (uint32_t)BUFFER_SIZE_WORDS, (uint32_t)BUFFER_SIZE_BYTES);
}

/* ============================================================
 * Print analysis and summary
 * ============================================================ */
static void print_results(void) {
    printf("\r\n--- Analysis ---\r\n");

    /* Find crossover point */
    int crossover_idx = -1;
    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        if (results[i].speedup >= 1.0f) {
            crossover_idx = i;
            break;
        }
    }

    if (crossover_idx >= 0) {
        printf("DMA advantage starts at: %lu words (%lu bytes)\r\n",
               results[crossover_idx].size_words,
               results[crossover_idx].size_words * 4);
    } else {
        printf("CPU memcpy faster for all tested sizes (DMA overhead dominates)\r\n");
    }

    /* Best speedup */
    float best = 0;
    int best_idx = 0;
    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        if (results[i].speedup > best) {
            best = results[i].speedup;
            best_idx = i;
        }
    }
    printf("Best DMA speedup: %.2fx at %lu words\r\n",
           best, results[best_idx].size_words);

    /* Throughput calculation */
    printf("\r\n--- Throughput ---\r\n");
    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        uint32_t bytes = results[i].size_words * 4;
        float dma_mbps = (results[i].dma_cycles > 0) ?
            ((float)bytes * SystemCoreClock) / ((float)results[i].dma_cycles * 1000000.0f) : 0;
        float cpu_mbps = (results[i].cpu_cycles > 0) ?
            ((float)bytes * SystemCoreClock) / ((float)results[i].cpu_cycles * 1000000.0f) : 0;
        printf("  %4lu words: DMA=%.1f MB/s  CPU=%.1f MB/s\r\n",
               results[i].size_words, dma_mbps, cpu_mbps);
    }

    /* Data integrity summary */
    uint8_t all_pass = 1;
    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        if (!results[i].data_ok) { all_pass = 0; break; }
    }
    printf("\r\nData Integrity: %s\r\n", all_pass ? "ALL PASS" : "FAILURES DETECTED");
    print_separator();
}

static void print_separator(void) {
    printf("==============================================================\r\n");
}

/* ============================================================
 * DWT Cycle Counter
 * ============================================================ */
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t DWT_GetCycles(void) {
    return DWT->CYCCNT;
}

static void DWT_Reset(void) {
    DWT->CYCCNT = 0;
}

/* ============================================================
 * System Clock: HSE 8MHz → PLL x9 → 72MHz
 * ============================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* HSE + PLL configuration */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* SYSCLK=72MHz, AHB=72MHz, APB1=36MHz, APB2=72MHz */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * GPIO: LED PC13
 * ============================================================ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LED PC13 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    LED_OFF();
}

/* ============================================================
 * USART1: PA9(TX), PA10(RX), 115200 baud
 * ============================================================ */
static void MX_USART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 = TX (AF Push-Pull) */
    GPIO_InitStruct.Pin   = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

    /* PA10 = RX (Input floating) */
    GPIO_InitStruct.Pin  = DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

    huart1.Instance          = DEBUG_UART;
    huart1.Init.BaudRate     = DEBUG_UART_BAUD;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================
 * DMA1 Init: Enable clock
 * ============================================================ */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* Channel 1 will be configured per-transfer in run_dma_transfer() */
}

/* ============================================================
 * DMA1 Channel 1 IRQ Handler
 * ============================================================ */
void DMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_m2m);
}

/* ============================================================
 * DMA Transfer Complete Callback
 * ============================================================ */
void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma) {
    if (hdma->Instance == DMA_CHANNEL) {
        dma_complete_flag = 1;
    }
}

/* ============================================================
 * Error Handler
 * ============================================================ */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
        LED_TOGGLE();
        for (volatile int i = 0; i < 200000; i++);
    }
}

/* ============================================================
 * HAL SysTick hook (required by HAL)
 * ============================================================ */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/* ---- Assertion handler ---- */
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    printf("ASSERT: %s:%lu\r\n", file, line);
    Error_Handler();
}
#endif
