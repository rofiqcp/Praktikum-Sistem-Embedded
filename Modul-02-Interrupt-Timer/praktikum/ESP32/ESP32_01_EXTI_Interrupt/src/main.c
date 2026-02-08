/**
 * ============================================================
 *  ESP32_01_EXTI_Interrupt
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi External Interrupt (EXTI) pada GPIO.
 *    Push button pada GPIO4 men-trigger falling-edge interrupt.
 *    ISR meng-set flag, main loop membaca flag dan toggle LED.
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up aktif)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *
 *  Cara Kerja:
 *    1. GPIO4 dikonfigurasi sebagai input dengan pull-up internal
 *    2. Interrupt dikonfigurasi pada falling edge (tombol ditekan)
 *    3. ISR handler meng-set volatile flag
 *    4. Main loop mengecek flag, toggle LED, dan log counter
 * ============================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "EXTI_INT";

/* ---- Volatile flag & counter (shared dengan ISR) ---- */
static volatile bool     isr_flag  = false;
static volatile uint32_t isr_count = 0;

/* ---- LED state ---- */
static bool led_state = false;

/**
 * ISR Handler - dipanggil saat falling edge pada BUTTON_PIN.
 * HARUS ber-atribut IRAM_ATTR agar kode tetap di IRAM.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    isr_flag = true;
    isr_count++;
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 External Interrupt Demo ===");
    ESP_LOGI(TAG, "Button pin : GPIO%d", BUTTON_PIN);
    ESP_LOGI(TAG, "LED pin    : GPIO%d", LED_PIN);

    /* ---- Konfigurasi LED (output) ---- */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);

    /* ---- Konfigurasi Button (input + pull-up + falling edge) ---- */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,      // Falling edge
    };
    gpio_config(&btn_conf);

    /* ---- Install ISR service & attach handler ---- */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, NULL);

    ESP_LOGI(TAG, "Interrupt aktif. Tekan tombol untuk toggle LED...");

    /* ---- Main Loop ---- */
    while (1) {
        if (isr_flag) {
            isr_flag = false;

            /* Toggle LED */
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state ? 1 : 0);

            ESP_LOGI(TAG, "Interrupt #%lu | LED = %s",
                     (unsigned long)isr_count,
                     led_state ? "ON" : "OFF");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
