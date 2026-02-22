/**
 * @file    main.c
 * @brief   P05 — Phantom Touch: Pull-UP Internal & Tombol Tanpa Resistor
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Up INTERNAL (~45kΩ)
 * ================================================================
 *
 * ESP32 memiliki pull-up internal ~45kΩ (sedikit berbeda dari
 * STM32 ~40kΩ, namun prinsip identik).
 *
 * Aktifkan via: GPIO_PULLUP_ENABLE dalam gpio_config_t
 * → Pin default = HIGH (3.3V via ~45kΩ internal)
 * → Saat tombol ditekan ke GND → pin = LOW
 * → Active-LOW behavior tanpa resistor eksternal!
 *
 * RANGKAIAN Pull-Up INTERNAL (minimalis!):
 *   VCC ─[~45kΩ internal]─ GPIO25 ─[BTN]─ GND
 *   Tidak perlu resistor eksternal apapun!
 *
 *   Tombol TIDAK ditekan : GPIO25 = HIGH (via internal ~45kΩ)
 *   Tombol DITEKAN       : GPIO25 = LOW  (short ke GND)
 *
 * Perbandingan P03 vs P05:
 *   P03: GPIO34 INPUT ONLY, harus pakai ext 220Ω pull-up
 *   P05: GPIO25 INPUT+PULLUP, tidak perlu resistor sama sekali!
 *
 * LED: GPIO4 ─[220Ω]─ LED ─ GND
 *
 * PLATFORM : ESP32 DevKit V1
 * FRAMEWORK: ESP-IDF (FreeRTOS)
 * ================================================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
/* GPIO25: Regular GPIO, supports internal pull-up */
#define BTN_PIN   GPIO_NUM_25

/* LED Output */
#define LED_PIN   GPIO_NUM_4

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static int led_state = 0;
static int last_btn  = 1;  /* Pull-up internal: default HIGH */

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* GPIO4: OUTPUT (LED) */
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);
    gpio_set_level(LED_PIN, 0);

    /* GPIO25: INPUT dengan PULL-UP INTERNAL ~45kΩ
     *
     * GPIO_PULLUP_ENABLE → mengaktifkan resistor ~45kΩ dari VCC ke GPIO25
     * Hasil: GPIO25 default = HIGH tanpa resistor eksternal!
     * Saat tombol1 kaki ke GND, kaki lain ke GPIO25 dan ditekan → LOW
     */
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     /* ← internal ~45kΩ aktif */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);
}

/* ================================================================
 *  MAIN APPLICATION
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();

    while (1)
    {
        /* Baca GPIO25: Active-Low internal pull-up */
        int btn = gpio_get_level(BTN_PIN);

        /* Toggle LED pada falling edge */
        if ((btn == 0) && (last_btn == 1))
        {
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        last_btn = btn;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
