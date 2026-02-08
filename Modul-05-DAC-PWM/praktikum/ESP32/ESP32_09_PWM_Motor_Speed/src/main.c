/**
 * ==========================================================================
 * PROGRAM 09: PWM Motor Speed Control (Kontrol Kecepatan Motor DC)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Mengontrol kecepatan dan arah putaran motor DC menggunakan driver
 *   L298N. PWM pada pin ENA mengatur kecepatan, sedangkan IN1/IN2
 *   mengatur arah putaran (maju/mundur).
 *
 * Koneksi Hardware:
 *   ESP32         L298N
 *   GPIO16   →    IN1  (kontrol arah)
 *   GPIO17   →    IN2  (kontrol arah)
 *   GPIO18   →    ENA  (PWM kecepatan)
 *   GND      →    GND
 *   
 *   L298N:
 *   - Motor A: OUT1, OUT2 → Motor DC
 *   - VCC: 5-12V (sesuai motor)
 *   - 5V: output regulator (atau input jika jumper dilepas)
 *
 * Logika Arah:
 *   IN1=HIGH, IN2=LOW  → Motor MAJU (CW)
 *   IN1=LOW,  IN2=HIGH → Motor MUNDUR (CCW)
 *   IN1=LOW,  IN2=LOW  → Motor BERHENTI (coast)
 *   IN1=HIGH, IN2=HIGH → Motor BERHENTI (brake)
 *
 * API yang digunakan:
 *   - gpio_config()          : Konfigurasi pin arah
 *   - ledc_timer_config()    : Konfigurasi timer PWM
 *   - ledc_channel_config()  : Konfigurasi channel PWM
 *   - ledc_set_duty()        : Set kecepatan motor
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_MOTOR";

/* Pin konfigurasi L298N */
#define MOTOR_IN1_PIN    16    /* GPIO16 = IN1 (arah) */
#define MOTOR_IN2_PIN    17    /* GPIO17 = IN2 (arah) */
#define MOTOR_ENA_PIN    18    /* GPIO18 = ENA (PWM kecepatan) */

/* Konfigurasi LEDC PWM */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_10_BIT   /* 10-bit (0-1023) */
#define LEDC_FREQUENCY   1000                 /* 1kHz untuk motor DC */
#define LEDC_MAX_DUTY    ((1 << 10) - 1)     /* 1023 */

/* Enumerasi arah motor */
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,       /* Maju (CW) */
    MOTOR_REVERSE,       /* Mundur (CCW) */
    MOTOR_BRAKE          /* Rem */
} motor_direction_t;

/**
 * Inisialisasi pin GPIO untuk kontrol arah motor
 */
static void motor_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_IN1_PIN) | (1ULL << MOTOR_IN2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* Default: motor berhenti */
    gpio_set_level(MOTOR_IN1_PIN, 0);
    gpio_set_level(MOTOR_IN2_PIN, 0);
}

/**
 * Atur arah putaran motor
 */
static void motor_set_direction(motor_direction_t dir)
{
    switch (dir) {
        case MOTOR_FORWARD:
            gpio_set_level(MOTOR_IN1_PIN, 1);
            gpio_set_level(MOTOR_IN2_PIN, 0);
            break;
        case MOTOR_REVERSE:
            gpio_set_level(MOTOR_IN1_PIN, 0);
            gpio_set_level(MOTOR_IN2_PIN, 1);
            break;
        case MOTOR_BRAKE:
            gpio_set_level(MOTOR_IN1_PIN, 1);
            gpio_set_level(MOTOR_IN2_PIN, 1);
            break;
        case MOTOR_STOP:
        default:
            gpio_set_level(MOTOR_IN1_PIN, 0);
            gpio_set_level(MOTOR_IN2_PIN, 0);
            break;
    }
}

/**
 * Atur kecepatan motor (0-100%)
 */
