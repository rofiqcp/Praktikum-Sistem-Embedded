/**
 * ==========================================================================
 *  ESP32_03_LED_Binary_Counter — 4-bit Binary Counter pada LED
 * ==========================================================================
 *  Modul   : 01 - GPIO Digital I/O
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF (PlatformIO)
 *
 *  Deskripsi:
 *    Program menghitung dari 0 sampai 15 (4-bit) dan menampilkan
 *    nilai biner pada 4 LED menggunakan operasi bitwise.
 *    Murni GPIO digital — TIDAK menggunakan PWM.
 *
 *    Contoh tampilan LED (1=ON, 0=OFF):
 *      Desimal 0  → LED: 0000
 *      Desimal 5  → LED: 0101
 *      Desimal 10 → LED: 1010
 *      Desimal 15 → LED: 1111
 *
 *  Rangkaian / Wiring:
 *    ESP32 GPIO16 ──► R 220Ω ──► LED BIT0 (LSB) ──► GND
 *    ESP32 GPIO17 ──► R 220Ω ──► LED BIT1        ──► GND
 *    ESP32 GPIO18 ──► R 220Ω ──► LED BIT2        ──► GND
 *    ESP32 GPIO19 ──► R 220Ω ──► LED BIT3 (MSB)  ──► GND
 *
 *  Komponen:
 *    - 4x LED (warna bebas)
 *    - 4x Resistor 220Ω
 *    - Breadboard + kabel jumper
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "BIN_COUNTER";

/* Array pin LED terurut dari BIT0 (LSB) ke BIT3 (MSB) */
static const gpio_num_t led_bits[NUM_BITS] = {
    LED_BIT0, LED_BIT1, LED_BIT2, LED_BIT3
};

/**
 * @brief Inisialisasi semua pin LED sebagai output
 */
static void gpio_init_leds(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_BIT0) |
                        (1ULL << LED_BIT1) |
                        (1ULL << LED_BIT2) |
                        (1ULL << LED_BIT3),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* Matikan semua LED saat awal */
    for (int i = 0; i < NUM_BITS; i++) {
        gpio_set_level(led_bits[i], 0);
    }

    ESP_LOGI(TAG, "GPIO LED terinitialisasi — BIT0:GPIO%d BIT1:GPIO%d BIT2:GPIO%d BIT3:GPIO%d",
             LED_BIT0, LED_BIT1, LED_BIT2, LED_BIT3);
}

/**
 * @brief Tampilkan nilai desimal pada 4 LED sebagai biner
 * @param value Nilai 0-15
 */
static void display_binary(uint8_t value)
{
    for (int bit = 0; bit < NUM_BITS; bit++) {
        int level = (value >> bit) & 0x01;
        gpio_set_level(led_bits[bit], level);
    }
}

/**
 * @brief Entry point utama
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== LED Binary Counter (0-15) ===");
    gpio_init_leds();

    uint8_t count = 0;

    while (1) {
        /* Tampilkan nilai biner pada LED */
        display_binary(count);

        /* Log nilai dalam desimal dan biner */
        ESP_LOGI(TAG, "Desimal: %2d  |  Biner: %d%d%d%d",
                 count,
                 (count >> 3) & 0x01,   /* BIT3 (MSB) */
                 (count >> 2) & 0x01,   /* BIT2 */
                 (count >> 1) & 0x01,   /* BIT1 */
                 (count >> 0) & 0x01);  /* BIT0 (LSB) */

        /* Increment, wrap-around 0-15 */
        count = (count + 1) & MAX_COUNT;

        vTaskDelay(pdMS_TO_TICKS(COUNT_DELAY_MS));
    }
}
