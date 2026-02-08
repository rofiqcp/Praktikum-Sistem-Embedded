/**
 * ==========================================================================
 * PROGRAM 04: DAC Audio Tone (Nada Audio DAC)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Menghasilkan nada audio 440Hz (A4) dan 880Hz (A5) menggunakan DAC.
 *   Gelombang sinus di-output ke speaker/buzzer yang terhubung ke pin DAC.
 *   Timer berkecepatan tinggi (~8kHz sample rate) digunakan untuk
 *   menghasilkan sinyal audio yang halus.
 *
 *   CATATAN ISR-SAFETY:
 *   dac_output_voltage() TIDAK ISR-safe, sehingga tidak boleh dipanggil
 *   langsung dari callback timer (konteks ISR). Solusinya: ISR mengirim
 *   notifikasi ke task FreeRTOS berprioritas tinggi yang melakukan
 *   output DAC yang sebenarnya.
 *
 * Koneksi Hardware:
 *   - ESP32: DAC1 = GPIO25 → Speaker/Buzzer (melalui amplifier)
 *   - GND speaker → GND ESP32
 *   - ESP32-S2/S3: GPIO18 (PWM) → Speaker/Buzzer
 *
 * API yang digunakan:
 *   - dac_output_voltage()       : Output nilai analog (dari task, BUKAN ISR)
 *   - esp_timer                  : Timer berkecepatan tinggi
 *   - xTaskNotifyFromISR()       : Notifikasi task dari ISR (ISR-safe)
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "DAC_AUDIO";

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

/* Konfigurasi audio */
#define SAMPLE_RATE      8000    /* Sample rate 8kHz */
#define TIMER_PERIOD_US  (1000000 / SAMPLE_RATE)  /* ~125 us */

/* Frekuensi nada yang tersedia */
#define TONE_A4  440    /* Nada A4 = 440 Hz */
#define TONE_A5  880    /* Nada A5 = 880 Hz (satu oktaf di atas A4) */

/* Tabel lookup sinus (256 titik) */
#define SINE_TABLE_SIZE 256
static uint8_t sine_table[SINE_TABLE_SIZE];

/* Status generator nada */
static volatile float phase_accumulator = 0.0f;
static volatile float phase_increment = 0.0f;
static volatile bool tone_active = true;

/* Handle task DAC output - task ini melakukan output DAC yang sebenarnya */
static TaskHandle_t dac_task_handle = NULL;

/**
 * Membuat tabel lookup sinus
 */
static void generate_sine_table(void)
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        float rad = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        sine_table[i] = (uint8_t)((sinf(rad) + 1.0f) * 127.5f);
    }
}

/**
 * Mengatur frekuensi nada yang dihasilkan
 * phase_increment = (frekuensi * SINE_TABLE_SIZE) / SAMPLE_RATE
 */
static void set_tone_frequency(float freq_hz)
{
    phase_increment = (freq_hz * SINE_TABLE_SIZE) / (float)SAMPLE_RATE;
    ESP_LOGI(TAG, "Frekuensi diatur ke: %.0f Hz (increment: %.2f)",
             freq_hz, phase_increment);
}

/**
 * Callback timer audio - dipanggil setiap periode sampling
 *
 * PENTING: Fungsi ini berjalan di konteks ISR, sehingga TIDAK boleh
 * memanggil dac_output_voltage() secara langsung (tidak ISR-safe).
 * Sebagai gantinya, kita mengirim notifikasi ke task DAC.
 */
