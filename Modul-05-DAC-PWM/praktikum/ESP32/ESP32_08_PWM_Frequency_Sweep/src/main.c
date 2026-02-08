/**
 * ==========================================================================
 * PROGRAM 08: PWM Frequency Sweep (Sapuan Frekuensi PWM)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Men-sweep frekuensi PWM dari 100Hz sampai 20kHz secara bertahap.
 *   Frekuensi timer LEDC diubah secara dinamis menggunakan
 *   ledc_set_freq(). Duty cycle dipertahankan pada 50%.
 *   Berguna untuk pengujian respons frekuensi filter/speaker.
 *
 * Koneksi Hardware:
 *   - Output PWM pada GPIO18
 *   - Oscilloscope atau frequency counter pada GPIO18
 *   - Opsional: speaker/buzzer untuk mendengar perubahan frekuensi
 *
 * API yang digunakan:
 *   - ledc_timer_config()    : Konfigurasi timer awal
 *   - ledc_channel_config()  : Konfigurasi channel
 *   - ledc_set_freq()        : Ubah frekuensi secara dinamis
 *   - ledc_set_duty()        : Set duty cycle
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "PWM_SWEEP";

/* Pin output PWM */
#define PWM_PIN          18

/* Konfigurasi LEDC */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_10_BIT   /* 10-bit resolusi (0-1023) */
#define LEDC_HALF_DUTY   512                  /* 50% duty cycle */

/* Range frekuensi sweep */
#define FREQ_START       100      /* Frekuensi awal: 100 Hz */
#define FREQ_END         20000    /* Frekuensi akhir: 20 kHz */
#define SWEEP_STEPS      50       /* Jumlah langkah sweep */
#define STEP_DELAY_MS    500      /* Delay antar langkah (ms) */

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM Frequency Sweep ===");
    ESP_LOGI(TAG, "PWM Pin: GPIO%d", PWM_PIN);
    ESP_LOGI(TAG, "Range: %d Hz → %d Hz", FREQ_START, FREQ_END);
    ESP_LOGI(TAG, "Langkah: %d | Delay: %d ms", SWEEP_STEPS, STEP_DELAY_MS);

    /* Konfigurasi timer LEDC dengan frekuensi awal */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = FREQ_START,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Konfigurasi channel LEDC */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = PWM_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = LEDC_HALF_DUTY,  /* 50% duty cycle */
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    ESP_LOGI(TAG, "LEDC dikonfigurasi, duty cycle 50%%");
    ESP_LOGI(TAG, "-------------------------------------------");

    int cycle = 0;
    while (1) {
        cycle++;
        ESP_LOGI(TAG, "[Siklus %d] Sweep maju: %d → %d Hz", cycle, FREQ_START, FREQ_END);

        /* Sweep maju: rendah → tinggi */
        for (int step = 0; step <= SWEEP_STEPS; step++) {
            /* Hitung frekuensi untuk langkah ini (skala logaritmik) */
            float ratio = (float)step / SWEEP_STEPS;
            /* Menggunakan interpolasi logaritmik untuk distribusi yang lebih merata */
            float log_start = logf((float)FREQ_START);
            float log_end = logf((float)FREQ_END);
            uint32_t freq = (uint32_t)expf(log_start + ratio * (log_end - log_start));

            /* Pastikan frekuensi dalam range yang valid */
            if (freq < 1) freq = 1;

            /* Set frekuensi baru */
            esp_err_t err = ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Gagal set frekuensi %lu Hz", (unsigned long)freq);
                continue;
            }

            /* Update duty cycle (mungkin berubah karena perubahan frekuensi) */
            ledc_set_duty(LEDC_MODE, LEDC_CH, LEDC_HALF_DUTY);
            ledc_update_duty(LEDC_MODE, LEDC_CH);

            /* Format: DATA:<frekuensi>,<step>,<total_step>,<siklus> */
            printf("DATA:%lu,%d,%d,%d\n", (unsigned long)freq, step, SWEEP_STEPS, cycle);
            ESP_LOGI(TAG, "Step %2d/%d | Frekuensi: %6lu Hz",
                     step, SWEEP_STEPS, (unsigned long)freq);

            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }

        ESP_LOGI(TAG, "[Siklus %d] Sweep mundur: %d → %d Hz", cycle, FREQ_END, FREQ_START);

        /* Sweep mundur: tinggi → rendah */
        for (int step = SWEEP_STEPS; step >= 0; step--) {
            float ratio = (float)step / SWEEP_STEPS;
            float log_start = logf((float)FREQ_START);
            float log_end = logf((float)FREQ_END);
            uint32_t freq = (uint32_t)expf(log_start + ratio * (log_end - log_start));

            if (freq < 1) freq = 1;

            ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
            ledc_set_duty(LEDC_MODE, LEDC_CH, LEDC_HALF_DUTY);
            ledc_update_duty(LEDC_MODE, LEDC_CH);

            printf("DATA:%lu,%d,%d,%d\n", (unsigned long)freq, step, SWEEP_STEPS, cycle);
            ESP_LOGI(TAG, "Step %2d/%d | Frekuensi: %6lu Hz",
                     step, SWEEP_STEPS, (unsigned long)freq);

            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }

        /* Jeda antar siklus */
        ESP_LOGI(TAG, "--- Siklus %d selesai, jeda 2 detik ---", cycle);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
