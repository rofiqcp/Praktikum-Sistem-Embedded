/**
 * @file    main.c
 * @brief   P03 — Sentinel Gate: Tombol Pull-UP Eksternal (220Ω ke 3.3V)
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Up Resistor EKSTERNAL
 * ================================================================
 *
 * ESP32 GPIO34 adalah pin INPUT ONLY (tidak bisa output, tidak ada
 * internal pull-up/down). Ini SEMPURNA untuk demonstrasi pull-up
 * EKSTERNAL karena kita TERPAKSA menggunakan resistor luar!
 *
 * RANGKAIAN Pull-Up External:
 *   3.3V ─[220Ω]─┬─ GPIO34 (INPUT, no internal pull)
 *                 │
 *              [BTN]
 *                 │
 *                GND
 *
 *   Tombol TIDAK ditekan : GPIO34 = HIGH (3.3V via 220Ω)
 *   Tombol DITEKAN       : GPIO34 = LOW  (0V, short ke GND)
 *   → Active-LOW behavior
 *
 * LED: GPIO4 ─[220Ω]─ LED+ LED- ─ GND
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
/* GPIO34: INPUT ONLY, tidak ada internal pull-up!
 * → Wajib pakai pull-up eksternal 220Ω ke 3.3V */
#define BTN_PIN   GPIO_NUM_34

/* LED Output */
#define LED_PIN   GPIO_NUM_4

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static int led_state  = 0;
static int last_btn   = 1;  /* Default HIGH (pull-up menjaga HIGH saat lepas) */

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* GPIO4: OUTPUT Push-Pull (LED) */
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);
    gpio_set_level(LED_PIN, 0);

    /* GPIO34: INPUT ONLY — tidak ada pull apapun (tidak bisa!)
     * Perlu pull-up eksternal 220Ω ke 3.3V di rangkaian.
     *
     * CATATAN: gpio_set_pull_mode(GPIO_NUM_34, ...) akan ERROR
     * karena GPIO34-39 hanyalan INPUT ONLY tanpa resistor internal. */
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,    /* Tidak bisa! INPUT ONLY */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  /* Tidak bisa! INPUT ONLY */
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
        /* Baca GPIO34: Active-Low pull-up
         * 0 = BTN DITEKAN (short ke GND)
         * 1 = BTN TIDAK DITEKAN (HIGH via eksternal 220Ω) */
        int btn = gpio_get_level(BTN_PIN);

        /* Toggle LED pada falling edge (HIGH → LOW = tekan baru) */
        if ((btn == 0) && (last_btn == 1))
        {
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state);
            vTaskDelay(pdMS_TO_TICKS(50));   /* debounce */
        }

        last_btn = btn;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
