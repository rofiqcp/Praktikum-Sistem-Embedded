/*
 * ==========================================================================
 *  ESP32 DAC Waveform Output - Modul 08 Program 10
 * ==========================================================================
 *
 *  KONSEP DAC + DMA di ESP32:
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │ STM32:                                                         │
 *  │  - DAC peripheral + DMA channel terpisah                      │
 *  │  - DMA membaca waveform buffer → DAC_DHR register             │
 *  │  - Timer trigger untuk sample rate                             │
 *  │                                                                 │
 *  │ ESP32:                                                         │
 *  │  - DAC bisa di-drive via I2S peripheral dengan DMA             │
 *  │  - ESP-IDF 5.x: dac_continuous API (built-in DMA support)     │
 *  │  - Alternatif: dac_output_voltage() dalam task loop            │
 *  │  - I2S-DAC mode memberikan output DMA yang sesungguhnya       │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  Program ini:
 *  1. Pre-compute 4 jenis waveform (sine, square, triangle, sawtooth)
 *  2. Output via dac_continuous (DMA-backed) jika tersedia
 *  3. Fallback ke task loop jika API tidak tersedia
 *  4. Switch waveform setiap 5 detik
 *  5. Print info: tipe, frekuensi, Vpp
 *
 *  Hardware: GPIO25 → Oscilloscope / speaker
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/dac_oneshot.h"
#include "config.h"

/*
 * Catatan tentang ESP-IDF DAC API:
 *
 * ESP-IDF 5.x menyediakan dua mode DAC:
 * 1. dac_oneshot - output satu nilai pada satu waktu (seperti HAL_DAC_SetValue)
 * 2. dac_continuous - output stream data via DMA (seperti STM32 DAC + DMA)
 *
 * Untuk waveform generation yang ideal, dac_continuous lebih baik.
 * Namun dac_continuous memerlukan konfigurasi yang lebih kompleks
 * dan mungkin tidak tersedia di semua versi ESP-IDF.
 *
 * Program ini menggunakan pendekatan yang robust:
 * - Primary: dac_oneshot dalam tight loop (reliable, selalu tersedia)
 * - Penjelasan bagaimana dac_continuous (DMA) akan bekerja
 */

static const char *TAG = "DAC_WAVE";

/* ==========================================================================
 *  Waveform Lookup Tables
 * ========================================================================== */

static uint8_t sine_wave[WAVEFORM_BUF_SIZE];
static uint8_t square_wave[WAVEFORM_BUF_SIZE];
static uint8_t triangle_wave[WAVEFORM_BUF_SIZE];
static uint8_t sawtooth_wave[WAVEFORM_BUF_SIZE];
static uint8_t *waveform_buffers[NUM_WAVEFORMS];

static const char *waveform_names[NUM_WAVEFORMS] = {
    "SINE", "SQUARE", "TRIANGLE", "SAWTOOTH"
};

static volatile int current_waveform = WAVEFORM_SINE;
static volatile bool dac_running = false;

/* ---- DAC Handle ---- */
static dac_oneshot_handle_t dac_handle = NULL;

/* ---- Timing Stats ---- */
static int64_t samples_output = 0;
static int64_t output_start_time = 0;
static float actual_sample_rate = 0;
static float actual_freq = 0;

/* ==========================================================================
 *  Waveform Generation (Pre-compute)
 * ==========================================================================
 *  Pre-compute semua waveform dalam buffer.
 *  Di STM32, buffer ini akan di-copy oleh DMA ke DAC register.
 *  Di ESP32, buffer ini dibaca oleh task atau dac_continuous driver.
 * ========================================================================== */

