/**
 * ============================================================
 *  STM32 SPI OLED SSD1306 DRIVER
 *  Modul 07 - SPI & Storage
 * ============================================================
 *  Deskripsi:
 *    Driver lengkap SSD1306 OLED 128x64 via SPI.
 *    Menampilkan teks, grafik, dan counter pada layar.
 *
 *  Wiring (SPI Mode):
 *    PA5  -> OLED SCK (CLK/D0)
 *    PA7  -> OLED MOSI (SDA/D1)
 *    PA4  -> OLED CS
 *    PB0  -> OLED DC (Data/Command)
 *    PB1  -> OLED RST (Reset)
 *    PA9  -> USB-TTL RX (debug)
 *    PA10 -> USB-TTL TX (debug)
 *
 *  Fitur:
 *    - Full init sequence SSD1306
 *    - 5x7 font table (space to Z)
 *    - Pixel-level drawing
 *    - String rendering
 *    - Incrementing counter demo
 * ============================================================
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "config.h"

/* ==================== Global Handles ==================== */
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

/* ==================== Display Buffer ==================== */
static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];

/* ==================== 5x7 Font Table ==================== */
/* Characters from Space (0x20) to 'Z' (0x5A) */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* Space */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* ) */
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, /* * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 9 */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* : */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* ; */
    {0x00, 0x08, 0x14, 0x22, 0x41}, /* < */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* = */
    {0x41, 0x22, 0x14, 0x08, 0x00}, /* > */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* ? */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* @ */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7F, 0x09, 0x09, 0x01, 0x01}, /* F */
    {0x3E, 0x41, 0x41, 0x51, 0x32}, /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x03, 0x04, 0x78, 0x04, 0x03}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* Z */
};

/* ==================== Function Prototypes ==================== */
void SystemClock_Config(void);
static void UART1_Init(void);
static void SPI1_Init(void);
static void GPIO_Init(void);
static void OLED_GPIO_Init(void);

static void ssd1306_reset(void);
static void ssd1306_send_cmd(uint8_t cmd);
static void ssd1306_send_data(const uint8_t *data, uint16_t len);
static void ssd1306_init(void);
static void ssd1306_update(void);
static void ssd1306_clear(void);
static void ssd1306_set_pixel(int16_t x, int16_t y, uint8_t color);
static void ssd1306_draw_char(int16_t x, int16_t y, char c);
static void ssd1306_draw_string(int16_t x, int16_t y, const char *str);
static void ssd1306_draw_hline(int16_t x, int16_t y, int16_t w);
static void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h);

/* ==================== Printf Retarget ==================== */
int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ============================================================
 *  SystemClock_Config
 *  HSE 8MHz -> PLL x9 -> SYSCLK 72MHz
 * ============================================================ */
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
        while (1);
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  UART1_Init - PA9(TX), PA10(RX), 115200 baud
 * ============================================================ */
static void UART1_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = UART1_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(UART1_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = UART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(UART1_RX_PORT, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = UART1_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  SPI1_Init - Master, Mode 0, 18MHz, 8-bit
 * ============================================================ */
static void SPI1_Init(void) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* SCK - PA5 */
    GPIO_InitStruct.Pin = SPI1_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_SCK_PORT, &GPIO_InitStruct);

    /* MOSI - PA7 */
    GPIO_InitStruct.Pin = SPI1_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_MOSI_PORT, &GPIO_InitStruct);

    /* MISO - PA6 (not used for OLED, but configure anyway) */
    GPIO_InitStruct.Pin = SPI1_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SPI1_MISO_PORT, &GPIO_InitStruct);

    /* CS - PA4 (manual control) */
    GPIO_InitStruct.Pin = SPI1_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_CS_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_SET);

    /* SPI Configuration */
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI1_DATA_SIZE;
    hspi1.Init.CLKPolarity = SPI1_CPOL;
    hspi1.Init.CLKPhase = SPI1_CPHA;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI1_PRESCALER;
    hspi1.Init.FirstBit = SPI1_FIRST_BIT;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        while (1);
    }
}

