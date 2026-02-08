/**
 * ============================================================================
 * STM32_01_Heap_Monitor
 * ============================================================================
 * Monitor FreeRTOS heap using xPortGetFreeHeapSize() and
 * xPortGetMinimumEverFreeHeapSize().
 *
 * A memory allocator task allocates and frees blocks of varying sizes.
 * A heap monitor task reports heap statistics every 2 seconds.
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
static void HeapMonitorTask(void *pvParameters);
static void MemoryAllocTask(void *pvParameters);
static void LedBlinkTask(void *pvParameters);

/* ── Printf redirect via UART1 ──────────────────────────────────────────── */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ── FreeRTOS hook functions ─────────────────────────────────────────────── */
void vApplicationMallocFailedHook(void)
{
    printf("[HOOK] Malloc failed! Free heap: %u bytes\r\n",
           (unsigned int)xPortGetFreeHeapSize());
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[HOOK] Stack overflow in task: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/* ── Allocation tracking ─────────────────────────────────────────────────── */
#define MAX_ALLOC_SLOTS  8

static void *allocTable[MAX_ALLOC_SLOTS];
static size_t allocSize[MAX_ALLOC_SLOTS];
static uint32_t totalAllocOps = 0;
static uint32_t totalFreeOps  = 0;

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32_01: FreeRTOS Heap Monitor\r\n");
    printf("========================================\r\n");
    printf("Total heap configured : %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("Free heap at start    : %u bytes\r\n",
           (unsigned int)xPortGetFreeHeapSize());
    printf("----------------------------------------\r\n\r\n");

    memset(allocTable, 0, sizeof(allocTable));
    memset(allocSize, 0, sizeof(allocSize));

    xTaskCreate(HeapMonitorTask, "HeapMon",  256, NULL, 3, NULL);
    xTaskCreate(MemoryAllocTask, "MemAlloc", 256, NULL, 2, NULL);
    xTaskCreate(LedBlinkTask,    "LED",      128, NULL, 1, NULL);

    vTaskStartScheduler();

    for (;;) { }
}

/* ── HeapMonitorTask ─────────────────────────────────────────────────────── */
static void HeapMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    uint32_t cycle = 0;

    for (;;)
    {
        size_t freeHeap    = xPortGetFreeHeapSize();
        size_t minEverFree = xPortGetMinimumEverFreeHeapSize();
        size_t usedHeap    = configTOTAL_HEAP_SIZE - freeHeap;
        uint32_t usagePct  = (usedHeap * 100) / configTOTAL_HEAP_SIZE;

        /* Count active allocations */
        uint32_t activeSlots = 0;
        size_t activeBytes = 0;
        for (int i = 0; i < MAX_ALLOC_SLOTS; i++)
        {
            if (allocTable[i] != NULL)
            {
                activeSlots++;
                activeBytes += allocSize[i];
            }
        }

        printf("[HEAP] Cycle=%lu | Free=%u | Used=%u | MinEver=%u | Usage=%lu%%\r\n",
               (unsigned long)cycle,
               (unsigned int)freeHeap,
               (unsigned int)usedHeap,
               (unsigned int)minEverFree,
               (unsigned long)usagePct);

        printf("[HEAP] ActiveSlots=%lu | ActiveBytes=%u | TotalAllocs=%lu | TotalFrees=%lu\r\n",
               (unsigned long)activeSlots,
               (unsigned int)activeBytes,
               (unsigned long)totalAllocOps,
               (unsigned long)totalFreeOps);

        /* Stack watermark for this task */
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        printf("[HEAP] HeapMon stack watermark: %u words\r\n\r\n",
               (unsigned int)watermark);

        cycle++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ── MemoryAllocTask ─────────────────────────────────────────────────────── */
static void MemoryAllocTask(void *pvParameters)
{
    (void)pvParameters;

    /* Sizes to cycle through */
    static const size_t sizes[] = { 64, 128, 256, 512, 100, 200, 300, 50 };
    uint32_t idx = 0;
    uint32_t round = 0;

    for (;;)
    {
        /* Phase 1 – allocate into the next free slot */
        int slot = -1;
        for (int i = 0; i < MAX_ALLOC_SLOTS; i++)
        {
            if (allocTable[i] == NULL)
            {
                slot = i;
                break;
            }
        }

        if (slot >= 0)
        {
            size_t sz = sizes[idx % (sizeof(sizes) / sizeof(sizes[0]))];
            void *ptr = pvPortMalloc(sz);
            if (ptr != NULL)
            {
                memset(ptr, 0xAA, sz);
                allocTable[slot] = ptr;
                allocSize[slot]  = sz;
                totalAllocOps++;

                printf("[ALLOC] Slot %d: %u bytes @ 0x%08lX | Free=%u\r\n",
                       slot, (unsigned int)sz, (unsigned long)(uintptr_t)ptr,
                       (unsigned int)xPortGetFreeHeapSize());
            }
            else
            {
                printf("[ALLOC] FAILED for %u bytes | Free=%u\r\n",
                       (unsigned int)sz, (unsigned int)xPortGetFreeHeapSize());
            }
            idx++;
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Phase 2 – every 4 allocs, free the oldest 2 slots */
        if ((totalAllocOps % 4) == 0 && totalAllocOps > 0)
        {
            int freed = 0;
            for (int i = 0; i < MAX_ALLOC_SLOTS && freed < 2; i++)
            {
                if (allocTable[i] != NULL)
                {
                    printf("[FREE]  Slot %d: %u bytes @ 0x%08lX | Free=%u\r\n",
                           i, (unsigned int)allocSize[i],
                           (unsigned long)(uintptr_t)allocTable[i],
                           (unsigned int)xPortGetFreeHeapSize());

                    vPortFree(allocTable[i]);
                    allocTable[i] = NULL;
                    allocSize[i]  = 0;
                    totalFreeOps++;
                    freed++;
                }
            }
            round++;
            printf("[ALLOC] Round %lu complete. Free heap: %u\r\n\r\n",
                   (unsigned long)round, (unsigned int)xPortGetFreeHeapSize());
        }

        vTaskDelay(pdMS_TO_TICKS(500));
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

    /* PC13 – on-board LED (active low) */
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
}

/* ── USART1: PA9 TX, PA10 RX, 115200 ────────────────────────────────────── */
static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA9 – TX */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 – RX */
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
