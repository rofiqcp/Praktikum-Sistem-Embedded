/**
 * @file    main.c
 * @brief   P08 — Speed Racer: Kecepatan GPIO & Pola LED Multi-Kecepatan
 *
 * ================================================================
 * KONSEP: GPIO DRIVE CAPABILITY (ESP32 equivalent of Slew Rate)
 * ================================================================
 *
 * ESP32 tidak memiliki "slew rate" seperti STM32. Sebagai gantinya,
 * ESP32 memiliki GPIO DRIVE CAPABILITY — berapa mA arus yang bisa
 * dikeluarkan/ditarik oleh pin:
 *
 *   GPIO_DRIVE_CAP_0 → ~5mA   (rendah)
 *   GPIO_DRIVE_CAP_1 → ~10mA  (sedang rendah)
 *   GPIO_DRIVE_CAP_2 → ~20mA  (default, sedang)
 *   GPIO_DRIVE_CAP_3 → ~40mA  (maksimum)
 *
 * Drive capability mempengaruhi:
 * - Seberapa cepat pin bisa mengisi/mengosongkan kapasitansi beban
 * - Berkaitan dengan edge speed sinyal (analog dengan slew rate)
 * - Semakin tinggi → tepi lebih tajam, EMI lebih besar
 *
 * DEMO (4 Button memilih pola + drive capability):
 *   BTN1 (GPIO25) → Running Light   + DRIVE_CAP_0 (rendah)
 *   BTN2 (GPIO26) → Knight Rider    + DRIVE_CAP_2 (default)
 *   BTN3 (GPIO27) → Alternating     + DRIVE_CAP_3 (maximum)
 *   BTN4 (GPIO32) → Binary Counter  + DRIVE_CAP_2
 *
 * ================================================================
 * RANGKAIAN:
 *   GPIO25,26,27,32 ─[BTNx]─ GND   (internal pull-up)
 *   GPIO4,5,13,14,15,16,17,18 ─[220Ω]─ LEDx ─ GND
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
#define NUM_LEDS 8
static const gpio_num_t LED[NUM_LEDS] = {
    GPIO_NUM_4,  GPIO_NUM_5,  GPIO_NUM_13, GPIO_NUM_14,
    GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18
};

#define BTN1_PIN   GPIO_NUM_25
#define BTN2_PIN   GPIO_NUM_26
#define BTN3_PIN   GPIO_NUM_27
#define BTN4_PIN   GPIO_NUM_32

/* ================================================================
 *  HELPERS
 * ================================================================ */
static void all_leds_off(void)
{
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], 0);
}

static void set_drive_cap(gpio_drive_cap_t cap)
{
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_drive_capability(LED[i], cap);
}

static int btn_active(gpio_num_t pin)
{
    return gpio_get_level(pin) == 0;   /* Active-LOW */
}

/* ================================================================
 *  GPIO INITIALIZATION
 * ================================================================ */
static void gpio_init_all(void)
{
    /* 8 LEDs OUTPUT */
    uint64_t led_mask = 0;
    for (int i = 0; i < NUM_LEDS; i++)
        led_mask |= (1ULL << LED[i]);

    gpio_config_t out_conf = {
        .pin_bit_mask = led_mask,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);
    all_leds_off();

    /* 4 Buttons INPUT + PULLUP */
    uint64_t btn_mask = (1ULL<<BTN1_PIN)|(1ULL<<BTN2_PIN)|
                        (1ULL<<BTN3_PIN)|(1ULL<<BTN4_PIN);
    gpio_config_t in_conf = {
        .pin_bit_mask = btn_mask,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);
}

/* ================================================================
 *  LED PATTERNS
 * ================================================================ */
static void pattern_running(uint32_t delay_ms)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (!btn_active(BTN1_PIN)) return;
        all_leds_off();
        gpio_set_level(LED[i], 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static void pattern_knight_rider(uint32_t delay_ms)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (!btn_active(BTN2_PIN)) return;
        all_leds_off();
        gpio_set_level(LED[i], 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    for (int i = NUM_LEDS - 2; i >= 1; i--)
    {
        if (!btn_active(BTN2_PIN)) return;
        all_leds_off();
        gpio_set_level(LED[i], 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static void pattern_alternating(uint32_t delay_ms)
{
    if (!btn_active(BTN3_PIN)) return;
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], i % 2 == 0 ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    if (!btn_active(BTN3_PIN)) return;
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], i % 2 == 1 ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

static void pattern_binary_counter(uint32_t delay_ms)
{
    static int cnt = 0;
    if (!btn_active(BTN4_PIN)) { cnt = 0; return; }
    for (int i = 0; i < NUM_LEDS; i++)
        gpio_set_level(LED[i], (cnt >> i) & 1);
    cnt = (cnt >= 255) ? 0 : cnt + 1;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

/* ================================================================
 *  MAIN APPLICATION
 * ================================================================ */
void app_main(void)
{
    gpio_init_all();

    while (1)
    {
        if (btn_active(BTN1_PIN))
        {
            /* DRIVE_CAP_0: rendah ~5mA — edge paling lambat */
            set_drive_cap(GPIO_DRIVE_CAP_0);
            pattern_running(120);
        }
        else if (btn_active(BTN2_PIN))
        {
            /* DRIVE_CAP_2: default ~20mA */
            set_drive_cap(GPIO_DRIVE_CAP_2);
            pattern_knight_rider(80);
        }
        else if (btn_active(BTN3_PIN))
        {
            /* DRIVE_CAP_3: maksimum ~40mA — edge tercepat */
            set_drive_cap(GPIO_DRIVE_CAP_3);
            pattern_alternating(300);
        }
        else if (btn_active(BTN4_PIN))
        {
            /* DRIVE_CAP_2: default */
            set_drive_cap(GPIO_DRIVE_CAP_2);
            pattern_binary_counter(40);
        }
        else
        {
            all_leds_off();
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}
