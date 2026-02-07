/**
 * @file main.c
 * @brief Program 05: Long Press vs Short Press Detection (ESP-IDF)
 *
 * Deskripsi:
 * Mendeteksi jenis penekanan button: short press, long press, very long press.
 * Masing-masing jenis penekanan memiliki aksi yang berbeda.
 *
 * Aksi:
 * - Short Press (<500ms): Toggle LED 1
 * - Long Press (1-3s): Toggle LED 2
 * - Very Long Press (>3s): Reset kedua LED
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

static const char *TAG = "PRESS_DETECT";

/* ==================== KONFIGURASI ==================== */
#define BUTTON_PIN      0
#define LED_1           2       /* LED untuk short press */
#define LED_2           4       /* LED untuk long press */

#define DEBOUNCE_MS     50
#define SHORT_PRESS_MIN 50
#define SHORT_PRESS_MAX 500
#define LONG_PRESS_MIN  1000
#define VERY_LONG_MS    3000

/* ==================== PRESS TYPE ==================== */
typedef enum {
    PRESS_NONE,
    PRESS_SHORT,
    PRESS_LONG,
    PRESS_VERY_LONG
} press_type_t;

/* ==================== VARIABEL GLOBAL ==================== */
static bool button_pressed = false;
static bool last_button_state = true;  /* Pull-up: HIGH */
static int64_t press_start_time = 0;
static int64_t last_debounce_time = 0;
static bool led1_state = false;
static bool led2_state = false;

static uint32_t short_press_count = 0;
static uint32_t long_press_count = 0;
static uint32_t very_long_press_count = 0;

/* ==================== FUNCTIONS ==================== */

static int64_t millis_now(void)
{
    return esp_timer_get_time() / 1000;
}

static press_type_t detect_press_type(int64_t duration)
{
    if (duration >= VERY_LONG_MS) {
        return PRESS_VERY_LONG;
    } else if (duration >= LONG_PRESS_MIN) {
        return PRESS_LONG;
    } else if (duration >= SHORT_PRESS_MIN) {
        return PRESS_SHORT;
    }
    return PRESS_NONE;
}

static void print_stats(void)
{
    ESP_LOGI(TAG, "--- Statistics ---");
    ESP_LOGI(TAG, "Short Press : %lu", (unsigned long)short_press_count);
    ESP_LOGI(TAG, "Long Press  : %lu", (unsigned long)long_press_count);
    ESP_LOGI(TAG, "Very Long   : %lu", (unsigned long)very_long_press_count);
    ESP_LOGI(TAG, "------------------");
}

static void handle_press(press_type_t press_type)
{
    switch (press_type) {
        case PRESS_SHORT:
            short_press_count++;
            led1_state = !led1_state;
            gpio_set_level(LED_1, led1_state);
            ESP_LOGI(TAG, ">>> SHORT PRESS #%lu - LED1 %s",
                     (unsigned long)short_press_count,
                     led1_state ? "ON" : "OFF");
            break;

        case PRESS_LONG:
            long_press_count++;
            led2_state = !led2_state;
            gpio_set_level(LED_2, led2_state);
            ESP_LOGI(TAG, ">>> LONG PRESS #%lu - LED2 %s",
                     (unsigned long)long_press_count,
                     led2_state ? "ON" : "OFF");
            break;

        case PRESS_VERY_LONG:
            very_long_press_count++;
            led1_state = false;
            led2_state = false;
            gpio_set_level(LED_1, 0);
            gpio_set_level(LED_2, 0);
            ESP_LOGI(TAG, ">>> VERY LONG PRESS #%lu - ALL LEDs OFF (RESET)",
                     (unsigned long)very_long_press_count);
            print_stats();
            break;

        default:
            ESP_LOGI(TAG, ">>> Press too short, ignored");
            break;
    }
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 05: Long/Short Press Detection");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi Button (input + pull-up) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    /* Konfigurasi LED pins (output) */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_1) | (1ULL << LED_2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_1, 0);
    gpio_set_level(LED_2, 0);

    ESP_LOGI(TAG, "Press Types:");
    ESP_LOGI(TAG, "  Short Press  : < %d ms -> Toggle LED1", SHORT_PRESS_MAX);
    ESP_LOGI(TAG, "  Long Press   : %d-%d ms -> Toggle LED2", LONG_PRESS_MIN, VERY_LONG_MS);
    ESP_LOGI(TAG, "  Very Long    : > %d ms -> Reset ALL", VERY_LONG_MS);
    ESP_LOGI(TAG, "Ready! Press the button...");

    static int64_t last_feedback = 0;

    while (1) {
        bool current_button_state = gpio_get_level(BUTTON_PIN);
        int64_t now = millis_now();

        /* Debounce */
        if (current_button_state != last_button_state) {
            last_debounce_time = now;
        }

        if ((now - last_debounce_time) > DEBOUNCE_MS) {
            /* Button baru saja ditekan */
            if (current_button_state == 0 && !button_pressed) {
                button_pressed = true;
                press_start_time = now;
                last_feedback = 0;
                ESP_LOGI(TAG, "Button pressed... ");
            }

            /* Button dilepas */
            if (current_button_state == 1 && button_pressed) {
                button_pressed = false;
                int64_t press_duration = now - press_start_time;
                ESP_LOGI(TAG, "released after %lld ms", press_duration);

                press_type_t press_type = detect_press_type(press_duration);
                handle_press(press_type);
            }

            /* Feedback saat masih ditekan */
            if (button_pressed) {
                int64_t current_duration = now - press_start_time;
                if (current_duration - last_feedback >= 500 && current_duration >= 500) {
                    last_feedback = current_duration;
                    if (current_duration >= VERY_LONG_MS) {
                        ESP_LOGI(TAG, "  -> VERY LONG detected!");
                    } else if (current_duration >= LONG_PRESS_MIN) {
                        ESP_LOGI(TAG, "  -> LONG press: %lld ms", current_duration);
                    }
                }
            }
        }

        last_button_state = current_button_state;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
