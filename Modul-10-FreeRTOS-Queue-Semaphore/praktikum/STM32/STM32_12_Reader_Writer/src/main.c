/**
 * STM32_12_Reader_Writer
 * Multiple readers + single writer using reader-writer lock.
 * Implemented with mutex + reader count. 4 readers, 2 writers.
 * Readers can share access, writers need exclusive access.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;

#define NUM_READERS  4
#define NUM_WRITERS  2

/* Reader-Writer Lock implementation */
static SemaphoreHandle_t xResourceMutex;     /* Exclusive access to shared resource */
static SemaphoreHandle_t xReaderCountMutex;  /* Protects reader_count */
static volatile int32_t  slReaderCount = 0;  /* Number of active readers */

/* Shared data (the resource) */
typedef struct {
    uint32_t value;
    uint32_t version;
    uint32_t last_writer;
    uint32_t timestamp;
} SharedData_t;

static volatile SharedData_t sharedData = {0, 0, 0, 0};

/* Statistics */
static volatile uint32_t ulReadOps[NUM_READERS] = {0};
static volatile uint32_t ulWriteOps[NUM_WRITERS] = {0};
static volatile uint32_t ulMaxConcurrentReaders = 0;
static volatile uint32_t ulWriterWaits = 0;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

static void LED_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void vApplicationMallocFailedHook(void) {
    printf("[ERROR] Malloc failed!\r\n");
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1);
}

/* Reader-Writer Lock: Start Reading */
static void rwlock_read_lock(void) {
    xSemaphoreTake(xReaderCountMutex, portMAX_DELAY);
    slReaderCount++;
    if (slReaderCount == 1) {
        /* First reader locks the resource */
        xSemaphoreTake(xResourceMutex, portMAX_DELAY);
    }
    if ((uint32_t)slReaderCount > ulMaxConcurrentReaders) {
        ulMaxConcurrentReaders = (uint32_t)slReaderCount;
    }
    xSemaphoreGive(xReaderCountMutex);
}

/* Reader-Writer Lock: Stop Reading */
static void rwlock_read_unlock(void) {
    xSemaphoreTake(xReaderCountMutex, portMAX_DELAY);
    slReaderCount--;
    if (slReaderCount == 0) {
        /* Last reader unlocks the resource */
        xSemaphoreGive(xResourceMutex);
    }
    xSemaphoreGive(xReaderCountMutex);
}

/* Reader-Writer Lock: Start Writing */
static void rwlock_write_lock(void) {
    xSemaphoreTake(xResourceMutex, portMAX_DELAY);
}

/* Reader-Writer Lock: Stop Writing */
static void rwlock_write_unlock(void) {
    xSemaphoreGive(xResourceMutex);
}

