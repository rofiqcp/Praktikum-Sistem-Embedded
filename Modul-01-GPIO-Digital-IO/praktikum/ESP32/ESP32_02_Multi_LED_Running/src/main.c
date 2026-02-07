/**
 * ==========================================================================
 *  ESP32_02_Multi_LED_Running — 4x LED Running Light Pattern
 * ==========================================================================
 *  Modul   : 01 - GPIO Digital I/O
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF (PlatformIO)
 *
 *  Deskripsi:
 *    Program menghidupkan 4 LED secara bergantian (running light).
 *    Hanya satu LED yang menyala pada satu waktu, kemudian berpindah
 *    ke LED berikutnya secara sekuensial dan berulang.
 *
 *  Rangkaian / Wiring:
 *    ESP32 GPIO16 ──► Resistor 220Ω ──► LED1 Anoda | Katoda ──► GND
 *    ESP32 GPIO17 ──► Resistor 220Ω ──► LED2 Anoda | Katoda ──► GND
 *    ESP32 GPIO18 ──► Resistor 220Ω ──► LED3 Anoda | Katoda ──► GND
 *    ESP32 GPIO19 ──► Resistor 220Ω ──► LED4 Anoda | Katoda ──► GND
 *    (Pin disesuaikan untuk S2: GPIO33-36, S3: GPIO4-7)
 *
 *  Komponen:
 *    - 4x LED (warna bebas)
 *    - 4x Resistor 220Ω
 *    - Breadboard + kabel jumper
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "RUNNING_LED";

/* Array pin LED agar mudah diiterasi */
static const gpio_num_t led_pins[NUM_LEDS] = {
    LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4
};

/**
 * @brief Inisialisasi semua pin LED sebagai output
 */
static void gpio_init_leds(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN_1) |
                        (1ULL << LED_PIN_2) |
                        (1ULL << LED_PIN_3) |
                        (1ULL << LED_PIN_4),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* Matikan semua LED saat awal */
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(led_pins[i], 0);
    }

    ESP_LOGI(TAG, "GPIO LED terinitialisasi: GPIO%d, GPIO%d, GPIO%d, GPIO%d",
             LED_PIN_1, LED_PIN_2, LED_PIN_3, LED_PIN_4);
}

/**
 * @brief Entry point utama
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Multi LED Running Light ===");
    gpio_init_leds();

    int current_led = 0;

    while (1) {
        /* Matikan semua LED */
        for (int i = 0; i < NUM_LEDS; i++) {
            gpio_set_level(led_pins[i], 0);
        }

        /* Nyalakan LED saat ini */
        gpio_set_level(led_pins[current_led], 1);

        ESP_LOGI(TAG, "LED %d ON  (GPIO%d)", current_led + 1, led_pins[current_led]);

        /* Pindah ke LED berikutnya (wrap-around) */
        current_led = (current_led + 1) % NUM_LEDS;

        vTaskDelay(pdMS_TO_TICKS(RUNNING_DELAY_MS));
    }
}
