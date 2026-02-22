/**
 * @file    main.c
 * @brief   P08 - GPIO Speed & 8-LED Patterns dengan 4 Button
 *
 * ================================================================
 * KONSEP: GPIO SPEED (Slew Rate) + HAL_GPIO_ReadPin Multi-Input
 * ================================================================
 *
 * GPIO SPEED (Slew Rate / Output Drive Speed):
 *   Speed mengontrol seberapa cepat sinyal GPIO berubah (naik/turun).
 *   BUKAN kecepatan periode toggle! Ini adalah "slew rate" output.
 *
 *   GPIO_SPEED_FREQ_LOW    ≈ 2  MHz slew → tepi sinyal lambat
 *   GPIO_SPEED_FREQ_MEDIUM ≈ 10 MHz slew → tepi sinyal sedang
 *   GPIO_SPEED_FREQ_HIGH   ≈ 50 MHz slew → tepi sinyal cepat (tajam)
 *
 *   Pengaruh ke LED:
 *   - Untuk LED, perbedaan tidak terlihat mata (terlalu cepat)
 *   - Perbedaan terlihat jelas dengan oscilloscope
 *   - Speed tinggi → noise/EMI lebih besar, pakai hanya jika perlu!
 *
 * HAL_GPIO_ReadPin untuk membaca banyak pin:
 *   Bisa baca pin dari port berbeda dalam satu loop.
 *   Gunakan IDR register untuk baca semuanya sekaligus (lebih efisien):
 *     uint16_t port_val = GPIOB->IDR;  // baca semua PB sekaligus
 *
 * FUNGSI YANG DIPELAJARI:
 *   HAL_GPIO_ReadPin()     - baca status pin
 *   HAL_GPIO_WritePin()    - tulis HIGH/LOW ke pin
 *   HAL_GPIO_TogglePin()   - toggle state pin
 *   HAL_GPIO_LockPin()     - kunci konfigurasi pin (tidak bisa diubah)
 *   HAL_GPIO_DeInit()      - reset pin ke default input floating
 *
 * ================================================================
 * DEMO (4 Button memilih pola LED + speed):
 *   BTN1 (PB0) → Pola 1: Running Light kiri (speed LOW)
 *   BTN2 (PB1) → Pola 2: Knight Rider ping-pong (speed MEDIUM)
 *   BTN3 (PB3) → Pola 3: Alternating blink (speed HIGH)
 *   BTN4 (PB4) → Pola 4: Binary Counter (speed MEDIUM)
 *   Tahan BTN bersamaan BTN4: demo HAL_GPIO_LockPin + DeInit
 *
 * RANGKAIAN:
 *   PB0-PB4 ─[BTN_x]─ GND  (internal pull-up)
 *   PA0-PA7 ─[220Ω]─ LED_x ─ GND
 *
 * HARDWARE: STM32F103C8T6 Blue Pill
 * CLOCK   : HSI → PLL × 16 = 64 MHz
 * ================================================================
 */

#include "stm32f1xx_hal.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define LED_PORT    GPIOA
#define LED_ALL     (GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|\
                     GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7)

#define BTN_PORT    GPIOB
#define BTN1_PIN    GPIO_PIN_0   /* Pola 1: Running Light */
#define BTN2_PIN    GPIO_PIN_1   /* Pola 2: Knight Rider  */
#define BTN3_PIN    GPIO_PIN_3   /* Pola 3: Alternating   */
#define BTN4_PIN    GPIO_PIN_4   /* Pola 4: Binary Counter*/

static const uint16_t LED[8] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7
};

/* ================================================================
 *  FUNCTION PROTOTYPES
 * ================================================================ */
void SystemClock_Config(void);
static void GPIO_Init(void);
static void Set_LED_Speed(uint32_t speed);
static void Pattern_Running(uint32_t delay_ms);
static void Pattern_KnightRider(uint32_t delay_ms);
static void Pattern_Alternating(uint32_t delay_ms);
static void Pattern_BinaryCounter(uint32_t delay_ms);
static uint8_t Btn_Pressed(uint16_t pin);

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
         *  BACA IDR REGISTER SEKALIGUS (lebih efisien dari ReadPin)
         *  IDR = Input Data Register: bit[x] = status pin PBx
         *  Active-LOW: bit=0 → PIN LOW → BTN DITEKAN
         * ===================================================== */
        uint16_t idr = GPIOB->IDR;

        if (!(idr & BTN1_PIN))
        {
            /* BTN1: Running Light, GPIO Speed LOW */
            Set_LED_Speed(GPIO_SPEED_FREQ_LOW);
            Pattern_Running(120);
        }
        else if (!(idr & BTN2_PIN))
        {
            /* BTN2: Knight Rider, GPIO Speed MEDIUM */
            Set_LED_Speed(GPIO_SPEED_FREQ_MEDIUM);
            Pattern_KnightRider(80);
        }
        else if (!(idr & BTN3_PIN))
        {
            /* BTN3: Alternating blink, GPIO Speed HIGH */
            Set_LED_Speed(GPIO_SPEED_FREQ_HIGH);
            Pattern_Alternating(300);
        }
        else if (!(idr & BTN4_PIN))
        {
            /* BTN4: Binary Counter, GPIO Speed MEDIUM */
            Set_LED_Speed(GPIO_SPEED_FREQ_MEDIUM);
            Pattern_BinaryCounter(40);
        }
        else
        {
            /* Tidak ada tombol: semua LED mati */
            HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
            HAL_Delay(20);
        }
    }
}

