/**
 * STM32_09_Mutex_Priority_Inversion
 * 3 tasks at different priorities demonstrating priority inheritance.
 * Low priority holds mutex, high priority needs it, medium runs.
 * FreeRTOS mutex provides priority inheritance to avoid unbounded inversion.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;
static SemaphoreHandle_t xSharedMutex;

static TaskHandle_t xHighTask, xMedTask, xLowTask;
static volatile uint32_t ulHighRunCount = 0;
static volatile uint32_t ulMedRunCount = 0;
static volatile uint32_t ulLowRunCount = 0;
static volatile uint32_t ulCycleCount = 0;

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

/* High Priority Task (priority 4) - needs the mutex */
static void vHighPriorityTask(void *pvParameters) {
    TickType_t xStartWait;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(500));

        printf("[HIGH  ] (pri=%lu) Trying to take mutex...\r\n",
               (unsigned long)uxTaskPriorityGet(NULL));
        xStartWait = xTaskGetTickCount();

        if (xSemaphoreTake(xSharedMutex, pdMS_TO_TICKS(5000)) == pdPASS) {
            TickType_t waitTime = xTaskGetTickCount() - xStartWait;
            ulHighRunCount++;

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            printf("[HIGH  ] GOT mutex after %lums (pri=%lu, run#%lu)\r\n",
                   (unsigned long)waitTime,
                   (unsigned long)uxTaskPriorityGet(NULL),
                   ulHighRunCount);

            /* Brief critical section */
            vTaskDelay(pdMS_TO_TICKS(100));

            xSemaphoreGive(xSharedMutex);
            printf("[HIGH  ] Released mutex\r\n");
        } else {
            printf("[HIGH  ] TIMEOUT waiting for mutex!\r\n");
        }
    }
}

/* Medium Priority Task (priority 3) - does NOT use mutex, just CPU work */
static void vMediumPriorityTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));

        ulMedRunCount++;
        printf("[MEDIUM] (pri=%lu) Running computation... (run#%lu)\r\n",
               (unsigned long)uxTaskPriorityGet(NULL), ulMedRunCount);

        /* Simulate CPU-intensive work */
        volatile uint32_t dummy = 0;
        for (volatile uint32_t i = 0; i < 100000; i++) {
            dummy += i;
        }

        printf("[MEDIUM] Computation done\r\n");
    }
}

/* Low Priority Task (priority 1) - holds the mutex for a long time */
static void vLowPriorityTask(void *pvParameters) {
    for (;;) {
        printf("[LOW   ] (pri=%lu) Taking mutex...\r\n",
               (unsigned long)uxTaskPriorityGet(NULL));

        if (xSemaphoreTake(xSharedMutex, portMAX_DELAY) == pdPASS) {
            ulLowRunCount++;
            ulCycleCount++;

            printf("[LOW   ] GOT mutex (pri=%lu, run#%lu)\r\n",
                   (unsigned long)uxTaskPriorityGet(NULL), ulLowRunCount);

            /* Hold mutex for a long time - simulating long critical section */
            printf("[LOW   ] Holding mutex for 1000ms (long operation)...\r\n");

            /* During this time, check if priority inheritance is happening */
            for (int i = 0; i < 10; i++) {
                vTaskDelay(pdMS_TO_TICKS(100));
                UBaseType_t currentPri = uxTaskPriorityGet(NULL);
                if (currentPri > 1) {
                    printf("[LOW   ] !!! Priority INHERITED: %lu -> %lu !!!\r\n",
                           1UL, (unsigned long)currentPri);
                }
            }

            printf("[LOW   ] Releasing mutex (pri=%lu)\r\n",
                   (unsigned long)uxTaskPriorityGet(NULL));
            xSemaphoreGive(xSharedMutex);

            printf("[LOW   ] Mutex released (pri=%lu, back to base)\r\n",
                   (unsigned long)uxTaskPriorityGet(NULL));

            /* Print cycle stats */
            if (ulCycleCount % 3 == 0) {
                printf("\r\n===== Priority Inversion Stats =====\r\n");
                printf("  High task runs:   %lu\r\n", ulHighRunCount);
                printf("  Medium task runs: %lu\r\n", ulMedRunCount);
                printf("  Low task runs:    %lu\r\n", ulLowRunCount);
                printf("  Cycles:           %lu\r\n", ulCycleCount);
                printf("  Free heap:        %u bytes\r\n",
                       (unsigned int)xPortGetFreeHeapSize());
                printf("====================================\r\n\r\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n=============================================\r\n");
    printf("  STM32 FreeRTOS Priority Inversion Demo\r\n");
    printf("  High(4) + Medium(3) + Low(1) + Mutex\r\n");
    printf("  FreeRTOS provides priority inheritance\r\n");
    printf("=============================================\r\n\r\n");

    xSharedMutex = xSemaphoreCreateMutex();
    if (xSharedMutex == NULL) {
        printf("[ERROR] Failed to create mutex!\r\n");
        while (1);
    }

    xTaskCreate(vLowPriorityTask, "Low", 256, NULL, 1, &xLowTask);
    xTaskCreate(vMediumPriorityTask, "Medium", 256, NULL, 3, &xMedTask);
    xTaskCreate(vHighPriorityTask, "High", 256, NULL, 4, &xHighTask);

    printf("[INIT] Tasks created (Low=1, Med=3, High=4)\r\n");
    printf("[INIT] Starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
