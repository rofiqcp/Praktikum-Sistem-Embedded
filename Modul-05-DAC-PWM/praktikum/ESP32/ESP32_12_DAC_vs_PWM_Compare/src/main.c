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
 * API yang digunakan (ESP-IDF v5.x):
 *   - dac_output_voltage()         : Output DAC
 *   - ledc_set_duty()              : Output PWM
 *   - adc_oneshot_read()           : Baca kembali tegangan (new API)
 *   - adc_cali_raw_to_voltage()    : Konversi ADC ke tegangan terkalibrasi
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "DAC_VS_PWM";

/* Deteksi ketersediaan DAC */
#if CONFIG_IDF_TARGET_ESP32
    #define HAS_DAC 1
    #include "driver/dac.h"
    #define DAC_CHAN       DAC_CHANNEL_1     /* GPIO25 */
    #define DAC_GPIO      25
    #define ADC_DAC_CH    ADC_CHANNEL_6     /* GPIO34 - baca output DAC */
    #define ADC_DAC_GPIO  34
    #define ADC_PWM_CH    ADC_CHANNEL_7     /* GPIO35 - baca output PWM */
    #define ADC_PWM_GPIO  35
#else
    #define HAS_DAC 0
    /* Tanpa DAC, bandingkan dua PWM dengan RC filter berbeda */
    #define ADC_PWM1_CH   ADC_CHANNEL_3    /* GPIO4 */
    #define ADC_PWM1_GPIO 4
    #define ADC_PWM2_CH   ADC_CHANNEL_4    /* GPIO5 */
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

/* ADC oneshot handle dan calibration handle */
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool cali_enabled = false;

/**
 * Inisialisasi kalibrasi ADC
 */
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    esp_err_t ret = ESP_FAIL;
    adc_cali_handle_t handle = NULL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Kalibrasi: curve fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = unit,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
        calibrated = true;
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Kalibrasi: line fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = unit,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
        calibrated = true;
    }
#endif

    *out_handle = handle;
    if (!calibrated) {
        ESP_LOGW(TAG, "Kalibrasi ADC tidak didukung, menggunakan konversi manual");
    }
    return calibrated;
}

/**
 * Inisialisasi ADC oneshot untuk pembacaan balik
 */
static void adc_init(void)
{
    /* Inisialisasi ADC unit */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    /* Konfigurasi channel */
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };

#if HAS_DAC
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_DAC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PWM_CH, &config));
    ESP_LOGI(TAG, "ADC dikonfigurasi: DAC→GPIO%d, PWM→GPIO%d",
             ADC_DAC_GPIO, ADC_PWM_GPIO);
#else
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PWM1_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PWM2_CH, &config));
    ESP_LOGI(TAG, "ADC dikonfigurasi: PWM1→GPIO%d, PWM2→GPIO%d",
             ADC_PWM1_GPIO, ADC_PWM2_GPIO);
#endif

    /* Inisialisasi kalibrasi */
    cali_enabled = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_12, &adc_cali_handle);
}

/**
 * Inisialisasi PWM
 */
static void pwm_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
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
 * Baca tegangan dari ADC channel (dalam mV)
 * Menggunakan kalibrasi jika tersedia, jika tidak konversi manual.
 */
static float adc_read_voltage_mv(adc_channel_t channel)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &raw));

    if (cali_enabled && adc_cali_handle != NULL) {
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv);
        return (float)voltage_mv;
    } else {
        /* Fallback: konversi manual (atenuasi 12dB ~ 0-3.3V) */
        return (raw * 3300.0f) / 4095.0f;
    }
}

/**
 * Baca tegangan rata-rata dari ADC channel (num_avg sampel)
 */
static float adc_read_avg_mv(adc_channel_t channel, int num_avg)
{
    float sum = 0;
    for (int i = 0; i < num_avg; i++) {
        sum += adc_read_voltage_mv(channel);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return sum / num_avg;
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
    if (cali_enabled) {
        ESP_LOGI(TAG, "Kalibrasi ADC: AKTIF (hasil dalam mV terkalibrasi)");
    } else {
        ESP_LOGW(TAG, "Kalibrasi ADC: TIDAK AKTIF (konversi manual)");
    }
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
            int num_avg = 16;
            float mv_a, mv_b;
#if HAS_DAC
            mv_a = adc_read_avg_mv(ADC_DAC_CH, num_avg);
            mv_b = adc_read_avg_mv(ADC_PWM_CH, num_avg);
#else
            mv_a = adc_read_avg_mv(ADC_PWM1_CH, num_avg);
            mv_b = adc_read_avg_mv(ADC_PWM2_CH, num_avg);
#endif

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
