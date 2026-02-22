/**
 * @file    main.c
 * @brief   P01 — LED Parade: Output Push-Pull & Pola Cahaya Digital
 *
 * ================================================================
 * KONSEP: GPIO OUTPUT PUSH-PULL (Active-HIGH)
 * ================================================================
 *
 * ESP32 GPIO_MODE_OUTPUT = Push-Pull:
 *   gpio_set_level(pin, 1) → pin = 3.3V → arus → LED MENYALA
 *   gpio_set_level(pin, 0) → pin = 0V   → LED MATI
 *
 * ESP32 Built-in LED (GPIO2) = Active-HIGH
 *   (berbeda dengan STM32 Blue Pill PC13 yang Active-LOW!)
 *
 * DEMO (4 Pola Otomatis):
 *   Pola 1: Semua 8 LED ON
 *   Pola 2: Semua 8 LED OFF
 *   Pola 3: Running Light GPIO4 → GPIO18 → kembali
 *   Pola 4: Binary Counter 0–255
 *
 * ================================================================
 * RANGKAIAN (8 LED Active-HIGH):
 *   GPIO4  ─[220Ω]─ LED+ LED- ─ GND    (LED 1)
 *   GPIO5  ─[220Ω]─ LED+ LED- ─ GND    (LED 2)
 *   GPIO13 ─[220Ω]─ LED+ LED- ─ GND    (LED 3)
 *   GPIO14 ─[220Ω]─ LED+ LED- ─ GND    (LED 4)
 *   GPIO15 ─[220Ω]─ LED+ LED- ─ GND    (LED 5)
 *   GPIO16 ─[220Ω]─ LED+ LED- ─ GND    (LED 6)
 *   GPIO17 ─[220Ω]─ LED+ LED- ─ GND    (LED 7)
 *   GPIO18 ─[220Ω]─ LED+ LED- ─ GND    (LED 8)
 *   GPIO2  = LED Built-in DevKit (Active-HIGH)
 *
 * PLATFORM : ESP32 DevKit V1
 * FRAMEWORK: ESP-IDF (FreeRTOS)
 * CLOCK    : 240 MHz Xtensa LX6 Dual-Core
 * ================================================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define LED_OB     GPIO_NUM_2     /* LED Built-in Active-HIGH */

#define NUM_LEDS   8
static const gpio_num_t LED[NUM_LEDS] = {
    GPIO_NUM_4,  GPIO_NUM_5,  GPIO_NUM_13, GPIO_NUM_14,
    GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18
};

/* ================================================================
 *  TIMING (ms)
 * ================================================================ */
#define T_ALL_ON    800
#define T_ALL_OFF   400
#define T_RUNNING   100
#define T_BINARY     50
#define T_PAUSE     600

/* ================================================================
 *  HELPER FUNCTIONS
 * ================================================================ */
static void all_leds(int level)
{
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], level);
    gpio_set_level(LED_OB, level);
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* Build bit mask for all 8 external LEDs + built-in */
    uint64_t mask = (1ULL << GPIO_NUM_2);
    for (int i = 0; i < NUM_LEDS; i++)
        mask |= (1ULL << LED[i]);

    gpio_config_t out_conf = {
        .pin_bit_mask = mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

    all_leds(0);   /* Semua mati di awal */
}

/* ================================================================
 *  MAIN APPLICATION (FreeRTOS entry point)
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();

    while (1)
    {
        /* ======================================================
         *  POLA 1: Semua LED ON
         *  gpio_set_level(pin, 1) → pin = 3.3V → LED menyala
         * ====================================================== */
        all_leds(1);
        vTaskDelay(pdMS_TO_TICKS(T_ALL_ON));

        /* ======================================================
         *  POLA 2: Semua LED OFF
         * ====================================================== */
        all_leds(0);
        vTaskDelay(pdMS_TO_TICKS(T_ALL_OFF));

        /* ======================================================
         *  POLA 3: Running Light
         *  Satu LED menyala bergeser dari LED[0] ke LED[7]
         * ====================================================== */
        for (int i = 0; i < NUM_LEDS; i++)
        {
            all_leds(0);
            gpio_set_level(LED[i], 1);
            vTaskDelay(pdMS_TO_TICKS(T_RUNNING));
        }
        all_leds(0);
        vTaskDelay(pdMS_TO_TICKS(T_PAUSE));

        /* ======================================================
         *  POLA 4: Binary Counter 0–255
         *  LED[0] = bit0 (LSB), LED[7] = bit7 (MSB)
         *  Contoh: count=5 = 0b00000101 → LED[0]=ON, LED[2]=ON
         *
         *  Berbeda dari STM32 yang pakai GPIOA->ODR langsung,
         *  ESP32 tidak punya port-wide register di GPIO driver.
         *  Gunakan loop set_level per pin (still <1µs per pin).
         * ====================================================== */
        for (int cnt = 0; cnt <= 255; cnt++)
        {
            for (int i = 0; i < NUM_LEDS; i++)
                gpio_set_level(LED[i], (cnt >> i) & 1);
            vTaskDelay(pdMS_TO_TICKS(T_BINARY));
        }
        all_leds(0);
        vTaskDelay(pdMS_TO_TICKS(T_PAUSE));
    }
}
