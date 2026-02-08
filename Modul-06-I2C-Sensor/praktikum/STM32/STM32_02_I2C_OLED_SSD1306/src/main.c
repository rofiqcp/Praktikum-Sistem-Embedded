/**
 * ==========================================================
 *  Modul 06 - STM32_02_I2C_OLED_SSD1306
 *  Mengendalikan OLED SSD1306 128x64 via I2C
 * ==========================================================
 *  Deskripsi:
 *    Menampilkan teks dan counter pada OLED SSD1306.
 *    Menggunakan HAL_I2C_Mem_Write() untuk perintah (0x00) dan data (0x40).
 *    Menyertakan font 5x7 untuk karakter ASCII.
 *  Koneksi:
 *    PB6 = SCL, PB7 = SDA (I2C1)
 *    SSD1306 address: 0x3C
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

#define SSD1306_ADDR       0x3C
#define SSD1306_ADDR_WRITE (SSD1306_ADDR << 1)
#define SSD1306_WIDTH      128
#define SSD1306_HEIGHT     64
#define SSD1306_PAGES      (SSD1306_HEIGHT / 8)

/* Buffer layar OLED */
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_PAGES];

/* Font 5x7 untuk karakter ASCII 32-127 */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32 (spasi)
    {0x00,0x00,0x5F,0x00,0x00}, // 33 !
    {0x00,0x07,0x00,0x07,0x00}, // 34 "
    {0x14,0x7F,0x14,0x7F,0x14}, // 35 #
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36 $
    {0x23,0x13,0x08,0x64,0x62}, // 37 %
    {0x36,0x49,0x55,0x22,0x50}, // 38 &
    {0x00,0x05,0x03,0x00,0x00}, // 39 '
    {0x00,0x1C,0x22,0x41,0x00}, // 40 (
    {0x00,0x41,0x22,0x1C,0x00}, // 41 )
    {0x08,0x2A,0x1C,0x2A,0x08}, // 42 *
    {0x08,0x08,0x3E,0x08,0x08}, // 43 +
    {0x00,0x50,0x30,0x00,0x00}, // 44 ,
    {0x08,0x08,0x08,0x08,0x08}, // 45 -
    {0x00,0x60,0x60,0x00,0x00}, // 46 .
    {0x20,0x10,0x08,0x04,0x02}, // 47 /
    {0x3E,0x51,0x49,0x45,0x3E}, // 48 0
    {0x00,0x42,0x7F,0x40,0x00}, // 49 1
    {0x42,0x61,0x51,0x49,0x46}, // 50 2
    {0x21,0x41,0x45,0x4B,0x31}, // 51 3
    {0x18,0x14,0x12,0x7F,0x10}, // 52 4
    {0x27,0x45,0x45,0x45,0x39}, // 53 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 54 6
    {0x01,0x71,0x09,0x05,0x03}, // 55 7
    {0x36,0x49,0x49,0x49,0x36}, // 56 8
    {0x06,0x49,0x49,0x29,0x1E}, // 57 9
    {0x00,0x36,0x36,0x00,0x00}, // 58 :
    {0x00,0x56,0x36,0x00,0x00}, // 59 ;
    {0x00,0x08,0x14,0x22,0x41}, // 60 <
    {0x14,0x14,0x14,0x14,0x14}, // 61 =
    {0x41,0x22,0x14,0x08,0x00}, // 62 >
    {0x02,0x01,0x51,0x09,0x06}, // 63 ?
    {0x32,0x49,0x79,0x41,0x3E}, // 64 @
    {0x7E,0x11,0x11,0x11,0x7E}, // 65 A
    {0x7F,0x49,0x49,0x49,0x36}, // 66 B
    {0x3E,0x41,0x41,0x41,0x22}, // 67 C
    {0x7F,0x41,0x41,0x22,0x1C}, // 68 D
    {0x7F,0x49,0x49,0x49,0x41}, // 69 E
    {0x7F,0x09,0x09,0x01,0x01}, // 70 F
    {0x3E,0x41,0x41,0x51,0x32}, // 71 G
    {0x7F,0x08,0x08,0x08,0x7F}, // 72 H
    {0x00,0x41,0x7F,0x41,0x00}, // 73 I
    {0x20,0x40,0x41,0x3F,0x01}, // 74 J
    {0x7F,0x08,0x14,0x22,0x41}, // 75 K
    {0x7F,0x40,0x40,0x40,0x40}, // 76 L
    {0x7F,0x02,0x04,0x02,0x7F}, // 77 M
    {0x7F,0x04,0x08,0x10,0x7F}, // 78 N
    {0x3E,0x41,0x41,0x41,0x3E}, // 79 O
    {0x7F,0x09,0x09,0x09,0x06}, // 80 P
    {0x3E,0x41,0x51,0x21,0x5E}, // 81 Q
    {0x7F,0x09,0x19,0x29,0x46}, // 82 R
    {0x46,0x49,0x49,0x49,0x31}, // 83 S
    {0x01,0x01,0x7F,0x01,0x01}, // 84 T
    {0x3F,0x40,0x40,0x40,0x3F}, // 85 U
    {0x1F,0x20,0x40,0x20,0x1F}, // 86 V
    {0x3F,0x40,0x38,0x40,0x3F}, // 87 W
    {0x63,0x14,0x08,0x14,0x63}, // 88 X
    {0x07,0x08,0x70,0x08,0x07}, // 89 Y
    {0x61,0x51,0x49,0x45,0x43}, // 90 Z
    {0x00,0x00,0x7F,0x41,0x41}, // 91 [
    {0x02,0x04,0x08,0x10,0x20}, // 92 backslash
    {0x41,0x41,0x7F,0x00,0x00}, // 93 ]
    {0x04,0x02,0x01,0x02,0x04}, // 94 ^
    {0x40,0x40,0x40,0x40,0x40}, // 95 _
    {0x00,0x01,0x02,0x04,0x00}, // 96 `
    {0x20,0x54,0x54,0x54,0x78}, // 97 a
    {0x7F,0x48,0x44,0x44,0x38}, // 98 b
    {0x38,0x44,0x44,0x44,0x20}, // 99 c
    {0x38,0x44,0x44,0x48,0x7F}, // 100 d
    {0x38,0x54,0x54,0x54,0x18}, // 101 e
    {0x08,0x7E,0x09,0x01,0x02}, // 102 f
    {0x08,0x14,0x54,0x54,0x3C}, // 103 g
    {0x7F,0x08,0x04,0x04,0x78}, // 104 h
    {0x00,0x44,0x7D,0x40,0x00}, // 105 i
    {0x20,0x40,0x44,0x3D,0x00}, // 106 j
    {0x00,0x7F,0x10,0x28,0x44}, // 107 k
    {0x00,0x41,0x7F,0x40,0x00}, // 108 l
    {0x7C,0x04,0x18,0x04,0x78}, // 109 m
    {0x7C,0x08,0x04,0x04,0x78}, // 110 n
    {0x38,0x44,0x44,0x44,0x38}, // 111 o
    {0x7C,0x14,0x14,0x14,0x08}, // 112 p
    {0x08,0x14,0x14,0x18,0x7C}, // 113 q
    {0x7C,0x08,0x04,0x04,0x08}, // 114 r
    {0x48,0x54,0x54,0x54,0x20}, // 115 s
    {0x04,0x3F,0x44,0x40,0x20}, // 116 t
    {0x3C,0x40,0x40,0x20,0x7C}, // 117 u
    {0x1C,0x20,0x40,0x20,0x1C}, // 118 v
    {0x3C,0x40,0x30,0x40,0x3C}, // 119 w
    {0x44,0x28,0x10,0x28,0x44}, // 120 x
    {0x0C,0x50,0x50,0x50,0x3C}, // 121 y
    {0x44,0x64,0x54,0x4C,0x44}, // 122 z
    {0x00,0x08,0x36,0x41,0x00}, // 123 {
    {0x00,0x00,0x7F,0x00,0x00}, // 124 |
    {0x00,0x41,0x36,0x08,0x00}, // 125 }
    {0x08,0x08,0x2A,0x1C,0x08}, // 126 ~
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

