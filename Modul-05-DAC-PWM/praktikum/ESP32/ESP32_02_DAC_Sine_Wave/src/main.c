/**
 * ==========================================================================
 * PROGRAM 02: DAC Sine Wave (Gelombang Sinus DAC)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Menghasilkan gelombang sinus menggunakan lookup table (256 titik)
 *   yang di-output melalui DAC. Timer periodik (esp_timer) digunakan
 *   untuk memperbarui nilai DAC pada interval tetap.
 *   Frekuensi output = 1 / (256 * periode_timer)
 *
 *   CATATAN ISR-SAFETY:
 *   dac_output_voltage() TIDAK ISR-safe, sehingga tidak boleh dipanggil
 *   langsung dari callback timer (konteks ISR). Solusinya: ISR mengirim
 *   notifikasi ke task FreeRTOS berprioritas tinggi yang melakukan
 *   output DAC yang sebenarnya.
 *
 * Koneksi Hardware:
 *   - ESP32: DAC1 = GPIO25 (output analog)
 *   - Oscilloscope pada GPIO25 untuk melihat gelombang
 *   - ESP32-S2/S3: GPIO18 (PWM + RC filter)
 *
 * API yang digunakan:
 *   - dac_output_enable()        : Mengaktifkan DAC
 *   - dac_output_voltage()       : Set nilai DAC (dari task, BUKAN ISR)
 *   - esp_timer_create()         : Membuat timer periodik
 *   - esp_timer_start_periodic() : Memulai timer
 *   - xTaskNotifyFromISR()       : Notifikasi task dari ISR (ISR-safe)
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "DAC_SINE";

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
    #define PWM_FREQ    50000          /* 50kHz carrier untuk smoothing */
    #define PWM_RES     LEDC_TIMER_8_BIT
#endif

/* Ukuran tabel lookup sinus */
#define SINE_TABLE_SIZE  256

/* Frekuensi gelombang sinus target (Hz) */
#define SINE_FREQ_HZ     100

/*
 * Periode timer dalam mikrodetik
 * Periode = 1.000.000 / (SINE_TABLE_SIZE * SINE_FREQ_HZ)
 * Contoh: 1.000.000 / (256 * 100) = ~39 us
 */
#define TIMER_PERIOD_US  (1000000 / (SINE_TABLE_SIZE * SINE_FREQ_HZ))

/* Tabel lookup sinus (diisi saat inisialisasi) */
static uint8_t sine_table[SINE_TABLE_SIZE];

/* Indeks posisi saat ini pada tabel */
static volatile uint32_t sine_index = 0;

/* Penghitung siklus untuk pelaporan */
static volatile uint32_t cycle_count = 0;

/* Handle task DAC output - task ini melakukan output DAC yang sebenarnya */
static TaskHandle_t dac_task_handle = NULL;

/**
 * Membuat tabel lookup sinus 256 titik
 * Nilai dipetakan ke range 0-255 untuk DAC 8-bit
 */
static void generate_sine_table(void)
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        /* sin() menghasilkan -1.0 sampai 1.0 */
        /* Pemetaan ke 0-255: (sin + 1) / 2 * 255 */
        float rad = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        sine_table[i] = (uint8_t)((sinf(rad) + 1.0f) * 127.5f);
    }
    ESP_LOGI(TAG, "Tabel sinus dibuat: %d titik", SINE_TABLE_SIZE);
}

/**
 * Callback timer - dipanggil secara periodik untuk memicu update DAC
 *
 * PENTING: Fungsi ini berjalan di konteks ISR, sehingga TIDAK boleh
 * memanggil dac_output_voltage() secara langsung (tidak ISR-safe).
 * Sebagai gantinya, kita mengirim notifikasi ke task DAC.
 */
static void IRAM_ATTR timer_callback(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Kirim notifikasi ke task DAC agar segera output nilai berikutnya */
    vTaskNotifyGiveFromISR(dac_task_handle, &xHigherPriorityTaskWoken);

    /* Jika task berprioritas lebih tinggi terbangun, minta context switch */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * Task DAC output - berjalan di konteks task (BUKAN ISR)
 * Menunggu notifikasi dari timer ISR, lalu melakukan output DAC
 *
 * Task ini berprioritas tinggi (configMAX_PRIORITIES - 1) agar
 * segera dijalankan setelah ISR selesai, menjaga timing yang akurat.
 */
static void dac_output_task(void *arg)
{
    while (1) {
        /* Tunggu notifikasi dari timer ISR */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Ambil nilai dari tabel sinus dan output ke DAC */
        uint8_t value = sine_table[sine_index];

#if HAS_DAC
        dac_output_voltage(DAC_CHAN, value);
#else
        ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, value);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
#endif

        /* Update indeks tabel sinus */
        sine_index++;
        if (sine_index >= SINE_TABLE_SIZE) {
            sine_index = 0;
            cycle_count++;
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
    ESP_LOGI(TAG, "=== ESP32 DAC Sine Wave Generator ===");

    /* Buat tabel lookup sinus */
    generate_sine_table();

#if HAS_DAC
    /* Aktifkan DAC */
    dac_output_enable(DAC_CHAN);
    ESP_LOGI(TAG, "DAC aktif pada GPIO%d", DAC_GPIO);
#else
    pwm_init();
#endif

    ESP_LOGI(TAG, "Frekuensi target : %d Hz", SINE_FREQ_HZ);
    ESP_LOGI(TAG, "Periode timer    : %d us", TIMER_PERIOD_US);
    ESP_LOGI(TAG, "Titik per siklus : %d", SINE_TABLE_SIZE);

    /*
     * Buat task DAC output dengan prioritas tinggi.
     * Task ini menunggu notifikasi dari ISR timer dan melakukan
     * output DAC yang sebenarnya (karena dac_output_voltage()
     * TIDAK ISR-safe).
     */
    xTaskCreate(dac_output_task, "dac_out", 2048,
                NULL, configMAX_PRIORITIES - 1, &dac_task_handle);
    ESP_LOGI(TAG, "Task DAC output dibuat (prioritas tinggi)");

    /* Buat dan mulai timer periodik */
    esp_timer_handle_t timer_handle;
    esp_timer_create_args_t timer_args = {
        .callback = timer_callback,
        .name = "sine_timer",
    };
    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, TIMER_PERIOD_US);

    ESP_LOGI(TAG, "Timer dimulai - Gelombang sinus berjalan");

    /* Loop pelaporan status */
    uint32_t last_cycle = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        uint32_t current_cycle = cycle_count;
        uint32_t freq_actual = current_cycle - last_cycle;
        last_cycle = current_cycle;

        /* Format: DATA:<siklus_total>,<frekuensi_aktual>,<indeks_saat_ini> */
        printf("DATA:%lu,%lu,%lu\n",
               (unsigned long)current_cycle,
               (unsigned long)freq_actual,
               (unsigned long)sine_index);
        ESP_LOGI(TAG, "Siklus: %lu | Frekuensi aktual: ~%lu Hz | Index: %lu",
                 (unsigned long)current_cycle,
                 (unsigned long)freq_actual,
                 (unsigned long)sine_index);
    }
}
