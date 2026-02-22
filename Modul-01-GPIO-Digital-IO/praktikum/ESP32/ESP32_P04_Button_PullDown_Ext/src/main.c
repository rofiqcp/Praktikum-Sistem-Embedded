/**
 * @file    main.c
 * @brief   P04 — Ground Guardian: Tombol Pull-DOWN Eksternal (220Ω ke GND)
 *
 * ================================================================
 * KONSEP: GPIO INPUT dengan Pull-Down Resistor EKSTERNAL
 * ================================================================
 *
 * ESP32 GPIO35 adalah INPUT ONLY (tidak ada internal pull-down).
 * Ini ideal untuk demo pull-down EKSTERNAL — kita harus menyediakan
 * resistor 220Ω dari GPIO35 ke GND secara fisik.
 *
 * RANGKAIAN Pull-Down External:
 *   3.3V ─[BTN]─┬─ GPIO35 (INPUT, no internal pull)
 *                │
 *             [220Ω]
 *                │
 *               GND
 *
 *   Tombol TIDAK ditekan : GPIO35 = LOW  (0V via 220Ω ke GND)
 *   Tombol DITEKAN       : GPIO35 = HIGH (3.3V masuk, arus via 220Ω)
 *   → Active-HIGH behavior: press = HIGH
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
/* GPIO35: INPUT ONLY, tidak ada internal pull-down!
 * → Wajib pakai pull-down eksternal 220Ω ke GND */
#define BTN_PIN   GPIO_NUM_35

/* LED Output */
#define LED_PIN   GPIO_NUM_4

/* ================================================================
 *  VARIABLES
 * ================================================================ */
static int led_state = 0;
static int last_btn  = 0;  /* Default LOW (pull-down menjaga LOW saat lepas) */

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

    /* GPIO35: INPUT ONLY — tanpa pull internal
     * Perlu pull-down eksternal 220Ω ke GND di rangkaian */
    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
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
        /* Baca GPIO35: Active-High pull-down
         * 1 = BTN DITEKAN (HIGH, 3.3V masuk)
         * 0 = BTN TIDAK DITEKAN (LOW via eksternal 220Ω ke GND) */
        int btn = gpio_get_level(BTN_PIN);

        /* Toggle LED pada rising edge (LOW → HIGH = tekan baru) */
        if ((btn == 1) && (last_btn == 0))
        {
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state);
            vTaskDelay(pdMS_TO_TICKS(50));   /* debounce */
        }

        last_btn = btn;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
