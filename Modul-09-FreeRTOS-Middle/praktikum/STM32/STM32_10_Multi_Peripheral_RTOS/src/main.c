// File utama program STM32_10_Multi_Peripheral_RTOS
// All peripherals combined
// Tasks untuk GPIO, ADC, I2C, SPI, UART
// Queue sets, multiple semaphores
// Comprehensive demo

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

// Handles
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

// Queues
QueueHandle_t adcQueue = NULL;
QueueHandle_t sensorQueue = NULL;
QueueHandle_t uartQueue = NULL;
QueueHandle_t peripheralQueueSet = NULL;

// Semaphores
SemaphoreHandle_t btn1Semaphore = NULL;
SemaphoreHandle_t btn2Semaphore = NULL;
SemaphoreHandle_t spiMutex = NULL;

// Task handles
TaskHandle_t gpioTaskHandle = NULL;
TaskHandle_t adcTaskHandle = NULL;
TaskHandle_t i2cTaskHandle = NULL;
TaskHandle_t spiTaskHandle = NULL;
TaskHandle_t uartTaskHandle = NULL;
TaskHandle_t monitorTaskHandle = NULL;

// Sensor data structure
typedef struct
{
    int16_t temperature;
    uint32_t pressure;
} SensorData_t;

// UART event structure
typedef struct
{
    char command[32];
    TickType_t timestamp;
} UartEvent_t;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
void GpioTask(void *argument);
void AdcTask(void *argument);
void I2cTask(void *argument);
void SpiTask(void *argument);
void UartTask(void *argument);
void MonitorTask(void *argument);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void UART_SendString(char *str);
void UART_SendNumber(int32_t num);
uint8_t ReadSensorI2C(SensorData_t *data);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    adcQueue = xQueueCreate(ADC_QUEUE_SIZE, sizeof(uint16_t));
    sensorQueue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(SensorData_t));
    uartQueue = xQueueCreate(UART_QUEUE_SIZE, sizeof(UartEvent_t));

    peripheralQueueSet = xQueueCreateSet(ADC_QUEUE_SIZE + SENSOR_QUEUE_SIZE + UART_QUEUE_SIZE);
    xQueueAddToSet(adcQueue, peripheralQueueSet);
    xQueueAddToSet(sensorQueue, peripheralQueueSet);
    xQueueAddToSet(uartQueue, peripheralQueueSet);

    btn1Semaphore = xSemaphoreCreateBinary();
    btn2Semaphore = xSemaphoreCreateBinary();
    spiMutex = xSemaphoreCreateMutex();

    xTaskCreate(GpioTask, "GPIO_Task", TASK_STACK_SIZE, NULL, GPIO_TASK_PRIORITY, &gpioTaskHandle);
    xTaskCreate(AdcTask, "ADC_Task", TASK_STACK_SIZE, NULL, ADC_TASK_PRIORITY, &adcTaskHandle);
    xTaskCreate(I2cTask, "I2C_Task", TASK_STACK_SIZE, NULL, I2C_TASK_PRIORITY, &i2cTaskHandle);
    xTaskCreate(SpiTask, "SPI_Task", TASK_STACK_SIZE, NULL, SPI_TASK_PRIORITY, &spiTaskHandle);
    xTaskCreate(UartTask, "UART_Task", TASK_STACK_SIZE, NULL, UART_TASK_PRIORITY, &uartTaskHandle);
    xTaskCreate(MonitorTask, "Monitor_Task", TASK_STACK_SIZE, NULL, 1, &monitorTaskHandle);

    UART_SendString("STM32 Multi-Peripheral RTOS Demo Ready\r\n");
    UART_SendString("All peripherals initialized\r\n");

    vTaskStartScheduler();

    while (1)
    {
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BUTTON1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BUTTON1_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BUTTON2_PIN;
    HAL_GPIO_Init(BUTTON2_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

static void MX_ADC1_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
#ifdef ADC_SAMPLETIME_239CYCLES_5
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
#else
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
#endif
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
}

static void MX_I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi1);
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void UART_SendString(char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void UART_SendNumber(int32_t num)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", num);
    UART_SendString(buf);
}

