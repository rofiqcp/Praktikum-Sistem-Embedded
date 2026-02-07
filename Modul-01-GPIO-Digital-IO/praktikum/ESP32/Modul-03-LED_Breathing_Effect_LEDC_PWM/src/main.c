/**
 * @file main.c
 * @brief Program 03: LED Breathing Effect menggunakan LEDC PWM (ESP-IDF)
 *
 * Deskripsi:
 * Menghasilkan efek breathing (fade in/out) pada LED menggunakan
 * peripheral LEDC (LED Control) ESP32 dengan ESP-IDF native API.
 *
 * Hardware:
 * - ESP32 DevKitC / S2 / S3
 * - LED dengan resistor 220Ohm
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LED_BREATH";

/* ==================== KONFIGURASI ==================== */
#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO     2
#endif

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_8_BIT   /* 8-bit: 0-255 */
#define LEDC_FREQUENCY      5000                /* 5 kHz */

#define BREATH_STEP         5
#define BREATH_DELAY_MS     20
#define BREATH_MIN          0
#define BREATH_MAX          255

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 03: LED Breathing (LEDC PWM)");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi LEDC Timer */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    /* Konfigurasi LEDC Channel */
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CONFIG_LED_GPIO,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(TAG, "LED Pin: GPIO%d", CONFIG_LED_GPIO);
    ESP_LOGI(TAG, "PWM Frequency: %d Hz", LEDC_FREQUENCY);
    ESP_LOGI(TAG, "PWM Resolution: 8-bit (0-255)");
    ESP_LOGI(TAG, "Breathing effect started...");

    int brightness = BREATH_MIN;
    int fade_amount = BREATH_STEP;

    while (1) {
        /* Set PWM duty cycle */
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

        /* Update brightness */
        brightness += fade_amount;

        /* Reverse direction at limits */
        if (brightness <= BREATH_MIN || brightness >= BREATH_MAX) {
            fade_amount = -fade_amount;

            int64_t time_ms = esp_timer_get_time() / 1000;
            if (brightness >= BREATH_MAX) {
                ESP_LOGI(TAG, "[%lld ms] Peak brightness: %d", time_ms, brightness);
            } else {
                ESP_LOGI(TAG, "[%lld ms] Minimum brightness: %d", time_ms, brightness);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BREATH_DELAY_MS));
    }
}
