/**
 * ==========================================================
 *  Modul 07 - STM32_03_BME280_Register_Driver
 * ==========================================================
 *  Deskripsi:
 *    Driver BME280 level register dengan kompensasi suhu, tekanan, kelembaban.
 *    Membaca data sensor via I2C dan menampilkan via USART1.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    BME280 (alamat 0x76)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *  Instruksi:
 *    1. Pasang BME280 di alamat 0x76
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    BME280_ADDR 0x76
 *    READ_INTERVAL_MS 2000
 * ==========================================================
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BME280_ADDR 0x76
#define READ_INTERVAL_MS 2000

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

// Register BME280
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_HUM_MSB 0xFD
#define BME280_REG_CALIB_00 0x88
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CTRL_HUM 0xF2

// Struktur kompensasi BME280
typedef struct {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
  uint8_t dig_H1;
  int16_t dig_H2;
  uint8_t dig_H3;
  int16_t dig_H4;
  int16_t dig_H5;
  int8_t dig_H6;
} BME280_CalibData;

BME280_CalibData calib_data;
int32_t t_fine;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void BME280_Init(void);
static void BME280_ReadCalibData(void);
static void BME280_ReadData(float *temp, float *press, float *hum);
static float BME280_CompensateTemp(int32_t adc_T);
static float BME280_CompensatePress(int32_t adc_P);
static float BME280_CompensateHum(int32_t adc_H);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  // Inisialisasi BME280
  BME280_Init();
  BME280_ReadCalibData();

  char msg[128];
  float temp, press, hum;

  while (1) {
    // Baca data sensor BME280
    BME280_ReadData(&temp, &press, &hum);

    // Tampilkan data via USART1
    sprintf(msg, "Suhu: %.2f C, Tekanan: %.2f hPa, Kelembaban: %.2f %%\r\n", temp, press / 100.0f, hum);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    // Toggle LED onboard
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(READ_INTERVAL_MS);
  }
}

// Inisialisasi BME280
static void BME280_Init(void) {
  uint8_t cmd[2];

  // Set oversampling suhu x1, tekanan x1, mode normal
  cmd[0] = BME280_REG_CTRL_MEAS;
  cmd[1] = 0x27; // Temp x1, Press x1, Mode normal
  HAL_I2C_Mem_Write(&hi2c1, BME280_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);

  // Set oversampling kelembaban x1
  cmd[0] = BME280_REG_CTRL_HUM;
  cmd[1] = 0x01; // Humidity x1
  HAL_I2C_Mem_Write(&hi2c1, BME280_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);
}

// Baca data kalibrasi BME280
static void BME280_ReadCalibData(void) {
  uint8_t calib_buf[26];
  // Baca kalibrasi suhu dan tekanan (0x88 - 0xA1)
  HAL_I2C_Mem_Read(&hi2c1, BME280_ADDR << 1, BME280_REG_CALIB_00, I2C_MEMADD_SIZE_8BIT, calib_buf, 26, 100);

  // Parse data kalibrasi
  calib_data.dig_T1 = (calib_buf[1] << 8) | calib_buf[0];
  calib_data.dig_T2 = (calib_buf[3] << 8) | calib_buf[2];
  calib_data.dig_T3 = (calib_buf[5] << 8) | calib_buf[4];
  calib_data.dig_P1 = (calib_buf[7] << 8) | calib_buf[6];
  calib_data.dig_P2 = (calib_buf[9] << 8) | calib_buf[8];
  calib_data.dig_P3 = (calib_buf[11] << 8) | calib_buf[10];
  calib_data.dig_P4 = (calib_buf[13] << 8) | calib_buf[12];
  calib_data.dig_P5 = (calib_buf[15] << 8) | calib_buf[14];
  calib_data.dig_P6 = (calib_buf[17] << 8) | calib_buf[16];
  calib_data.dig_P7 = (calib_buf[19] << 8) | calib_buf[18];
  calib_data.dig_P8 = (calib_buf[21] << 8) | calib_buf[20];
  calib_data.dig_P9 = (calib_buf[23] << 8) | calib_buf[22];
  calib_data.dig_H1 = calib_buf[25];

  // Baca kalibrasi kelembaban (0xE1 - 0xE7)
  uint8_t hum_calib[7];
  HAL_I2C_Mem_Read(&hi2c1, BME280_ADDR << 1, 0xE1, I2C_MEMADD_SIZE_8BIT, hum_calib, 7, 100);
  calib_data.dig_H2 = (hum_calib[1] << 8) | hum_calib[0];
  calib_data.dig_H3 = hum_calib[2];
  calib_data.dig_H4 = (hum_calib[3] << 4) | (hum_calib[4] & 0x0F);
  calib_data.dig_H5 = (hum_calib[5] << 4) | (hum_calib[4] >> 4);
  calib_data.dig_H6 = hum_calib[6];
}

// Baca data suhu, tekanan, kelembaban
static void BME280_ReadData(float *temp, float *press, float *hum) {
  uint8_t data_buf[8];
  // Baca data tekanan, suhu, kelembaban (0xF7 - 0xFE)
  HAL_I2C_Mem_Read(&hi2c1, BME280_ADDR << 1, BME280_REG_PRESS_MSB, I2C_MEMADD_SIZE_8BIT, data_buf, 8, 100);

  // Parse ADC values
  int32_t adc_P = (data_buf[0] << 12) | (data_buf[1] << 4) | (data_buf[2] >> 4);
  int32_t adc_T = (data_buf[3] << 12) | (data_buf[4] << 4) | (data_buf[5] >> 4);
  int32_t adc_H = (data_buf[6] << 8) | data_buf[7];

  // Kompensasi data
  *temp = BME280_CompensateTemp(adc_T);
  *press = BME280_CompensatePress(adc_P);
  *hum = BME280_CompensateHum(adc_H);
}

// Kompensasi suhu
static float BME280_CompensateTemp(int32_t adc_T) {
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) *
          ((int32_t)calib_data.dig_T3)) >> 14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T / 100.0f;
}

// Kompensasi tekanan
static float BME280_CompensatePress(int32_t adc_P) {
  int64_t var1, var2, p;
  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
  var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
  var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + ((var1 * (int64_t)calib_data.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
  if (var1 == 0) return 0; // Hindari division by zero
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
  return (float)p / 256.0f;
}

// Kompensasi kelembaban
static float BME280_CompensateHum(int32_t adc_H) {
  int32_t v_x1_u32r;
  v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - (((int32_t)calib_data.dig_H5) * v_x1_u32r)) +
                 ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)calib_data.dig_H6)) >> 10) * (((v_x1_u32r *
                 ((int32_t)calib_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
                 ((int32_t)calib_data.dig_H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib_data.dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (float)(v_x1_u32r >> 12) / 1024.0f;
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
