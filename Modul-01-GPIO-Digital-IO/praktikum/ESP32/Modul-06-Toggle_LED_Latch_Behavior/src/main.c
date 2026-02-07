/**
 * @file main.c
 * @brief Program 06: Toggle LED dengan Latch Behavior (ESP-IDF)
 *
 * Implementasi latch/flip-flop behavior menggunakan button dan LED.
 * Satu tekan = ON, tekan lagi = OFF (toggle persistent)
 * Menggunakan GPIO ISR service ESP-IDF.
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "TOGGLE_LATCH";

/* ==================== KONFIGURASI ==================== */
#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO     2
#endif

#define BUTTON_PIN          0       /* BOOT button */
#define DEBOUNCE_MS         50

/* ==================== VARIABEL ==================== */
static volatile bool latch_state = false;
static volatile bool button_flag = false;
static volatile int64_t last_interrupt_time = 0;

/* ==================== ISR ==================== */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time() / 1000;
    if (now - last_interrupt_time > DEBOUNCE_MS) {
        button_flag = true;
        last_interrupt_time = now;
    }
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 06: Toggle Latch Behavior");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi Button (input + pull-up + falling edge interrupt) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    /* Konfigurasi LED (output) */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << CONFIG_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(CONFIG_LED_GPIO, 0);

    /* Install GPIO ISR service and add handler */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    ESP_LOGI(TAG, "Latch initialized: OFF");
    ESP_LOGI(TAG, "Press button to toggle latch state");

    while (1) {
        if (button_flag) {
            button_flag = false;

            latch_state = !latch_state;
            gpio_set_level(CONFIG_LED_GPIO, latch_state);

            ESP_LOGI(TAG, "[%lld ms] LATCH = %s",
                     esp_timer_get_time() / 1000,
                     latch_state ? "ON (SET)" : "OFF (RESET)");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
