/**
 * @file main.c
 * @brief Program 09 - Input Capture (Pulse Width Measurement)
 * @details Mengukur lebar pulsa (pulse width) pada pin input menggunakan
 *          GPIO interrupt + esp_timer. Rising edge mencatat timestamp awal,
 *          falling edge menghitung selisih = lebar pulsa.
 *
 * @modul    Modul 02 - Interrupt & Timer
 * @board    ESP32 DOIT DevKit V1 / Lolin S2 Mini / ESP32-S3
 * @framework ESP-IDF
 *
 * @koneksi Hardware:
 *   ESP32 GPIO4  <-- Push button --> GND (dengan pull-up internal)
 *   ESP32 GPIO2  --> LED (+) --> 220Ω --> GND
 *   (Opsional: signal generator ke GPIO4 untuk pulse presisi)
 *
 * @cara_kerja:
 *   1. GPIO4 dikonfigurasi interrupt ANYEDGE (rising + falling)
 *   2. Rising edge: catat timestamp start (esp_timer_get_time)
 *   3. Falling edge: hitung pulse_width = now - start
 *   4. Main loop menampilkan hasil pengukuran pulse width
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "INPUT_CAPTURE";

/* Variabel shared dengan ISR */
static volatile int64_t rise_time_us = 0;
static volatile int64_t pulse_width_us = 0;
static volatile bool new_measurement = false;
static volatile uint32_t capture_count = 0;
static volatile bool capturing = false;

/**
 * @brief ISR handler untuk GPIO input capture
 * Detect rising edge (start capture) dan falling edge (end capture)
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();
    int level = gpio_get_level(INPUT_CAPTURE_PIN);

    if (level == 1) {
        /* Rising edge - mulai capture */
        rise_time_us = now;
        capturing = true;
    } else if (capturing) {
        /* Falling edge - selesai capture */
        int64_t width = now - rise_time_us;
        if (width >= MIN_PULSE_US && width <= MAX_PULSE_US) {
            pulse_width_us = width;
            new_measurement = true;
            capture_count++;
        }
        capturing = false;
    }
}

/**
 * @brief Inisialisasi GPIO
 */
static void gpio_init(void)
{
    /* LED output */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_STATUS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_conf));

    /* Input capture pin - interrupt on any edge */
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << INPUT_CAPTURE_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_conf));

    /* Install ISR service dan register handler */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(INPUT_CAPTURE_PIN, gpio_isr_handler, NULL));
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Input Capture - Pulse Width Measurement ===");
    ESP_LOGI(TAG, "Input pin: GPIO%d (press button or apply signal)", INPUT_CAPTURE_PIN);
    ESP_LOGI(TAG, "Valid range: %d us - %d us", MIN_PULSE_US, MAX_PULSE_US);

    gpio_init();
    gpio_set_level(LED_STATUS_PIN, 0);

    ESP_LOGI(TAG, "Waiting for pulse on GPIO%d...", INPUT_CAPTURE_PIN);

    while (1) {
        if (new_measurement) {
            new_measurement = false;

            /* Toggle LED setiap ada measurement baru */
            static bool led_state = false;
            led_state = !led_state;
            gpio_set_level(LED_STATUS_PIN, led_state ? 1 : 0);

            /* Konversi ke unit yang sesuai */
            int64_t width = pulse_width_us;
            if (width < 10000) {
                ESP_LOGI(TAG, "Pulse #%lu: %lld us (%.2f ms)",
                         (unsigned long)capture_count,
                         (long long)width,
                         width / 1000.0);
            } else {
                ESP_LOGI(TAG, "Pulse #%lu: %.2f ms (%.3f s)",
                         (unsigned long)capture_count,
                         width / 1000.0,
                         width / 1000000.0);
            }

            /* Estimasi frekuensi jika pulse periodik */
            if (width > 0) {
                double freq = 1000000.0 / (width * 2.0); /* approximate */
                ESP_LOGI(TAG, "  Estimated freq (if periodic): %.2f Hz", freq);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
