/**
 * STM32_01_Queue_Basic
 * Send integers via queue. Producer task sends counter, consumer receives and toggles LED.
 * Basic queue usage demonstration on STM32F103 BluePill.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

/* Private variables */
static UART_HandleTypeDef huart1;
static QueueHandle_t xDataQueue;

/* Counters for statistics */
static volatile uint32_t ulSendCount = 0;
static volatile uint32_t ulReceiveCount = 0;
static volatile uint32_t ulSendFailCount = 0;

/* printf redirect to UART1 */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* System Clock Configuration - 72MHz from 8MHz HSE */
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

/* UART1 Initialization - PA9(TX), PA10(RX) at 115200 baud */
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

/* LED GPIO Init - PC13 onboard LED (active low) */
static void LED_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off */
}

/* Error hooks */
void vApplicationMallocFailedHook(void) {
    printf("[ERROR] Malloc failed!\r\n");
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("[ERROR] Stack overflow in task: %s\r\n", pcTaskName);
    while (1);
}

/* Producer Task - sends incrementing counter to queue */
static void vProducerTask(void *pvParameters) {
    uint32_t ulCounter = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[Producer] Task started\r\n");

    for (;;) {
        ulCounter++;

        if (xQueueSend(xDataQueue, &ulCounter, pdMS_TO_TICKS(100)) == pdPASS) {
            ulSendCount++;
            printf("[Producer] Sent: %lu (total sent: %lu)\r\n", ulCounter, ulSendCount);
        } else {
            ulSendFailCount++;
            printf("[Producer] Queue full! Failed count: %lu\r\n", ulSendFailCount);
        }

        /* Print queue status */
        UBaseType_t uxSpaces = uxQueueSpacesAvailable(xDataQueue);
        UBaseType_t uxWaiting = uxQueueMessagesWaiting(xDataQueue);
        printf("[Producer] Queue status - waiting: %u, spaces: %u\r\n",
               (unsigned int)uxWaiting, (unsigned int)uxSpaces);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

/* Consumer Task - receives from queue and toggles LED */
static void vConsumerTask(void *pvParameters) {
    uint32_t ulReceivedValue;

    printf("[Consumer] Task started\r\n");

    for (;;) {
        if (xQueueReceive(xDataQueue, &ulReceivedValue, pdMS_TO_TICKS(1000)) == pdPASS) {
            ulReceiveCount++;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            printf("[Consumer] Received: %lu (total received: %lu) - LED toggled\r\n",
                   ulReceivedValue, ulReceiveCount);
        } else {
            printf("[Consumer] Queue empty - timeout\r\n");
        }
    }
}

/* Statistics Task */
static void vStatsTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("\r\n===== Queue Statistics =====\r\n");
        printf("  Sent:     %lu\r\n", ulSendCount);
        printf("  Received: %lu\r\n", ulReceiveCount);
        printf("  Failed:   %lu\r\n", ulSendFailCount);
        printf("  Pending:  %u\r\n", (unsigned int)uxQueueMessagesWaiting(xDataQueue));
        printf("  Free Heap: %u bytes\r\n", (unsigned int)xPortGetFreeHeapSize());
        printf("============================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 FreeRTOS Queue Basic Demo\r\n");
    printf("  Queue Size: 5 items (uint32_t)\r\n");
    printf("========================================\r\n\r\n");

    /* Create queue with capacity of 5 uint32_t items */
    xDataQueue = xQueueCreate(5, sizeof(uint32_t));
    if (xDataQueue == NULL) {
        printf("[ERROR] Failed to create queue!\r\n");
        while (1);
    }
    printf("[INIT] Queue created successfully\r\n");

    /* Create tasks */
    xTaskCreate(vProducerTask, "Producer", 256, NULL, 2, NULL);
    xTaskCreate(vConsumerTask, "Consumer", 256, NULL, 2, NULL);
    xTaskCreate(vStatsTask, "Stats", 256, NULL, 1, NULL);

    printf("[INIT] Tasks created, starting scheduler...\r\n\r\n");

    vTaskStartScheduler();

    /* Should never reach here */
    printf("[ERROR] Scheduler exited!\r\n");
    while (1);
}
