/**
 * STM32_01_UART_Echo
 * ------------------
 * USART1 polling echo: receive one byte, transmit it back.
 * TX = PA9, RX = PA10
 * Works on F103 (Blue Pill) and F4xx (F401/F411 Black Pill).
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#endif
#include "config.h"

/* -------- Global handles -------- */
UART_HandleTypeDef huart1;

/* -------- Prototypes -------- */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void Error_Handler(void);

/* Minimal _write stub so newlib doesn't pull in semihosting */
int _write(int file, char *ptr, int len) { (void)file; (void)ptr; return len; }

/* ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    const char *banner = "UART Echo Ready\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)banner, strlen(banner), UART_TIMEOUT);

    uint8_t rx;

    while (1)
    {
        /* Block until one byte arrives (with timeout) */
        if (HAL_UART_Receive(&huart1, &rx, 1, UART_TIMEOUT) == HAL_OK)
        {
            /* Echo it back */
            HAL_UART_Transmit(&huart1, &rx, 1, UART_TIMEOUT);

            /* Toggle LED on each received byte */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}

/* ================================================================
 *  Clock configuration – F103 72 MHz  /  F4xx 84 MHz
 * ================================================================ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

#if defined(STM32F103xB)
    /* ---- F103: HSE 8 MHz → PLL 72 MHz ---- */
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInit) != HAL_OK) Error_Handler();

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2) != HAL_OK) Error_Handler();

#elif defined(STM32F401xC) || defined(STM32F411xE)
    /* ---- F4xx: HSE 25 MHz → PLL 84 MHz ---- */
    RCC_OscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInit.HSEState       = RCC_HSE_ON;
    RCC_OscInit.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInit.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInit.PLL.PLLM       = 25;
    RCC_OscInit.PLL.PLLN       = 168;
    RCC_OscInit.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInit.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInit) != HAL_OK) Error_Handler();

    RCC_ClkInit.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInit.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInit, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
#endif
}

/* ================================================================
 *  GPIO: LED on PC13
 * ================================================================ */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* LED PC13 */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off (active-low) */
}

/* ================================================================
 *  USART1 Init – PA9 TX, PA10 RX
 * ================================================================ */
static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#if defined(STM32F103xB)
    /* F103: TX = AF Push-Pull, RX = Input floating */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_10;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#elif defined(STM32F401xC) || defined(STM32F411xE)
    /* F4xx: Both TX & RX = AF Push-Pull, AF7 */
    GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUD;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

/* ================================================================ */
void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}

#include <string.h>   /* strlen */
