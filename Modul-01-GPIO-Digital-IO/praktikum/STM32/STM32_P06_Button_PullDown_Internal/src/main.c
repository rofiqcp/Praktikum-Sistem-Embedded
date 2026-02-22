/**
 * @file    main.c
 * @brief   P06 - Push Button Pull-Down Resistor INTERNAL
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Down INTERNAL (~40kΩ)
 * ================================================================
 *
 * Pull-Down Internal:
 *   STM32 menyediakan resistor pull-down internal ~40kΩ dari pin ke GND.
 *   Diaktifkan via PUPDR register: PUPDR[1:0] = 10 → pull-down.
 *
 *   GPIO Config: GPIO_MODE_INPUT + GPIO_PULLDOWN
 *
 *   Pin default = LOW (0V via ~40kΩ internal ke GND)
 *   Saat tombol ditekan → pin = HIGH (3.3V masuk)
 *   → Active-HIGH: press = HIGH
 *
 * ================================================================
 * RANGKAIAN Pull-Down INTERNAL (minimalis!):
 *
 *   3.3V ─[BTN]─ PB1 ─[~40kΩ internal]─ GND
 *                  ↑
 *           (kaki tombol 1 ke VCC 3.3V,
 *            kaki tombol 2 ke PB1)
 *           Tidak perlu resistor eksternal!
 *
 *   Tombol OPEN  : PB1 = LOW  (via ~40kΩ internal ke GND) → LED OFF
 *   Tombol PRESS : PB1 = HIGH (3.3V masuk dari VCC)       → LED ON
 *
 * Perbandingan P04 vs P06:
 *   P04: Pull-Down External 220Ω (perlu resistor + kabel)
 *   P06: Pull-Down Internal ~40kΩ (langsung sambung tombol ke 3.3V!)
 *
 * LED: PA1 ─[220Ω]─ LED+ LED- ─ GND
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
/* Button: INPUT dengan pull-down INTERNAL */
#define BTN_PORT   GPIOB
#define BTN_PIN    GPIO_PIN_1    /* PB1 → satu kaki BTN; kaki lain → 3.3V */

/* LED Output */
#define LED_PORT   GPIOA
#define LED_PIN    GPIO_PIN_1    /* PA1 ─[220Ω]─ LED */

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static uint8_t led_state = 0;
static uint8_t last_btn  = 0;   /* Pull-down: default LOW = tidak ditekan */

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
         *  Dengan pull-down internal ~40kΩ:
         *    GPIO_PIN_RESET = LOW  = pin via pull-down = TIDAK DITEKAN
         *    GPIO_PIN_SET   = HIGH = 3.3V masuk dari BTN = DITEKAN
         * ===================================================== */
        GPIO_PinState btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

        /* Toggle LED pada rising edge (LOW → HIGH) */
        if ((btn == GPIO_PIN_SET) && (last_btn == 0))
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

    /* ---- PA1: OUTPUT PUSH-PULL (LED) ---- */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- PB1: INPUT dengan PULL-DOWN INTERNAL ----
     *
     *  GPIO_PULLDOWN mengaktifkan resistor ~40kΩ internal
     *  antara PB1 dan GND melalui register PUPDR:
     *    PUPDR[3:2] = 10 → pull-down aktif
     *
     *  Hasil: PB1 default = LOW (0V) tanpa komponen luar.
     *  Saat tombol ditekan (hubungkan PB1 ke 3.3V) → PB1 = HIGH.
     *  Arus saat ditekan: 3.3V / 40kΩ ≈ 82.5 µA (mengalir ke GND)
     *
     *  PENTING: Jangan hubungkan langsung ke 5V!
     *    STM32F103C8T6 adalah 3.3V MCU. Input max = 3.6V.
     *    GPIO tidak 5V tolerant kecuali ditandai "FT" di datasheet.
     */
    GPIO_InitStruct.Pin   = BTN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;  /* ← INTERNAL pull-down ~40kΩ */
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
