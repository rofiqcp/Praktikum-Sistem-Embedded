/**
 * STM32_05_Memory_Pool
 * 
 * Fixed-size memory pool allocator (fragmentation-free).
 * Pool of 8 blocks x 64 bytes each, managed with a FreeRTOS queue as free-list.
 * Multiple tasks request/release blocks from pool.
 * Prints pool usage statistics over UART1.
 * 
 * Hardware: STM32F103C8 BluePill
 * - UART1: PA9 (TX), PA10 (RX) @ 115200
 * - LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---- Defines ---- */
#define POOL_BLOCK_SIZE     64
#define POOL_NUM_BLOCKS     8
#define LED_PIN             GPIO_PIN_13
#define LED_PORT            GPIOC

/* ---- Globals ---- */
static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xPrintMutex;

/* Memory Pool */
static uint8_t ucPoolMemory[POOL_NUM_BLOCKS][POOL_BLOCK_SIZE];
static QueueHandle_t xFreeBlockQueue;
static volatile uint32_t ulAllocCount = 0;
static volatile uint32_t ulFreeCount = 0;
static volatile uint32_t ulAllocFailCount = 0;

/* ---- _write for printf ---- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ---- Clock Config ---- */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ---- GPIO Init ---- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PC13 LED */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); /* LED off */
}

/* ---- UART1 Init ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 TX */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 RX */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

/* ---- Memory Pool Functions ---- */
static void MemPool_Init(void)
{
    xFreeBlockQueue = xQueueCreate(POOL_NUM_BLOCKS, sizeof(void *));
    configASSERT(xFreeBlockQueue != NULL);

    for (int i = 0; i < POOL_NUM_BLOCKS; i++)
    {
        void *pBlock = (void *)ucPoolMemory[i];
        xQueueSend(xFreeBlockQueue, &pBlock, 0);
    }

    printf("[POOL] Initialized: %d blocks x %d bytes = %d bytes total\r\n",
           POOL_NUM_BLOCKS, POOL_BLOCK_SIZE, POOL_NUM_BLOCKS * POOL_BLOCK_SIZE);
}

static void *MemPool_Alloc(TickType_t xTimeout)
{
    void *pBlock = NULL;
    if (xQueueReceive(xFreeBlockQueue, &pBlock, xTimeout) == pdTRUE)
    {
        ulAllocCount++;
        return pBlock;
    }
    ulAllocFailCount++;
    return NULL;
}

static BaseType_t MemPool_Free(void *pBlock)
{
    if (pBlock == NULL) return pdFALSE;

    if (xQueueSend(xFreeBlockQueue, &pBlock, 0) == pdTRUE)
    {
        ulFreeCount++;
        return pdTRUE;
    }
    return pdFALSE;
}

static uint32_t MemPool_GetFreeCount(void)
{
    return (uint32_t)uxQueueMessagesWaiting(xFreeBlockQueue);
}

static uint32_t MemPool_GetUsedCount(void)
{
    return POOL_NUM_BLOCKS - MemPool_GetFreeCount();
}

