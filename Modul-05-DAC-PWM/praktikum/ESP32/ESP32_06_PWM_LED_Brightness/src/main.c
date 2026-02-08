/**
 * ==========================================================================
 * PROGRAM 06: PWM LED Brightness (Kontrol Kecerahan LED via Serial)
 * ==========================================================================
 * Modul 05 - DAC & PWM | Praktikum Sistem Embedded
 *
 * Deskripsi:
 *   Mengontrol kecerahan LED menggunakan LEDC PWM. Pengguna memasukkan
 *   nilai persentase (0-100) melalui serial monitor, yang kemudian
 *   dikonversi menjadi duty cycle PWM.
 *
 * Koneksi Hardware:
 *   - ESP32: LED built-in pada GPIO2
 *   - Atau LED eksternal + resistor 220Ω pada GPIO2
 *
 * Pin Mapping:
 *   ESP32 DevKit V1 : GPIO2
 *   Lolin S2 Mini   : GPIO15
 *   ESP32-S3        : GPIO2
 *
 * Penggunaan:
 *   - Buka Serial Monitor (115200 baud)
 *   - Ketik angka 0-100 lalu Enter
 *   - LED akan berubah kecerahan sesuai persentase
 *
 * API yang digunakan:
 *   - ledc_timer_config()    : Konfigurasi timer PWM
 *   - ledc_channel_config()  : Konfigurasi channel PWM
 *   - ledc_set_duty()        : Set duty cycle
 *   - ledc_update_duty()     : Terapkan duty cycle baru
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "PWM_BRIGHT";

/* Konfigurasi pin LED */
#if CONFIG_IDF_TARGET_ESP32
    #define LED_PIN  2
#elif CONFIG_IDF_TARGET_ESP32S2
    #define LED_PIN  15
#elif CONFIG_IDF_TARGET_ESP32S3
    #define LED_PIN  2
#else
    #define LED_PIN  2
#endif

/* Konfigurasi LEDC PWM */
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CH          LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_13_BIT   /* Resolusi 13-bit */
#define LEDC_FREQUENCY   5000                 /* 5kHz */
#define LEDC_MAX_DUTY    ((1 << 13) - 1)     /* 8191 */

/* Konfigurasi UART */
#define UART_NUM         UART_NUM_0
#define UART_BUF_SIZE    256

/**
 * Konversi persentase (0-100) ke duty cycle (0-LEDC_MAX_DUTY)
 */
static uint32_t percentage_to_duty(int percentage)
{
    if (percentage <= 0) return 0;
    if (percentage >= 100) return LEDC_MAX_DUTY;
    return (uint32_t)((LEDC_MAX_DUTY * percentage) / 100);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 PWM LED Brightness Control ===");
    ESP_LOGI(TAG, "LED Pin: GPIO%d", LED_PIN);

    /* Konfigurasi UART untuk membaca input */
    uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);

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
        .gpio_num   = LED_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CH,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    ESP_LOGI(TAG, "LEDC dikonfigurasi (5kHz, 13-bit)");
    ESP_LOGI(TAG, "-------------------------------------------");
    ESP_LOGI(TAG, "Masukkan persentase kecerahan (0-100):");
    ESP_LOGI(TAG, "Contoh: ketik '75' untuk 75%% kecerahan");
    ESP_LOGI(TAG, "-------------------------------------------");

    /* Buffer untuk membaca input serial */
    uint8_t rx_buf[UART_BUF_SIZE];
    char line_buf[64];
    int line_pos = 0;
    int current_pct = 0;

    while (1) {
        /* Baca data dari UART */
        int len = uart_read_bytes(UART_NUM, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                char c = (char)rx_buf[i];

                /* Deteksi akhir baris (Enter) */
                if (c == '\n' || c == '\r') {
                    if (line_pos > 0) {
                        line_buf[line_pos] = '\0';

                        /* Parsing input sebagai angka */
                        int pct = atoi(line_buf);

                        /* Validasi range 0-100 */
                        if (pct < 0) pct = 0;
                        if (pct > 100) pct = 100;

                        current_pct = pct;

                        /* Konversi ke duty cycle dan terapkan */
                        uint32_t duty = percentage_to_duty(pct);
                        ledc_set_duty(LEDC_MODE, LEDC_CH, duty);
                        ledc_update_duty(LEDC_MODE, LEDC_CH);

                        /* Tampilkan informasi */
                        printf("DATA:%d,%lu\n", pct, (unsigned long)duty);
                        ESP_LOGI(TAG, "Kecerahan: %d%% | Duty: %lu/%d",
                                 pct, (unsigned long)duty, LEDC_MAX_DUTY);

                        line_pos = 0;
                    }
                } else if (line_pos < (int)(sizeof(line_buf) - 1)) {
                    line_buf[line_pos++] = c;
                }
            }
        }

        /* Tampilkan status periodik setiap 5 detik */
        static int tick_count = 0;
        tick_count++;
        if (tick_count >= 50) {  /* 50 * 100ms = 5 detik */
            tick_count = 0;
            uint32_t duty = percentage_to_duty(current_pct);
            printf("DATA:%d,%lu\n", current_pct, (unsigned long)duty);
            ESP_LOGI(TAG, "[Status] Kecerahan saat ini: %d%% | Duty: %lu",
                     current_pct, (unsigned long)duty);
        }
    }
}
