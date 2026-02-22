/**
 * @file    main.c
 * @brief   P10 - Keypad Matrix 4x4 (8-Pin)
 *
 * ================================================================
 * KONSEP: KEYPAD MATRIX SCANNING
 * ================================================================
 *
 * Keypad 4x4 = 16 tombol dengan HANYA 8 kabel (4 baris + 4 kolom).
 * Ini mungkin karena teknik MATRIX SCANNING:
 *
 * Layout:
 *       Col1  Col2  Col3  Col4
 *        PB12  PB13  PB14  PB15
 *   Row1 PB8 [ 1 ] [ 2 ] [ 3 ] [ A ]
 *   Row2 PB9 [ 4 ] [ 5 ] [ 6 ] [ B ]
 *   Row3 PB10[ 7 ] [ 8 ] [ 9 ] [ C ]
 *   Row4 PB11[ * ] [ 0 ] [ # ] [ D ]
 *
 * ALGORITMA SCANNING:
 *   1. Set SEMUA baris ke HIGH (tidak aktif)
 *   2. Set SATU baris ke LOW (aktif)
 *   3. Baca semua kolom: jika kolom = LOW → tombol ditekan
 *   4. Ulangi untuk semua baris
 *
 * Mengapa baris OUTPUT dan kolom INPUT?
 *   - Baris (OUTPUT): kita drive LOW satu per satu
 *   - Kolom (INPUT PULLUP): kita baca, jika LOW = tombol pada
 *     baris aktif di kolom itu sedang ditekan
 *   - Ketika tombol ditekan: arus dari pull-up kolom
 *     → masuk ke kolom → melalui tombol → keluar ke baris yang LOW
 *     → kolom terbaca LOW ✓
 *
 * OUTPUT:
 *   8 LED PA0-PA7 menampilkan nomor tombol (0-15) dalam biner
 *
 * ================================================================
 * RANGKAIAN:
 *   Keypad Row1─PB8, Row2─PB9, Row3─PB10, Row4─PB11  (OUTPUT_PP)
 *   Keypad Col1─PB12, Col2─PB13, Col3─PB14, Col4─PB15 (INPUT PULLUP)
 *   PA0─[220Ω]─LED1─GND  ...  PA7─[220Ω]─LED8─GND
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define KP_PORT    GPIOB

/* Baris: OUTPUT — di-drive LOW satu per satu */
static const uint16_t ROW[4] = {
    GPIO_PIN_8,    /* Row 1 */
    GPIO_PIN_9,    /* Row 2 */
    GPIO_PIN_10,   /* Row 3 */
    GPIO_PIN_11    /* Row 4 */
};
#define ROW_ALL  (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11)

/* Kolom: INPUT PULLUP — baca LOW saat tombol ditekan */
static const uint16_t COL[4] = {
    GPIO_PIN_12,   /* Col 1 */
    GPIO_PIN_13,   /* Col 2 */
    GPIO_PIN_14,   /* Col 3 */
    GPIO_PIN_15    /* Col 4 */
};

/* Keymap: baris × kolom → nomor tombol (0-15) */
static const uint8_t KEYMAP[4][4] = {
    { 1,  2,  3, 10},   /* Row1: 1, 2, 3, A (10) */
    { 4,  5,  6, 11},   /* Row2: 4, 5, 6, B (11) */
    { 7,  8,  9, 12},   /* Row3: 7, 8, 9, C (12) */
    {15,  0, 14, 13}    /* Row4: *(15), 0, #(14), D(13) */
};

#define LED_PORT   GPIOA
#define LED_ALL    (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|\
                    GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static int8_t  last_key      = -1;   /* Tombol terakhir dibaca */
static uint8_t led_value     = 0;    /* Nilai yang ditampilkan di LED */

/* ================================================================
 *  FUNCTION PROTOTYPES
 * ================================================================ */
