/**
 * ==========================================================================
 *  ESP32_12 - LED Manufacturing Test Pattern
 * ==========================================================================
 *  Runs a comprehensive set of LED test patterns used in manufacturing QC
 *  to verify that all LEDs and their GPIO connections are working.
 *
 *  Test Patterns (in order):
 *    1. ALL ON     – verify all LEDs light up
 *    2. ALL OFF    – verify all LEDs turn off
 *    3. Walking 1  – one LED at a time, left to right
 *    4. Walking 0  – all ON except one, sweep left to right
 *    5. Binary     – count 0–255, display on 8 LEDs
 *    6. Alternating – odd/even pattern toggle
 *
 *  Hardware:
 *    - 8x LED with 220Ω series resistors
 *
 *  Wiring:
 *    GPIO2  ──►|── 220Ω ── GND   (LED0 / LSB)
 *    GPIO4  ──►|── 220Ω ── GND   (LED1)
 *    GPIO16 ──►|── 220Ω ── GND   (LED2)
 *    GPIO17 ──►|── 220Ω ── GND   (LED3)
 *    GPIO18 ──►|── 220Ω ── GND   (LED4)
 *    GPIO19 ──►|── 220Ω ── GND   (LED5)
 *    GPIO21 ──►|── 220Ω ── GND   (LED6)
 *    GPIO22 ──►|── 220Ω ── GND   (LED7 / MSB)
 *
 *  API Used:
 *    - gpio_config(), gpio_set_level()
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "TEST_PAT";

/* Array of LED pins in bit order (index 0 = LSB) */
static const gpio_num_t led_pins[NUM_LEDS] = {
    LED0_PIN, LED1_PIN, LED2_PIN, LED3_PIN,
    LED4_PIN, LED5_PIN, LED6_PIN, LED7_PIN
};

/* ------------------------------------------------------------------ */
/*  Helper: set all LEDs to a byte pattern                             */
/* ------------------------------------------------------------------ */
static void set_leds_byte(uint8_t pattern)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(led_pins[i], (pattern >> i) & 1);
    }
}

/* ------------------------------------------------------------------ */
/*  Helper: log current LED states as a visual bar                     */
/* ------------------------------------------------------------------ */
static void log_led_bar(const char *label, uint8_t pattern)
{
    char bar[NUM_LEDS * 3 + 1];
    int pos = 0;
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
        bar[pos++] = (pattern & (1 << i)) ? '*' : '.';
        bar[pos++] = ' ';
    }
    bar[pos - 1] = '\0';

    ESP_LOGI(TAG, "  %-20s [%s]  (0x%02X)", label, bar, pattern);
}

/* ------------------------------------------------------------------ */
/*  GPIO initialisation                                                */
/* ------------------------------------------------------------------ */
static void gpio_init_leds(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << led_pins[i]),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(led_pins[i], 0);
        ESP_LOGI(TAG, "  LED%d → GPIO%d", i, led_pins[i]);
    }
}

/* ================================================================== */
/*  Test Pattern 1: ALL ON                                             */
/* ================================================================== */
static void test_all_on(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━ Pattern 1: ALL ON ━━━━");
    ESP_LOGI(TAG, "  Verify: all 8 LEDs should be lit");

    set_leds_byte(0xFF);
    log_led_bar("All ON", 0xFF);

    vTaskDelay(pdMS_TO_TICKS(ALL_ON_HOLD_MS));
}

/* ================================================================== */
/*  Test Pattern 2: ALL OFF                                            */
/* ================================================================== */
static void test_all_off(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━ Pattern 2: ALL OFF ━━━━");
    ESP_LOGI(TAG, "  Verify: all 8 LEDs should be dark");

    set_leds_byte(0x00);
    log_led_bar("All OFF", 0x00);

    vTaskDelay(pdMS_TO_TICKS(ALL_OFF_HOLD_MS));
}

