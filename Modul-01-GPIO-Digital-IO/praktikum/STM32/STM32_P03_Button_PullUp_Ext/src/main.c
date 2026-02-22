/**
 * @file    main.c
 * @brief   P03 - Push Button Pull-Up Resistor External
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Up Resistor EKSTERNAL
 * ================================================================
 *
 * Mengapa butuh pull-up resistor?
 *   Jika pin dikonfigurasi INPUT tanpa pull resistor (floating),
 *   saat tombol tidak ditekan, pin tidak terhubung ke mana-mana.
 *   Kondisi pin = TIDAK TERDEFINISI (noise, bisa terbaca 0 atau 1).
 *   → BERBAHAYA! Program bisa berperilaku random.
 *
 * SOLUSI: Pull-Up Resistor Eksternal
 *   • Hubungkan resistor dari VCC (3.3V) ke pin GPIO.
 *   • Saat tombol TIDAK ditekan: pin = HIGH (3.3V melalui resistor)
 *   • Saat tombol DITEKAN     : pin = LOW  (0V, karena short ke GND)
 *   → Kondisi pin selalu terdefinisi! (Active-LOW behavior)
 *
 * Kenapa 220Ω? (biasanya pakai 10kΩ)
 *   • Pull-up ideal: 10kΩ (arus kecil ~0.33mA saat tombol ditekan)
 *   • 220Ω juga boleh untuk demo (arus ~15mA, masih aman untuk GPIO)
 *   • Resistor lebih kecil = sinyal lebih stabil tapi boros daya
 *
 * GPIO Config: GPIO_MODE_INPUT + GPIO_NOPULL
 *   ← External resistor yang bertugas, bukan internal GPIO
 *
 * ================================================================
 * RANGKAIAN Pull-Up External:
 *
 *   3.3V ─[220Ω]─┬─ PB0 (GPIO INPUT, NOPULL)
 *                 │
 *              [BTN]
 *                 │
 *                GND
 *
 *   Tombol OPEN  : PB0 = HIGH (3.3V via 220Ω) → LED OFF
 *   Tombol PRESS : PB0 = LOW  (0V, short GND)  → LED ON
 *   → Active-LOW: press = LOW
 *
 * LED Indicator:
 *   PA0 ─[220Ω]─ LED+ LED- ─ GND
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
/* Button: INPUT dengan pull-up EXTERNAL */
#define BTN_PORT   GPIOB
#define BTN_PIN    GPIO_PIN_0    /* PB0 ke pin tengah tombol */
/* Rangkaian: 3.3V ─[220Ω]─ PB0 ─[BTN]─ GND */

/* LED Output: menyala saat tombol ditekan */
#define LED_PORT   GPIOA
#define LED_PIN    GPIO_PIN_0    /* PA0 ─[220Ω]─ LED */

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static uint8_t led_state = 0;   /* Status LED saat ini */
static uint8_t last_btn  = 1;   /* Status tombol sebelumnya (1=HIGH=OPEN) */

/* ================================================================
 *  FUNCTION PROTOTYPES
 * ================================================================ */
void SystemClock_Config(void);
static void GPIO_Init(void);

/* ================================================================
 *  MAIN
 * ================================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    /* Padam LED dan onboard di awal */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    while (1)
    {
        /* =====================================================
         *  BACA STATUS TOMBOL
         *  HAL_GPIO_ReadPin() mengembalikan:
         *    GPIO_PIN_RESET (0) = LOW  = tombol DITEKAN   (active-low)
         *    GPIO_PIN_SET   (1) = HIGH = tombol TIDAK DITEKAN
         * ===================================================== */
        GPIO_PinState btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

        /* =====================================================
         *  TOGGLE LED saat tombol baru ditekan
         *  Deteksi FALLING EDGE: HIGH → LOW
         *  (last_btn=1 artinya sebelumnya tidak ditekan,
         *   btn=0 artinya sekarang ditekan → ini tekan baru)
         * ===================================================== */
        if ((btn == GPIO_PIN_RESET) && (last_btn == 1))
        {
            /* Tombol baru ditekan → toggle LED */
            led_state = !led_state;
            HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                              led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

            /* Simple delay untuk debounce dasar (tanpa state machine) */
            HAL_Delay(50);
        }

        /* Simpan status tombol sebelumnya */
        last_btn = (btn == GPIO_PIN_SET) ? 1 : 0;

        HAL_Delay(10);  /* Polling period 10ms */
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

    /* ---- PA0: OUTPUT PUSH-PULL (LED Indicator) ---- */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);  /* LED mati di awal */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- PB0: INPUT FLOATING (pull-up dari EXTERNAL 220Ω) ----
     *
     *  GPIO_MODE_INPUT + GPIO_NOPULL:
     *    Pin dikonfigurasi sebagai input murni.
     *    Tidak ada resistor internal yang aktif.
     *    Pull-up resistor HARUS dari rangkaian LUAR.
     *
     *  Kenapa NOPULL di sini?
     *    Karena kita sengaja demonstrasi pull-up EXTERNAL.
     *    Resistor 220Ω antara 3.3V dan PB0 menjaga pin tetap HIGH
     *    saat tombol tidak ditekan.
     *
     *  PENTING: Tanpa pull-up eksternal DAN tanpa pull internal,
     *    pin akan FLOATING → pembacaan tidak dapat diprediksi!
     */
    GPIO_InitStruct.Pin   = BTN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;    /* ← External resistor yang pull-up */
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
