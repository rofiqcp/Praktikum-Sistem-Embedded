/**
 * ==========================================================
 *  Modul 06 - STM32_09_I2C_Multi_Sensor
 *  BMP280 + BH1750 pada bus I2C yang sama
 * ==========================================================
 *  Deskripsi:
 *    Membaca data dari BMP280 (suhu, tekanan) dan BH1750 (cahaya)
 *    secara bergantian pada bus I2C yang sama.
 *    Menampilkan data gabungan melalui UART.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    BMP280: 0x76, BH1750: 0x23
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

/* Alamat sensor */
#define BMP280_ADDR       0x76
#define BMP280_ADDR_W     (BMP280_ADDR << 1)
#define BH1750_ADDR       0x23
#define BH1750_ADDR_W     (BH1750_ADDR << 1)

/* Register BMP280 */
#define BMP280_REG_CHIP_ID   0xD0
#define BMP280_REG_RESET     0xE0
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG    0xF5
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_REG_CALIB     0x88

/* Perintah BH1750 */
#define BH1750_POWER_ON       0x01
#define BH1750_CONT_HRES      0x10

/* Struktur kalibrasi BMP280 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
} BMP280_Calib;

static BMP280_Calib cal;
static int32_t t_fine;

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

/* =================== BMP280 Functions =================== */

void BMP280_ReadCalib(void) {
    uint8_t d[24];
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR_W, BMP280_REG_CALIB,
                     I2C_MEMADD_SIZE_8BIT, d, 24, HAL_MAX_DELAY);
    cal.dig_T1 = (uint16_t)(d[1]<<8|d[0]);
    cal.dig_T2 = (int16_t)(d[3]<<8|d[2]);
    cal.dig_T3 = (int16_t)(d[5]<<8|d[4]);
    cal.dig_P1 = (uint16_t)(d[7]<<8|d[6]);
    cal.dig_P2 = (int16_t)(d[9]<<8|d[8]);
    cal.dig_P3 = (int16_t)(d[11]<<8|d[10]);
    cal.dig_P4 = (int16_t)(d[13]<<8|d[12]);
    cal.dig_P5 = (int16_t)(d[15]<<8|d[14]);
    cal.dig_P6 = (int16_t)(d[17]<<8|d[16]);
    cal.dig_P7 = (int16_t)(d[19]<<8|d[18]);
    cal.dig_P8 = (int16_t)(d[21]<<8|d[20]);
    cal.dig_P9 = (int16_t)(d[23]<<8|d[22]);
}

void BMP280_Init(void) {
    uint8_t val;
    val = 0xB6; /* Soft reset */
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDR_W, BMP280_REG_RESET,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
    HAL_Delay(100);
    BMP280_ReadCalib();
    val = 0xA0; /* Config: filter x16, standby 1s */
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDR_W, BMP280_REG_CONFIG,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
    val = 0x57; /* ctrl_meas: osrs_t x2, osrs_p x16, normal */
    HAL_I2C_Mem_Write(&hi2c1, BMP280_ADDR_W, BMP280_REG_CTRL_MEAS,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
}

void BMP280_Read(float *suhu, float *tekanan) {
    uint8_t d[6];
    HAL_I2C_Mem_Read(&hi2c1, BMP280_ADDR_W, BMP280_REG_PRESS_MSB,
                     I2C_MEMADD_SIZE_8BIT, d, 6, HAL_MAX_DELAY);

    int32_t adc_P = (int32_t)((d[0]<<16)|(d[1]<<8)|d[2]) >> 4;
    int32_t adc_T = (int32_t)((d[3]<<16)|(d[4]<<8)|d[5]) >> 4;

    /* Kompensasi suhu */
    int32_t v1 = ((((adc_T>>3)-((int32_t)cal.dig_T1<<1)))*((int32_t)cal.dig_T2))>>11;
    int32_t v2 = (((((adc_T>>4)-((int32_t)cal.dig_T1))*
                  ((adc_T>>4)-((int32_t)cal.dig_T1)))>>12)*((int32_t)cal.dig_T3))>>14;
    t_fine = v1 + v2;
    *suhu = ((t_fine * 5 + 128) >> 8) / 100.0f;

    /* Kompensasi tekanan */
    int64_t p1 = ((int64_t)t_fine) - 128000;
    int64_t p2 = p1 * p1 * (int64_t)cal.dig_P6;
    p2 = p2 + ((p1 * (int64_t)cal.dig_P5) << 17);
    p2 = p2 + (((int64_t)cal.dig_P4) << 35);
    p1 = ((p1 * p1 * (int64_t)cal.dig_P3) >> 8) + ((p1 * (int64_t)cal.dig_P2) << 12);
    p1 = (((((int64_t)1) << 47) + p1)) * ((int64_t)cal.dig_P1) >> 33;
    if (p1 == 0) { *tekanan = 0; return; }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - p2) * 3125) / p1;
    p1 = (((int64_t)cal.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    p2 = (((int64_t)cal.dig_P8) * p) >> 19;
    p = ((p + p1 + p2) >> 8) + (((int64_t)cal.dig_P7) << 4);
    *tekanan = (uint32_t)p / 256.0f / 100.0f;
}

/* =================== BH1750 Functions =================== */

void BH1750_Init(void) {
    uint8_t cmd = BH1750_POWER_ON;
    HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR_W, &cmd, 1, HAL_MAX_DELAY);
    HAL_Delay(10);
    cmd = BH1750_CONT_HRES;
    HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR_W, &cmd, 1, HAL_MAX_DELAY);
    HAL_Delay(180);
}

