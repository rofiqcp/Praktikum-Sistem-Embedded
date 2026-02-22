/**
 * STM32_11_Producer_Consumer
 * 3 producers, 1 consumer, bounded buffer queue size 8.
 * Rate monitoring with queue fill level tracking.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;

#define BUFFER_SIZE       8
#define NUM_PRODUCERS     3

static QueueHandle_t xBufferQueue;
static SemaphoreHandle_t xPrintMutex;

typedef struct {
    uint32_t producer_id;
    uint32_t sequence;
    uint32_t data;
    uint32_t timestamp;
} Item_t;

static volatile uint32_t ulProduced[NUM_PRODUCERS] = {0};
static volatile uint32_t ulConsumed = 0;
static volatile uint32_t ulDropped[NUM_PRODUCERS] = {0};
static volatile uint32_t ulFillSamples = 0;
static volatile uint32_t ulFillSum = 0;
static volatile uint32_t ulMaxFill = 0;

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

/* Producer Task */
static void vProducerTask(void *pvParameters) {
    uint32_t id = (uint32_t)pvParameters;
    Item_t item;
    /* Different production rates */
    TickType_t xRate = pdMS_TO_TICKS(300 + id * 200);

    printf("[Producer%lu] Started (rate: %lums)\r\n", id, (unsigned long)(300 + id * 200));

    for (;;) {
        ulProduced[id]++;
        item.producer_id = id;
        item.sequence = ulProduced[id];
        item.data = id * 1000 + ulProduced[id];
        item.timestamp = xTaskGetTickCount();

        UBaseType_t fill = uxQueueMessagesWaiting(xBufferQueue);
        ulFillSamples++;
        ulFillSum += fill;
        if (fill > ulMaxFill) ulMaxFill = fill;

        if (xQueueSend(xBufferQueue, &item, pdMS_TO_TICKS(200)) == pdPASS) {
            printf("[Producer%lu] Sent #%lu (data=%lu, fill=%u/%d)\r\n",
                   id, item.sequence, item.data,
                   (unsigned int)(fill + 1), BUFFER_SIZE);
        } else {
            ulDropped[id]++;
            printf("[Producer%lu] BUFFER FULL! Dropped #%lu (dropped total=%lu)\r\n",
                   id, item.sequence, ulDropped[id]);
        }

        vTaskDelay(xRate);
    }
}

/* Consumer Task */
static void vConsumerTask(void *pvParameters) {
    Item_t item;

    printf("[Consumer] Started (processing rate: 400ms)\r\n");

    for (;;) {
        if (xQueueReceive(xBufferQueue, &item, pdMS_TO_TICKS(2000)) == pdPASS) {
            ulConsumed++;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            uint32_t latency = xTaskGetTickCount() - item.timestamp;
            UBaseType_t fill = uxQueueMessagesWaiting(xBufferQueue);

            printf("[Consumer] #%lu from P%lu: data=%lu latency=%lums (fill=%u/%d)\r\n",
                   ulConsumed, item.producer_id, item.data,
                   (unsigned long)latency, (unsigned int)fill, BUFFER_SIZE);

            /* Simulate processing time */
            vTaskDelay(pdMS_TO_TICKS(400));
        } else {
            printf("[Consumer] Buffer empty - timeout\r\n");
        }
    }
}

/* Rate Monitor Task */
static void vMonitorTask(void *pvParameters) {
    uint32_t lastProduced[NUM_PRODUCERS] = {0};
    uint32_t lastConsumed = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        uint32_t totalProd = 0;
        printf("\r\n===== Producer-Consumer Statistics =====\r\n");
        printf("  Buffer size: %d\r\n", BUFFER_SIZE);
        printf("  Current fill: %u/%d\r\n",
               (unsigned int)uxQueueMessagesWaiting(xBufferQueue), BUFFER_SIZE);

        if (ulFillSamples > 0) {
            printf("  Avg fill: %.1f\r\n", (float)ulFillSum / ulFillSamples);
        }
        printf("  Max fill: %lu\r\n", ulMaxFill);

        printf("\n  Per-producer:\r\n");
        for (int i = 0; i < NUM_PRODUCERS; i++) {
            uint32_t rate = ulProduced[i] - lastProduced[i];
            lastProduced[i] = ulProduced[i];
            totalProd += ulProduced[i];
            printf("    P%d: produced=%lu, dropped=%lu, rate=%lu/5s\r\n",
                   i, ulProduced[i], ulDropped[i], rate);
        }

        uint32_t consumeRate = ulConsumed - lastConsumed;
        lastConsumed = ulConsumed;

        printf("\n  Consumer: consumed=%lu, rate=%lu/5s\r\n", ulConsumed, consumeRate);
        printf("  Total produced: %lu, total dropped: %lu\r\n",
               totalProd, ulDropped[0] + ulDropped[1] + ulDropped[2]);
        printf("  Free heap: %u bytes\r\n", (unsigned int)xPortGetFreeHeapSize());
        printf("========================================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32 FreeRTOS Producer-Consumer Demo\r\n");
    printf("  %d producers, 1 consumer, buffer=%d\r\n",
           NUM_PRODUCERS, BUFFER_SIZE);
    printf("==========================================\r\n\r\n");

    xBufferQueue = xQueueCreate(BUFFER_SIZE, sizeof(Item_t));
    xPrintMutex = xSemaphoreCreateMutex();

    if (xBufferQueue == NULL || xPrintMutex == NULL) {
        printf("[ERROR] Failed to create queue or mutex!\r\n");
        while (1);
    }

    char name[16];
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        snprintf(name, sizeof(name), "Producer%d", i);
        xTaskCreate(vProducerTask, name, 256, (void *)(uint32_t)i, 2, NULL);
    }

    xTaskCreate(vConsumerTask, "Consumer", 256, NULL, 3, NULL);
    xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);

    printf("[INIT] All tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