/* =================== SSD1306 Driver =================== */

/* Kirim perintah ke SSD1306 */
void SSD1306_WriteCmd(uint8_t cmd) {
    HAL_I2C_Mem_Write(&hi2c1, SSD1306_ADDR_WRITE, 0x00, I2C_MEMADD_SIZE_8BIT,
                      &cmd, 1, HAL_MAX_DELAY);
}

/* Inisialisasi OLED SSD1306 */
void SSD1306_Init(void) {
    HAL_Delay(100); /* Tunggu power stabil */

    SSD1306_WriteCmd(0xAE); /* Display OFF */
    SSD1306_WriteCmd(0xD5); /* Set clock divider */
    SSD1306_WriteCmd(0x80);
    SSD1306_WriteCmd(0xA8); /* Set multiplex ratio */
    SSD1306_WriteCmd(0x3F); /* 64 baris */
    SSD1306_WriteCmd(0xD3); /* Set display offset */
    SSD1306_WriteCmd(0x00);
    SSD1306_WriteCmd(0x40); /* Set start line */
    SSD1306_WriteCmd(0x8D); /* Charge pump */
    SSD1306_WriteCmd(0x14); /* Enable charge pump */
    SSD1306_WriteCmd(0x20); /* Memory addressing mode */
    SSD1306_WriteCmd(0x00); /* Horizontal addressing */
    SSD1306_WriteCmd(0xA1); /* Segment remap */
    SSD1306_WriteCmd(0xC8); /* COM scan direction */
    SSD1306_WriteCmd(0xDA); /* COM pins config */
    SSD1306_WriteCmd(0x12);
    SSD1306_WriteCmd(0x81); /* Contrast */
    SSD1306_WriteCmd(0xCF);
    SSD1306_WriteCmd(0xD9); /* Pre-charge period */
    SSD1306_WriteCmd(0xF1);
    SSD1306_WriteCmd(0xDB); /* VCOMH deselect */
    SSD1306_WriteCmd(0x40);
    SSD1306_WriteCmd(0xA4); /* Entire display ON (GDDRAM) */
    SSD1306_WriteCmd(0xA6); /* Normal display (bukan invert) */
    SSD1306_WriteCmd(0xAF); /* Display ON */
}

