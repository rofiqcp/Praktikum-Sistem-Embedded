/**
 * ==========================================================
 *  Modul 07 - STM32_04_MPU6050_IMU_Interrupt_Ready
 * ==========================================================
 *  Deskripsi:
 *    Baca data akselerometer dan giroskop MPU6050 dengan interrupt ready.
 *    Menggunakan konfigurasi interrupt untuk sinyal data siap.
 *  Hardware:
 *    STM32F103C8 / STM32F401CC / STM32F411CE
 *    MPU6050 (alamat 0x68)
 *  Koneksi Pin:
 *    PB6 = SCL (I2C1), PB7 = SDA (I2C1)
 *    PA9 = TX (USART1), PA10 = RX (USART1)
 *    PC13 = LED Onboard
 *    PB0 = INT (MPU6050 interrupt)
 *  Instruksi:
 *    1. Pasang MPU6050 di alamat 0x68, hubungkan pin INT ke PB0
 *    2. Ubah nilai #define untuk variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    MPU6050_ADDR 0x68
 *    ACCEL_SENSITIVITY 16384
 * ==========================================================
 */

#include "config.h"
#include <stdio.h>
#include <string.h>

#define MPU6050_ADDR 0x68
#define ACCEL_SENSITIVITY 16384 // 16384 LSB/g untuk rentang ±2g

// Handle peripheral
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
EXTI_HandleTypeDef hexti0; // Handle EXTI untuk interrupt PB0

// Variabel global untuk data IMU
volatile uint8_t mpu_int_flag = 0;
int16_t accel_x, accel_y, accel_z;
int16_t gyro_x, gyro_y, gyro_z;

// Deklarasi fungsi
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MPU6050_Init(void);
static void MPU6050_ReadData(void);
static void MPU6050_EnableInterrupt(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

int main(void) {
  // Inisialisasi HAL dan peripheral
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  // Inisialisasi MPU6050 dan aktifkan interrupt
  MPU6050_Init();
  MPU6050_EnableInterrupt();

  char msg[128];

  while (1) {
    // Tunggu interrupt dari MPU6050
    if (mpu_int_flag) {
      mpu_int_flag = 0;
      // Baca data sensor
      MPU6050_ReadData();

      // Hitung akselerasi dalam g
      float accel_x_g = (float)accel_x / ACCEL_SENSITIVITY;
      float accel_y_g = (float)accel_y / ACCEL_SENSITIVITY;
      float accel_z_g = (float)accel_z / ACCEL_SENSITIVITY;

      // Tampilkan data via USART1
      sprintf(msg, "Accel (g): X=%.2f, Y=%.2f, Z=%.2f\r\n", accel_x_g, accel_y_g, accel_z_g);
      HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

      // Toggle LED onboard
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
    HAL_Delay(10);
  }
}

// Inisialisasi MPU6050
static void MPU6050_Init(void) {
  uint8_t cmd[2];

  // Keluarkan dari sleep mode, set clock source PLL
  cmd[0] = 0x6B; // PWR_MGMT_1 register
  cmd[1] = 0x00;
  HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);

  // Set rentang akselerometer ±2g
  cmd[0] = 0x1B; // ACCEL_CONFIG register
  cmd[1] = 0x00; // ±2g
  HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);

  // Set rentang giroskop ±250 °/s
  cmd[0] = 0x1B; // GYRO_CONFIG register
  cmd[1] = 0x00; // ±250 °/s
  HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);
}

// Aktifkan interrupt data ready MPU6050
static void MPU6050_EnableInterrupt(void) {
  uint8_t cmd[2];
  // Aktifkan interrupt data ready
  cmd[0] = 0x38; // INT_ENABLE register
  cmd[1] = 0x01; // Data ready interrupt
  HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);

  // Konfigurasi pin INT sebagai push-pull
  cmd[0] = 0x37; // INT_PIN_CFG register
  cmd[1] = 0x00; // INT pin active high, push-pull
  HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR << 1, cmd[0], I2C_MEMADD_SIZE_8BIT, &cmd[1], 1, 100);
}

// Baca data akselerometer dan giroskop
static void MPU6050_ReadData(void) {
  uint8_t data_buf[14];
  // Baca data dari register 0x3B (ACCEL_XOUT_H)
  HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR << 1, 0x3B, I2C_MEMADD_SIZE_8BIT, data_buf, 14, 100);

  // Parse data akselerometer (big-endian)
  accel_x = (data_buf[0] << 8) | data_buf[1];
  accel_y = (data_buf[2] << 8) | data_buf[3];
  accel_z = (data_buf[4] << 8) | data_buf[5];

  // Parse data giroskop (big-endian)
  gyro_x = (data_buf[8] << 8) | data_buf[9];
  gyro_y = (data_buf[10] << 8) | data_buf[11];
  gyro_z = (data_buf[12] << 8) | data_buf[13];
}

// Callback interrupt EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == GPIO_PIN_0) { // Pin PB0 (MPU6050 INT)
    mpu_int_flag = 1;
  }
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

  // Konfigurasi PB0 sebagai input interrupt (MPU6050 INT)
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // Interrupt naik
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Enable interrupt EXTI0
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
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

// Interrupt handler EXTI0
void EXTI0_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}
