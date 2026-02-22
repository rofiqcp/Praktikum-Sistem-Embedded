/**
 * ============================================================================
 * ESP32_02_WiFi_Station - Koneksi ke WiFi Access Point (Station Mode)
 * ============================================================================
 * 
 * Modul 13 - Network & IoT | Praktikum Sistem Embedded
 * 
 * DESKRIPSI:
 *   Program ini menghubungkan ESP32 ke WiFi Access Point menggunakan
 *   mode Station (STA). Menangani event koneksi, disconnection,
 *   dan mendapatkan IP address via DHCP. Termasuk retry logic
 *   dengan maximum retry count.
 * 
 * KONSEP YANG DIPELAJARI:
 *   - WiFi Station mode connection
 *   - Event-driven programming (WiFi & IP events)
 *   - DHCP client untuk mendapatkan IP address
 *   - FreeRTOS Event Groups untuk sinkronisasi
 *   - Error handling dan retry mechanism
 * 
 * WIRING DIAGRAM:
 *   Tidak ada wiring tambahan - ESP32 board saja.
 *   Pastikan ada WiFi AP yang bisa dijangkau.
 * 
 *   [ESP32 DevKit v1]
 *   - USB --> PC (serial monitor)
 *   - Pastikan WiFi AP aktif dengan SSID & password yang sesuai
 * 
 * KONFIGURASI:
 *   Ubah WIFI_SSID dan WIFI_PASS sesuai jaringan WiFi Anda!
 * 
 * EXPECTED OUTPUT (Serial Monitor 115200 baud):
 *   I (xxx) WIFI_STA: Connecting to SSID: MyNetwork ...
 *   I (xxx) WIFI_STA: WIFI_EVENT_STA_CONNECTED
 *   I (xxx) WIFI_STA: Got IP Address: 192.168.1.100
 *   I (xxx) WIFI_STA: Netmask: 255.255.255.0
 *   I (xxx) WIFI_STA: Gateway: 192.168.1.1
 *   I (xxx) WIFI_STA: WiFi Connected Successfully!
 * 
 * AUTHOR: Praktikum Sistem Embedded
 * DATE: 2026
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
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "WIFI_STA";

/* ===== KONFIGURASI WiFi - UBAH SESUAI JARINGAN ANDA ===== */
#define WIFI_SSID       "YourSSID"          // Ganti dengan SSID WiFi Anda
#define WIFI_PASS       "YourPassword"      // Ganti dengan password WiFi Anda
#define MAX_RETRY       5                   // Maksimum percobaan koneksi
/* ========================================================= */

/* Event group bits untuk sinkronisasi status koneksi */
#define WIFI_CONNECTED_BIT  BIT0    // Berhasil terhubung & dapat IP
#define WIFI_FAIL_BIT       BIT1    // Gagal setelah MAX_RETRY

/* Event group handle */
static EventGroupHandle_t s_wifi_event_group;

/* Retry counter */
static int s_retry_count = 0;

/**
 * Event handler untuk WiFi dan IP events
 * Dipanggil otomatis oleh event loop saat terjadi event WiFi/IP
 * 
 * @param arg - user data (tidak digunakan)
 * @param event_base - base event (WIFI_EVENT atau IP_EVENT)
 * @param event_id - ID event spesifik
 * @param event_data - data event (tergantung event type)
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    /* === WiFi Events === */
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                /* WiFi driver sudah started, mulai connect */
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START - Memulai koneksi...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED - Terhubung ke AP!");
                ESP_LOGI(TAG, "Menunggu IP address dari DHCP...");
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *event = 
                    (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGW(TAG, "WIFI_EVENT_STA_DISCONNECTED (reason: %d)", 
                         event->reason);

                /* Retry logic */
                if (s_retry_count < MAX_RETRY) {
                    s_retry_count++;
                    ESP_LOGI(TAG, "Retry koneksi (%d/%d)...", 
                             s_retry_count, MAX_RETRY);
                    esp_wifi_connect();
                } else {
                    ESP_LOGE(TAG, "Gagal connect setelah %d percobaan!", MAX_RETRY);
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                break;
            }

            default:
                ESP_LOGI(TAG, "WiFi event tidak ditangani: %ld", event_id);
                break;
        }
    }

    /* === IP Events === */
    if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "===== Got IP Address =====");
                ESP_LOGI(TAG, "  IP Address : " IPSTR, IP2STR(&event->ip_info.ip));
                ESP_LOGI(TAG, "  Netmask    : " IPSTR, IP2STR(&event->ip_info.netmask));
                ESP_LOGI(TAG, "  Gateway    : " IPSTR, IP2STR(&event->ip_info.gw));
                ESP_LOGI(TAG, "==========================");
                s_retry_count = 0;  // Reset counter
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                break;
            }

            case IP_EVENT_STA_LOST_IP:
                ESP_LOGW(TAG, "IP_EVENT_STA_LOST_IP - IP address hilang!");
                break;

            default:
                break;
        }
    }
}

/**
 * Inisialisasi dan koneksi WiFi Station
 * Return: ESP_OK jika berhasil connect, ESP_FAIL jika gagal
 */
static esp_err_t wifi_station_init(void)
{
    /* Buat event group untuk sinkronisasi */
    s_wifi_event_group = xEventGroupCreate();

    /* 1. Init NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. Init netif dan event loop */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* 3. Init WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 4. Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    /* 5. Konfigurasi WiFi STA */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,  // Minimum auth mode
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    /* 6. Start WiFi (akan trigger WIFI_EVENT_STA_START) */
    ESP_LOGI(TAG, "Connecting to SSID: %s ...", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 7. Tunggu hasil koneksi (blocking) */
    ESP_LOGI(TAG, "Menunggu hasil koneksi...");
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,       // Jangan clear bits saat return
        pdFALSE,       // Wait for ANY bit (bukan ALL)
        portMAX_DELAY  // Tunggu tanpa batas waktu
    );

    /* 8. Cek hasil */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi Connected Successfully to '%s'!", WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi Connection FAILED to '%s'", WIFI_SSID);
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    return ESP_FAIL;
}

/**
 * Task untuk monitoring status WiFi secara berkala
 */
static void wifi_monitor_task(void *pvParameters)
{
    wifi_ap_record_t ap_info;
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "[Monitor] SSID: %s | RSSI: %d dBm | Channel: %d",
                     ap_info.ssid, ap_info.rssi, ap_info.primary);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * Main entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 WiFi Station - Modul 13 ===");

    /* Inisialisasi dan koneksi WiFi */
    esp_err_t result = wifi_station_init();

    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Koneksi berhasil! Memulai monitoring...");
        /* Buat task monitoring */
        xTaskCreate(wifi_monitor_task, "wifi_monitor", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGE(TAG, "Koneksi gagal. Restart ESP32 untuk mencoba lagi.");
    }

    /* Main loop - bisa ditambahkan logika aplikasi di sini */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "[Main] Sistem berjalan... heap free: %lu bytes",
                 (unsigned long)esp_get_free_heap_size());
    }
}
