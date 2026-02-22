/**
 * ============================================================================
 * ESP32_04_TCP_Client_Server - TCP Socket (Client & Server) via lwIP
 * ============================================================================
 * 
 * Modul 13 - Network & IoT | Praktikum Sistem Embedded
 * 
 * DESKRIPSI:
 *   Program ini mendemonstrasikan penggunaan TCP socket pada ESP32:
 *   1. Connect ke WiFi AP terlebih dahulu
 *   2. Menjalankan TCP Server di port 8080 (menerima & echo data)
 *   3. Menjalankan TCP Client yang connect ke server eksternal
 *   Menggunakan lwIP BSD socket API (socket, bind, listen, accept, etc.)
 * 
 * KONSEP YANG DIPELAJARI:
 *   - TCP/IP Socket programming (BSD Sockets API)
 *   - Server: socket() -> bind() -> listen() -> accept() -> recv()/send()
 *   - Client: socket() -> connect() -> send()/recv()
 *   - Multi-threaded server (FreeRTOS tasks)
 *   - Error handling pada socket operations
 * 
 * WIRING DIAGRAM:
 *   Tidak ada wiring tambahan.
 * 
 *   [ESP32 DevKit v1]  <--WiFi-->  [Router/AP]  <-->  [PC/Client]
 *   - USB --> PC (serial monitor)
 *   - ESP32 jadi TCP server di port 8080
 *   - PC bisa connect pakai: nc <esp32_ip> 8080
 *     atau pakai script debug_analysis.py
 * 
 * KONFIGURASI:
 *   Ubah WIFI_SSID, WIFI_PASS sesuai jaringan Anda!
 *   TCP_SERVER_PORT = 8080 (bisa diubah)
 *   Untuk TCP client demo, ubah REMOTE_SERVER_IP
 * 
 * EXPECTED OUTPUT:
 *   I (xxx) TCP: WiFi Connected, IP: 192.168.1.100
 *   I (xxx) TCP: TCP Server listening on port 8080
 *   I (xxx) TCP: Client connected from 192.168.1.50:12345
 *   I (xxx) TCP: Received 12 bytes: "Hello ESP32"
 *   I (xxx) TCP: Echo sent back to client
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

/* BSD Socket headers (lwIP) */
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"

static const char *TAG = "TCP";

/* ===== KONFIGURASI ===== */
#define WIFI_SSID           "YourSSID"          // Ganti dengan SSID Anda
#define WIFI_PASS           "YourPassword"      // Ganti dengan password Anda
#define MAX_RETRY           5

#define TCP_SERVER_PORT     8080                // Port untuk TCP server
#define TCP_RX_BUFFER_SIZE  1024                // Ukuran buffer receive
#define MAX_TCP_CLIENTS     3                   // Maks concurrent clients

/* Untuk demo TCP client (opsional) */
#define REMOTE_SERVER_IP    "192.168.1.100"     // IP server tujuan
#define REMOTE_SERVER_PORT  8080                // Port server tujuan
/* ======================== */

/* Event group dan retry counter untuk WiFi */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
static int s_retry_count = 0;

/* Flag WiFi connected */
static bool s_wifi_connected = false;

/**
 * WiFi event handler - sama seperti modul Station
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            s_wifi_connected = false;
            if (s_retry_count < MAX_RETRY) {
                s_retry_count++;
                ESP_LOGW(TAG, "WiFi retry %d/%d...", s_retry_count, MAX_RETRY);
                esp_wifi_connect();
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi Connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        s_wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * Inisialisasi WiFi Station dan tunggu koneksi
 * @return ESP_OK jika berhasil, ESP_FAIL jika gagal
 */
static esp_err_t wifi_init_and_connect(void)
{
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

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

    ESP_LOGI(TAG, "Connecting to WiFi '%s'...", WIFI_SSID);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}

/**
 * Handle satu client TCP yang terhubung (echo server)
 * Berjalan di task terpisah untuk setiap client
 */
static void tcp_client_handler_task(void *pvParameters)
{
    int client_sock = (int)(intptr_t)pvParameters;
    char rx_buffer[TCP_RX_BUFFER_SIZE];
    int total_bytes = 0;

    ESP_LOGI(TAG, "[Handler] Handling client on socket %d", client_sock);

    while (1) {
        /* Terima data dari client */
        int len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len < 0) {
            ESP_LOGE(TAG, "[Handler] recv() error: errno=%d", errno);
            break;
        } else if (len == 0) {
            /* Client disconnect (connection closed) */
            ESP_LOGI(TAG, "[Handler] Client disconnected (socket %d)", client_sock);
            break;
        }

        /* Data diterima - null terminate untuk logging */
        rx_buffer[len] = '\0';
        total_bytes += len;
        ESP_LOGI(TAG, "[Handler] Received %d bytes: \"%s\"", len, rx_buffer);

        /* Echo data kembali ke client */
        int sent = send(client_sock, rx_buffer, len, 0);
        if (sent < 0) {
            ESP_LOGE(TAG, "[Handler] send() error: errno=%d", errno);
            break;
        }
        ESP_LOGI(TAG, "[Handler] Echo sent back (%d bytes)", sent);
    }

    ESP_LOGI(TAG, "[Handler] Total bytes processed: %d", total_bytes);

    /* Tutup socket client */
    shutdown(client_sock, SHUT_RDWR);
    close(client_sock);

    /* Hapus task ini */
    vTaskDelete(NULL);
}

