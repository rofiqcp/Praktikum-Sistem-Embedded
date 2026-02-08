/**
 * ============================================================
 * STM32_03_DMA_UART_RX
 * UART1 DMA Receive with Idle Line Detection
 * DMA1 Channel 5 = USART1_RX (Circular Mode)
 * Target: STM32F103C8T6 Blue Pill (72MHz)
 * Framework: STM32Cube HAL
 * ============================================================
 *
 * Demonstrates continuous UART reception using DMA in circular
 * mode with idle line interrupt for variable-length messages.
 *
 * Key techniques:
 *   - DMA circular mode for continuous reception
 *   - UART IDLE line interrupt to detect end-of-message
 *   - DMA NDTR register to calculate received byte count
 *   - Double buffer tracking (last_pos vs current_pos)
 *   - Hex dump of received data
 *   - Statistics: total bytes, idle events, half/full transfers
 *
 * Hardware: UART1 PA9/PA10, LED PC13
 * Send data via serial terminal to test reception.
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ---- Handle Declarations ---- */
UART_HandleTypeDef huart1;
DMA_HandleTypeDef  hdma_uart1_rx;

/* ---- RX Buffers ---- */
static uint8_t rx_dma_buffer[RX_BUFFER_SIZE];
static uint8_t rx_process_buffer[RX_PROCESS_BUFFER_SIZE];

/* ---- State tracking ---- */
static volatile uint16_t last_dma_pos     = 0;
static volatile uint8_t  idle_detected    = 0;
static volatile uint16_t rx_data_length   = 0;

/* ---- Statistics ---- */
typedef struct {
    uint32_t total_bytes;
    uint32_t idle_events;
    uint32_t half_transfer_events;
    uint32_t full_transfer_events;
    uint32_t overrun_errors;
    uint32_t message_count;
} rx_stats_t;

static volatile rx_stats_t stats = {0};
static uint32_t last_stats_print = 0;

/* ---- Forward Declarations ---- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_Init(void);
static void MX_DMA_Init(void);
static void DWT_Init(void);
static void start_dma_reception(void);
static void process_received_data(void);
static void hex_dump(const uint8_t *data, uint16_t length);
static void print_statistics(void);
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
    printf("STM32F103 DMA UART RX with Idle Line Detection\r\n");
    printf("System Clock: %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("UART: 115200 baud, DMA1 Channel 5 Circular\r\n");
    printf("RX Buffer: %d bytes\r\n", RX_BUFFER_SIZE);
    printf("Send data via serial terminal to test...\r\n");
    print_separator();

    /* Start DMA reception in circular mode */
    start_dma_reception();

    last_stats_print = HAL_GetTick();

    while (1) {
        /* Process any received data on idle detection */
        if (idle_detected) {
            idle_detected = 0;
            process_received_data();
        }

        /* Print statistics periodically */
        if (HAL_GetTick() - last_stats_print >= STATS_PRINT_INTERVAL_MS) {
            last_stats_print = HAL_GetTick();
            print_statistics();
            LED_TOGGLE();
        }
    }
}

/* ============================================================
 * Start DMA reception in circular mode
 * ============================================================ */
static void start_dma_reception(void) {
    /* Clear buffer */
    memset(rx_dma_buffer, 0, RX_BUFFER_SIZE);
    last_dma_pos = 0;

    /* Start DMA reception */
    if (HAL_UART_Receive_DMA(&huart1, rx_dma_buffer, RX_BUFFER_SIZE) != HAL_OK) {
        printf("ERROR: Failed to start DMA RX\r\n");
        Error_Handler();
    }

    /* Enable UART IDLE line interrupt */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    printf("DMA RX started (circular mode)\r\n");
}

/* ============================================================
 * Process received data from DMA buffer
 * Handles circular buffer wrap-around
 * ============================================================ */
