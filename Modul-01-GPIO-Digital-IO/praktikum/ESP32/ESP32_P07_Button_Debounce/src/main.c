/**
 * @file    main.c
 * @brief   P07 — Clean Contact: Debounce State Machine & Penghitung Akurat
 *
 * ================================================================
 * KONSEP: SOFTWARE DEBOUNCE dengan STATE MACHINE
 * ================================================================
 *
 * Bouncing: tombol mekanikal "memantul" 1-50ms sebelum stabil.
 * Tanpa debounce: 1 tekan fisik → bisa terbaca 5-20 press!
 *
 * SOLUSI: State Machine dengan esp_timer_get_time() (microseconds):
 *
 *   RELEASED ──(pin=LOW)──→ DEBOUNCING ──(stabil >50ms)──→ PRESSED ──(pin=HIGH)──→ RELEASED
 *                                │
 *                         (pin=HIGH sebelum 50ms → kembali RELEASED)
 *
 * 4 TOMBOL + 4 LED INDEPENDEN:
 *   BTN1 (GPIO25) ↔ LED1 (GPIO4)
 *   BTN2 (GPIO26) ↔ LED2 (GPIO5)
 *   BTN3 (GPIO27) ↔ LED3 (GPIO13)
 *   BTN4 (GPIO32) ↔ LED4 (GPIO14)
 *
 * ================================================================
 * RANGKAIAN:
 *   GPIO25 ─[BTN1]─ GND  (internal pull-up aktif, Active-LOW)
 *   GPIO26 ─[BTN2]─ GND
 *   GPIO27 ─[BTN3]─ GND
 *   GPIO32 ─[BTN4]─ GND
 *   GPIO4  ─[220Ω]─ LED1 ─ GND
 *   GPIO5  ─[220Ω]─ LED2 ─ GND
 *   GPIO13 ─[220Ω]─ LED3 ─ GND
 *   GPIO14 ─[220Ω]─ LED4 ─ GND
 *
 * PLATFORM : ESP32 DevKit V1
 * FRAMEWORK: ESP-IDF (FreeRTOS)
 * ================================================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/* ================================================================
 *  PIN DEFINITIONS
 * ================================================================ */
#define NUM_BTN  4

static const gpio_num_t BTN_PINS[NUM_BTN] = {
    GPIO_NUM_25,   /* BTN1 */
    GPIO_NUM_26,   /* BTN2 */
    GPIO_NUM_27,   /* BTN3 */
    GPIO_NUM_32    /* BTN4 */
};

static const gpio_num_t LED_PINS[NUM_BTN] = {
    GPIO_NUM_4,    /* LED1 */
    GPIO_NUM_5,    /* LED2 */
    GPIO_NUM_13,   /* LED3 */
    GPIO_NUM_14    /* LED4 */
};

/* ================================================================
 *  DEBOUNCE CONFIG
 * ================================================================ */
#define DEBOUNCE_US  50000LL   /* 50ms dalam microseconds */

/* ================================================================
 *  BUTTON DEBOUNCE STATE MACHINE
 * ================================================================ */
typedef enum {
    BTN_RELEASED,
    BTN_DEBOUNCING,
    BTN_PRESSED
} btn_state_t;

static btn_state_t btn_state[NUM_BTN]       = {BTN_RELEASED};
static int64_t     btn_debounce_us[NUM_BTN] = {0};
static int         led_state[NUM_BTN]       = {0};

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* 4 BTN pins: INPUT + PULLUP (Active-LOW) */
    uint64_t btn_mask = 0;
    for (int i = 0; i < NUM_BTN; i++)
        btn_mask |= (1ULL << BTN_PINS[i]);

    gpio_config_t in_conf = {
        .pin_bit_mask = btn_mask,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);

    /* 4 LED pins: OUTPUT */
    uint64_t led_mask = 0;
    for (int i = 0; i < NUM_BTN; i++)
        led_mask |= (1ULL << LED_PINS[i]);

    gpio_config_t out_conf = {
        .pin_bit_mask = led_mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

    for (int i = 0; i < NUM_BTN; i++)
        gpio_set_level(LED_PINS[i], 0);
}

/* ================================================================
 *  DEBOUNCE PROCESSING LOOP
 * ================================================================ */
static void button_process(void)
{
    int64_t now = esp_timer_get_time();   /* Waktu sistem dalam MICROSECONDS */

    for (int i = 0; i < NUM_BTN; i++)
    {
        int pin = gpio_get_level(BTN_PINS[i]);  /* 0=LOW=ditekan (active-low) */

        switch (btn_state[i])
        {
            case BTN_RELEASED:
                if (pin == 0)
                {
                    /* Pin baru LOW → mulai debounce timer */
                    btn_debounce_us[i] = now;
                    btn_state[i]       = BTN_DEBOUNCING;
                }
                break;

            case BTN_DEBOUNCING:
                if (pin == 1)
                {
                    /* Kembali HIGH sebelum 50ms → bounce! Reset */
                    btn_state[i] = BTN_RELEASED;
                }
                else if ((now - btn_debounce_us[i]) >= DEBOUNCE_US)
                {
                    /* Tetap LOW selama ≥50ms → VALID! Toggle LED */
                    led_state[i] = !led_state[i];
                    gpio_set_level(LED_PINS[i], led_state[i]);
                    btn_state[i] = BTN_PRESSED;
                }
                break;

            case BTN_PRESSED:
                if (pin == 1)
                {
                    /* Tombol dilepas */
                    btn_state[i] = BTN_RELEASED;
                }
                break;
        }
    }
}

/* ================================================================
 *  MAIN APPLICATION
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();

    while (1)
    {
        button_process();
        vTaskDelay(pdMS_TO_TICKS(5));   /* Polling 5ms */
    }
}
