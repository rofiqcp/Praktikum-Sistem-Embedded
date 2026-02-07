/**
 * @file main.c
 * @brief Program 02: Multi-LED Running Pattern (ESP-IDF)
 *
 * Deskripsi:
 * Mengendalikan beberapa LED dengan berbagai pola running.
 * Termasuk pola: running, bounce, fill, dan blink all.
 * Menggunakan ESP-IDF native GPIO API.
 *
 * Hardware:
 * - ESP32 DevKitC / S2 / S3
 * - 4x LED dengan resistor 220Ohm
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MULTI_LED";

/* ==================== KONFIGURASI ==================== */
#define NUM_LEDS        4
#define LED_1           2
#define LED_2           4
#define LED_3           5
#define LED_4           18

static const gpio_num_t LED_PINS[NUM_LEDS] = {LED_1, LED_2, LED_3, LED_4};

#define PATTERN_DELAY_MS    150

/* Pattern IDs */
#define PATTERN_RUNNING     0
#define PATTERN_BOUNCE      1
#define PATTERN_FILL        2
#define PATTERN_BLINK_ALL   3

/* ==================== VARIABEL GLOBAL ==================== */
static uint8_t current_led = 0;
static uint8_t current_pattern = PATTERN_RUNNING;
static bool direction = true;
static uint8_t fill_count = 0;

/* ==================== FUNCTION PROTOTYPES ==================== */
static void init_leds(void);
static void all_leds_off(void);
static void all_leds_on(void);
static void running_pattern(void);
static void bounce_pattern(void);
static void fill_pattern(void);
static void blink_all_pattern(void);
static void show_current_pattern(void);

/* ==================== FUNCTIONS ==================== */

static void init_leds(void)
{
    /* Konfigurasi semua LED pin sekaligus dengan bitmask */
    uint64_t pin_mask = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        pin_mask |= (1ULL << LED_PINS[i]);
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Initializing LEDs on GPIO: %d %d %d %d",
             LED_PINS[0], LED_PINS[1], LED_PINS[2], LED_PINS[3]);
}

static void all_leds_off(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(LED_PINS[i], 0);
    }
}

static void all_leds_on(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(LED_PINS[i], 1);
    }
}

static void running_pattern(void)
{
    all_leds_off();
    gpio_set_level(LED_PINS[current_led], 1);
    ESP_LOGI(TAG, "Running: LED %d ON", current_led + 1);

    current_led++;
    if (current_led >= NUM_LEDS) {
        current_led = 0;
    }
}

static void bounce_pattern(void)
{
    all_leds_off();
    gpio_set_level(LED_PINS[current_led], 1);
    ESP_LOGI(TAG, "Bounce: LED %d ON (%s)", current_led + 1,
             direction ? "->" : "<-");

    if (direction) {
        current_led++;
        if (current_led >= NUM_LEDS - 1) {
            direction = false;
        }
    } else {
        if (current_led == 0) {
            direction = true;
        } else {
            current_led--;
        }
    }
}

static void fill_pattern(void)
{
    for (int i = 0; i <= fill_count; i++) {
        gpio_set_level(LED_PINS[i], 1);
    }
    for (int i = fill_count + 1; i < NUM_LEDS; i++) {
        gpio_set_level(LED_PINS[i], 0);
    }

    ESP_LOGI(TAG, "Fill: %d LED(s) ON", fill_count + 1);

    fill_count++;
    if (fill_count >= NUM_LEDS) {
        fill_count = 0;
        all_leds_off();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void blink_all_pattern(void)
{
    static bool state = false;
    state = !state;

    if (state) {
        all_leds_on();
        ESP_LOGI(TAG, "Blink: ALL ON");
    } else {
        all_leds_off();
        ESP_LOGI(TAG, "Blink: ALL OFF");
    }
}

static void show_current_pattern(void)
{
    const char *patterns[] = {"RUNNING", "BOUNCE", "FILL", "BLINK ALL"};
    ESP_LOGI(TAG, ">>> Pattern changed to: %s", patterns[current_pattern]);
}

/* ==================== SERIAL INPUT TASK ==================== */
static void serial_task(void *arg)
{
    /* Disable line buffering on stdin for immediate char reads */
    setvbuf(stdin, NULL, _IONBF, 0);

    while (1) {
        int c = getchar();
        if (c != EOF && c >= '0' && c <= '3') {
            current_pattern = c - '0';
            current_led = 0;
            fill_count = 0;
            direction = true;
            all_leds_off();
            show_current_pattern();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 02: Multi-LED Running Pattern");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    init_leds();

    ESP_LOGI(TAG, "Patterns available:");
    ESP_LOGI(TAG, "  0 - Running (satu arah)");
    ESP_LOGI(TAG, "  1 - Bounce (bolak-balik)");
    ESP_LOGI(TAG, "  2 - Fill (mengisi)");
    ESP_LOGI(TAG, "  3 - Blink All");
    ESP_LOGI(TAG, "Ketik angka 0-3 untuk ganti pattern");

    /* Start serial input task */
    xTaskCreate(serial_task, "serial_task", 2048, NULL, 5, NULL);

    /* Main loop - update LED pattern */
    while (1) {
        switch (current_pattern) {
            case PATTERN_RUNNING:
                running_pattern();
                break;
            case PATTERN_BOUNCE:
                bounce_pattern();
                break;
            case PATTERN_FILL:
                fill_pattern();
                break;
            case PATTERN_BLINK_ALL:
                blink_all_pattern();
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(PATTERN_DELAY_MS));
    }
}
