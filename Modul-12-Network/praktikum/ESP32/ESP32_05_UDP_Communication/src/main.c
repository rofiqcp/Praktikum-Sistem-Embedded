/**
 * ============================================================================
 * ESP32_05_UDP_Communication
 * Modul 13 - Network & IoT: UDP Datagram Communication
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan komunikasi UDP (User Datagram Protocol)
 *   pada ESP32 menggunakan ESP-IDF framework. Fitur meliputi:
 *   - Koneksi WiFi Station mode
 *   - Membuat UDP socket untuk kirim/terima datagram
 *   - Unicast: kirim ke IP tertentu
 *   - Broadcast: kirim ke 255.255.255.255
 *   - Menggunakan sendto() dan recvfrom()
 * 
 * Wiring Diagram:
 *   Tidak ada wiring khusus - menggunakan WiFi internal ESP32.
 *   Pastikan ESP32 terhubung ke jaringan WiFi yang sama dengan PC.
 * 
 *   [ESP32] ~~~WiFi~~~ [Router] --- [PC / UDP Client]
 * 
 * Expected Output:
 *   I (xxxx) UDP: WiFi connected, IP: 192.168.x.x
 *   I (xxxx) UDP: Socket created successfully
 *   I (xxxx) UDP: Broadcast sent: "ESP32 Broadcast #1"
 *   I (xxxx) UDP: Received 20 bytes from 192.168.x.x:12345
 *   I (xxxx) UDP: Data: Hello from PC
 * 
 * Konfigurasi:
 *   - Ubah WIFI_SSID dan WIFI_PASS sesuai jaringan Anda
 *   - UDP_PORT default: 3333
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

/* ======================== KONFIGURASI ======================== */
#define WIFI_SSID           "YourWiFiSSID"          // Ganti dengan SSID WiFi Anda
#define WIFI_PASS           "YourWiFiPassword"      // Ganti dengan password WiFi
#define WIFI_MAX_RETRY      5                       // Maksimum percobaan koneksi

#define UDP_PORT            3333                    // Port UDP untuk komunikasi
#define UDP_BROADCAST_PORT  3334                    // Port untuk broadcast
#define RX_BUFFER_SIZE      256                     // Ukuran buffer penerima

static const char *TAG = "UDP";

/* Event group untuk sinkronisasi WiFi */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static int s_retry_num = 0;
static char s_device_ip[16] = {0};  // Menyimpan IP address ESP32

/* ======================== WIFI EVENT HANDLER ======================== */
/**
 * Handler untuk event WiFi dan IP
 * Menangani: connect, disconnect, got_ip
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retry koneksi WiFi... (%d/%d)", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Gagal konek WiFi setelah %d percobaan", WIFI_MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(s_device_ip, sizeof(s_device_ip), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "WiFi connected, IP: %s", s_device_ip);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ======================== INISIALISASI WIFI ======================== */
/**
 * Inisialisasi WiFi dalam mode Station (STA)
 * Return: ESP_OK jika berhasil konek
 */
static esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register event handlers */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL, &instance_got_ip));

    /* Konfigurasi WiFi */
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

    ESP_LOGI(TAG, "WiFi init selesai, menunggu koneksi...");

    /* Tunggu sampai konek atau gagal */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                       pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

/* ======================== UDP BROADCAST TASK ======================== */
/**
 * Task untuk mengirim broadcast UDP secara periodik
 * Broadcast ke alamat 255.255.255.255 agar semua device di jaringan menerima
 */
static void udp_broadcast_task(void *pvParameters)
{
    /* Buat socket UDP */
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Gagal membuat broadcast socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    /* Aktifkan opsi broadcast pada socket */
    int broadcast_enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    /* Alamat tujuan broadcast */
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);  // 255.255.255.255
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_BROADCAST_PORT);

    int msg_count = 0;
    char tx_buffer[128];

    while (1) {
        msg_count++;
        snprintf(tx_buffer, sizeof(tx_buffer),
                 "{\"src\":\"%s\",\"type\":\"broadcast\",\"seq\":%d,\"heap\":%lu}",
                 s_device_ip, msg_count, (unsigned long)esp_get_free_heap_size());

        int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Broadcast gagal: errno %d", errno);
        } else {
            ESP_LOGI(TAG, "Broadcast #%d sent (%d bytes): %s", msg_count, err, tx_buffer);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));  // Kirim setiap 5 detik
    }

    close(sock);
    vTaskDelete(NULL);
}

/* ======================== UDP SERVER TASK ======================== */
/**
 * Task utama UDP server: mendengarkan datagram masuk dan membalas
 * Menggunakan recvfrom() untuk menerima data beserta info pengirim
 */
static void udp_server_task(void *pvParameters)
{
    char rx_buffer[RX_BUFFER_SIZE];
    char tx_buffer[RX_BUFFER_SIZE];

    /* Buat socket UDP */
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Gagal membuat socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created successfully");

    /* Bind socket ke port */
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);

    int err = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Bind gagal: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket bound to port %d - Menunggu datagram...", UDP_PORT);

    /* Set timeout agar recvfrom tidak blocking selamanya */
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1) {
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);

        /* Terima datagram - recvfrom() mengembalikan data dan alamat pengirim */
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                           (struct sockaddr *)&source_addr, &addr_len);

        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Timeout - tidak ada data, lanjut loop */
                ESP_LOGI(TAG, "Waiting for UDP data on port %d...", UDP_PORT);
                continue;
            }
            ESP_LOGE(TAG, "recvfrom error: errno %d", errno);
            break;
        }

        /* Data diterima - proses */
        rx_buffer[len] = '\0';  // Null-terminate string

        /* Ambil IP address pengirim */
        char sender_ip[16];
        inet_ntoa_r(source_addr.sin_addr, sender_ip, sizeof(sender_ip));
        int sender_port = ntohs(source_addr.sin_port);

        ESP_LOGI(TAG, "Received %d bytes from %s:%d", len, sender_ip, sender_port);
        ESP_LOGI(TAG, "Data: %s", rx_buffer);

        /* Kirim balasan (echo + info) menggunakan sendto() */
        char echo_part[64];
        int echo_len = len < 60 ? len : 60;
        memcpy(echo_part, rx_buffer, echo_len);
        echo_part[echo_len] = '\0';
        snprintf(tx_buffer, sizeof(tx_buffer),
                 "ACK from ESP32 (%s): Received '%s' (%d bytes)",
                 s_device_ip, echo_part, len);

        int sent = sendto(sock, tx_buffer, strlen(tx_buffer), 0,
                          (struct sockaddr *)&source_addr, addr_len);
        if (sent < 0) {
            ESP_LOGE(TAG, "sendto gagal: errno %d", errno);
        } else {
            ESP_LOGI(TAG, "Reply sent to %s:%d (%d bytes)", sender_ip, sender_port, sent);
        }
    }

    close(sock);
    ESP_LOGW(TAG, "UDP server task ended");
    vTaskDelete(NULL);
}

/* ======================== MAIN APP ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 UDP Communication Demo ===");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());

    /* Inisialisasi NVS - diperlukan oleh WiFi */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Koneksi WiFi */
    ESP_LOGI(TAG, "Menginisialisasi WiFi...");
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi gagal! Program berhenti.");
        return;
    }

    /* Mulai task UDP */
    ESP_LOGI(TAG, "Memulai UDP tasks...");
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
    xTaskCreate(udp_broadcast_task, "udp_broadcast", 4096, NULL, 4, NULL);

    /* Main loop - monitoring */
    while (1) {
        ESP_LOGI(TAG, "[Monitor] Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(30000));  // Log setiap 30 detik
    }
}
