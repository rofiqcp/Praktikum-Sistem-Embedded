/**
 * STM32_01_Software_Timer_Basic
 * One-shot timer toggles LED after 3s, auto-reload timer blinks at 1s.
 * Demonstrates xTimerCreate, xTimerStart for both timer types.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

/* --- UART handle for printf --- */
UART_HandleTypeDef huart1;

/* --- Timer handles --- */
static TimerHandle_t xOneShotTimer;
static TimerHandle_t xAutoReloadTimer;

/* --- Counters --- */
static volatile uint32_t ulOneShotCount = 0;
static volatile uint32_t ulAutoReloadCount = 0;

/* ======================== System Config ======================== */
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

    /* PC13 LED (active low) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
}

/* ======================== Timer Callbacks ======================== */
static void vOneShotTimerCallback(TimerHandle_t xTimer)
{
    ulOneShotCount++;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    printf("[ONE-SHOT] Timer fired! Count=%lu, Tick=%lu\r\n",
           ulOneShotCount, (unsigned long)xTaskGetTickCount());
    printf("[ONE-SHOT] This timer will NOT fire again (one-shot)\r\n");
}

static void vAutoReloadTimerCallback(TimerHandle_t xTimer)
{
    ulAutoReloadCount++;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    printf("[AUTO-RELOAD] Timer fired! Count=%lu, Tick=%lu\r\n",
           ulAutoReloadCount, (unsigned long)xTaskGetTickCount());
}

/* ======================== Monitor Task ======================== */
static void vMonitorTask(void *pvParameters)
{
    for (;;)
    {
        printf("\r\n--- Timer Status @ Tick=%lu ---\r\n",
               (unsigned long)xTaskGetTickCount());
        printf("  One-shot fires : %lu\r\n", ulOneShotCount);
        printf("  Auto-reload fires: %lu\r\n", ulAutoReloadCount);
        printf("  One-shot active  : %s\r\n",
               xTimerIsTimerActive(xOneShotTimer) ? "YES" : "NO");
        printf("  Auto-reload active: %s\r\n",
               xTimerIsTimerActive(xAutoReloadTimer) ? "YES" : "NO");

        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        printf("  Monitor stack HWM: %lu words\r\n", (unsigned long)hwm);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ======================== Hook Functions ======================== */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/* ======================== Main ======================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    GPIO_Init();

    printf("\r\n=== STM32 FreeRTOS Software Timer Basic ===\r\n");
    printf("One-shot timer: 3000ms, Auto-reload timer: 1000ms\r\n\r\n");

    /* Create one-shot timer (3 second delay) */
    xOneShotTimer = xTimerCreate(
        "OneShot",
        pdMS_TO_TICKS(3000),
        pdFALSE,               /* One-shot */
        (void *)0,
        vOneShotTimerCallback
    );

    /* Create auto-reload timer (1 second interval) */
    xAutoReloadTimer = xTimerCreate(
        "AutoReload",
        pdMS_TO_TICKS(1000),
        pdTRUE,                /* Auto-reload */
        (void *)1,
        vAutoReloadTimerCallback
    );

    if (xOneShotTimer != NULL && xAutoReloadTimer != NULL)
    {
        xTimerStart(xOneShotTimer, 0);
        xTimerStart(xAutoReloadTimer, 0);
        printf("[INIT] Both timers started successfully\r\n");
    }
    else
    {
        printf("[ERROR] Failed to create timers!\r\n");
    }

    /* Create monitor task */
    xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);

    printf("[INIT] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}
