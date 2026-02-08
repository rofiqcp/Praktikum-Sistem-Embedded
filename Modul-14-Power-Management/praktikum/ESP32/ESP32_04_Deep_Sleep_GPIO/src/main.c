/**
 * ==========================================================================
 * ESP32_04_Deep_Sleep_GPIO - Deep Sleep dengan GPIO Wakeup (ext0/ext1)
 * ==========================================================================
 * 
 * Modul 14 - Power Management
 * Program 4: Deep sleep with GPIO wakeup using ext0 and ext1
 * 
 * KONSEP:
 * - ext0: Wakeup dari SATU GPIO menggunakan RTC controller
 *   → Hanya GPIO yang terhubung ke RTC (GPIO 0,2,4,12-15,25-27,32-39)
 *   → Bisa wake on HIGH atau LOW
 * 
 * - ext1: Wakeup dari BEBERAPA GPIO sekaligus
 *   → Bisa dikonfigurasi ANY_HIGH atau ALL_LOW
 *   → Bisa mendeteksi GPIO mana yang memicu wakeup
 * 
 * WIRING / KONEKSI:
 * ┌─────────────────────────────────────────────────┐
 * │  ESP32          Komponen                         │
 * │  GPIO2  ──────► LED (+) ──► R(220Ω) ──► GND     │
 * │  GPIO33 ──┬──► Push Button 1 ──► GND (ext0)     │
 * │           └──► R(10kΩ) pull-up ke 3.3V           │
 * │  GPIO32 ──┬──► Push Button 2 ──► GND (ext1)     │
 * │           └──► R(10kΩ) pull-up ke 3.3V           │
 * │  GPIO25 ──┬──► Push Button 3 ──► GND (ext1)     │
 * │           └──► R(10kΩ) pull-up ke 3.3V           │
 * └─────────────────────────────────────────────────┘
 * 
 * CATATAN PENTING:
 * - ext0 dan ext1 tidak bisa digunakan BERSAMAAN
 * - Program ini mendemo keduanya secara bergantian
 * - Push button ditekan → GPIO LOW → trigger wakeup
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * [GPIO_WAKE] Boot count: 2
 * [GPIO_WAKE] Wakeup cause: EXT0 (GPIO33)
 * [GPIO_WAKE] Mode: ext0 on GPIO33
 * [GPIO_WAKE] Press button on GPIO33 to wake from deep sleep
 * ========================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "soc/rtc.h"

static const char *TAG = "GPIO_WAKE";

/* Konfigurasi Pin */
#define LED_PIN         GPIO_NUM_2      // LED indikator
#define EXT0_GPIO       GPIO_NUM_33     // Push button 1 (ext0 wakeup)
#define EXT1_GPIO_A     GPIO_NUM_32     // Push button 2 (ext1 wakeup)
#define EXT1_GPIO_B     GPIO_NUM_25     // Push button 3 (ext1 wakeup)

/* RTC memory variables - bertahan selama deep sleep */
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static int ext0_wakeup_count = 0;
RTC_DATA_ATTR static int ext1_wakeup_count = 0;
RTC_DATA_ATTR static int timer_wakeup_count = 0;
RTC_DATA_ATTR static int use_ext1_mode = 0;  // Toggle antara ext0 dan ext1

/**
 * Inisialisasi GPIO untuk LED
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
 * Dapatkan string penyebab wakeup
 */
static const char* get_wakeup_reason_str(esp_sleep_wakeup_cause_t cause)
{
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:      return "EXT0 (single GPIO)";
        case ESP_SLEEP_WAKEUP_EXT1:      return "EXT1 (multi GPIO)";
        case ESP_SLEEP_WAKEUP_TIMER:     return "TIMER (backup)";
        case ESP_SLEEP_WAKEUP_UNDEFINED: return "POWER ON / RESET";
        default:                         return "UNKNOWN";
    }
}

/**
 * Analisis ext1 wakeup: GPIO mana yang memicu
 * Bitmask dari esp_sleep_get_ext1_wakeup_status() menunjukkan GPIO penyebab
 */
static void analyze_ext1_wakeup(void)
{
    uint64_t wakeup_status = esp_sleep_get_ext1_wakeup_status();

    if (wakeup_status == 0) {
        ESP_LOGW(TAG, "ext1 wakeup status: 0 (tidak terdeteksi GPIO spesifik)");
        return;
    }

    /* Cari GPIO mana yang triggered */
    int gpio_num = __builtin_ffsll(wakeup_status) - 1;
    ESP_LOGI(TAG, "ext1 wakeup status bitmask: 0x%llx", wakeup_status);
    ESP_LOGI(TAG, "GPIO yang memicu wakeup: GPIO%d", gpio_num);

    /* Cek setiap GPIO yang dikonfigurasi */
    if (wakeup_status & (1ULL << EXT1_GPIO_A)) {
        ESP_LOGI(TAG, "  → GPIO%d (Button 2) triggered!", EXT1_GPIO_A);
    }
    if (wakeup_status & (1ULL << EXT1_GPIO_B)) {
        ESP_LOGI(TAG, "  → GPIO%d (Button 3) triggered!", EXT1_GPIO_B);
    }
}

/**
 * LED blink pattern
 */