static void motor_set_speed(int speed_pct)
{
    if (speed_pct < 0) speed_pct = 0;
    if (speed_pct > 100) speed_pct = 100;

    uint32_t duty = (LEDC_MAX_DUTY * speed_pct) / 100;
    ledc_set_duty(LEDC_MODE, LEDC_CH, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CH);
}

/**
 * Mendapatkan nama string untuk arah motor
 */
static const char* direction_to_string(motor_direction_t dir)
{
    switch (dir) {
        case MOTOR_FORWARD: return "MAJU";
        case MOTOR_REVERSE: return "MUNDUR";
        case MOTOR_BRAKE:   return "REM";
        case MOTOR_STOP:    return "BERHENTI";
        default:            return "UNKNOWN";
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM Motor Speed Control ===");
    ESP_LOGI(TAG, "IN1: GPIO%d | IN2: GPIO%d | ENA: GPIO%d",
             MOTOR_IN1_PIN, MOTOR_IN2_PIN, MOTOR_ENA_PIN);

    /* Inisialisasi GPIO arah motor */
    motor_gpio_init();
    ESP_LOGI(TAG, "GPIO motor dikonfigurasi");

    /* Konfigurasi LEDC PWM untuk kecepatan */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t ch_conf = {
        .gpio_num   = MOTOR_ENA_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    ESP_LOGI(TAG, "PWM motor dikonfigurasi (1kHz, 10-bit)");
    ESP_LOGI(TAG, "-------------------------------------------");

    int cycle = 0;
    while (1) {
        cycle++;

        /* ---- MAJU: Ramp naik ---- */
        ESP_LOGI(TAG, "[Siklus %d] MAJU - Ramp naik", cycle);
        motor_set_direction(MOTOR_FORWARD);

        for (int speed = 0; speed <= 100; speed += 10) {
            motor_set_speed(speed);
            printf("DATA:MAJU,%d,%d\n", speed, cycle);
            ESP_LOGI(TAG, "MAJU | Kecepatan: %3d%%", speed);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* ---- MAJU: Ramp turun ---- */
        ESP_LOGI(TAG, "[Siklus %d] MAJU - Ramp turun", cycle);
        for (int speed = 100; speed >= 0; speed -= 10) {
            motor_set_speed(speed);
            printf("DATA:MAJU,%d,%d\n", speed, cycle);
            ESP_LOGI(TAG, "MAJU | Kecepatan: %3d%%", speed);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* ---- Berhenti sejenak ---- */
        motor_set_direction(MOTOR_STOP);
        motor_set_speed(0);
        printf("DATA:BERHENTI,0,%d\n", cycle);
        ESP_LOGI(TAG, "Motor berhenti - jeda 2 detik");
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* ---- MUNDUR: Ramp naik ---- */
        ESP_LOGI(TAG, "[Siklus %d] MUNDUR - Ramp naik", cycle);
        motor_set_direction(MOTOR_REVERSE);

        for (int speed = 0; speed <= 100; speed += 10) {
            motor_set_speed(speed);
            printf("DATA:MUNDUR,%d,%d\n", speed, cycle);
            ESP_LOGI(TAG, "MUNDUR | Kecepatan: %3d%%", speed);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* ---- MUNDUR: Ramp turun ---- */
        ESP_LOGI(TAG, "[Siklus %d] MUNDUR - Ramp turun", cycle);
        for (int speed = 100; speed >= 0; speed -= 10) {
            motor_set_speed(speed);
            printf("DATA:MUNDUR,%d,%d\n", speed, cycle);
            ESP_LOGI(TAG, "MUNDUR | Kecepatan: %3d%%", speed);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* ---- Rem ---- */
        motor_set_direction(MOTOR_BRAKE);
        motor_set_speed(0);
        printf("DATA:REM,0,%d\n", cycle);
        ESP_LOGI(TAG, "Motor DIREM - jeda 2 detik");
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "--- Siklus %d selesai ---", cycle);
    }
}
