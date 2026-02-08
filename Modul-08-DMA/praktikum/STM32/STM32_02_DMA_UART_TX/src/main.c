/**
 * ============================================================
 * STM32_02_DMA_UART_TX
 * UART1 Transmit: Blocking vs DMA vs Interrupt Comparison
 * DMA1 Channel 4 = USART1_TX
 * Target: STM32F103C8T6 Blue Pill (72MHz)
 * Framework: STM32Cube HAL
 * ============================================================
 *
 * Compares three UART TX methods:
 *   1. HAL_UART_Transmit()      - Blocking (CPU fully occupied)
 *   2. HAL_UART_Transmit_DMA()  - DMA-backed (CPU free during TX)
 *   3. HAL_UART_Transmit_IT()   - Interrupt-based (CPU partially busy)
 *
 * During DMA transfer, counts LED toggles to demonstrate CPU freedom.
 * Uses DWT cycle counter for precise timing measurements.
 *
 * Hardware: UART1 PA9/PA10, LED PC13
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ---- Handle Declarations ---- */
UART_HandleTypeDef huart1;
DMA_HandleTypeDef  hdma_uart1_tx;

/* ---- TX Buffer ---- */
static uint8_t tx_buffer[TX_BUFFER_SIZE];

/* ---- Flags ---- */
static volatile uint8_t tx_dma_complete  = 0;
static volatile uint8_t tx_it_complete   = 0;

/* ---- Test sizes ---- */
static const uint16_t test_sizes[NUM_TEST_SIZES] = {
    TEST_SIZE_1, TEST_SIZE_2, TEST_SIZE_3, TEST_SIZE_4
};

/* ---- Timing results ---- */
typedef struct {
    uint16_t size;
    uint32_t blocking_cycles;
    uint32_t dma_cycles;
    uint32_t it_cycles;
    uint32_t led_toggles_blocking;
    uint32_t led_toggles_dma;
    uint32_t led_toggles_it;
} uart_tx_result_t;

static uart_tx_result_t results[NUM_TEST_SIZES];
static uint32_t test_iteration = 0;

/* ---- Forward Declarations ---- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);
static void MX_DMA_Init(void);
static void DWT_Init(void);
static void init_tx_data(void);
static uint32_t count_led_toggles_for_ms(uint32_t ms);
static void run_blocking_test(uint16_t size, uart_tx_result_t *result);
static void run_dma_test(uint16_t size, uart_tx_result_t *result);
static void run_it_test(uint16_t size, uart_tx_result_t *result);
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
    MX_DMA_Init();
    MX_USART1_Init();
    DWT_Init();

    print_separator();
    printf("STM32F103 DMA UART TX Comparison\r\n");
    printf("System Clock: %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("UART Baud: %d, DMA1 Channel 4\r\n", DEBUG_UART_BAUD);
    print_separator();

    /* Fill TX buffer with printable ASCII pattern */
    init_tx_data();

    while (1) {
        test_iteration++;
        printf("\r\n=== Test Iteration %lu ===\r\n", test_iteration);

        for (int i = 0; i < NUM_TEST_SIZES; i++) {
            uint16_t size = test_sizes[i];
            memset(&results[i], 0, sizeof(uart_tx_result_t));
            results[i].size = size;

            printf("\r\n--- Testing %u bytes ---\r\n", size);

            /* Test 1: Blocking */
            run_blocking_test(size, &results[i]);
            HAL_Delay(50);

            /* Test 2: DMA */
            run_dma_test(size, &results[i]);
            HAL_Delay(50);

            /* Test 3: Interrupt */
            run_it_test(size, &results[i]);
            HAL_Delay(50);
        }

        print_results();

        LED_TOGGLE();
        HAL_Delay(PRINT_INTERVAL_MS);
    }
}

/* ============================================================
 * Initialize TX data with printable ASCII pattern
 * ============================================================ */
