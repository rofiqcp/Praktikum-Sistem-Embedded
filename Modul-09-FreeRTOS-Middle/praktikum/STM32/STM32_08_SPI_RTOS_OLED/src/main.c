// File utama program STM32_08_SPI_RTOS_OLED
// SPI OLED dengan RTOS
// Task 1: Prepare display data
// Task 2: Update OLED via SPI
// Mutex untuk SPI bus

#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include "semphr.h"

// Mendeklarasikan handle untuk SPI1
SPI_HandleTypeDef hspi1;
// Mendeklarasikan handle untuk UART
UART_HandleTypeDef huart1;
// Mendeklarasikan handle untuk mutex SPI
SemaphoreHandle_t spiMutex = NULL;
// Mendeklarasikan handle untuk task prepare display
TaskHandle_t prepareTaskHandle = NULL;
// Mendeklarasikan handle untuk task update OLED
TaskHandle_t updateTaskHandle = NULL;
// Mendeklarasikan buffer untuk display data
uint8_t display_buffer[DISPLAY_BUFFER_SIZE];
// Mendeklarasikan counter untuk animasi
uint16_t animation_counter = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
void PrepareDisplayTask(void *argument);
void UpdateOledTask(void *argument);
void UART_SendString(char *str);
void OLED_SendData(uint8_t *data, uint16_t size);
void OLED_SendCommand(uint8_t cmd);
void OLED_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    spiMutex = xSemaphoreCreateMutex();
    OLED_Init();

    xTaskCreate(PrepareDisplayTask, "Prepare_Task", TASK_STACK_SIZE, NULL, PREPARE_TASK_PRIORITY, &prepareTaskHandle);
    xTaskCreate(UpdateOledTask, "Update_OLED_Task", TASK_STACK_SIZE, NULL, UPDATE_TASK_PRIORITY, &updateTaskHandle);

    UART_SendString("STM32 SPI RTOS OLED Ready\r\n");

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

    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OLED_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(OLED_RST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OLED_DC_PIN;
    HAL_GPIO_Init(OLED_DC_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OLED_CS_PIN;
    HAL_GPIO_Init(OLED_CS_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}

static void MX_SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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

void OLED_SendCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}

void OLED_SendData(uint8_t *data, uint16_t size)
{
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, data, size, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET);
}

void OLED_Init(void)
{
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(100);

    if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE)
    {
        OLED_SendCommand(0xAE);
        OLED_SendCommand(0x20);
        OLED_SendCommand(0x00);
        OLED_SendCommand(0x81);
        OLED_SendCommand(0x7F);
        OLED_SendCommand(0xA6);
        OLED_SendCommand(0xA8);
        OLED_SendCommand(0x3F);
        OLED_SendCommand(0xD3);
        OLED_SendCommand(0x00);
        OLED_SendCommand(0xD5);
        OLED_SendCommand(0x80);
        OLED_SendCommand(0xD9);
        OLED_SendCommand(0xF1);
        OLED_SendCommand(0xDA);
        OLED_SendCommand(0x12);
        OLED_SendCommand(0xDB);
        OLED_SendCommand(0x40);
        OLED_SendCommand(0x8D);
        OLED_SendCommand(0x14);
        OLED_SendCommand(0xAF);

        xSemaphoreGive(spiMutex);
    }
}

void PrepareDisplayTask(void *argument)
{
    uint16_t i;
    while (1)
    {
        animation_counter++;

        for (i = 0; i < DISPLAY_BUFFER_SIZE; i++)
        {
            display_buffer[i] = (uint8_t)(128 + 127 * sin((2.0 * 3.14159265 * (i + animation_counter)) / DISPLAY_BUFFER_SIZE));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void UpdateOledTask(void *argument)
{
    while (1)
    {
        if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE)
        {
            OLED_SendData(display_buffer, DISPLAY_BUFFER_SIZE);
            xSemaphoreGive(spiMutex);
        }

        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
