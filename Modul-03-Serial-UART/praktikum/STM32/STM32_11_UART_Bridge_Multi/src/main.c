/**
 * STM32_11_UART_Bridge_Multi
 * 
 * Bridge between USART1 and USART2.
 * USART1 (PA9/PA10) — connected to PC/terminal at 115200 baud
 * USART2 (PA2/PA3)  — connected to external device at 9600 baud
 * 
 * Data from USART1 is forwarded to USART2 and vice versa.
 * Both UARTs use interrupt-based receive.
 * Byte counts are tracked for each direction.
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
UART_HandleTypeDef huart2;

/* Single-byte RX for each UART */
static uint8_t rx1_byte;
static uint8_t rx2_byte;

/* Byte counters */
static volatile uint32_t bytes_uart1_to_uart2 = 0;
static volatile uint32_t bytes_uart2_to_uart1 = 0;

static uint32_t last_report_time = 0;

/* ==================== UART Callbacks ==================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        /* Data from PC → forward to external device */
        HAL_UART_Transmit(&huart2, &rx1_byte, 1, 10);
        bytes_uart1_to_uart2++;
        HAL_UART_Receive_IT(&huart1, &rx1_byte, 1);
    }
    else if (huart->Instance == USART2) {
        /* Data from external device → forward to PC */
        HAL_UART_Transmit(&huart1, &rx2_byte, 1, 10);
        bytes_uart2_to_uart1++;
        HAL_UART_Receive_IT(&huart2, &rx2_byte, 1);
    }
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
    huart1.Init.BaudRate = UART1_BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = UART2_BAUD;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

#ifdef STM32F103xB
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
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

        HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

#ifdef STM32F103xB
        /* PA2 = TX (AF Push-Pull) */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* PA3 = RX (Input) */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

        HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

/* ==================== IRQ Handlers ==================== */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
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
    MX_USART2_UART_Init();

    printf("\r\n===== UART Bridge: USART1 <-> USART2 =====\r\n");
    printf("USART1 (PA9/PA10): %lu baud — PC/Terminal\r\n", (unsigned long)UART1_BAUD);
    printf("USART2 (PA2/PA3):  %lu baud — External Device\r\n", (unsigned long)UART2_BAUD);
    printf("Bridge active. Forwarding data...\r\n\r\n");

    /* Start interrupt-based receive on both UARTs */
    HAL_UART_Receive_IT(&huart1, &rx1_byte, 1);
    HAL_UART_Receive_IT(&huart2, &rx2_byte, 1);

    last_report_time = HAL_GetTick();

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Periodic statistics report */
        if ((now - last_report_time) >= REPORT_INTERVAL_MS) {
            last_report_time = now;

            printf("[Bridge Stats] UART1->UART2: %lu bytes | UART2->UART1: %lu bytes\r\n",
                   (unsigned long)bytes_uart1_to_uart2,
                   (unsigned long)bytes_uart2_to_uart1);
        }
    }
}
