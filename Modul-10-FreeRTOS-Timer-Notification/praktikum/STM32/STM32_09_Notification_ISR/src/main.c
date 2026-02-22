/**
 * STM32_09_Notification_ISR
 * Button PA0 ISR → vTaskNotifyGiveFromISR.
 * Compare response time with equivalent binary semaphore approach.
 * Print tick measurements for both methods.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TaskHandle_t xNotifyTask = NULL;
static TaskHandle_t xSemaphoreTask = NULL;
static SemaphoreHandle_t xBinarySemaphore = NULL;

static volatile TickType_t xISRTick = 0;
static volatile uint32_t ulUseNotification = 1; /* Toggle between methods */

/* Timing results */
#define MAX_SAMPLES 50
static uint32_t ulNotifyTimes[MAX_SAMPLES];
static uint32_t ulSemaphoreTimes[MAX_SAMPLES];
static volatile uint32_t ulNotifyIdx = 0;
static volatile uint32_t ulSemaphoreIdx = 0;
static volatile uint32_t ulTotalNotify = 0;
static volatile uint32_t ulTotalSemaphore = 0;

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
        xISRTick = xTaskGetTickCountFromISR();

        if (ulUseNotification)
        {
            vTaskNotifyGiveFromISR(xNotifyTask, &xHigherPriorityTaskWoken);
        }
        else
        {
            xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* Notification handler task */
static void vNotifyHandlerTask(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t xResponseTime = xTaskGetTickCount() - xISRTick;
        ulTotalNotify++;

        if (ulNotifyIdx < MAX_SAMPLES)
        {
            ulNotifyTimes[ulNotifyIdx++] = (uint32_t)xResponseTime;
        }

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        printf("[NOTIFY] Response: %lu ticks, Total=%lu\r\n",
               (unsigned long)xResponseTime, ulTotalNotify);

        /* Switch to semaphore mode after this */
        ulUseNotification = 0;
    }
}

/* Semaphore handler task */
static void vSemaphoreHandlerTask(void *pvParameters)
{
    for (;;)
    {
        xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);

        TickType_t xResponseTime = xTaskGetTickCount() - xISRTick;
        ulTotalSemaphore++;

        if (ulSemaphoreIdx < MAX_SAMPLES)
        {
            ulSemaphoreTimes[ulSemaphoreIdx++] = (uint32_t)xResponseTime;
        }

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        printf("[SEMA] Response: %lu ticks, Total=%lu\r\n",
               (unsigned long)xResponseTime, ulTotalSemaphore);

        /* Switch back to notification mode */
        ulUseNotification = 1;
    }
}

static void vReportTask(void *pvParameters)
{
    for (;;)
    {
        printf("\r\n--- ISR Response Comparison ---\r\n");
        printf("Mode: %s\r\n", ulUseNotification ? "NOTIFICATION" : "SEMAPHORE");
        printf("Notify samples: %lu, Semaphore samples: %lu\r\n",
               ulNotifyIdx, ulSemaphoreIdx);

        if (ulNotifyIdx > 0)
        {
            uint32_t sum = 0, mn = 0xFFFFFFFF, mx = 0;
            for (uint32_t i = 0; i < ulNotifyIdx; i++)
            {
                sum += ulNotifyTimes[i];
                if (ulNotifyTimes[i] < mn) mn = ulNotifyTimes[i];
                if (ulNotifyTimes[i] > mx) mx = ulNotifyTimes[i];
            }
            printf("Notification: avg=%lu, min=%lu, max=%lu ticks\r\n",
                   sum / ulNotifyIdx, mn, mx);
        }

        if (ulSemaphoreIdx > 0)
        {
            uint32_t sum = 0, mn = 0xFFFFFFFF, mx = 0;
            for (uint32_t i = 0; i < ulSemaphoreIdx; i++)
            {
                sum += ulSemaphoreTimes[i];
                if (ulSemaphoreTimes[i] < mn) mn = ulSemaphoreTimes[i];
                if (ulSemaphoreTimes[i] > mx) mx = ulSemaphoreTimes[i];
            }
            printf("Semaphore:    avg=%lu, min=%lu, max=%lu ticks\r\n",
                   sum / ulSemaphoreIdx, mn, mx);
        }

        printf("Press PA0 to add samples (alternates method)\r\n");

        vTaskDelay(pdMS_TO_TICKS(10000));
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

    printf("\r\n=== STM32 Notification ISR Comparison ===\r\n");
    printf("PA0: alternates notification vs semaphore\r\n\r\n");

    xBinarySemaphore = xSemaphoreCreateBinary();

    xTaskCreate(vNotifyHandlerTask, "Notify", 256, NULL, 3, &xNotifyTask);
    xTaskCreate(vSemaphoreHandlerTask, "Sema", 256, NULL, 3, &xSemaphoreTask);
    xTaskCreate(vReportTask, "Report", 512, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
