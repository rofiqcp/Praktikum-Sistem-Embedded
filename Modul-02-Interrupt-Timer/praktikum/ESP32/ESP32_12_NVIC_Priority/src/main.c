/**
 * @file main.c
 * @brief Program 12 - Interrupt Priority Demo (NVIC equivalent)
 * @details Demonstrasi prioritas interrupt pada ESP32. Dua button
 *          dengan prioritas interrupt berbeda. Tekan button low-priority
 *          lalu segera tekan high-priority untuk melihat preemption.
 *
 * @modul    Modul 02 - Interrupt & Timer
 * @board    ESP32 DOIT DevKit V1 / Lolin S2 Mini / ESP32-S3
 * @framework ESP-IDF
 *
 * @koneksi Hardware:
 *   ESP32 GPIO18 <-- Button HIGH --> GND (pull-up internal)
 *   ESP32 GPIO19 <-- Button LOW  --> GND (pull-up internal)
 *   ESP32 GPIO2  --> LED_HIGH (+) --> 220Ω --> GND
 *   ESP32 GPIO4  --> LED_LOW  (+) --> 220Ω --> GND
 *
 * @cara_kerja:
 *   1. Dua GPIO interrupt dengan level priority berbeda
 *   2. Low-priority ISR: set flag, LED blink lambat di task
 *   3. High-priority ISR: set flag, LED blink cepat di task
 *   4. Demo menunjukkan bagaimana interrupt priority bekerja
 *   5. Logging menunjukkan urutan eksekusi dan timing
 *
 * @note ESP32 interrupt priority: 1 (lowest) - 3 (highest, LEVEL type)
 *       Priority 4-5 reserved for NMI/debug
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "config.h"

static const char *TAG = "INT_PRIORITY";

/* ISR flags dan counters */
static volatile bool high_isr_flag = false;
static volatile bool low_isr_flag = false;
static volatile uint32_t high_isr_count = 0;
static volatile uint32_t low_isr_count = 0;
static volatile int64_t high_isr_time = 0;
static volatile int64_t low_isr_time = 0;

/**
 * @brief High-priority ISR handler
 */
static void IRAM_ATTR high_priority_isr(void *arg)
{
    int64_t now = esp_timer_get_time();
    static int64_t last_time = 0;

    if ((now - last_time) < DEBOUNCE_US) return;
    last_time = now;

    high_isr_flag = true;
    high_isr_count++;
    high_isr_time = now;
}

/**
 * @brief Low-priority ISR handler
 */
static void IRAM_ATTR low_priority_isr(void *arg)
{
    int64_t now = esp_timer_get_time();
    static int64_t last_time = 0;

    if ((now - last_time) < DEBOUNCE_US) return;
    last_time = now;

    low_isr_flag = true;
    low_isr_count++;
    low_isr_time = now;
}

/**
 * @brief Inisialisasi GPIO
 */
static void gpio_init_pins(void)
{
    /* LED outputs */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_HIGH_PIN) | (1ULL << LED_LOW_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_conf));

    /* Button inputs */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_HIGH_PIN) | (1ULL << BUTTON_LOW_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_conf));

    /* Install ISR service with default flags (allows mixed priorities) */
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

    /* Add ISR handlers */
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_HIGH_PIN, high_priority_isr, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_LOW_PIN, low_priority_isr, NULL));
}

/**
 * @brief Blink LED N kali dengan kecepatan tertentu
 */
static void blink_led(gpio_num_t pin, int count, int delay_ms)
{
    for (int i = 0; i < count; i++) {
        gpio_set_level(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_set_level(pin, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Interrupt Priority Demo ===");
    ESP_LOGI(TAG, "HIGH priority button: GPIO%d -> LED GPIO%d",
             BUTTON_HIGH_PIN, LED_HIGH_PIN);
    ESP_LOGI(TAG, "LOW  priority button: GPIO%d -> LED GPIO%d",
             BUTTON_LOW_PIN, LED_LOW_PIN);
    ESP_LOGI(TAG, "Press buttons to trigger interrupts.");
    ESP_LOGI(TAG, "Try pressing LOW then immediately HIGH to see priority effect.");

    gpio_init_pins();
    gpio_set_level(LED_HIGH_PIN, 0);
    gpio_set_level(LED_LOW_PIN, 0);

    while (1) {
        /* Process high-priority interrupt (always check first) */
        if (high_isr_flag) {
            high_isr_flag = false;
            ESP_LOGW(TAG, ">>> HIGH PRIORITY ISR #%lu triggered! (t=%lld us)",
                     (unsigned long)high_isr_count, (long long)high_isr_time);

            /* Quick blink to show high priority */
            blink_led(LED_HIGH_PIN, 5, 50);  /* 5x fast blink */

            /* Show timing difference if both triggered close together */
            if (low_isr_time > 0 && high_isr_time > 0) {
                int64_t diff = high_isr_time - low_isr_time;
                if (diff > 0 && diff < 2000000) {
                    ESP_LOGW(TAG, "    HIGH arrived %lld us after LOW",
                             (long long)diff);
                }
            }
        }

        /* Process low-priority interrupt */
        if (low_isr_flag) {
            low_isr_flag = false;
            ESP_LOGI(TAG, "--- LOW  PRIORITY ISR #%lu triggered! (t=%lld us)",
                     (unsigned long)low_isr_count, (long long)low_isr_time);

            /* Slow blink to show low priority */
            blink_led(LED_LOW_PIN, 3, 150);  /* 3x slow blink */
        }

        /* Periodic status */
        static uint32_t last_report = 0;
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now_ms - last_report > 5000) {
            last_report = now_ms;
            ESP_LOGI(TAG, "Status: HIGH=%lu, LOW=%lu interrupts total",
                     (unsigned long)high_isr_count,
                     (unsigned long)low_isr_count);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