static void generate_waveforms(void)
{
    ESP_LOGI(TAG, "Generating waveform lookup tables (%d points each)...",
             WAVEFORM_BUF_SIZE);

    for (int i = 0; i < WAVEFORM_BUF_SIZE; i++) {
        float phase = (float)i / WAVEFORM_BUF_SIZE;

        /* Sine wave: sin(2π * i / N), mapped to 0-255 */
        sine_wave[i] = (uint8_t)(127.5f + 127.5f *
                        sinf(2.0f * M_PI * phase));

        /* Square wave: 0 or 255 */
        square_wave[i] = (phase < 0.5f) ? 255 : 0;

        /* Triangle wave: linear ramp up then down */
        if (phase < 0.5f) {
            triangle_wave[i] = (uint8_t)(phase * 2.0f * 255.0f);
        } else {
            triangle_wave[i] = (uint8_t)((1.0f - phase) * 2.0f * 255.0f);
        }

        /* Sawtooth wave: linear ramp 0 to 255 */
        sawtooth_wave[i] = (uint8_t)(phase * 255.0f);
    }

    waveform_buffers[WAVEFORM_SINE] = sine_wave;
    waveform_buffers[WAVEFORM_SQUARE] = square_wave;
    waveform_buffers[WAVEFORM_TRIANGLE] = triangle_wave;
    waveform_buffers[WAVEFORM_SAWTOOTH] = sawtooth_wave;

    /* Print first few values of each */
    for (int w = 0; w < NUM_WAVEFORMS; w++) {
        printf("  %s: [%d, %d, %d, %d, ... , %d, %d]\n",
               waveform_names[w],
               waveform_buffers[w][0],
               waveform_buffers[w][1],
               waveform_buffers[w][2],
               waveform_buffers[w][3],
               waveform_buffers[w][WAVEFORM_BUF_SIZE - 2],
               waveform_buffers[w][WAVEFORM_BUF_SIZE - 1]);
    }
}

/* ==========================================================================
 *  DAC Initialization
 * ========================================================================== */

static esp_err_t dac_init(void)
{
    dac_oneshot_config_t config = {
        .chan_id = DAC_CHAN,
    };

    esp_err_t ret = dac_oneshot_new_channel(&config, &dac_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DAC init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "DAC initialized:");
    ESP_LOGI(TAG, "  Channel: DAC_CHAN_0 (GPIO%d)", DAC_GPIO);
    ESP_LOGI(TAG, "  Resolution: 8-bit (0-255)");
    ESP_LOGI(TAG, "  Voltage range: 0 - %.1fV", DAC_VDD);
    ESP_LOGI(TAG, "  Mode: oneshot (task-driven loop)");

    return ESP_OK;
}

/* ==========================================================================
 *  DAC Waveform Output Task
 * ==========================================================================
 *  Metode: Task loop dengan dac_oneshot_output_voltage()
 *
 *  Ini BUKAN true DMA output. True DMA memerlukan:
 *  - dac_continuous_new_channels() untuk setup DMA
 *  - dac_continuous_write_cyclically() untuk output loop
 *
 *  Metode task loop ini tetap berguna untuk:
 *  - Demonstrasi konsep waveform generation
 *  - Frekuensi rendah hingga menengah (<10 kHz)
 *  - Debugging dan prototyping
 *
 *  Untuk high-frequency output (>10 kHz):
 *  Gunakan dac_continuous API atau I2S-DAC mode
 * ========================================================================== */

static void dac_output_task(void *arg)
{
    ESP_LOGI(TAG, "DAC output task started (target freq: %d Hz)",
             WAVEFORM_FREQ_HZ);

    /*
     * Delay antar sample dihitung dari target frequency dan buffer size.
     * actual_delay = 1 / (freq * buffer_size)
     *
     * Catatan: vTaskDelay() memiliki resolusi terbatas (1 tick = 1ms default)
     * Untuk sample rate tinggi, kita gunakan esp_rom_delay_us() atau
     * tight loop tanpa delay.
     *
     * True DMA output (dac_continuous) tidak perlu delay karena
     * sample rate diatur oleh clock peripheral.
     */

    /* Calculate ideal delay per sample in microseconds */
    int delay_per_sample_us = 1000000 / (WAVEFORM_FREQ_HZ * WAVEFORM_BUF_SIZE);
    if (delay_per_sample_us < 1) delay_per_sample_us = 1;

    ESP_LOGI(TAG, "  Delay per sample: %d us", delay_per_sample_us);
    ESP_LOGI(TAG, "  Expected sample rate: %d Hz",
             1000000 / delay_per_sample_us);

    output_start_time = esp_timer_get_time();
    dac_running = true;

    while (1) {
        int waveform_idx = current_waveform;
        uint8_t *wave_buf = waveform_buffers[waveform_idx];

        /* Output one complete cycle */
        for (int i = 0; i < WAVEFORM_BUF_SIZE; i++) {
            dac_oneshot_output_voltage(dac_handle, wave_buf[i]);
            samples_output++;

            /* Precise delay using esp_rom_delay_us for better accuracy */
            if (delay_per_sample_us > 0) {
                esp_rom_delay_us(delay_per_sample_us);
            }
        }

        /* Calculate actual rate every 100 cycles */
        if (samples_output % (WAVEFORM_BUF_SIZE * 100) == 0) {
            int64_t elapsed = esp_timer_get_time() - output_start_time;
            if (elapsed > 0) {
                actual_sample_rate = (float)samples_output * 1000000.0f /
                                    (float)elapsed;
                actual_freq = actual_sample_rate / WAVEFORM_BUF_SIZE;
            }
        }

        /* Yield briefly to prevent watchdog trigger */
        if (samples_output % (WAVEFORM_BUF_SIZE * 50) == 0) {
            vTaskDelay(1);
        }
    }
}

