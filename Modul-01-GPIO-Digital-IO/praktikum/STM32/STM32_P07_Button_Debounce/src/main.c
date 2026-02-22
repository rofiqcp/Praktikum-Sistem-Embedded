/**
 * @file    main.c
 * @brief   P07 - Push Button Debounce
 *
 * ================================================================
 * KONSEP: BUTTON DEBOUNCING (Software)
 * ================================================================
 *
 * Masalah BOUNCING:
 *   Saat tombol ditekan, kontak mekanikal memantul (bounce)
 *   selama 1-20 ms sebelum stabil. CPU yang membaca ulang cepat
 *   akan mendeteksi banyak "tekan" palsu dari satu tekan fisik.
 *
 *   Contoh tanpa debounce:
 *     1 tekan fisik → bisa terdeteksi 5-20x press
 *     → LED toggle 5-20x → tampak berkedip random
 *
 * SOLUSI: Software Debounce dengan HAL_GetTick()
 *   HAL_GetTick() mengembalikan waktu sistem dalam MILLISECONDS
 *   (diinkremen setiap 1ms oleh SysTick_Handler → HAL_IncTick).
 *
 *   STATE MACHINE DEBOUNCING untuk setiap tombol:
 *   ┌─────────────┐  pin=LOW      ┌──────────────┐  hold >50ms  ┌─────────────┐
 *   │   RELEASED  │──────────────→│  DEBOUNCING  │─────────────→│   PRESSED   │
 *   └─────────────┘               └──────────────┘               └─────────────┘
 *          ↑                            │ pin=HIGH                      │
 *          │                            ↓ (bounce, reset timer)         │ pin=HIGH
 *          └───────────────────────────────────────────────────────────┘
 *
 *   Logika:
 *   1. Saat pin pertama kali LOW → catat waktu (debounce_start)
 *   2. Jika dalam DEBOUNCE_MS (50ms) pin kembali HIGH → masih bounce
 *   3. Jika tetap LOW selama DEBOUNCE_MS → tekan valid → toggle LED
 *
 * 4 TOMBOL + 4 LED INDEPENDEN:
 *   BTN1 (PB0) ↔ LED1 (PA0)
 *   BTN2 (PB1) ↔ LED2 (PA1)
 *   BTN3 (PB3) ↔ LED3 (PA2)   ← skip PB2 (BOOT1/strapping pin)
 *   BTN4 (PB4) ↔ LED4 (PA3)
 *
 * ================================================================
 * RANGKAIAN:
 *   PB0 ─[BTN1]─ GND  (internal pull-up aktif)
 *   PB1 ─[BTN2]─ GND
 *   PB3 ─[BTN3]─ GND
 *   PB4 ─[BTN4]─ GND
 *   PA0 ─[220Ω]─ LED1 ─ GND
 *   PA1 ─[220Ω]─ LED2 ─ GND
 *   PA2 ─[220Ω]─ LED3 ─ GND
 *   PA3 ─[220Ω]─ LED4 ─ GND
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
/* 4 Button pins (INPUT, internal PULLUP, Active-LOW) */
static const uint16_t BTN_PINS[4] = {
    GPIO_PIN_0,  /* PB0: BTN1 */
    GPIO_PIN_1,  /* PB1: BTN2 */
    GPIO_PIN_3,  /* PB3: BTN3 (skip PB2 = BOOT1) */
    GPIO_PIN_4   /* PB4: BTN4 */
};
#define BTN_PORT  GPIOB

/* 4 LED pins (OUTPUT, Active-HIGH) */
static const uint16_t LED_PINS[4] = {
    GPIO_PIN_0,  /* PA0: LED1 */
    GPIO_PIN_1,  /* PA1: LED2 */
    GPIO_PIN_2,  /* PA2: LED3 */
    GPIO_PIN_3   /* PA3: LED4 */
};
#define LED_PORT  GPIOA

/* ================================================================
 *  DEBOUNCE CONFIG
 * ================================================================ */
