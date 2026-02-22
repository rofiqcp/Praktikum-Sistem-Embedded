/**
 * STM32_02_Timer_Period_Change
 * Button PA0 cycles timer period: 200ms→500ms→1000ms→2000ms.
 * Uses xTimerChangePeriod(). LED blink rate changes visually.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TimerHandle_t xBlinkTimer;
static const uint32_t ulPeriods[] = {200, 500, 1000, 2000};
static const uint32_t ulNumPeriods = 4;
static volatile uint32_t ulPeriodIndex = 0;
static volatile uint32_t ulBlinkCount = 0;
static volatile uint32_t ulButtonPresses = 0;

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

    /* PC13 LED */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    /* PA0 Button with EXTI rising edge */
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

        ulPeriodIndex = (ulPeriodIndex + 1) % ulNumPeriods;
        ulButtonPresses++;

        xTimerChangePeriodFromISR(xBlinkTimer,
                                  pdMS_TO_TICKS(ulPeriods[ulPeriodIndex]),
                                  &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void vBlinkTimerCallback(TimerHandle_t xTimer)
{
    ulBlinkCount++;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    printf("[BLINK] Toggle #%lu, Period=%lums, Tick=%lu\r\n",
           ulBlinkCount, ulPeriods[ulPeriodIndex],
           (unsigned long)xTaskGetTickCount());
}

static void vMonitorTask(void *pvParameters)
{
    uint32_t lastPresses = 0;
    for (;;)
    {
        if (ulButtonPresses != lastPresses)
        {
            lastPresses = ulButtonPresses;
            printf("[PERIOD] Changed to %lums (press #%lu)\r\n",
                   ulPeriods[ulPeriodIndex], ulButtonPresses);
        }

        printf("[STATUS] Blinks=%lu, Period=%lums, Presses=%lu, HWM=%lu\r\n",
               ulBlinkCount, ulPeriods[ulPeriodIndex], ulButtonPresses,
               (unsigned long)uxTaskGetStackHighWaterMark(NULL));

        vTaskDelay(pdMS_TO_TICKS(3000));
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

    printf("\r\n=== STM32 Timer Period Change ===\r\n");
    printf("Press PA0 to cycle: 200ms->500ms->1000ms->2000ms\r\n\r\n");

    xBlinkTimer = xTimerCreate("Blink", pdMS_TO_TICKS(ulPeriods[0]),
                               pdTRUE, NULL, vBlinkTimerCallback);

    if (xBlinkTimer != NULL)
    {
        xTimerStart(xBlinkTimer, 0);
        printf("[INIT] Timer started at %lums\r\n", ulPeriods[0]);
    }

    xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
