/**
 * ESP32_07_Touch_Wakeup
 * 
 * Konsep: Touch pad wakeup dari deep sleep.
 * - Konfigurasi touch pad T0 (GPIO4) dengan threshold
 * - Masuk deep sleep, bangun saat pad disentuh
 * - Baca nilai touch pad, tentukan pad mana yang memicu wakeup
 * 
 * Wiring: Kabel/pad sentuh pada GPIO4 (T0), LED opsional pada GPIO2
 * 
 * Framework: ESP-IDF (bukan Arduino)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/touch_pad.h"
#include "driver/gpio.h"
#include "soc/sens_periph.h"

static const char *TAG = "TOUCH_WAKEUP";

#define LED_PIN             GPIO_NUM_2
#define TOUCH_PAD_CHANNEL   TOUCH_PAD_NUM0  // GPIO4
#define TOUCH_THRESHOLD     400             // Threshold sentuh (makin kecil = makin sensitif)
#define WAKEUP_DELAY_SEC    30              // Timeout deep sleep jika tidak ada sentuhan

// Counter boot disimpan di RTC memory (bertahan saat deep sleep)
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int touch_wakeup_count = 0;

/**
 * Inisialisasi LED indikator
 */
static void init_led(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

/**
 * Kedipkan LED beberapa kali sebagai indikator
 */
static void blink_led(int times, int delay_ms)
{
    for (int i = 0; i < times; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * Inisialisasi touch pad
 */
static void init_touch_pad(void)
{
    // Inisialisasi driver touch pad
    ESP_ERROR_CHECK(touch_pad_init());

    // Set tegangan referensi untuk charging/discharging
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

    // Konfigurasi channel touch pad T0 (GPIO4)
    touch_pad_config(TOUCH_PAD_CHANNEL, TOUCH_THRESHOLD);

    // Kalibrasi - baca nilai awal tanpa sentuhan
    touch_pad_filter_start(10);
    vTaskDelay(pdMS_TO_TICKS(200));
}

/**
 * Baca dan tampilkan nilai touch pad
 */
static void read_touch_values(void)
{
    uint16_t touch_value = 0;
    uint16_t touch_filtered = 0;

    // Baca nilai mentah
    touch_pad_read(TOUCH_PAD_CHANNEL, &touch_value);
    // Baca nilai terfilter
    touch_pad_read_filtered(TOUCH_PAD_CHANNEL, &touch_filtered);

    ESP_LOGI(TAG, "Touch Pad T0 (GPIO4): raw=%d, filtered=%d, threshold=%d",
             touch_value, touch_filtered, TOUCH_THRESHOLD);

    if (touch_filtered < TOUCH_THRESHOLD) {
        ESP_LOGW(TAG, ">>> PAD TERSENTUH! (filtered=%d < threshold=%d)",
                 touch_filtered, TOUCH_THRESHOLD);
    } else {
        ESP_LOGI(TAG, "Pad tidak tersentuh (filtered=%d >= threshold=%d)",
                 touch_filtered, TOUCH_THRESHOLD);
    }
}

/**
 * Analisis penyebab wakeup
 */
static void print_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            touch_wakeup_count++;
            ESP_LOGW(TAG, "=== Wakeup oleh TOUCH PAD! (touch wakeup ke-%d) ===",
                     touch_wakeup_count);
            // Cek touch pad mana yang memicu
            {
                touch_pad_t pad = esp_sleep_get_touchpad_wakeup_status();
                if (pad == TOUCH_PAD_CHANNEL) {
                    ESP_LOGI(TAG, "Touch pad yang memicu: T0 (GPIO4)");
                } else {
                    ESP_LOGI(TAG, "Touch pad yang memicu: T%d", (int)pad);
                }
            }
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wakeup oleh TIMER (timeout tanpa sentuhan)");
            break;
        default:
            ESP_LOGI(TAG, "Boot pertama atau reset (cause=%d)", (int)cause);
            break;
    }
}

/**
 * Konfigurasi dan masuk deep sleep dengan touch wakeup
 */
static void enter_deep_sleep_with_touch(void)
{
    ESP_LOGI(TAG, "--------------------------------------------");
    ESP_LOGI(TAG, "Konfigurasi touch pad wakeup...");

    // Re-init touch pad untuk deep sleep
    touch_pad_filter_stop();
    touch_pad_deinit();
    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(TOUCH_PAD_CHANNEL, TOUCH_THRESHOLD);

    // Aktifkan wakeup dari touch pad
    ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());

    // Juga aktifkan timer wakeup sebagai backup
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup((uint64_t)WAKEUP_DELAY_SEC * 1000000ULL));

    ESP_LOGW(TAG, "Masuk DEEP SLEEP... Sentuh GPIO4 untuk bangunkan!");
    ESP_LOGI(TAG, "Atau akan bangun otomatis setelah %d detik", WAKEUP_DELAY_SEC);
    ESP_LOGI(TAG, "--------------------------------------------");

    // Tunggu log terkirim
    vTaskDelay(pdMS_TO_TICKS(100));

    // Masuk deep sleep
    esp_deep_sleep_start();
}

void app_main(void)
{
    boot_count++;

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 Touch Pad Wakeup - Power Management");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Boot ke-%d | Touch wakeup total: %d", boot_count, touch_wakeup_count);

    // Analisis penyebab bangun
    print_wakeup_reason();

    // Inisialisasi LED
    init_led();

    // Kedipkan LED: 3x untuk touch wakeup, 1x untuk lainnya
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD) {
        blink_led(5, 100);  // Kedip cepat 5x saat touch wakeup
    } else {
        blink_led(2, 300);  // Kedip lambat 2x saat boot biasa
    }

    // Inisialisasi dan baca touch pad
    init_touch_pad();

    // Baca beberapa kali untuk menunjukkan perubahan nilai
    ESP_LOGI(TAG, "\n--- Pembacaan Touch Pad (5 sampel) ---");
    for (int i = 0; i < 5; i++) {
        read_touch_values();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Tampilkan ringkasan statistik
    ESP_LOGI(TAG, "\n--- Ringkasan ---");
    ESP_LOGI(TAG, "Total boot: %d", boot_count);
    ESP_LOGI(TAG, "Total touch wakeup: %d", touch_wakeup_count);
    ESP_LOGI(TAG, "Threshold: %d", TOUCH_THRESHOLD);

    // Masuk deep sleep
    enter_deep_sleep_with_touch();
}