/* ================================================================== */
/*  Test Pattern 3: Walking 1 (one LED at a time, L→R)                 */
/* ================================================================== */
static void test_walking_one(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━ Pattern 3: Walking 1 (L→R) ━━━━");
    ESP_LOGI(TAG, "  One LED lights up at a time, sweeping left to right");

    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t pattern = (1 << i);
        set_leds_byte(pattern);

        char label[32];
        snprintf(label, sizeof(label), "Walk1 step %d", i);
        log_led_bar(label, pattern);

        vTaskDelay(pdMS_TO_TICKS(WALK_STEP_MS));
    }

    /* Reverse direction */
    for (int i = NUM_LEDS - 2; i >= 0; i--) {
        uint8_t pattern = (1 << i);
        set_leds_byte(pattern);
        vTaskDelay(pdMS_TO_TICKS(WALK_STEP_MS));
    }

    set_leds_byte(0x00);
}

/* ================================================================== */
/*  Test Pattern 4: Walking 0 (all ON except one, sweep)               */
/* ================================================================== */
static void test_walking_zero(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━ Pattern 4: Walking 0 (all ON - 1) ━━━━");
    ESP_LOGI(TAG, "  All LEDs ON except one, sweeping position");

    for (int i = 0; i < NUM_LEDS; i++) {
        uint8_t pattern = (uint8_t)(~(1 << i));
        set_leds_byte(pattern);

        char label[32];
        snprintf(label, sizeof(label), "Walk0 step %d", i);
        log_led_bar(label, pattern);

        vTaskDelay(pdMS_TO_TICKS(WALK_STEP_MS));
    }

    set_leds_byte(0x00);
}

/* ================================================================== */
/*  Test Pattern 5: Binary Count 0–255                                 */
/* ================================================================== */
static void test_binary_count(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━ Pattern 5: Binary Count 0–255 ━━━━");
    ESP_LOGI(TAG, "  Counting in binary across 8 LEDs (256 steps)");

    for (int val = 0; val <= 255; val++) {
        set_leds_byte((uint8_t)val);

        /* Log every 16th value to avoid flooding */
        if (val % 32 == 0) {
            char label[32];
            snprintf(label, sizeof(label), "Count %3d", val);
            log_led_bar(label, (uint8_t)val);
        }

        vTaskDelay(pdMS_TO_TICKS(BINARY_STEP_MS));
    }

    ESP_LOGI(TAG, "  Binary count complete (0–255)");
    set_leds_byte(0x00);
}

/* ================================================================== */
/*  Test Pattern 6: Alternating (odd / even)                           */
/* ================================================================== */
static void test_alternating(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━ Pattern 6: Alternating (odd/even) ━━━━");

    for (int rep = 0; rep < 6; rep++) {
        /* Pattern A: 0xAA = 10101010 (even positions) */
        set_leds_byte(0xAA);
        log_led_bar("Even bits (0xAA)", 0xAA);
        vTaskDelay(pdMS_TO_TICKS(ALT_HOLD_MS));

        /* Pattern B: 0x55 = 01010101 (odd positions) */
        set_leds_byte(0x55);
        log_led_bar("Odd bits  (0x55)", 0x55);
        vTaskDelay(pdMS_TO_TICKS(ALT_HOLD_MS));
    }

    set_leds_byte(0x00);
}

/* ================================================================== */
void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " ESP32_12 - LED Manufacturing Test Pattern");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "8 LEDs on GPIO: 2, 4, 16, 17, 18, 19, 21, 22");
    ESP_LOGI(TAG, "Legend: * = ON,  . = OFF   (MSB ← → LSB)");
    ESP_LOGI(TAG, "");

    gpio_init_leds();

    int cycle = 0;
    while (1) {
        cycle++;
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║       Test Cycle #%-4d                   ║", cycle);
        ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");

        /* Run all test patterns in sequence */
        test_all_on();
        vTaskDelay(pdMS_TO_TICKS(PATTERN_PAUSE_MS));

        test_all_off();
        vTaskDelay(pdMS_TO_TICKS(PATTERN_PAUSE_MS));

        test_walking_one();
        vTaskDelay(pdMS_TO_TICKS(PATTERN_PAUSE_MS));

        test_walking_zero();
        vTaskDelay(pdMS_TO_TICKS(PATTERN_PAUSE_MS));

        test_binary_count();
        vTaskDelay(pdMS_TO_TICKS(PATTERN_PAUSE_MS));

        test_alternating();
        vTaskDelay(pdMS_TO_TICKS(PATTERN_PAUSE_MS));

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "══════ All patterns complete. Repeating … ══════");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
