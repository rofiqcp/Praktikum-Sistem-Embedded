/**
 * @file main.c
 * @brief Program 10: GPIO Matrix / Pin Reassignment (ESP-IDF)
 *
 * Demonstrasi GPIO Matrix ESP32 - kemampuan memetakan
 * sinyal peripheral ke GPIO yang berbeda secara software.
 * PWM breathing effect berpindah antar pin saat button ditekan.
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "GPIO_MATRIX";

/* ==================== KONFIGURASI ==================== */
#define LED_ORIGINAL    2
#define LED_ALT_1       4
#define LED_ALT_2       5
#define LED_ALT_3       18

#define BUTTON_PIN      0
#define PWM_FREQ        5000
#define PWM_RESOLUTION  LEDC_TIMER_8_BIT    /* 8-bit: 0-255 */

#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_TIMER_NUM  LEDC_TIMER_0
#define LEDC_CH         LEDC_CHANNEL_0

static const gpio_num_t led_pins[] = {LED_ORIGINAL, LED_ALT_1, LED_ALT_2, LED_ALT_3};
static const uint8_t num_pins = sizeof(led_pins) / sizeof(led_pins[0]);

/* ==================== VARIABEL ==================== */
static uint8_t current_pin = 0;
static volatile bool switch_pin_flag = false;
static volatile int64_t last_switch_time = 0;

/* ==================== ISR ==================== */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time() / 1000;
    if (now - last_switch_time > 300) {
        switch_pin_flag = true;
        last_switch_time = now;
    }
}

/* ==================== FUNCTIONS ==================== */

static void setup_ledc_on_pin(gpio_num_t pin)
{
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CH,
        .timer_sel      = LEDC_TIMER_NUM,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = pin,
        .duty           = 0,
        .hpoint         = 0,
    };
    ledc_channel_config(&ledc_channel);
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 10: GPIO Matrix Demo");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi semua LED pins sebagai output (untuk reset state) */
    uint64_t led_mask = 0;
    for (int i = 0; i < num_pins; i++) {
        led_mask |= (1ULL << led_pins[i]);
    }
    gpio_config_t led_conf = {
        .pin_bit_mask = led_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    for (int i = 0; i < num_pins; i++) {
        gpio_set_level(led_pins[i], 0);
    }

    /* Konfigurasi Button (input + pull-up + interrupt) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    /* Konfigurasi LEDC Timer */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = PWM_RESOLUTION,
        .timer_num        = LEDC_TIMER_NUM,
        .freq_hz          = PWM_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    /* Start with first pin */
    setup_ledc_on_pin(led_pins[current_pin]);

    ESP_LOGI(TAG, "ESP32 GPIO Matrix Features:");
    ESP_LOGI(TAG, "  - Peripheral signals can be mapped to almost any GPIO");
    ESP_LOGI(TAG, "  - PWM, SPI, I2C, UART can use different pins");
    ESP_LOGI(TAG, "  - Great for PCB routing flexibility");
    ESP_LOGI(TAG, "Available LED pins:");
    for (int i = 0; i < num_pins; i++) {
        ESP_LOGI(TAG, "  %d. GPIO%d", i + 1, led_pins[i]);
    }
    ESP_LOGI(TAG, "Currently active: GPIO%d", led_pins[current_pin]);
    ESP_LOGI(TAG, "Press button to switch PWM to next pin");

    int brightness = 0;
    int fade_amount = 5;

    while (1) {
        /* Handle pin switching */
        if (switch_pin_flag) {
            switch_pin_flag = false;

            /* Stop PWM on current pin by setting duty to 0 then resetting GPIO */
            ledc_set_duty(LEDC_MODE, LEDC_CH, 0);
            ledc_update_duty(LEDC_MODE, LEDC_CH);
            ledc_stop(LEDC_MODE, LEDC_CH, 0);

            /* Reset old pin to plain GPIO output LOW */
            gpio_reset_pin(led_pins[current_pin]);
            gpio_set_direction(led_pins[current_pin], GPIO_MODE_OUTPUT);
            gpio_set_level(led_pins[current_pin], 0);

            /* Move to next pin */
            current_pin = (current_pin + 1) % num_pins;

            /* Attach LEDC to new pin */
            setup_ledc_on_pin(led_pins[current_pin]);

            ESP_LOGI(TAG, ">>> PWM switched to GPIO%d", led_pins[current_pin]);
        }

        /* Breathing effect on current pin */
        ledc_set_duty(LEDC_MODE, LEDC_CH, brightness);
        ledc_update_duty(LEDC_MODE, LEDC_CH);

        brightness += fade_amount;
        if (brightness <= 0 || brightness >= 255) {
            fade_amount = -fade_amount;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