/* Bersihkan buffer */
void SSD1306_Clear(void) {
    memset(ssd1306_buffer, 0, sizeof(ssd1306_buffer));
}

/* Kirim buffer ke display */
void SSD1306_Update(void) {
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        SSD1306_WriteCmd(0xB0 + page);  /* Set page */
        SSD1306_WriteCmd(0x00);          /* Set kolom rendah */
        SSD1306_WriteCmd(0x10);          /* Set kolom tinggi */

        HAL_I2C_Mem_Write(&hi2c1, SSD1306_ADDR_WRITE, 0x40, I2C_MEMADD_SIZE_8BIT,
                          &ssd1306_buffer[SSD1306_WIDTH * page],
                          SSD1306_WIDTH, HAL_MAX_DELAY);
    }
}

/* Set pixel pada koordinat (x, y) */
void SSD1306_SetPixel(int16_t x, int16_t y, uint8_t warna) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;

    if (warna) {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
    } else {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
    }
}

/* Tulis satu karakter pada posisi (x, y) */
void SSD1306_WriteChar(int16_t x, int16_t y, char ch) {
    if (ch < 32 || ch > 126) ch = '?';
    uint8_t idx = ch - 32;

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t data = font5x7[idx][col];
        for (uint8_t bit = 0; bit < 7; bit++) {
            SSD1306_SetPixel(x + col, y + bit, (data >> bit) & 1);
        }
    }
}

/* Tulis string pada posisi (x, y) */
void SSD1306_WriteString(int16_t x, int16_t y, const char *str) {
    while (*str) {
        SSD1306_WriteChar(x, y, *str);
        x += 6; /* 5 piksel font + 1 spasi */
        str++;
    }
}

/* Gambar garis horizontal */
void SSD1306_DrawHLine(int16_t x, int16_t y, int16_t w) {
    for (int16_t i = 0; i < w; i++) {
        SSD1306_SetPixel(x + i, y, 1);
    }
}

/* Gambar kotak (outline) */
void SSD1306_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    for (int16_t i = 0; i < w; i++) {
        SSD1306_SetPixel(x + i, y, 1);
        SSD1306_SetPixel(x + i, y + h - 1, 1);
    }
    for (int16_t i = 0; i < h; i++) {
        SSD1306_SetPixel(x, y + i, 1);
        SSD1306_SetPixel(x + w - 1, y + i, 1);
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_Init();
    MX_I2C1_Init();

    printf("\r\n===================================\r\n");
    printf("  STM32 OLED SSD1306 I2C\r\n");
    printf("  Alamat: 0x3C\r\n");
    printf("===================================\r\n");

    /* Cek apakah SSD1306 terhubung */
    if (HAL_I2C_IsDeviceReady(&hi2c1, SSD1306_ADDR_WRITE, 3, 100) != HAL_OK) {
        printf("[ERROR] SSD1306 tidak ditemukan di alamat 0x3C!\r\n");
        while (1) { HAL_Delay(1000); }
    }
    printf("[OK] SSD1306 terdeteksi\r\n");

    /* Inisialisasi OLED */
    SSD1306_Init();
    printf("[OK] OLED diinisialisasi\r\n");

    uint32_t counter = 0;
    char buf[32];

    while (1) {
        SSD1306_Clear();

        /* Judul */
        SSD1306_DrawRect(0, 0, 128, 64);
        SSD1306_WriteString(10, 4, "STM32 OLED");
        SSD1306_DrawHLine(4, 14, 120);

        /* Info */
        SSD1306_WriteString(4, 18, "Modul 06 I2C");
        SSD1306_WriteString(4, 28, "SSD1306 128x64");

        /* Counter */
        snprintf(buf, sizeof(buf), "Count: %lu", counter);
        SSD1306_WriteString(4, 40, buf);

        /* Tick waktu */
        uint32_t detik = HAL_GetTick() / 1000;
        snprintf(buf, sizeof(buf), "Uptime: %lus", detik);
        SSD1306_WriteString(4, 52, buf);

        /* Kirim ke display */
        SSD1306_Update();

        printf("[INFO] Counter: %lu, Uptime: %lu s\r\n", counter, detik);

        counter++;
        HAL_Delay(1000);
    }
}
