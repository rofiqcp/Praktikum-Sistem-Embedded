/**
 * ==========================================================
 *  Modul 07 - STM32_10_I2C_RTOS_Error_Recovery
 * ==========================================================
 *  Deskripsi:
 *    FreeRTOS task dengan watchdog/error recovery.
 *    Menunjukkan resiliensi RTOS dalam menangani error I2C.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    Sensor I2C (alamat 0x76, misal BME280)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang sensor I2C di alamat 0x76
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    WATCHDOG_TIMEOUT_MS 5000
 *    MAX_RETRY 3
 *    SENSOR_ADDR 0x76
 * ==========================================================
 */

#include "main.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

#define WATCHDOG_TIMEOUT_MS 5000
#define MAX_RETRY 3
#define SENSOR_ADDR 0x76

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Handle RTOS
TaskHandle_t xSensorTaskHandle = NULL;
TaskHandle_t xWatchdogTaskHandle = NULL;
SemaphoreHandle_t xI2CMutex = NULL;

// Variabel error tracking
volatile uint32_t error_count = 0;
volatile uint32_t recovery_count = 0;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
void StartSensorTask(void *argument);
void StartWatchdogTask(void *argument);
static void I2C_ErrorRecovery(void);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  // Buat mutex untuk akses I2C
  xI2CMutex = xSemaphoreCreateMutex();
  if (xI2CMutex == NULL) {
    Error_Handler();
  }

  // Buat task RTOS
  xTaskCreate(StartSensorTask, "SensorTask", configMINIMAL_STACK_SIZE * 2, NULL, 2, &xSensorTaskHandle);
  xTaskCreate(StartWatchdogTask, "WatchdogTask", configMINIMAL_STACK_SIZE * 2, NULL, 1, &xWatchdogTaskHandle);

  // Mulai scheduler
  vTaskStartScheduler();

  while (1) {
    Error_Handler();
  }
}

// Task sensor: baca data dengan error handling
void StartSensorTask(void *argument) {
  char msg[128];
  uint32_t retry_count;
  uint8_t sensor_data[8];
  TickType_t last_wake_time = xTaskGetTickCount();

  while (1) {
    // Coba baca sensor dengan retry
    for (retry_count = 0; retry_count < MAX_RETRY; retry_count++) {
      // Ambil mutex I2C
      if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(100)) == pdPASS) {
        // Simulasi baca data sensor (register 0xFA = temp MSB)
        HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, SENSOR_ADDR << 1, 0xFA, I2C_MEMADD_SIZE_8BIT, sensor_data, 8, 100);

        xSemaphoreGive(xI2CMutex);

        if (status == HAL_OK) {
          // Baca berhasil
          sprintf(msg, "Data sensor ok: %02X%02X\r\n", sensor_data[0], sensor_data[1]);
          HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
          break;
        }
      }

      // Error terjadi
      error_count++;
      sprintf(msg, "Error baca sensor, retry %lu/%d\r\n", retry_count + 1, MAX_RETRY);
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

      if (retry_count == MAX_RETRY - 1) {
        // Max retry tercapai, lakukan recovery
        I2C_ErrorRecovery();
        recovery_count++;
        sprintf(msg, "Recovery dilakukan, total: %lu\r\n", recovery_count);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      }

      vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Toggle LED onboard
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(2000));
  }
}

// Task watchdog: monitor error dan reset jika perlu
void StartWatchdogTask(void *argument) {
  char msg[64];
  TickType_t last_wake_time = xTaskGetTickCount();

  while (1) {
    // Cek statistik error
    if (error_count > 10) {
      sprintf(msg, "Watchdog: error_count=%lu, reset sistem?\r\n", error_count);
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      // Di aplikasi nyata, bisa trigger reset sistem di sini
    }

    sprintf(msg, "Watchdog: errors=%lu, recovery=%lu\r\n", error_count, recovery_count);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS));
  }
}

// Lakukan recovery I2C (reset bus)
static void I2C_ErrorRecovery(void) {
  // Reset I2C peripheral
  HAL_I2C_DeInit(&hi2c1);
  MX_I2C1_Init();

  // Toggle clock SCL manual untuk membersihkan bus
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Kirim 9 pulse clock untuk reset slave
  for (uint8_t i = 0; i < 9; i++) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_Delay(1);
  }

  // Kembalikan pin ke fungsi I2C
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
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
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
