/**
 * STM32_03_Timer_ID_Multiple
 * 3 timers share single callback. pvTimerGetTimerID() distinguishes them.
 * Timer0=500ms, Timer1=1000ms, Timer2=2000ms.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TimerHandle_t xTimers[3];
static const uint32_t ulTimerPeriods[] = {500, 1000, 2000};
static volatile uint32_t ulTimerCounts[3] = {0, 0, 0};

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

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* Single callback for all 3 timers */
static void vSharedTimerCallback(TimerHandle_t xTimer)
{
    uint32_t ulTimerID = (uint32_t)pvTimerGetTimerID(xTimer);

    if (ulTimerID < 3)
    {
        ulTimerCounts[ulTimerID]++;
        printf("[TIMER-%lu] Fired! Period=%lums, Count=%lu, Tick=%lu\r\n",
               ulTimerID, ulTimerPeriods[ulTimerID],
               ulTimerCounts[ulTimerID],
               (unsigned long)xTaskGetTickCount());

        /* Toggle LED only on Timer 0 for visual feedback */
        if (ulTimerID == 0)
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}

static void vMonitorTask(void *pvParameters)
{
    for (;;)
    {
        printf("\r\n--- Multi-Timer Status @ Tick=%lu ---\r\n",
               (unsigned long)xTaskGetTickCount());
        for (int i = 0; i < 3; i++)
        {
            printf("  Timer-%d (%lums): %lu fires\r\n",
                   i, ulTimerPeriods[i], ulTimerCounts[i]);
        }

        /* Verify timing ratios */
        if (ulTimerCounts[0] > 0)
        {
            printf("  Ratio T1/T0: %.2f (expected 0.50)\r\n",
                   (float)ulTimerCounts[1] / (float)ulTimerCounts[0]);
            printf("  Ratio T2/T0: %.2f (expected 0.25)\r\n",
                   (float)ulTimerCounts[2] / (float)ulTimerCounts[0]);
        }

        printf("  Monitor HWM: %lu\r\n",
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

    printf("\r\n=== STM32 Timer ID Multiple ===\r\n");
    printf("3 timers, 1 callback, pvTimerGetTimerID\r\n\r\n");

    const char *names[] = {"Tim500", "Tim1000", "Tim2000"};

    for (int i = 0; i < 3; i++)
    {
        xTimers[i] = xTimerCreate(names[i],
                                  pdMS_TO_TICKS(ulTimerPeriods[i]),
                                  pdTRUE,
                                  (void *)(uint32_t)i,
                                  vSharedTimerCallback);
        if (xTimers[i] != NULL)
        {
            xTimerStart(xTimers[i], 0);
            printf("[INIT] Timer-%d created: %lums\r\n", i, ulTimerPeriods[i]);
        }
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
