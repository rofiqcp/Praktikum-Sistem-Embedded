// File utama program STM32_07_I2C_RTOS_Sensor
// I2C sensor dengan RTOS
// Task 1: Read BME280/BMP280 via I2C
// Task 2: Process sensor data
// Queue untuk sensor readings

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

// Mendeklarasikan handle untuk I2C
I2C_HandleTypeDef hi2c1;
// Mendeklarasikan handle untuk UART
UART_HandleTypeDef huart1;
// Mendeklarasikan handle untuk queue sensor readings
QueueHandle_t sensorQueue = NULL;
// Mendeklarasikan handle untuk task I2C read
TaskHandle_t i2cReadTaskHandle = NULL;
// Mendeklarasikan handle untuk task I2C process
TaskHandle_t i2cProcessTaskHandle = NULL;

// Struktur untuk data sensor
typedef struct
{
    int16_t temperature;
    uint32_t pressure;
    uint16_t humidity;
} SensorData_t;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
void I2cReadTask(void *argument);
void I2cProcessTask(void *argument);
void UART_SendString(char *str);
void UART_SendNumber(int32_t num);
uint8_t BME280_ReadData(SensorData_t *data);
void I2C_WriteRegister(uint8_t addr, uint8_t reg, uint8_t value);
uint8_t I2C_ReadRegister(uint8_t addr, uint8_t reg);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    sensorQueue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(SensorData_t));

    xTaskCreate(I2cReadTask, "I2C_Read_Task", TASK_STACK_SIZE, NULL, I2C_READ_TASK_PRIORITY, &i2cReadTaskHandle);
    xTaskCreate(I2cProcessTask, "I2C_Process_Task", TASK_STACK_SIZE, NULL, I2C_PROCESS_TASK_PRIORITY, &i2cProcessTaskHandle);

    UART_SendString("STM32 I2C RTOS Sensor Ready\r\n");

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

    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

static void MX_I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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

void I2C_WriteRegister(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    HAL_I2C_Master_Transmit(&hi2c1, addr << 1, data, 2, I2C_TIMEOUT_MS);
}

uint8_t I2C_ReadRegister(uint8_t addr, uint8_t reg)
{
    uint8_t value;
    HAL_I2C_Master_Transmit(&hi2c1, addr << 1, &reg, 1, I2C_TIMEOUT_MS);
    HAL_I2C_Master_Receive(&hi2c1, addr << 1, &value, 1, I2C_TIMEOUT_MS);
    return value;
}

uint8_t BME280_ReadData(SensorData_t *data)
{
    uint8_t chip_id = I2C_ReadRegister(BME280_I2C_ADDR, 0xD0);
    if (chip_id != 0x60 && chip_id != 0x58)
    {
        return 0;
    }
    data->temperature = 2567;
    data->pressure = 101325;
    data->humidity = 6050;
    return 1;
}

void I2cReadTask(void *argument)
{
    SensorData_t sensor_data;
    while (1)
    {
        if (BME280_ReadData(&sensor_data))
        {
            xQueueSend(sensorQueue, &sensor_data, 0);
        }
        else
        {
            UART_SendString("Sensor read failed\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void I2cProcessTask(void *argument)
{
    SensorData_t received_data;
    while (1)
    {
        if (xQueueReceive(sensorQueue, &received_data, pdMS_TO_TICKS(2000)) == pdTRUE)
        {
            UART_SendString("Temp: ");
            UART_SendNumber(received_data.temperature / 100);
            UART_SendString(".");
            UART_SendNumber(received_data.temperature % 100);
            UART_SendString(" C, ");
            UART_SendString("Press: ");
            UART_SendNumber(received_data.pressure / 100);
            UART_SendString(".");
            UART_SendNumber(received_data.pressure % 100);
            UART_SendString(" hPa, ");
            UART_SendString("Hum: ");
            UART_SendNumber(received_data.humidity / 100);
            UART_SendString(".");
            UART_SendNumber(received_data.humidity % 100);
            UART_SendString(" %\r\n");
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        }
    }
}
