/**
 * ==========================================================================
 * PROGRAM 05: PWM LED Breathing (LED Bernapas dengan PWM)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Efek LED "bernapas" menggunakan LEDC PWM dengan fungsi fade hardware.
 *   LED menyala secara perlahan (fade in 2 detik) lalu mati perlahan
 *   (fade out 2 detik) secara terus-menerus. Menggunakan fade hardware
 *   dari LEDC peripheral untuk transisi yang halus.
 *
 * Koneksi Hardware:
 *   - ESP32: LED built-in pada GPIO2
 *   - Atau LED eksternal + resistor 220Ω pada GPIO2
 *
 * Pin Mapping:
 *   ESP32 DevKit V1 : GPIO2
 *   Lolin S2 Mini   : GPIO15
 *   ESP32-S3        : GPIO2
 *
 * API yang digunakan:
 *   - ledc_timer_config()       : Konfigurasi timer PWM
 *   - ledc_channel_config()     : Konfigurasi channel PWM
 *   - ledc_fade_func_install()  : Install fungsi fade
 *   - ledc_set_fade_with_time() : Set fade dengan durasi
 *   - ledc_fade_start()         : Mulai fade
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_BREATH";

/* Konfigurasi pin LED berdasarkan board */
#if CONFIG_IDF_TARGET_ESP32
    #define LED_PIN  2       /* GPIO2 pada ESP32 DevKit */
#elif CONFIG_IDF_TARGET_ESP32S2
    #define LED_PIN  15      /* GPIO15 pada Lolin S2 Mini */
#elif CONFIG_IDF_TARGET_ESP32S3
    #define LED_PIN  2       /* GPIO2 pada ESP32-S3 */
#else
    #define LED_PIN  2
#endif

/* Konfigurasi LEDC PWM */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_13_BIT   /* Resolusi 13-bit (0-8191) */
#define LEDC_FREQUENCY   5000                 /* Frekuensi 5kHz */
#define LEDC_MAX_DUTY    ((1 << 13) - 1)     /* 8191 = duty cycle maksimum */

/* Durasi fade dalam milidetik */
#define FADE_IN_TIME_MS   2000   /* 2 detik fade in */
#define FADE_OUT_TIME_MS  2000   /* 2 detik fade out */

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM LED Breathing Effect ===");
    ESP_LOGI(TAG, "LED Pin: GPIO%d", LED_PIN);
    ESP_LOGI(TAG, "Frekuensi PWM: %d Hz", LEDC_FREQUENCY);
    ESP_LOGI(TAG, "Resolusi: 13-bit (0-%d)", LEDC_MAX_DUTY);

    /* Konfigurasi timer LEDC */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));
    ESP_LOGI(TAG, "Timer LEDC dikonfigurasi");

    /* Konfigurasi channel LEDC */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = LED_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,            /* Mulai dari 0 (LED mati) */
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));
    ESP_LOGI(TAG, "Channel LEDC dikonfigurasi");

    /* Install fungsi fade (diperlukan untuk hardware fade) */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ESP_LOGI(TAG, "Fungsi fade terinstal");

    ESP_LOGI(TAG, "Memulai efek bernapas (fade in: %dms, fade out: %dms)",
             FADE_IN_TIME_MS, FADE_OUT_TIME_MS);

    int cycle = 0;
    while (1) {
        cycle++;

        /* Fase Fade In: LED menyala perlahan (0 → max) */
        ESP_LOGI(TAG, "[Siklus %d] Fade IN - menyala perlahan...", cycle);
        printf("DATA:FADE_IN,%d,%d\n", LEDC_MAX_DUTY, cycle);

        ledc_set_fade_with_time(LEDC_MODE, LEDC_CH,
                                LEDC_MAX_DUTY, FADE_IN_TIME_MS);
        ledc_fade_start(LEDC_MODE, LEDC_CH, LEDC_FADE_WAIT_DONE);

        /* Fase Fade Out: LED mati perlahan (max → 0) */
        ESP_LOGI(TAG, "[Siklus %d] Fade OUT - mati perlahan...", cycle);
        printf("DATA:FADE_OUT,0,%d\n", cycle);

        ledc_set_fade_with_time(LEDC_MODE, LEDC_CH,
                                0, FADE_OUT_TIME_MS);
        ledc_fade_start(LEDC_MODE, LEDC_CH, LEDC_FADE_WAIT_DONE);

        ESP_LOGI(TAG, "[Siklus %d] Selesai", cycle);
    }
}
