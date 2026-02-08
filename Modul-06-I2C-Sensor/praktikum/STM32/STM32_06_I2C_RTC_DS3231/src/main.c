/**
 * ==========================================================
 *  Modul 06 - STM32_06_I2C_RTC_DS3231
 *  Jam waktu nyata (RTC) DS3231 via I2C
 * ==========================================================
 *  Deskripsi:
 *    Set dan baca waktu DS3231. BCD conversion helpers.
 *    Baca suhu internal. Set alarm.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    DS3231 address: 0x68
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

#define DS3231_ADDR        0x68
#define DS3231_ADDR_WRITE  (DS3231_ADDR << 1)

/* Register DS3231 */
#define DS3231_REG_SECONDS  0x00
#define DS3231_REG_MINUTES  0x01
#define DS3231_REG_HOURS    0x02
#define DS3231_REG_DAY      0x03
#define DS3231_REG_DATE     0x04
#define DS3231_REG_MONTH    0x05
#define DS3231_REG_YEAR     0x06
#define DS3231_REG_ALARM1   0x07
#define DS3231_REG_ALARM2   0x0B
#define DS3231_REG_CONTROL  0x0E
#define DS3231_REG_STATUS   0x0F
#define DS3231_REG_TEMP_MSB 0x11
#define DS3231_REG_TEMP_LSB 0x12

/* Struktur waktu */
typedef struct {
    uint8_t detik;
    uint8_t menit;
    uint8_t jam;
    uint8_t hari;     /* 1=Senin ... 7=Minggu */
    uint8_t tanggal;
    uint8_t bulan;
    uint8_t tahun;    /* 0-99 (ditambah 2000) */
} DS3231_Time;

/* Nama hari dalam bahasa Indonesia */
static const char *nama_hari[] = {
    "", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu", "Minggu"
};

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

/* =================== BCD Helpers =================== */

/* Konversi desimal ke BCD */
uint8_t Dec2BCD(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

/* Konversi BCD ke desimal */
uint8_t BCD2Dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

/* =================== DS3231 Driver =================== */

/* Baca satu register */
uint8_t DS3231_ReadReg(uint8_t reg) {
    uint8_t val;
    HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT,
                     &val, 1, HAL_MAX_DELAY);
    return val;
}

/* Tulis satu register */
void DS3231_WriteReg(uint8_t reg, uint8_t val) {
    HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDR_WRITE, reg, I2C_MEMADD_SIZE_8BIT,
                      &val, 1, HAL_MAX_DELAY);
}

/* Set waktu ke DS3231 */
void DS3231_SetTime(DS3231_Time *t) {
    uint8_t data[7];
    data[0] = Dec2BCD(t->detik);
    data[1] = Dec2BCD(t->menit);
    data[2] = Dec2BCD(t->jam);       /* Format 24 jam */
    data[3] = Dec2BCD(t->hari);
    data[4] = Dec2BCD(t->tanggal);
    data[5] = Dec2BCD(t->bulan);
    data[6] = Dec2BCD(t->tahun);

    HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDR_WRITE, DS3231_REG_SECONDS,
                      I2C_MEMADD_SIZE_8BIT, data, 7, HAL_MAX_DELAY);
    printf("[OK] Waktu diset ke %02d:%02d:%02d %02d/%02d/20%02d\r\n",
           t->jam, t->menit, t->detik, t->tanggal, t->bulan, t->tahun);
}

/* Baca waktu dari DS3231 */
void DS3231_GetTime(DS3231_Time *t) {
    uint8_t data[7];
    HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDR_WRITE, DS3231_REG_SECONDS,
                     I2C_MEMADD_SIZE_8BIT, data, 7, HAL_MAX_DELAY);

    t->detik  = BCD2Dec(data[0] & 0x7F);
    t->menit  = BCD2Dec(data[1] & 0x7F);
    t->jam    = BCD2Dec(data[2] & 0x3F); /* Format 24 jam */
    t->hari   = BCD2Dec(data[3] & 0x07);
    t->tanggal = BCD2Dec(data[4] & 0x3F);
    t->bulan  = BCD2Dec(data[5] & 0x1F);
    t->tahun  = BCD2Dec(data[6]);
}