/* ============================================================
 *  GPIO_Init - LED PC13
 * ============================================================ */
static void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* ============================================================
 *  OLED_GPIO_Init - DC (PB0) and RST (PB1)
 * ============================================================ */
static void OLED_GPIO_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* DC Pin - PB0 */
    GPIO_InitStruct.Pin = OLED_DC_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OLED_DC_PORT, &GPIO_InitStruct);

    /* RST Pin - PB1 */
    GPIO_InitStruct.Pin = OLED_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OLED_RST_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
}

/* ============================================================
 *  SSD1306 Low-Level Functions
 * ============================================================ */

/* Hardware reset: pulse RST low */
static void ssd1306_reset(void) {
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
}

/* Send command: DC=LOW, CS=LOW, transmit, CS=HIGH */
static void ssd1306_send_cmd(uint8_t cmd) {
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET);   /* DC LOW = Command */
    HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);   /* CS LOW */
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_SET);     /* CS HIGH */
}

/* Send data: DC=HIGH, CS=LOW, transmit, CS=HIGH */
static void ssd1306_send_data(const uint8_t *data, uint16_t len) {
    HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET);     /* DC HIGH = Data */
    HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_RESET);   /* CS LOW */
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SPI1_CS_PORT, SPI1_CS_PIN, GPIO_PIN_SET);     /* CS HIGH */
}

/* ============================================================
 *  SSD1306 Initialization Sequence
 * ============================================================ */
static void ssd1306_init(void) {
    ssd1306_reset();

    HAL_Delay(100);

    ssd1306_send_cmd(SSD1306_CMD_DISPLAY_OFF);          /* 0xAE: Display OFF */

    ssd1306_send_cmd(SSD1306_CMD_SET_CLOCK_DIV);        /* 0xD5: Clock divide */
    ssd1306_send_cmd(0x80);                              /* Suggested ratio 0x80 */

    ssd1306_send_cmd(SSD1306_CMD_SET_MUX_RATIO);        /* 0xA8: MUX ratio */
    ssd1306_send_cmd(SSD1306_HEIGHT - 1);                /* 63 */

    ssd1306_send_cmd(SSD1306_CMD_SET_DISPLAY_OFFSET);   /* 0xD3: Display offset */
    ssd1306_send_cmd(0x00);                              /* No offset */

    ssd1306_send_cmd(SSD1306_CMD_SET_START_LINE | 0x0);  /* 0x40: Start line 0 */

    ssd1306_send_cmd(SSD1306_CMD_CHARGE_PUMP);           /* 0x8D: Charge pump */
    ssd1306_send_cmd(0x14);                              /* Enable charge pump */

    ssd1306_send_cmd(SSD1306_CMD_SET_MEMORY_MODE);       /* 0x20: Memory mode */
    ssd1306_send_cmd(0x00);                              /* Horizontal addressing */

    ssd1306_send_cmd(SSD1306_CMD_SEG_REMAP);             /* 0xA1: Segment remap */
    ssd1306_send_cmd(SSD1306_CMD_COM_SCAN_DEC);          /* 0xC8: COM scan decrement */

    ssd1306_send_cmd(SSD1306_CMD_SET_COM_PINS);          /* 0xDA: COM pins config */
    ssd1306_send_cmd(0x12);                              /* Alternative COM, disable remap */

    ssd1306_send_cmd(SSD1306_CMD_SET_CONTRAST);          /* 0x81: Contrast */
    ssd1306_send_cmd(0xCF);                              /* High contrast */

    ssd1306_send_cmd(SSD1306_CMD_SET_PRECHARGE);         /* 0xD9: Precharge period */
    ssd1306_send_cmd(0xF1);                              /* Phase 1=15, Phase 2=1 */

    ssd1306_send_cmd(SSD1306_CMD_SET_VCOMH);             /* 0xDB: VCOMH deselect */
    ssd1306_send_cmd(0x40);                              /* 0.77 x Vcc */

    ssd1306_send_cmd(SSD1306_CMD_ENTIRE_DISPLAY_ON);     /* 0xA4: Resume from RAM */
    ssd1306_send_cmd(SSD1306_CMD_NORMAL_DISPLAY);        /* 0xA6: Normal display */

    ssd1306_send_cmd(SSD1306_CMD_DISPLAY_ON);            /* 0xAF: Display ON */

    printf("[OLED] SSD1306 initialized\n");
    printf("[OLED] Init commands sent: AE,D5,80,A8,3F,D3,00,40,8D,14,20,00,A1,C8,DA,12,81,CF,D9,F1,DB,40,A4,A6,AF\n");
}

