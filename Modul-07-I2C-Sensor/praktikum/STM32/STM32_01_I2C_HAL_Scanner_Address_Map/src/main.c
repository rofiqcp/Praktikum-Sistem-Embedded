/**
 * ==========================================================
 *  Modul 07 - STM32_01_I2C_HAL_Scanner_Address_Map
 * ==========================================================
 *  Deskripsi:
 *    Scan alamat I2C 0x01 hingga 0x7F menggunakan HAL_I2C_IsDeviceReady.
 *    Hasil scan ditampilkan melalui USART1.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang sensor sesuai alamat
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    I2C_SCAN_START 0x01
 *    I2C_SCAN_END 0x7F
 *    SCAN_DELAY_MS 3000
 * ==========================================================
 */

#include "config.h"
#include <stdio.h>
#include <string.h>

#define I2C_SCAN_START 0x01
#define I2C_SCAN_END 0x7F
#define SCAN_DELAY_MS 3000

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Deklarasi fungsi inisialisasi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  uint8_t scan_addr;
  char msg[64];

  while (1) {
    // Tampilkan info awal scan
    sprintf(msg, "Mulai scan I2C 0x%02X - 0x%02X\r\n", I2C_SCAN_START, I2C_SCAN_END);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    // Loop scan semua alamat I2C
    for (scan_addr = I2C_SCAN_START; scan_addr <= I2C_SCAN_END; scan_addr++) {
      // Cek kesiapan device di alamat scan_addr (alamat 7-bit digeser ke kiri 1 bit)
      if (HAL_I2C_IsDeviceReady(&hi2c1, scan_addr << 1, 3, 100) == HAL_OK) {
        sprintf(msg, "Device ditemukan di alamat 0x%02X\r\n", scan_addr);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      }
    }

    // Tampilkan info selesai scan
    sprintf(msg, "Scan selesai, tunggu %d ms\r\n", SCAN_DELAY_MS);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // Toggle LED onboard
    HAL_Delay(SCAN_DELAY_MS);
  }
}

// Inisialisasi I2C1
static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000; // 100kHz I2C
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

  // Enable clock GPIO
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Konfigurasi PC13 sebagai output (LED onboard)
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Konfigurasi PB6 (SCL) dan PB7 (SDA) sebagai I2C1
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Konfigurasi PA9 (TX) dan PA10 (RX) sebagai USART1
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
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
