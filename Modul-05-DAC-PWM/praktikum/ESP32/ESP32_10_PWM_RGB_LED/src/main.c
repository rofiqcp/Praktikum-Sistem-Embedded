/**
 * ==========================================================================
 * PROGRAM 10: PWM RGB LED (LED RGB dengan Efek Rainbow)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Mengontrol LED RGB (common cathode) menggunakan 3 channel LEDC PWM.
 *   Menghasilkan efek rainbow cycle dengan konversi warna HSV ke RGB
 *   untuk transisi warna yang halus dan kontinu.
 *
 * Koneksi Hardware:
 *   ESP32 DevKit:
 *     GPIO25 → R (merah) + resistor 220Ω
 *     GPIO26 → G (hijau) + resistor 220Ω
 *     GPIO27 → B (biru)  + resistor 220Ω
 *     GND    → Katoda (common cathode)
 *
 *   ESP32-S2/S3 (GPIO25/26 tidak ada DAC):
 *     GPIO2  → R
 *     GPIO4  → G
 *     GPIO5  → B
 *
 * API yang digunakan:
 *   - ledc_timer_config()    : Konfigurasi timer PWM
 *   - ledc_channel_config()  : Konfigurasi 3 channel (R, G, B)
 *   - ledc_set_duty()        : Set duty cycle per channel
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_RGB";

/* Konfigurasi pin RGB berdasarkan board */
#if CONFIG_IDF_TARGET_ESP32
    #define PIN_RED    25
    #define PIN_GREEN  26
    #define PIN_BLUE   27
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define PIN_RED    2
    #define PIN_GREEN  4
    #define PIN_BLUE   5
#else
    #define PIN_RED    25
    #define PIN_GREEN  26
    #define PIN_BLUE   27
#endif

/* Konfigurasi LEDC PWM */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES    LEDC_TIMER_8_BIT    /* 8-bit (0-255) */
#define LEDC_FREQUENCY   5000                 /* 5kHz */

/* Channel untuk masing-masing warna */
#define CH_RED     LEDC_CHANNEL_0
#define CH_GREEN   LEDC_CHANNEL_1
#define CH_BLUE    LEDC_CHANNEL_2

/**
 * Konversi warna HSV ke RGB
 * 
 * Parameter:
 *   h: Hue (0-360)
 *   s: Saturation (0.0-1.0)
 *   v: Value/Brightness (0.0-1.0)
 *   r, g, b: Output warna (0-255)
 */
static void hsv_to_rgb(float h, float s, float v,
                       uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;                  /* Chroma */
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r1, g1, b1;

    if (h < 60.0f) {
        r1 = c; g1 = x; b1 = 0;
    } else if (h < 120.0f) {
        r1 = x; g1 = c; b1 = 0;
    } else if (h < 180.0f) {
        r1 = 0; g1 = c; b1 = x;
    } else if (h < 240.0f) {
        r1 = 0; g1 = x; b1 = c;
    } else if (h < 300.0f) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    *r = (uint8_t)((r1 + m) * 255.0f);
    *g = (uint8_t)((g1 + m) * 255.0f);
    *b = (uint8_t)((b1 + m) * 255.0f);
}

/**
 * Set warna RGB pada LED
 */
static void rgb_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    ledc_set_duty(LEDC_MODE, CH_RED, r);
    ledc_update_duty(LEDC_MODE, CH_RED);

    ledc_set_duty(LEDC_MODE, CH_GREEN, g);
    ledc_update_duty(LEDC_MODE, CH_GREEN);

    ledc_set_duty(LEDC_MODE, CH_BLUE, b);
    ledc_update_duty(LEDC_MODE, CH_BLUE);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM RGB LED Rainbow ===");
    ESP_LOGI(TAG, "Pin R: GPIO%d | G: GPIO%d | B: GPIO%d",
             PIN_RED, PIN_GREEN, PIN_BLUE);

    /* Konfigurasi timer LEDC (satu timer untuk semua channel) */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Konfigurasi channel Merah */
    ledc_channel_config_t ch_red = {
        .gpio_num   = PIN_RED,
        .speed_mode = LEDC_MODE,
        .channel    = CH_RED,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_red));

    /* Konfigurasi channel Hijau */
    ledc_channel_config_t ch_green = {
        .gpio_num   = PIN_GREEN,
        .speed_mode = LEDC_MODE,
        .channel    = CH_GREEN,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_green));

    /* Konfigurasi channel Biru */
    ledc_channel_config_t ch_blue = {
        .gpio_num   = PIN_BLUE,
        .speed_mode = LEDC_MODE,
        .channel    = CH_BLUE,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_blue));

    ESP_LOGI(TAG, "LEDC 3 channel dikonfigurasi (5kHz, 8-bit)");
    ESP_LOGI(TAG, "Memulai efek rainbow cycle...");

    /* Loop utama: rainbow cycle */
    float hue = 0.0f;           /* Hue awal (derajat) */
    float hue_step = 1.0f;      /* Langkah hue per iterasi */
    float saturation = 1.0f;    /* Saturasi penuh */
    float value = 1.0f;         /* Kecerahan penuh */

    int cycle = 0;
    while (1) {
        uint8_t r, g, b;

        /* Konversi HSV ke RGB */
        hsv_to_rgb(hue, saturation, value, &r, &g, &b);

        /* Terapkan warna ke LED */
        rgb_set_color(r, g, b);

        /* Tampilkan info setiap 30 derajat hue (12 warna utama) */
        if ((int)hue % 30 == 0) {
            /* Format: DATA:<hue>,<r>,<g>,<b>,<siklus> */
            printf("DATA:%.0f,%d,%d,%d,%d\n", hue, r, g, b, cycle);
            ESP_LOGI(TAG, "Hue: %3.0f° | R:%3d G:%3d B:%3d",
                     hue, r, g, b);
        }

        /* Update hue */
        hue += hue_step;
        if (hue >= 360.0f) {
            hue -= 360.0f;
            cycle++;
            ESP_LOGI(TAG, "--- Siklus rainbow %d selesai ---", cycle);
        }

        /* Delay untuk transisi halus (~30ms = ~33 fps) */
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
