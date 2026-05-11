#include "stm32f1xx_hal.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

uint8_t txBuffer[BUFFER_SIZE];
uint8_t rxBuffer[BUFFER_SIZE];

void SystemClock_Config(void);
void GPIO_Init(void);
void SPI_Master_Init(void);
void UART_Init(void);
void Error_Handler(void);

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void SPI_Master_Init(void)
{
    SPI_MASTER_CLK_ENABLE();
    SPI_MASTER_GPIO_CLK_ENABLE();
    
    // Configure SPI pins
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // SCK, MOSI
    GPIO_InitStruct.Pin = SPI_SCK_PIN | SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);
    
    // MISO
    GPIO_InitStruct.Pin = SPI_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);
    
    // CS (Software controlled)
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET);  // CS high (inactive)
    
    // Configure SPI
    hspi1.Instance = SPI_MASTER;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;  // ~1 MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

void UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // TX (PA9)
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // RX (PA10)
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart1.Instance = UART_INSTANCE;
    huart1.Init.BaudRate = UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

void GPIO_Init(void)
{
    LED_GPIO_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;  // 8MHz * 9 = 72MHz
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    UART_Init();
    SPI_Master_Init();
    
    printf("\r\n=== STM32 SPI Master - Controls ESP32 Slave ===\r\n");
    printf("Pin Configuration:\r\n");
    printf("  SCK:  PA5\r\n");
    printf("  MISO: PA6\r\n");
    printf("  MOSI: PA7\r\n");
    printf("  CS:   PA4\r\n\r\n");
    
    uint32_t counter = 0;
    
    while (1) {
        // Prepare data to send
        snprintf((char *)txBuffer, BUFFER_SIZE, "STM32_CMD:%lu", counter++);
        memset(rxBuffer, 0, BUFFER_SIZE);
        
        // CS Low (Select slave)
        HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);
        
        // Transmit and receive data
        if (HAL_SPI_TransmitReceive(&hspi1, txBuffer, rxBuffer, BUFFER_SIZE, 1000) == HAL_OK) {
            // CS High (Deselect slave)
            HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET);
            
            rxBuffer[BUFFER_SIZE - 1] = '\0';
            printf("Sent: %s\r\n", txBuffer);
            printf("Received from ESP32: %s\r\n\r\n", rxBuffer);
            
            // Toggle LED
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        } else {
            HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET);
            printf("SPI Transaction failed\r\n");
        }
        
        HAL_Delay(1000);
    }
}

void Error_Handler(void)
{
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        HAL_Delay(100);
    }
}
