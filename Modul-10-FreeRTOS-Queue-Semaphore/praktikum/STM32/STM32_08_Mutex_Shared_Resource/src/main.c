/**
 * STM32_08_Mutex_Shared_Resource
 * Compare unsafe vs safe counter increment with mutex.
 * Phase 1: Without mutex (race condition), Phase 2: With mutex (correct).
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xCounterMutex;

#define INCREMENTS_PER_TASK  1000
#define NUM_TASKS            2

static volatile uint32_t ulSharedCounter = 0;
static volatile uint8_t ucPhase = 0;  /* 0=unsafe, 1=safe */
static volatile uint8_t ucTasksDone = 0;
static uint32_t ulUnsafeResult = 0;
static uint32_t ulSafeResult = 0;

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

/* Unsafe increment task - no mutex protection */
static void vUnsafeTask(void *pvParameters) {
    uint32_t taskID = (uint32_t)pvParameters;

    printf("[Unsafe%lu] Starting %d increments WITHOUT mutex\r\n",
           taskID, INCREMENTS_PER_TASK);

    for (int i = 0; i < INCREMENTS_PER_TASK; i++) {
        /* Deliberate race condition: read-modify-write without protection */
        uint32_t temp = ulSharedCounter;
        /* Small delay to increase chance of race condition */
        for (volatile int j = 0; j < 10; j++);
        ulSharedCounter = temp + 1;

        if (i % 200 == 0) {
            printf("[Unsafe%lu] Progress: %d/%d (counter=%lu)\r\n",
                   taskID, i, INCREMENTS_PER_TASK, ulSharedCounter);
        }
    }

    printf("[Unsafe%lu] DONE\r\n", taskID);
    ucTasksDone++;

    /* Wait for both tasks to finish */
    if (ucTasksDone >= NUM_TASKS) {
        ulUnsafeResult = ulSharedCounter;
        printf("\r\n[PHASE1] UNSAFE RESULT: %lu (expected: %d)\r\n",
               ulUnsafeResult, INCREMENTS_PER_TASK * NUM_TASKS);
        if (ulUnsafeResult != INCREMENTS_PER_TASK * NUM_TASKS) {
            printf("[PHASE1] RACE CONDITION DETECTED! Lost %ld increments\r\n",
                   (long)(INCREMENTS_PER_TASK * NUM_TASKS - ulUnsafeResult));
        }

        /* Reset for Phase 2 */
        ulSharedCounter = 0;
        ucTasksDone = 0;
        ucPhase = 1;
        printf("\r\n--- Starting Phase 2: WITH Mutex ---\r\n\r\n");
    }

    vTaskDelete(NULL);
}

/* Safe increment task - mutex protected */
static void vSafeTask(void *pvParameters) {
    uint32_t taskID = (uint32_t)pvParameters;

    /* Wait for Phase 2 */
    while (ucPhase == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("[Safe%lu] Starting %d increments WITH mutex\r\n",
           taskID, INCREMENTS_PER_TASK);

    for (int i = 0; i < INCREMENTS_PER_TASK; i++) {
        xSemaphoreTake(xCounterMutex, portMAX_DELAY);
        {
            uint32_t temp = ulSharedCounter;
            for (volatile int j = 0; j < 10; j++);
            ulSharedCounter = temp + 1;
        }
        xSemaphoreGive(xCounterMutex);

        if (i % 200 == 0) {
            printf("[Safe%lu] Progress: %d/%d (counter=%lu)\r\n",
                   taskID, i, INCREMENTS_PER_TASK, ulSharedCounter);
        }
    }

    printf("[Safe%lu] DONE\r\n", taskID);
    ucTasksDone++;

    if (ucTasksDone >= NUM_TASKS) {
        ulSafeResult = ulSharedCounter;
        printf("\r\n[PHASE2] SAFE RESULT: %lu (expected: %d)\r\n",
               ulSafeResult, INCREMENTS_PER_TASK * NUM_TASKS);

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        printf("\r\n========== COMPARISON ==========\r\n");
        printf("  Expected:        %d\r\n", INCREMENTS_PER_TASK * NUM_TASKS);
        printf("  Unsafe result:   %lu (%s)\r\n", ulUnsafeResult,
               ulUnsafeResult == INCREMENTS_PER_TASK * NUM_TASKS ? "CORRECT" : "RACE CONDITION!");
        printf("  Safe result:     %lu (%s)\r\n", ulSafeResult,
               ulSafeResult == INCREMENTS_PER_TASK * NUM_TASKS ? "CORRECT" : "ERROR!");
        printf("  Lost (unsafe):   %ld\r\n",
               (long)(INCREMENTS_PER_TASK * NUM_TASKS - ulUnsafeResult));
        printf("  Lost (safe):     %ld\r\n",
               (long)(INCREMENTS_PER_TASK * NUM_TASKS - ulSafeResult));
        printf("================================\r\n");
    }

    vTaskDelete(NULL);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n================================================\r\n");
    printf("  STM32 FreeRTOS Mutex Shared Resource Demo\r\n");
    printf("  Phase 1: Without mutex (race condition)\r\n");
    printf("  Phase 2: With mutex (protected)\r\n");
    printf("  %d tasks x %d increments = %d expected\r\n",
           NUM_TASKS, INCREMENTS_PER_TASK, NUM_TASKS * INCREMENTS_PER_TASK);
    printf("================================================\r\n\r\n");

    xCounterMutex = xSemaphoreCreateMutex();
    if (xCounterMutex == NULL) {
        printf("[ERROR] Failed to create mutex!\r\n");
        while (1);
    }

    printf("--- Starting Phase 1: WITHOUT Mutex ---\r\n\r\n");

    /* Phase 1: Unsafe tasks */
    xTaskCreate(vUnsafeTask, "Unsafe0", 256, (void *)0, 2, NULL);
    xTaskCreate(vUnsafeTask, "Unsafe1", 256, (void *)1, 2, NULL);

    /* Phase 2: Safe tasks (will wait for phase flag) */
    xTaskCreate(vSafeTask, "Safe0", 256, (void *)0, 2, NULL);
    xTaskCreate(vSafeTask, "Safe1", 256, (void *)1, 2, NULL);

    vTaskStartScheduler();

    while (1);
}
