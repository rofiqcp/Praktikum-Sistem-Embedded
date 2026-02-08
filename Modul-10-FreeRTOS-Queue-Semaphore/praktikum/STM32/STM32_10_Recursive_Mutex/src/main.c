/**
 * STM32_10_Recursive_Mutex
 * Recursive mutex allowing nested locking from the same task.
 * Demonstrates safe nested resource access patterns.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xRecursiveMutex;

static volatile uint32_t ulSharedResource = 0;
static volatile uint32_t ulNestingDepthMax = 0;
static volatile uint32_t ulTotalOperations = 0;

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

/* Low-level function that also needs the mutex (nested lock) */
static void low_level_access(const char *caller, uint32_t depth) {
    printf("[%s] Depth %lu: Taking recursive mutex (nested)...\r\n", caller, depth);

    if (xSemaphoreTakeRecursive(xRecursiveMutex, pdMS_TO_TICKS(1000)) == pdPASS) {
        if (depth > ulNestingDepthMax) {
            ulNestingDepthMax = depth;
        }

        ulSharedResource += 10;
        printf("[%s] Depth %lu: Resource = %lu (nested lock held)\r\n",
               caller, depth, ulSharedResource);

        vTaskDelay(pdMS_TO_TICKS(50));

        xSemaphoreGiveRecursive(xRecursiveMutex);
        printf("[%s] Depth %lu: Nested mutex released\r\n", caller, depth);
    } else {
        printf("[%s] Depth %lu: FAILED to take recursive mutex!\r\n", caller, depth);
    }
}

/* Mid-level function that also needs the mutex (second level) */
static void mid_level_access(const char *caller, uint32_t depth) {
    printf("[%s] Depth %lu: Taking recursive mutex (mid)...\r\n", caller, depth);

    if (xSemaphoreTakeRecursive(xRecursiveMutex, pdMS_TO_TICKS(1000)) == pdPASS) {
        ulSharedResource += 5;
        printf("[%s] Depth %lu: Resource = %lu (mid lock held)\r\n",
               caller, depth, ulSharedResource);

        /* Call lower level - this will try to take the mutex again */
        low_level_access(caller, depth + 1);

        xSemaphoreGiveRecursive(xRecursiveMutex);
        printf("[%s] Depth %lu: Mid mutex released\r\n", caller, depth);
    }
}

/* Task A - takes mutex at multiple nesting levels */
static void vTaskA(void *pvParameters) {
    printf("[TaskA] Started - demonstrates nested recursive mutex\r\n");

    for (;;) {
        ulTotalOperations++;
        printf("\r\n[TaskA] === Operation #%lu - Taking mutex (top level) ===\r\n",
               ulTotalOperations);

        if (xSemaphoreTakeRecursive(xRecursiveMutex, pdMS_TO_TICKS(2000)) == pdPASS) {
            ulSharedResource += 1;
            printf("[TaskA] Depth 0: Resource = %lu (top lock held)\r\n", ulSharedResource);

            /* Call mid-level which calls low-level - 3 levels of nesting */
            mid_level_access("TaskA", 1);

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            xSemaphoreGiveRecursive(xRecursiveMutex);
            printf("[TaskA] Depth 0: Top mutex released. Resource final = %lu\r\n",
                   ulSharedResource);
        } else {
            printf("[TaskA] TIMEOUT at top level!\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* Task B - also uses the same recursive mutex */
static void vTaskB(void *pvParameters) {
    printf("[TaskB] Started - competes for same recursive mutex\r\n");

    for (;;) {
        printf("\r\n[TaskB] Attempting to take mutex...\r\n");

        if (xSemaphoreTakeRecursive(xRecursiveMutex, pdMS_TO_TICKS(3000)) == pdPASS) {
            ulSharedResource += 100;
            printf("[TaskB] GOT mutex. Resource = %lu\r\n", ulSharedResource);

            /* Single-level nesting */
            low_level_access("TaskB", 1);

            xSemaphoreGiveRecursive(xRecursiveMutex);
            printf("[TaskB] Mutex released. Resource = %lu\r\n", ulSharedResource);
        } else {
            printf("[TaskB] TIMEOUT waiting for mutex!\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* Stats Task */
static void vStatsTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        printf("\r\n===== Recursive Mutex Statistics =====\r\n");
        printf("  Total operations:  %lu\r\n", ulTotalOperations);
        printf("  Max nesting depth: %lu\r\n", ulNestingDepthMax);
        printf("  Shared resource:   %lu\r\n", ulSharedResource);
        printf("  Free heap:         %u bytes\r\n",
               (unsigned int)xPortGetFreeHeapSize());
        printf("======================================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32 FreeRTOS Recursive Mutex Demo\r\n");
    printf("  Nested locking from same task\r\n");
    printf("==========================================\r\n\r\n");

    xRecursiveMutex = xSemaphoreCreateRecursiveMutex();
    if (xRecursiveMutex == NULL) {
        printf("[ERROR] Failed to create recursive mutex!\r\n");
        while (1);
    }

    xTaskCreate(vTaskA, "TaskA", 512, NULL, 2, NULL);
    xTaskCreate(vTaskB, "TaskB", 512, NULL, 2, NULL);
    xTaskCreate(vStatsTask, "Stats", 256, NULL, 1, NULL);

    printf("[INIT] Tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
