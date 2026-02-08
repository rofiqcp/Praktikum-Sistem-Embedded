/**
 * STM32_05_Timer_Timeout_Monitor
 * Heartbeat monitoring with auto-reload watchdog timer (5s).
 * Simulated task sends heartbeats. If missed, timeout fires warning.
 * xTimerReset() refreshes the watchdog on each heartbeat.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static TimerHandle_t xWatchdogTimer;
static volatile uint32_t ulHeartbeatCount = 0;
static volatile uint32_t ulTimeoutCount = 0;
static volatile uint32_t ulHeartbeatInterval = 2000; /* Normal: 2s < 5s timeout */

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

/* Watchdog timeout - no heartbeat received within 5s */
static void vWatchdogCallback(TimerHandle_t xTimer)
{
    ulTimeoutCount++;
    /* Rapid blink = alarm */
    for (int i = 0; i < 6; i++)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        /* Small busy-wait in timer callback for visual effect */
        volatile uint32_t cnt = 0;
        while (cnt < 100000) cnt++;
    }

    printf("[TIMEOUT] *** WATCHDOG FIRED *** #%lu! No heartbeat for 5s! Tick=%lu\r\n",
           ulTimeoutCount, (unsigned long)xTaskGetTickCount());
}

/* Simulated heartbeat task - sends periodic heartbeats */
static void vHeartbeatTask(void *pvParameters)
{
    uint32_t ulCycle = 0;

    for (;;)
    {
        ulCycle++;
        ulHeartbeatCount++;

        /* Reset watchdog timer (feed the dog) */
        xTimerReset(xWatchdogTimer, pdMS_TO_TICKS(100));

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        printf("[HEARTBEAT] #%lu sent, watchdog reset, Tick=%lu\r\n",
               ulHeartbeatCount, (unsigned long)xTaskGetTickCount());

        /* Every 5 cycles, simulate a hang (delay 7s > 5s timeout) */
        if (ulCycle % 5 == 0 && ulCycle > 0)
        {
            printf("[HEARTBEAT] *** Simulating hang for 7 seconds! ***\r\n");
            vTaskDelay(pdMS_TO_TICKS(7000));
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(ulHeartbeatInterval));
        }
    }
}

static void vMonitorTask(void *pvParameters)
{
    for (;;)
    {
        printf("[MONITOR] Heartbeats=%lu, Timeouts=%lu, Interval=%lums, HWM=%lu\r\n",
               ulHeartbeatCount, ulTimeoutCount, ulHeartbeatInterval,
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

    printf("\r\n=== STM32 Timer Timeout Monitor ===\r\n");
    printf("Watchdog: 5s, Heartbeat: 2s, Hang every 5 beats\r\n\r\n");

    /* 5 second watchdog timer (auto-reload to keep checking) */
    xWatchdogTimer = xTimerCreate("Watchdog",
                                  pdMS_TO_TICKS(5000),
                                  pdTRUE,
                                  NULL,
                                  vWatchdogCallback);

    if (xWatchdogTimer != NULL)
    {
        xTimerStart(xWatchdogTimer, 0);
        printf("[INIT] Watchdog timer started (5s)\r\n");
    }

    xTaskCreate(vHeartbeatTask, "Heartbeat", 256, NULL, 2, NULL);
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