/* ==========================================================================
 *  Waveform Switcher and Display Task
 * ========================================================================== */

static void display_task(void *arg)
{
    int switch_counter = 0;

    vTaskDelay(pdMS_TO_TICKS(2000));  /* Wait for output to stabilize */

    while (1) {
        /* Print current waveform info */
        float vpp = DAC_VDD;  /* Full range for all waveforms */
        float v_offset = 0;

        if (current_waveform == WAVEFORM_SINE) {
            vpp = DAC_VDD;
            v_offset = DAC_VDD / 2.0f;
        } else if (current_waveform == WAVEFORM_SQUARE) {
            vpp = DAC_VDD;
        }

        printf("\n");
        printf("╔══════════════════════════════════════════════════════╗\n");
        printf("║          DAC WAVEFORM OUTPUT STATUS                 ║\n");
        printf("╠══════════════════════════════════════════════════════╣\n");
        printf("║  Waveform:      %-10s                           ║\n",
               waveform_names[current_waveform]);
        printf("║  Target Freq:   %6d Hz                           ║\n",
               WAVEFORM_FREQ_HZ);
        printf("║  Actual Freq:   %8.1f Hz                         ║\n",
               actual_freq);
        printf("║  Sample Rate:   %8.0f Hz                         ║\n",
               actual_sample_rate);
        printf("║  Buffer Size:   %6d points                       ║\n",
               WAVEFORM_BUF_SIZE);
        printf("║  Vpp:           %6.2f V                            ║\n",
               vpp);
        printf("║  Resolution:    8-bit (%.1f mV/step)                ║\n",
               DAC_VDD * 1000.0f / 255.0f);
        printf("║  Total Samples: %lld                                ║\n",
               samples_output);
        printf("╠══════════════════════════════════════════════════════╣\n");

        /* ASCII waveform preview */
        printf("║  Waveform Preview:                                   ║\n");
        printf("║  ");

        uint8_t *buf = waveform_buffers[current_waveform];
        int preview_width = 50;
        int step = WAVEFORM_BUF_SIZE / preview_width;
        if (step < 1) step = 1;
        int height = 8;

        /* Print waveform row by row */
        for (int row = 0; row < height; row++) {
            int threshold = 255 - (row * 255 / (height - 1));
            printf("║  ");
            for (int col = 0; col < preview_width && col * step < WAVEFORM_BUF_SIZE; col++) {
                int val = buf[col * step];
                if (abs(val - threshold) < (255 / height / 2)) {
                    printf("*");
                } else {
                    printf(" ");
                }
            }
            printf("  ║\n");
        }

        printf("╠══════════════════════════════════════════════════════╣\n");
        printf("║  Output Mode: dac_oneshot (task-driven loop)        ║\n");
        printf("║                                                      ║\n");
        printf("║  Untuk TRUE DMA output, gunakan:                     ║\n");
        printf("║  • dac_continuous_new_channels()   [ESP-IDF 5.x]     ║\n");
        printf("║  • dac_continuous_write_cyclically()                  ║\n");
        printf("║  • Sample rate diatur oleh DMA clock                 ║\n");
        printf("║                                                      ║\n");
        printf("║  Di STM32: DAC + DMA + Timer trigger                 ║\n");
        printf("║  Di ESP32: DAC + I2S DMA (dac_continuous API)        ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n");

        /* Wait and count for waveform switch */
        for (int s = 0; s < WAVEFORM_SWITCH_SEC; s++) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            switch_counter++;

            if (s == WAVEFORM_SWITCH_SEC - 1) {
                /* Switch to next waveform */
                current_waveform = (current_waveform + 1) % NUM_WAVEFORMS;
                ESP_LOGI(TAG, "Switching to waveform: %s",
                         waveform_names[current_waveform]);
            }
        }
    }
}

