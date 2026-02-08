/**
 * ==========================================================================
 * PROGRAM 07: PWM Servo Control (Kontrol Servo Motor SG90)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Mengontrol servo motor SG90 menggunakan LEDC PWM pada 50Hz.
 *   Servo bergerak dari 0° ke 180° dan kembali secara bertahap.
 *   Pulse width: 1ms (0°) sampai 2ms (180°) pada periode 20ms (50Hz).
 *
 * Koneksi Hardware:
 *   - Servo SG90: Signal → GPIO18, VCC → 5V, GND → GND
 *   - PENTING: Gunakan catu daya terpisah untuk servo (bukan dari USB)
 *
 * Rumus Duty Cycle:
 *   Resolusi 16-bit = 65535
 *   Duty untuk 1ms = 1ms / 20ms * 65535 = 3277
 *   Duty untuk 2ms = 2ms / 20ms * 65535 = 6554
 *   Duty untuk sudut = 3277 + (sudut/180) * (6554 - 3277)
 *
 * API yang digunakan:
 *   - ledc_timer_config()    : Konfigurasi timer 50Hz
 *   - ledc_channel_config()  : Konfigurasi channel PWM
 *   - ledc_set_duty()        : Set duty cycle
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_SERVO";

/* Pin servo motor */
#define SERVO_PIN        18

/* Konfigurasi LEDC untuk servo */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_16_BIT   /* Resolusi 16-bit untuk presisi tinggi */
#define LEDC_FREQUENCY   50                   /* 50Hz = periode 20ms untuk servo */

/* Konstanta servo SG90 */
#define SERVO_MIN_PULSEWIDTH_US  1000   /* Pulse width minimum 1ms (0°) */
#define SERVO_MAX_PULSEWIDTH_US  2000   /* Pulse width maksimum 2ms (180°) */
#define SERVO_MAX_ANGLE          180    /* Sudut maksimum servo */

/* Resolusi penuh 16-bit */
#define LEDC_FULL_DUTY  ((1 << 16) - 1)  /* 65535 */

/**
 * Konversi sudut servo (0-180°) ke duty cycle LEDC
 * 
 * Rumus:
 *   pulse_width = min_pulse + (sudut / max_sudut) * (max_pulse - min_pulse)
 *   duty = pulse_width / periode * resolusi_penuh
 *   periode = 1/50Hz = 20000 us
 */
static uint32_t angle_to_duty(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;

    /* Hitung pulse width dalam mikroseconds */
    uint32_t pulse_us = SERVO_MIN_PULSEWIDTH_US +
        ((uint32_t)angle * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US)) / SERVO_MAX_ANGLE;

    /* Konversi ke duty cycle: duty = pulse_us / 20000 * 65535 */
    uint32_t duty = (pulse_us * LEDC_FULL_DUTY) / 20000;

    return duty;
}

/**
 * Atur posisi servo ke sudut tertentu
 */
static void servo_set_angle(int angle)
{
    uint32_t duty = angle_to_duty(angle);
    ledc_set_duty(LEDC_MODE, LEDC_CH, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CH);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM Servo Motor Control ===");
    ESP_LOGI(TAG, "Servo Pin: GPIO%d", SERVO_PIN);
    ESP_LOGI(TAG, "Frekuensi: 50Hz | Resolusi: 16-bit");

    /* Konfigurasi timer LEDC */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Konfigurasi channel LEDC */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = SERVO_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    ESP_LOGI(TAG, "LEDC dikonfigurasi untuk servo");
    ESP_LOGI(TAG, "Duty 0°  : %lu", (unsigned long)angle_to_duty(0));
    ESP_LOGI(TAG, "Duty 90° : %lu", (unsigned long)angle_to_duty(90));
    ESP_LOGI(TAG, "Duty 180°: %lu", (unsigned long)angle_to_duty(180));
    ESP_LOGI(TAG, "-------------------------------------------");
    ESP_LOGI(TAG, "Memulai sweep servo 0° → 180° → 0°");

    int sweep_step = 5;      /* Langkah sudut per iterasi (derajat) */
    int delay_ms = 100;      /* Delay antar langkah (ms) */
    int cycle = 0;

    while (1) {
        cycle++;
        ESP_LOGI(TAG, "[Siklus %d] Sweep 0° → 180°", cycle);

        /* Sweep dari 0° ke 180° */
        for (int angle = 0; angle <= 180; angle += sweep_step) {
            servo_set_angle(angle);
            uint32_t duty = angle_to_duty(angle);
            uint32_t pulse_us = SERVO_MIN_PULSEWIDTH_US +
                ((uint32_t)angle * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US)) / SERVO_MAX_ANGLE;

            /* Format: DATA:<sudut>,<duty>,<pulse_us>,<siklus> */
            printf("DATA:%d,%lu,%lu,%d\n", angle, (unsigned long)duty,
                   (unsigned long)pulse_us, cycle);
            ESP_LOGI(TAG, "Sudut: %3d° | Duty: %5lu | Pulse: %4lu us",
                     angle, (unsigned long)duty, (unsigned long)pulse_us);

            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }

        ESP_LOGI(TAG, "[Siklus %d] Sweep 180° → 0°", cycle);

        /* Sweep dari 180° ke 0° */
        for (int angle = 180; angle >= 0; angle -= sweep_step) {
            servo_set_angle(angle);
            uint32_t duty = angle_to_duty(angle);
            uint32_t pulse_us = SERVO_MIN_PULSEWIDTH_US +
                ((uint32_t)angle * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US)) / SERVO_MAX_ANGLE;

            printf("DATA:%d,%lu,%lu,%d\n", angle, (unsigned long)duty,
                   (unsigned long)pulse_us, cycle);
            ESP_LOGI(TAG, "Sudut: %3d° | Duty: %5lu | Pulse: %4lu us",
                     angle, (unsigned long)duty, (unsigned long)pulse_us);

            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }

        /* Jeda di posisi 0° sebelum siklus berikutnya */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
