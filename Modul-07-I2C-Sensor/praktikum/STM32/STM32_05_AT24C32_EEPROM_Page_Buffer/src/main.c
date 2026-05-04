/**
 * ==========================================================
 *  Modul 07 - STM32_05_AT24C32_EEPROM_Page_Buffer
 * ==========================================================
 *  Deskripsi:
 *    Baca/tulis EEPROM AT24C32 menggunakan page write/read.
 *    Menguji penulisan TEST_RECORDS record data.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    AT24C32 (alamat 0x50)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang AT24C32 di alamat 0x50
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    EEPROM_ADDR 0x50
 *    PAGE_SIZE 32
 *    TEST_RECORDS 100
 * ==========================================================
 */

#include "config.h"
#include <stdio.h>
#include <string.h>

#define EEPROM_ADDR 0x50
#define PAGE_SIZE 32
#define TEST_RECORDS 100

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Buffer data
uint8_t write_buf[PAGE_SIZE + 2]; // 2 byte alamat + PAGE_SIZE data
uint8_t read_buf[PAGE_SIZE];
uint16_t current_addr = 0;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void EEPROM_WritePage(uint16_t mem_addr, uint8_t *data, uint8_t len);
static void EEPROM_ReadPage(uint16_t mem_addr, uint8_t *data, uint8_t len);
static void EEPROM_WriteByte(uint16_t mem_addr, uint8_t data);
static uint8_t EEPROM_ReadByte(uint16_t mem_addr);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  char msg[128];
  uint8_t test_data[PAGE_SIZE];
  uint8_t read_data[PAGE_SIZE];
  uint8_t pass_count = 0;

  // Inisialisasi data uji
  for (uint8_t i = 0; i < PAGE_SIZE; i++) {
    test_data[i] = i + 1;
  }

  while (1) {
    sprintf(msg, "Mulai uji EEPROM, records: %d\r\n", TEST_RECORDS);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    pass_count = 0;
    current_addr = 0;

    // Uji penulisan dan pembacaan per halaman
    for (uint16_t rec = 0; rec < TEST_RECORDS; rec++) {
      // Tulis halaman penuh
      EEPROM_WritePage(current_addr, test_data, PAGE_SIZE);
      HAL_Delay(10); // Tunggu penulisan selesai

      // Baca halaman yang ditulis
      EEPROM_ReadPage(current_addr, read_data, PAGE_SIZE);

      // Verifikasi data
      uint8_t match = 1;
      for (uint8_t i = 0; i < PAGE_SIZE; i++) {
        if (read_data[i] != test_data[i]) {
          match = 0;
          break;
        }
      }

      if (match) {
        pass_count++;
      } else {
        sprintf(msg, "Gagal di record %d, alamat 0x%04X\r\n", rec, current_addr);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      }

      current_addr += PAGE_SIZE;
      if (current_addr >= 4096) { // AT24C32 kapasitas 4KB
        current_addr = 0;
      }
    }

    sprintf(msg, "Uji selesai: %d/%d record lulus\r\n", pass_count, TEST_RECORDS);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    // Toggle LED onboard
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(5000);
  }
}

// Tulis satu halaman EEPROM (maks PAGE_SIZE byte)
static void EEPROM_WritePage(uint16_t mem_addr, uint8_t *data, uint8_t len) {
  if (len > PAGE_SIZE) len = PAGE_SIZE;

  // Susun buffer: 2 byte alamat + data
  write_buf[0] = (mem_addr >> 8) & 0xFF; // Alamat tinggi
  write_buf[1] = mem_addr & 0xFF; // Alamat rendah
  memcpy(&write_buf[2], data, len);

  // Kirim data via I2C
  HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR << 1, write_buf, len + 2, 100);
}

// Baca satu halaman EEPROM
static void EEPROM_ReadPage(uint16_t mem_addr, uint8_t *data, uint8_t len) {
  if (len > PAGE_SIZE) len = PAGE_SIZE;

  // Kirim alamat memori yang akan dibaca
  uint8_t addr_buf[2];
  addr_buf[0] = (mem_addr >> 8) & 0xFF;
  addr_buf[1] = mem_addr & 0xFF;
  HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR << 1, addr_buf, 2, 100);

  // Baca data dari EEPROM
  HAL_I2C_Master_Receive(&hi2c1, EEPROM_ADDR << 1, data, len, 100);
}

// Tulis satu byte EEPROM
static void EEPROM_WriteByte(uint16_t mem_addr, uint8_t data) {
  uint8_t buf[3];
  buf[0] = (mem_addr >> 8) & 0xFF;
  buf[1] = mem_addr & 0xFF;
  buf[2] = data;
  HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR << 1, buf, 3, 100);
}

// Baca satu byte EEPROM
static uint8_t EEPROM_ReadByte(uint16_t mem_addr) {
  uint8_t addr_buf[2];
  addr_buf[0] = (mem_addr >> 8) & 0xFF;
  addr_buf[1] = mem_addr & 0xFF;
  HAL_I2C_Master_Transmit(&hi2c1, EEPROM_ADDR << 1, addr_buf, 2, 100);

  uint8_t data;
  HAL_I2C_Master_Receive(&hi2c1, EEPROM_ADDR << 1, &data, 1, 100);
  return data;
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
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
  uint32_t flash_latency;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

#if defined(STM32F401xC) || defined(STM32F411xE)
  // F4 (F401/F411) configuration
  RCC_OscInitStruct.PLL.PLLM = 25;
#if defined(STM32F401xC)
  RCC_OscInitStruct.PLL.PLLN = 336; // 25MHz/25 * 336 = 336MHz VCO, /4 = 84MHz
  flash_latency = FLASH_LATENCY_2;
#else
  RCC_OscInitStruct.PLL.PLLN = 400; // 25MHz/25 * 400 = 400MHz VCO, /4 = 100MHz
  flash_latency = FLASH_LATENCY_3;
#endif
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
#else
  // F1 (Bluepill) configuration
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  flash_latency = FLASH_LATENCY_2;
#endif

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flash_latency) != HAL_OK) {
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