static void process_received_data(void) {
    /* Get current DMA write position */
    uint16_t current_pos = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart1_rx);

    if (current_pos == last_dma_pos) {
        return; /* No new data */
    }

    uint16_t length = 0;

    if (current_pos > last_dma_pos) {
        /* No wrap-around: simple copy */
        length = current_pos - last_dma_pos;
        if (length <= RX_PROCESS_BUFFER_SIZE) {
            memcpy(rx_process_buffer, &rx_dma_buffer[last_dma_pos], length);
        }
    } else {
        /* Wrap-around: copy in two parts */
        uint16_t part1 = RX_BUFFER_SIZE - last_dma_pos;
        uint16_t part2 = current_pos;
        length = part1 + part2;

        if (length <= RX_PROCESS_BUFFER_SIZE) {
            memcpy(rx_process_buffer, &rx_dma_buffer[last_dma_pos], part1);
            if (part2 > 0) {
                memcpy(&rx_process_buffer[part1], rx_dma_buffer, part2);
            }
        }
    }

    last_dma_pos = current_pos;

    if (length > 0 && length <= RX_PROCESS_BUFFER_SIZE) {
        stats.total_bytes += length;
        stats.message_count++;

        printf("\r\n[MSG #%lu] Received %u bytes:\r\n", stats.message_count, length);

        /* Print as string (if printable) */
        printf("  Text: ");
        for (uint16_t i = 0; i < length && i < 64; i++) {
            if (rx_process_buffer[i] >= 0x20 && rx_process_buffer[i] < 0x7F) {
                printf("%c", rx_process_buffer[i]);
            } else {
                printf(".");
            }
        }
        if (length > 64) printf("...(truncated)");
        printf("\r\n");

        /* Hex dump */
        hex_dump(rx_process_buffer, (length > 128) ? 128 : length);
    }
}

/* ============================================================
 * Hex dump utility
 * ============================================================ */
static void hex_dump(const uint8_t *data, uint16_t length) {
    printf("  Hex dump (%u bytes):\r\n", length);
    for (uint16_t i = 0; i < length; i += HEX_BYTES_PER_LINE) {
        printf("  %04X: ", i);

        /* Hex values */
        for (uint16_t j = 0; j < HEX_BYTES_PER_LINE && (i + j) < length; j++) {
            printf("%02X ", data[i + j]);
        }
        /* Padding for incomplete lines */
        for (uint16_t j = length - i; j < HEX_BYTES_PER_LINE && (i + length - i) < length; j++) {
            printf("   ");
        }
        printf(" | ");

        /* ASCII */
        for (uint16_t j = 0; j < HEX_BYTES_PER_LINE && (i + j) < length; j++) {
            uint8_t c = data[i + j];
            printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }
        printf("\r\n");
    }
}

/* ============================================================
 * Print reception statistics
 * ============================================================ */
static void print_statistics(void) {
    printf("\r\n--- RX Statistics ---\r\n");
    printf("  Total bytes received : %lu\r\n", stats.total_bytes);
    printf("  Messages (idle evts) : %lu\r\n", stats.idle_events);
    printf("  Half-transfer events : %lu\r\n", stats.half_transfer_events);
    printf("  Full-transfer events : %lu\r\n", stats.full_transfer_events);
    printf("  Overrun errors       : %lu\r\n", stats.overrun_errors);
    printf("  DMA buffer position  : %u / %d\r\n",
           (uint16_t)(RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart1_rx)),
           RX_BUFFER_SIZE);
    printf("--------------------\r\n");
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

/* ============================================================
 * USART1 IRQ Handler — handles IDLE line detection
 * ============================================================ */
void USART1_IRQHandler(void) {
    /* Check for IDLE line interrupt */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE)) {
        /* Clear IDLE flag: read SR then DR */
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);

        stats.idle_events++;
        idle_detected = 1;
    }

    /* Check for overrun error */
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart1);
        stats.overrun_errors++;
    }

    /* Let HAL handle other UART interrupts */
    HAL_UART_IRQHandler(&huart1);
}

/* ============================================================
 * DMA1 Channel 5 IRQ Handler (USART1_RX)
 * ============================================================ */
void DMA1_Channel5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_uart1_rx);
}

/* ============================================================
 * DMA Half-Transfer Callback
 * ============================================================ */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        stats.half_transfer_events++;
    }
}

/* ============================================================
 * DMA Full-Transfer Callback
 * ============================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        stats.full_transfer_events++;
    }
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
 * GPIO: LED PC13
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
 * DMA1 Init: Channel 5 for USART1_RX (Circular mode)
 * ============================================================ */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA1 Channel 5: USART1_RX */
    hdma_uart1_rx.Instance                 = UART_RX_DMA_CHANNEL;
    hdma_uart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_uart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_uart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_uart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_uart1_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_uart1_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_uart1_rx) != HAL_OK) {
        Error_Handler();
    }

    /* Link DMA to UART RX */
    __HAL_LINKDMA(&huart1, hdmarx, hdma_uart1_rx);

    /* Enable DMA interrupt */
    HAL_NVIC_SetPriority(UART_RX_DMA_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(UART_RX_DMA_IRQn);
}

/* ============================================================
 * USART1: PA9(TX), PA10(RX), 115200 baud
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

    /* Enable USART1 interrupt (for IDLE detection) */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
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

void SysTick_Handler(void) {
    HAL_IncTick();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    printf("ASSERT: %s:%lu\r\n", file, line);
    Error_Handler();
}
#endif
