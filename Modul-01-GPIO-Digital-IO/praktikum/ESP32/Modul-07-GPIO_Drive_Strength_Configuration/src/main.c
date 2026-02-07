/**
 * @file main.c
 * @brief Program 07: GPIO Drive Strength Configuration (ESP-IDF)
 *
 * Konfigurasi drive strength GPIO ESP32 untuk berbagai kebutuhan arus.
 * ESP32 mendukung 4 level: 5mA, 10mA, 20mA, 40mA
 * Menggunakan ESP-IDF native GPIO driver.
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "GPIO_DRIVE";

/* ==================== KONFIGURASI ==================== */
#define LED_WEAK        2       /* Low drive strength */
#define LED_NORMAL      4       /* Default drive strength */
#define LED_STRONG      5       /* High drive strength */
#define BUTTON_PIN      0       /* Cycle drive strength */

/* ==================== VARIABEL ==================== */
static uint8_t current_drive = 0;
static const char *drive_names[] = {"5mA (WEAK)", "10mA (DEFAULT)", "20mA (MEDIUM)", "40mA (STRONG)"};
static const gpio_drive_cap_t drive_values[] = {
    GPIO_DRIVE_CAP_0, GPIO_DRIVE_CAP_1, GPIO_DRIVE_CAP_2, GPIO_DRIVE_CAP_3
};

static volatile bool button_flag = false;
static volatile int64_t last_press_time = 0;

/* ==================== ISR ==================== */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time() / 1000;
    if (now - last_press_time > 200) {
        button_flag = true;
        last_press_time = now;
    }
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 07: GPIO Drive Strength Config");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi LED pins (output) */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_WEAK) | (1ULL << LED_NORMAL) | (1ULL << LED_STRONG),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    /* Konfigurasi Button (input + pull-up + interrupt) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    /* Set berbagai drive strength */
    gpio_set_drive_capability(LED_WEAK, GPIO_DRIVE_CAP_0);     /* 5mA */
    gpio_set_drive_capability(LED_NORMAL, GPIO_DRIVE_CAP_2);   /* 20mA (default) */
    gpio_set_drive_capability(LED_STRONG, GPIO_DRIVE_CAP_3);   /* 40mA */

    /* Install GPIO ISR service */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    ESP_LOGI(TAG, "Drive Strength Levels:");
    ESP_LOGI(TAG, "  GPIO_DRIVE_CAP_0: ~5mA");
    ESP_LOGI(TAG, "  GPIO_DRIVE_CAP_1: ~10mA");
    ESP_LOGI(TAG, "  GPIO_DRIVE_CAP_2: ~20mA (default)");
    ESP_LOGI(TAG, "  GPIO_DRIVE_CAP_3: ~40mA");
    ESP_LOGI(TAG, "LED Configurations:");
    ESP_LOGI(TAG, "  GPIO%d: 5mA (WEAK)", LED_WEAK);
    ESP_LOGI(TAG, "  GPIO%d: 20mA (NORMAL)", LED_NORMAL);
    ESP_LOGI(TAG, "  GPIO%d: 40mA (STRONG)", LED_STRONG);

    /* Nyalakan semua LED */
    gpio_set_level(LED_WEAK, 1);
    gpio_set_level(LED_NORMAL, 1);
    gpio_set_level(LED_STRONG, 1);

    ESP_LOGI(TAG, "Semua LED menyala - perhatikan perbedaan brightness!");
    ESP_LOGI(TAG, "Tekan button untuk cycle drive strength pada GPIO%d", LED_WEAK);

    int64_t last_report = 0;

    while (1) {
        if (button_flag) {
            button_flag = false;
            current_drive = (current_drive + 1) % 4;
            gpio_set_drive_capability(LED_WEAK, drive_values[current_drive]);
            ESP_LOGI(TAG, "GPIO%d Drive Strength: %s", LED_WEAK, drive_names[current_drive]);
        }

        /* Periodic status report */
        int64_t now = esp_timer_get_time() / 1000;
        if (now - last_report > 2000) {
            last_report = now;
            gpio_drive_cap_t cap;
            gpio_get_drive_capability(LED_WEAK, &cap);
            ESP_LOGI(TAG, "[%lld] GPIO%d current drive cap: %d", now, LED_WEAK, cap);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
