/**
 * ==========================================================================
 *  ESP32_06_Toggle_Latch — Toggle LED dengan Edge Detection
 * ==========================================================================
 *  Modul   : 01 - GPIO Digital I/O
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF (PlatformIO)
 *
 *  Deskripsi:
 *    Program mendeteksi rising edge pada push button (transisi dari
 *    ditekan ke dilepas) dan men-toggle (latch) LED setiap kali
 *    edge terdeteksi.
 *
 *    Logika Edge Detection:
 *      - Simpan state button sebelumnya (prev) dan saat ini (curr)
 *      - Rising edge: prev=0 (ditekan), curr=1 (dilepas)
 *      - Pada setiap rising edge → toggle LED
 *      - Software debounce menghindari multiple-toggle akibat bouncing
 *
 *    Catatan: Karena button active-LOW, "rising edge" di sini berarti
 *    tombol DILEPAS. Kita mendeteksi saat button berpindah dari
 *    pressed (0) ke released (1) — yaitu saat jari diangkat.
 *
 *  Rangkaian / Wiring:
 *    Button:
 *      ESP32 GPIO4 ──┬── Resistor 10kΩ ── VCC 3.3V  (pull-up)
 *                     └── Push Button ── GND
 *      (Ditekan = LOW/0, Dilepas = HIGH/1)
 *
 *    LED:
 *      ESP32 GPIO2 ──► Resistor 220Ω ──► LED Anoda | Katoda ──► GND
 *
 *  Komponen:
 *    - 1x Push button
 *    - 1x Resistor 10kΩ (pull-up)
 *    - 1x LED
 *    - 1x Resistor 220Ω
 *    - Breadboard + kabel jumper
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "TOGGLE_LATCH";

/**
 * @brief Inisialisasi GPIO untuk button dan LED
 */
static void gpio_init_all(void)
{
    /* Konfigurasi LED sebagai output */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

    /* Konfigurasi Button sebagai input dengan pull-up */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    ESP_LOGI(TAG, "GPIO terinitialisasi — Button:GPIO%d  LED:GPIO%d", BUTTON_PIN, LED_PIN);
}

/**
 * @brief Baca button dengan debounce sederhana
 * @return Nilai stabil button (0 atau 1)
 */
static int read_button_debounced(void)
{
    static int stable_state = 1;        /* Stabil terakhir (1=dilepas) */
    static int last_raw = 1;            /* Pembacaan raw sebelumnya */
    static int64_t last_change_ms = 0;  /* Waktu perubahan terakhir */

    int raw = gpio_get_level(BUTTON_PIN);
    int64_t now = esp_timer_get_time() / 1000;

    if (raw != last_raw) {
        /* Nilai berubah → reset timer debounce */
        last_change_ms = now;
        last_raw = raw;
    } else if ((now - last_change_ms) >= DEBOUNCE_MS) {
        /* Nilai stabil selama DEBOUNCE_MS → update stable_state */
        stable_state = raw;
    }

    return stable_state;
}

/**
 * @brief Entry point utama
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Toggle Latch (Edge Detection) ===");
    ESP_LOGI(TAG, "Tekan tombol untuk toggle LED ON/OFF");
    gpio_init_all();

    int prev_state = 1;     /* State sebelumnya (1=dilepas, karena pull-up) */
    bool led_state = false;
    int toggle_count = 0;

    while (1) {
        int curr_state = read_button_debounced();

        /*
         * Deteksi rising edge: prev=0 (ditekan), curr=1 (dilepas)
         * Toggle dilakukan saat button DILEPAS untuk menghindari
         * aksi ganda jika button ditahan.
         */
        if (prev_state == 0 && curr_state == 1) {
            /* Rising edge terdeteksi → toggle LED */
            led_state = !led_state;
            gpio_set_level(LED_PIN, (uint32_t)led_state);
            toggle_count++;

            ESP_LOGI(TAG, "Edge #%d — LED %s",
                     toggle_count, led_state ? "ON" : "OFF");
        }

        /* Simpan state untuk iterasi berikutnya */
        prev_state = curr_state;

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
