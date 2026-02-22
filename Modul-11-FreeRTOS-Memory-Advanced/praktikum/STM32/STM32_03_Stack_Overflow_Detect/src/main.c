/**
 * ============================================================================
 * STM32_03_Stack_Overflow_Detect
 * ============================================================================
 * configCHECK_FOR_STACK_OVERFLOW = 2  (fill-pattern method)
 * Create tasks with deliberately small stacks to trigger overflow.
 * vApplicationStackOverflowHook reports the offending task.
 * Also monitors stack high-water marks for tasks that do NOT overflow.
 *
 * Platform : STM32F103C8 (Blue Pill)
 * UART1    : PA9 (TX), PA10 (RX) @ 115200
 * LED      : PC13 (active low)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* ── Peripheral handles ──────────────────────────────────────────────────── */
UART_HandleTypeDef huart1;

/* ── Forward declarations ────────────────────────────────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void SafeTask(void *pvParameters);
static void WatermarkMonitorTask(void *pvParameters);
static void RecursiveTask(void *pvParameters);
static void BigLocalVarTask(void *pvParameters);
static void LedBlinkTask(void *pvParameters);

/* ── Printf redirect ────────────────────────────────────────────────────── */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ── Overflow detection flag ─────────────────────────────────────────────── */
static volatile uint32_t overflowDetected = 0;
static char overflowTaskName[configMAX_TASK_NAME_LEN + 1];

/* ── FreeRTOS hooks ──────────────────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    printf("[HOOK] Malloc failed! Free=%u\r\n",
           (unsigned int)xPortGetFreeHeapSize());
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    overflowDetected++;

    /* Copy task name safely */
    int i;
    for (i = 0; i < configMAX_TASK_NAME_LEN && pcTaskName[i] != '\0'; i++)
    {
        overflowTaskName[i] = pcTaskName[i];
    }
    overflowTaskName[i] = '\0';

    /*
     * NOTE: We do NOT call printf here because the stack is corrupted.
     * Instead we set flags and let the monitor task report.
     * In a real system you would typically halt or reset.
     * For demonstration, we blink the LED fast and halt.
     */
    /* Rapid blink to indicate overflow */
    for (int j = 0; j < 20; j++)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        for (volatile int k = 0; k < 100000; k++) { }
    }

    /* Transmit error message directly (stack may be corrupted, use minimal stack) */
    const char msg1[] = "\r\n[OVERFLOW] Stack overflow detected in task: ";
    HAL_UART_Transmit(&huart1, (const uint8_t *)msg1, sizeof(msg1) - 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (const uint8_t *)overflowTaskName, (uint16_t)i, HAL_MAX_DELAY);
    const char msg2[] = "\r\n[OVERFLOW] System halted for safety.\r\n";
    HAL_UART_Transmit(&huart1, (const uint8_t *)msg2, sizeof(msg2) - 1, HAL_MAX_DELAY);

    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/* ── Task handles for watermark monitoring ───────────────────────────────── */
static TaskHandle_t hSafeTask      = NULL;
static TaskHandle_t hMonitorTask   = NULL;
static TaskHandle_t hRecursiveTask = NULL;
static TaskHandle_t hBigLocalTask  = NULL;
static TaskHandle_t hLedTask       = NULL;

/* ── Test phase control ──────────────────────────────────────────────────── */
static volatile uint32_t testPhase = 0;

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32_03: Stack Overflow Detection\r\n");
    printf("==========================================\r\n");
    printf("configCHECK_FOR_STACK_OVERFLOW = %d\r\n", configCHECK_FOR_STACK_OVERFLOW);
    printf("Method 2: Fill-pattern checking\r\n");
    printf("Heap: %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("------------------------------------------\r\n\r\n");

    /* Safe tasks with adequate stacks */
    xTaskCreate(SafeTask,             "Safe",     256, NULL, 2, &hSafeTask);
    xTaskCreate(WatermarkMonitorTask, "WMark",    256, NULL, 3, &hMonitorTask);
    xTaskCreate(LedBlinkTask,         "LED",      128, NULL, 1, &hLedTask);

    printf("[INIT] Safe tasks created. Starting scheduler...\r\n");
    printf("[INIT] Dangerous tasks will be created after 10 seconds.\r\n\r\n");

    vTaskStartScheduler();

    for (;;) { }
}

