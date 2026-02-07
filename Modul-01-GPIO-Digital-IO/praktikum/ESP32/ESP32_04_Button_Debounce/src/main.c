/**
 * ==========================================================================
 *  ESP32_04_Button_Debounce — Software Debounce dengan State Machine
 * ==========================================================================
 *  Modul   : 01 - GPIO Digital I/O
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF (PlatformIO)
 *
 *  Deskripsi:
 *    Program membaca push button dengan software debounce menggunakan
 *    state machine 3-state: IDLE → PRESS_DETECTED → CONFIRMED.
 *    Setiap penekanan tombol yang valid (ter-debounce) akan men-toggle
 *    LED ON/OFF.
 *
 *    State Machine:
 *      IDLE            : Menunggu button ditekan (LOW)
 *      PRESS_DETECTED  : Button terdeteksi LOW, mulai timer debounce
 *      CONFIRMED       : Debounce selesai, validasi button masih LOW
 *
 *  Rangkaian / Wiring:
 *    Button:
 *      VCC 3.3V ──► Resistor 10kΩ ──┬──► ESP32 GPIO4 (input)
 *                                    │
 *                              Push Button
 *                                    │
 *                                   GND
 *    (Button ditekan = LOW, dilepas = HIGH)
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

static const char *TAG = "DEBOUNCE";

/* ── State machine states ─────────────────────────────────────────────── */
typedef enum {
    STATE_IDLE,             /* Menunggu penekanan */
    STATE_PRESS_DETECTED,   /* Penekanan terdeteksi, menunggu debounce */
    STATE_CONFIRMED         /* Penekanan terkonfirmasi, menunggu release */
} debounce_state_t;

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
 * @brief Entry point utama
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Button Debounce (State Machine) ===");
    gpio_init_all();

    debounce_state_t state = STATE_IDLE;
    int64_t debounce_start = 0;
    bool led_state = false;
    int press_count = 0;

    while (1) {
        int btn_level = gpio_get_level(BUTTON_PIN);     /* 0=ditekan, 1=dilepas */
        int64_t now = esp_timer_get_time() / 1000;      /* Waktu dalam ms */

        switch (state) {
            case STATE_IDLE:
                if (btn_level == 0) {
                    /* Button terdeteksi ditekan → mulai debounce */
                    state = STATE_PRESS_DETECTED;
                    debounce_start = now;
                    ESP_LOGD(TAG, "State: IDLE → PRESS_DETECTED");
                }
                break;

            case STATE_PRESS_DETECTED:
                if (btn_level == 1) {
                    /* Button dilepas sebelum debounce selesai → noise, kembali ke IDLE */
                    state = STATE_IDLE;
                    ESP_LOGD(TAG, "State: PRESS_DETECTED → IDLE (noise)");
                } else if ((now - debounce_start) >= DEBOUNCE_MS) {
                    /* Debounce time tercapai dan button masih ditekan → CONFIRMED */
                    state = STATE_CONFIRMED;

                    /* Toggle LED */
                    led_state = !led_state;
                    gpio_set_level(LED_PIN, (uint32_t)led_state);
                    press_count++;

                    ESP_LOGI(TAG, "Tekan #%d VALID — LED %s",
                             press_count, led_state ? "ON" : "OFF");
                }
                break;

            case STATE_CONFIRMED:
                if (btn_level == 1) {
                    /* Button dilepas → kembali ke IDLE, siap terima penekanan baru */
                    state = STATE_IDLE;
                    ESP_LOGD(TAG, "State: CONFIRMED → IDLE (released)");
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
