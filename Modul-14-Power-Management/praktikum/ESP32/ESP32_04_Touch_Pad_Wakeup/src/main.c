/**
 * ============================================================================
 * PROJECT  : ESP32_04_Touch_Pad_Wakeup
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Deep Sleep dengan Touch Pad Wake-up
 *
 * DESKRIPSI:
 * ESP32 memiliki 10 sensor kapasitif (touch pad) yang bisa digunakan sebagai
 * sumber wake-up dari deep sleep. Sangat berguna untuk interface tanpa tombol
 * fisik — cukup sentuh pin/pad logam.
 *
 * ============================================================================
 * WIRING
 * ============================================================================
 * Touch Pad → GPIO4 (TOUCH_PAD_NUM0) — hubungkan ke kabel/pad logam
 * LED       → GPIO2 (built-in)
 *
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/touch_pad.h"
#include "driver/gpio.h"

#define LED_PIN         GPIO_NUM_2
#define TOUCH_PAD_NO    TOUCH_PAD_NUM0   /* GPIO4 */
#define TOUCH_THRESH    400

static const char *TAG = "TOUCH_WAKE";

RTC_DATA_ATTR static int boot_count = 0;

void app_main(void)
{
    boot_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Touch Pad Wake-up Demo       ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "=== Boot #%d ===", boot_count);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TOUCHPAD) {
        touch_pad_t pin;
        touch_pad_get_wakeup_status(&pin);
        ESP_LOGI(TAG, "Woke up by TOUCH PAD #%d!", pin);
    } else {
        ESP_LOGI(TAG, "Wake-up cause: %d (power on / reset)", cause);
    }

    /* LED feedback */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 5; i++) {
        gpio_set_level(LED_PIN, i % 2);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    gpio_set_level(LED_PIN, 0);

    /* Baca nilai touch saat ini */
    touch_pad_init();
    touch_pad_config(TOUCH_PAD_NO, TOUCH_THRESH);
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_filter_start(10);

    vTaskDelay(pdMS_TO_TICKS(200));

    uint16_t touch_val;
    touch_pad_read_filtered(TOUCH_PAD_NO, &touch_val);
    ESP_LOGI(TAG, "Touch value: %d (threshold: %d)", touch_val, TOUCH_THRESH);

    /* Setup touch pad wake-up */
    esp_sleep_enable_touchpad_wakeup();

    /* Backup timer wake-up (60 detik) */
    esp_sleep_enable_timer_wakeup(60 * 1000000ULL);

    ESP_LOGI(TAG, "Entering deep sleep... Touch GPIO4 to wake up!");
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_deep_sleep_start();
}
