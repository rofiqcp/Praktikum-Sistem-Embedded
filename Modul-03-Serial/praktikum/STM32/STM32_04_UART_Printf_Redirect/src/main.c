/**
 * STM32_04_UART_Printf_Redirect
 * ------------------------------
 * Redirect printf() to USART1 via _write().
 * Periodically prints a formatted sensor data table.
 * Receives 't' to toggle LED, 'r' to print report.
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal.h"
#endif
#include "config.h"
#include <stdio.h>
#include <string.h>

/* ================================================================
 *  Global handles
 * ================================================================ */
UART_HandleTypeDef huart1;

/* -------- Prototypes -------- */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void Error_Handler(void);

/* ================================================================
 *  _write – redirect stdout/stderr to USART1
 * ================================================================ */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ================================================================
 *  Simple pseudo-random (LCG)
 * ================================================================ */
static uint32_t rng_state = 12345;
static uint32_t pseudo_rand(void)
{
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

static uint32_t rand_range(uint32_t lo, uint32_t hi)
{
    return lo + (pseudo_rand() % (hi - lo + 1));
}

/* ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    printf("\r\n=== Printf Redirect Ready ===\r\n");
    printf("Commands: 't' = toggle LED, 'r' = sensor report\r\n\r\n");

    uint32_t last_report = 0;
    uint32_t report_count = 0;

    while (1)
    {
        /* --- Check for incoming command (polling, non-blocking) --- */
        uint8_t cmd;
        if (HAL_UART_Receive(&huart1, &cmd, 1, 10) == HAL_OK)
        {
            switch (cmd)
            {
            case 't':
            case 'T':
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                printf("[CMD] LED toggled\r\n");
                break;
            case 'r':
            case 'R':
                /* Force immediate report */
                last_report = 0;
                break;
            default:
                printf("[CMD] Unknown: '%c' (0x%02X)\r\n", cmd, cmd);
                break;
            }
        }

        /* --- Periodic sensor report every 5 s --- */
        uint32_t now = HAL_GetTick();
        if (now - last_report >= 5000)
        {
            last_report = now;
            report_count++;

            /* Seed with tick for variation */
            rng_state = now ^ 0xDEAD;

            uint32_t temp_raw  = rand_range(SENSOR_MIN, SENSOR_MAX);
            uint32_t humid_raw = rand_range(SENSOR_MIN, SENSOR_MAX);
            uint32_t light_raw = rand_range(SENSOR_MIN, SENSOR_MAX);

            printf("+------+----------+----------+----------+\r\n");
            printf("| #%-4lu | Temp     | Humid    | Light    |\r\n", report_count);
            printf("+------+----------+----------+----------+\r\n");
            printf("| Raw  | %-8lu | %-8lu | %-8lu |\r\n", temp_raw, humid_raw, light_raw);
            printf("| Volt | %lu.%03lu V  | %lu.%03lu V  | %lu.%03lu V  |\r\n",
                   (temp_raw  * 3300 / 4095) / 1000, (temp_raw  * 3300 / 4095) % 1000,
                   (humid_raw * 3300 / 4095) / 1000, (humid_raw * 3300 / 4095) % 1000,
                   (light_raw * 3300 / 4095) / 1000, (light_raw * 3300 / 4095) % 1000);
            printf("+------+----------+----------+----------+\r\n");
            printf("  Uptime: %lu ms  |  LED: %s\r\n\r\n",
                   now,
                   HAL_GPIO_ReadPin(LED_PORT, LED_PIN) == GPIO_PIN_RESET ? "ON" : "OFF");
        }
    }
}

/* ================================================================
 *  Clock – F103 72 MHz / F4xx 84 MHz
 * ================================================================ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInit = {0};
    RCC_ClkInitTypeDef RCC_ClkInit = {0};

#if defined(STM32F103xB)
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
 *  GPIO: LED PC13
 * ================================================================ */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ================================================================
 *  USART1 Init – PA9 TX, PA10 RX
 * ================================================================ */
static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

#if defined(STM32F103xB)
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_10;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#elif defined(STM32F401xC) || defined(STM32F411xE)
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
