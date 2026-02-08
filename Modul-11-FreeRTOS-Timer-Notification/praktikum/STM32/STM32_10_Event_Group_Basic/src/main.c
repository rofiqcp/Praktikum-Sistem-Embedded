/**
 * STM32_10_Event_Group_Basic
 * 3 simulated sensor tasks each set a bit in event group.
 * Main task waits for ALL bits with xEventGroupWaitBits().
 * LED indicates all-sensors-ready state.
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

static EventGroupHandle_t xSensorEventGroup;

/* Event bits for each sensor */
#define SENSOR_TEMP_BIT   (1 << 0)
#define SENSOR_HUMID_BIT  (1 << 1)
#define SENSOR_PRESS_BIT  (1 << 2)
#define ALL_SENSORS_BITS  (SENSOR_TEMP_BIT | SENSOR_HUMID_BIT | SENSOR_PRESS_BIT)

static volatile uint32_t ulAllReadyCount = 0;

void SystemClock_Config(void)
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

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

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

/* Sensor task - simulates reading and sets bit when ready */
typedef struct {
    const char *pcName;
    EventBits_t uxBit;
    uint32_t    ulDelayMs;
} SensorConfig_t;

static void vSensorTask(void *pvParameters)
{
    SensorConfig_t *cfg = (SensorConfig_t *)pvParameters;

    for (;;)
    {
        /* Simulate sensor reading delay */
        vTaskDelay(pdMS_TO_TICKS(cfg->ulDelayMs));

        /* Set bit to indicate sensor data ready */
        EventBits_t uxBits = xEventGroupSetBits(xSensorEventGroup, cfg->uxBit);

        printf("[%s] Ready! Set bit 0x%02lX, Group=0x%02lX, Tick=%lu\r\n",
               cfg->pcName, (unsigned long)cfg->uxBit,
               (unsigned long)uxBits,
               (unsigned long)xTaskGetTickCount());
    }
}

/* Main collector task - waits for all sensors */
static void vCollectorTask(void *pvParameters)
{
    for (;;)
    {
        printf("[COLLECTOR] Waiting for all sensors (bits=0x%02X)...\r\n",
               ALL_SENSORS_BITS);

        /* Wait for ALL sensor bits, clear on exit, wait forever */
        EventBits_t uxBits = xEventGroupWaitBits(
            xSensorEventGroup,
            ALL_SENSORS_BITS,
            pdTRUE,           /* Clear bits on exit */
            pdTRUE,           /* Wait for ALL bits */
            portMAX_DELAY
        );

        ulAllReadyCount++;
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* LED ON */

        printf("[COLLECTOR] *** ALL SENSORS READY *** #%lu, bits=0x%02lX, Tick=%lu\r\n",
               ulAllReadyCount, (unsigned long)uxBits,
               (unsigned long)xTaskGetTickCount());

        /* Brief LED on to indicate all-ready */
        vTaskDelay(pdMS_TO_TICKS(200));
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED OFF */
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("[ERROR] Stack overflow: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/* Static sensor configs */
static SensorConfig_t sensorConfigs[3] = {
    {"TEMP",     SENSOR_TEMP_BIT,   1000},
    {"HUMIDITY", SENSOR_HUMID_BIT,  1500},
    {"PRESSURE", SENSOR_PRESS_BIT,  2000},
};

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    GPIO_Init();

    printf("\r\n=== STM32 Event Group Basic ===\r\n");
    printf("3 sensors set bits, collector waits for all\r\n\r\n");

    xSensorEventGroup = xEventGroupCreate();

    if (xSensorEventGroup != NULL)
    {
        /* Create sensor tasks with different periods */
        xTaskCreate(vSensorTask, "Temp", 256, &sensorConfigs[0], 2, NULL);
        xTaskCreate(vSensorTask, "Humid", 256, &sensorConfigs[1], 2, NULL);
        xTaskCreate(vSensorTask, "Press", 256, &sensorConfigs[2], 2, NULL);
        xTaskCreate(vCollectorTask, "Collect", 256, NULL, 3, NULL);

        printf("[INIT] Sensors: Temp=1s, Humid=1.5s, Press=2s\r\n\r\n");
    }
    else
    {
        printf("[ERROR] Failed to create event group!\r\n");
    }

    vTaskStartScheduler();
    for (;;);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