/* Reader Task */
static void vReaderTask(void *pvParameters) {
    uint32_t id = (uint32_t)pvParameters;
    TickType_t xDelay = pdMS_TO_TICKS(200 + id * 100);

    printf("[Reader%lu] Started (period: %lums)\r\n", id, (unsigned long)(200 + id * 100));

    for (;;) {
        rwlock_read_lock();
        {
            ulReadOps[id]++;

            /* Read shared data (multiple readers can be here simultaneously) */
            uint32_t val = sharedData.value;
            uint32_t ver = sharedData.version;
            uint32_t writer = sharedData.last_writer;
            int32_t concurrent = slReaderCount;

            printf("[Reader%lu] Read: val=%lu ver=%lu writer=W%lu (concurrent_readers=%ld, total=%lu)\r\n",
                   id, val, ver, writer, (long)concurrent, ulReadOps[id]);

            /* Simulate read time */
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        rwlock_read_unlock();

        vTaskDelay(xDelay);
    }
}

/* Writer Task */
static void vWriterTask(void *pvParameters) {
    uint32_t id = (uint32_t)pvParameters;
    TickType_t xDelay = pdMS_TO_TICKS(1000 + id * 500);

    printf("[Writer%lu] Started (period: %lums)\r\n", id, (unsigned long)(1000 + id * 500));

    for (;;) {
        printf("[Writer%lu] Requesting exclusive access...\r\n", id);
        TickType_t waitStart = xTaskGetTickCount();

        rwlock_write_lock();
        {
            TickType_t waitTime = xTaskGetTickCount() - waitStart;
            if (waitTime > 10) ulWriterWaits++;

            ulWriteOps[id]++;

            /* Modify shared data (exclusive access) */
            sharedData.value += (id + 1) * 10;
            sharedData.version++;
            sharedData.last_writer = id;
            sharedData.timestamp = xTaskGetTickCount();

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            printf("[Writer%lu] WROTE: val=%lu ver=%lu (waited=%lums, total=%lu)\r\n",
                   id, sharedData.value, sharedData.version,
                   (unsigned long)waitTime, ulWriteOps[id]);

            /* Simulate write time */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        rwlock_write_unlock();

        printf("[Writer%lu] Released exclusive access\r\n", id);

        vTaskDelay(xDelay);
    }
}

/* Stats Task */
static void vStatsTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        uint32_t totalReads = 0, totalWrites = 0;
        for (int i = 0; i < NUM_READERS; i++) totalReads += ulReadOps[i];
        for (int i = 0; i < NUM_WRITERS; i++) totalWrites += ulWriteOps[i];

        printf("\r\n===== Reader-Writer Statistics =====\r\n");
        printf("  Current readers:       %ld\r\n", (long)slReaderCount);
        printf("  Max concurrent readers:%lu\r\n", ulMaxConcurrentReaders);
        printf("  Writer waits:          %lu\r\n", ulWriterWaits);
        printf("  Data version:          %lu\r\n", sharedData.version);
        printf("  Data value:            %lu\r\n", sharedData.value);

        printf("\n  Per-reader ops:\r\n");
        for (int i = 0; i < NUM_READERS; i++) {
            printf("    Reader%d: %lu\r\n", i, ulReadOps[i]);
        }

        printf("  Per-writer ops:\r\n");
        for (int i = 0; i < NUM_WRITERS; i++) {
            printf("    Writer%d: %lu\r\n", i, ulWriteOps[i]);
        }

        printf("\n  Total reads:  %lu\r\n", totalReads);
        printf("  Total writes: %lu\r\n", totalWrites);
        if (totalWrites > 0) {
            printf("  Read/Write ratio: %.1f:1\r\n", (float)totalReads / totalWrites);
        }
        printf("  Free heap: %u bytes\r\n", (unsigned int)xPortGetFreeHeapSize());
        printf("====================================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32 FreeRTOS Reader-Writer Demo\r\n");
    printf("  %d readers, %d writers\r\n", NUM_READERS, NUM_WRITERS);
    printf("==========================================\r\n\r\n");

    /* Create reader-writer lock primitives */
    xResourceMutex = xSemaphoreCreateMutex();
    xReaderCountMutex = xSemaphoreCreateMutex();

    if (xResourceMutex == NULL || xReaderCountMutex == NULL) {
        printf("[ERROR] Failed to create mutexes!\r\n");
        while (1);
    }

    /* Create reader tasks */
    char name[16];
    for (int i = 0; i < NUM_READERS; i++) {
        snprintf(name, sizeof(name), "Reader%d", i);
        xTaskCreate(vReaderTask, name, 256, (void *)(uint32_t)i, 2, NULL);
    }

    /* Create writer tasks */
    for (int i = 0; i < NUM_WRITERS; i++) {
        snprintf(name, sizeof(name), "Writer%d", i);
        xTaskCreate(vWriterTask, name, 256, (void *)(uint32_t)i, 3, NULL);
    }

    xTaskCreate(vStatsTask, "Stats", 256, NULL, 1, NULL);

    printf("[INIT] All tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
