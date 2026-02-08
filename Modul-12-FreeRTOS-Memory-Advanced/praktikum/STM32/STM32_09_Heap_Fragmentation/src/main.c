/**
 * STM32_09_Heap_Fragmentation
 * 
 * Demonstrates heap fragmentation with FreeRTOS heap_4 allocator.
 * - Phase 1: Allocate blocks of varying sizes (32, 128, 64, 256, 96 bytes)
 * - Phase 2: Free every other block to create gaps
 * - Phase 3: Try to allocate a large block that fits total free but not in any gap
 * - Tracks: total free, largest free block, number of blocks
 * 
 * Platform: STM32F103C8 BluePill
 * UART1: PA9(TX), PA10(RX) @ 115200
 * LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/* ---- UART handle ---- */
UART_HandleTypeDef huart1;

/* ---- Printf redirect ---- */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ---- Clock Configuration: 72 MHz ---- */
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

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ---- GPIO Init: PC13 LED ---- */
static void GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
}

/* ---- UART1 Init ---- */
static void UART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* PA9 TX */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* PA10 RX */
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

/* ---- Allocation tracking ---- */
#define MAX_BLOCKS 20

typedef struct {
    void    *ptr;
    size_t   size;
    uint8_t  active;
} MemBlock_t;

static MemBlock_t blocks[MAX_BLOCKS];
static int block_count = 0;

static void print_heap_stats(const char *label)
{
    HeapStats_t stats;
    vPortGetHeapStats(&stats);
    printf("[HEAP_STATS] %s\r\n", label);
    printf("  Free bytes      : %u\r\n", (unsigned)stats.xAvailableHeapSpaceInBytes);
    printf("  Largest free blk: %u\r\n", (unsigned)stats.xSizeOfLargestFreeBlockInBytes);
    printf("  Smallest free   : %u\r\n", (unsigned)stats.xSizeOfSmallestFreeBlockInBytes);
    printf("  Free blocks     : %u\r\n", (unsigned)stats.xNumberOfFreeBlocks);
    printf("  Min ever free   : %u\r\n", (unsigned)stats.xMinimumEverFreeBytesRemaining);
    printf("  Alloc calls     : %u\r\n", (unsigned)stats.xNumberOfSuccessfulAllocations);
    printf("  Free calls      : %u\r\n", (unsigned)stats.xNumberOfSuccessfulFrees);
    printf("\r\n");
}

static void *tracked_alloc(size_t size)
{
    void *ptr = pvPortMalloc(size);
    if (ptr != NULL && block_count < MAX_BLOCKS) {
        blocks[block_count].ptr    = ptr;
        blocks[block_count].size   = size;
        blocks[block_count].active = 1;
        block_count++;
        printf("[ALLOC] Block #%d: addr=0x%08lX size=%u\r\n",
               block_count - 1, (uint32_t)ptr, (unsigned)size);
    } else if (ptr == NULL) {
        printf("[ALLOC_FAIL] Could not allocate %u bytes\r\n", (unsigned)size);
    }
    return ptr;
}

static void tracked_free(int index)
{
    if (index < block_count && blocks[index].active) {
        printf("[FREE] Block #%d: addr=0x%08lX size=%u\r\n",
               index, (uint32_t)blocks[index].ptr, (unsigned)blocks[index].size);
        vPortFree(blocks[index].ptr);
        blocks[index].ptr    = NULL;
        blocks[index].active = 0;
    }
}

static void print_block_map(void)
{
    printf("[BLOCK_MAP]\r\n");
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].active) {
            printf("  [%02d] USED  addr=0x%08lX size=%3u\r\n",
                   i, (uint32_t)blocks[i].ptr, (unsigned)blocks[i].size);
        } else {
            printf("  [%02d] FREE  (was %3u bytes)\r\n",
                   i, (unsigned)blocks[i].size);
        }
    }
    printf("\r\n");
}

