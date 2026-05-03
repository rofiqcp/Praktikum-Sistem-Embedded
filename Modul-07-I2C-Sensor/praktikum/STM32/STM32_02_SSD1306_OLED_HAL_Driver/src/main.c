/**
 * ==========================================================
 *  Modul 07 - STM32_02_SSD1306_OLED_HAL_Driver
 * ==========================================================
 *  Deskripsi:
 *    Kontrol OLED SSD1306 via I2C menggunakan HAL_I2C_Mem_Write.
 *    Menampilkan teks sederhana di layar OLED.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    OLED SSD1306 (alamat 0x3C)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang OLED SSD1306 di alamat 0x3C
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    OLED_ADDR 0x3C
 *    OLED_FLIP 0
 * ==========================================================
 */

#include "config.h"
#include <stdio.h>
#include <string.h>

#define OLED_ADDR 0x3C
#define OLED_FLIP 0

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Fungsi inisialisasi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void OLED_Init(void);
static void OLED_Clear(void);
static void OLED_SetCursor(uint8_t col, uint8_t row);
static void OLED_WriteString(char *str);

// Command & data buffer OLED
uint8_t oled_buf[1024]; // Buffer frame OLED 128x64
uint8_t oled_cmd[2];

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  // Inisialisasi OLED
  OLED_Init();
  OLED_Clear();

  char msg[64];
  uint8_t counter = 0;

  while (1) {
    // Tampilkan teks di OLED
    OLED_SetCursor(0, 0);
    OLED_WriteString("Modul 07 - OLED");
    OLED_SetCursor(0, 1);
    sprintf(msg, "Counter: %d", counter++);
    OLED_WriteString(msg);
    OLED_SetCursor(0, 2);
    OLED_WriteString("Alamat: 0x3C");

    // Toggle LED onboard
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(1000);
  }
}

// Inisialisasi OLED SSD1306
static void OLED_Init(void) {
  // Daftar inisialisasi command OLED
  uint8_t init_cmds[] = {
    0xAE, // Display off
    0xD5, 0x80, // Set display clock divider
    0xA8, 0x3F, // Set multiplex ratio (1/64)
    0xD3, 0x00, // Set display offset
    0x40, // Set start line
    0x8D, 0x14, // Charge pump on
    0x20, 0x00, // Memory mode: horizontal
    0xA1, // Segment remap
    0xC8, // COM output scan direction
    0xDA, 0x12, // COM pins hardware config
    0x81, 0xCF, // Contrast control
    0xD9, 0xF1, // Pre-charge period
    0xDB, 0x40, // VCOMH deselect level
    0xA4, // Entire display on
    0xA6, // Normal display
    0xAF  // Display on
  };

  // Kirim command inisialisasi
  for (uint8_t i = 0; i < sizeof(init_cmds); i++) {
    oled_cmd[0] = 0x00; // Command byte
    oled_cmd[1] = init_cmds[i];
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
  }

  // Jika OLED_FLIP diaktifkan, flip layar
  if (OLED_FLIP) {
    oled_cmd[0] = 0x00;
    oled_cmd[1] = 0xA0; // Segment remap flip
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
    oled_cmd[1] = 0xC0; // COM scan flip
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
  }
}

// Bersihkan layar OLED
static void OLED_Clear(void) {
  memset(oled_buf, 0x00, sizeof(oled_buf));
  // Kirim buffer frame ke OLED
  oled_cmd[0] = 0x00; // Command byte
  oled_cmd[1] = 0x21; // Set column address
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
  oled_cmd[1] = 0x00; // Start column 0
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
  oled_cmd[1] = 0x7F; // End column 127
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);

  oled_cmd[1] = 0x22; // Set page address
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
  oled_cmd[1] = 0x00; // Start page 0
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);
  oled_cmd[1] = 0x07; // End page 7
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, oled_cmd, 2, 100);

  // Kirim buffer data ke OLED
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x40, I2C_MEMADD_SIZE_8BIT, oled_buf, sizeof(oled_buf), 1000);
}

// Set kursor OLED (kolom, baris)
static void OLED_SetCursor(uint8_t col, uint8_t row) {
  // Set column address
  uint8_t cmd[3] = {0x00, 0x21, col};
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, cmd, 3, 100);
  cmd[1] = 0x7F; // End column
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, cmd, 3, 100);

  // Set page address (row)
  cmd[1] = 0x22;
  cmd[2] = row;
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, cmd, 3, 100);
  cmd[2] = 0x07;
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, cmd, 3, 100);
}

// Tulis string ke OLED
static void OLED_WriteString(char *str) {
  while (*str) {
    // Karakter ASCII 8x8 font (sederhana)
    uint8_t char_buf[8] = {0};
    // Contoh font karakter (hanya huruf besar dan angka)
    // Implementasi font sederhana, atau gunakan library font
    // Di sini asumsikan karakter 8x8
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR << 1, 0x40, I2C_MEMADD_SIZE_8BIT, char_buf, 8, 100);
    str++;
  }
}

// Inisialisasi I2C1 (sama seperti project 1)
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

// Inisialisasi USART1 (sama seperti project 1)
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

// Inisialisasi GPIO (sama seperti project 1)
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

// Konfigurasi clock sistem (sama seperti project 1)
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