static void init_tx_data(void) {
    for (int i = 0; i < TX_BUFFER_SIZE; i++) {
        tx_buffer[i] = '0' + (i % 10);
    }
    printf("TX buffer initialized: %d bytes\r\n", TX_BUFFER_SIZE);
}

/* ============================================================
 * Test 1: Blocking UART Transmit
 * ============================================================ */
static void run_blocking_test(uint16_t size, uart_tx_result_t *result) {
    volatile uint32_t toggles = 0;

    DWT->CYCCNT = 0;
    uint32_t start = DWT->CYCCNT;

    /* Blocking transmit — CPU is stuck here */
    HAL_UART_Transmit(&huart1, tx_buffer, size, HAL_MAX_DELAY);

    uint32_t end = DWT->CYCCNT;
    result->blocking_cycles = end - start;

    /* CPU was 100% occupied, so try toggling after (simulated 0 toggles during) */
    result->led_toggles_blocking = 0;

    printf("  Blocking: %lu cycles\r\n", result->blocking_cycles);
}

/* ============================================================
 * Test 2: DMA UART Transmit
 * ============================================================ */
static void run_dma_test(uint16_t size, uart_tx_result_t *result) {
    tx_dma_complete = 0;
    volatile uint32_t toggles = 0;

    DWT->CYCCNT = 0;
    uint32_t start = DWT->CYCCNT;

    /* Start DMA transmit — CPU is FREE after this call */
    if (HAL_UART_Transmit_DMA(&huart1, tx_buffer, size) != HAL_OK) {
        printf("  DMA TX start failed!\r\n");
        return;
    }

    /* Count LED toggles while DMA is transferring (shows CPU freedom) */
    while (!tx_dma_complete) {
        LED_TOGGLE();
        toggles++;
        /* Small delay to make toggles visible */
        for (volatile int d = 0; d < 10; d++);
    }

    uint32_t end = DWT->CYCCNT;
    result->dma_cycles = end - start;
    result->led_toggles_dma = toggles;

    printf("  DMA:      %lu cycles, %lu LED toggles (CPU free!)\r\n",
           result->dma_cycles, toggles);
}

/* ============================================================
 * Test 3: Interrupt-based UART Transmit
 * ============================================================ */
static void run_it_test(uint16_t size, uart_tx_result_t *result) {
    tx_it_complete = 0;
    volatile uint32_t toggles = 0;

    DWT->CYCCNT = 0;
    uint32_t start = DWT->CYCCNT;

    /* Start interrupt-based transmit */
    if (HAL_UART_Transmit_IT(&huart1, tx_buffer, size) != HAL_OK) {
        printf("  IT TX start failed!\r\n");
        return;
    }

    /* Count LED toggles while IT is transferring */
    while (!tx_it_complete) {
        LED_TOGGLE();
        toggles++;
        for (volatile int d = 0; d < 10; d++);
    }

    uint32_t end = DWT->CYCCNT;
    result->it_cycles = end - start;
    result->led_toggles_it = toggles;

    printf("  IT:       %lu cycles, %lu LED toggles\r\n",
           result->it_cycles, toggles);
}

/* ============================================================
 * Print comparison results
 * ============================================================ */