float BH1750_ReadLux(void) {
    uint8_t d[2];
    HAL_I2C_Master_Receive(&hi2c1, BH1750_ADDR_W | 1, d, 2, HAL_MAX_DELAY);
    uint16_t raw = (d[0] << 8) | d[1];
    return raw / 1.2f;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n==========================================\r\n");
    printf("  STM32 Multi Sensor I2C\r\n");
    printf("  BMP280 (0x76) + BH1750 (0x23)\r\n");
    printf("==========================================\r\n");

    /* Cek kedua sensor */
    uint8_t bmp_ok = (HAL_I2C_IsDeviceReady(&hi2c1, BMP280_ADDR_W, 3, 100) == HAL_OK);
    uint8_t bh_ok  = (HAL_I2C_IsDeviceReady(&hi2c1, BH1750_ADDR_W, 3, 100) == HAL_OK);

    printf("[%s] BMP280 @ 0x76\r\n", bmp_ok ? "OK" : "GAGAL");
    printf("[%s] BH1750 @ 0x23\r\n", bh_ok ? "OK" : "GAGAL");

    if (!bmp_ok && !bh_ok) {
        printf("[ERROR] Tidak ada sensor yang ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }

    /* Inisialisasi sensor yang tersedia */
    if (bmp_ok) {
        BMP280_Init();
        printf("[OK] BMP280 diinisialisasi\r\n");
    }
    if (bh_ok) {
        BH1750_Init();
        printf("[OK] BH1750 diinisialisasi\r\n");
    }

    uint32_t pembacaan = 0;
    float suhu = 0, tekanan = 0, lux = 0;

    while (1) {
        pembacaan++;

        /* Baca BMP280 */
        if (bmp_ok) {
            BMP280_Read(&suhu, &tekanan);
        }

        /* Baca BH1750 */
        if (bh_ok) {
            lux = BH1750_ReadLux();
        }

        /* Estimasi ketinggian */
        float ketinggian = bmp_ok ? 44330.0f * (1.0f - powf(tekanan / 1013.25f, 0.1903f)) : 0;

        /* Output data gabungan */
        printf("DATA,%lu,%.2f,%.2f,%.1f,%.1f\r\n",
               pembacaan, suhu, tekanan, lux, ketinggian);
        printf("  [BMP280] Suhu: %.2f C | Tekanan: %.2f hPa | Ketinggian: %.1f m\r\n",
               suhu, tekanan, ketinggian);
        printf("  [BH1750] Cahaya: %.1f lux\r\n", lux);

        /* Indeks kenyamanan sederhana */
        if (bmp_ok && bh_ok) {
            const char *kenyamanan;
            if (suhu >= 20 && suhu <= 26 && lux >= 200 && lux <= 1000) {
                kenyamanan = "Nyaman";
            } else if (suhu < 18 || suhu > 30) {
                kenyamanan = "Tidak nyaman (suhu)";
            } else if (lux < 50) {
                kenyamanan = "Terlalu gelap";
            } else {
                kenyamanan = "Cukup";
            }
            printf("  Kenyamanan: %s\r\n", kenyamanan);
        }

        HAL_Delay(2000);
    }
}