/* Baca suhu internal DS3231 (resolusi 0.25°C) */
float DS3231_GetTemperature(void) {
    uint8_t msb = DS3231_ReadReg(DS3231_REG_TEMP_MSB);
    uint8_t lsb = DS3231_ReadReg(DS3231_REG_TEMP_LSB);

    int16_t temp = ((int16_t)(int8_t)msb << 2) | (lsb >> 6);
    return temp * 0.25f;
}

/* Set Alarm 1 (detik, menit, jam, tanggal) */
void DS3231_SetAlarm1(uint8_t jam, uint8_t menit, uint8_t detik) {
    uint8_t data[4];
    data[0] = Dec2BCD(detik);         /* Alarm 1 detik */
    data[1] = Dec2BCD(menit);         /* Alarm 1 menit */
    data[2] = Dec2BCD(jam);           /* Alarm 1 jam */
    data[3] = 0x80;                    /* A1M4=1: alarm ketika jam/menit/detik cocok */

    HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDR_WRITE, DS3231_REG_ALARM1,
                      I2C_MEMADD_SIZE_8BIT, data, 4, HAL_MAX_DELAY);

    /* Enable Alarm 1 interrupt */
    uint8_t ctrl = DS3231_ReadReg(DS3231_REG_CONTROL);
    ctrl |= 0x05;  /* A1IE=1, INTCN=1 */
    DS3231_WriteReg(DS3231_REG_CONTROL, ctrl);

    printf("[OK] Alarm 1 diset ke %02d:%02d:%02d\r\n", jam, menit, detik);
}

/* Cek dan hapus flag alarm */
uint8_t DS3231_CheckAlarm(void) {
    uint8_t status = DS3231_ReadReg(DS3231_REG_STATUS);
    uint8_t alarm_flags = status & 0x03;

    if (alarm_flags) {
        /* Hapus flag alarm */
        DS3231_WriteReg(DS3231_REG_STATUS, status & ~0x03);
    }

    return alarm_flags;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 DS3231 RTC I2C\r\n");
    printf("  Alamat: 0x68\r\n");
    printf("===================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, DS3231_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] DS3231 tidak ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }
    printf("[OK] DS3231 terdeteksi\r\n");

    /* Set waktu awal (hanya perlu sekali, komentar setelah set) */
    DS3231_Time waktu_set = {
        .detik  = 0,
        .menit  = 30,
        .jam    = 14,
        .hari   = 6,      /* Sabtu */
        .tanggal = 7,
        .bulan  = 2,
        .tahun  = 26       /* 2026 */
    };
    DS3231_SetTime(&waktu_set);

    /* Set alarm 1: pukul 14:31:00 */
    DS3231_SetAlarm1(14, 31, 0);

    DS3231_Time waktu;
    uint32_t pembacaan = 0;

    while (1) {
        pembacaan++;

        /* Baca waktu */
        DS3231_GetTime(&waktu);

        /* Baca suhu */
        float suhu = DS3231_GetTemperature();

        /* Cek alarm */
        uint8_t alarm = DS3231_CheckAlarm();

        /* Cetak data */
        printf("DATA,%lu,%02d:%02d:%02d,%s,%02d/%02d/20%02d,%.2f,%d\r\n",
               pembacaan, waktu.jam, waktu.menit, waktu.detik,
               (waktu.hari >= 1 && waktu.hari <= 7) ? nama_hari[waktu.hari] : "?",
               waktu.tanggal, waktu.bulan, waktu.tahun, suhu, alarm);

        printf("  Waktu : %s, %02d/%02d/20%02d %02d:%02d:%02d\r\n",
               (waktu.hari >= 1 && waktu.hari <= 7) ? nama_hari[waktu.hari] : "?",
               waktu.tanggal, waktu.bulan, waktu.tahun,
               waktu.jam, waktu.menit, waktu.detik);
        printf("  Suhu  : %.2f C\r\n", suhu);

        if (alarm & 0x01) printf("  >>> ALARM 1 AKTIF! <<<\r\n");
        if (alarm & 0x02) printf("  >>> ALARM 2 AKTIF! <<<\r\n");

        HAL_Delay(1000);
    }
}
