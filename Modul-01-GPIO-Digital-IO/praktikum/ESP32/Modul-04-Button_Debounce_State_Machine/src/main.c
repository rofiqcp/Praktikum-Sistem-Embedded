/**
 * @file main.c
 * @brief Program 04: Button Debounce menggunakan State Machine (ESP-IDF)
 *
 * Deskripsi:
 * Implementasi debounce pushbutton menggunakan state machine.
 * Menghindari pembacaan ganda akibat bouncing mekanis switch.
 *
 * Hardware:
 * - ESP32 DevKitC / S2 / S3
 * - Pushbutton (atau gunakan BOOT button di GPIO0)
 * - LED indicator
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BTN_DEBOUNCE";

/* ==================== KONFIGURASI ==================== */
#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO     2
#endif

#define BUTTON_PIN          0       /* GPIO0 - Boot button on DevKitC */
#define DEBOUNCE_MS         50
#define LONG_PRESS_MS       1000

/* ==================== DEBOUNCE STATE MACHINE ==================== */
typedef enum {
    BTN_IDLE,
    BTN_DEBOUNCE,
    BTN_PRESSED,
    BTN_RELEASED
} button_state_t;

/* ==================== VARIABEL GLOBAL ==================== */
static button_state_t button_state = BTN_IDLE;
static bool last_button_read = true;    /* Pull-up: HIGH = not pressed */
static bool current_button_read = true;
static int64_t debounce_start_time = 0;
static int64_t press_start_time = 0;
static uint32_t press_count = 0;
static bool led_state = false;

/* ==================== FUNCTIONS ==================== */

static int64_t millis_now(void)
{
    return esp_timer_get_time() / 1000;
}

static void handle_button_press(void)
{
    press_count++;
    led_state = !led_state;
    gpio_set_level(CONFIG_LED_GPIO, led_state);

    ESP_LOGI(TAG, "[%lld ms] Button PRESSED #%lu - LED %s",
             millis_now(), (unsigned long)press_count,
             led_state ? "ON" : "OFF");
}

static void handle_button_release(void)
{
    int64_t press_duration = millis_now() - press_start_time;

    if (press_duration >= LONG_PRESS_MS) {
        ESP_LOGI(TAG, "[%lld ms] Button RELEASED - Duration: %lld ms (LONG PRESS)",
                 millis_now(), press_duration);
    } else {
        ESP_LOGI(TAG, "[%lld ms] Button RELEASED - Duration: %lld ms (short press)",
                 millis_now(), press_duration);
    }
}

static void update_button_state(void)
{
    current_button_read = gpio_get_level(BUTTON_PIN);
    int64_t now = millis_now();

    switch (button_state) {
        case BTN_IDLE:
            if (current_button_read == 0 && last_button_read == 1) {
                button_state = BTN_DEBOUNCE;
                debounce_start_time = now;
            }
            break;

        case BTN_DEBOUNCE:
            if (now - debounce_start_time >= DEBOUNCE_MS) {
                if (current_button_read == 0) {
                    button_state = BTN_PRESSED;
                    press_start_time = now;
                    handle_button_press();
                } else {
                    button_state = BTN_IDLE;
                }
            }
            break;

        case BTN_PRESSED:
            if (current_button_read == 1) {
                button_state = BTN_RELEASED;
                debounce_start_time = now;
            }
            break;

        case BTN_RELEASED:
            if (now - debounce_start_time >= DEBOUNCE_MS) {
                if (current_button_read == 1) {
                    handle_button_release();
                    button_state = BTN_IDLE;
                } else {
                    button_state = BTN_PRESSED;
                }
            }
            break;
    }

    last_button_read = current_button_read;
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 04: Button Debounce");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi Button pin (input + pull-up) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    /* Konfigurasi LED pin (output) */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << CONFIG_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(CONFIG_LED_GPIO, 0);

    ESP_LOGI(TAG, "Button Pin: GPIO%d (aktif LOW)", BUTTON_PIN);
    ESP_LOGI(TAG, "LED Pin: GPIO%d", CONFIG_LED_GPIO);
    ESP_LOGI(TAG, "Debounce Time: %d ms", DEBOUNCE_MS);
    ESP_LOGI(TAG, "Tekan button untuk toggle LED...");

    while (1) {
        update_button_state();
        vTaskDelay(pdMS_TO_TICKS(1));  /* 1ms polling */
    }
}