#define DEBOUNCE_MS  50U   /* Waktu stabilisasi tombol (ms) */
#define NUM_BTN      4     /* Jumlah tombol */

/* ================================================================
 *  BUTTON DEBOUNCE STATE MACHINE
 * ================================================================ */
typedef enum {
    BTN_STATE_RELEASED,    /* Tombol tidak ditekan */
    BTN_STATE_DEBOUNCING,  /* Terdeteksi LOW, tunggu stabil */
    BTN_STATE_PRESSED,     /* Tekan sudah dikonfirmasi valid */
} btn_state_t;

static btn_state_t btn_state[NUM_BTN]      = {BTN_STATE_RELEASED};
static uint32_t    btn_debounce_ms[NUM_BTN] = {0};
static uint8_t     led_state[NUM_BTN]       = {0};

/* ================================================================
 *  FUNCTION PROTOTYPES
 * ================================================================ */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void Button_Process(void);

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    while (1)
    {
        Button_Process();
        HAL_Delay(5);  /* Polling setiap 5ms: lebih dari cukup untuk 50ms debounce */
    }
}

/* ================================================================
 *  BUTTON DEBOUNCE PROCESSING
 *  Mengelola state machine untuk semua 4 tombol secara independen
 * ================================================================ */
static void Button_Process(void)
{
    uint32_t now = HAL_GetTick();  /* Waktu sekarang dalam ms */

    for (int i = 0; i < NUM_BTN; i++)
    {
        /* Baca status pin tombol (active-low: ditekan = LOW = RESET) */
        GPIO_PinState pin = HAL_GPIO_ReadPin(BTN_PORT, BTN_PINS[i]);

        switch (btn_state[i])
        {
            /* =================================================
             *  STATE: RELEASED (tombol tidak ditekan)
             * ================================================= */
            case BTN_STATE_RELEASED:
                if (pin == GPIO_PIN_RESET)
                {
                    /* Pin baru jadi LOW → mulai debounce timer */
                    btn_debounce_ms[i] = now;
                    btn_state[i] = BTN_STATE_DEBOUNCING;
                }
                break;

            /* =================================================
             *  STATE: DEBOUNCING (tunggu sampai stabil)
             * ================================================= */
            case BTN_STATE_DEBOUNCING:
                if (pin == GPIO_PIN_SET)
                {
                    /* Pin kembali HIGH dalam DEBOUNCE_MS → masih bounce */
                    btn_state[i] = BTN_STATE_RELEASED;
                }
                else if ((now - btn_debounce_ms[i]) >= DEBOUNCE_MS)
                {
                    /* Tetap LOW selama >= DEBOUNCE_MS → VALID!
                     * Toggle LED yang sesuai */
                    led_state[i] = !led_state[i];
                    HAL_GPIO_WritePin(LED_PORT, LED_PINS[i],
                                      led_state[i] ? GPIO_PIN_SET : GPIO_PIN_RESET);
                    btn_state[i] = BTN_STATE_PRESSED;
                }
                break;

            /* =================================================
             *  STATE: PRESSED (tekan sudah dikonfirmasi)
             *  Tunggu tombol dilepas sebelum terima tekan berikutnya
             * ================================================= */
            case BTN_STATE_PRESSED:
                if (pin == GPIO_PIN_SET)
                {
                    /* Pin kembali HIGH → tombol dilepas */
                    btn_state[i] = BTN_STATE_RELEASED;
                }
                break;

            default:
                btn_state[i] = BTN_STATE_RELEASED;
                break;
        }
    }
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* ---- PA0-PA3: OUTPUT untuk 4 LED ---- */
    uint16_t all_leds = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_WritePin(LED_PORT, all_leds, GPIO_PIN_RESET);  /* Semua mati di awal */
    GPIO_InitStruct.Pin   = all_leds;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- PB0, PB1, PB3, PB4: INPUT PULLUP untuk 4 Tombol ---- */
    uint16_t all_btns = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Pin   = all_btns;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;      /* Internal ~40kΩ pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BTN_PORT, &GPIO_InitStruct);
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
