/**
 * @file    main.c
 * @brief   P05 - Push Button Pull-Up Resistor INTERNAL
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Up INTERNAL (~40kΩ)
 * ================================================================
 *
 * Keunggulan Internal Pull-Up vs External:
 *   ✓ TIDAK perlu resistor eksternal → wiring lebih sederhana
 *   ✓ Space saving pada PCB
 *   ✓ Aktifkan dengan satu baris kode: GPIO_PULLUP
 *
 * Resistensi pull-up internal STM32F103:
 *   Nilai tipikal: 30kΩ – 50kΩ (bergantung batch chip)
 *   Arus saat tombol ditekan: 3.3V/40kΩ ≈ 0.083 mA (hemat daya)
 *
 * GPIO Config: GPIO_MODE_INPUT + GPIO_PULLUP
 *   → STM32 HAL mengaktifkan PUPDR register untuk pull-up internal
 *   → Pin default = HIGH (3.3V via ~40kΩ internal)
 *   → Active-LOW: saat tombol ditekan ke GND → pin = LOW
 *
 * ================================================================
 * RANGKAIAN Pull-Up INTERNAL (minimalis!):
 *
 *   VCC ─[~40kΩ internal]─ PB0 ─[BTN]─ GND
 *                              ↑
 *                        (pin PB0 langsung ke 1 kaki tombol,
 *                         kaki lain tombol ke GND)
 *                        Tidak perlu resistor eksternal!
 *
 *   Tombol OPEN  : PB0 = HIGH (via ~40kΩ internal) → LED OFF
 *   Tombol PRESS : PB0 = LOW  (short ke GND)       → LED ON
 *
 * Perbandingan P03 vs P05:
 *   P03: Pull-Up External 220Ω (perlu resistor + kabel)
 *   P05: Pull-Up Internal ~40kΩ (langsung sambung tombol ke GND!)
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
/* Button: INPUT dengan pull-up INTERNAL */
#define BTN_PORT   GPIOB
#define BTN_PIN    GPIO_PIN_0    /* PB0 → satu kaki BTN; kaki lain → GND */

/* LED Output */
#define LED_PORT   GPIOA
#define LED_PIN    GPIO_PIN_0    /* PA0 ─[220Ω]─ LED */

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static uint8_t led_state = 0;
static uint8_t last_btn  = 1;   /* Pull-up: default HIGH = tidak ditekan */

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

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    while (1)
    {
        /* =====================================================
         *  BACA TOMBOL
         *  Dengan pull-up internal ~40kΩ:
         *    GPIO_PIN_SET   = HIGH = pin via pull-up = TIDAK DITEKAN
         *    GPIO_PIN_RESET = LOW  = pin ke GND     = DITEKAN
         *  Sama behavior dengan P03 (pull-up external)!
         * ===================================================== */
        GPIO_PinState btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

        /* Toggle LED pada falling edge (HIGH → LOW) */
        if ((btn == GPIO_PIN_RESET) && (last_btn == 1))
        {
            led_state = !led_state;
            HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                              led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_Delay(50);
        }

        last_btn = (btn == GPIO_PIN_SET) ? 1 : 0;
        HAL_Delay(10);
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

    /* ---- PA0: OUTPUT PUSH-PULL (LED) ---- */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- PB0: INPUT dengan PULL-UP INTERNAL ----
     *
     *  GPIO_PULLUP mengaktifkan resistor ~40kΩ internal
     *  antara VCC dan PB0 melalui register PUPDR:
     *    PUPDR[1:0] = 01 → pull-up aktif
     *
     *  Hasil: PB0 default = HIGH (3.3V) tanpa resistor eksternal.
     *  Saat tombol ditekan (short ke GND) → PB0 = LOW.
     *
     *  KELEBIHAN:
     *    - Tidak butuh resistor luar → lebih sedikit komponen
     *    - Hemat daya: 3.3V / 40kΩ = 82.5 µA saat ditekan
     *
     *  KEKURANGAN:
     *    - Nilai tidak presisi (30-50kΩ, non-guaranteed)
     *    - Tidak bisa digunakan jika pin butuh pull berbeda
     */
    GPIO_InitStruct.Pin   = BTN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;    /* ← INTERNAL pull-up ~40kΩ */
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