/* ================================================================
 *  UBAH GPIO SPEED untuk PA0-PA7
 *  Demonstrasi: ubah slew rate GPIO saat runtime
 * ================================================================ */
static void Set_LED_Speed(uint32_t speed)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = LED_ALL;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = speed;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/* ================================================================
 *  POLA 1: Running Light (PA0 → PA7)
 *  1 LED aktif, bergeser kanan setiap delay_ms
 * ================================================================ */
static void Pattern_Running(uint32_t delay_ms)
{
    for (int i = 0; i < 8; i++)
    {
        if (Btn_Pressed(BTN1_PIN) == 0) return;  /* Keluar jika BTN1 dilepas */
        HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_PORT, LED[i], GPIO_PIN_SET);
        HAL_Delay(delay_ms);
    }
}

/* ================================================================
 *  POLA 2: Knight Rider (ping-pong PA0↔PA7)
 *  Gerak maju lalu mundur
 * ================================================================ */
static void Pattern_KnightRider(uint32_t delay_ms)
{
    for (int i = 0; i < 8; i++)
    {
        if (Btn_Pressed(BTN2_PIN) == 0) return;
        HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_PORT, LED[i], GPIO_PIN_SET);
        HAL_Delay(delay_ms);
    }
    for (int i = 6; i >= 1; i--)
    {
        if (Btn_Pressed(BTN2_PIN) == 0) return;
        HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_PORT, LED[i], GPIO_PIN_SET);
        HAL_Delay(delay_ms);
    }
}

/* ================================================================
 *  POLA 3: Alternating Blink
 *  PA0,PA2,PA4,PA6 ON ↔ PA1,PA3,PA5,PA7 ON
 * ================================================================ */
static void Pattern_Alternating(uint32_t delay_ms)
{
    uint16_t even = GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_6;
    uint16_t odd  = GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_7;

    if (Btn_Pressed(BTN3_PIN) == 0) return;
    HAL_GPIO_WritePin(LED_PORT, even, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_PORT, odd,  GPIO_PIN_RESET);
    HAL_Delay(delay_ms);

    if (Btn_Pressed(BTN3_PIN) == 0) return;
    HAL_GPIO_WritePin(LED_PORT, even, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, odd,  GPIO_PIN_SET);
    HAL_Delay(delay_ms);
}

/* ================================================================
 *  POLA 4: Binary Counter (GPIOA->ODR direct write)
 *  Demonstrasi akses register langsung untuk efisiensi
 * ================================================================ */
static void Pattern_BinaryCounter(uint32_t delay_ms)
{
    static uint16_t cnt = 0;
    if (Btn_Pressed(BTN4_PIN) == 0) { cnt = 0; return; }
    GPIOA->ODR = (GPIOA->ODR & 0xFF00U) | (cnt & 0xFFU);
    cnt = (cnt >= 255) ? 0 : (cnt + 1);
    HAL_Delay(delay_ms);
}

/* ================================================================
 *  CEK APAKAH TOMBOL TERTENTU SEDANG DITEKAN (active-low)
 *  Kembalikan 1 jika ditekan, 0 jika tidak
 * ================================================================ */
static uint8_t Btn_Pressed(uint16_t pin)
{
    return (HAL_GPIO_ReadPin(BTN_PORT, pin) == GPIO_PIN_RESET) ? 1 : 0;
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA0-PA7: OUTPUT PUSH-PULL (8 LED) */
    HAL_GPIO_WritePin(LED_PORT, LED_ALL, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_ALL;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   /* Default LOW */
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* PB0,PB1,PB3,PB4: INPUT PULLUP (4 Buttons) */
    uint16_t btns = BTN1_PIN | BTN2_PIN | BTN3_PIN | BTN4_PIN;
    GPIO_InitStruct.Pin   = btns;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
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
