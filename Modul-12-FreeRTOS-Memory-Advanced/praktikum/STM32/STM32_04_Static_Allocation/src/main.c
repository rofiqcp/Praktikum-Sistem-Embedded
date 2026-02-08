/**
 * ============================================================================
 * STM32_04_Static_Allocation
 * ============================================================================
 * Use xTaskCreateStatic() and xQueueCreateStatic() — no dynamic allocation
 * after initialisation.
 *
 * configSUPPORT_STATIC_ALLOCATION  = 1
 * configSUPPORT_DYNAMIC_ALLOCATION = 1  (kept for FreeRTOS internals)
 *
 * Must provide:
 *   - vApplicationGetIdleTaskMemory()
 *   - vApplicationGetTimerTaskMemory()
 *
 * Platform : STM32F103C8 (Blue Pill)
 * UART1    : PA9 (TX), PA10 (RX) @ 115200
 * LED      : PC13 (active low)
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

/* ── Peripheral handles ──────────────────────────────────────────────────── */
UART_HandleTypeDef huart1;

/* ── Forward declarations ────────────────────────────────────────────────── */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void ProducerTask(void *pvParameters);
static void ConsumerTask(void *pvParameters);
static void StatusTask(void *pvParameters);
static void LedBlinkTask(void *pvParameters);

/* ── Printf redirect ────────────────────────────────────────────────────── */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

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
    printf("[HOOK] Stack overflow: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/* ════════════════════════════════════════════════════════════════════════════
 * Static memory for Idle task (required when configSUPPORT_STATIC_ALLOCATION=1)
 * ════════════════════════════════════════════════════════════════════════════ */
static StaticTask_t xIdleTaskTCB;
static StackType_t  xIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Static memory for Timer task
 * ════════════════════════════════════════════════════════════════════════════ */
static StaticTask_t xTimerTaskTCB;
static StackType_t  xTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    uint32_t      *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = xTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ════════════════════════════════════════════════════════════════════════════
 * Static memory for application tasks
 * ════════════════════════════════════════════════════════════════════════════ */
#define PRODUCER_STACK_SIZE  256
#define CONSUMER_STACK_SIZE  256
#define STATUS_STACK_SIZE    256
#define LED_STACK_SIZE       128

static StaticTask_t xProducerTCB;
static StackType_t  xProducerStack[PRODUCER_STACK_SIZE];

static StaticTask_t xConsumerTCB;
static StackType_t  xConsumerStack[CONSUMER_STACK_SIZE];

static StaticTask_t xStatusTCB;
static StackType_t  xStatusStack[STATUS_STACK_SIZE];

static StaticTask_t xLedTCB;
static StackType_t  xLedStack[LED_STACK_SIZE];

/* ════════════════════════════════════════════════════════════════════════════
 * Static queue
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    uint32_t id;
    uint32_t value;
    uint32_t timestamp;
} QueueItem_t;

#define QUEUE_LENGTH  8

static StaticQueue_t xQueueControlBlock;
static uint8_t       xQueueStorageArea[QUEUE_LENGTH * sizeof(QueueItem_t)];
static QueueHandle_t xDataQueue = NULL;

/* ── Task handles ────────────────────────────────────────────────────────── */
static TaskHandle_t hProducer = NULL;
static TaskHandle_t hConsumer = NULL;
static TaskHandle_t hStatus   = NULL;
static TaskHandle_t hLed      = NULL;

/* ── Counters ────────────────────────────────────────────────────────────── */
static volatile uint32_t itemsProduced = 0;
static volatile uint32_t itemsConsumed = 0;

/* ── Record initial free heap to detect dynamic allocs ───────────────────── */
static size_t heapAfterInit = 0;

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32_04: Static Allocation Demo\r\n");
    printf("==========================================\r\n");
    printf("configSUPPORT_STATIC_ALLOCATION  = %d\r\n", configSUPPORT_STATIC_ALLOCATION);
    printf("configSUPPORT_DYNAMIC_ALLOCATION = %d\r\n", configSUPPORT_DYNAMIC_ALLOCATION);
    printf("Heap size: %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("Free heap before init: %u bytes\r\n",
           (unsigned int)xPortGetFreeHeapSize());
    printf("------------------------------------------\r\n\r\n");

    /* Create static queue */
    xDataQueue = xQueueCreateStatic(QUEUE_LENGTH,
                                    sizeof(QueueItem_t),
                                    xQueueStorageArea,
                                    &xQueueControlBlock);

    if (xDataQueue == NULL)
    {
        printf("[ERROR] Failed to create static queue!\r\n");
        for (;;) { }
    }
    printf("[INIT] Static queue created: %d items x %u bytes\r\n",
           QUEUE_LENGTH, (unsigned int)sizeof(QueueItem_t));

    /* Create static tasks */
    hProducer = xTaskCreateStatic(ProducerTask, "Producer",
                                  PRODUCER_STACK_SIZE, NULL, 2,
                                  xProducerStack, &xProducerTCB);
    printf("[INIT] Producer task created statically (stack=%d words)\r\n",
           PRODUCER_STACK_SIZE);

    hConsumer = xTaskCreateStatic(ConsumerTask, "Consumer",
                                  CONSUMER_STACK_SIZE, NULL, 2,
                                  xConsumerStack, &xConsumerTCB);
    printf("[INIT] Consumer task created statically (stack=%d words)\r\n",
           CONSUMER_STACK_SIZE);

    hStatus = xTaskCreateStatic(StatusTask, "Status",
                                STATUS_STACK_SIZE, NULL, 3,
                                xStatusStack, &xStatusTCB);
    printf("[INIT] Status task created statically (stack=%d words)\r\n",
           STATUS_STACK_SIZE);

    hLed = xTaskCreateStatic(LedBlinkTask, "LED",
                             LED_STACK_SIZE, NULL, 1,
                             xLedStack, &xLedTCB);
    printf("[INIT] LED task created statically (stack=%d words)\r\n",
           LED_STACK_SIZE);

    /* Record heap state after all static init */
    heapAfterInit = xPortGetFreeHeapSize();
    printf("\r\n[INIT] Free heap after all static init: %u bytes\r\n",
           (unsigned int)heapAfterInit);
    printf("[INIT] No further dynamic allocations should occur.\r\n");
    printf("[INIT] Starting scheduler...\r\n\r\n");

    vTaskStartScheduler();

    for (;;) { }
}

