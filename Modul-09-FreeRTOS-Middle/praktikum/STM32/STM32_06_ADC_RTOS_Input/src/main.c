// File utama program STM32_06_ADC_RTOS_Input
// ADC dengan RTOS
// Task 1: Read ADC continuously
// Task 2: Process dan filter ADC data
// Queue untuk ADC values

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "queue.h"

// Mendeklarasikan handle untuk ADC
ADC_HandleTypeDef hadc1;
// Mendeklarasikan handle untuk UART
UART_HandleTypeDef huart1;
// Mendeklarasikan handle untuk queue ADC values
QueueHandle_t adcQueue = NULL;
// Mendeklarasikan handle untuk task ADC read
TaskHandle_t adcReadTaskHandle = NULL;
// Mendeklarasikan handle untuk task ADC process
TaskHandle_t adcProcessTaskHandle = NULL;
// Mendeklarasikan buffer untuk filter moving average
uint16_t filter_buffer[FILTER_SIZE];
// Mendeklarasikan index untuk filter buffer
uint8_t filter_index = 0;
// Mendeklarasikan flag untuk menandakan buffer penuh
uint8_t filter_full = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
void AdcReadTask(void *argument);
void AdcProcessTask(void *argument);
void UART_SendString(char *str);
void UART_SendNumber(uint16_t num);
uint16_t MovingAverageFilter(uint16_t new_value);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USART1_UART_Init();

    adcQueue = xQueueCreate(ADC_QUEUE_SIZE, sizeof(uint16_t));

    xTaskCreate(AdcReadTask, "ADC_Read_Task", TASK_STACK_SIZE, NULL, ADC_READ_TASK_PRIORITY, &adcReadTaskHandle);
    xTaskCreate(AdcProcessTask, "ADC_Process_Task", TASK_STACK_SIZE, NULL, ADC_PROCESS_TASK_PRIORITY, &adcProcessTaskHandle);

    UART_SendString("STM32 ADC RTOS Input Ready\r\n");

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

static void MX_ADC1_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL;
    sConfig.Rank = 1;
#ifdef ADC_SAMPLETIME_55CYCLES_5
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
#else
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
#endif
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
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

void UART_SendNumber(uint16_t num)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", num);
    UART_SendString(buf);
}

uint16_t MovingAverageFilter(uint16_t new_value)
{
    filter_buffer[filter_index] = new_value;
    filter_index++;
    if (filter_index >= FILTER_SIZE)
    {
        filter_index = 0;
        filter_full = 1;
    }

    uint32_t sum = 0;
    uint8_t count;
    if (filter_full)
    {
        count = FILTER_SIZE;
    }
    else
    {
        count = filter_index;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        sum += filter_buffer[i];
    }
    return (uint16_t)(sum / count);
}

void AdcReadTask(void *argument)
{
    uint16_t adc_value;
    while (1)
    {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            adc_value = HAL_ADC_GetValue(&hadc1);
            xQueueSend(adcQueue, &adc_value, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void AdcProcessTask(void *argument)
{
    uint16_t raw_adc_value;
    uint16_t filtered_value;
    uint32_t send_counter = 0;
    while (1)
    {
        if (xQueueReceive(adcQueue, &raw_adc_value, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            filtered_value = MovingAverageFilter(raw_adc_value);
            send_counter++;
            if (send_counter >= 10)
            {
                send_counter = 0;
                UART_SendString("Raw: ");
                UART_SendNumber(raw_adc_value);
                UART_SendString(" Filtered: ");
                UART_SendNumber(filtered_value);
                UART_SendString("\r\n");
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            }
        }
    }
}
