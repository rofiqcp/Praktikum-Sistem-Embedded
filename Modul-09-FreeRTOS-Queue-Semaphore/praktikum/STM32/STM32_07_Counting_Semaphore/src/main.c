/**
 * STM32_07_Counting_Semaphore
 * 3 resources (simulated LEDs), 5 worker tasks using counting semaphore.
 * Demonstrates resource pool management with counting semaphore.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xResourceSemaphore;

#define MAX_RESOURCES     3
#define NUM_WORKERS       5

static volatile uint32_t ulResourceUsers = 0;
static volatile uint32_t ulTotalAcquires[NUM_WORKERS] = {0};
static volatile uint32_t ulTotalWaits[NUM_WORKERS] = {0};
static volatile uint32_t ulMaxConcurrent = 0;

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

/* Worker Task - competes for limited resources */
static void vWorkerTask(void *pvParameters) {
    uint32_t workerID = (uint32_t)pvParameters;
    /* Different work durations per worker */
    TickType_t xWorkTime = pdMS_TO_TICKS(500 + workerID * 200);
    TickType_t xRestTime = pdMS_TO_TICKS(300 + workerID * 100);

    printf("[Worker%lu] Started (work=%lums, rest=%lums)\r\n",
           workerID, (unsigned long)(500 + workerID * 200),
           (unsigned long)(300 + workerID * 100));

    for (;;) {
        UBaseType_t available = uxSemaphoreGetCount(xResourceSemaphore);
        printf("[Worker%lu] Requesting resource (available: %u/%d)\r\n",
               workerID, (unsigned int)available, MAX_RESOURCES);

        TickType_t waitStart = xTaskGetTickCount();

        if (xSemaphoreTake(xResourceSemaphore, pdMS_TO_TICKS(5000)) == pdPASS) {
            TickType_t waitTime = xTaskGetTickCount() - waitStart;
            ulTotalAcquires[workerID]++;
            ulResourceUsers++;

            if (ulResourceUsers > ulMaxConcurrent) {
                ulMaxConcurrent = ulResourceUsers;
            }

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            printf("[Worker%lu] ACQUIRED resource (users: %lu/%d, waited: %lums, total: %lu)\r\n",
                   workerID, ulResourceUsers, MAX_RESOURCES,
                   (unsigned long)waitTime, ulTotalAcquires[workerID]);

            /* Simulate work with resource */
            vTaskDelay(xWorkTime);

            ulResourceUsers--;
            xSemaphoreGive(xResourceSemaphore);

            printf("[Worker%lu] RELEASED resource (users: %lu/%d)\r\n",
                   workerID, ulResourceUsers, MAX_RESOURCES);
        } else {
            ulTotalWaits[workerID]++;
            printf("[Worker%lu] TIMEOUT waiting for resource!\r\n", workerID);
        }

        /* Rest before trying again */
        vTaskDelay(xRestTime);
    }
}

/* Stats Task */
static void vStatsTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        printf("\r\n===== Counting Semaphore Statistics =====\r\n");
        printf("  Max resources:    %d\r\n", MAX_RESOURCES);
        printf("  Current users:    %lu\r\n", ulResourceUsers);
        printf("  Max concurrent:   %lu\r\n", ulMaxConcurrent);
        printf("  Available:        %u\r\n",
               (unsigned int)uxSemaphoreGetCount(xResourceSemaphore));

        printf("\n  Per-worker stats:\r\n");
        for (int i = 0; i < NUM_WORKERS; i++) {
            printf("    Worker%d: acquires=%lu, timeouts=%lu\r\n",
                   i, ulTotalAcquires[i], ulTotalWaits[i]);
        }
        printf("  Free heap: %u bytes\r\n", (unsigned int)xPortGetFreeHeapSize());
        printf("==========================================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n============================================\r\n");
    printf("  STM32 FreeRTOS Counting Semaphore Demo\r\n");
    printf("  %d resources, %d workers\r\n", MAX_RESOURCES, NUM_WORKERS);
    printf("============================================\r\n\r\n");

    /* Create counting semaphore: max=3, initial=3 */
    xResourceSemaphore = xSemaphoreCreateCounting(MAX_RESOURCES, MAX_RESOURCES);
    if (xResourceSemaphore == NULL) {
        printf("[ERROR] Failed to create counting semaphore!\r\n");
        while (1);
    }

    /* Create 5 worker tasks */
    char taskName[16];
    for (int i = 0; i < NUM_WORKERS; i++) {
        snprintf(taskName, sizeof(taskName), "Worker%d", i);
        xTaskCreate(vWorkerTask, taskName, 256, (void *)(uint32_t)i, 2, NULL);
    }

    xTaskCreate(vStatsTask, "Stats", 256, NULL, 1, NULL);

    printf("[INIT] All tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