/* ── SafeTask – normal operation with adequate stack ─────────────────────── */
static void SafeTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t counter = 0;
    char buffer[32];

    for (;;)
    {
        /* Do some moderate stack work */
        snprintf(buffer, sizeof(buffer), "Safe counter: %lu", (unsigned long)counter);
        printf("[SAFE] %s | Watermark=%u words\r\n",
               buffer,
               (unsigned int)uxTaskGetStackHighWaterMark(NULL));

        counter++;

        /* After 10 seconds, create the dangerous tasks */
        if (counter == 5 && testPhase == 0)
        {
            testPhase = 1;
            printf("\r\n[SAFE] === Creating DANGEROUS tasks ===\r\n");
            printf("[SAFE] Phase 1: RecursiveTask with 96-word stack\r\n\r\n");

            /* Deliberately small stack – 96 words = 384 bytes */
            BaseType_t ret = xTaskCreate(RecursiveTask, "Recurse", 96, NULL, 2, &hRecursiveTask);
            if (ret != pdPASS)
            {
                printf("[SAFE] Failed to create RecursiveTask\r\n");
            }
        }

        if (counter == 10 && testPhase == 1)
        {
            testPhase = 2;
            printf("\r\n[SAFE] === Phase 2: BigLocalVarTask with 80-word stack ===\r\n\r\n");

            /* Even smaller stack – 80 words = 320 bytes */
            BaseType_t ret = xTaskCreate(BigLocalVarTask, "BigLocal", 80, NULL, 2, &hBigLocalTask);
            if (ret != pdPASS)
            {
                printf("[SAFE] Failed to create BigLocalVarTask\r\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ── WatermarkMonitorTask – reports all task watermarks ──────────────────── */
static void WatermarkMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        printf("[WMARK] --- Stack Watermark Report ---\r\n");

        if (hSafeTask != NULL)
        {
            printf("[WMARK] Safe     : %u words remaining\r\n",
                   (unsigned int)uxTaskGetStackHighWaterMark(hSafeTask));
        }
        if (hMonitorTask != NULL)
        {
            printf("[WMARK] WMark    : %u words remaining\r\n",
                   (unsigned int)uxTaskGetStackHighWaterMark(hMonitorTask));
        }
        if (hLedTask != NULL)
        {
            printf("[WMARK] LED      : %u words remaining\r\n",
                   (unsigned int)uxTaskGetStackHighWaterMark(hLedTask));
        }
        if (hRecursiveTask != NULL)
        {
            printf("[WMARK] Recurse  : %u words remaining\r\n",
                   (unsigned int)uxTaskGetStackHighWaterMark(hRecursiveTask));
        }
        if (hBigLocalTask != NULL)
        {
            printf("[WMARK] BigLocal : %u words remaining\r\n",
                   (unsigned int)uxTaskGetStackHighWaterMark(hBigLocalTask));
        }

        printf("[WMARK] Overflow count: %lu\r\n",
               (unsigned long)overflowDetected);
        printf("[WMARK] Free heap: %u | Phase: %lu\r\n\r\n",
               (unsigned int)xPortGetFreeHeapSize(),
               (unsigned long)testPhase);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ── RecursiveTask – deliberate deep recursion to blow the stack ──────────── */
static volatile uint32_t recursion_depth = 0;

static void recursive_function(uint32_t depth)
{
    volatile uint8_t local_buffer[32];    /* 32 bytes per frame */
    memset((void *)local_buffer, (int)depth, sizeof(local_buffer));

    recursion_depth = depth;
    printf("[RECURSE] Depth=%lu | Watermark=%u\r\n",
           (unsigned long)depth,
           (unsigned int)uxTaskGetStackHighWaterMark(NULL));

    vTaskDelay(pdMS_TO_TICKS(500));

    /* Keep recursing until stack overflow is detected */
    recursive_function(depth + 1);
}

static void RecursiveTask(void *pvParameters)
{
    (void)pvParameters;

    printf("[RECURSE] Starting recursive calls with 96-word stack...\r\n");
    printf("[RECURSE] Each frame uses ~32+ bytes of stack\r\n");

    recursive_function(0);

    /* Should never reach here */
    printf("[RECURSE] Recursion ended at depth %lu\r\n",
           (unsigned long)recursion_depth);
    vTaskSuspend(NULL);
}

/* ── BigLocalVarTask – large local arrays blow the stack ──────────────────── */
static void BigLocalVarTask(void *pvParameters)
{
    (void)pvParameters;

    printf("[BIGLOCAL] Starting with 80-word stack...\r\n");
    printf("[BIGLOCAL] Will allocate large local arrays...\r\n");

    /* Step 1: Small locals – should be fine */
    {
        volatile uint8_t small[16];
        memset((void *)small, 0xAA, sizeof(small));
        printf("[BIGLOCAL] Step 1: 16-byte local OK | Watermark=%u\r\n",
               (unsigned int)uxTaskGetStackHighWaterMark(NULL));
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Step 2: Medium locals – getting tight */
    {
        volatile uint8_t medium[64];
        memset((void *)medium, 0xBB, sizeof(medium));
        printf("[BIGLOCAL] Step 2: 64-byte local OK | Watermark=%u\r\n",
               (unsigned int)uxTaskGetStackHighWaterMark(NULL));
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Step 3: Large locals – should trigger overflow */
    {
        volatile uint8_t large[256];
        memset((void *)large, 0xCC, sizeof(large));
        printf("[BIGLOCAL] Step 3: 256-byte local | Watermark=%u\r\n",
               (unsigned int)uxTaskGetStackHighWaterMark(NULL));
    }

    /* Should never reach here */
    printf("[BIGLOCAL] All steps completed without overflow?!\r\n");
    vTaskSuspend(NULL);
}

/* ── LedBlinkTask ────────────────────────────────────────────────────────── */
static void LedBlinkTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ── System Clock: HSE 8 MHz → PLL 72 MHz ───────────────────────────────── */
static void SystemClock_Config(void)
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

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ── GPIO: PC13 LED ──────────────────────────────────────────────────────── */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* ── USART1: PA9 TX, PA10 RX, 115200 ────────────────────────────────────── */
static void MX_USART1_UART_Init(void)
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
