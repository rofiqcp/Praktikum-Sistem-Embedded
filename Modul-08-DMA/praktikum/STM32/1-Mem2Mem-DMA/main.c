/*
 * Project: STM32 DMA Memory to Memory Transfer
 * Board: STM32F103C8T6 (Blue Pill)
 * Purpose: Demonstrate copying data between arrays using DMA1 Channel 1
 */

#include "main.h"

/* Private variables ---------------------------------------------------------*/
DMA_HandleTypeDef hdma_memtomem_dma1_channel1;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
#define BUFFER_SIZE 32
const uint32_t srcBuffer[BUFFER_SIZE] = {
    0x11111111, 0x22222222, 0x33333333, 0x44444444,
    0x55555555, 0x66666666, 0x77777777, 0x88888888,
    // ... fill rest as needed, logic will generate pattern
};
uint32_t dstBuffer[BUFFER_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);

int main(void) {
    /* MCU Configuration */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */
    // Initialize Source Buffer with pattern
    uint32_t *pSrc = (uint32_t*)srcBuffer; // Cast const away for demo filling if needed
    for(int i=0; i<BUFFER_SIZE; i++) {
        ((uint32_t*)srcBuffer)[i] = 0xDEAD0000 + i;
        dstBuffer[i] = 0; // Clear Destination
    }

    // 1. Start DMA Transfer
    // Source: srcBuffer, Dest: dstBuffer, Size: 32 Words
    HAL_DMA_Start(&hdma_memtomem_dma1_channel1, (uint32_t)srcBuffer, (uint32_t)dstBuffer, BUFFER_SIZE);

    // 2. Wait for Transfer Complete (Polling for demo)
    // In real app, we usually don't block.
    HAL_StatusTypeDef status = HAL_DMA_PollForTransfer(&hdma_memtomem_dma1_channel1, HAL_DMA_FULL_TRANSFER, 1000);

    // 3. Verify
    if (status == HAL_OK) {
        // Check content
        int errors = 0;
        for(int i=0; i<BUFFER_SIZE; i++) {
            if(dstBuffer[i] != srcBuffer[i]) {
                errors++;
            }
        }
        
        if(errors == 0) {
            // Success: Turn on PC13 LED (Active Low)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        } else {
            // Data Mismatch
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
        }
    } else {
        // DMA Error
    }
    /* USER CODE END 2 */

    while (1) {
        /* USER CODE BEGIN 3 */
        // Blink if success to show life
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(500);
        /* USER CODE END 3 */
    }
}

/**
  * Enable DMA controller clock
  * Configure DMA for Memory to Memory
  */
static void MX_DMA_Init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Configure DMA request hdma_memtomem_dma1_channel1 on DMA1_Channel1 */
    hdma_memtomem_dma1_channel1.Instance = DMA1_Channel1;
    hdma_memtomem_dma1_channel1.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_memtomem_dma1_channel1.Init.PeriphInc = DMA_PINC_ENABLE;
    hdma_memtomem_dma1_channel1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_memtomem_dma1_channel1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_memtomem_dma1_channel1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_memtomem_dma1_channel1.Init.Mode = DMA_NORMAL;
    hdma_memtomem_dma1_channel1.Init.Priority = DMA_PRIORITY_LOW;
    
    if (HAL_DMA_Init(&hdma_memtomem_dma1_channel1) != HAL_OK) {
        // Error Handler
    }
}

/* ... SystemClock_Config and other Inits omitted for brevity ... */
