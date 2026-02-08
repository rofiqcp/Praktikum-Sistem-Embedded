/**
 * ==========================================================================
 * PROGRAM 12: DAC vs PWM Compare (Perbandingan DAC dan PWM)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Membandingkan output DAC murni dengan PWM+RC filter. Sinyal ramp
 *   (0→3.3V) dihasilkan pada kedua output, kemudian dibaca kembali
 *   menggunakan ADC untuk membandingkan akurasi kedua metode.
 *
 * Koneksi Hardware (ESP32 original):
 *   - DAC: GPIO25 → ADC1_CH6 (GPIO34) via jumper wire
 *   - PWM: GPIO18 → RC filter (R=10k, C=100nF) → ADC1_CH7 (GPIO35)
 *   - RC filter: GPIO18 → [R 10kΩ] → junction → [C 100nF] → GND
 *     junction → GPIO35 (ADC input)
 *
 * Untuk ESP32-S2/S3 (tanpa DAC):
 *   - PWM1: GPIO18 → RC filter → ADC1_CH3 (GPIO4)
 *   - PWM2: GPIO17 → RC filter → ADC1_CH4 (GPIO5)
 *   (Membandingkan 2 konfigurasi RC filter berbeda)
 *
 * API yang digunakan:
 *   - dac_output_voltage()    : Output DAC
 *   - ledc_set_duty()         : Output PWM
 *   - adc1_get_raw()          : Baca kembali tegangan
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "DAC_VS_PWM";

/* Deteksi ketersediaan DAC */
#if CONFIG_IDF_TARGET_ESP32
    #define HAS_DAC 1
    #include "driver/dac.h"
    #define DAC_CHAN       DAC_CHANNEL_1     /* GPIO25 */
    #define DAC_GPIO      25
    #define ADC_DAC_CH    ADC1_CHANNEL_6    /* GPIO34 - baca output DAC */
    #define ADC_DAC_GPIO  34
    #define ADC_PWM_CH    ADC1_CHANNEL_7    /* GPIO35 - baca output PWM */
    #define ADC_PWM_GPIO  35
#else
    #define HAS_DAC 0
    /* Tanpa DAC, bandingkan dua PWM dengan RC filter berbeda */
    #define ADC_PWM1_CH   ADC1_CHANNEL_3   /* GPIO4 */
    #define ADC_PWM1_GPIO 4
    #define ADC_PWM2_CH   ADC1_CHANNEL_4   /* GPIO5 */
    #define ADC_PWM2_GPIO 5
    #define PWM2_GPIO     17               /* PWM kedua pada GPIO17 */
#endif

/* Pin PWM utama (sama untuk semua board) */
#define PWM_GPIO         18

/* Konfigurasi LEDC PWM */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_8_BIT   /* 8-bit agar setara dengan DAC */
#define LEDC_FREQUENCY   5000                /* 5kHz */
#define LEDC_MAX_DUTY    255

#if !HAS_DAC
/* Channel PWM kedua untuk perbandingan */
#define LEDC_CH2         LEDC_CHANNEL_1
#endif

/* Jumlah langkah ramp */
#define RAMP_STEPS       32
/* Delay per langkah (ms) - cukup lama agar RC filter stabil */
#define STEP_DELAY_MS    200

/**
 * Inisialisasi ADC untuk pembacaan balik
 */
static void adc_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);

#if HAS_DAC
    adc1_config_channel_atten(ADC_DAC_CH, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC_PWM_CH, ADC_ATTEN_DB_12);
    ESP_LOGI(TAG, "ADC dikonfigurasi: DAC→GPIO%d, PWM→GPIO%d",
             ADC_DAC_GPIO, ADC_PWM_GPIO);
#else
    adc1_config_channel_atten(ADC_PWM1_CH, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC_PWM2_CH, ADC_ATTEN_DB_12);
    ESP_LOGI(TAG, "ADC dikonfigurasi: PWM1→GPIO%d, PWM2→GPIO%d",
             ADC_PWM1_GPIO, ADC_PWM2_GPIO);
#endif
}

/**
 * Inisialisasi PWM
 */
static void pwm_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Channel PWM utama */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

#if !HAS_DAC
    /* Channel PWM kedua (untuk S2/S3) */
    ledc_channel_config_t ch2_conf = {
        .gpio_num   = PWM2_GPIO,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH2,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch2_conf));
    ESP_LOGW(TAG, "DAC tidak tersedia! Membandingkan 2 output PWM");
#endif

    ESP_LOGI(TAG, "PWM dikonfigurasi: GPIO%d (%dHz, 8-bit)",
             PWM_GPIO, LEDC_FREQUENCY);
}

/**
 * Konversi nilai ADC 12-bit ke tegangan (mV)
 * Menggunakan atenuasi 12dB: range ~0-3.3V
 */
