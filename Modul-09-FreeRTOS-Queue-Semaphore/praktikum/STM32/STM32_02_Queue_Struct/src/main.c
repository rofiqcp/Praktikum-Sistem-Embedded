/**
 * STM32_02_Queue_Struct
 * Send struct with sensor_type, value, timestamp, sensor_id via queue.
 * 3 sensor tasks send data to 1 processor task.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef huart1;

/* Sensor data structure */
typedef struct {
    uint8_t  sensor_type;    /* 0=Temperature, 1=Humidity, 2=Light */
    float    value;
    uint32_t timestamp;      /* Tick count */
    uint8_t  sensor_id;
} SensorData_t;

static QueueHandle_t xSensorQueue;
static volatile uint32_t ulTotalProcessed = 0;
static volatile uint32_t ulPerSensorCount[3] = {0, 0, 0};

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

static const char *sensor_type_name(uint8_t type) {
    switch (type) {
        case 0: return "Temperature";
        case 1: return "Humidity";
        case 2: return "Light";
        default: return "Unknown";
    }
}

/* Simulated sensor reading */
static float simulate_sensor_value(uint8_t type, uint32_t counter) {
    switch (type) {
        case 0: return 20.0f + (float)(counter % 15) + 0.5f;     /* Temperature: 20-35°C */
        case 1: return 40.0f + (float)(counter % 40) + 0.3f;     /* Humidity: 40-80% */
        case 2: return 100.0f + (float)(counter % 900);           /* Light: 100-1000 lux */
        default: return 0.0f;
    }
}

/* Sensor Task - parameterized by sensor type */
static void vSensorTask(void *pvParameters) {
    uint8_t sensor_type = (uint8_t)(uint32_t)pvParameters;
    SensorData_t data;
    uint32_t counter = 0;
    /* Different sampling rates per sensor type */
    TickType_t xDelay = pdMS_TO_TICKS(500 + sensor_type * 300);

    printf("[Sensor%d] %s task started (period: %lums)\r\n",
           sensor_type, sensor_type_name(sensor_type),
           (unsigned long)(500 + sensor_type * 300));

    for (;;) {
        counter++;
        data.sensor_type = sensor_type;
        data.sensor_id = sensor_type;
        data.value = simulate_sensor_value(sensor_type, counter);
        data.timestamp = xTaskGetTickCount();

        if (xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS) {
            printf("[Sensor%d] Sent %s: %.1f (t=%lu)\r\n",
                   sensor_type, sensor_type_name(sensor_type),
                   data.value, data.timestamp);
        } else {
            printf("[Sensor%d] Queue FULL - data lost!\r\n", sensor_type);
        }

        vTaskDelay(xDelay);
    }
}

/* Processor Task - receives and processes all sensor data */
static void vProcessorTask(void *pvParameters) {
    SensorData_t received;

    printf("[Processor] Data processor task started\r\n");

    for (;;) {
        if (xQueueReceive(xSensorQueue, &received, pdMS_TO_TICKS(2000)) == pdPASS) {
            ulTotalProcessed++;
            ulPerSensorCount[received.sensor_type]++;

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

            printf("[Processor] #%lu | ID:%d Type:%s Value:%.1f Time:%lu\r\n",
                   ulTotalProcessed, received.sensor_id,
                   sensor_type_name(received.sensor_type),
                   received.value, received.timestamp);
        } else {
            printf("[Processor] No data received (timeout)\r\n");
        }
    }
}

/* Statistics Task */
static void vStatsTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        printf("\r\n===== Sensor Queue Statistics =====\r\n");
        printf("  Total processed:  %lu\r\n", ulTotalProcessed);
        printf("  Temperature:      %lu\r\n", ulPerSensorCount[0]);
        printf("  Humidity:         %lu\r\n", ulPerSensorCount[1]);
        printf("  Light:            %lu\r\n", ulPerSensorCount[2]);
        printf("  Queue pending:    %u\r\n",
               (unsigned int)uxQueueMessagesWaiting(xSensorQueue));
        printf("  Free heap:        %u bytes\r\n",
               (unsigned int)xPortGetFreeHeapSize());
        printf("===================================\r\n\r\n");
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    LED_GPIO_Init();

    printf("\r\n========================================\r\n");
    printf("  STM32 FreeRTOS Queue Struct Demo\r\n");
    printf("  3 Sensors -> 1 Processor via Queue\r\n");
    printf("========================================\r\n\r\n");

    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    if (xSensorQueue == NULL) {
        printf("[ERROR] Failed to create sensor queue!\r\n");
        while (1);
    }

    xTaskCreate(vSensorTask, "TempSensor", 256, (void *)0, 2, NULL);
    xTaskCreate(vSensorTask, "HumSensor",  256, (void *)1, 2, NULL);
    xTaskCreate(vSensorTask, "LightSensor", 256, (void *)2, 2, NULL);
    xTaskCreate(vProcessorTask, "Processor", 256, NULL, 3, NULL);
    xTaskCreate(vStatsTask, "Stats", 256, NULL, 1, NULL);

    printf("[INIT] All tasks created, starting scheduler...\r\n\r\n");
    vTaskStartScheduler();

    while (1);
}
