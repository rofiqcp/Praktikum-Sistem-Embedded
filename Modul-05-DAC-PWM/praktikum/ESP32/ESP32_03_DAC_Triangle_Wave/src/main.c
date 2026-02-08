/**
 * ==========================================================================
 * PROGRAM 03: DAC Triangle Wave (Gelombang Segitiga DAC)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Menghasilkan gelombang segitiga: naik dari 0→255 lalu turun 255→0
 *   secara terus-menerus. Frekuensi dapat dikonfigurasi melalui
 *   periode timer. Arah dan nilai saat ini ditampilkan di serial.
 *
 * Koneksi Hardware:
 *   - ESP32: DAC1 = GPIO25 (output analog)
 *   - Oscilloscope pada GPIO25 untuk melihat gelombang
 *   - ESP32-S2/S3: GPIO18 (PWM + RC filter)
 *
 * API yang digunakan:
 *   - dac_output_enable()    : Mengaktifkan DAC
 *   - dac_output_voltage()   : Set nilai DAC
 *   - esp_timer              : Timer periodik
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "DAC_TRIANGLE";

/* Deteksi ketersediaan DAC */
#if CONFIG_IDF_TARGET_ESP32
    #define HAS_DAC 1
    #include "driver/dac.h"
    #define DAC_CHAN   DAC_CHANNEL_1
    #define DAC_GPIO  25
#else
    #define HAS_DAC 0
    #include "driver/ledc.h"
    #define PWM_GPIO    18
    #define PWM_TIMER   LEDC_TIMER_0
    #define PWM_CHANNEL LEDC_CHANNEL_0
    #define PWM_FREQ    50000
    #define PWM_RES     LEDC_TIMER_8_BIT
#endif

/* Frekuensi gelombang segitiga (Hz) */
#define TRIANGLE_FREQ_HZ  50

/*
 * Periode timer dalam mikrodetik
 * Satu siklus penuh = 512 langkah (256 naik + 256 turun)
 * Periode = 1.000.000 / (512 * TRIANGLE_FREQ_HZ)
 */
#define TIMER_PERIOD_US  (1000000 / (512 * TRIANGLE_FREQ_HZ))

/* Variabel status gelombang */
static volatile uint8_t current_value = 0;
static volatile int8_t direction = 1;       /* 1 = naik, -1 = turun */
static volatile uint32_t cycle_count = 0;

/**
 * Callback timer - update nilai DAC setiap periode
 */
static void IRAM_ATTR timer_callback(void *arg)
{
#if HAS_DAC
    dac_output_voltage(DAC_CHAN, current_value);
#else
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, current_value);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
#endif

    /* Update nilai berdasarkan arah */
    if (direction == 1) {
        if (current_value == 255) {
            direction = -1;  /* Balik arah ke turun */
        } else {
            current_value++;
        }
    } else {
        if (current_value == 0) {
            direction = 1;   /* Balik arah ke naik */
            cycle_count++;   /* Satu siklus penuh selesai */
        } else {
            current_value--;
        }
    }
}

#if !HAS_DAC
/**
 * Inisialisasi LEDC PWM sebagai pengganti DAC
 */
static void pwm_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num        = PWM_TIMER,
        .duty_resolution = PWM_RES,
        .freq_hz         = PWM_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {
        .gpio_num   = PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_CHANNEL,
        .timer_sel  = PWM_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch_conf);

    ESP_LOGW(TAG, "DAC tidak tersedia! Menggunakan PWM pada GPIO%d", PWM_GPIO);
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 DAC Triangle Wave Generator ===");

#if HAS_DAC
    dac_output_enable(DAC_CHAN);
    ESP_LOGI(TAG, "DAC aktif pada GPIO%d", DAC_GPIO);
#else
    pwm_init();
#endif

    ESP_LOGI(TAG, "Frekuensi target : %d Hz", TRIANGLE_FREQ_HZ);
    ESP_LOGI(TAG, "Periode timer    : %d us", TIMER_PERIOD_US);
    ESP_LOGI(TAG, "Langkah/siklus   : 512 (256 naik + 256 turun)");

    /* Buat dan mulai timer periodik */
    esp_timer_handle_t timer_handle;
    esp_timer_create_args_t timer_args = {
        .callback = timer_callback,
        .name = "triangle_timer",
    };
    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, TIMER_PERIOD_US);

    ESP_LOGI(TAG, "Timer dimulai - Gelombang segitiga berjalan");

    /* Loop pelaporan status */
    uint32_t last_cycle = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(500));

        uint32_t current_cycle = cycle_count;
        const char *dir_str = (direction == 1) ? "NAIK" : "TURUN";

        /* Format: DATA:<nilai>,<arah>,<siklus> */
        printf("DATA:%d,%s,%lu\n", current_value, dir_str,
               (unsigned long)current_cycle);
        ESP_LOGI(TAG, "Nilai: %3d/255 | Arah: %s | Siklus: %lu",
                 current_value, dir_str, (unsigned long)current_cycle);

        /* Hitung frekuensi aktual setiap detik */
        if (current_cycle != last_cycle) {
            uint32_t freq = (current_cycle - last_cycle) * 2;
            ESP_LOGI(TAG, "Frekuensi aktual: ~%lu Hz", (unsigned long)freq);
            last_cycle = current_cycle;
        }
    }
}