static void blink_led(int count, int delay_ms)
{
    for (int i = 0; i < count; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * Konfigurasi ext0 wakeup (single GPIO)
 * GPIO33 dengan internal pull-up, wake on LOW (button pressed)
 */
static void configure_ext0_wakeup(void)
{
    ESP_LOGI(TAG, "Konfigurasi ext0 wakeup pada GPIO%d", EXT0_GPIO);
    ESP_LOGI(TAG, "  → Wake on LOW (tekan button untuk wake)");
    ESP_LOGI(TAG, "  → Internal pull-up enabled");

    /* Enable pull-up pada RTC GPIO */
    rtc_gpio_pullup_en(EXT0_GPIO);
    rtc_gpio_pulldown_dis(EXT0_GPIO);

    /* Konfigurasi ext0: wake saat GPIO LOW (0) */
    esp_err_t ret = esp_sleep_enable_ext0_wakeup(EXT0_GPIO, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal konfigurasi ext0: %s", esp_err_to_name(ret));
    }
}

/**
 * Konfigurasi ext1 wakeup (multiple GPIO)
 * GPIO32 + GPIO25, wake ketika salah satu LOW (ALL_LOW mode)
 */
static void configure_ext1_wakeup(void)
{
    ESP_LOGI(TAG, "Konfigurasi ext1 wakeup pada GPIO%d + GPIO%d", EXT1_GPIO_A, EXT1_GPIO_B);
    ESP_LOGI(TAG, "  → Mode: ANY_LOW (tekan salah satu button)");
    ESP_LOGI(TAG, "  → Internal pull-up enabled");

    /* Enable pull-up pada RTC GPIOs */
    rtc_gpio_pullup_en(EXT1_GPIO_A);
    rtc_gpio_pulldown_dis(EXT1_GPIO_A);
    rtc_gpio_pullup_en(EXT1_GPIO_B);
    rtc_gpio_pulldown_dis(EXT1_GPIO_B);

    /* Bitmask: GPIO32 = bit 32, GPIO25 = bit 25 */
    uint64_t ext1_mask = (1ULL << EXT1_GPIO_A) | (1ULL << EXT1_GPIO_B);

    /* ESP_EXT1_WAKEUP_ALL_LOW: wake ketika SEMUA pin menjadi LOW
     * Dengan pull-up + button to GND, ini berarti tekan SALAH SATU button */
    esp_err_t ret = esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal konfigurasi ext1: %s", esp_err_to_name(ret));
    }
}

/**
 * Entry point utama
 */
void app_main(void)
{
    int64_t wake_time = esp_timer_get_time();
    boot_count++;

    init_led();

    /* Dapatkan penyebab wakeup */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "  ESP32 DEEP SLEEP - GPIO Wakeup (ext0/ext1)");
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "Boot count: %d", boot_count);
    ESP_LOGI(TAG, "Wakeup cause: %s", get_wakeup_reason_str(cause));

    /* Analisis penyebab wakeup */
    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ext0_wakeup_count++;
            ESP_LOGI(TAG, "ext0 wakeup dari GPIO%d!", EXT0_GPIO);
            blink_led(2, 100);  // 2 blink cepat = ext0
            break;

        case ESP_SLEEP_WAKEUP_EXT1:
            ext1_wakeup_count++;
            ESP_LOGI(TAG, "ext1 wakeup terdeteksi!");
            analyze_ext1_wakeup();
            blink_led(3, 100);  // 3 blink cepat = ext1
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            timer_wakeup_count++;
            ESP_LOGW(TAG, "Backup timer wakeup (tidak ada button ditekan)");
            blink_led(1, 500);  // 1 blink lambat = timer
            break;

        default:
            ESP_LOGI(TAG, "First boot / power on reset");
            blink_led(5, 100);  // 5 blink = first boot
            break;
    }

    /* Statistik wakeup */
    ESP_LOGI(TAG, "--- Wakeup Statistics ---");
    ESP_LOGI(TAG, "ext0 wakeups: %d", ext0_wakeup_count);
    ESP_LOGI(TAG, "ext1 wakeups: %d", ext1_wakeup_count);
    ESP_LOGI(TAG, "Timer wakeups: %d", timer_wakeup_count);

    /* Toggle antara mode ext0 dan ext1 setiap boot */
    use_ext1_mode = !use_ext1_mode;

    int active_ms = (int)((esp_timer_get_time() - wake_time) / 1000);

    /* Data terformat untuk Python */
    const char *mode_str = use_ext1_mode ? "ext1" : "ext0";
    ESP_LOGI(TAG, "DATA,%d,%s,%d,%d,%d,%d",
             boot_count, get_wakeup_reason_str(cause),
             ext0_wakeup_count, ext1_wakeup_count,
             timer_wakeup_count, active_ms);

    /* Konfigurasi wakeup untuk siklus berikutnya */
    ESP_LOGI(TAG, "");
    if (use_ext1_mode) {
        ESP_LOGI(TAG, ">>> Mode berikutnya: ext1 (multi GPIO)");
        ESP_LOGI(TAG, ">>> Tekan button GPIO%d atau GPIO%d untuk wake", EXT1_GPIO_A, EXT1_GPIO_B);
        configure_ext1_wakeup();
    } else {
        ESP_LOGI(TAG, ">>> Mode berikutnya: ext0 (single GPIO)");
        ESP_LOGI(TAG, ">>> Tekan button GPIO%d untuk wake", EXT0_GPIO);
        configure_ext0_wakeup();
    }

    /* Backup timer wakeup (30 detik) jika button tidak ditekan */
    esp_sleep_enable_timer_wakeup(30 * 1000000ULL);
    ESP_LOGI(TAG, "Backup timer: 30 seconds (jika button tidak ditekan)");

    ESP_LOGI(TAG, "Entering deep sleep... (tekan button untuk wake!)");
    ESP_LOGI(TAG, "================================================");

    /* Flush dan masuk deep sleep */
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(200));

    esp_deep_sleep_start();
}
