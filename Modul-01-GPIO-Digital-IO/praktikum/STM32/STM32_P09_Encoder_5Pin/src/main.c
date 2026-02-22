/**
 * @file    main.c
 * @brief   P09 - Rotary Encoder 5-Pin (KY-040 type)
 *
 * ================================================================
 * KONSEP: ROTARY ENCODER — Quadrature Signal Decoding
 * ================================================================
 *
 * Rotary Encoder 5-pin (KY-040 atau serupa):
 *   Pin 1: GND
 *   Pin 2: VCC (3.3V)
 *   Pin 3: SW  → push-button built-in  (active-low, perlu pull-up)
 *   Pin 4: DT  → data/phase signal     (quadrature channel B)
 *   Pin 5: CLK → clock signal          (quadrature channel A)
 *
 * QUADRATURE ENCODING:
 *   CLK dan DT adalah sinyal kotak 90° tergeser. Pola berulang
 *   setiap tick. Dengan mengamati CLK falling edge dan status DT:
 *
 *   Rotasi CLOCKWISE (CW, +):
 *     CLK ↓ (falling) saat DT = HIGH → hitung naik
 *
 *   Rotasi COUNTER-CLOCKWISE (CCW, -):
 *     CLK ↓ (falling) saat DT = LOW  → hitung turun
 *
 *   Sinyal (CW):                    (CCW):
 *   CLK: ─┐  ┌─┐              CLK: ─┐  ┌─┐
 *         └──┘  └─             DT:  ─┐┌─┘  └─
 *   DT:  ──┐┌──┘                    └┘
 *          └┘
 *
 * OUTPUT:
 *   8 LED PA0-PA7 menampilkan count (0-255) dalam BINER
 *   SW tekan: reset count ke 128 (tengah)
 *   CW (+1), CCW (-1)
 *
 * ================================================================
 * RANGKAIAN:
 *   Encoder GND → GND
 *   Encoder VCC → 3.3V
 *   Encoder CLK → PB12  (INPUT, NOPULL — encoder sudah ada pull)
 *   Encoder DT  → PB13  (INPUT, NOPULL)
 *   Encoder SW  → PB14  (INPUT, PULLUP — button ke GND saat tekan)
 *   PA0─[220Ω]─LED1─GND  ...  PA7─[220Ω]─LED8─GND
 *
 * CATATAN: Jika encoder tidak memiliki pull-up internal (tanpa modul),
 *   gunakan PULLUP pada CLK dan DT juga.
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define ENC_PORT   GPIOB
#define ENC_CLK    GPIO_PIN_12   /* PB12: CLK (channel A) */
#define ENC_DT     GPIO_PIN_13   /* PB13: DT  (channel B) */
#define ENC_SW     GPIO_PIN_14   /* PB14: SW  (push button) */

#define LED_PORT   GPIOA
#define LED_ALL    (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|\
                    GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)

/* ================================================================
 *  ENCODER STATE
 * ================================================================ */
static int16_t  enc_count     = 128;  /* Mulai dari tengah (0-255) */
static uint8_t  last_clk      = 1;   /* State CLK sebelumnya */
static uint8_t  last_sw       = 1;   /* State SW sebelumnya */
static uint32_t last_enc_time = 0;   /* Untuk debounce encoder */
static uint32_t last_sw_time  = 0;   /* Untuk debounce SW */

#define ENC_DEBOUNCE_MS  5U    /* Debounce encoder: 5ms */
#define SW_DEBOUNCE_MS   50U   /* Debounce SW: 50ms */

/* ================================================================
 *  FUNCTION PROTOTYPES
 * ================================================================ */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void Encoder_Read(void);
static void LED_Show_Count(void);

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    /* Tampilkan nilai awal di LED */
    LED_Show_Count();

    while (1)
    {
        Encoder_Read();
        HAL_Delay(1);  /* Polling 1ms untuk encoder */
    }
}

/* ================================================================
 *  BACA ENCODER: QUADRATURE DECODING + SW
 * ================================================================ */
static void Encoder_Read(void)
{
    uint32_t now = HAL_GetTick();

    /* ---- CLK: Deteksi FALLING EDGE ---- */
    uint8_t clk = (HAL_GPIO_ReadPin(ENC_PORT, ENC_CLK) == GPIO_PIN_SET) ? 1 : 0;
    uint8_t dt  = (HAL_GPIO_ReadPin(ENC_PORT, ENC_DT)  == GPIO_PIN_SET) ? 1 : 0;

    /* Deteksi falling edge CLK: sebelumnya HIGH (1), sekarang LOW (0) */
    if ((last_clk == 1) && (clk == 0))
    {
        /* debounce encoder: abaikan jika terlalu cepat */
        if ((now - last_enc_time) >= ENC_DEBOUNCE_MS)
        {
            last_enc_time = now;
            if (dt == 1)
            {
                /* DT HIGH saat CLK turun → Clockwise (CW) → hitung naik */
                enc_count++;
                if (enc_count > 255) enc_count = 255;
            }
            else
            {
                /* DT LOW saat CLK turun → Counter-Clockwise (CCW) → hitung turun */
                enc_count--;
                if (enc_count < 0) enc_count = 0;
            }
            LED_Show_Count();
        }
    }
    last_clk = clk;

    /* ---- SW: Tombol encoder (active-low, pull-up) ---- */
    uint8_t sw = (HAL_GPIO_ReadPin(ENC_PORT, ENC_SW) == GPIO_PIN_SET) ? 1 : 0;

    /* Deteksi falling edge SW */
    if ((last_sw == 1) && (sw == 0))
    {
        if ((now - last_sw_time) >= SW_DEBOUNCE_MS)
        {
            last_sw_time = now;
            /* Reset count ke 128 (tengah range 0-255) */
            enc_count = 128;
            LED_Show_Count();
        }
    }
    last_sw = sw;
}

/* ================================================================
 *  TAMPILKAN COUNT PADA 8 LED DALAM BINER
 *  LED PA0 = bit0 (LSB), PA7 = bit7 (MSB)
 *  Contoh: count=5 = 0b00000101 → PA0=ON, PA1=OFF, PA2=ON, ...
 * ================================================================ */
static void LED_Show_Count(void)
{
    uint8_t val = (uint8_t)(enc_count & 0xFF);
    /* Gunakan BSRR register untuk atomic write semua bit sekaligus:
     *   BSRR bit[15:0] = SET mask, BSRR bit[31:16] = RESET mask
     *   Tulis semuanya sekaligus → tidak ada glitch/flicker */
    uint16_t set_mask   = (uint16_t)val;           /* bit yang harus HIGH */
    uint16_t reset_mask = (uint16_t)(~val & 0xFF); /* bit yang harus LOW  */
    GPIOA->BSRR = ((uint32_t)reset_mask << 16) | set_mask;
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA0-PA7: OUTPUT PUSH-PULL (8 LED indicator) */
    HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_ALL;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* PB12 (CLK): INPUT, PULLUP
     *   Encoder KY-040 module biasanya sudah ada pull-up onboard.
     *   GPIO_PULLUP ditambahkan untuk menjamin kondisi terdefinisi. */
    GPIO_InitStruct.Pin   = ENC_CLK;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ENC_PORT, &GPIO_InitStruct);

    /* PB13 (DT): INPUT, PULLUP */
    GPIO_InitStruct.Pin   = ENC_DT;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ENC_PORT, &GPIO_InitStruct);

    /* PB14 (SW): INPUT, PULLUP (active-low push-button) */
    GPIO_InitStruct.Pin   = ENC_SW;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ENC_PORT, &GPIO_InitStruct);
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
