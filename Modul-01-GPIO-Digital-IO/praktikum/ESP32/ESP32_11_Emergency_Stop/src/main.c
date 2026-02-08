/**
 * ==========================================================================
 *  ESP32_11 - Emergency Stop (Fail-Safe Logic)
 * ==========================================================================
 *  Implements an industrial-style Emergency Stop (E-Stop) with fail-safe
 *  wiring. The E-Stop button is Normally Closed (NC), meaning:
 *
 *    ┌──────────────────────────────────────────────────────┐
 *    │  NORMAL: NC button closed → current flows → HIGH    │
 *    │  EMERGENCY: button pressed / wire broken → LOW      │
 *    │  FAIL-SAFE: any fault defaults to EMERGENCY state   │
 *    └──────────────────────────────────────────────────────┘
 *
 *  Reset procedure (deliberate action required):
 *    1. Release the E-Stop button (GPIO goes back HIGH)
 *    2. Hold button released for ≥ 2 seconds continuously
 *    3. System resets to NORMAL state
 *
 *  Hardware:
 *    - 1x Push button (wired as NC, normally closed)
 *    - 1x 10 kΩ pull-up resistor (3.3V → GPIO4)
 *    - 1x LED on GPIO2 with 220Ω resistor
 *    - 1x Active buzzer on GPIO16 (or LED as substitute)
 *
 *  Wiring:
 *    3.3V ── 10kΩ ──┬── GPIO4 (ESTOP_PIN)
 *                    │
 *                [NC BTN] ── GND
 *
 *    GPIO2  ──►|── 220Ω ── GND   (Status LED)
 *    GPIO16 ── Buzzer(+) ── GND   (Buzzer)
 *
 *  States:
 *    NORMAL    → LED slow heartbeat, buzzer OFF
 *    EMERGENCY → LED fast blink, buzzer ON, log alarm
 *    RESETTING → LED solid ON, buzzer OFF, waiting for 2s hold
 *
 *  API Used:
 *    - gpio_config(), gpio_set_level(), gpio_get_level()
 *    - esp_timer_get_time()
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "E-STOP";

/* ---- System states ---- */
typedef enum {
    STATE_NORMAL,
    STATE_EMERGENCY,
    STATE_RESETTING,
} system_state_t;

static system_state_t   state           = STATE_NORMAL;
static int64_t          reset_start_ms  = 0;
static int64_t          last_blink_ms   = 0;
static bool             led_on          = false;
static uint32_t         emergency_count = 0;

/* ------------------------------------------------------------------ */
/*  GPIO initialisation                                                */
/* ------------------------------------------------------------------ */
static void gpio_init_all(void)
{
    /* E-Stop input – pull-up enabled (hardware pull-up also recommended) */
    gpio_config_t estop_conf = {
        .pin_bit_mask = (1ULL << ESTOP_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&estop_conf));

    /* LED output */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_STATUS_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_conf));

    /* Buzzer output */
    gpio_config_t buzz_conf = {
        .pin_bit_mask = (1ULL << BUZZER_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&buzz_conf));

    /* Initial state: LED off, buzzer off */
    gpio_set_level(LED_STATUS_PIN, 0);
    gpio_set_level(BUZZER_PIN, 0);

    ESP_LOGI(TAG, "GPIO initialised:");
    ESP_LOGI(TAG, "  E-Stop : GPIO%d (input, pull-up, NC button)", ESTOP_PIN);
    ESP_LOGI(TAG, "  LED    : GPIO%d (output)", LED_STATUS_PIN);
    ESP_LOGI(TAG, "  Buzzer : GPIO%d (output)", BUZZER_PIN);
}

/* ------------------------------------------------------------------ */
/*  State names for logging                                            */
/* ------------------------------------------------------------------ */
static const char *state_name(system_state_t s)
{
    switch (s) {
        case STATE_NORMAL:    return "NORMAL";
        case STATE_EMERGENCY: return "*** EMERGENCY ***";
        case STATE_RESETTING: return "RESETTING";
        default:              return "UNKNOWN";
    }
}

