/**
 * @file main.c
 * @brief P01: LED Output HIGH - Dasar GPIO Output Active-High
 *
 * KONSEP:
 *   Output HIGH (GPIO_PIN_SET)   → PA0 = 3.3V → Arus mengalir → LED MENYALA
 *   Output LOW  (GPIO_PIN_RESET) → PA0 = 0V   → Tidak ada arus → LED MATI
 *
 * RANGKAIAN:
 *   PA0 ──[220Ω]──[LED Anoda(+)]──[LED Katoda(-)]── GND
 *
 * HASIL YANG DIHARAPKAN:
 *   LED berkedip dengan interval 500ms ON / 500ms OFF
 *
 * Platform: STM32F103C8T6 (Blue Pill)
 * Framework: STM32Cube HAL
 */

#include "config.h"

/* ---- Function Prototypes ---- */
void SystemClock_Config(void);
void MX_GPIO_Init(void);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    /* Inisialisasi HAL (SysTick, HAL_Delay) */
    HAL_Init();

    /* Konfigurasi System Clock: HSE 8MHz → PLL → 72MHz */
    SystemClock_Config();

    /* Inisialisasi GPIO */
    MX_GPIO_Init();

    /* ========================================
     *  LOOP UTAMA
     * ======================================== */
    while (1)
    {
        /* === LANGKAH 1: Output HIGH → LED MENYALA ===
         *   PA0 = 3.3V
         *   Arus: 3.3V → 220Ω → Anoda → Katoda → GND
         *   Arus ≈ (3.3 - 2.0) / 220 ≈ 5.9 mA
         */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        HAL_Delay(BLINK_ON_MS);

        /* === LANGKAH 2: Output LOW → LED MATI ===
         *   PA0 = 0V
         *   Tidak ada beda potensial → tidak ada arus → LED mati
         */
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(BLINK_OFF_MS);
    }
}

/* ============================================================
 *  GPIO Initialization
 * ============================================================ */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock untuk GPIOA */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Pastikan LED mati sebelum dikonfigurasi */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /* Konfigurasi PA0 sebagai OUTPUT PUSH-PULL
     *
     *  Mode OUTPUT_PP   : push-pull (bisa HIGH dan LOW)
     *  Pull NOPULL      : tidak ada internal resistor (sudah ada LED load)
     *  Speed FREQ_LOW   : cukup untuk aplikasi LED (rendah = hemat daya)
     */
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;    /* Output Push-Pull */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;            /* Tidak pakai pull-up/down */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/* ============================================================
 *  System Clock Configuration (72MHz dari HSE 8MHz via PLL)
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Aktifkan HSE Oscillator dan konfigurasi PLL */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL9;  /* 8MHz x 9 = 72MHz */
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Konfigurasi bus clock */
    RCC_ClkInitStruct.ClockType           = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                          | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource        = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider       = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider      = RCC_HCLK_DIV2;  /* APB1 max 36MHz */
    RCC_ClkInitStruct.APB2CLKDivider      = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
