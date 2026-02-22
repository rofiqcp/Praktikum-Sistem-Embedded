/**
 * @file    main.c
 * @brief   P04 - Push Button Pull-Down Resistor External
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Down Resistor EKSTERNAL
 * ================================================================
 *
 * Pull-Down Resistor:
 *   Kebalikan dari pull-up. Resistor terhubung dari GPIO ke GND.
 *
 *   • Tombol TIDAK ditekan: pin = LOW  (0V melalui resistor ke GND)
 *   • Tombol DITEKAN      : pin = HIGH (3.3V masuk melalui tombol)
 *   → Active-HIGH behavior: press = HIGH
 *
 * RANGKAIAN Pull-Down External:
 *
 *   3.3V ─[BTN]─┬─ PB1 (GPIO INPUT, NOPULL)
 *                │
 *             [220Ω]
 *                │
 *               GND
 *
 *   Tombol OPEN  : PB1 = LOW  (0V via 220Ω ke GND) → LED OFF
 *   Tombol PRESS : PB1 = HIGH (3.3V masuk, arus via 220Ω ke GND)
 *   → Active-HIGH: press = HIGH
 *
 * Perbandingan P03 vs P04:
 *   P03 Pull-Up   Ext: pin default=HIGH, press=LOW  (Active-LOW)
 *   P04 Pull-Down Ext: pin default=LOW,  press=HIGH (Active-HIGH)
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
/* Button: INPUT dengan pull-down EXTERNAL */
#define BTN_PORT   GPIOB
#define BTN_PIN    GPIO_PIN_1    /* PB1 ke pin tengah tombol */
/* Rangkaian: VCC ─[BTN]─ PB1 ─[220Ω]─ GND */

/* LED Output: menyala saat tombol ditekan */
#define LED_PORT   GPIOA
#define LED_PIN    GPIO_PIN_1    /* PA1 ─[220Ω]─ LED */

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static uint8_t led_state = 0;
static uint8_t last_btn  = 0;   /* 0=LOW=OPEN (default dengan pull-down) */

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
         *  BACA STATUS TOMBOL
         *  GPIO_PIN_SET   (1) = HIGH = tombol DITEKAN   (active-high)
         *  GPIO_PIN_RESET (0) = LOW  = tombol TIDAK DITEKAN
         * ===================================================== */
        GPIO_PinState btn = HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN);

        /* =====================================================
         *  TOGGLE LED saat tombol baru ditekan
         *  Deteksi RISING EDGE: LOW → HIGH
         *  (last_btn=0 artinya sebelumnya LOW=tidak ditekan,
         *   btn=1  artinya sekarang HIGH=ditekan → tekan baru)
         * ===================================================== */
        if ((btn == GPIO_PIN_SET) && (last_btn == 0))
        {
            /* Tombol baru ditekan → toggle LED */
            led_state = !led_state;
            HAL_GPIO_WritePin(LED_PORT, LED_PIN,
                              led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);

            HAL_Delay(50);  /* Simple debounce delay */
        }

        /* Simpan status tombol sebelumnya */
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

    /* ---- PA1: OUTPUT PUSH-PULL (LED Indicator) ---- */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- PB1: INPUT FLOATING (pull-down dari EXTERNAL 220Ω) ----
     *
     *  GPIO_MODE_INPUT + GPIO_NOPULL:
     *    Resistor 220Ω antara PB1 dan GND menjaga pin tetap LOW
     *    saat tombol tidak ditekan (memastikan kondisi terdefinisi).
     *
     *  Saat tombol ditekan: 3.3V → PB1 (HIGH), arus 15mA via 220Ω ke GND.
     *  Resistor berfungsi sebagai current limiter sekaligus pull-down.
     *
     *  CATATAN: Untuk aplikasi nyata, gunakan 10kΩ untuk efisiensi daya.
     *    220Ω dipakai di sini karena itu yang tersedia di kit praktikum.
     */
    GPIO_InitStruct.Pin   = BTN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;    /* ← External pull-down 220Ω */
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
