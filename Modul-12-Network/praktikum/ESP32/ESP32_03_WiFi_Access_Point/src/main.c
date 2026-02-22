/**
 * ============================================================================
 * ESP32_03_WiFi_Access_Point - Membuat Soft-AP (Access Point Mode)
 * ============================================================================
 * 
 * Modul 13 - Network & IoT | Praktikum Sistem Embedded
 * 
 * DESKRIPSI:
 *   Program ini membuat ESP32 menjadi WiFi Access Point (Soft-AP).
 *   Client (HP/laptop) dapat terhubung ke jaringan yang dibuat ESP32.
 *   DHCP server berjalan otomatis untuk memberikan IP ke client.
 *   Program memonitor koneksi/diskoneksi client dan menampilkan
 *   MAC address serta IP address client.
 * 
 * KONSEP YANG DIPELAJARI:
 *   - WiFi Access Point (Soft-AP) mode
 *   - DHCP Server konfigurasi
 *   - Event handling untuk AP_STACONNECTED / AP_STADISCONNECTED
 *   - MAC address tracking dari connected clients
 *   - Network interface konfigurasi
 * 
 * WIRING DIAGRAM:
 *   Tidak ada wiring tambahan - ESP32 board saja.
 * 
 *   [ESP32 DevKit v1]         [Client Device]
 *   - Soft-AP aktif    <----> HP/Laptop connect ke SSID ESP32
 *   - USB --> PC              (untuk serial monitor)
 * 
 * CARA PENGUJIAN:
 *   1. Flash program ke ESP32
 *   2. Buka serial monitor (115200 baud)
 *   3. Cari SSID "ESP32_AP_Test" di HP/laptop
 *   4. Connect dengan password "esp32pass"
 *   5. Perhatikan log di serial monitor
 * 
 * EXPECTED OUTPUT:
 *   I (xxx) WIFI_AP: ESP32 Access Point started!
 *   I (xxx) WIFI_AP:   SSID     : ESP32_AP_Test
 *   I (xxx) WIFI_AP:   Password : esp32pass
 *   I (xxx) WIFI_AP:   Channel  : 1
 *   I (xxx) WIFI_AP:   IP       : 192.168.4.1
 *   I (xxx) WIFI_AP: Client connected! MAC: aa:bb:cc:dd:ee:ff
 *   I (xxx) WIFI_AP: Client assigned IP: 192.168.4.2
 * 
 * AUTHOR: Praktikum Sistem Embedded
 * DATE: 2026
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "lwip/inet.h"

static const char *TAG = "WIFI_AP";

/* ===== KONFIGURASI ACCESS POINT ===== */
#define AP_SSID             "ESP32_AP_Test"     // Nama jaringan WiFi
#define AP_PASS             "esp32pass"         // Password (min 8 karakter)
#define AP_CHANNEL          1                   // Channel WiFi (1-13)
#define AP_MAX_CONNECTIONS  4                   // Maks client yang bisa connect
/* ===================================== */

/* Tracking connected clients */
static int s_connected_clients = 0;

/**
 * Event handler untuk Access Point events
 * Menangani koneksi dan diskoneksi client
 */
static void wifi_ap_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "Access Point STARTED");
                break;

            case WIFI_EVENT_AP_STOP:
                ESP_LOGW(TAG, "Access Point STOPPED");
                break;

            case WIFI_EVENT_AP_STACONNECTED: {
                /* Client baru terhubung */
                wifi_event_ap_staconnected_t *event = 
                    (wifi_event_ap_staconnected_t *)event_data;
                s_connected_clients++;
                ESP_LOGI(TAG, "========== Client CONNECTED ==========");
                ESP_LOGI(TAG, "  MAC Address : " MACSTR, MAC2STR(event->mac));
                ESP_LOGI(TAG, "  AID         : %d", event->aid);
                ESP_LOGI(TAG, "  Total Clients: %d", s_connected_clients);
                ESP_LOGI(TAG, "=======================================");
                break;
            }

            case WIFI_EVENT_AP_STADISCONNECTED: {
                /* Client disconnect */
                wifi_event_ap_stadisconnected_t *event = 
                    (wifi_event_ap_stadisconnected_t *)event_data;
                s_connected_clients--;
                if (s_connected_clients < 0) s_connected_clients = 0;
                ESP_LOGW(TAG, "========= Client DISCONNECTED =========");
                ESP_LOGW(TAG, "  MAC Address : " MACSTR, MAC2STR(event->mac));
                ESP_LOGW(TAG, "  AID         : %d", event->aid);
                ESP_LOGW(TAG, "  Total Clients: %d", s_connected_clients);
                ESP_LOGW(TAG, "=======================================");
                break;
            }

            default:
                break;
        }
    }

    /* IP event - client mendapat IP dari DHCP */
    if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
            ip_event_ap_staipassigned_t *event = 
                (ip_event_ap_staipassigned_t *)event_data;
            ESP_LOGI(TAG, "Client assigned IP: " IPSTR, IP2STR(&event->ip));
        }
    }
}

/**
 * Inisialisasi dan konfigurasi WiFi Access Point
 */
static void wifi_ap_init(void)
{
    /* 1. Init NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    /* 2. Init network stack */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 3. Buat default AP netif (termasuk DHCP server) */
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    
    /* Konfigurasi IP address untuk AP (default: 192.168.4.1) */
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    /* 4. Init WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 5. Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ap_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &wifi_ap_event_handler, NULL, NULL));

    /* 6. Konfigurasi AP */
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASS,
            .max_connection = AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK,     // WPA2 security
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    /* Jika password kosong, set OPEN mode */
    if (strlen(AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        ESP_LOGW(TAG, "Password kosong - menggunakan OPEN mode (tidak aman!)");
    }

    /* 7. Set mode AP dan apply konfigurasi */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    /* 8. Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Tampilkan info AP */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Access Point Started!        ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  SSID     : %-24s║", AP_SSID);
    ESP_LOGI(TAG, "║  Password : %-24s║", AP_PASS);
    ESP_LOGI(TAG, "║  Channel  : %-24d║", AP_CHANNEL);
    ESP_LOGI(TAG, "║  Max Conn : %-24d║", AP_MAX_CONNECTIONS);
    ESP_LOGI(TAG, "║  IP       : 192.168.4.1              ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Silakan connect perangkat ke SSID: %s", AP_SSID);
}

/**
 * Task monitoring - tampilkan status AP secara berkala
 */
static void ap_monitor_task(void *pvParameters)
{
    wifi_sta_list_t sta_list;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // Setiap 10 detik

        ESP_LOGI(TAG, "--- AP Status ---");
        ESP_LOGI(TAG, "Connected clients: %d", s_connected_clients);

        /* Dapatkan list client yang terhubung */
        if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK) {
            for (int i = 0; i < sta_list.num; i++) {
                ESP_LOGI(TAG, "  Client %d: MAC=" MACSTR " | RSSI=%d",
                         i + 1,
                         MAC2STR(sta_list.sta[i].mac),
                         sta_list.sta[i].rssi);
            }
        }

        ESP_LOGI(TAG, "Free heap: %lu bytes",
                 (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "-----------------");
    }
}

/**
 * Main entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 WiFi Access Point - Modul 13 ===");

    /* Inisialisasi AP */
    wifi_ap_init();

    /* Buat monitoring task */
    xTaskCreate(ap_monitor_task, "ap_monitor", 4096, NULL, 5, NULL);

    /* Main loop */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
