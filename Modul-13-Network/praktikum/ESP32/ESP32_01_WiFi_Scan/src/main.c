/**
 * ============================================================================
 * ESP32_01_WiFi_Scan - Pemindaian Jaringan WiFi (WiFi Network Scanner)
 * ============================================================================
 * 
 * Modul 13 - Network & IoT | Praktikum Sistem Embedded
 * 
 * DESKRIPSI:
 *   Program ini melakukan scanning Access Point WiFi di sekitar ESP32.
 *   Menampilkan informasi SSID, RSSI (kekuatan sinyal), channel,
 *   dan mode autentikasi dalam format tabel.
 * 
 * KONSEP YANG DIPELAJARI:
 *   - Inisialisasi WiFi subsystem ESP-IDF
 *   - NVS (Non-Volatile Storage) initialization
 *   - Network Interface (netif) initialization  
 *   - WiFi scanning API: esp_wifi_scan_start(), esp_wifi_scan_get_ap_records()
 *   - Interpretasi RSSI dan mode autentikasi
 * 
 * WIRING DIAGRAM:
 *   Tidak ada wiring tambahan - hanya ESP32 board saja.
 *   ESP32 menggunakan antena WiFi internal (onboard).
 * 
 *   [ESP32 DevKit v1]
 *   - USB --> PC (untuk serial monitor)
 * 
 * EXPECTED OUTPUT (Serial Monitor 115200 baud):
 *   ===== WiFi Scan Results =====
 *   No | SSID                 | RSSI | CH | Auth Mode
 *   ---|----------------------|------|----|----------
 *    1 | MyHomeWiFi           |  -45 |  6 | WPA2_PSK
 *    2 | OfficeNet            |  -62 | 11 | WPA2_PSK
 *    3 | OpenNetwork          |  -78 |  1 | OPEN
 *   Total AP ditemukan: 3
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

static const char *TAG = "WIFI_SCAN";

/* Jumlah maksimum AP yang akan di-record */
#define MAX_AP_COUNT 20

/* Interval scan dalam milidetik */
#define SCAN_INTERVAL_MS 10000

/**
 * Konversi auth mode enum ke string yang mudah dibaca
 * @param authmode - wifi_auth_mode_t dari ESP-IDF
 * @return string representasi auth mode
 */
static const char* get_auth_mode_str(wifi_auth_mode_t authmode)
{
    switch (authmode) {
        case WIFI_AUTH_OPEN:            return "OPEN";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
        default:                        return "UNKNOWN";
    }
}

/**
 * Konversi RSSI ke indikator kekuatan sinyal (signal quality)
 * @param rssi - nilai RSSI dalam dBm
 * @return string deskripsi kualitas sinyal
 */
static const char* get_signal_quality(int rssi)
{
    if (rssi >= -50) return "Excellent";
    if (rssi >= -60) return "Good";
    if (rssi >= -70) return "Fair";
    if (rssi >= -80) return "Weak";
    return "Very Weak";
}

/**
 * Inisialisasi WiFi dalam mode Station untuk scanning
 * Langkah: NVS -> netif -> event loop -> wifi init -> set mode STA
 */
static void wifi_init_for_scan(void)
{
    /* 1. Inisialisasi NVS - diperlukan oleh WiFi driver */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash erasing dan re-init...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized");

    /* 2. Inisialisasi TCP/IP network interface */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_LOGI(TAG, "Network interface initialized");

    /* 3. Buat default event loop (untuk WiFi events) */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 4. Buat default WiFi STA netif */
    esp_netif_create_default_wifi_sta();

    /* 5. Inisialisasi WiFi dengan config default */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 6. Set mode ke Station (STA) untuk scanning */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    /* 7. Start WiFi driver */
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi started in STA mode - ready to scan");
}

/**
 * Melakukan WiFi scan dan menampilkan hasilnya dalam format tabel
 */
static void perform_wifi_scan(void)
{
    /* Konfigurasi scan - scan semua channel, active scan */
    wifi_scan_config_t scan_config = {
        .ssid = NULL,           // Scan semua SSID
        .bssid = NULL,          // Scan semua BSSID
        .channel = 0,           // Scan semua channel (1-13)
        .show_hidden = true,    // Tampilkan hidden network juga
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    ESP_LOGI(TAG, "Memulai WiFi scan...");

    /* Mulai scan (blocking mode - true = tunggu sampai selesai) */
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan gagal: %s", esp_err_to_name(err));
        return;
    }

    /* Ambil jumlah AP yang ditemukan */
    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Scan selesai. Ditemukan %d Access Point", ap_count);

    if (ap_count == 0) {
        ESP_LOGW(TAG, "Tidak ada AP ditemukan!");
        return;
    }

    /* Batasi jumlah record yang diambil */
    uint16_t record_count = (ap_count > MAX_AP_COUNT) ? MAX_AP_COUNT : ap_count;
    wifi_ap_record_t ap_records[MAX_AP_COUNT];

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_count, ap_records));

    /* Tampilkan header tabel */
    printf("\n");
    printf("========== WiFi Scan Results ==========\n");
    printf(" No | %-24s | RSSI | CH | %-10s | Quality\n", "SSID", "Auth Mode");
    printf("----|--------------------------|------|----|------------|----------\n");

    /* Tampilkan setiap AP yang ditemukan */
    for (int i = 0; i < record_count; i++) {
        /* Handle SSID kosong (hidden network) */
        const char *ssid = (strlen((char *)ap_records[i].ssid) > 0) 
                           ? (char *)ap_records[i].ssid 
                           : "[Hidden]";

        printf(" %2d | %-24s | %4d | %2d | %-10s | %s\n",
               i + 1,
               ssid,
               ap_records[i].rssi,
               ap_records[i].primary,
               get_auth_mode_str(ap_records[i].authmode),
               get_signal_quality(ap_records[i].rssi));
    }

    printf("========================================\n");
    printf("Total AP ditemukan: %d (ditampilkan: %d)\n\n", ap_count, record_count);

    /* Log ringkasan statistik */
    int open_count = 0, secured_count = 0;
    int best_rssi = -127;
    for (int i = 0; i < record_count; i++) {
        if (ap_records[i].authmode == WIFI_AUTH_OPEN) {
            open_count++;
        } else {
            secured_count++;
        }
        if (ap_records[i].rssi > best_rssi) {
            best_rssi = ap_records[i].rssi;
        }
    }
    ESP_LOGI(TAG, "Statistik: Open=%d, Secured=%d, Best RSSI=%d dBm",
             open_count, secured_count, best_rssi);
}

/**
 * Main entry point - app_main()
 * Inisialisasi WiFi lalu scan secara berkala
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 WiFi Scanner - Modul 13 ===");
    ESP_LOGI(TAG, "Inisialisasi WiFi subsystem...");

    /* Inisialisasi WiFi untuk scanning */
    wifi_init_for_scan();

    /* Loop: scan setiap SCAN_INTERVAL_MS milidetik */
    while (1) {
        perform_wifi_scan();
        ESP_LOGI(TAG, "Scan berikutnya dalam %d detik...", SCAN_INTERVAL_MS / 1000);
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}
