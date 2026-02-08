/**
 * ============================================================
 *  ESP32_06_Watchdog_Timer
 * ============================================================
 *  Modul   : 02 - Interrupt & Timer
 *  Board   : ESP32 / ESP32-S2 / ESP32-S3
 *  Framework: ESP-IDF
 *
 *  Deskripsi:
 *    Demonstrasi Task Watchdog Timer (TWDT) pada ESP32.
 *    TWDT memonitor task — jika task tidak memanggil
 *    esp_task_wdt_reset() dalam waktu timeout, sistem
 *    akan melakukan panic & reset.
 *
 *  Koneksi Hardware:
 *    - GPIO4  <- Push Button (ke GND, internal pull-up aktif)
 *    - GPIO2  -> LED (+ resistor 220Ω ke GND)
 *
 *  Cara Kerja:
 *    Normal Mode:
 *      - LED blink setiap 1 detik
 *      - Watchdog di-feed (reset) setiap loop
 *      - Log: "Feeding watchdog..."
 *
 *    Hang Simulation (tekan tombol):
 *      - LED menyala terus (tidak blink)
 *      - Watchdog TIDAK di-feed
 *      - Log: "STARVING watchdog..."
 *      - Setelah 5 detik → WDT panic → ESP32 reset
 *
 *  Setelah reset, sistem kembali ke Normal Mode.
 * ============================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

#include "config.h"

static const char *TAG = "WDT_DEMO";

/* ---- State flags ---- */
static volatile bool simulate_hang = false;

/**
 * GPIO ISR Handler - toggle mode hang/normal.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    simulate_hang = true;   // Sekali tekan → masuk mode hang
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Watchdog Timer Demo ===");
    ESP_LOGI(TAG, "LED pin    : GPIO%d", LED_PIN);
    ESP_LOGI(TAG, "Button pin : GPIO%d", BUTTON_PIN);
    ESP_LOGI(TAG, "WDT timeout: %d detik", WDT_TIMEOUT_S);

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
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    /* ---- Install ISR service & attach handler ---- */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, NULL);

    /* ============================================================
     *  Inisialisasi Task Watchdog Timer (TWDT)
     *
     *  esp_task_wdt_config_t:
     *    .timeout_ms    = timeout dalam milliseconds
     *    .idle_core_mask = bitmask core yang idle task-nya dimonitor
     *    .trigger_panic  = true → WDT timeout menyebabkan panic & reset
     * ============================================================ */
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms    = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,            // Jangan monitor idle task
        .trigger_panic  = true,         // Panic on timeout → reset
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));

    /* Tambahkan task saat ini (app_main) ke monitoring TWDT */
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));     // NULL = current task

    ESP_LOGI(TAG, "TWDT diinisialisasi: timeout=%d s, panic=ON", WDT_TIMEOUT_S);
    ESP_LOGI(TAG, "Normal mode: LED blink, WDT di-feed setiap loop");
    ESP_LOGI(TAG, "Tekan tombol → simulasi hang → WDT reset setelah %d s", WDT_TIMEOUT_S);

    /* ---- LED state ---- */
    bool led_state = false;
    uint32_t loop_count = 0;

    /* ---- Main Loop ---- */
    while (1) {
        loop_count++;

        if (!simulate_hang) {
            /* ==== Normal Mode ==== */
            /* Feed watchdog */
            esp_task_wdt_reset();

            /* Blink LED */
            led_state = !led_state;
            gpio_set_level(LED_PIN, led_state ? 1 : 0);

            ESP_LOGI(TAG, "[NORMAL ] Loop #%lu | Feeding watchdog ✓ | LED = %s",
                     (unsigned long)loop_count,
                     led_state ? "ON" : "OFF");

        } else {
            /* ==== Hang Simulation ==== */
            /* TIDAK memanggil esp_task_wdt_reset() → WDT akan timeout */

            /* LED tetap menyala (indikator hang) */
            gpio_set_level(LED_PIN, 1);

            ESP_LOGW(TAG, "[STARVE ] Loop #%lu | NOT feeding watchdog ✗ | "
                     "WDT reset dalam ~%d detik...",
                     (unsigned long)loop_count, WDT_TIMEOUT_S);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));    // 1 detik per loop
    }
}
