/**
 * @file main.c
 * @brief Program 10 - Rotary Encoder Interface (Quadrature Decode)
 * @details Membaca rotary encoder KY-040 menggunakan GPIO interrupt.
 *          Decode quadrature A/B untuk menentukan arah putaran.
 *          Posisi di-track dan ditampilkan via serial.
 *
 * @modul    Modul 02 - Interrupt & Timer
 * @board    ESP32 DOIT DevKit V1 / Lolin S2 Mini / ESP32-S3
 * @framework ESP-IDF
 *
 * @koneksi Hardware:
 *   KY-040 CLK --> ESP32 GPIO18
 *   KY-040 DT  --> ESP32 GPIO19
 *   KY-040 SW  --> ESP32 GPIO4  (pull-up internal)
 *   KY-040 +   --> 3.3V
 *   KY-040 GND --> GND
 *   ESP32 GPIO2 --> LED (+) --> 220Ω --> GND
 *
 * @cara_kerja:
 *   1. Channel A dan B dikonfigurasi dengan interrupt
 *   2. Pada rising/falling edge A, baca state B
 *   3. Jika A != B → clockwise (CW), increment position
 *   4. Jika A == B → counter-clockwise (CCW), decrement position
 *   5. Push button toggle LED dan reset position
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "ENCODER";

/* Variabel encoder (volatile untuk ISR) */
static volatile int32_t encoder_position = 0;
static volatile int32_t last_reported_position = -1;
static volatile bool direction_cw = true;
static volatile int64_t last_edge_time = 0;
static volatile bool button_pressed = false;

/**
 * @brief ISR handler untuk Channel A (CLK) - decode quadrature
 */
static void IRAM_ATTR encoder_a_isr(void *arg)
{
    int64_t now = esp_timer_get_time();

    /* Software debounce */
    if ((now - last_edge_time) < DEBOUNCE_US) {
        return;
    }
    last_edge_time = now;

    /* Read both channels */
    int a_state = gpio_get_level(ENCODER_A_PIN);
    int b_state = gpio_get_level(ENCODER_B_PIN);

    /* Quadrature decode:
     * Jika A leading B → CW
     * Jika B leading A → CCW */
    if (a_state != b_state) {
        /* CW rotation */
        direction_cw = true;
        if (encoder_position < POSITION_MAX) {
            encoder_position++;
        }
    } else {
        /* CCW rotation */
        direction_cw = false;
        if (encoder_position > POSITION_MIN) {
            encoder_position--;
        }
    }
}

/**
 * @brief ISR handler untuk push button (SW)
 */
static void IRAM_ATTR button_isr(void *arg)
{
    int64_t now = esp_timer_get_time();
    static int64_t last_btn_time = 0;

    if ((now - last_btn_time) < 200000) { /* 200ms debounce */
        return;
    }
    last_btn_time = now;

    button_pressed = true;
}

/**
 * @brief Inisialisasi GPIO
 */
static void gpio_init(void)
{
    /* LED output */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_conf));

    /* Encoder channels A & B - input with pull-up */
    gpio_config_t enc_conf = {
        .pin_bit_mask = (1ULL << ENCODER_A_PIN) | (1ULL << ENCODER_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&enc_conf));

    /* Encoder button - input with pull-up, falling edge */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << ENCODER_SW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_conf));

    /* Install ISR service */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_A_PIN, encoder_a_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_SW_PIN, button_isr, NULL));
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Rotary Encoder Interface ===");
    ESP_LOGI(TAG, "Encoder: A=GPIO%d, B=GPIO%d, SW=GPIO%d",
             ENCODER_A_PIN, ENCODER_B_PIN, ENCODER_SW_PIN);
    ESP_LOGI(TAG, "Position range: %d - %d", POSITION_MIN, POSITION_MAX);

    gpio_init();
    gpio_set_level(LED_PIN, 0);

    bool led_state = false;

    ESP_LOGI(TAG, "Turn encoder to change position. Press button to reset.");

    while (1) {
        /* Handle button press */
        if (button_pressed) {
            button_pressed = false;
            encoder_position = 0;
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state ? 1 : 0);
            ESP_LOGW(TAG, "Button pressed! Position RESET to 0 | LED: %s",
                     led_state ? "ON" : "OFF");
        }

        /* Report position changes */
        int32_t pos = encoder_position;
        if (pos != last_reported_position) {
            last_reported_position = pos;

            /* Visual bar */
            char bar[52] = {0};
            int bar_len = (pos * 50) / POSITION_MAX;
            for (int i = 0; i < 50; i++) {
                bar[i] = (i < bar_len) ? '#' : '-';
            }

            ESP_LOGI(TAG, "Position: %3ld | Dir: %s | [%s] %ld%%",
                     (long)pos,
                     direction_cw ? "CW " : "CCW",
                     bar,
                     (long)(pos * 100 / POSITION_MAX));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