/* ============================================================
 *  SSD1306 Display Update - Write buffer to display
 * ============================================================ */
static void ssd1306_update(void) {
    /* Set column address: 0 to 127 */
    ssd1306_send_cmd(SSD1306_CMD_SET_COL_ADDR);
    ssd1306_send_cmd(0x00);
    ssd1306_send_cmd(SSD1306_WIDTH - 1);

    /* Set page address: 0 to 7 */
    ssd1306_send_cmd(SSD1306_CMD_SET_PAGE_ADDR);
    ssd1306_send_cmd(0x00);
    ssd1306_send_cmd(SSD1306_PAGES - 1);

    /* Send entire buffer */
    ssd1306_send_data(ssd1306_buffer, SSD1306_BUFFER_SIZE);
}

/* ============================================================
 *  SSD1306 Clear Buffer
 * ============================================================ */
static void ssd1306_clear(void) {
    memset(ssd1306_buffer, 0x00, SSD1306_BUFFER_SIZE);
}

/* ============================================================
 *  SSD1306 Set Pixel
 * ============================================================ */
static void ssd1306_set_pixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;

    if (color) {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
    } else {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
    }
}

/* ============================================================
 *  SSD1306 Draw Horizontal Line
 * ============================================================ */
static void ssd1306_draw_hline(int16_t x, int16_t y, int16_t w) {
    for (int16_t i = 0; i < w; i++) {
        ssd1306_set_pixel(x + i, y, 1);
    }
}

/* ============================================================
 *  SSD1306 Draw Rectangle (outline)
 * ============================================================ */
static void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
    /* Top and bottom */
    ssd1306_draw_hline(x, y, w);
    ssd1306_draw_hline(x, y + h - 1, w);
    /* Left and right */
    for (int16_t i = 0; i < h; i++) {
        ssd1306_set_pixel(x, y + i, 1);
        ssd1306_set_pixel(x + w - 1, y + i, 1);
    }
}

/* ============================================================
 *  SSD1306 Draw Character (5x7 font)
 * ============================================================ */
static void ssd1306_draw_char(int16_t x, int16_t y, char c) {
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) {
        c = ' ';
    }
    uint8_t idx = c - FONT_FIRST_CHAR;

    for (uint8_t col = 0; col < FONT_WIDTH; col++) {
        uint8_t line = font5x7[idx][col];
        for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
            if (line & (1 << row)) {
                ssd1306_set_pixel(x + col, y + row, 1);
            }
        }
    }
}

/* ============================================================
 *  SSD1306 Draw String
 * ============================================================ */
static void ssd1306_draw_string(int16_t x, int16_t y, const char *str) {
    int16_t cur_x = x;
    while (*str) {
        if (cur_x + FONT_WIDTH > SSD1306_WIDTH) break;
        ssd1306_draw_char(cur_x, y, *str);
        cur_x += FONT_WIDTH + 1;  /* 1 pixel spacing */
        str++;
    }
}

/* ============================================================
 *  Number to String (simple itoa for positive numbers)
 * ============================================================ */
