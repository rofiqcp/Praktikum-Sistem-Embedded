/**
 * STM32_04_Timer_Debounce
 * Button PA0 EXTI triggers xTimerResetFromISR() on one-shot 50ms timer.
 * Only stable press triggers action. Shows raw vs filtered event counts.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TimerHandle_t xDebounceTimer;
static volatile uint32_t ulRawInterrupts = 0;
static volatile uint32_t ulDebouncedPresses = 0;
static volatile uint32_t ulLEDState = 0;

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
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* PA0 button - both edges to catch bouncing */
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        ulRawInterrupts++;

        /* Reset the debounce timer - restarts the 50ms window */
        xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void vDebounceTimerCallback(TimerHandle_t xTimer)
{
    /* Timer expired = button stable for 50ms → valid press */
    ulDebouncedPresses++;
    ulLEDState = !ulLEDState;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,
                      ulLEDState ? GPIO_PIN_RESET : GPIO_PIN_SET);

    printf("[DEBOUNCE] Valid press #%lu! Raw IRQs=%lu, Filtered=%lu, Ratio=%.1f:1\r\n",
           ulDebouncedPresses, ulRawInterrupts,
           ulDebouncedPresses,
           ulDebouncedPresses > 0 ?
               (float)ulRawInterrupts / (float)ulDebouncedPresses : 0.0f);
}

static void vMonitorTask(void *pvParameters)
{
    for (;;)
    {
        printf("[STATUS] Raw=%lu, Debounced=%lu, LED=%s, HWM=%lu\r\n",
               ulRawInterrupts, ulDebouncedPresses,
               ulLEDState ? "ON" : "OFF",
               (unsigned long)uxTaskGetStackHighWaterMark(NULL));

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("[ERROR] Stack overflow: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    GPIO_Init();

    printf("\r\n=== STM32 Timer Debounce ===\r\n");
    printf("PA0 button with 50ms debounce timer\r\n\r\n");

    /* One-shot 50ms debounce timer */
    xDebounceTimer = xTimerCreate("Debounce",
                                  pdMS_TO_TICKS(50),
                                  pdFALSE,    /* One-shot */
                                  NULL,
                                  vDebounceTimerCallback);

    if (xDebounceTimer == NULL)
    {
        printf("[ERROR] Failed to create debounce timer!\r\n");
    }

    xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);

    printf("[INIT] Debounce ready. Press PA0 button...\r\n\r\n");
    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
