/**
 * ==========================================================
 *  Modul 06 - STM32_07_I2C_Light_BH1750
 *  Sensor cahaya BH1750 via I2C
 * ==========================================================
 *  Deskripsi:
 *    Membaca intensitas cahaya dalam satuan Lux dari BH1750.
 *    Menggunakan HAL_I2C_Master_Transmit() untuk mengirim perintah
 *    dan HAL_I2C_Master_Receive() untuk membaca 2 byte data.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    BH1750 address: 0x23 (ADDR pin LOW)
 * ==========================================================
 */

#ifdef STM32F1
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif

#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;
I2C_HandleTypeDef hi2c1;

#define BH1750_ADDR        0x23
#define BH1750_ADDR_WRITE  (BH1750_ADDR << 1)

/* Perintah BH1750 */
#define BH1750_POWER_ON         0x01
#define BH1750_POWER_OFF        0x00
#define BH1750_RESET            0x07
#define BH1750_CONT_HRES_MODE   0x10  /* Mode kontinu resolusi tinggi: 1 lux */
#define BH1750_CONT_HRES2_MODE  0x11  /* Mode kontinu resolusi tinggi 2: 0.5 lux */
#define BH1750_CONT_LRES_MODE   0x13  /* Mode kontinu resolusi rendah: 4 lux */
#define BH1750_ONCE_HRES_MODE   0x20  /* Mode sekali resolusi tinggi */
#define BH1750_ONCE_HRES2_MODE  0x21  /* Mode sekali resolusi tinggi 2 */
#define BH1750_ONCE_LRES_MODE   0x23  /* Mode sekali resolusi rendah */

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

/* =================== BH1750 Driver =================== */

/* Kirim perintah ke BH1750 */
HAL_StatusTypeDef BH1750_SendCmd(uint8_t cmd) {
    return HAL_I2C_Master_Transmit(&hi2c1, BH1750_ADDR_WRITE, &cmd, 1, HAL_MAX_DELAY);
}

/* Inisialisasi BH1750 */
void BH1750_Init(void) {
    BH1750_SendCmd(BH1750_POWER_ON);
    HAL_Delay(10);
    BH1750_SendCmd(BH1750_RESET);
    HAL_Delay(10);
    /* Set mode pengukuran kontinu resolusi tinggi (1 lux) */
    BH1750_SendCmd(BH1750_CONT_HRES_MODE);
    HAL_Delay(180); /* Tunggu pengukuran pertama selesai (max 180ms) */
    printf("[OK] BH1750 diinisialisasi (Mode: Continuous H-Res)\r\n");
}

/* Baca intensitas cahaya dalam Lux */
float BH1750_ReadLux(void) {
    uint8_t data[2] = {0};
    HAL_I2C_Master_Receive(&hi2c1, BH1750_ADDR_WRITE | 1, data, 2, HAL_MAX_DELAY);

    /* Nilai mentah: 16-bit (MSB first) */
    uint16_t raw = (data[0] << 8) | data[1];

    /* Konversi ke Lux: raw / 1.2 (untuk mode H-Res) */
    float lux = raw / 1.2f;
    return lux;
}

/* Ganti mode pengukuran BH1750 */
void BH1750_SetMode(uint8_t mode) {
    BH1750_SendCmd(mode);

    const char *mode_str;
    switch (mode) {
        case BH1750_CONT_HRES_MODE:  mode_str = "Continuous H-Res (1 lux)"; break;
        case BH1750_CONT_HRES2_MODE: mode_str = "Continuous H-Res2 (0.5 lux)"; break;
        case BH1750_CONT_LRES_MODE:  mode_str = "Continuous L-Res (4 lux)"; break;
        case BH1750_ONCE_HRES_MODE:  mode_str = "One-Time H-Res"; break;
        default: mode_str = "Unknown"; break;
    }
    printf("[INFO] Mode: %s\r\n", mode_str);
}

/* Klasifikasi tingkat cahaya */
const char* BH1750_KlasifikasiCahaya(float lux) {
    if (lux < 1)       return "Gelap total";
    if (lux < 10)      return "Sangat redup";
    if (lux < 50)      return "Redup";
    if (lux < 200)     return "Cahaya dalam ruangan";
    if (lux < 500)     return "Cahaya kantor";
    if (lux < 1000)    return "Mendung";
    if (lux < 10000)   return "Siang hari (teduh)";
    if (lux < 50000)   return "Sinar matahari langsung";
    return "Sangat terang";
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 BH1750 Sensor Cahaya\r\n");
    printf("  Alamat: 0x23\r\n");
    printf("===================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, BH1750_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] BH1750 tidak ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }
    printf("[OK] BH1750 terdeteksi\r\n");

    /* Inisialisasi */
    BH1750_Init();

    uint32_t pembacaan = 0;
    float lux_min = 65535.0f, lux_max = 0.0f;
    float lux_sum = 0.0f;

    while (1) {
        pembacaan++;

        float lux = BH1750_ReadLux();

        /* Statistik */
        if (lux < lux_min) lux_min = lux;
        if (lux > lux_max) lux_max = lux;
        lux_sum += lux;
        float lux_avg = lux_sum / pembacaan;

        printf("DATA,%lu,%.1f,%.1f,%.1f,%.1f\r\n",
               pembacaan, lux, lux_min, lux_max, lux_avg);
        printf("  Cahaya : %.1f lux [%s]\r\n", lux, BH1750_KlasifikasiCahaya(lux));
        printf("  Min=%.1f Max=%.1f Avg=%.1f\r\n", lux_min, lux_max, lux_avg);

        HAL_Delay(1000);
    }
}
