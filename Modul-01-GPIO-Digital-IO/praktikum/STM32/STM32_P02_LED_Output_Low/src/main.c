/**
 * @file    main.c
 * @brief   P02 - LED Output LOW: Active-LOW & Open-Drain
 *
 * ================================================================
 * KONSEP 1: ACTIVE-LOW (PC13 Blue Pill built-in)
 * ================================================================
 * Rangkaian: VCC ─[R]─ LED+ LED- ─ PC13
 *
 *   PC13 = LOW  (RESET, 0V)   → beda potensial = VCC → arus ada → LED NYALA
 *   PC13 = HIGH (SET,   3.3V) → beda potensial = 0   → arus = 0 → LED MATI
 *
 * Ini TERBALIK dari Active-HIGH! RESET = MENYALA, SET = MATI.
 * Blue Pill menggunakan konfigurasi ini untuk LED onboard PC13.
 *
 * ================================================================
 * KONSEP 2: OPEN-DRAIN vs PUSH-PULL
 * ================================================================
 * PUSH-PULL (PA0):
 *   ODR=1 → P-MOS ON, N-MOS OFF → PA0 = VCC (3.3V, aktif kuat)
 *   ODR=0 → P-MOS OFF, N-MOS ON → PA0 = GND (0V,   aktif kuat)
 *
 * OPEN-DRAIN (PA1):
 *   ODR=0 → N-MOS ON  → PA1 = GND (0V, aktif kuat) ✓
 *   ODR=1 → N-MOS OFF → PA1 = Hi-Z (mengambang!) ← TIDAK BISA drive HIGH!
 *   Agar PA1 bisa HIGH saat ODR=1, HARUS ada pull-up:
 *   - External pull-up 10kΩ: PA1 = VCC kuat
 *   - Internal pull-up ~40kΩ: PA1 = VCC lemah (LED redup)
 *
 * DEMO PERBANDINGAN:
 *   PA0 PP HIGH → LED terang  (impedansi rendah, arus besar)
 *   PA1 OD HIGH → LED redup   (through 40kΩ internal, arus kecil)
 *   PA1 OD LOW  → LED mati    (N-MOS tarik ke GND)
 *
 * ================================================================
 * RANGKAIAN:
 *   VCC ─[bawaan]─ LED+ LED- ─ PC13    (Active-LOW onboard)
 *   PA0 ─[220Ω]─ LED+ LED- ─ GND      (Push-Pull, Active-HIGH)
 *   PA1 ─[220Ω]─ LED+ LED- ─ GND      (Open-Drain + internal PULLUP)
 *
 * PLATFORM : STM32F103C8T6 (Blue Pill)
 * FRAMEWORK: STM32Cube HAL
 * CLOCK    : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define LED_PP_PORT   GPIOA
#define LED_PP_PIN    GPIO_PIN_0    /* Push-Pull output (Active-HIGH) */

#define LED_OD_PORT   GPIOA
#define LED_OD_PIN    GPIO_PIN_1    /* Open-Drain output + internal pull-up */

#define LED_OB_PORT   GPIOC
#define LED_OB_PIN    GPIO_PIN_13  /* Onboard LED (Active-LOW) */