static void IRAM_ATTR audio_timer_callback(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Kirim notifikasi ke task DAC agar segera output nilai berikutnya */
    vTaskNotifyGiveFromISR(dac_task_handle, &xHigherPriorityTaskWoken);

    /* Jika task berprioritas lebih tinggi terbangun, minta context switch */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * Task DAC output - berjalan di konteks task (BUKAN ISR)
 * Menunggu notifikasi dari timer ISR, lalu melakukan output DAC.
 * Menggunakan phase accumulator untuk menghasilkan frekuensi yang tepat.
 *
 * Task ini berprioritas tinggi (configMAX_PRIORITIES - 1) agar
 * segera dijalankan setelah ISR selesai, menjaga timing audio yang akurat.
 */
static void dac_output_task(void *arg)
{
    while (1) {
        /* Tunggu notifikasi dari timer ISR */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (!tone_active) {
            /* Senyap: output nilai tengah */
#if HAS_DAC
            dac_output_voltage(DAC_CHAN, 128);
#else
            ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, 128);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
#endif
            continue;
        }

        /* Ambil nilai dari tabel sinus berdasarkan fase */
        uint8_t index = (uint8_t)((int)phase_accumulator % SINE_TABLE_SIZE);
        uint8_t value = sine_table[index];

#if HAS_DAC
        dac_output_voltage(DAC_CHAN, value);
#else
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, value);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
#endif

        /* Update phase accumulator */
        phase_accumulator += phase_increment;
        if (phase_accumulator >= SINE_TABLE_SIZE) {
            phase_accumulator -= SINE_TABLE_SIZE;
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
    ESP_LOGI(TAG, "=== ESP32 DAC Audio Tone Generator ===");

    /* Buat tabel sinus */
    generate_sine_table();

#if HAS_DAC
    dac_output_enable(DAC_CHAN);
    ESP_LOGI(TAG, "DAC aktif pada GPIO%d", DAC_GPIO);
#else
    pwm_init();
#endif

    ESP_LOGI(TAG, "Sample rate : %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "Periode     : %d us", TIMER_PERIOD_US);

    /*
     * Buat task DAC output dengan prioritas tinggi.
     * Task ini menunggu notifikasi dari ISR timer dan melakukan
     * output DAC yang sebenarnya (karena dac_output_voltage()
     * TIDAK ISR-safe).
     */
    xTaskCreate(dac_output_task, "dac_out", 2048,
                NULL, configMAX_PRIORITIES - 1, &dac_task_handle);
    ESP_LOGI(TAG, "Task DAC output dibuat (prioritas tinggi)");

    /* Buat dan mulai timer audio */
    esp_timer_handle_t audio_timer;
    esp_timer_create_args_t timer_args = {
        .callback = audio_timer_callback,
        .name = "audio_timer",
    };
    esp_timer_create(&timer_args, &audio_timer);
    esp_timer_start_periodic(audio_timer, TIMER_PERIOD_US);

    ESP_LOGI(TAG, "Timer audio dimulai");
    ESP_LOGI(TAG, "Pola: A4(440Hz) 2s → Senyap 1s → A5(880Hz) 2s → Senyap 1s");

    int iteration = 0;
    while (1) {
        /* Mainkan nada A4 (440 Hz) selama 2 detik */
        set_tone_frequency(TONE_A4);
        tone_active = true;
        printf("DATA:A4,%d,%d\n", TONE_A4, iteration);
        ESP_LOGI(TAG, "♪ Memainkan A4 (%d Hz)...", TONE_A4);
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* Jeda senyap 1 detik */
        tone_active = false;
        printf("DATA:SILENT,0,%d\n", iteration);
        ESP_LOGI(TAG, "... Senyap ...");
        vTaskDelay(pdMS_TO_TICKS(1000));

        /* Mainkan nada A5 (880 Hz) selama 2 detik */
        set_tone_frequency(TONE_A5);
        tone_active = true;
        printf("DATA:A5,%d,%d\n", TONE_A5, iteration);
        ESP_LOGI(TAG, "♪ Memainkan A5 (%d Hz)...", TONE_A5);
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* Jeda senyap 1 detik */
        tone_active = false;
        printf("DATA:SILENT,0,%d\n", iteration);
        ESP_LOGI(TAG, "... Senyap ...");
        vTaskDelay(pdMS_TO_TICKS(1000));

        iteration++;
        ESP_LOGI(TAG, "--- Iterasi %d selesai ---", iteration);
    }
}