static float adc_to_voltage(int raw)
{
    return (raw * 3300.0f) / 4095.0f;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 DAC vs PWM Comparison ===");

    /* Inisialisasi periferal */
    adc_init();
    pwm_init();

#if HAS_DAC
    dac_output_enable(DAC_CHAN);
    ESP_LOGI(TAG, "DAC aktif pada GPIO%d", DAC_GPIO);
    ESP_LOGI(TAG, "Koneksi: DAC(GPIO%d)→ADC(GPIO%d), PWM(GPIO%d)→RC→ADC(GPIO%d)",
             DAC_GPIO, ADC_DAC_GPIO, PWM_GPIO, ADC_PWM_GPIO);
#else
    ESP_LOGI(TAG, "Mode: Perbandingan 2 PWM dengan RC filter");
    ESP_LOGI(TAG, "PWM1(GPIO%d)→RC→ADC(GPIO%d), PWM2(GPIO%d)→RC→ADC(GPIO%d)",
             PWM_GPIO, ADC_PWM1_GPIO, PWM2_GPIO, ADC_PWM2_GPIO);
#endif

    ESP_LOGI(TAG, "Langkah ramp: %d | Delay: %d ms/langkah", RAMP_STEPS, STEP_DELAY_MS);
    ESP_LOGI(TAG, "-------------------------------------------");

    /* Header tabel */
#if HAS_DAC
    ESP_LOGI(TAG, " Step | Target(mV) | DAC(mV) | PWM(mV) | Err_DAC | Err_PWM");
#else
    ESP_LOGI(TAG, " Step | Target(mV) | PWM1(mV)| PWM2(mV)| Err_1   | Err_2");
#endif
    ESP_LOGI(TAG, "------+------------+---------+---------+---------+--------");

    int cycle = 0;
    while (1) {
        cycle++;
        ESP_LOGI(TAG, "=== Siklus %d: Ramp 0 → 3.3V ===", cycle);

        float total_err_a = 0;  /* Akumulasi error metode A (DAC/PWM1) */
        float total_err_b = 0;  /* Akumulasi error metode B (PWM/PWM2) */
        int valid_samples = 0;

        for (int step = 0; step <= RAMP_STEPS; step++) {
            /* Hitung nilai target (0-255 untuk 8-bit) */
            uint8_t target_val = (uint8_t)((step * 255) / RAMP_STEPS);
            float target_mv = (target_val * 3300.0f) / 255.0f;

            /* Set output DAC dan PWM ke nilai yang sama */
#if HAS_DAC
            dac_output_voltage(DAC_CHAN, target_val);
#endif
            ledc_set_duty(LEDC_MODE, LEDC_CH, target_val);
            ledc_update_duty(LEDC_MODE, LEDC_CH);

#if !HAS_DAC
            ledc_set_duty(LEDC_MODE, LEDC_CH2, target_val);
            ledc_update_duty(LEDC_MODE, LEDC_CH2);
#endif

            /* Tunggu agar RC filter stabil */
            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));

            /* Baca ADC (rata-rata 16 sampel untuk akurasi) */
            int adc_a_sum = 0, adc_b_sum = 0;
            int num_avg = 16;
            for (int i = 0; i < num_avg; i++) {
#if HAS_DAC
                adc_a_sum += adc1_get_raw(ADC_DAC_CH);
                adc_b_sum += adc1_get_raw(ADC_PWM_CH);
#else
                adc_a_sum += adc1_get_raw(ADC_PWM1_CH);
                adc_b_sum += adc1_get_raw(ADC_PWM2_CH);
#endif
                vTaskDelay(pdMS_TO_TICKS(1));
            }

            int adc_a_raw = adc_a_sum / num_avg;
            int adc_b_raw = adc_b_sum / num_avg;
            float mv_a = adc_to_voltage(adc_a_raw);
            float mv_b = adc_to_voltage(adc_b_raw);

            /* Hitung error (selisih dari target) */
            float err_a = mv_a - target_mv;
            float err_b = mv_b - target_mv;
            total_err_a += fabsf(err_a);
            total_err_b += fabsf(err_b);
            valid_samples++;

            /* Format: DATA:<step>,<target_mv>,<mv_a>,<mv_b>,<err_a>,<err_b>,<siklus> */
            printf("DATA:%d,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n",
                   step, target_mv, mv_a, mv_b, err_a, err_b, cycle);

            ESP_LOGI(TAG, " %3d  |  %7.1f  | %7.1f | %7.1f | %+6.1f | %+6.1f",
                     step, target_mv, mv_a, mv_b, err_a, err_b);
        }

        /* Ringkasan siklus */
        if (valid_samples > 0) {
            float avg_err_a = total_err_a / valid_samples;
            float avg_err_b = total_err_b / valid_samples;

            ESP_LOGI(TAG, "------+------------+---------+---------+---------+--------");
#if HAS_DAC
            ESP_LOGI(TAG, "Rata-rata error absolut: DAC=%.1f mV | PWM=%.1f mV",
                     avg_err_a, avg_err_b);

            if (avg_err_a < avg_err_b) {
                ESP_LOGI(TAG, ">> DAC lebih akurat (error %.1f mV lebih kecil)",
                         avg_err_b - avg_err_a);
            } else {
                ESP_LOGI(TAG, ">> PWM+RC lebih akurat (error %.1f mV lebih kecil)",
                         avg_err_a - avg_err_b);
            }
#else
            ESP_LOGI(TAG, "Rata-rata error absolut: PWM1=%.1f mV | PWM2=%.1f mV",
                     avg_err_a, avg_err_b);
#endif

            /* Format ringkasan */
            printf("SUMMARY:%d,%.1f,%.1f,%d\n",
                   cycle, avg_err_a, avg_err_b, valid_samples);
        }

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Keuntungan DAC: Sinyal analog murni, tidak perlu filter");
        ESP_LOGI(TAG, "Keuntungan PWM: Efisien daya, resolusi fleksibel, semua GPIO");
        ESP_LOGI(TAG, "");

        /* Jeda antar siklus */
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