/* ==========================================================================
 *  Explanation: dac_continuous (True DMA) Mode
 * ==========================================================================
 *  Jika menggunakan dac_continuous API (ESP-IDF 5.x):
 *
 *  #include "driver/dac_continuous.h"
 *
 *  dac_continuous_handle_t cont_handle;
 *  dac_continuous_config_t cont_cfg = {
 *      .chan_mask = DAC_CHANNEL_MASK_CH0,
 *      .desc_num = 4,
 *      .buf_size = 1024,
 *      .freq_hz = SAMPLE_RATE_HZ,
 *      .offset = 0,
 *      .clk_src = DAC_DIGI_CLK_SRC_APLL,  // APLL for precise frequency
 *      .chan_mode = DAC_CHANNEL_MODE_SIMUL,
 *  };
 *
 *  dac_continuous_new_channels(&cont_cfg, &cont_handle);
 *  dac_continuous_enable(cont_handle);
 *
 *  // Output waveform cyclically via DMA - CPU free!
 *  dac_continuous_write_cyclically(cont_handle, sine_wave,
 *                                   WAVEFORM_BUF_SIZE, NULL);
 *
 *  Keuntungan dac_continuous:
 *  - Sample rate presisi (APLL clock)
 *  - CPU completely free selama output
 *  - DMA otomatis loop buffer
 *  - Mirip dengan STM32 DAC + DMA + Timer
 * ========================================================================== */

/* ==========================================================================
 *  Main Application
 * ========================================================================== */

void app_main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║   ESP32 DAC Waveform Output - Modul 08 Program 10  ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  DAC output via task loop (dac_oneshot)             ║\n");
    printf("║  True DMA: dac_continuous API (lihat kode)          ║\n");
    printf("║                                                      ║\n");
    printf("║  GPIO25 → DAC output (8-bit, 0-3.3V)               ║\n");
    printf("║  Waveforms: Sine, Square, Triangle, Sawtooth        ║\n");
    printf("║  Auto-switch setiap %d detik                        ║\n",
           WAVEFORM_SWITCH_SEC);
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    /* Generate waveform lookup tables */
    generate_waveforms();

    /* Initialize DAC */
    esp_err_t ret = dac_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DAC init failed!");
        return;
    }

    /* Print frequency calculation */
    float ideal_sample_rate = (float)WAVEFORM_FREQ_HZ * WAVEFORM_BUF_SIZE;
    printf("\n  Frequency Calculation:\n");
    printf("  ─────────────────────────────────\n");
    printf("  Target freq:     %d Hz\n", WAVEFORM_FREQ_HZ);
    printf("  Buffer size:     %d points\n", WAVEFORM_BUF_SIZE);
    printf("  Ideal sample rate: %.0f Hz\n", ideal_sample_rate);
    printf("  Sample period:   %.1f us\n",
           1000000.0f / ideal_sample_rate);
    printf("  f_out = sample_rate / buffer_size\n");
    printf("  f_out = %.0f / %d = %d Hz\n\n",
           ideal_sample_rate, WAVEFORM_BUF_SIZE, WAVEFORM_FREQ_HZ);

    /* Create DAC output task (high priority for timing accuracy) */
    xTaskCreatePinnedToCore(dac_output_task, "dac_out",
                            DAC_TASK_STACK, NULL,
                            DAC_TASK_PRIO, NULL, 1);

    /* Create display task */
    xTaskCreate(display_task, "display",
                DISPLAY_TASK_STACK, NULL,
                DISPLAY_TASK_PRIO, NULL);

    ESP_LOGI(TAG, "DAC waveform output started!");
}
