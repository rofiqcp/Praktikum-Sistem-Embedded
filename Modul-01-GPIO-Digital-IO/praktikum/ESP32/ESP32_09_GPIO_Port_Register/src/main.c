/**
 * ==========================================================================
 *  ESP32_09 - GPIO Port Register (Direct Register-Level Access)
 * ==========================================================================
 *  Demonstrates direct register-level GPIO read/write on ESP32.
 *  This is the ESP32 equivalent of STM32's GPIO BSRR register!
 *
 *  ESP32 GPIO Output Registers:
 *  ┌─────────────────────┬───────────────────────────────────────┐
 *  │ Register            │ Function                              │
 *  ├─────────────────────┼───────────────────────────────────────┤
 *  │ GPIO_OUT_REG        │ Read/write current output state       │
 *  │ GPIO_OUT_W1TS_REG   │ Write-1-to-set (set bits to HIGH)    │
 *  │ GPIO_OUT_W1TC_REG   │ Write-1-to-clear (set bits to LOW)   │
 *  │ GPIO_IN_REG         │ Read input level of GPIO 0-31        │
 *  └─────────────────────┴───────────────────────────────────────┘
 *
 *  W1TS = Write 1 To Set   (like STM32 BSRR lower 16 bits)
 *  W1TC = Write 1 To Clear (like STM32 BSRR upper 16 bits)
 *
 *  Hardware:
 *    - 4x LED on GPIO16, GPIO17, GPIO18, GPIO19
 *    - 4x 220Ω series resistors
 *
 *  Wiring:
 *    GPIO16 ──►|── 220Ω ── GND   (LED0)
 *    GPIO17 ──►|── 220Ω ── GND   (LED1)
 *    GPIO18 ──►|── 220Ω ── GND   (LED2)
 *    GPIO19 ──►|── 220Ω ── GND   (LED3)
 *
 *  API Used:
 *    - gpio_config()            (initial pin setup)
 *    - REG_WRITE()              (direct register write)
 *    - REG_READ()               (direct register read)
 *    - gpio_set_level()         (HAL-level, for speed comparison)
 *    - esp_timer_get_time()     (microsecond benchmarking)
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "PORT_REG";

static const gpio_num_t led_pins[NUM_LEDS] = {
    LED0_PIN, LED1_PIN, LED2_PIN, LED3_PIN
};

static const uint32_t led_masks[NUM_LEDS] = {
    LED0_MASK, LED1_MASK, LED2_MASK, LED3_MASK
};

/* ------------------------------------------------------------------ */
/*  Initialise GPIOs using the normal driver (needed once)             */
/* ------------------------------------------------------------------ */
static void gpio_init_leds(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = ALL_LEDS_MASK,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "GPIOs 16-19 configured as output");
}

/* ------------------------------------------------------------------ */
/*  Helper: read and display current GPIO output register              */
/* ------------------------------------------------------------------ */
static void log_gpio_out_state(const char *label)
{
    uint32_t reg_val = REG_READ(GPIO_OUT_REG);

    char bits[NUM_LEDS + 1];
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
        bits[NUM_LEDS - 1 - i] = (reg_val & led_masks[i]) ? '1' : '0';
    }
    bits[NUM_LEDS] = '\0';

    ESP_LOGI(TAG, "  %-25s → GPIO_OUT = 0x%08lX  LEDs[19..16] = %s",
             label, (unsigned long)reg_val, bits);
}

/* ------------------------------------------------------------------ */
/*  Demo 1: Set / Clear using W1TS and W1TC registers                  */
/* ------------------------------------------------------------------ */
static void demo_set_clear(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "──── Demo 1: W1TS / W1TC register access ────");

    /* All LEDs OFF */
    REG_WRITE(GPIO_OUT_W1TC_REG, ALL_LEDS_MASK);
    log_gpio_out_state("All OFF (W1TC)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));

    /* Turn on LED0 only */
    REG_WRITE(GPIO_OUT_W1TS_REG, LED0_MASK);
    log_gpio_out_state("LED0 ON (W1TS)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));

    /* Turn on LED1 without affecting LED0 */
    REG_WRITE(GPIO_OUT_W1TS_REG, LED1_MASK);
    log_gpio_out_state("LED1 ON (W1TS)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));

    /* Turn on LED2 and LED3 simultaneously */
    REG_WRITE(GPIO_OUT_W1TS_REG, LED2_MASK | LED3_MASK);
    log_gpio_out_state("LED2+3 ON (W1TS)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));

    /* Turn off LED1 and LED3, keep LED0 and LED2 */
    REG_WRITE(GPIO_OUT_W1TC_REG, LED1_MASK | LED3_MASK);
    log_gpio_out_state("LED1+3 OFF (W1TC)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));

    /* All LEDs ON */
    REG_WRITE(GPIO_OUT_W1TS_REG, ALL_LEDS_MASK);
    log_gpio_out_state("All ON (W1TS)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));

    /* All LEDs OFF */
    REG_WRITE(GPIO_OUT_W1TC_REG, ALL_LEDS_MASK);
    log_gpio_out_state("All OFF (W1TC)");
    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));
}

/* ------------------------------------------------------------------ */
/*  Demo 2: Walking-1 using register writes                            */
/* ------------------------------------------------------------------ */
static void demo_walking_register(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "──── Demo 2: Walking-1 via register writes ────");

    for (int i = 0; i < NUM_LEDS; i++) {
        /* All off first */
        REG_WRITE(GPIO_OUT_W1TC_REG, ALL_LEDS_MASK);
        /* Turn on only LED[i] */
        REG_WRITE(GPIO_OUT_W1TS_REG, led_masks[i]);

        char label[32];
        snprintf(label, sizeof(label), "LED%d walking", i);
        log_gpio_out_state(label);

        vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));
    }

    REG_WRITE(GPIO_OUT_W1TC_REG, ALL_LEDS_MASK);
}

