/**
 * ============================================================================
 * PROJECT  : ESP32_10_WiFi_Power_Save
 * MODUL    : 14 - Power Management & Low-Power Design
 * PLATFORM : ESP-IDF
 *
 * JUDUL    : WiFi Power Save Mode (Modem Sleep)
 *
 * DESKRIPSI:
 * Demonstrasi mode penghematan daya WiFi pada ESP32:
 * - WIFI_PS_NONE      : WiFi selalu aktif (konsumsi tinggi ~120mA)
 * - WIFI_PS_MIN_MODEM : Modem tidur saat idle, bangun setiap DTIM (~20mA avg)
 * - WIFI_PS_MAX_MODEM : Modem tidur lebih lama, latency lebih tinggi (~15mA)
 *
 * Program terhubung ke WiFi lalu melakukan benchmark pada masing-masing mode.
 *
 * HARDWARE:
 * - ESP32 DevKit V1
 * - Akses WiFi (ubah SSID/Password di kode)
 *
 * CATATAN: Ubah WIFI_SSID dan WIFI_PASS sesuai jaringan anda!
 *
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "WIFI_PS";

/* ===== UBAH SESUAI JARINGAN ANDA ===== */
#define WIFI_SSID       "YourSSID"
#define WIFI_PASS       "YourPassword"
/* ====================================== */

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define MAX_RETRY           5

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry connecting... (%d/%d)", s_retry_num, MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to '%s'...", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to '%s'", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "Failed to connect!");
    }
}

static void test_power_save_mode(wifi_ps_type_t mode, const char *name)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "────────────────────────────────────────");
    ESP_LOGI(TAG, "Testing: %s", name);
    ESP_LOGI(TAG, "────────────────────────────────────────");

    esp_err_t ret = esp_wifi_set_ps(mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set power save mode: %s", esp_err_to_name(ret));
        return;
    }

    /* Verifikasi mode yang aktif */
    wifi_ps_type_t current_mode;
    esp_wifi_get_ps(&current_mode);
    ESP_LOGI(TAG, "Active PS mode: %d", current_mode);

    /* Deskripsi mode */
    switch (mode) {
        case WIFI_PS_NONE:
            ESP_LOGI(TAG, "Modem ALWAYS ON → ~120mA");
            ESP_LOGI(TAG, "Latency: Lowest | Power: Highest");
            break;
        case WIFI_PS_MIN_MODEM:
            ESP_LOGI(TAG, "Modem sleep at DTIM interval → ~20mA avg");
            ESP_LOGI(TAG, "Latency: Low | Power: Medium");
            break;
        case WIFI_PS_MAX_MODEM:
            ESP_LOGI(TAG, "Modem sleep at listen interval → ~15mA avg");
            ESP_LOGI(TAG, "Latency: Higher | Power: Lowest");
            break;
        default:
            break;
    }

    /* Simulasi aktivitas selama 5 detik */
    ESP_LOGI(TAG, "Running for 5 seconds...");
    for (int i = 0; i < 5; i++) {
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        ESP_LOGI(TAG, "  [%d] RSSI=%d dBm, Channel=%d",
                 i + 1, ap_info.rssi, ap_info.primary);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Mode '%s' test complete.", name);
}

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  WiFi Power Save Mode Demo               ║");
    ESP_LOGI(TAG, "║  Modul 14: Power Management               ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    /* NVS init (required for WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Connect WiFi */
    wifi_init_sta();

    /* Test each power save mode */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Testing WiFi Power Save modes...");
    ESP_LOGI(TAG, "Use multimeter on VCC to measure actual current");

    /* Mode 1: No power save */
    test_power_save_mode(WIFI_PS_NONE, "WIFI_PS_NONE (Always On)");

    /* Mode 2: Minimum modem sleep */
    test_power_save_mode(WIFI_PS_MIN_MODEM, "WIFI_PS_MIN_MODEM (DTIM Sleep)");

    /* Mode 3: Maximum modem sleep */
    test_power_save_mode(WIFI_PS_MAX_MODEM, "WIFI_PS_MAX_MODEM (Max Sleep)");

    /* Summary */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  WiFi Power Save Summary                      ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  NONE      : ~120mA  Low latency              ║");
    ESP_LOGI(TAG, "║  MIN_MODEM : ~20mA   Medium latency           ║");
    ESP_LOGI(TAG, "║  MAX_MODEM : ~15mA   Higher latency           ║");
    ESP_LOGI(TAG, "║  Deep Sleep: ~10µA   WiFi OFF                 ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════╝");

    /* Disconnect dan masuk deep sleep sebagai demo */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Disconnecting WiFi and entering deep sleep (10s)...");
    esp_wifi_disconnect();
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(500));

    esp_sleep_enable_timer_wakeup(10 * 1000000ULL);
    ESP_LOGI(TAG, "Deep sleep current: ~10µA (WiFi OFF completely)");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}