static void uint_to_str(uint32_t val, char *buf, uint8_t buf_size) {
    char tmp[12];
    uint8_t i = 0;

    if (val == 0) {
        tmp[i++] = '0';
    } else {
        while (val > 0 && i < 10) {
            tmp[i++] = '0' + (val % 10);
            val /= 10;
        }
    }

    /* Reverse */
    uint8_t len = (i < buf_size - 1) ? i : buf_size - 1;
    for (uint8_t j = 0; j < len; j++) {
        buf[j] = tmp[len - 1 - j];
    }
    buf[len] = '\0';
}

/* ============================================================
 *  Main Entry Point
 * ============================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    UART1_Init();
    SPI1_Init();
    GPIO_Init();
    OLED_GPIO_Init();

    printf("\n\n");
    printf("============================================\n");
    printf("  STM32F103 SPI OLED SSD1306 Driver\n");
    printf("  Modul 07 - SPI & Storage\n");
    printf("============================================\n");
    printf("  SYSCLK : 72 MHz\n");
    printf("  SPI1   : 18 MHz (Prescaler /4)\n");
    printf("  Display: SSD1306 128x64\n");
    printf("  Pins   : SCK=PA5, MOSI=PA7, CS=PA4\n");
    printf("           DC=PB0, RST=PB1\n");
    printf("============================================\n");

    /* Initialize OLED */
    ssd1306_init();

    /* Initial display: title */
    ssd1306_clear();
    ssd1306_draw_rect(0, 0, 128, 64);
    ssd1306_draw_string(10, 4, "STM32 SPI");
    ssd1306_draw_string(10, 16, "MODUL 07");
    ssd1306_draw_hline(4, 28, 120);
    ssd1306_draw_string(10, 32, "SSD1306 OLED");
    ssd1306_draw_string(10, 44, "128X64 PIXEL");
    ssd1306_update();
    printf("[OLED] Title screen displayed\n");

    HAL_Delay(3000);

    /* Counter demo */
    uint32_t counter = 0;
    char count_str[12];
    char line_buf[24];

    printf("[OLED] Starting counter demo...\n");

    while (1) {
        ssd1306_clear();

        /* Header */
        ssd1306_draw_rect(0, 0, 128, 14);
        ssd1306_draw_string(4, 3, "STM32 SPI DEMO");

        /* Line 1: Module info */
        ssd1306_draw_string(4, 18, "MODUL 07 - SPI");

        /* Separator */
        ssd1306_draw_hline(4, 28, 120);

        /* Line 2: Counter */
        uint_to_str(counter, count_str, sizeof(count_str));
        /* Build "COUNT: xxxxx" */
        memset(line_buf, 0, sizeof(line_buf));
        strcpy(line_buf, "COUNT: ");
        strcat(line_buf, count_str);
        ssd1306_draw_string(4, 32, line_buf);

        /* Line 3: Uptime */
        uint32_t seconds = HAL_GetTick() / 1000;
        uint_to_str(seconds, count_str, sizeof(count_str));
        memset(line_buf, 0, sizeof(line_buf));
        strcpy(line_buf, "TIME: ");
        strcat(line_buf, count_str);
        strcat(line_buf, "S");
        ssd1306_draw_string(4, 44, line_buf);

        /* Progress bar */
        uint8_t bar_width = (uint8_t)((counter % 100) * 120 / 100);
        ssd1306_draw_rect(3, 55, 122, 8);
        for (int16_t i = 0; i < bar_width; i++) {
            for (int16_t j = 0; j < 6; j++) {
                ssd1306_set_pixel(4 + i, 56 + j, 1);
            }
        }

        ssd1306_update();

        /* Serial debug output every 10 counts */
        if (counter % 10 == 0) {
            printf("[OLED] Counter=%lu, Uptime=%lus\n", counter, seconds);
        }

        counter++;

        /* Toggle LED */
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        HAL_Delay(500);
    }
}
