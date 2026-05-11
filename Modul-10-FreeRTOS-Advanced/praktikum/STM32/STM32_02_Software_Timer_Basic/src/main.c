#include <stdio.h>
/**
 * ==========================================================
 *  Modul 07 - STM32_08_I2C_RTOS_Multi_Task
 * ==========================================================
 *  Deskripsi:
 *    3 FreeRTOS task: Task1 baca BME280, Task2 update OLED, Task3 log EEPROM.
 *    Menunjukkan RTOS scheduling dengan I2C.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    BME280 (0x76), OLED SSD1306 (0x3C), AT24C32 (0x50)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang sensor sesuai alamat
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    BME_TASK_PRIO 2
 *    OLED_TASK_PRIO 2
 *    LOG_TASK_PRIO 1
 * ==========================================================
 */

#include "main.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

#define BME_TASK_PRIO 2
#define OLED_TASK_PRIO 2
#define LOG_TASK_PRIO 1

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Handle RTOS
TaskHandle_t xBmeTaskHandle = NULL;
TaskHandle_t xOledTaskHandle = NULL;
TaskHandle_t xLogTaskHandle = NULL;
QueueHandle_t xDataQueue = NULL; // Queue untuk kirim data BME280 ke OLED dan EEPROM

// Struktur data sensor
typedef struct {
  float temp;
  float press;
  float hum;
  uint32_t timestamp;
} SensorData;

// Deklarasi fungsi
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_USART1_UART_Init(void);
void StartBmeTask(void *argument);
void StartOledTask(void *argument);
void StartLogTask(void *argument);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  // Buat queue untuk data sensor (kapasitas 5, ukuran struktur SensorData)
  xDataQueue = xQueueCreate(5, sizeof(SensorData));
  if (xDataQueue == NULL) {
    Error_Handler();
  }

  // Buat task RTOS
  xTaskCreate(StartBmeTask, "BME_Task", configMINIMAL_STACK_SIZE * 2, NULL, BME_TASK_PRIO, &xBmeTaskHandle);
  xTaskCreate(StartOledTask, "OLED_Task", configMINIMAL_STACK_SIZE * 2, NULL, OLED_TASK_PRIO, &xOledTaskHandle);
  xTaskCreate(StartLogTask, "LOG_Task", configMINIMAL_STACK_SIZE * 2, NULL, LOG_TASK_PRIO, &xLogTaskHandle);

  // Mulai scheduler RTOS
  vTaskStartScheduler();

  // Seharusnya tidak sampai sini
  while (1) {
    Error_Handler();
  }
}

// Task 1: Baca BME280 setiap 2 detik
void StartBmeTask(void *argument) {
  SensorData data;
  uint32_t count = 0;

  while (1) {
    // Baca data BME280 (asumsikan sudah ada fungsi BME280_ReadData)
    // Di sini simulasi data untuk contoh
    data.temp = 25.5f + (count % 10) * 0.5f;
    data.press = 1013.25f;
    data.hum = 60.0f + (count % 20);
    data.timestamp = xTaskGetTickCount();

    // Kirim data ke queue
    xQueueSend(xDataQueue, &data, portMAX_DELAY);

    // Toggle LED onboard
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    count++;
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// Task 2: Update OLED dengan data dari queue
void StartOledTask(void *argument) {
  SensorData data;
  char msg[64];

  while (1) {
    // Tunggu data dari queue
    if (xQueueReceive(xDataQueue, &data, portMAX_DELAY) == pdPASS) {
      // Tampilkan di OLED (asumsikan OLED_WriteString ada)
      sprintf(msg, "T:%.1fC P:%.0fhPa", data.temp, data.press);
      // OLED_WriteString(msg); // Implementasi OLED sesuai project 2
    }
  }
}

// Task 3: Log data ke EEPROM
void StartLogTask(void *argument) {
  SensorData data;
  uint16_t eeprom_addr = 0;

  while (1) {
    // Tunggu data dari queue (peek saja, tidak hapus dari queue)
    if (xQueuePeek(xDataQueue, &data, portMAX_DELAY) == pdPASS) {
      // Log ke EEPROM (asumsikan EEPROM_WritePage ada)
      // EEPROM_WritePage(eeprom_addr, (uint8_t*)&data, sizeof(SensorData));
      eeprom_addr += sizeof(SensorData);
      if (eeprom_addr >= 4096) eeprom_addr = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// Inisialisasi I2C1
void MX_I2C1_Init(void) {
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
void MX_USART1_UART_Init(void) {
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
void MX_GPIO_Init(void) {
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
#if !defined(STM32F103xB)
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
#endif
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#if !defined(STM32F103xB)
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
#endif
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