static void print_results(void) {
    printf("\r\n");
    print_separator();
    printf("UART TX Method Comparison\r\n");
    print_separator();
    printf("%-6s | %-14s %-14s %-14s | %-8s %-8s\r\n",
           "Bytes", "Blocking(cyc)", "DMA(cyc)", "IT(cyc)", "DMA_Tgl", "IT_Tgl");
    printf("-------+---------------------------------------------+------------------\r\n");

    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        uart_tx_result_t *r = &results[i];
        printf("%-6u | %-14lu %-14lu %-14lu | %-8lu %-8lu\r\n",
               r->size, r->blocking_cycles, r->dma_cycles, r->it_cycles,
               r->led_toggles_dma, r->led_toggles_it);
    }

    printf("\r\n--- CPU Utilization Analysis ---\r\n");
    for (int i = 0; i < NUM_TEST_SIZES; i++) {
        uart_tx_result_t *r = &results[i];

        /* Estimate CPU utilization: blocking=100%, DMA/IT based on toggle ratios */
        float dma_free_pct = 0, it_free_pct = 0;
        if (r->dma_cycles > 0 && r->blocking_cycles > 0) {
            /* DMA CPU time is just setup, rest is free */
            float dma_overhead_ratio = (r->led_toggles_dma > 0) ? 
                (1.0f - (1.0f / (float)(r->led_toggles_dma + 1))) * 100.0f : 0.0f;
            dma_free_pct = dma_overhead_ratio;
        }
        if (r->it_cycles > 0 && r->blocking_cycles > 0) {
            float it_overhead_ratio = (r->led_toggles_it > 0) ?
                (1.0f - (1.0f / (float)(r->led_toggles_it + 1))) * 100.0f : 0.0f;
            it_free_pct = it_overhead_ratio;
        }

        printf("  %3u bytes: Blocking=100%% CPU | DMA=~%.0f%% free | IT=~%.0f%% free\r\n",
               r->size, dma_free_pct, it_free_pct);
    }

    printf("\r\nConclusion:\r\n");
    printf("  - Blocking: simplest but CPU cannot do anything else\r\n");
    printf("  - DMA: CPU completely free during transfer (best for large data)\r\n");
    printf("  - IT: CPU interrupted per byte (overhead scales with size)\r\n");
    print_separator();
}

static void print_separator(void) {
    printf("==============================================================\r\n");
}

/* ============================================================
 * DWT Cycle Counter Init
 * ============================================================ */
static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* ============================================================
 * HAL UART TX Complete Callbacks
 * ============================================================ */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        /* Check if it was DMA or IT based on the handle state */
        if (huart->gState == HAL_UART_STATE_READY) {
            /* Both DMA and IT end up here */
            tx_dma_complete = 1;
            tx_it_complete  = 1;
        }
    }
}

/* ============================================================
 * DMA1 Channel 4 IRQ Handler (USART1_TX)
 * ============================================================ */
void DMA1_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_uart1_tx);
}

/* ============================================================
 * USART1 IRQ Handler (for IT mode)
 * ============================================================ */
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

/* ============================================================
 * System Clock: HSE 8MHz → PLL x9 → 72MHz
 * ============================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

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
 * GPIO Init: LED PC13
 * ============================================================ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    LED_OFF();
}

/* ============================================================
 * DMA1 Init: Enable clock, configure Channel 4 for USART1_TX
 * ============================================================ */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Configure DMA1 Channel 4 for USART1 TX */
    hdma_uart1_tx.Instance                 = UART_TX_DMA_CHANNEL;
    hdma_uart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_uart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_uart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_uart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_uart1_tx.Init.Mode                = DMA_NORMAL;
    hdma_uart1_tx.Init.Priority            = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_uart1_tx) != HAL_OK) {
        Error_Handler();
    }

    /* Link DMA to UART TX */
    __HAL_LINKDMA(&huart1, hdmatx, hdma_uart1_tx);

    /* Enable DMA1 Channel 4 interrupt */
    HAL_NVIC_SetPriority(UART_TX_DMA_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(UART_TX_DMA_IRQn);
}

/* ============================================================
 * USART1 Init: PA9(TX), PA10(RX), 115200 baud
 * ============================================================ */
static void MX_USART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 = TX */
    GPIO_InitStruct.Pin   = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_PORT, &GPIO_InitStruct);

    /* PA10 = RX */
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

    /* Enable USART1 interrupt for IT mode */
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
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
 * SysTick Handler
 * ============================================================ */
void SysTick_Handler(void) {
    HAL_IncTick();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    printf("ASSERT: %s:%lu\r\n", file, line);
    Error_Handler();
}
#endif