/* ---- Fragmentation Demo Task ---- */
static void vFragmentationTask(void *pvParameters)
{
    (void)pvParameters;

    printf("\r\n========================================\r\n");
    printf("  STM32_09: Heap Fragmentation Demo\r\n");
    printf("  Heap allocator: heap_4\r\n");
    printf("  Total heap: %u bytes\r\n", (unsigned)configTOTAL_HEAP_SIZE);
    printf("========================================\r\n\r\n");

    print_heap_stats("Initial state");

    /* Phase 1: Allocate blocks of varying sizes */
    printf("--- PHASE 1: Allocating blocks ---\r\n");
    size_t sizes[] = {32, 128, 64, 256, 96, 48, 160, 80, 200, 112};
    int num_allocs = 10;

    for (int i = 0; i < num_allocs; i++) {
        tracked_alloc(sizes[i]);
    }
    print_heap_stats("After Phase 1 (all allocated)");
    print_block_map();

    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Phase 2: Free every other block to create gaps */
    printf("--- PHASE 2: Freeing every other block ---\r\n");
    for (int i = 0; i < num_allocs; i += 2) {
        tracked_free(i);
    }
    print_heap_stats("After Phase 2 (every other freed)");
    print_block_map();

    /* Calculate total freed */
    size_t total_freed = 0;
    for (int i = 0; i < num_allocs; i += 2) {
        total_freed += blocks[i].size;
    }
    printf("[ANALYSIS] Total freed: %u bytes (in scattered gaps)\r\n", (unsigned)total_freed);

    HeapStats_t stats;
    vPortGetHeapStats(&stats);
    printf("[ANALYSIS] Largest contiguous free: %u bytes\r\n",
           (unsigned)stats.xSizeOfLargestFreeBlockInBytes);
    printf("[ANALYSIS] Number of free blocks: %u\r\n\r\n",
           (unsigned)stats.xNumberOfFreeBlocks);

    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Phase 3: Try to allocate a block that fits total free but not largest gap */
    printf("--- PHASE 3: Large allocation attempt ---\r\n");
    size_t target_size = stats.xSizeOfLargestFreeBlockInBytes + 32;
    if (target_size > stats.xAvailableHeapSpaceInBytes) {
        target_size = stats.xAvailableHeapSpaceInBytes - 8;
    }
    printf("[ATTEMPT] Trying to allocate %u bytes...\r\n", (unsigned)target_size);
    printf("[ATTEMPT] Total free: %u bytes, Largest gap: %u bytes\r\n",
           (unsigned)stats.xAvailableHeapSpaceInBytes,
           (unsigned)stats.xSizeOfLargestFreeBlockInBytes);

    void *big_block = pvPortMalloc(target_size);
    if (big_block == NULL) {
        printf("[FRAGMENTATION] FAILED! %u bytes free but no contiguous block large enough\r\n",
               (unsigned)stats.xAvailableHeapSpaceInBytes);
        printf("[FRAGMENTATION] This demonstrates EXTERNAL FRAGMENTATION\r\n");
    } else {
        printf("[SUCCESS] Allocated %u bytes at 0x%08lX\r\n",
               (unsigned)target_size, (uint32_t)big_block);
        printf("[NOTE] heap_4 coalesced adjacent free blocks\r\n");
        vPortFree(big_block);
    }
    printf("\r\n");

    print_heap_stats("After Phase 3");

    /* Phase 4: Demonstrate defragmentation by freeing all */
    printf("--- PHASE 4: Cleanup - freeing remaining blocks ---\r\n");
    for (int i = 0; i < num_allocs; i++) {
        tracked_free(i);
    }
    print_heap_stats("After full cleanup (heap_4 coalesces)");

    /* Phase 5: Now the large allocation should work */
    printf("--- PHASE 5: Retry large allocation after cleanup ---\r\n");
    big_block = pvPortMalloc(target_size);
    if (big_block != NULL) {
        printf("[SUCCESS] Now allocated %u bytes at 0x%08lX\r\n",
               (unsigned)target_size, (uint32_t)big_block);
        printf("[LESSON] After freeing all, heap_4 coalesces -> no fragmentation\r\n");
        vPortFree(big_block);
    } else {
        printf("[FAIL] Still could not allocate\r\n");
    }

    print_heap_stats("Final state");
    printf("[DONE] Fragmentation demo complete\r\n\r\n");

    /* Blink LED to indicate completion */
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ---- FreeRTOS hooks ---- */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(100);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
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

    printf("\r\n\r\n--- System Boot ---\r\n");

    xTaskCreate(vFragmentationTask, "FragDemo", 512, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