/* ── ProducerTask ────────────────────────────────────────────────────────── */
static void ProducerTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t counter = 0;

    for (;;)
    {
        QueueItem_t item;
        item.id        = counter;
        item.value     = counter * 10 + 7;
        item.timestamp = HAL_GetTick();

        BaseType_t result = xQueueSend(xDataQueue, &item, pdMS_TO_TICKS(100));
        if (result == pdPASS)
        {
            printf("[PRODUCER] Sent item #%lu: value=%lu, tick=%lu\r\n",
                   (unsigned long)item.id,
                   (unsigned long)item.value,
                   (unsigned long)item.timestamp);
            itemsProduced++;
        }
        else
        {
            printf("[PRODUCER] Queue full, item #%lu dropped\r\n",
                   (unsigned long)counter);
        }

        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ── ConsumerTask ────────────────────────────────────────────────────────── */
static void ConsumerTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        QueueItem_t item;
        BaseType_t result = xQueueReceive(xDataQueue, &item, pdMS_TO_TICKS(2000));

        if (result == pdPASS)
        {
            uint32_t latency = HAL_GetTick() - item.timestamp;
            printf("[CONSUMER] Recv item #%lu: value=%lu, latency=%lu ms\r\n",
                   (unsigned long)item.id,
                   (unsigned long)item.value,
                   (unsigned long)latency);
            itemsConsumed++;
        }
        else
        {
            printf("[CONSUMER] Queue empty (timeout)\r\n");
        }
    }
}

/* ── StatusTask – verifies no dynamic allocation after init ──────────────── */
static void StatusTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t cycle = 0;

    for (;;)
    {
        size_t currentFree = xPortGetFreeHeapSize();
        size_t minEver     = xPortGetMinimumEverFreeHeapSize();

        printf("[STATUS] Cycle=%lu | Produced=%lu | Consumed=%lu\r\n",
               (unsigned long)cycle,
               (unsigned long)itemsProduced,
               (unsigned long)itemsConsumed);

        printf("[STATUS] Free heap: %u | MinEver: %u | AfterInit: %u\r\n",
               (unsigned int)currentFree,
               (unsigned int)minEver,
               (unsigned int)heapAfterInit);

        /* Check if any dynamic allocation happened after init */
        if (currentFree < heapAfterInit)
        {
            printf("[STATUS] WARNING: Dynamic allocation detected! Lost %u bytes\r\n",
                   (unsigned int)(heapAfterInit - currentFree));
        }
        else
        {
            printf("[STATUS] OK: No dynamic allocation after init\r\n");
        }

        /* Stack watermarks */
        printf("[STATUS] Watermarks: Producer=%u Consumer=%u Status=%u LED=%u\r\n",
               (unsigned int)uxTaskGetStackHighWaterMark(hProducer),
               (unsigned int)uxTaskGetStackHighWaterMark(hConsumer),
               (unsigned int)uxTaskGetStackHighWaterMark(hStatus),
               (unsigned int)uxTaskGetStackHighWaterMark(hLed));

        /* Queue status */
        UBaseType_t queueWaiting = uxQueueMessagesWaiting(xDataQueue);
        UBaseType_t queueFree    = uxQueueSpacesAvailable(xDataQueue);
        printf("[STATUS] Queue: %u/%d items | Free=%u slots\r\n\r\n",
               (unsigned int)queueWaiting, QUEUE_LENGTH,
               (unsigned int)queueFree);

        cycle++;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
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