/**
 * TCP Server task - listen pada port TCP_SERVER_PORT
 * Menerima koneksi client dan spawn handler task
 */
static void tcp_server_task(void *pvParameters)
{
    /* 1. Buat socket TCP */
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "[Server] socket() gagal: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "[Server] Socket created");

    /* Set socket option - reuse address */
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Bind ke port */
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(TCP_SERVER_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),  // Bind ke semua interface
    };

    int err = bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "[Server] bind() gagal: errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "[Server] Bound to port %d", TCP_SERVER_PORT);

    /* 3. Listen untuk koneksi */
    err = listen(listen_sock, MAX_TCP_CLIENTS);
    if (err != 0) {
        ESP_LOGE(TAG, "[Server] listen() gagal: errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  TCP Server listening on port %-5d    ║", TCP_SERVER_PORT);
    ESP_LOGI(TAG, "║  Waiting for connections...            ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");

    /* 4. Accept loop - terima koneksi client terus-menerus */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        /* Blocking call - tunggu client connect */
        int client_sock = accept(listen_sock, 
                                 (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(TAG, "[Server] accept() error: errno=%d", errno);
            continue;
        }

        /* Client terhubung - log info */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntoa_r(client_addr.sin_addr, client_ip, sizeof(client_ip));
        ESP_LOGI(TAG, "[Server] Client connected from %s:%d",
                 client_ip, ntohs(client_addr.sin_port));

        /* Spawn task baru untuk handle client ini */
        char task_name[32];
        snprintf(task_name, sizeof(task_name), "tcp_client_%d", client_sock);
        xTaskCreate(tcp_client_handler_task, task_name, 4096,
                    (void *)(intptr_t)client_sock, 5, NULL);
    }

    /* Tidak akan sampai sini, tapi untuk cleanup */
    close(listen_sock);
    vTaskDelete(NULL);
}

/**
 * TCP Client demo task - connect ke remote server dan kirim data
 * Ini adalah contoh client, bisa diaktifkan/dinonaktifkan
 */
static void tcp_client_demo_task(void *pvParameters)
{
    /* Tunggu sebentar agar server siap */
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "[Client] Attempting connection to %s:%d...",
             REMOTE_SERVER_IP, REMOTE_SERVER_PORT);

    /* 1. Buat socket */
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(TAG, "[Client] socket() error: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    /* 2. Setup server address */
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(REMOTE_SERVER_PORT),
    };
    inet_pton(AF_INET, REMOTE_SERVER_IP, &dest_addr.sin_addr);

    /* 3. Connect ke server */
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "[Client] connect() gagal: errno=%d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "[Client] Connected to server!");

    /* 4. Kirim beberapa pesan test */
    const char *messages[] = {
        "Hello from ESP32!",
        "TCP Socket Test Message",
        "Praktikum Modul 13 - Network",
    };
    char rx_buffer[256];

    for (int i = 0; i < 3; i++) {
        /* Kirim data */
        int sent = send(sock, messages[i], strlen(messages[i]), 0);
        if (sent < 0) {
            ESP_LOGE(TAG, "[Client] send() error: errno=%d", errno);
            break;
        }
        ESP_LOGI(TAG, "[Client] Sent: \"%s\" (%d bytes)", messages[i], sent);

        /* Terima response (echo dari server) */
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len > 0) {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG, "[Client] Received echo: \"%s\"", rx_buffer);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    /* 5. Tutup koneksi */
    ESP_LOGI(TAG, "[Client] Closing connection");
    shutdown(sock, SHUT_RDWR);
    close(sock);
    ESP_LOGI(TAG, "[Client] Demo complete");

    vTaskDelete(NULL);
}

/**
 * Main entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 TCP Client/Server - Modul 13 ===");

    /* 1. Connect ke WiFi terlebih dahulu */
    esp_err_t wifi_result = wifi_init_and_connect();
    if (wifi_result != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed! Cannot start TCP.");
        return;
    }

    /* 2. Start TCP server task */
    ESP_LOGI(TAG, "Starting TCP server...");
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);

    /*
     * 3. (Opsional) Start TCP client demo task
     * Uncomment baris di bawah jika ingin test client ke server lain.
     * Pastikan REMOTE_SERVER_IP dan REMOTE_SERVER_PORT sudah benar.
     */
    // xTaskCreate(tcp_client_demo_task, "tcp_client", 4096, NULL, 5, NULL);

    /* Main loop - status monitoring */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(15000));
        ESP_LOGI(TAG, "[Main] Server running | heap: %lu bytes",
                 (unsigned long)esp_get_free_heap_size());
    }
}
