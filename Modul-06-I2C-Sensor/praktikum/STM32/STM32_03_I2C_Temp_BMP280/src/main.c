/**
 * ==========================================================
 *  Modul 06 - STM32_03_I2C_Temp_BMP280
 *  Membaca suhu dan tekanan dari sensor BMP280 via I2C
 * ==========================================================
 *  Deskripsi:
 *    Membaca Chip ID, kalibrasi, dan data suhu/tekanan BMP280.
 *    Menggunakan formula kompensasi resmi dari datasheet Bosch.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    BMP280 address: 0x76
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

#define BMP280_ADDR        0x76
#define BMP280_ADDR_WRITE  (BMP280_ADDR << 1)

/* Register BMP280 */
#define BMP280_REG_CHIP_ID    0xD0
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CTRL_MEAS  0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_PRESS_MSB  0xF7
#define BMP280_REG_CALIB      0x88

/* Struktur data kalibrasi BMP280 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData;

static BMP280_CalibData calib;
static int32_t t_fine; /* Variabel global untuk kompensasi silang */

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

/* =================== BMP280 Driver =================== */

/* Baca satu register dari BMP280 */
uint8_t BMP280_ReadReg(uint8_t reg) {
    uint8_t val;
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT,
                     &val, 1, HAL_MAX_DELAY);
    return val;
}

/* Tulis satu register ke BMP280 */
void BMP280_WriteReg(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT,
                      &val, 1, HAL_MAX_DELAY);
}

/* Baca data kalibrasi dari BMP280 */
void BMP280_ReadCalibration(void) {
    uint8_t data[24];
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR_WRITE, BMP280_REG_CALIB, I2C_MEMADD_SIZE_8BIT,
                     data, 24, HAL_MAX_DELAY);

    calib.dig_T1 = (uint16_t)(data[1] << 8 | data[0]);
    calib.dig_T2 = (int16_t)(data[3] << 8 | data[2]);
    calib.dig_T3 = (int16_t)(data[5] << 8 | data[4]);
    calib.dig_P1 = (uint16_t)(data[7] << 8 | data[6]);
    calib.dig_P2 = (int16_t)(data[9] << 8 | data[8]);
    calib.dig_P3 = (int16_t)(data[11] << 8 | data[10]);
    calib.dig_P4 = (int16_t)(data[13] << 8 | data[12]);
    calib.dig_P5 = (int16_t)(data[15] << 8 | data[14]);
    calib.dig_P6 = (int16_t)(data[17] << 8 | data[16]);
    calib.dig_P7 = (int16_t)(data[19] << 8 | data[18]);
    calib.dig_P8 = (int16_t)(data[21] << 8 | data[20]);
    calib.dig_P9 = (int16_t)(data[23] << 8 | data[22]);

    printf("[CALIB] T1=%u T2=%d T3=%d\r\n", calib.dig_T1, calib.dig_T2, calib.dig_T3);
    printf("[CALIB] P1=%u P2=%d P3=%d\r\n", calib.dig_P1, calib.dig_P2, calib.dig_P3);
}

/* Inisialisasi BMP280: mode normal, oversampling x16 */
void BMP280_Init(void) {
    /* Soft reset */
    BMP280_WriteReg(BMP280_REG_RESET, 0xB6);
    HAL_Delay(100);

    /* Config: standby 1000ms, filter x16 */
    BMP280_WriteReg(BMP280_REG_CONFIG, 0xA0);

    /* ctrl_meas: temp osrs x2, press osrs x16, mode normal */
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, 0x57);
}

/* Kompensasi suhu (formula Bosch) - mengembalikan suhu dalam 0.01 °C */
int32_t BMP280_CompensateTemp(int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
            ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

/* Kompensasi tekanan (formula Bosch) - mengembalikan tekanan dalam Pa (Q24.8) */
uint32_t BMP280_CompensatePress(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) +
           ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (uint32_t)p;
}

/* Baca data suhu dan tekanan dari BMP280 */
void BMP280_Read(float *suhu, float *tekanan) {
    uint8_t data[6];
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR_WRITE, BMP280_REG_PRESS_MSB, I2C_MEMADD_SIZE_8BIT,
                     data, 6, HAL_MAX_DELAY);

    /* ADC mentah: 20-bit */
    int32_t adc_P = (int32_t)((data[0] << 16) | (data[1] << 8) | data[2]) >> 4;
    int32_t adc_T = (int32_t)((data[3] << 16) | (data[4] << 8) | data[5]) >> 4;

    /* Kompensasi */
    int32_t temp_raw = BMP280_CompensateTemp(adc_T);
    uint32_t press_raw = BMP280_CompensatePress(adc_P);

    *suhu = temp_raw / 100.0f;
    *tekanan = press_raw / 256.0f / 100.0f; /* Konversi ke hPa */
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 BMP280 I2C Sensor\r\n");
    printf("  Alamat: 0x76\r\n");
    printf("===================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, BMP280_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] BMP280 tidak ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }

    /* Baca Chip ID */
    uint8_t chip_id = BMP280_ReadReg(BMP280_REG_CHIP_ID);
    printf("[INFO] Chip ID: 0x%02X (seharusnya 0x58)\r\n", chip_id);

    if (chip_id != 0x58) {
        printf("[WARN] Chip ID tidak sesuai! Mungkin BME280 (0x60)?\r\n");
    }

    /* Baca kalibrasi dan inisialisasi */
    BMP280_ReadCalibration();
    BMP280_Init();
    printf("[OK] BMP280 diinisialisasi\r\n");

    uint32_t pembacaan = 0;
    float suhu, tekanan;

    while (1) {
        pembacaan++;

        BMP280_Read(&suhu, &tekanan);

        /* Estimasi ketinggian berdasarkan tekanan (formula barometrik sederhana) */
        float ketinggian = 44330.0f * (1.0f - powf(tekanan / 1013.25f, 0.1903f));

        printf("DATA,%lu,%.2f,%.2f,%.1f\r\n", pembacaan, suhu, tekanan, ketinggian);
        printf("  Suhu     : %.2f C\r\n", suhu);
        printf("  Tekanan  : %.2f hPa\r\n", tekanan);
        printf("  Ketinggian: %.1f m\r\n", ketinggian);

        HAL_Delay(2000);
    }
}
