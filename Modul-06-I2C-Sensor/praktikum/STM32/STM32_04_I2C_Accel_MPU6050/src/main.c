/**
 * ==========================================================
 *  Modul 06 - STM32_04_I2C_Accel_MPU6050
 *  Membaca akselerometer dan giroskop MPU6050 via I2C
 * ==========================================================
 *  Deskripsi:
 *    Membaca WHO_AM_I, wake up, baca data akselerometer dan giroskop.
 *    Konversi ke satuan g dan dps (degrees per second).
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    MPU6050 address: 0x68
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

#define MPU6050_ADDR        0x68
#define MPU6050_ADDR_WRITE  (MPU6050_ADDR << 1)

/* Register MPU6050 */
#define MPU6050_REG_WHO_AM_I    0x75
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_PWR_MGMT_2  0x6C
#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_TEMP_OUT_H   0x41
#define MPU6050_REG_GYRO_XOUT_H  0x43

/* Faktor konversi */
#define ACCEL_SENSITIVITY_2G   16384.0f  /* LSB/g untuk +-2g */
#define GYRO_SENSITIVITY_250   131.0f    /* LSB/dps untuk +-250dps */

/* Struktur data IMU */
typedef struct {
    float ax, ay, az;  /* Akselerasi dalam g */
    float gx, gy, gz;  /* Kecepatan sudut dalam dps */
    float temp;        /* Suhu dalam Celcius */
} MPU6050_Data;

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void SystemClock_Config(void) {
#ifdef STM32F1
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#else
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8;
    osc.PLL.PLLN = 336;
    osc.PLL.PLLP = RCC_PLLP_DIV4;
    osc.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
#endif
}

void MX_USART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_9; g.Mode = GPIO_MODE_AF_PP; g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    g.Pin = GPIO_PIN_10; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
#else
    g.Pin = GPIO_PIN_9|GPIO_PIN_10; g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_HIGH; g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &g);
#endif
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

void MX_I2C1_Init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
#ifdef STM32F1
    g.Pin = GPIO_PIN_6|GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);
#else
    g.Pin = GPIO_PIN_6|GPIO_PIN_7;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &g);
#endif
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/* =================== MPU6050 Driver =================== */

/* Baca satu register */
uint8_t MPU6050_ReadReg(uint8_t reg) {
    uint8_t val;
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT,
                     &val, 1, HAL_MAX_DELAY);
    return val;
}

/* Tulis satu register */
void MPU6050_WriteReg(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT,
                      &val, 1, HAL_MAX_DELAY);
}

/* Inisialisasi MPU6050 */
int MPU6050_Init(void) {
    /* Cek WHO_AM_I */
    uint8_t who = MPU6050_ReadReg(MPU6050_REG_WHO_AM_I);
    printf("[INFO] WHO_AM_I: 0x%02X (seharusnya 0x68)\r\n", who);

    if (who != 0x68) {
        printf("[ERROR] MPU6050 tidak teridentifikasi!\r\n");
        return -1;
    }

    /* Wake up dari sleep mode (reset bit SLEEP) */
    MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00);
    HAL_Delay(100);

    /* Set sample rate divider: 1kHz / (1+7) = 125Hz */
    MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x07);

    /* DLPF config: bandwidth 44Hz */
    MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03);

    /* Akselerometer: +-2g */
    MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00);

    /* Giroskop: +-250 dps */
    MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00);

    printf("[OK] MPU6050 diinisialisasi\r\n");
    return 0;
}

/* Baca semua data sensor (accel + temp + gyro = 14 bytes) */
void MPU6050_ReadAll(MPU6050_Data *data) {
    uint8_t buf[14];
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR_WRITE, MPU6050_REG_ACCEL_XOUT_H,
                     I2C_MEMADD_SIZE_8BIT, buf, 14, HAL_MAX_DELAY);

    /* Akselerometer mentah (16-bit signed, big-endian) */
    int16_t raw_ax = (int16_t)(buf[0] << 8 | buf[1]);
    int16_t raw_ay = (int16_t)(buf[2] << 8 | buf[3]);
    int16_t raw_az = (int16_t)(buf[4] << 8 | buf[5]);

    /* Suhu mentah */
    int16_t raw_temp = (int16_t)(buf[6] << 8 | buf[7]);

    /* Giroskop mentah */
    int16_t raw_gx = (int16_t)(buf[8] << 8 | buf[9]);
    int16_t raw_gy = (int16_t)(buf[10] << 8 | buf[11]);
    int16_t raw_gz = (int16_t)(buf[12] << 8 | buf[13]);

    /* Konversi ke satuan fisik */
    data->ax = raw_ax / ACCEL_SENSITIVITY_2G;
    data->ay = raw_ay / ACCEL_SENSITIVITY_2G;
    data->az = raw_az / ACCEL_SENSITIVITY_2G;

    data->gx = raw_gx / GYRO_SENSITIVITY_250;
    data->gy = raw_gy / GYRO_SENSITIVITY_250;
    data->gz = raw_gz / GYRO_SENSITIVITY_250;

    /* Formula suhu dari datasheet */
    data->temp = (raw_temp / 340.0f) + 36.53f;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 MPU6050 I2C IMU\r\n");
    printf("  Alamat: 0x68\r\n");
    printf("===================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] MPU6050 tidak ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }

    /* Inisialisasi */
    if (MPU6050_Init() != 0) {
        while (1) { HAL_Delay(1000); }
    }

    MPU6050_Data imu;
    uint32_t pembacaan = 0;

    while (1) {
        pembacaan++;
        MPU6050_ReadAll(&imu);

        /* Hitung sudut kemiringan (roll & pitch) dari akselerometer */
        float roll  = atan2f(imu.ay, imu.az) * 180.0f / 3.14159f;
        float pitch = atan2f(-imu.ax, sqrtf(imu.ay * imu.ay + imu.az * imu.az)) * 180.0f / 3.14159f;

        printf("DATA,%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.1f\r\n",
               pembacaan, imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz, imu.temp);
        printf("  Accel: X=%.3fg Y=%.3fg Z=%.3fg\r\n", imu.ax, imu.ay, imu.az);
        printf("  Gyro : X=%.2f Y=%.2f Z=%.2f dps\r\n", imu.gx, imu.gy, imu.gz);
        printf("  Suhu : %.1f C\r\n", imu.temp);
        printf("  Roll : %.1f  Pitch: %.1f\r\n", roll, pitch);

        HAL_Delay(500);
    }
}