void SystemClock_Config(void);
static void GPIO_Init(void);
static int8_t Keypad_Scan(void);
static void LED_Show(uint8_t val);

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    LED_Show(0);  /* Tampilkan 0 di awal */

    while (1)
    {
        /* Scan keypad setiap 5ms */
        int8_t key = Keypad_Scan();

        if (key >= 0 && key != last_key)
        {
            /* Tombol baru ditekan → tampilkan di LED */
            led_value = (uint8_t)key;
            LED_Show(led_value);
            last_key = key;
        }
        else if (key < 0)
        {
            /* Tidak ada tombol → reset last_key */
            last_key = -1;
        }

        HAL_Delay(5);
    }
}

/* ================================================================
 *  KEYPAD SCANNING ALGORITHM
 *  Return: 0-15 (nomor tombol), -1 (tidak ada tombol)
 * ================================================================ */
static int8_t Keypad_Scan(void)
{
    /* Set semua baris ke HIGH (inactive) dahulu */
    HAL_GPIO_WritePin(KP_PORT, ROW_ALL, GPIO_PIN_SET);

    for (int row = 0; row < 4; row++)
    {
        /* ================================================
         *  LANGKAH 1: Drive ROW[row] ke LOW (aktifkan baris)
         *  Baris lain tetap HIGH (tidak aktif)
         * ================================================ */
        HAL_GPIO_WritePin(KP_PORT, ROW[row], GPIO_PIN_RESET);

        /* Beri sedikit waktu agar sinyal stabil */
        HAL_Delay(1);

        /* ================================================
         *  LANGKAH 2: Baca semua kolom
         *  Jika kolom LOW → tombol pada (row, col) ditekan
         *
         *  Mengapa kolom bisa LOW?
         *  Karena COL dihubungkan ke internal pull-up (HIGH default).
         *  Saat tombol ditekan → COL terhubung ke ROW yang LOW
         *  → arus dari pull-up → masuk COL → keluar ROW (LOW)
         *  → COL terbaca LOW
         * ================================================ */
        for (int col = 0; col < 4; col++)
        {
            if (HAL_GPIO_ReadPin(KP_PORT, COL[col]) == GPIO_PIN_RESET)
            {
                /* Tombol ditemukan di (row, col) */

                /* Set baris kembali ke HIGH sebelum return */
                HAL_GPIO_WritePin(KP_PORT, ROW_ALL, GPIO_PIN_SET);

                return (int8_t)KEYMAP[row][col];
            }
        }

        /* Set baris kembali ke HIGH sebelum ke baris berikut */
        HAL_GPIO_WritePin(KP_PORT, ROW[row], GPIO_PIN_SET);
    }

    return -1;  /* Tidak ada tombol yang ditekan */
}

/* ================================================================
 *  TAMPILKAN NILAI DI 8 LED (BINER)
 *  PA0 = bit0 (LSB), PA7 = bit7 (MSB)
 *  Contoh: val=5 = 0b00000101 → PA0=ON, PA1=OFF, PA2=ON
 * ================================================================ */
static void LED_Show(uint8_t val)
{
    /* BSRR (Bit Set/Reset Register) untuk atomic update semua pin
     *   bit[15:0]  = set mask  (pin yang harus HIGH)
     *   bit[31:16] = reset mask (pin yang harus LOW)
     */
    uint16_t set_m   = (uint16_t)val;
    uint16_t reset_m = (uint16_t)(~val & 0xFF);
    GPIOA->BSRR = ((uint32_t)reset_m << 16U) | set_m;
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA0-PA7: OUTPUT PUSH-PULL (8 LED) */
    HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_ALL;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* PB8-PB11: OUTPUT PUSH-PULL (Keypad Rows)
     *   Drive LOW satu per satu saat scanning
     *   Default: HIGH (tidak aktif) */
    HAL_GPIO_WritePin(KP_PORT, ROW_ALL, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = ROW_ALL;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KP_PORT, &GPIO_InitStruct);

    /* PB12-PB15: INPUT PULLUP (Keypad Columns)
     *   Default HIGH via internal ~40kΩ.
     *   LOW ketika tombol di baris aktif ditekan. */
    uint16_t col_all = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Pin   = col_all;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KP_PORT, &GPIO_InitStruct);
}

/* ================================================================
 *  SYSTEM CLOCK: HSI → PLL × 16 = 64 MHz
 * ================================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { while (1); }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { while (1); }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
