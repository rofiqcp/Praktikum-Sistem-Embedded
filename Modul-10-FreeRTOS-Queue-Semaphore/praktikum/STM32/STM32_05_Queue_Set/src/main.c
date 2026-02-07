/**
 * STM32_05_Queue_Set
 * Two queues in a QueueSet. Multiplexed receiver using xQueueSelectFromSet.
 * Demonstrates receiving from multiple sources through a single wait point.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;

static QueueHandle_t xTempQueue;
static QueueHandle_t xAlarmQueue;
static QueueSetHandle_t xQueueSet;

typedef struct {
    float    temperature;
    uint32_t timestamp;
} TempData_t;

typedef struct {
    uint8_t  alarm_code;
    char     message[20];
    uint32_t timestamp;
} AlarmData_t;

static volatile uint32_t ulTempCount = 0;
static volatile uint32_t ulAlarmCount = 0;

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

/* Temperature sender task */
static void vTempSenderTask(void *pvParameters) {
    TempData_t data;
    float base_temp = 25.0f;
    uint32_t counter = 0;

    printf("[TempSender] Task started (period: 800ms)\r\n");

    for (;;) {
        counter++;
        data.temperature = base_temp + (float)(counter % 10) - 2.0f;
        data.timestamp = xTaskGetTickCount();

        if (xQueueSend(xTempQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[TempSender] Sent temp=%.1f°C (t=%lu)\r\n",
                   data.temperature, data.timestamp);
        } else {
            printf("[TempSender] Queue full!\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

/* Alarm sender task */
static void vAlarmSenderTask(void *pvParameters) {
    AlarmData_t alarm;
    uint8_t alarm_codes[] = {1, 2, 3, 1, 2};
    const char *alarm_msgs[] = {"OVERTEMP", "LOW_BATT", "SENSOR_FAIL", "OVERTEMP", "LOW_BATT"};
    uint32_t idx = 0;

    printf("[AlarmSender] Task started (period: 2000ms)\r\n");

    for (;;) {
        alarm.alarm_code = alarm_codes[idx % 5];
        strncpy(alarm.message, alarm_msgs[idx % 5], sizeof(alarm.message) - 1);
        alarm.message[sizeof(alarm.message) - 1] = '\0';
        alarm.timestamp = xTaskGetTickCount();

        if (xQueueSend(xAlarmQueue, &alarm, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[AlarmSender] Sent alarm: code=%d msg=%s (t=%lu)\r\n",
                   alarm.alarm_code, alarm.message, alarm.timestamp);
        } else {
            printf("[AlarmSender] Queue full!\r\n");
        }

        idx++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* Multiplexed receiver task - uses QueueSet */
static void vReceiverTask(void *pvParameters) {
    QueueSetMemberHandle_t xActivatedMember;
    TempData_t tempData;
    AlarmData_t alarmData;

    printf("[Receiver] Multiplexed receiver started\r\n");

    for (;;) {
        /* Wait for any queue in the set to have data */
        xActivatedMember = xQueueSelectFromSet(xQueueSet, pdMS_TO_TICKS(3000));

        if (xActivatedMember == NULL) {
            printf("[Receiver] Timeout - no data from any queue\r\n");
            continue;
        }

        if (xActivatedMember == xTempQueue) {
            if (xQueueReceive(xTempQueue, &tempData, 0) == pdPASS) {
                ulTempCount++;
                printf("[Receiver] TEMP #%lu: %.1f°C (t=%lu)\r\n",
                       ulTempCount, tempData.temperature, tempData.timestamp);

                /* Toggle LED on high temperature */
                if (tempData.temperature > 30.0f) {
                    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                    printf("[Receiver] HIGH TEMP WARNING! LED toggled\r\n");
                }
            }
        } else if (xActivatedMember == xAlarmQueue) {
            if (xQueueReceive(xAlarmQueue, &alarmData, 0) == pdPASS) {
                ulAlarmCount++;
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* LED ON */
                printf("[Receiver] ALARM #%lu: [%d] %s (t=%lu) - LED ON\r\n",
                       ulAlarmCount, alarmData.alarm_code,
                       alarmData.message, alarmData.timestamp);
                vTaskDelay(pdMS_TO_TICKS(200));
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED OFF */
            }
        }
    }
}

/* Stats Task */
static void vStatsTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        printf("\r\n===== Queue Set Statistics =====\r\n");
        printf("  Temp readings:  %lu\r\n", ulTempCount);
        printf("  Alarm events:   %lu\r\n", ulAlarmCount);
        printf("  Temp Q pending: %u\r\n",
               (unsigned int)uxQueueMessagesWaiting(xTempQueue));
        printf("  Alarm Q pending:%u\r\n",
               (unsigned int)uxQueueMessagesWaiting(xAlarmQueue));
        printf("  Free heap:      %u bytes\r\n",
               (unsigned int)xPortGetFreeHeapSize());
        printf("================================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 FreeRTOS Queue Set Demo\r\n");
    printf("  Multiplexed receiver with QueueSet\r\n");
    printf("========================================\r\n\r\n");

    /* Create queues */
    xTempQueue = xQueueCreate(5, sizeof(TempData_t));
    xAlarmQueue = xQueueCreate(5, sizeof(AlarmData_t));

    if (xTempQueue == NULL || xAlarmQueue == NULL) {
        printf("[ERROR] Failed to create queues!\r\n");
        while (1);
    }

    /* Create queue set - total size = sum of all queue lengths */
    xQueueSet = xQueueCreateSet(5 + 5);
    if (xQueueSet == NULL) {
        printf("[ERROR] Failed to create queue set!\r\n");
        while (1);
    }

    /* Add queues to the set */
    xQueueAddToSet(xTempQueue, xQueueSet);
    xQueueAddToSet(xAlarmQueue, xQueueSet);

    printf("[INIT] Queue set created with 2 queues\r\n");

    xTaskCreate(vTempSenderTask, "TempSend", 256, NULL, 2, NULL);
    xTaskCreate(vAlarmSenderTask, "AlarmSend", 256, NULL, 2, NULL);
    xTaskCreate(vReceiverTask, "Receiver", 256, NULL, 3, NULL);
    xTaskCreate(vStatsTask, "Stats", 256, NULL, 1, NULL);

    printf("[INIT] Tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
