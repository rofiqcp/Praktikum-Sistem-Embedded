/**
 * ==========================================================
 *  Modul 07 - STM32_09_I2C_RTOS_DMA_Transfer
 * ==========================================================
 *  Deskripsi:
 *    Transfer I2C DMA dengan FreeRTOS queue/semaphore.
 *    Gunakan DMA untuk transfer data I2C tanpa blocking.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    Sensor I2C (alamat 0x76, misal BME280)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang sensor I2C
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    DMA_QUEUE_LEN 5
 *    TRANSFER_SIZE 64
 * ==========================================================
 */

#include "main.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define DMA_QUEUE_LEN 5
#define TRANSFER_SIZE 64

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_i2c1_tx;
DMA_HandleTypeDef hdma_i2c1_rx;

// Handle RTOS
QueueHandle_t xDmaQueue = NULL;
SemaphoreHandle_t xDmaSemaphore = NULL;
TaskHandle_t xDmaTaskHandle = NULL;

// Buffer transfer
uint8_t dma_tx_buf[TRANSFER_SIZE];
uint8_t dma_rx_buf[TRANSFER_SIZE];
volatile uint8_t dma_done = 0;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_DMA_Init(void);
void StartDmaTask(void *argument);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  // Buat queue dan semaphore
  xDmaQueue = xQueueCreate(DMA_QUEUE_LEN, TRANSFER_SIZE);
  xDmaSemaphore = xSemaphoreCreateBinary();
  if (xDmaQueue == NULL || xDmaSemaphore == NULL) {
    Error_Handler();
  }

  // Buat task RTOS
  xTaskCreate(StartDmaTask, "DMA_Task", configMINIMAL_STACK_SIZE * 2, NULL, 2, &xDmaTaskHandle);

  // Mulai scheduler
  vTaskStartScheduler();

  while (1) {
    Error_Handler();
  }
}

// Task DMA: trigger transfer dan tunggu selesai
void StartDmaTask(void *argument) {
  uint32_t count = 0;

  while (1) {
    // Isi buffer TX dengan data uji
    for (uint8_t i = 0; i < TRANSFER_SIZE; i++) {
      dma_tx_buf[i] = (count + i) & 0xFF;
    }

    // Trigger I2C mem write DMA (alamat 0x76, register 0x00)
    if (HAL_I2C_Mem_Write_DMA(&hi2c1, 0x76 << 1, 0x00, I2C_MEMADD_SIZE_8BIT, dma_tx_buf, TRANSFER_SIZE) != HAL_OK) {
      // Error
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    // Tunggu semaphore dari callback DMA
    if (xSemaphoreTake(xDmaSemaphore, portMAX_DELAY) == pdPASS) {
      // Transfer selesai
      char msg[64];
      sprintf(msg, "DMA transfer %lu selesai\r\n", count);
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    }

    count++;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Callback selesai TX DMA
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    // Beri sinyal semaphore ke task
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xDmaSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

// Inisialisasi DMA
static void MX_DMA_Init(void) {
  __HAL_RCC_DMA1_CLK_ENABLE();

  // DMA I2C1 TX
  hdma_i2c1_tx.Instance = DMA1_Channel6;
  hdma_i2c1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_i2c1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_i2c1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_i2c1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_i2c1_tx.Init.Mode = DMA_NORMAL;
  hdma_i2c1_tx.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_i2c1_tx) != HAL_OK) {
    Error_Handler();
  }
  __HAL_LINKDMA(&hi2c1, hdmatx, hdma_i2c1_tx);

  // DMA I2C1 RX
  hdma_i2c1_rx.Instance = DMA1_Channel7;
  hdma_i2c1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_i2c1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_i2c1_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_i2c1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_i2c1_rx.Init.Mode = DMA_NORMAL;
  hdma_i2c1_rx.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_i2c1_rx) != HAL_OK) {
    Error_Handler();
  }
  __HAL_LINKDMA(&hi2c1, hdmarx, hdma_i2c1_rx);

  // Enable DMA interrupt
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

// Inisialisasi I2C1
static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
}

// Inisialisasi USART1
static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
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

// Inisialisasi GPIO
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Konfigurasi clock sistem
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

#if defined(STM32F103xB)
  // F1 series (bluepill_f103c8)
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
#else
  // F4 series (stm32f401cc, stm32f411ce)
  RCC_OscInitStruct.PLL.PLLM = 25;
#if defined(STM32F411xE)
  RCC_OscInitStruct.PLL.PLLN = 400; // 100MHz for F411
#else
  RCC_OscInitStruct.PLL.PLLN = 336; // 84MHz for F401
#endif
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
#endif

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
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

// Error handler
void Error_Handler(void) {
  __disable_irq();
  while (1) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(500);
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  // Handler assert gagal
}
#endif

// DMA interrupt handlers
void DMA1_Channel6_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_i2c1_tx);
}
void DMA1_Channel7_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}
