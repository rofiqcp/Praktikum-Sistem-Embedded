/**
 * @file main.c
 * @brief Program 09: LED Brightness Control via Serial (ESP-IDF)
 *
 * Mengontrol kecerahan LED menggunakan LEDC PWM.
 * Input dari Serial: angka 0-100 untuk persentase brightness.
 * Termasuk gamma correction untuk perceived linear brightness.
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BRIGHTNESS";

/* ==================== KONFIGURASI ==================== */
#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO     2
#endif

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_10_BIT   /* 10-bit: 0-1023 */
#define LEDC_FREQUENCY      5000

#define BTN_UP              0       /* Button increase */
#define BTN_DOWN            4       /* Button decrease */

/* ==================== VARIABEL ==================== */
static int brightness = 50;

/* ==================== FUNCTIONS ==================== */

static int clamp(int val, int min_val, int max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

static int map_value(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void set_brightness(int percent)
{
    int pwm_value = map_value(percent, 0, 100, 0, 1023);

    /* Gamma correction for perceived linear brightness */
    float gamma = 2.2f;
    int corrected_value = (int)(powf((float)percent / 100.0f, gamma) * 1023.0f);

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, corrected_value);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    ESP_LOGI(TAG, "Brightness: %3d%% (PWM: %4d, Gamma: %4d)", percent, pwm_value, corrected_value);

    /* Print brightness bar */
    int bars = percent / 5;
    printf("[");
    for (int i = 0; i < 20; i++) {
        printf("%s", (i < bars) ? "#" : ".");
    }
    printf("]\n\n");
}

/* ==================== SERIAL INPUT TASK ==================== */
static void serial_task(void *arg)
{
    setvbuf(stdin, NULL, _IONBF, 0);
    char input_buf[16];
    int buf_idx = 0;

    while (1) {
        int c = getchar();
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (c == '\n' || c == '\r') {
            if (buf_idx > 0) {
                input_buf[buf_idx] = '\0';

                char cmd = input_buf[0];
                if (cmd == 'u' || cmd == 'U') {
                    brightness = clamp(brightness + 10, 0, 100);
                } else if (cmd == 'd' || cmd == 'D') {
                    brightness = clamp(brightness - 10, 0, 100);
                } else if (cmd == 'm' || cmd == 'M') {
                    brightness = 100;
                } else if (cmd == 'n' || cmd == 'N') {
                    brightness = 0;
                } else if (isdigit((unsigned char)cmd)) {
                    int value = atoi(input_buf);
                    brightness = clamp(value, 0, 100);
                }

                set_brightness(brightness);
                buf_idx = 0;
            }
        } else if (buf_idx < (int)(sizeof(input_buf) - 1)) {
            input_buf[buf_idx++] = (char)c;
        }
    }
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 09: LED Brightness Control");
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

    /* Konfigurasi Button pins (input + pull-up) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    ESP_LOGI(TAG, "Commands:");
    ESP_LOGI(TAG, "  Type 0-100 : Set brightness percentage");
    ESP_LOGI(TAG, "  Type 'u'   : Increase by 10%%");
    ESP_LOGI(TAG, "  Type 'd'   : Decrease by 10%%");
    ESP_LOGI(TAG, "  Type 'm'   : Set to max (100%%)");
    ESP_LOGI(TAG, "  Type 'n'   : Set to min (0%%)");
    ESP_LOGI(TAG, "Or use buttons:");
    ESP_LOGI(TAG, "  GPIO%d: Increase", BTN_UP);
    ESP_LOGI(TAG, "  GPIO%d: Decrease", BTN_DOWN);

    set_brightness(brightness);

    /* Start serial input task */
    xTaskCreate(serial_task, "serial_task", 4096, NULL, 5, NULL);

    /* Main loop - handle button input */
    int64_t last_btn_press = 0;

    while (1) {
        int64_t now = esp_timer_get_time() / 1000;
        if (now - last_btn_press > 200) {
            if (gpio_get_level(BTN_UP) == 0) {
                brightness = clamp(brightness + 5, 0, 100);
                set_brightness(brightness);
                last_btn_press = now;
            }
            if (gpio_get_level(BTN_DOWN) == 0) {
                brightness = clamp(brightness - 5, 0, 100);
                set_brightness(brightness);
                last_btn_press = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
