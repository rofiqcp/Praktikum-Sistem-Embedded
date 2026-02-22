/**
 * STM32_10_UART_Timeout_Parser
 * 
 * Timeout-based packet detection using SysTick (HAL_GetTick()).
 * Buffers incoming bytes via UART interrupt.
 * If no new byte arrives within TIMEOUT_MS, packet is considered complete.
 * Displays received packet as hex dump.
 */

#include "config.h"

#ifdef STM32F103xB
  #include "stm32f1xx_hal.h"
#elif defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32F411xE)
  #include "stm32f4xx_hal.h"
#else
  #error "Unsupported MCU"
#endif

#include <stdio.h>
#include <string.h>

/* ==================== Global Variables ==================== */
UART_HandleTypeDef huart1;

/* RX buffer and state */
static uint8_t rx_byte;
static uint8_t packet_buf[MAX_PACKET];
static volatile uint16_t packet_len = 0;
static volatile uint32_t last_byte_time = 0;
static volatile uint8_t receiving = 0;

static uint32_t packet_count = 0;

/* ==================== UART Callback ==================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (packet_len < MAX_PACKET) {
            packet_buf[packet_len] = rx_byte;
            packet_len++;
        }
        last_byte_time = HAL_GetTick();
        receiving = 1;

        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ==================== Hex Dump ==================== */
static void print_hex_dump(const uint8_t *data, uint16_t len)
{
    for (uint16_t offset = 0; offset < len; offset += HEX_COLS) {
        /* Address */
        printf("  %04X: ", offset);

        /* Hex bytes */
        for (uint16_t i = 0; i < HEX_COLS; i++) {
            if (offset + i < len) {
                printf("%02X ", data[offset + i]);
            } else {
                printf("   ");
            }
            if (i == 7) printf(" ");  /* middle separator */
        }

        /* ASCII */
        printf(" |");
        for (uint16_t i = 0; i < HEX_COLS; i++) {
            if (offset + i < len) {
                uint8_t c = data[offset + i];
                printf("%c", (c >= 0x20 && c <= 0x7E) ? c : '.');
            } else {
                printf(" ");
            }
        }
        printf("|\r\n");
    }
}

/* ==================== Process Packet ==================== */
static void process_packet(void)
{
    uint16_t len = packet_len;

    if (len == 0) return;

    packet_count++;
    printf("--- Packet #%lu [%u bytes] (timeout=%ums) ---\r\n",
           (unsigned long)packet_count, len, TIMEOUT_MS);
    print_hex_dump(packet_buf, len);
    printf("\r\n");

    /* Reset for next packet */
    packet_len = 0;
    receiving = 0;
}

/* ==================== System Configuration ==================== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef STM32F103xB
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();

#ifdef STM32F103xB
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA9 = TX (AF Push-Pull) */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* PA10 = RX (Input) */
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

/* ==================== IRQ Handler ==================== */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

/* ==================== Retarget printf ==================== */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void Error_Handler(void)
{
    while (1) {
        /* Stay here */
    }
}

/* ==================== Main ==================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();

    printf("\r\n===== UART Timeout Parser =====\r\n");
    printf("Timeout: %u ms\r\n", TIMEOUT_MS);
    printf("Max packet: %u bytes\r\n", MAX_PACKET);
    printf("Waiting for data...\r\n\r\n");

    /* Start receiving via interrupt */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    while (1) {
        /* Check for timeout: if we are receiving and enough time has passed */
        if (receiving && packet_len > 0) {
            uint32_t now = HAL_GetTick();
            uint32_t elapsed = now - last_byte_time;

            if (elapsed >= TIMEOUT_MS) {
                /* Timeout expired — packet is complete */
                process_packet();
            }
        }
    }
}
