/**
 * @file main.c
 * @brief P02: LED Output LOW - Dasar GPIO Output Active-Low
 *
 * KONSEP:
 *   Active-LOW: LED menyala ketika pin = LOW (GPIO_PIN_RESET)
 *
 *   Rangkaian PC13 Built-in Blue Pill:
 *     3.3V ──[R]──[LED Anoda(+)]──[LED Katoda(-)]── PC13
 *
 *   PC13 = RESET (0V)  → Beda potensial 3.3V → Arus mengalir → LED MENYALA ✓
 *   PC13 = SET   (3.3V)→ Beda potensial 0V   → Tidak ada arus → LED MATI   ✗
 *
 * PERBANDINGAN:
 *   PA0 (Active HIGH): SET=MENYALA, RESET=MATI
 *   PC13 (Active LOW): RESET=MENYALA, SET=MATI   ← TERBALIK!
 *
 * HASIL YANG DIHARAPKAN:
 *   PC13 dan PA0 blink berlawanan fase:
 *   - Ketika PA0 ON  → PC13 OFF
 *   - Ketika PA0 OFF → PC13 ON
 *
 * Platform: STM32F103C8T6 (Blue Pill)
 */

#include "config.h"

void SystemClock_Config(void);
void MX_GPIO_Init(void);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    while (1)
    {
        /* ================================================
         *  FASE 1: PC13 MENYALA, PA0 MATI
         *  PC13: RESET = 0V → LED ON  (active-low)
         *  PA0:  RESET = 0V → LED OFF (active-high)
         * ================================================ */
        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_RESET); /* PC13 → LED ON  */
        HAL_GPIO_WritePin(LED_EXT_PORT,     LED_EXT_PIN,     GPIO_PIN_RESET); /* PA0  → LED OFF */
        HAL_Delay(BLINK_DELAY_MS);

        /* ================================================
         *  FASE 2: PC13 MATI, PA0 MENYALA
         *  PC13: SET = 3.3V → LED OFF (active-low)
         *  PA0:  SET = 3.3V → LED ON  (active-high)
         * ================================================ */
        HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET);  /* PC13 → LED OFF */
        HAL_GPIO_WritePin(LED_EXT_PORT,     LED_EXT_PIN,     GPIO_PIN_SET);  /* PA0  → LED ON  */
        HAL_Delay(BLINK_DELAY_MS);
    }
}

/* ============================================================
 *  GPIO Initialization
 * ============================================================ */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Kondisi awal: PC13 = HIGH (LED OFF karena active-low) */
    HAL_GPIO_WritePin(LED_BUILTIN_PORT, LED_BUILTIN_PIN, GPIO_PIN_SET);
    /* Kondisi awal: PA0 = LOW (LED OFF karena active-high) */
    HAL_GPIO_WritePin(LED_EXT_PORT, LED_EXT_PIN, GPIO_PIN_RESET);

    /* Konfigurasi PC13 sebagai output (LED built-in active-low) */
    GPIO_InitStruct.Pin   = LED_BUILTIN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_BUILTIN_PORT, &GPIO_InitStruct);

    /* Konfigurasi PA0 sebagai output (LED external active-high) */
    GPIO_InitStruct.Pin   = LED_EXT_PIN;
    HAL_GPIO_Init(LED_EXT_PORT, &GPIO_InitStruct);
}

/* ============================================================
 *  System Clock Configuration
 * ============================================================ */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