uint8_t ReadSensorI2C(SensorData_t *data)
{
    data->temperature = 2550;
    data->pressure = 101325;
    return 1;
}

void GpioTask(void *argument)
{
    uint8_t led_state = 0;
    while (1)
    {
        if (xSemaphoreTake(btn1Semaphore, 0) == pdTRUE)
        {
            led_state = !led_state;
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, led_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
            UART_SendString("GPIO Task: Button 1 - LED ");
            UART_SendString(led_state ? "ON\r\n" : "OFF\r\n");
        }

        if (xSemaphoreTake(btn2Semaphore, 0) == pdTRUE)
        {
            UART_SendString("GPIO Task: Button 2 - LED Blink 3x\r\n");
            for (int i = 0; i < 6; i++)
            {
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void AdcTask(void *argument)
{
    uint16_t adc_value;
    while (1)
    {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            adc_value = HAL_ADC_GetValue(&hadc1);
            xQueueSend(adcQueue, &adc_value, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void I2cTask(void *argument)
{
    SensorData_t sensor_data;
    while (1)
    {
        if (ReadSensorI2C(&sensor_data))
        {
            xQueueSend(sensorQueue, &sensor_data, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void SpiTask(void *argument)
{
    uint16_t counter = 0;
    while (1)
    {
        if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE)
        {
            uint8_t spi_data = (uint8_t)(counter & 0xFF);
            HAL_SPI_Transmit(&hspi1, &spi_data, 1, HAL_MAX_DELAY);
            xSemaphoreGive(spiMutex);
        }
        counter++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void UartTask(void *argument)
{
    uint8_t rx_char;
    char cmd_buffer[32];
    uint8_t cmd_index = 0;
    UartEvent_t event;
    while (1)
    {
        if (HAL_UART_Receive(&huart1, &rx_char, 1, 100) == HAL_OK)
        {
            if (rx_char == '\n' || rx_char == '\r')
            {
                cmd_buffer[cmd_index] = '\0';
                if (cmd_index > 0)
                {
                    strcpy(event.command, cmd_buffer);
                    event.timestamp = xTaskGetTickCount();
                    xQueueSend(uartQueue, &event, 0);
                }
                cmd_index = 0;
            }
            else
            {
                cmd_buffer[cmd_index++] = rx_char;
                if (cmd_index >= 31)
                {
                    cmd_index = 0;
                }
            }
        }
    }
}

void MonitorTask(void *argument)
{
    QueueSetMemberHandle_t activeQueue;
    uint16_t adc_value;
    SensorData_t sensor_data;
    UartEvent_t uart_event;
    while (1)
    {
        activeQueue = xQueueSelectFromSet(peripheralQueueSet, pdMS_TO_TICKS(1000));
        if (activeQueue != NULL)
        {
            if (activeQueue == adcQueue)
            {
                if (xQueueReceive(adcQueue, &adc_value, 0) == pdTRUE)
                {
                    UART_SendString("Monitor: ADC = ");
                    UART_SendNumber(adc_value);
                    UART_SendString("\r\n");
                }
            }
            else if (activeQueue == sensorQueue)
            {
                if (xQueueReceive(sensorQueue, &sensor_data, 0) == pdTRUE)
                {
                    UART_SendString("Monitor: Temp = ");
                    UART_SendNumber(sensor_data.temperature / 100);
                    UART_SendString(" C\r\n");
                }
            }
            else if (activeQueue == uartQueue)
            {
                if (xQueueReceive(uartQueue, &uart_event, 0) == pdTRUE)
                {
                    UART_SendString("Monitor: CMD = ");
                    UART_SendString(uart_event.command);
                    UART_SendString("\r\n");
                }
            }
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin == BUTTON1_PIN)
    {
        xSemaphoreGiveFromISR(btn1Semaphore, &xHigherPriorityTaskWoken);
    }
    else if (GPIO_Pin == BUTTON2_PIN)
    {
        xSemaphoreGiveFromISR(btn2Semaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON1_PIN);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(BUTTON2_PIN);
}
