/**
 * @file    main.c
 * @brief   P06 — Force Field: Pull-DOWN Internal & Logika Active-HIGH
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Down INTERNAL (~45kΩ)
 * ================================================================
 *
 * ESP32 memiliki pull-down internal ~45kΩ.
 * Aktifkan via: GPIO_PULLDOWN_ENABLE dalam gpio_config_t
 * → Pin default = LOW (0V via ~45kΩ internal ke GND)
 * → Saat tombol ditekan ke 3.3V → pin = HIGH
 * → Active-HIGH behavior tanpa resistor eksternal!
 *
 * RANGKAIAN Pull-Down INTERNAL (minimalis!):
 *   3.3V ─[BTN]─ GPIO26 ─[~45kΩ internal]─ GND
 *   Tidak perlu resistor eksternal!
 *
 *   Tombol TIDAK ditekan : GPIO26 = LOW  (via internal ~45kΩ ke GND)
 *   Tombol DITEKAN       : GPIO26 = HIGH (3.3V masuk dari VCC)
 *   → Active-HIGH: press = 1
 *
 * Perbandingan P04 vs P06:
 *   P04: GPIO35 INPUT ONLY, harus pakai ext 220Ω pull-down
 *   P06: GPIO26 INPUT+PULLDOWN, tidak perlu resistor luar!
 *
 * PERINGATAN ESP32:
 *   Jangan sambungkan 5V ke GPIO! ESP32 adalah 3.3V MCU.
 *   Input maksimum = 3.6V.
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
/* GPIO26: Regular GPIO, supports internal pull-down */
#define BTN_PIN   GPIO_NUM_26

/* LED Output */
#define LED_PIN   GPIO_NUM_4

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static int led_state = 0;
static int last_btn  = 0;  /* Pull-down internal: default LOW */

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

    /* GPIO26: INPUT dengan PULL-DOWN INTERNAL ~45kΩ
     *
     * GPIO_PULLDOWN_ENABLE → mengaktifkan resistor ~45kΩ dari GPIO26 ke GND
     * Hasil: GPIO26 default = LOW tanpa resistor eksternal!
     * Saat tombol (satu kaki ke 3.3V, satu kaki ke GPIO26) ditekan → HIGH
     */
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,   /* ← internal ~45kΩ ke GND */
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
        /* Baca GPIO26: Active-High internal pull-down */
        int btn = gpio_get_level(BTN_PIN);

        /* Toggle LED pada rising edge (LOW→HIGH = tombol baru ditekan) */
        if ((btn == 1) && (last_btn == 0))
        {
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        last_btn = btn;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
