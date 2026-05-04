/**
 * ==========================================================
 *  Modul 07 - STM32_07_Internal_RTC_Backup_Register
 * ==========================================================
 *  Deskripsi:
 *    Gunakan RTC internal STM32 dan backup register untuk menyimpan data.
 *    Tampilkan waktu RTC dan data backup via USART1.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pastikan battery backup RTC terpasang (CR2032)
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    RTC_WAKEUP_SEC 10
 *    BACKUP_REG RTC_BKP_DR0
 * ==========================================================
 */

#include "config.h"
#include <stdio.h>
#include <string.h>

#define RTC_WAKEUP_SEC 10
#if defined(STM32F103xB)
// F1: backup register number (0-9), not a define
#define BACKUP_REG 0
#else
#define BACKUP_REG RTC_BKP_DR0
#endif

// Handle peripheral
RTC_HandleTypeDef hrtc;
UART_HandleTypeDef huart1;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
static void RTC_WakeUpCallback(void);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();

  char msg[128];
  uint32_t backup_data = 0;

  // Baca data backup register
#if defined(STM32F103xB)
  backup_data = HAL_RTCEx_BKUPRead(&hrtc, BACKUP_REG);
#else
  backup_data = HAL_RTCEx_BKUPRead(&hrtc, BACKUP_REG);
#endif

  snprintf(msg, sizeof(msg), "Data backup: %lu\r\n", backup_data);
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

  // Tulis data baru ke backup register
  backup_data++;
#if defined(STM32F103xB)
  HAL_RTCEx_BKUPWrite(&hrtc, BACKUP_REG, backup_data);
#else
  HAL_RTCEx_BKUPWrite(&hrtc, BACKUP_REG, backup_data);
#endif

  snprintf(msg, sizeof(msg), "Data backup diupdate: %lu\r\n", backup_data);
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

  // Konfigurasi wakeup timer RTC (hanya untuk F4)
#if !defined(STM32F103xB)
  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, RTC_WAKEUP_SEC * 1000 / 100, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
#endif

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

  while (1) {
    // Baca waktu RTC
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Tampilkan waktu dan tanggal
    snprintf(msg, sizeof(msg), "Waktu: %02d:%02d:%02d %02d/%02d/20%02d\r\n",
            sTime.Hours, sTime.Minutes, sTime.Seconds,
            sDate.Date, sDate.Month, sDate.Year);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    // Toggle LED onboard
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(1000);
  }
}

// Callback wakeup timer RTC
void RTC_WakeUpCallback(void) {
  // Handler wakeup timer (opsional)
}

// Inisialisasi RTC
static void MX_RTC_Init(void) {
  hrtc.Instance = RTC;
#if defined(STM32F103xB)
  // F1 series
  hrtc.Init.AsynchPrediv = 0x7F; // 32.768kHz / (127+1) = 256Hz
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
#else
  // F4 series
  hrtc.Init.AsynchPrediv = 0x7F; // 32.768kHz / (127+1) = 256Hz
  hrtc.Init.SynchPrediv = 0xFF;   // 256Hz / (255+1) = 1Hz
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
#endif
  if (HAL_RTC_Init(&hrtc) != HAL_OK) {
    Error_Handler();
  }

  // Set waktu awal (12:00:00)
  RTC_TimeTypeDef sTime = {0};
  sTime.Hours = 12;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
    Error_Handler();
  }

  // Set tanggal awal (1/1/2024)
  RTC_DateTypeDef sDate = {0};
  sDate.Date = 1;
  sDate.Month = 1;
  sDate.Year = 24;
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
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

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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

#if defined(STM32F103xB)
  // Enable RTC clock for F1
  __HAL_RCC_RTC_ENABLE();
#endif
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

// RTC wakeup interrupt handler
void RTC_WKUP_IRQHandler(void) {
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}
