/**
 * @file    main.c
 * @brief   P10 — Matrix Commander: Pemindaian Keypad 4×4 & Tampilan LED
 *
 * ================================================================
 * KONSEP: KEYPAD MATRIX 4×4 SCANNING
 * ================================================================
 *
 * 16 tombol dengan hanya 8 kabel (4 ROW + 4 COL).
 * Teknik MATRIX SCANNING:
 *
 * Layout keypad:
 *         COL1  COL2  COL3  COL4
 *          G19   G18   G17   G16
 *   ROW1 G25 [1] [2] [3] [A]
 *   ROW2 G26 [4] [5] [6] [B]
 *   ROW3 G27 [7] [8] [9] [C]
 *   ROW4 G32 [*] [0] [#] [D]
 *
 * ALGORITMA:
 *   1. Set semua ROW = HIGH (inactive)
 *   2. Pull satu ROW ke LOW (active)
 *   3. Baca semua COL: COL=LOW → tombol ditekan di (ROW, COL)
 *   4. Ulangi untuk semua ROW
 *
 * ROW: OUTPUT (drive LOW satu per satu)
 * COL: INPUT PULLUP (baca LOW saat tombol ditekan)
 *
 * OUTPUT: 4 LED menampilkan nilai tombol (0-15) dalam biner
 *   GPIO4=[bit0], GPIO5=[bit1], GPIO13=[bit2], GPIO14=[bit3]
 *
 * ================================================================
 * RANGKAIAN:
 *   Keypad Row1─GPIO25, Row2─GPIO26, Row3─GPIO27, Row4─GPIO32  (OUT)
 *   Keypad Col1─GPIO19, Col2─GPIO18, Col3─GPIO17, Col4─GPIO16  (IN PU)
 *   GPIO4─[220Ω]─LED1─GND ... GPIO14─[220Ω]─LED4─GND
 *
 * PLATFORM : ESP32 DevKit V1
 * FRAMEWORK: ESP-IDF (FreeRTOS)
 * ================================================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
static const gpio_num_t ROW[4] = {
    GPIO_NUM_25,  /* Row 1 */
    GPIO_NUM_26,  /* Row 2 */
    GPIO_NUM_27,  /* Row 3 */
    GPIO_NUM_32   /* Row 4 */
};
#define ROW_MASK ((1ULL<<GPIO_NUM_25)|(1ULL<<GPIO_NUM_26)|\
                  (1ULL<<GPIO_NUM_27)|(1ULL<<GPIO_NUM_32))

static const gpio_num_t COL[4] = {
    GPIO_NUM_19,  /* Col 1 */
    GPIO_NUM_18,  /* Col 2 */
    GPIO_NUM_17,  /* Col 3 */
    GPIO_NUM_16   /* Col 4 */
};

/* Keymap: row × col → tombol value 0-15 */
static const uint8_t KEYMAP[4][4] = {
    { 1,  2,  3, 10},   /* Row 1: 1, 2, 3, A */
    { 4,  5,  6, 11},   /* Row 2: 4, 5, 6, B */
    { 7,  8,  9, 12},   /* Row 3: 7, 8, 9, C */
    {15,  0, 14, 13}    /* Row 4: *, 0, #, D  */
};

/* 4 LED pins untuk display 4-bit (nilai 0-15) */
#define NUM_LEDS 4
static const gpio_num_t LED[NUM_LEDS] = {
    GPIO_NUM_4,   /* bit0 */
    GPIO_NUM_5,   /* bit1 */
    GPIO_NUM_13,  /* bit2 */
    GPIO_NUM_14   /* bit3 */
};

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* 4 ROW: OUTPUT (drive LOW untuk scanning) */
    gpio_config_t row_conf = {
        .pin_bit_mask = ROW_MASK,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&row_conf);

    /* Set semua ROW ke HIGH (inactive) */
    for (int i = 0; i < 4; i++)
        gpio_set_level(ROW[i], 1);

    /* 4 COL: INPUT + PULLUP */
    uint64_t col_mask = (1ULL<<GPIO_NUM_19)|(1ULL<<GPIO_NUM_18)|
                        (1ULL<<GPIO_NUM_17)|(1ULL<<GPIO_NUM_16);
    gpio_config_t col_conf = {
        .pin_bit_mask = col_mask,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&col_conf);

    /* 4 LEDs: OUTPUT */
    uint64_t led_mask = 0;
    for (int i = 0; i < NUM_LEDS; i++)
        led_mask |= (1ULL << LED[i]);

    gpio_config_t led_conf = {
        .pin_bit_mask = led_mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], 0);
}

/* ================================================================
 *  LED SHOW: tampilkan nilai 4-bit pada 4 LED
 * ================================================================ */
static void led_show(uint8_t val)
{
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], (val >> i) & 1);
}

/* ================================================================
 *  KEYPAD SCANNING
 *  Return: 0-15 = tombol ditekan, -1 = tidak ada tombol
 * ================================================================ */
static int keypad_scan(void)
{
    /* Set semua ROW ke HIGH (idle) */
    for (int r = 0; r < 4; r++)
        gpio_set_level(ROW[r], 1);

    for (int r = 0; r < 4; r++)
    {
        /* Drive ROW[r] ke LOW (aktifkan baris ini) */
        gpio_set_level(ROW[r], 0);
        vTaskDelay(pdMS_TO_TICKS(1));   /* Settling time */

        /* Baca semua kolom */
        for (int c = 0; c < 4; c++)
        {
            if (gpio_get_level(COL[c]) == 0)
            {
                /* Tombol ditekan! Set kembali ROW ke HIGH dan return */
                gpio_set_level(ROW[r], 1);
                return (int)KEYMAP[r][c];
            }
        }

        /* Set ROW[r] kembali ke HIGH sebelum ke baris berikutnya */
        gpio_set_level(ROW[r], 1);
    }

    return -1;   /* Tidak ada tombol */
}

/* ================================================================
 *  MAIN APPLICATION
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();
    led_show(0);

    int last_key = -1;

    while (1)
    {
        int key = keypad_scan();

        if (key >= 0 && key != last_key)
        {
            /* Tombol baru ditekan → tampilkan di LED */
            led_show((uint8_t)key);
            last_key = key;
        }
        else if (key < 0)
        {
            last_key = -1;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
