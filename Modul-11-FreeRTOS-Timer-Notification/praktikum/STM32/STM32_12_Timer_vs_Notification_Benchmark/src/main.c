/**
 * STM32_12_Timer_vs_Notification_Benchmark
 * Run 1000 iterations comparing:
 *   1) Timer callback response
 *   2) Task notification response
 *   3) Binary semaphore response
 * Print results table with min/max/avg ticks.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

#define BENCH_ITERATIONS  1000
#define TIMER_PERIOD_MS   1

/* Benchmark results */
typedef struct {
    uint32_t ulMin;
    uint32_t ulMax;
    uint32_t ulTotal;
    uint32_t ulCount;
} BenchResult_t;

static BenchResult_t xTimerResult;
static BenchResult_t xNotifyResult;
static BenchResult_t xSemaResult;

static TaskHandle_t xBenchTask = NULL;
static TaskHandle_t xNotifyWaitTask = NULL;
static SemaphoreHandle_t xBenchSemaphore = NULL;
static TimerHandle_t xBenchTimer = NULL;

static volatile TickType_t xStartTick = 0;
static volatile uint8_t ucTimerDone = 0;

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

static void vUpdateResult(BenchResult_t *r, uint32_t val)
{
    if (val < r->ulMin) r->ulMin = val;
    if (val > r->ulMax) r->ulMax = val;
    r->ulTotal += val;
    r->ulCount++;
}

static void vInitResult(BenchResult_t *r)
{
    r->ulMin = 0xFFFFFFFF;
    r->ulMax = 0;
    r->ulTotal = 0;
    r->ulCount = 0;
}

/* Timer callback for benchmark */
static void vBenchTimerCallback(TimerHandle_t xTimer)
{
    TickType_t xElapsed = xTaskGetTickCount() - xStartTick;
    vUpdateResult(&xTimerResult, (uint32_t)xElapsed);
    ucTimerDone = 1;
}

/* Notification receiver for benchmark */
static void vNotifyReceiverTask(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        TickType_t xElapsed = xTaskGetTickCount() - xStartTick;
        vUpdateResult(&xNotifyResult, (uint32_t)xElapsed);
    }
}

/* Main benchmark task */
static void vBenchmarkTask(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); /* Let system settle */

    printf("\r\n========================================\r\n");
    printf("  BENCHMARK: Timer vs Notify vs Sema\r\n");
    printf("  Iterations: %d each\r\n", BENCH_ITERATIONS);
    printf("========================================\r\n\r\n");

    /* ========== 1) Timer callback benchmark ========== */
    printf("[BENCH] Running Timer benchmark (%d iterations)...\r\n", BENCH_ITERATIONS);
    vInitResult(&xTimerResult);

    for (int i = 0; i < BENCH_ITERATIONS; i++)
    {
        ucTimerDone = 0;
        xStartTick = xTaskGetTickCount();
        xTimerStart(xBenchTimer, portMAX_DELAY);

        /* Wait for callback to complete */
        while (!ucTimerDone)
        {
            vTaskDelay(1);
        }

        if ((i + 1) % 200 == 0)
        {
            printf("  Timer: %d/%d done\r\n", i + 1, BENCH_ITERATIONS);
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
    printf("[BENCH] Timer benchmark complete\r\n\r\n");

    /* ========== 2) Task notification benchmark ========== */
    printf("[BENCH] Running Notification benchmark (%d iterations)...\r\n", BENCH_ITERATIONS);
    vInitResult(&xNotifyResult);

    for (int i = 0; i < BENCH_ITERATIONS; i++)
    {
        xStartTick = xTaskGetTickCount();
        xTaskNotifyGive(xNotifyWaitTask);
        vTaskDelay(pdMS_TO_TICKS(TIMER_PERIOD_MS + 1));

        if ((i + 1) % 200 == 0)
        {
            printf("  Notify: %d/%d done\r\n", i + 1, BENCH_ITERATIONS);
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
    printf("[BENCH] Notification benchmark complete\r\n\r\n");

    /* ========== 3) Binary semaphore benchmark ========== */
    printf("[BENCH] Running Semaphore benchmark (%d iterations)...\r\n", BENCH_ITERATIONS);
    vInitResult(&xSemaResult);

    for (int i = 0; i < BENCH_ITERATIONS; i++)
    {
        xStartTick = xTaskGetTickCount();
        xSemaphoreGive(xBenchSemaphore);

        /* Measure give+take cycle */
        if (xSemaphoreTake(xBenchSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            TickType_t xElapsed = xTaskGetTickCount() - xStartTick;
            vUpdateResult(&xSemaResult, (uint32_t)xElapsed);
        }

        if ((i + 1) % 200 == 0)
        {
            printf("  Sema: %d/%d done\r\n", i + 1, BENCH_ITERATIONS);
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
    printf("[BENCH] Semaphore benchmark complete\r\n\r\n");

    /* ========== Print Results Table ========== */
    printf("========================================\r\n");
    printf("  BENCHMARK RESULTS (%d iterations)\r\n", BENCH_ITERATIONS);
    printf("========================================\r\n");
    printf("%-15s %8s %8s %8s %8s\r\n", "Method", "Min", "Max", "Avg", "Count");
    printf("%-15s %8s %8s %8s %8s\r\n", "------", "---", "---", "---", "-----");

    if (xTimerResult.ulCount > 0)
    {
        printf("%-15s %8lu %8lu %8lu %8lu\r\n", "Timer",
               xTimerResult.ulMin, xTimerResult.ulMax,
               xTimerResult.ulTotal / xTimerResult.ulCount,
               xTimerResult.ulCount);
    }

    if (xNotifyResult.ulCount > 0)
    {
        printf("%-15s %8lu %8lu %8lu %8lu\r\n", "Notification",
               xNotifyResult.ulMin, xNotifyResult.ulMax,
               xNotifyResult.ulTotal / xNotifyResult.ulCount,
               xNotifyResult.ulCount);
    }

    if (xSemaResult.ulCount > 0)
    {
        printf("%-15s %8lu %8lu %8lu %8lu\r\n", "Semaphore",
               xSemaResult.ulMin, xSemaResult.ulMax,
               xSemaResult.ulTotal / xSemaResult.ulCount,
               xSemaResult.ulCount);
    }

    printf("========================================\r\n");
    printf("(All values in ticks, 1 tick = 1ms)\r\n\r\n");

    /* Done - blink LED rapidly */
    printf("[BENCH] Benchmark complete! LED blinking.\r\n");
    for (;;)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(200));
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

    printf("\r\n=== STM32 Timer vs Notification Benchmark ===\r\n");
    printf("%d iterations per method\r\n\r\n", BENCH_ITERATIONS);

    /* Create benchmark resources */
    xBenchSemaphore = xSemaphoreCreateBinary();
    xBenchTimer = xTimerCreate("Bench",
                               pdMS_TO_TICKS(TIMER_PERIOD_MS),
                               pdFALSE,
                               NULL,
                               vBenchTimerCallback);

    xTaskCreate(vNotifyReceiverTask, "NotifyRx", 256, NULL, 4, &xNotifyWaitTask);
    xTaskCreate(vBenchmarkTask, "Bench", 512, NULL, 2, &xBenchTask);

    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