/* ---- Producer Task: allocates blocks, fills data, then releases ---- */
static void vProducerTask(void *pvParameters)
{
    uint32_t ulTaskId = (uint32_t)pvParameters;
    uint32_t ulCycle = 0;

    for (;;)
    {
        ulCycle++;

        /* Try to allocate 1-3 blocks */
        uint32_t ulNumBlocks = (ulCycle % 3) + 1;
        void *pBlocks[3] = {NULL, NULL, NULL};
        uint32_t ulGot = 0;

        for (uint32_t i = 0; i < ulNumBlocks; i++)
        {
            pBlocks[i] = MemPool_Alloc(pdMS_TO_TICKS(100));
            if (pBlocks[i] != NULL)
            {
                /* Fill with pattern */
                memset(pBlocks[i], (uint8_t)(ulTaskId * 0x10 + i), POOL_BLOCK_SIZE);
                ulGot++;
            }
        }

        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            printf("[POOL] Task%lu cycle%lu: alloc=%lu/%lu free=%lu used=%lu\r\n",
                   ulTaskId, ulCycle, ulGot, ulNumBlocks,
                   MemPool_GetFreeCount(), MemPool_GetUsedCount());
            xSemaphoreGive(xPrintMutex);
        }

        /* Hold blocks for a while */
        vTaskDelay(pdMS_TO_TICKS(200 + ulTaskId * 50));

        /* Verify data integrity before freeing */
        for (uint32_t i = 0; i < ulGot; i++)
        {
            if (pBlocks[i] != NULL)
            {
                uint8_t expected = (uint8_t)(ulTaskId * 0x10 + i);
                uint8_t *pData = (uint8_t *)pBlocks[i];
                BaseType_t bCorrupted = pdFALSE;

                for (int j = 0; j < POOL_BLOCK_SIZE; j++)
                {
                    if (pData[j] != expected)
                    {
                        bCorrupted = pdTRUE;
                        break;
                    }
                }

                if (bCorrupted)
                {
                    if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
                    {
                        printf("[POOL] WARNING: Task%lu block%lu corrupted!\r\n",
                               ulTaskId, i);
                        xSemaphoreGive(xPrintMutex);
                    }
                }

                MemPool_Free(pBlocks[i]);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ---- Consumer Task: allocates a single block, uses it briefly ---- */
static void vConsumerTask(void *pvParameters)
{
    uint32_t ulTaskId = (uint32_t)pvParameters;
    uint32_t ulCycle = 0;

    for (;;)
    {
        ulCycle++;

        void *pBlock = MemPool_Alloc(pdMS_TO_TICKS(500));
        if (pBlock != NULL)
        {
            /* Write a message into the block */
            snprintf((char *)pBlock, POOL_BLOCK_SIZE,
                     "Consumer%lu msg#%lu tick=%lu",
                     ulTaskId, ulCycle, (uint32_t)xTaskGetTickCount());

            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[POOL] Consumer%lu: wrote [%s]\r\n", ulTaskId, (char *)pBlock);
                xSemaphoreGive(xPrintMutex);
            }

            vTaskDelay(pdMS_TO_TICKS(150));
            MemPool_Free(pBlock);
        }
        else
        {
            if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                printf("[POOL] Consumer%lu: alloc FAILED (pool exhausted)\r\n", ulTaskId);
                xSemaphoreGive(xPrintMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300 + ulTaskId * 100));
    }
}

/* ---- Stats Task ---- */
static void vStatsTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(3000));

        if (xSemaphoreTake(xPrintMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            uint32_t ulFree = MemPool_GetFreeCount();
            uint32_t ulUsed = MemPool_GetUsedCount();
            uint32_t ulUtil = (ulUsed * 100) / POOL_NUM_BLOCKS;

            printf("\r\n===== MEMORY POOL STATS =====\r\n");
            printf("[STATS] Total blocks : %d\r\n", POOL_NUM_BLOCKS);
            printf("[STATS] Block size   : %d bytes\r\n", POOL_BLOCK_SIZE);
            printf("[STATS] Free blocks  : %lu\r\n", ulFree);
            printf("[STATS] Used blocks  : %lu\r\n", ulUsed);
            printf("[STATS] Utilization  : %lu%%\r\n", ulUtil);
            printf("[STATS] Total allocs : %lu\r\n", ulAllocCount);
            printf("[STATS] Total frees  : %lu\r\n", ulFreeCount);
            printf("[STATS] Alloc fails  : %lu\r\n", ulAllocFailCount);
            printf("[STATS] Heap remain  : %u bytes\r\n",
                   (unsigned int)xPortGetFreeHeapSize());
            printf("=============================\r\n\r\n");

            /* Toggle LED */
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

            xSemaphoreGive(xPrintMutex);
        }
    }
}

/* ---- FreeRTOS Hooks ---- */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/* ---- Main ---- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART1_Init();

    printf("\r\n\r\n========================================\r\n");
    printf("  STM32_05 Memory Pool Allocator\r\n");
    printf("  8 blocks x 64 bytes, Queue free-list\r\n");
    printf("========================================\r\n\r\n");

    xPrintMutex = xSemaphoreCreateMutex();
    configASSERT(xPrintMutex != NULL);

    MemPool_Init();

    xTaskCreate(vProducerTask, "Prod1", 256, (void *)1, 2, NULL);
    xTaskCreate(vProducerTask, "Prod2", 256, (void *)2, 2, NULL);
    xTaskCreate(vConsumerTask, "Cons1", 256, (void *)1, 3, NULL);
    xTaskCreate(vConsumerTask, "Cons2", 256, (void *)2, 3, NULL);
    xTaskCreate(vStatsTask,   "Stats", 256, NULL,       1, NULL);

    printf("[MAIN] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    for (;;) {}
}
