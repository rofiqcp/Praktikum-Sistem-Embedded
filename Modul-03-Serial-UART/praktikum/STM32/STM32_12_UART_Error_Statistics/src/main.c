/**
 * STM32_12_UART_Error_Statistics
 * 
 * Monitor and count UART errors with parity checking enabled.
 * Overrides HAL_UART_ErrorCallback (weak function) to track:
 *   - Parity errors (PE)
 *   - Framing errors (FE)
 *   - Overrun errors (ORE)
 *   - Noise errors (NE)
 * 
 * Periodically prints error statistics and health status.
 * Health: GOOD (0 errors), WARNING (<5), CRITICAL (>=5)
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

/* RX */
static uint8_t rx_byte;
static volatile uint32_t rx_count = 0;

/* Error counters */
static volatile uint32_t parity_err  = 0;
static volatile uint32_t framing_err = 0;
static volatile uint32_t overrun_err = 0;
static volatile uint32_t noise_err   = 0;

static uint32_t last_report_time = 0;
static uint32_t report_number = 0;

/* ==================== UART Callbacks ==================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        rx_count++;
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/**
 * Override weak HAL_UART_ErrorCallback
 * Called when UART error interrupt fires
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint32_t err = huart->ErrorCode;

        if (err & HAL_UART_ERROR_PE) {
            parity_err++;
        }
        if (err & HAL_UART_ERROR_FE) {
            framing_err++;
        }
        if (err & HAL_UART_ERROR_ORE) {
            overrun_err++;
        }
        if (err & HAL_UART_ERROR_NE) {
            noise_err++;
        }

        /* Clear error flags and restart receive */
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ==================== Health Status ==================== */
static const char* get_health_status(uint32_t total_errors)
{
    if (total_errors == 0) {
        return "GOOD";
    } else if (total_errors <= CRIT_THRESHOLD) {
        return "WARNING";
    } else {
        return "CRITICAL";
    }
}

static void print_report(void)
{
    uint32_t total = parity_err + framing_err + overrun_err + noise_err;
    const char *status = get_health_status(total);

    report_number++;
    printf("\r\n========== UART Error Report #%lu ==========\r\n",
           (unsigned long)report_number);
    printf("  Bytes received : %lu\r\n", (unsigned long)rx_count);
    printf("  Parity errors  : %lu\r\n", (unsigned long)parity_err);
    printf("  Framing errors : %lu\r\n", (unsigned long)framing_err);
    printf("  Overrun errors : %lu\r\n", (unsigned long)overrun_err);
    printf("  Noise errors   : %lu\r\n", (unsigned long)noise_err);
    printf("  Total errors   : %lu\r\n", (unsigned long)total);
    printf("  Health status  : %s\r\n", status);
    printf("=============================================\r\n");
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
    huart1.Init.WordLength = UART_WORDLENGTH_9B;   /* 8 data + 1 parity bit */
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_EVEN;
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

        /* Enable UART error interrupt via NVIC */
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

    printf("\r\n===== UART Error Statistics Monitor =====\r\n");
    printf("Baud: %lu, Parity: EVEN, WordLen: 9-bit\r\n", (unsigned long)UART_BAUD);
    printf("Report interval: %u ms\r\n", REPORT_INTERVAL_MS);
    printf("Monitoring UART errors...\r\n\r\n");

    /* Enable error interrupts (EIE for framing/overrun/noise in DMA mode) */
    /* For interrupt mode, errors are caught via HAL_UART_IRQHandler */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_PE);   /* Parity Error interrupt */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);  /* Error interrupt (FE, ORE, NE) */

    /* Start receiving via interrupt */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    last_report_time = HAL_GetTick();

    while (1) {
        uint32_t now = HAL_GetTick();

        if ((now - last_report_time) >= REPORT_INTERVAL_MS) {
            last_report_time = now;
            print_report();
        }
    }
}
