/**
 * ==========================================================
 *  Modul 07 - STM32_06_DS3231_RTC_External_BCD
 * ==========================================================
 *  Deskripsi:
 *    Baca waktu dari RTC eksternal DS3231 dengan konversi BCD ke desimal.
 *    Set alarm dan tampilkan via USART1.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    DS3231 (alamat 0x68)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang DS3231 di alamat 0x68
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    DS3231_ADDR 0x68
 *    ALARM_AFTER_SEC 15
 * ==========================================================
 */

#include "config.h"
#include <string.h>

#define DS3231_ADDR 0x68
#define ALARM_AFTER_SEC 15

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Struktur waktu
typedef struct {
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
  uint8_t day;
  uint8_t date;
  uint8_t month;
  uint8_t year;
} DS3231_Time;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void DS3231_ReadTime(DS3231_Time *time);
static void DS3231_SetAlarm(uint8_t sec);
static void DS3231_ClearAlarmFlag(void);
static uint8_t BcdToDec(uint8_t bcd);
static uint8_t DecToBcd(uint8_t dec);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  DS3231_Time time;
  char msg[128];
  uint8_t alarm_set = 0;

  while (1) {
    // Baca waktu dari DS3231
    DS3231_ReadTime(&time);

    // Tampilkan waktu (format: YY-MM-DD HH:MM:SS)
    sprintf(msg, "Waktu: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
            BcdToDec(time.year), BcdToDec(time.month), BcdToDec(time.date),
            BcdToDec(time.hour), BcdToDec(time.min), BcdToDec(time.sec));
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    // Set alarm sekali saja
    if (!alarm_set) {
      DS3231_SetAlarm((BcdToDec(time.sec) + ALARM_AFTER_SEC) % 60);
      alarm_set = 1;
      sprintf(msg, "Alarm diatur %d detik lagi\r\n", ALARM_AFTER_SEC);
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    }

    // Cek flag alarm (register 0x0F bit 0)
    uint8_t status;
    HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDR << 1, 0x0F, I2C_MEMADD_SIZE_8BIT, &status, 1, 100);
    if (status & 0x01) {
      sprintf(msg, "Alarm terpicu!\r\n");
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
      DS3231_ClearAlarmFlag();
      alarm_set = 0; // Reset alarm
    }

    // Toggle LED onboard setiap detik
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(1000);
  }
}

// Baca waktu dari DS3231
static void DS3231_ReadTime(DS3231_Time *time) {
  uint8_t buf[7];
  // Baca register 0x00 (detik) hingga 0x06 (tahun)
  HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDR << 1, 0x00, I2C_MEMADD_SIZE_8BIT, buf, 7, 100);

  time->sec = buf[0];
  time->min = buf[1];
  time->hour = buf[2];
  time->day = buf[3];
  time->date = buf[4];
  time->month = buf[5];
  time->year = buf[6];
}

// Set alarm detik DS3231
static void DS3231_SetAlarm(uint8_t sec) {
  uint8_t cmd[2];
  // Set alarm 1 detik (register 0x07 - 0x0A)
  cmd[0] = 0x07; // Alarm 1 detik register
  cmd[1] = DecToBcd(sec); // Detik alarm
  HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);

  // Aktifkan alarm 1 interrupt
  cmd[0] = 0x0E; // Control register
  uint8_t ctrl;
  HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &ctrl, 1, 100);
  ctrl |= 0x01; // Alarm 1 interrupt enable
  HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &ctrl, 1, 100);
}

// Bersihkan flag alarm
static void DS3231_ClearAlarmFlag(void) {
  uint8_t status;
  HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDR << 1, 0x0F, I2C_MEMADD_SIZE_8BIT, &status, 1, 100);
  status &= ~0x01; // Clear alarm 1 flag
  HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDR << 1, 0x0F, I2C_MEMADD_SIZE_8BIT, &status, 1, 100);
}

// Konversi BCD ke desimal
static uint8_t BcdToDec(uint8_t bcd) {
  return (bcd >> 4) * 10 + (bcd & 0x0F);
}

// Konversi desimal ke BCD
static uint8_t DecToBcd(uint8_t dec) {
  return ((dec / 10) << 4) | (dec % 10);
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