/* ------------------------------------------------------------------ */
/*  Demo 3: Read GPIO_OUT_REG to determine current state               */
/* ------------------------------------------------------------------ */
static void demo_read_register(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "──── Demo 3: Read GPIO_OUT_REG ────");

    /* Set a known pattern: LED0=ON, LED1=OFF, LED2=ON, LED3=OFF → 0101 */
    REG_WRITE(GPIO_OUT_W1TC_REG, ALL_LEDS_MASK);
    REG_WRITE(GPIO_OUT_W1TS_REG, LED0_MASK | LED2_MASK);

    uint32_t reg_val = REG_READ(GPIO_OUT_REG);

    ESP_LOGI(TAG, "  Set pattern 0101 (LED0+LED2 ON)");
    ESP_LOGI(TAG, "  GPIO_OUT_REG = 0x%08lX", (unsigned long)reg_val);

    for (int i = 0; i < NUM_LEDS; i++) {
        bool on = (reg_val & led_masks[i]) != 0;
        ESP_LOGI(TAG, "    LED%d (GPIO%d) = %s", i, led_pins[i], on ? "ON" : "OFF");
    }

    vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS * 2));
    REG_WRITE(GPIO_OUT_W1TC_REG, ALL_LEDS_MASK);
}

/* ------------------------------------------------------------------ */
/*  Demo 4: Speed comparison – register vs gpio_set_level()            */
/* ------------------------------------------------------------------ */
static void demo_speed_comparison(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "──── Demo 4: Speed comparison ────");
    ESP_LOGI(TAG, "  Toggling LED0 %d times each method …", SPEED_TEST_CYCLES);

    /* --- Method A: gpio_set_level() (HAL) --- */
    int64_t t_start = esp_timer_get_time();
    for (int i = 0; i < SPEED_TEST_CYCLES; i++) {
        gpio_set_level(LED0_PIN, 1);
        gpio_set_level(LED0_PIN, 0);
    }
    int64_t t_hal = esp_timer_get_time() - t_start;

    /* --- Method B: Direct register W1TS / W1TC --- */
    t_start = esp_timer_get_time();
    for (int i = 0; i < SPEED_TEST_CYCLES; i++) {
        REG_WRITE(GPIO_OUT_W1TS_REG, LED0_MASK);
        REG_WRITE(GPIO_OUT_W1TC_REG, LED0_MASK);
    }
    int64_t t_reg = esp_timer_get_time() - t_start;

    ESP_LOGI(TAG, "  ┌──────────────────────────────────────┐");
    ESP_LOGI(TAG, "  │  gpio_set_level(): %7lld µs        │", (long long)t_hal);
    ESP_LOGI(TAG, "  │  REG_WRITE()     : %7lld µs        │", (long long)t_reg);
    if (t_reg > 0) {
        ESP_LOGI(TAG, "  │  Speedup         : %.1fx faster     │",
                 (double)t_hal / (double)t_reg);
    }
    ESP_LOGI(TAG, "  └──────────────────────────────────────┘");
}

/* ================================================================== */
void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " ESP32_09 - GPIO Port Register Access");
    ESP_LOGI(TAG, " (ESP32 equivalent of STM32 BSRR)");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "LED pins: GPIO%d, GPIO%d, GPIO%d, GPIO%d",
             LED0_PIN, LED1_PIN, LED2_PIN, LED3_PIN);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Register map:");
    ESP_LOGI(TAG, "  GPIO_OUT_REG      = 0x%08X", GPIO_OUT_REG);
    ESP_LOGI(TAG, "  GPIO_OUT_W1TS_REG = 0x%08X", GPIO_OUT_W1TS_REG);
    ESP_LOGI(TAG, "  GPIO_OUT_W1TC_REG = 0x%08X", GPIO_OUT_W1TC_REG);
    ESP_LOGI(TAG, "  GPIO_IN_REG       = 0x%08X", GPIO_IN_REG);
    ESP_LOGI(TAG, "");

    gpio_init_leds();

    while (1) {
        demo_set_clear();
        demo_walking_register();
        demo_read_register();
        demo_speed_comparison();

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "══════ Restarting demos in 3 s ══════");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
