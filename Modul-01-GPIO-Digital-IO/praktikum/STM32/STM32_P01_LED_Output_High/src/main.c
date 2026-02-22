/**
 * @file    main.c
 * @brief   P01 - LED Output HIGH: GPIO_MODE_OUTPUT_PP
 *
 * ================================================================
 * KONSEP: GPIO OUTPUT PUSH-PULL (Active-HIGH)
 * ================================================================
 *
 * Push-Pull: pin dapat mendorong (PUSH) ke VCC (HIGH)
 *            dan menarik (PULL) ke GND (LOW) secara aktif.
 *
 *   GPIO_PIN_SET   (1) → PA_ = 3.3V → arus mengalir → LED MENYALA
 *   GPIO_PIN_RESET (0) → PA_ = 0V   → tidak ada arus → LED MATI
 *
 * FUNGSI HAL yang dipelajari:
 *   HAL_GPIO_WritePin(Port, Pin, GPIO_PIN_SET/RESET)
 *   HAL_GPIO_TogglePin(Port, Pin)
 *   HAL_GPIO_ReadPin(Port, Pin)
 *
 * DEMO (4 Pola Otomatis):
 *   Pola 1: Semua 8 LED ON  (WritePin SET)
 *   Pola 2: Semua 8 LED OFF (WritePin RESET)
 *   Pola 3: Running Light PA0 → PA7
 *   Pola 4: Binary Counter 0–255 (via register ODR)
 *
 * ================================================================
 * RANGKAIAN (8 LED Active-HIGH + LED onboard Active-LOW):
 *   PA0 ─[220Ω]─ LED+ LED- ─ GND    (LED 1)
 *   PA1 ─[220Ω]─ LED+ LED- ─ GND    (LED 2)
 *   PA2 ─[220Ω]─ LED+ LED- ─ GND    (LED 3)
 *   PA3 ─[220Ω]─ LED+ LED- ─ GND    (LED 4)
 *   PA4 ─[220Ω]─ LED+ LED- ─ GND    (LED 5)
 *   PA5 ─[220Ω]─ LED+ LED- ─ GND    (LED 6)
 *   PA6 ─[220Ω]─ LED+ LED- ─ GND    (LED 7)
 *   PA7 ─[220Ω]─ LED+ LED- ─ GND    (LED 8)
 *   PC13= LED Onboard Blue Pill     (Active-LOW, sudah built-in)
 *
 * PLATFORM : STM32F103C8T6 (Blue Pill)
 * FRAMEWORK: STM32Cube HAL
 * CLOCK    : HSI (8MHz internal) → PLL ×16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
/* 8 LED External pada GPIOA (Active-HIGH) */
#define LED_PORT    GPIOA
#define LED_ALL     (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                     GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7)

/* LED Onboard PC13 Blue Pill (Active-LOW) */
#define LED_OB_PORT  GPIOC
#define LED_OB_PIN   GPIO_PIN_13

/* Array individual pin untuk running light */
static const uint16_t LED[8] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7
};

/* ================================================================
 *  TIMING (ms)
 * ================================================================ */
#define T_ALL_ON   800    /* Semua LED menyala */
#define T_ALL_OFF  400    /* Semua LED mati */
#define T_RUNNING  100    /* Running light per LED */
#define T_BINARY    50    /* Binary counter per step */
#define T_PAUSE    600    /* Jeda antar pola */

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
        /* =====================================================
         *  POLA 1: Semua LED ON
         *  HAL_GPIO_WritePin(port, pins, GPIO_PIN_SET)
         *  → semua PA0-PA7 = 3.3V → 8 LED menyala bersamaan
         *  → PC13 RESET = 0V → LED onboard menyala (active-low)
         * ===================================================== */
        HAL_GPIO_WritePin(LED_PORT,    LED_ALL,    GPIO_PIN_SET);    /* ext ON  */
        HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_RESET);  /* obd ON  */
        HAL_Delay(T_ALL_ON);

        /* =====================================================
         *  POLA 2: Semua LED OFF
         *  GPIO_PIN_RESET → pin = 0V → tidak ada beda potensial
         *  → LED tidak mendapat arus → mati
         * ===================================================== */
        HAL_GPIO_WritePin(LED_PORT,    LED_ALL,    GPIO_PIN_RESET);  /* ext OFF */
        HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_SET);    /* obd OFF */
        HAL_Delay(T_ALL_OFF);

        /* =====================================================
         *  POLA 3: Running Light  PA0 → PA1 → ... → PA7
         *  Hanya 1 LED menyala dalam satu waktu
         *  Bergeser setiap T_RUNNING ms
         * ===================================================== */
        for (int i = 0; i < 8; i++)
        {
            HAL_GPIO_WritePin(LED_PORT, LED_ALL,  GPIO_PIN_RESET); /* matikan semua */
            HAL_GPIO_WritePin(LED_PORT, LED[i],   GPIO_PIN_SET);   /* nyalakan satu */
            HAL_Delay(T_RUNNING);
        }
        HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
        HAL_Delay(T_PAUSE);

        /* =====================================================
         *  POLA 4: Binary Counter 0 → 255
         *  PA0 = bit0 (LSB) ... PA7 = bit7 (MSB)
         *  Nilai biner terlihat dari pola LED
         *  Contoh: 5 = 0b00000101 → PA0=ON, PA1=OFF, PA2=ON
         *
         *  Optimasi: tulis langsung ke register ODR
         *  ODR (Output Data Register) adalah register 16-bit
         *  yang mengontrol output semua pin port A.
         *  bit[0] = PA0, bit[1] = PA1, ..., bit[7] = PA7
         *
         *  Mask 0xFF00 menjaga bit PA8-PA15 tidak berubah.
         * ===================================================== */
        for (uint16_t cnt = 0; cnt <= 255; cnt++)
        {
            GPIOA->ODR = (GPIOA->ODR & 0xFF00U) | (uint16_t)(cnt & 0xFFU);
            HAL_Delay(T_BINARY);
        }
        HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
        HAL_Delay(T_PAUSE);
    }
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Aktifkan clock AHB untuk GPIOA dan GPIOC */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Kondisi awal: semua LED mati */
    HAL_GPIO_WritePin(LED_PORT,    LED_ALL,    GPIO_PIN_RESET);  /* ext = 0V  = mati */
    HAL_GPIO_WritePin(LED_OB_PORT, LED_OB_PIN, GPIO_PIN_SET);   /* PC13=3.3V = mati (active-low) */

    /* ---- Konfigurasi PA0-PA7: OUTPUT PUSH-PULL ----
     *
     *  Mode  = GPIO_MODE_OUTPUT_PP
     *    Push-Pull: P-MOS (untuk HIGH) dan N-MOS (untuk LOW)
     *    keduanya tersedia. Pin dapat drive HIGH kuat (3.3V, ~8mA)
     *    maupun LOW kuat (0V, ~8mA).
     *
     *  Pull  = GPIO_NOPULL
     *    LED adalah beban itu sendiri. Tidak butuh pull-up/down.
     *    Menambahkan pull-up akan boros daya saat LED mati.
     *
     *  Speed = GPIO_SPEED_FREQ_LOW  (2 MHz slew rate)
     *    Cukup untuk LED. Tidak butuh switching cepat.
     *    Semakin rendah speed → EMI lebih rendah, daya lebih hemat.
     */
    GPIO_InitStruct.Pin   = LED_ALL;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* ---- Konfigurasi PC13: OUTPUT PUSH-PULL (LED onboard active-low) ---- */
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
