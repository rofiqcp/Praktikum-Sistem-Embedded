/**
 * STM32_11_Memory_Leak_Detection
 * 
 * Implements heap tracing by wrapping pvPortMalloc/vPortFree with logging.
 * - Tracks every allocation and free with address, size, and task name
 * - Deliberately leaks memory from one task (allocate without free)
 * - Another task detects leak by checking heap decreases over time
 * - Prints leak report showing unfreed allocations
 * 
 * Platform: STM32F103C8 BluePill
 * UART1: PA9(TX), PA10(RX) @ 115200
 * LED: PC13 (active low)
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
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
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/* ---- UART1 Init ---- */
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

/* ---- Memory Allocation Tracker ---- */
#define MAX_TRACKED_ALLOCS  50

typedef struct {
    void     *addr;
    size_t    size;
    char      task_name[configMAX_TASK_NAME_LEN];
    uint32_t  tick;
    uint8_t   active;     /* 1=allocated, 0=freed */
    uint32_t  alloc_id;
} AllocRecord_t;

static AllocRecord_t alloc_records[MAX_TRACKED_ALLOCS];
static volatile int  alloc_record_count = 0;
static volatile uint32_t alloc_id_counter = 0;
static SemaphoreHandle_t tracker_mutex = NULL;

/* Tracked malloc wrapper */
static void *tracked_malloc(size_t size)
{
    void *ptr = pvPortMalloc(size);
    if (ptr == NULL) {
        return NULL;
    }

    if (xSemaphoreTake(tracker_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (alloc_record_count < MAX_TRACKED_ALLOCS) {
            AllocRecord_t *rec = &alloc_records[alloc_record_count];
            rec->addr     = ptr;
            rec->size     = size;
            rec->tick     = xTaskGetTickCount();
            rec->active   = 1;
            rec->alloc_id = alloc_id_counter++;

            TaskHandle_t current = xTaskGetCurrentTaskHandle();
            if (current != NULL) {
                strncpy(rec->task_name, pcTaskGetName(current), configMAX_TASK_NAME_LEN - 1);
                rec->task_name[configMAX_TASK_NAME_LEN - 1] = '\0';
            } else {
                strncpy(rec->task_name, "unknown", configMAX_TASK_NAME_LEN - 1);
            }

            printf("[TRACK_ALLOC] id=%lu addr=0x%08lX size=%u task=%s tick=%lu\r\n",
                   rec->alloc_id, (uint32_t)ptr, (unsigned)size,
                   rec->task_name, rec->tick);

            alloc_record_count++;
        }
        xSemaphoreGive(tracker_mutex);
    }

    return ptr;
}

/* Tracked free wrapper */
static void tracked_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    if (xSemaphoreTake(tracker_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        int found = 0;
        for (int i = 0; i < alloc_record_count; i++) {
            if (alloc_records[i].addr == ptr && alloc_records[i].active) {
                printf("[TRACK_FREE] id=%lu addr=0x%08lX size=%u task=%s\r\n",
                       alloc_records[i].alloc_id, (uint32_t)ptr,
                       (unsigned)alloc_records[i].size,
                       alloc_records[i].task_name);
                alloc_records[i].active = 0;
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("[TRACK_FREE_UNKNOWN] addr=0x%08lX (not in tracker)\r\n",
                   (uint32_t)ptr);
        }
        xSemaphoreGive(tracker_mutex);
    }

    vPortFree(ptr);
}

/* Print leak report */
static void print_leak_report(void)
{
    if (xSemaphoreTake(tracker_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
        return;
    }

    int leak_count = 0;
    size_t leak_bytes = 0;

    printf("\r\n[LEAK_REPORT] ==============================\r\n");
    printf("[LEAK_REPORT] Memory Leak Detection Report\r\n");
    printf("[LEAK_REPORT] Total tracked allocations: %d\r\n", alloc_record_count);

    for (int i = 0; i < alloc_record_count; i++) {
        if (alloc_records[i].active) {
            leak_count++;
            leak_bytes += alloc_records[i].size;
            printf("[LEAK] id=%lu addr=0x%08lX size=%u task=%s age=%lu ms\r\n",
                   alloc_records[i].alloc_id,
                   (uint32_t)alloc_records[i].addr,
                   (unsigned)alloc_records[i].size,
                   alloc_records[i].task_name,
                   (xTaskGetTickCount() - alloc_records[i].tick));
        }
    }

    printf("[LEAK_REPORT] Active (unfreed) allocations: %d\r\n", leak_count);
    printf("[LEAK_REPORT] Total leaked bytes: %u\r\n", (unsigned)leak_bytes);

    /* Count freed */
    int freed_count = 0;
    for (int i = 0; i < alloc_record_count; i++) {
        if (!alloc_records[i].active) {
            freed_count++;
        }
    }
    printf("[LEAK_REPORT] Properly freed allocations: %d\r\n", freed_count);

    if (leak_count == 0) {
        printf("[LEAK_REPORT] STATUS: NO LEAKS DETECTED\r\n");
    } else {
        printf("[LEAK_REPORT] STATUS: LEAKS DETECTED!\r\n");
    }
    printf("[LEAK_REPORT] ==============================\r\n\r\n");

    xSemaphoreGive(tracker_mutex);
}

/* ---- Good Task: allocates and properly frees ---- */
static void vGoodTask(void *pvParameters)
{
    (void)pvParameters;
    int cycle = 0;

    while (1) {
        cycle++;
        printf("[GOOD_TASK] Cycle %d: allocating...\r\n", cycle);

        /* Allocate various buffers */
        void *buf1 = tracked_malloc(64);
        void *buf2 = tracked_malloc(32);
        void *buf3 = tracked_malloc(128);

        if (buf1 && buf2 && buf3) {
            /* Use the buffers */
            memset(buf1, 0xAA, 64);
            memset(buf2, 0xBB, 32);
            memset(buf3, 0xCC, 128);
            printf("[GOOD_TASK] Using 3 buffers (%d + %d + %d = %d bytes)\r\n",
                   64, 32, 128, 64 + 32 + 128);
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Properly free all buffers */
        tracked_free(buf1);
        tracked_free(buf2);
        tracked_free(buf3);
        printf("[GOOD_TASK] Cycle %d: all freed properly\r\n\r\n", cycle);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ---- Leaky Task: allocates but sometimes forgets to free ---- */
static void vLeakyTask(void *pvParameters)
{
    (void)pvParameters;
    int cycle = 0;

    while (1) {
        cycle++;
        printf("[LEAKY_TASK] Cycle %d: allocating...\r\n", cycle);

        /* Allocate a buffer */
        void *buf = tracked_malloc(48);
        if (buf) {
            memset(buf, 0xDD, 48);
            printf("[LEAKY_TASK] Allocated 48 bytes at 0x%08lX\r\n", (uint32_t)buf);
        }

        /* Deliberately LEAK: do NOT free buf! */
        printf("[LEAKY_TASK] Cycle %d: 'forgetting' to free! (LEAK)\r\n\r\n", cycle);

        vTaskDelay(pdMS_TO_TICKS(3000));

        /* Stop leaking after 5 cycles to avoid running out */
        if (cycle >= 5) {
            printf("[LEAKY_TASK] Stopping after %d leak cycles\r\n", cycle);
            printf("[LEAKY_TASK] Total leaked: ~%d bytes\r\n\r\n", cycle * 48);
            vTaskDelay(portMAX_DELAY);
        }
    }
}

/* ---- Detector Task: monitors heap and generates reports ---- */
static void vDetectorTask(void *pvParameters)
{
    (void)pvParameters;
    int report_cycle = 0;
    size_t prev_free = 0;

    /* Wait for initial setup */
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("\r\n================================================\r\n");
    printf("  STM32_11: Memory Leak Detection\r\n");
    printf("  Heap: %u bytes, Tracker: %d slots\r\n",
           (unsigned)configTOTAL_HEAP_SIZE, MAX_TRACKED_ALLOCS);
    printf("  GoodTask: alloc+free properly\r\n");
    printf("  LeakyTask: alloc without free!\r\n");
    printf("================================================\r\n\r\n");

    while (1) {
        report_cycle++;

        HeapStats_t stats;
        vPortGetHeapStats(&stats);
        size_t current_free = stats.xAvailableHeapSpaceInBytes;

        printf("[DETECTOR] === Report #%d ===\r\n", report_cycle);
        printf("[DETECTOR] Heap free     : %u bytes\r\n", (unsigned)current_free);
        printf("[DETECTOR] Min ever free : %u bytes\r\n",
               (unsigned)stats.xMinimumEverFreeBytesRemaining);
        printf("[DETECTOR] Largest block : %u bytes\r\n",
               (unsigned)stats.xSizeOfLargestFreeBlockInBytes);
        printf("[DETECTOR] Alloc calls   : %u\r\n",
               (unsigned)stats.xNumberOfSuccessfulAllocations);
        printf("[DETECTOR] Free calls    : %u\r\n",
               (unsigned)stats.xNumberOfSuccessfulFrees);

        /* Detect leak: alloc count > free count over time */
        int32_t unmatched = (int32_t)stats.xNumberOfSuccessfulAllocations -
                            (int32_t)stats.xNumberOfSuccessfulFrees;
        printf("[DETECTOR] Unmatched allocs: %ld\r\n", (long)unmatched);

        if (prev_free > 0 && current_free < prev_free) {
            size_t lost = prev_free - current_free;
            printf("[DETECTOR] WARNING: Heap decreased by %u bytes since last check!\r\n",
                   (unsigned)lost);
            printf("[DETECTOR] Possible memory leak detected!\r\n");
        } else if (prev_free > 0 && current_free == prev_free) {
            printf("[DETECTOR] Heap stable (no change)\r\n");
        }
        prev_free = current_free;

        /* Print full leak report every 3 cycles */
        if (report_cycle % 3 == 0) {
            print_leak_report();
        }
        printf("\r\n");

        /* Blink LED */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ---- FreeRTOS hooks ---- */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    print_leak_report();
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

    tracker_mutex = xSemaphoreCreateMutex();
    if (tracker_mutex == NULL) {
        printf("[ERROR] Failed to create tracker mutex\r\n");
        while (1);
    }

    xTaskCreate(vDetectorTask, "Detector", 512, NULL, 3, NULL);
    xTaskCreate(vGoodTask,     "GoodTask", 256, NULL, 2, NULL);
    xTaskCreate(vLeakyTask,    "LeakyTask", 256, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}
