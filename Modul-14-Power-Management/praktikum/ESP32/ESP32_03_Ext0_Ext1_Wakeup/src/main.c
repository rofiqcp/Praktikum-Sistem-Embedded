/**
 * ============================================================================
 * PROJECT  : ESP32_03_Ext0_Ext1_Wakeup
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : Deep Sleep dengan External Wake-up (ext0 & ext1)
 *
 * DESKRIPSI:
 * Demonstrasi dua jenis external wake-up pada ESP32:
 * - ext0: Satu GPIO RTC tertentu, trigger pada level HIGH atau LOW
 * - ext1: Beberapa GPIO RTC, trigger ANY_HIGH atau ALL_LOW
 * Juga dikombinasikan dengan timer wake-up sebagai backup.
 *
 * ============================================================================
 * WIRING
 * ============================================================================
 * Button 1 → GPIO33 (ext0, pull-up, active LOW)
 * Button 2 → GPIO32 (ext1, pull-up, active LOW)
 * LED      → GPIO2 (built-in)
 *
 * ============================================================================
 * EXPECTED OUTPUT
 * ============================================================================
 *   I (xxx) EXT_WAKE: === Boot #3 ===
 *   I (xxx) EXT_WAKE: Wake-up cause: EXT0 (GPIO33)
 *   I (xxx) EXT_WAKE: Configured: Timer(10s) + EXT0(GPIO33) + EXT1(GPIO32)
 *   I (xxx) EXT_WAKE: Entering deep sleep...
 * ============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#define LED_PIN      GPIO_NUM_2
#define EXT0_PIN     GPIO_NUM_33   /* RTC GPIO untuk ext0 */
#define EXT1_PIN     GPIO_NUM_32   /* RTC GPIO untuk ext1 */
#define TIMER_SEC    10

static const char *TAG = "EXT_WAKE";

RTC_DATA_ATTR static int boot_count = 0;

static void print_wakeup_cause(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake-up cause: TIMER (%d seconds)", TIMER_SEC);
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wake-up cause: EXT0 (GPIO%d)", EXT0_PIN);
            break;
        case ESP_SLEEP_WAKEUP_EXT1: {
            uint64_t mask = esp_sleep_get_ext1_wakeup_status();
            ESP_LOGI(TAG, "Wake-up cause: EXT1 (mask: 0x%llx)", mask);
            /* Cari GPIO mana yang trigger */
            for (int i = 0; i < 40; i++) {
                if (mask & (1ULL << i)) {
                    ESP_LOGI(TAG, "  -> GPIO%d triggered", i);
                }
            }
            break;
        }
        default:
            ESP_LOGI(TAG, "Wake-up cause: POWER ON / RESET (%d)", cause);
            break;
    }
}

void app_main(void)
{
    boot_count++;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Ext0 + Ext1 Wake-up Demo     ║");
    ESP_LOGI(TAG, "║   Modul 14: Power Management         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "=== Boot #%d ===", boot_count);

    print_wakeup_cause();

    /* LED indicator: flash */
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* === Konfigurasi Wake-up Sources === */

    /* 1. Timer wake-up (backup) */
    esp_sleep_enable_timer_wakeup(TIMER_SEC * 1000000ULL);

    /* 2. EXT0: GPIO33, trigger saat LOW (button ditekan) */
    /*    Perlu pull-up pada RTC domain */
    rtc_gpio_pullup_en(EXT0_PIN);
    rtc_gpio_pulldown_dis(EXT0_PIN);
    esp_sleep_enable_ext0_wakeup(EXT0_PIN, 0);  /* 0 = LOW level */

    /* 3. EXT1: GPIO32, trigger saat ANY_HIGH */
    /*    Untuk ext1 ALL_LOW, gunakan ESP_EXT1_WAKEUP_ALL_LOW */
    uint64_t ext1_mask = (1ULL << EXT1_PIN);
    esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ALL_LOW);

    ESP_LOGI(TAG, "Configured: Timer(%ds) + EXT0(GPIO%d LOW) + EXT1(GPIO%d ALL_LOW)",
             TIMER_SEC, EXT0_PIN, EXT1_PIN);
    ESP_LOGI(TAG, "Entering deep sleep...");
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_deep_sleep_start();
}
