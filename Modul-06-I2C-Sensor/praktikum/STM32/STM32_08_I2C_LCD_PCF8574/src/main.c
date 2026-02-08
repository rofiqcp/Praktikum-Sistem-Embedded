/**
 * ==========================================================
 *  Modul 06 - STM32_08_I2C_LCD_PCF8574
 *  LCD 16x2 dengan I2C backpack PCF8574
 * ==========================================================
 *  Deskripsi:
 *    Mengendalikan LCD 16x2 karakter melalui PCF8574 I2C expander.
 *    Mode 4-bit. Enable pulse via I2C. Menampilkan teks dan counter.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    PCF8574 address: 0x27
 *  PCF8574 bit mapping:
 *    P0 = RS, P1 = RW, P2 = EN, P3 = Backlight
 *    P4-P7 = D4-D7 (data nibble tinggi)
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

#define PCF8574_ADDR        0x27
#define PCF8574_ADDR_WRITE  (PCF8574_ADDR << 1)

/* Bit mapping PCF8574 ke LCD */
#define LCD_RS  (1 << 0)   /* Register Select */
#define LCD_RW  (1 << 1)   /* Read/Write */
#define LCD_EN  (1 << 2)   /* Enable */
#define LCD_BL  (1 << 3)   /* Backlight */

/* LCD Commands */
#define LCD_CMD_CLEAR        0x01
#define LCD_CMD_HOME         0x02
#define LCD_CMD_ENTRY_MODE   0x06
#define LCD_CMD_DISPLAY_ON   0x0C
#define LCD_CMD_DISPLAY_OFF  0x08
#define LCD_CMD_FUNCTION_SET 0x28  /* 4-bit, 2 baris, 5x8 font */
#define LCD_CMD_SET_DDRAM    0x80
#define LCD_CMD_LINE2        0xC0

static uint8_t lcd_backlight = LCD_BL; /* Backlight default ON */

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

/* =================== LCD I2C Driver =================== */

/* Kirim satu byte ke PCF8574 */
void LCD_I2C_Write(uint8_t data) {
    HAL_I2C_Master_Transmit(&hi2c1, PCF8574_ADDR_WRITE, &data, 1, HAL_MAX_DELAY);
}

/* Generate pulse Enable (rising-falling edge) */
void LCD_PulseEnable(uint8_t data) {
    LCD_I2C_Write(data | LCD_EN | lcd_backlight);  /* EN=1 */
    HAL_Delay(1);
    LCD_I2C_Write((data & ~LCD_EN) | lcd_backlight); /* EN=0 */
    HAL_Delay(1);
}

/* Kirim 4-bit nibble ke LCD */
void LCD_SendNibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = (nibble & 0xF0) | rs | lcd_backlight;
    LCD_PulseEnable(data);
}

/* Kirim byte (perintah atau data) ke LCD dalam mode 4-bit */
void LCD_SendByte(uint8_t byte, uint8_t rs) {
    /* Kirim nibble tinggi dulu */
    LCD_SendNibble(byte & 0xF0, rs);
    /* Lalu nibble rendah */
    LCD_SendNibble((byte << 4) & 0xF0, rs);
}

/* Kirim perintah ke LCD (RS=0) */
void LCD_Command(uint8_t cmd) {
    LCD_SendByte(cmd, 0);
    if (cmd == LCD_CMD_CLEAR || cmd == LCD_CMD_HOME) {
        HAL_Delay(2); /* Perintah clear/home butuh waktu lebih lama */
    }
}

/* Kirim data karakter ke LCD (RS=1) */
void LCD_Data(uint8_t data) {
    LCD_SendByte(data, LCD_RS);
}

