/**
 * ============================================================================
 * STM32_02_Memory_Allocation
 * ============================================================================
 * Demonstrate pvPortMalloc() / vPortFree() with heap_4 allocator.
 * Allocate arrays of different sizes, show fragmentation potential,
 * and compare sequential vs random-size allocation patterns.
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
static void SequentialAllocTask(void *pvParameters);
static void RandomAllocTask(void *pvParameters);
static void FragmentationTestTask(void *pvParameters);
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

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static void print_heap_status(const char *tag)
{
    size_t free_heap = xPortGetFreeHeapSize();
    size_t min_ever  = xPortGetMinimumEverFreeHeapSize();
    size_t used      = configTOTAL_HEAP_SIZE - free_heap;
    printf("[%s] Free=%u | Used=%u | MinEver=%u\r\n",
           tag,
           (unsigned int)free_heap,
           (unsigned int)used,
           (unsigned int)min_ever);
}

/* Simple pseudo-random based on a linear congruential generator */
static uint32_t rng_state = 12345;
static uint32_t simple_rand(void)
{
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

/* ── main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32_02: Memory Allocation Patterns\r\n");
    printf("==========================================\r\n");
    printf("Heap size: %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    print_heap_status("INIT");
    printf("------------------------------------------\r\n\r\n");

    xTaskCreate(SequentialAllocTask,    "SeqAlloc",  512, NULL, 3, NULL);
    xTaskCreate(RandomAllocTask,        "RndAlloc",  512, NULL, 2, NULL);
    xTaskCreate(FragmentationTestTask,  "FragTest",  512, NULL, 1, NULL);
    xTaskCreate(LedBlinkTask,           "LED",       128, NULL, 1, NULL);

    vTaskStartScheduler();

    for (;;) { }
}

/* ── SequentialAllocTask ─────────────────────────────────────────────────── */
static void SequentialAllocTask(void *pvParameters)
{
    (void)pvParameters;

    /* Wait a moment so output is readable */
    vTaskDelay(pdMS_TO_TICKS(500));

    printf("\r\n=== Sequential Allocation Test ===\r\n");
    print_heap_status("SEQ-START");

    /* Allocate blocks of increasing size */
    static const size_t seq_sizes[] = { 32, 64, 128, 256, 512 };
    static const int num_sizes = sizeof(seq_sizes) / sizeof(seq_sizes[0]);
    void *seq_ptrs[5] = { NULL };

    for (int i = 0; i < num_sizes; i++)
    {
        seq_ptrs[i] = pvPortMalloc(seq_sizes[i]);
        if (seq_ptrs[i] != NULL)
        {
            memset(seq_ptrs[i], (uint8_t)(0xA0 + i), seq_sizes[i]);
            printf("[SEQ] Alloc %d: %u bytes @ 0x%08lX | Free=%u\r\n",
                   i, (unsigned int)seq_sizes[i],
                   (unsigned long)(uintptr_t)seq_ptrs[i],
                   (unsigned int)xPortGetFreeHeapSize());
        }
        else
        {
            printf("[SEQ] Alloc %d: FAILED for %u bytes\r\n",
                   i, (unsigned int)seq_sizes[i]);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    print_heap_status("SEQ-ALL-ALLOC");

    /* Free in reverse order (best case for heap_4) */
    printf("[SEQ] Freeing in reverse order...\r\n");
    for (int i = num_sizes - 1; i >= 0; i--)
    {
        if (seq_ptrs[i] != NULL)
        {
            printf("[SEQ] Free %d: %u bytes @ 0x%08lX\r\n",
                   i, (unsigned int)seq_sizes[i],
                   (unsigned long)(uintptr_t)seq_ptrs[i]);
            vPortFree(seq_ptrs[i]);
            seq_ptrs[i] = NULL;
            print_heap_status("SEQ-FREE");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    printf("[SEQ] Sequential test complete.\r\n\r\n");

    /* Now allocate again and free in forward order */
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("=== Sequential Forward-Free Test ===\r\n");
    for (int i = 0; i < num_sizes; i++)
    {
        seq_ptrs[i] = pvPortMalloc(seq_sizes[i]);
        if (seq_ptrs[i] != NULL)
        {
            memset(seq_ptrs[i], (uint8_t)(0xB0 + i), seq_sizes[i]);
            printf("[SEQ-FWD] Alloc %d: %u bytes @ 0x%08lX | Free=%u\r\n",
                   i, (unsigned int)seq_sizes[i],
                   (unsigned long)(uintptr_t)seq_ptrs[i],
                   (unsigned int)xPortGetFreeHeapSize());
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    printf("[SEQ-FWD] Freeing in forward order...\r\n");
    for (int i = 0; i < num_sizes; i++)
    {
        if (seq_ptrs[i] != NULL)
        {
            printf("[SEQ-FWD] Free %d: %u bytes @ 0x%08lX\r\n",
                   i, (unsigned int)seq_sizes[i],
                   (unsigned long)(uintptr_t)seq_ptrs[i]);
            vPortFree(seq_ptrs[i]);
            seq_ptrs[i] = NULL;
            print_heap_status("SEQ-FWD");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    printf("[SEQ-FWD] Forward-free test complete.\r\n\r\n");

    /* Suspend this task – it's done */
    vTaskSuspend(NULL);
}

/* ── RandomAllocTask ─────────────────────────────────────────────────────── */
#define RAND_SLOTS 6

static void RandomAllocTask(void *pvParameters)
{
    (void)pvParameters;

    /* Let sequential test finish first */
    vTaskDelay(pdMS_TO_TICKS(15000));

    printf("\r\n=== Random Allocation Test ===\r\n");
    print_heap_status("RND-START");

    void  *rnd_ptrs[RAND_SLOTS]  = { NULL };
    size_t rnd_sizes[RAND_SLOTS] = { 0 };

    for (int round = 0; round < 5; round++)
    {
        printf("[RND] --- Round %d ---\r\n", round);

        /* Allocate random sizes into free slots */
        for (int i = 0; i < RAND_SLOTS; i++)
        {
            if (rnd_ptrs[i] == NULL)
            {
                size_t sz = 16 + (simple_rand() % 400);
                rnd_ptrs[i] = pvPortMalloc(sz);
                if (rnd_ptrs[i] != NULL)
                {
                    memset(rnd_ptrs[i], 0xCC, sz);
                    rnd_sizes[i] = sz;
                    printf("[RND] Alloc slot %d: %u bytes @ 0x%08lX | Free=%u\r\n",
                           i, (unsigned int)sz,
                           (unsigned long)(uintptr_t)rnd_ptrs[i],
                           (unsigned int)xPortGetFreeHeapSize());
                }
                else
                {
                    printf("[RND] Alloc slot %d: FAILED for %u bytes\r\n",
                           i, (unsigned int)sz);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        print_heap_status("RND-AFTER-ALLOC");

        /* Free every other slot to create fragmentation */
        for (int i = 0; i < RAND_SLOTS; i += 2)
        {
            if (rnd_ptrs[i] != NULL)
            {
                printf("[RND] Free slot %d: %u bytes @ 0x%08lX\r\n",
                       i, (unsigned int)rnd_sizes[i],
                       (unsigned long)(uintptr_t)rnd_ptrs[i]);
                vPortFree(rnd_ptrs[i]);
                rnd_ptrs[i]  = NULL;
                rnd_sizes[i] = 0;
            }
        }

        print_heap_status("RND-AFTER-FREE");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* Clean up remaining */
    printf("[RND] Cleaning remaining allocations...\r\n");
    for (int i = 0; i < RAND_SLOTS; i++)
    {
        if (rnd_ptrs[i] != NULL)
        {
            vPortFree(rnd_ptrs[i]);
            rnd_ptrs[i] = NULL;
        }
    }
    print_heap_status("RND-CLEANUP");
    printf("[RND] Random test complete.\r\n\r\n");

    vTaskSuspend(NULL);
}

/* ── FragmentationTestTask ───────────────────────────────────────────────── */
#define FRAG_SLOTS 10

static void FragmentationTestTask(void *pvParameters)
{
    (void)pvParameters;

    /* Wait for other tests */
    vTaskDelay(pdMS_TO_TICKS(35000));

    printf("\r\n=== Fragmentation Analysis ===\r\n");
    print_heap_status("FRAG-START");

    void  *frag_ptrs[FRAG_SLOTS]  = { NULL };
    size_t frag_sizes[FRAG_SLOTS] = { 0 };

    /* Step 1: Allocate 10 blocks of 256 bytes */
    printf("[FRAG] Step 1: Allocating %d x 256 bytes...\r\n", FRAG_SLOTS);
    for (int i = 0; i < FRAG_SLOTS; i++)
    {
        frag_ptrs[i] = pvPortMalloc(256);
        frag_sizes[i] = 256;
        if (frag_ptrs[i] != NULL)
        {
            memset(frag_ptrs[i], (uint8_t)i, 256);
            printf("[FRAG] Block %d @ 0x%08lX | Free=%u\r\n",
                   i, (unsigned long)(uintptr_t)frag_ptrs[i],
                   (unsigned int)xPortGetFreeHeapSize());
        }
        else
        {
            printf("[FRAG] Block %d FAILED\r\n", i);
        }
    }

    print_heap_status("FRAG-ALLOC-ALL");
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Step 2: Free odd-indexed blocks to create gaps */
    printf("[FRAG] Step 2: Freeing odd-indexed blocks (fragmentation)...\r\n");
    for (int i = 1; i < FRAG_SLOTS; i += 2)
    {
        if (frag_ptrs[i] != NULL)
        {
            printf("[FRAG] Free block %d @ 0x%08lX\r\n",
                   i, (unsigned long)(uintptr_t)frag_ptrs[i]);
            vPortFree(frag_ptrs[i]);
            frag_ptrs[i] = NULL;
            frag_sizes[i] = 0;
        }
    }

    print_heap_status("FRAG-AFTER-ODD-FREE");
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Step 3: Try to allocate a large block that should fail due to fragmentation */
    size_t large_size = 1024;
    printf("[FRAG] Step 3: Trying large alloc of %u bytes...\r\n",
           (unsigned int)large_size);
    void *large_ptr = pvPortMalloc(large_size);
    if (large_ptr != NULL)
    {
        printf("[FRAG] Large alloc SUCCESS @ 0x%08lX (heap_4 coalesced!)\r\n",
               (unsigned long)(uintptr_t)large_ptr);
        vPortFree(large_ptr);
    }
    else
    {
        printf("[FRAG] Large alloc FAILED - fragmentation prevents it\r\n");
    }

    print_heap_status("FRAG-AFTER-LARGE");
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Step 4: Free even-indexed blocks */
    printf("[FRAG] Step 4: Freeing remaining even-indexed blocks...\r\n");
    for (int i = 0; i < FRAG_SLOTS; i += 2)
    {
        if (frag_ptrs[i] != NULL)
        {
            vPortFree(frag_ptrs[i]);
            frag_ptrs[i] = NULL;
        }
    }

    print_heap_status("FRAG-ALL-FREE");

    /* Step 5: Try large alloc again after full cleanup */
    printf("[FRAG] Step 5: Retrying large alloc after full cleanup...\r\n");
    large_ptr = pvPortMalloc(large_size);
    if (large_ptr != NULL)
    {
        printf("[FRAG] Large alloc SUCCESS @ 0x%08lX\r\n",
               (unsigned long)(uintptr_t)large_ptr);
        vPortFree(large_ptr);
    }
    else
    {
        printf("[FRAG] Large alloc FAILED even after cleanup\r\n");
    }

    print_heap_status("FRAG-FINAL");
    printf("[FRAG] Fragmentation analysis complete.\r\n\r\n");

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
