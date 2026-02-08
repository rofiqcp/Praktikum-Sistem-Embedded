/**
 * STM32_08_Notification_Counting
 * Task notification as counting semaphore.
 * Event generator increments count, handler decrements one at a time.
 * LED pulses once per event.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TaskHandle_t xHandlerTask = NULL;
static volatile uint32_t ulEventsGenerated = 0;
static volatile uint32_t ulEventsProcessed = 0;

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

/* Event generator - sends burst of events */
static void vEventGeneratorTask(void *pvParameters)
{
    uint32_t ulBatch = 0;

    for (;;)
    {
        ulBatch++;
        /* Generate 1 to 5 events in a burst */
        uint32_t ulBurstCount = (ulBatch % 5) + 1;

        printf("[GEN] Batch #%lu: Generating %lu events\r\n", ulBatch, ulBurstCount);

        for (uint32_t i = 0; i < ulBurstCount; i++)
        {
            ulEventsGenerated++;
            /* Increment notification count (counting semaphore Give) */
            xTaskNotifyGive(xHandlerTask);
            printf("[GEN]   Event #%lu sent\r\n", ulEventsGenerated);
        }

        /* Wait before next burst */
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* Event handler - processes events one at a time */
static void vEventHandlerTask(void *pvParameters)
{
    uint32_t ulCount;

    for (;;)
    {
        /* Decrement by 1 each time (pdFALSE = don't clear all) */
        ulCount = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        if (ulCount > 0)
        {
            ulEventsProcessed++;

            /* LED pulse for each event processed */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* ON */
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* OFF */
            vTaskDelay(pdMS_TO_TICKS(100));

            printf("[HANDLER] Event #%lu processed (pending=%lu), Tick=%lu\r\n",
                   ulEventsProcessed,
                   ulEventsGenerated - ulEventsProcessed,
                   (unsigned long)xTaskGetTickCount());
        }
    }
}

static void vMonitorTask(void *pvParameters)
{
    for (;;)
    {
        printf("[MONITOR] Generated=%lu, Processed=%lu, Pending=%lu, HWM=%lu\r\n",
               ulEventsGenerated, ulEventsProcessed,
               ulEventsGenerated - ulEventsProcessed,
               (unsigned long)uxTaskGetStackHighWaterMark(xHandlerTask));

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

    printf("\r\n=== STM32 Notification Counting ===\r\n");
    printf("Counting semaphore via task notifications\r\n\r\n");

    xTaskCreate(vEventHandlerTask, "Handler", 256, NULL, 3, &xHandlerTask);
    xTaskCreate(vEventGeneratorTask, "Generator", 256, NULL, 1, NULL);
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