/* Inisialisasi LCD 16x2 mode 4-bit */
void LCD_Init(void) {
    HAL_Delay(50); /* Tunggu power up */

    /* Sekuens inisialisasi 4-bit mode (sesuai datasheet HD44780) */
    LCD_SendNibble(0x30, 0);  /* Function set: 8-bit */
    HAL_Delay(5);
    LCD_SendNibble(0x30, 0);  /* Function set: 8-bit */
    HAL_Delay(1);
    LCD_SendNibble(0x30, 0);  /* Function set: 8-bit */
    HAL_Delay(1);
    LCD_SendNibble(0x20, 0);  /* Function set: 4-bit */
    HAL_Delay(1);

    /* Konfigurasi LCD */
    LCD_Command(LCD_CMD_FUNCTION_SET);  /* 4-bit, 2 baris, 5x8 */
    LCD_Command(LCD_CMD_DISPLAY_OFF);   /* Display OFF */
    LCD_Command(LCD_CMD_CLEAR);         /* Clear display */
    LCD_Command(LCD_CMD_ENTRY_MODE);    /* Entry mode: increment, no shift */
    LCD_Command(LCD_CMD_DISPLAY_ON);    /* Display ON, cursor OFF */

    printf("[OK] LCD 16x2 I2C diinisialisasi\r\n");
}

/* Bersihkan layar LCD */
void LCD_Clear(void) {
    LCD_Command(LCD_CMD_CLEAR);
}

/* Set posisi kursor (baris 0-1, kolom 0-15) */
void LCD_SetCursor(uint8_t baris, uint8_t kolom) {
    uint8_t addr = (baris == 0) ? 0x00 : 0x40;
    LCD_Command(LCD_CMD_SET_DDRAM | (addr + kolom));
}

/* Tulis string ke LCD */
void LCD_Print(const char *str) {
    while (*str) {
        LCD_Data(*str);
        str++;
    }
}

/* Tulis string pada posisi tertentu */
void LCD_PrintAt(uint8_t baris, uint8_t kolom, const char *str) {
    LCD_SetCursor(baris, kolom);
    LCD_Print(str);
}

/* Kontrol backlight */
void LCD_Backlight(uint8_t on) {
    lcd_backlight = on ? LCD_BL : 0;
    LCD_I2C_Write(lcd_backlight);
}

/* Buat karakter kustom (0-7) */
void LCD_CreateChar(uint8_t lokasi, uint8_t *pola) {
    LCD_Command(0x40 | ((lokasi & 0x07) << 3));
    for (int i = 0; i < 8; i++) {
        LCD_Data(pola[i]);
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 LCD 16x2 I2C (PCF8574)\r\n");
    printf("  Alamat: 0x27\r\n");
    printf("===================================\r\n");

    /* Cek koneksi */
    if (HAL_I2C_IsDeviceReady(&hi2c1, PCF8574_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] PCF8574 tidak ditemukan!\r\n");
        while (1) { HAL_Delay(1000); }
    }
    printf("[OK] PCF8574 terdeteksi\r\n");

    /* Inisialisasi LCD */
    LCD_Init();

    /* Karakter kustom: simbol derajat */
    uint8_t char_derajat[8] = {0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00};
    /* Karakter kustom: simbol hati */
    uint8_t char_hati[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};
    LCD_CreateChar(0, char_derajat);
    LCD_CreateChar(1, char_hati);

    /* Tampilan awal */
    LCD_Clear();
    LCD_PrintAt(0, 0, "STM32 I2C LCD");
    LCD_PrintAt(1, 0, "Modul 06 ");
    LCD_Data(1); /* Tampilkan karakter hati */

    HAL_Delay(3000);

    uint32_t counter = 0;
    char buf[17];

    while (1) {
        counter++;
        uint32_t detik = HAL_GetTick() / 1000;

        LCD_Clear();

        /* Baris 1: Judul */
        LCD_PrintAt(0, 0, "I2C LCD STM32");

        /* Baris 2: Counter dan uptime */
        snprintf(buf, sizeof(buf), "C:%lu T:%lus", counter, detik);
        LCD_PrintAt(1, 0, buf);

        printf("DATA,counter=%lu,uptime=%lu\r\n", counter, detik);

        HAL_Delay(1000);
    }
}