#define DELAY_MS  700

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

    while (1)
    {
        /* =======================================================
         *  TAHAP 1: Semua LED menyala bersamaan
         *  PC13 RESET (0V) → Active-LOW onboard ON
         *  PA0  SET   (3.3V) → Push-Pull ON (terang)
         *  PA1  RESET (0V) → Open-Drain N-MOS active → mati
         * ======================================================= */
        HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_RESET); /* PC13 ON  (active-low) */
        HAL_GPIO_WritePin(LED_PP_PORT, LED_PP_PIN, GPIO_PIN_SET);   /* PA0  ON  (active-high PP) */
        HAL_GPIO_WritePin(LED_OD_PORT, LED_OD_PIN, GPIO_PIN_RESET); /* PA1  OFF (OD pulls LOW)   */
        HAL_Delay(DELAY_MS);

        /* =======================================================
         *  TAHAP 2: Demonstrasi Push-Pull vs Open-Drain HIGH
         *  PA0 SET  → P-MOS aktif → 3.3V low-impedance → LED terang
         *  PA1 SET  → N-MOS off  → Hi-Z → VCC via 40kΩ internal
         *             arus = (3.3-2.0)/(40000+220) ≈ 0.032 mA → LED sangat redup
         *  PC13 SET → Active-LOW MATI
         * ======================================================= */
        HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_SET);   /* PC13 OFF (active-low) */
        HAL_GPIO_WritePin(LED_PP_PORT, LED_PP_PIN, GPIO_PIN_SET);   /* PA0 ON terang  (PP HIGH) */
        HAL_GPIO_WritePin(LED_OD_PORT, LED_OD_PIN, GPIO_PIN_SET);   /* PA1 ON redup   (OD Hi-Z + pullup) */
        HAL_Delay(DELAY_MS);

        /* =======================================================
         *  TAHAP 3: Hanya Open-Drain LOW (N-MOS aktif)
         *  PA1 RESET → N-MOS ON → PA1 = 0V kuat → LED mati
         *  Ini satu-satunya cara OD drive LOW secara aktif kuat
         * ======================================================= */
        HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_RESET); /* PC13 ON  */
        HAL_GPIO_WritePin(LED_PP_PORT, LED_PP_PIN, GPIO_PIN_RESET); /* PA0 OFF  (PP LOW)  */
        HAL_GPIO_WritePin(LED_OD_PORT, LED_OD_PIN, GPIO_PIN_RESET); /* PA1 OFF  (OD LOW aktif) */
        HAL_Delay(DELAY_MS);

        /* =======================================================
         *  TAHAP 4: Semua mati
         *  PC13 SET  → Active-LOW OFF
         *  PA0  RESET → PP mati
         *  PA1  SET   → OD Hi-Z → via pull-up → LED sangat redup/mati
         * ======================================================= */
        HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_SET);   /* PC13 OFF */
        HAL_GPIO_WritePin(LED_PP_PORT, LED_PP_PIN, GPIO_PIN_RESET); /* PA0  OFF */
        HAL_GPIO_WritePin(LED_OD_PORT, LED_OD_PIN, GPIO_PIN_SET);   /* PA1  Hi-Z (redup) */
        HAL_Delay(DELAY_MS);
    }
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Default: semua LED mati */
    HAL_GPIO_WritePin(LED_PP_PORT, LED_PP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_OD_PORT, LED_OD_PIN, GPIO_PIN_SET);   /* OD: SET = Hi-Z */
    HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_SET);   /* PC13 Active-LOW: SET = OFF */

    /* PA0: OUTPUT PUSH-PULL (Active-HIGH reference)
     *   Bisa drive HIGH kuat (3.3V, impedansi rendah)
     *   Bisa drive LOW  kuat (0V,   impedansi rendah)
     */
    GPIO_InitStruct.Pin   = LED_PP_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;   /* Push-Pull */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PP_PORT, &GPIO_InitStruct);

    /* PA1: OUTPUT OPEN-DRAIN + internal PULLUP
     *   GPIO_MODE_OUTPUT_OD: hanya N-MOS (pull LOW aktif)
     *   GPIO_PULLUP: tambahkan ~40kΩ dari VCC ke PA1 (untuk HIGH lemah)
     *
     *   ODR=0 → N-MOS ON  → PA1 = 0V (aktif kuat)
     *   ODR=1 → N-MOS OFF → PA1 float → VCC via 40kΩ (lemah, arus kecil)
     */
    GPIO_InitStruct.Pin   = LED_OD_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;   /* Open-Drain */
    GPIO_InitStruct.Pull  = GPIO_PULLUP;           /* ~40kΩ internal pull-up */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_OD_PORT, &GPIO_InitStruct);

    /* PC13: OUTPUT PUSH-PULL (Active-LOW onboard LED) */
    GPIO_InitStruct.Pin   = LED_OB_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_OB_PORT, &GPIO_InitStruct);
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
