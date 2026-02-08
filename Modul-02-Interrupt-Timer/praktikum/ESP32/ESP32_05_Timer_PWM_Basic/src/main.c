/**
 * ============================================================
 *  ESP32_05_Timer_PWM_Basic
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi konfigurasi Hardware Timer untuk mode PWM
 *    menggunakan LEDC (LED Control) peripheral ESP32.
 *    Fokus pada KONFIGURASI TIMER, bukan aplikasi PWM.
 *
 *    LEDC menggunakan hardware timer internal untuk generate
 *    sinyal PWM. Program ini menunjukkan bagaimana timer
 *    dikonfigurasi: frekuensi, resolusi, dan duty cycle.
 *
 *  Koneksi Hardware:
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *    - Opsional: Oscilloscope pada GPIO2 untuk melihat sinyal
 *
 *  Cara Kerja:
 *    1. Konfigurasi LEDC timer: 50 Hz, 13-bit resolusi
 *    2. Konfigurasi LEDC channel: GPIO2, duty 50%
 *    3. Log parameter timer (frekuensi, resolusi, duty)
 *    4. Main loop membaca dan log duty secara berkala
 *
 *  Catatan:
 *    Ini adalah SATU-SATUNYA program PWM di Modul 02.
 *    Tujuannya mendemonstrasikan mode PWM dari timer,
 *    bukan aplikasi PWM yang lebih lanjut.
 * ============================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "TIMER_PWM";

/* ---- LEDC Configuration ---- */
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT   // 13-bit → 0..8191
#define LEDC_MAX_DUTY       ((1 << 13) - 1)      // 8191

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Timer PWM Mode Demo ===");
    ESP_LOGI(TAG, "PWM pin       : GPIO%d", PWM_PIN);
    ESP_LOGI(TAG, "PWM frequency : %d Hz", PWM_FREQ);

    /* ============================================================
     *  Step 1: Konfigurasi LEDC Timer
     *  Timer adalah inti dari PWM — menentukan frekuensi dan resolusi.
     * ============================================================ */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,       // 13-bit (0-8191)
        .timer_num       = LEDC_TIMER,
        .freq_hz         = PWM_FREQ,            // 50 Hz
        .clk_cfg         = LEDC_AUTO_CLK,       // Auto-select clock source
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Hitung dan log parameter timer */
    uint32_t max_duty = LEDC_MAX_DUTY;
    uint32_t half_duty = max_duty / 2;          // 50% duty
    float period_ms = 1000.0f / PWM_FREQ;

    ESP_LOGI(TAG, "--- Timer Parameters ---");
    ESP_LOGI(TAG, "  Timer        : LEDC_TIMER_%d", LEDC_TIMER);
    ESP_LOGI(TAG, "  Resolution   : 13-bit (0 - %lu)", (unsigned long)max_duty);
    ESP_LOGI(TAG, "  Frequency    : %d Hz", PWM_FREQ);
    ESP_LOGI(TAG, "  Period       : %.2f ms", period_ms);
    ESP_LOGI(TAG, "  Clock source : AUTO");

    /* ============================================================
     *  Step 2: Konfigurasi LEDC Channel
     *  Channel menghubungkan timer ke GPIO dan mengatur duty cycle.
     * ============================================================ */
    ledc_channel_config_t channel_conf = {
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = PWM_PIN,
        .duty       = half_duty,                // 50% duty cycle
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    float duty_pct = (float)half_duty / (float)max_duty * 100.0f;
    float high_ms  = period_ms * duty_pct / 100.0f;
    float low_ms   = period_ms - high_ms;

    ESP_LOGI(TAG, "--- Channel Parameters ---");
    ESP_LOGI(TAG, "  Channel      : LEDC_CHANNEL_%d", LEDC_CHANNEL);
    ESP_LOGI(TAG, "  GPIO         : %d", PWM_PIN);
    ESP_LOGI(TAG, "  Duty value   : %lu / %lu", (unsigned long)half_duty, (unsigned long)max_duty);
    ESP_LOGI(TAG, "  Duty percent : %.1f%%", duty_pct);
    ESP_LOGI(TAG, "  HIGH time    : %.2f ms", high_ms);
    ESP_LOGI(TAG, "  LOW  time    : %.2f ms", low_ms);

    ESP_LOGI(TAG, "PWM output aktif pada GPIO%d.", PWM_PIN);
    ESP_LOGI(TAG, "Gunakan oscilloscope untuk melihat sinyal.");

    /* ============================================================
     *  Main Loop: Tampilkan status timer PWM secara berkala
     * ============================================================ */
    uint32_t loop_count = 0;
    while (1) {
        loop_count++;

        /* Baca duty aktual dari hardware */
        uint32_t current_duty = ledc_get_duty(LEDC_MODE, LEDC_CHANNEL);
        float current_pct = (float)current_duty / (float)max_duty * 100.0f;

        if (loop_count % 10 == 0) {
            ESP_LOGI(TAG, "PWM Status: freq=%d Hz | duty=%lu/%lu (%.1f%%) | period=%.2f ms",
                     PWM_FREQ,
                     (unsigned long)current_duty,
                     (unsigned long)max_duty,
                     current_pct,
                     period_ms);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