/* ------------------------------------------------------------------ */
/*  State machine                                                      */
/* ------------------------------------------------------------------ */
static void process_state_machine(void)
{
    int estop_level = gpio_get_level(ESTOP_PIN);
    int64_t now_ms  = esp_timer_get_time() / 1000;

    bool estop_triggered = (estop_level == ESTOP_ACTIVE_LEVEL);

    switch (state) {

    /* ========== NORMAL ========== */
    case STATE_NORMAL:
        /* Slow heartbeat blink */
        if ((now_ms - last_blink_ms) >= NORMAL_BLINK_MS) {
            led_on = !led_on;
            gpio_set_level(LED_STATUS_PIN, led_on ? 1 : 0);
            last_blink_ms = now_ms;
        }
        gpio_set_level(BUZZER_PIN, 0);  /* Buzzer off */

        if (estop_triggered) {
            emergency_count++;
            state = STATE_EMERGENCY;
            ESP_LOGE(TAG, "╔══════════════════════════════════════════╗");
            ESP_LOGE(TAG, "║       !!! EMERGENCY STOP #%-4lu !!!       ║",
                     (unsigned long)emergency_count);
            ESP_LOGE(TAG, "║  E-Stop activated (GPIO%d = LOW)         ║", ESTOP_PIN);
            ESP_LOGE(TAG, "║  Cause: button pressed or wire broken    ║");
            ESP_LOGE(TAG, "╚══════════════════════════════════════════╝");
        }
        break;

    /* ========== EMERGENCY ========== */
    case STATE_EMERGENCY:
        /* Fast blink LED */
        if ((now_ms - last_blink_ms) >= FAST_BLINK_MS) {
            led_on = !led_on;
            gpio_set_level(LED_STATUS_PIN, led_on ? 1 : 0);
            last_blink_ms = now_ms;
        }

        /* Buzzer ON continuously */
        gpio_set_level(BUZZER_PIN, 1);

        /* Check if E-Stop released (button back to normal) */
        if (!estop_triggered) {
            state          = STATE_RESETTING;
            reset_start_ms = now_ms;
            ESP_LOGW(TAG, "E-Stop released — hold for %d ms to reset …", RESET_HOLD_MS);
            gpio_set_level(LED_STATUS_PIN, 1);  /* LED solid ON */
            gpio_set_level(BUZZER_PIN, 0);      /* Buzzer off during reset */
        }
        break;

    /* ========== RESETTING ========== */
    case STATE_RESETTING:
        /* LED solid ON while waiting */
        gpio_set_level(LED_STATUS_PIN, 1);
        gpio_set_level(BUZZER_PIN, 0);

        if (estop_triggered) {
            /* E-Stop pressed again → back to emergency */
            state = STATE_EMERGENCY;
            ESP_LOGE(TAG, "Reset aborted! E-Stop re-triggered.");
        } else if ((now_ms - reset_start_ms) >= RESET_HOLD_MS) {
            /* Held released for long enough → reset to normal */
            state  = STATE_NORMAL;
            led_on = false;
            gpio_set_level(LED_STATUS_PIN, 0);
            ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║         System RESET to NORMAL           ║");
            ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
        } else {
            /* Still waiting */
            int64_t remaining = RESET_HOLD_MS - (now_ms - reset_start_ms);
            if (remaining % 500 < POLL_INTERVAL_MS) {
                ESP_LOGW(TAG, "  Resetting … hold %lld ms more", (long long)remaining);
            }
        }
        break;
    }
}

/* ================================================================== */
void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " ESP32_11 - Emergency Stop (Fail-Safe)");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Fail-safe design:");
    ESP_LOGI(TAG, "  Button type  : Normally Closed (NC)");
    ESP_LOGI(TAG, "  Normal state : GPIO reads HIGH (circuit closed)");
    ESP_LOGI(TAG, "  Emergency    : GPIO reads LOW  (pressed / wire break)");
    ESP_LOGI(TAG, "  Reset method : Release button + hold for %d ms", RESET_HOLD_MS);
    ESP_LOGI(TAG, "");

    gpio_init_all();

    /* Verify initial E-Stop state */
    int initial = gpio_get_level(ESTOP_PIN);
    if (initial == ESTOP_NORMAL_LEVEL) {
        ESP_LOGI(TAG, "Initial E-Stop state: NORMAL (GPIO%d = HIGH)", ESTOP_PIN);
        ESP_LOGI(TAG, "System starting in NORMAL mode");
    } else {
        ESP_LOGW(TAG, "Initial E-Stop state: TRIGGERED (GPIO%d = LOW)!", ESTOP_PIN);
        ESP_LOGW(TAG, "Check wiring! System starting in EMERGENCY mode");
        state = STATE_EMERGENCY;
        emergency_count = 1;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Running … current state: %s", state_name(state));
    ESP_LOGI(TAG, "");

    while (1) {
        process_state_machine();
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
