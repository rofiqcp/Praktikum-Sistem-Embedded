/**
 * ============================================================================
 * ESP32_07_HTTP_Client
 * Modul 13 - Network & IoT: HTTP Client GET Request
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan HTTP Client pada ESP32 menggunakan
 *   `esp_http_client.h` dari ESP-IDF. Fitur:
 *   - HTTP GET request ke server eksternal
 *   - Parse response headers dan body
 *   - Periodic data fetching setiap interval tertentu
 *   - Logging detail request/response
 * 
 * Wiring Diagram:
 *   Tidak ada wiring khusus - menggunakan WiFi internal.
 *   ESP32 mengakses internet melalui WiFi router.
 * 
 *   [ESP32] ~~~WiFi~~~ [Router] --- [Internet] --- [httpbin.org]
 * 
 * Expected Output:
 *   I (xxxx) HTTP_CLI: WiFi connected, IP: 192.168.x.x
 *   I (xxxx) HTTP_CLI: === HTTP GET Request #1 ===
 *   I (xxxx) HTTP_CLI: URL: http://httpbin.org/get
 *   I (xxxx) HTTP_CLI: Status: 200, Content-Length: 285
 *   I (xxxx) HTTP_CLI: Response body (285 bytes):
 *   I (xxxx) HTTP_CLI: {"origin": "xxx.xxx.xxx.xxx", ...}
 * 
 * Catatan:
 *   - httpbin.org adalah layanan gratis untuk testing HTTP
 *   - Bisa diganti dengan server lokal (Python mock server)
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_tls.h"

/* ======================== KONFIGURASI ======================== */
#define WIFI_SSID           "YourWiFiSSID"
#define WIFI_PASS           "YourWiFiPassword"
#define WIFI_MAX_RETRY      5

/* URL target untuk HTTP request */
#define HTTP_URL_GET        "http://httpbin.org/get"
#define HTTP_URL_IP         "http://httpbin.org/ip"
#define HTTP_URL_HEADERS    "http://httpbin.org/headers"
#define FETCH_INTERVAL_SEC  30          // Interval fetch periodik

#define MAX_HTTP_RECV_BUFFER    1024    // Buffer untuk response body
#define MAX_HTTP_OUTPUT_BUFFER  2048    // Output buffer keseluruhan

static const char *TAG = "HTTP_CLI";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
static int s_retry_num = 0;

/* Buffer global untuk menyimpan response */
static char s_response_buffer[MAX_HTTP_OUTPUT_BUFFER];
static int s_response_len = 0;

/* ======================== WIFI HANDLER ======================== */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retry WiFi (%d/%d)...", s_retry_num, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t inst_any, inst_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, &inst_any));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL, &inst_ip));

    wifi_config_t wifi_config = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS,
                 .threshold.authmode = WIFI_AUTH_WPA2_PSK },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_FAIL;
}

/* ======================== HTTP EVENT HANDLER ======================== */
/**
 * Event handler untuk esp_http_client
 * Dipanggil saat ada event: header, data, finish, error
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        /* Tampilkan setiap response header yang diterima */
        ESP_LOGI(TAG, "Header: %s = %s", evt->header_key, evt->header_value);
        break;

    case HTTP_EVENT_ON_DATA:
        /**
         * Data response diterima (bisa dipanggil beberapa kali untuk data besar)
         * Kita kumpulkan di buffer global
         */
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (s_response_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
            memcpy(s_response_buffer + s_response_len, evt->data, evt->data_len);
            s_response_len += evt->data_len;
        } else {
            ESP_LOGW(TAG, "Response buffer penuh!");
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        s_response_buffer[s_response_len] = '\0';  // Null-terminate
        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;

    default:
        break;
    }
    return ESP_OK;
}

/* ======================== HTTP GET FUNCTION ======================== */
/**
 * Lakukan HTTP GET request ke URL tertentu
 * Menggunakan esp_http_client API
 */
static esp_err_t perform_http_get(const char *url, int request_num)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== HTTP GET Request #%d ===", request_num);
    ESP_LOGI(TAG, "URL: %s", url);

    /* Reset response buffer */
    s_response_len = 0;
    memset(s_response_buffer, 0, sizeof(s_response_buffer));

    /* Konfigurasi HTTP client */
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,            // Timeout 10 detik
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .user_agent = "ESP32-HTTP-Client/1.0",
    };

    /* Inisialisasi HTTP client */
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Gagal inisialisasi HTTP client");
        return ESP_FAIL;
    }

    /* Set custom header */
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "X-ESP32-Chip", "ESP32");
    esp_http_client_set_header(client, "X-Request-Num", "1");

    /* Catat waktu mulai untuk ukur latency */
    int64_t t_start = esp_timer_get_time();

    /* Eksekusi HTTP request - perform() blocking sampai selesai */
    esp_err_t err = esp_http_client_perform(client);

    int64_t t_end = esp_timer_get_time();
    int elapsed_ms = (int)((t_end - t_start) / 1000);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);

        ESP_LOGI(TAG, "Status: %d, Content-Length: %d, Time: %d ms",
                 status_code, content_length, elapsed_ms);

        /* Tampilkan response body (batasi agar log tidak terlalu panjang) */
        if (s_response_len > 0) {
            ESP_LOGI(TAG, "Response body (%d bytes):", s_response_len);
            /* Print per baris agar ESP_LOG tidak terpotong */
            char *line = strtok(s_response_buffer, "\n");
            while (line != NULL) {
                ESP_LOGI(TAG, "  %s", line);
                line = strtok(NULL, "\n");
            }
        }

        /* Coba parse field "origin" dari JSON httpbin response */
        char *origin = strstr(s_response_buffer, "\"origin\"");
        if (origin) {
            ESP_LOGI(TAG, ">> Public IP found in response");
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET gagal: %s (time: %d ms)", esp_err_to_name(err), elapsed_ms);
    }

    /* Cleanup - penting untuk menghindari memory leak */
    esp_http_client_cleanup(client);

    return err;
}

/* ======================== HTTP CLIENT TASK ======================== */
/**
 * Task untuk melakukan periodic HTTP GET requests
 * Mengambil data dari beberapa endpoint secara bergantian
 */
static void http_client_task(void *pvParameters)
{
    /* Daftar URL yang akan di-fetch secara bergantian */
    const char *urls[] = {
        HTTP_URL_GET,
        HTTP_URL_IP,
        HTTP_URL_HEADERS,
    };
    int url_count = sizeof(urls) / sizeof(urls[0]);
    int request_num = 0;

    /* Tunggu sebentar setelah WiFi konek */
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        request_num++;
        int url_idx = (request_num - 1) % url_count;

        ESP_LOGI(TAG, "--- Periodic Fetch #%d (every %d sec) ---",
                 request_num, FETCH_INTERVAL_SEC);

        perform_http_get(urls[url_idx], request_num);

        ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "Next fetch in %d seconds...\n", FETCH_INTERVAL_SEC);

        vTaskDelay(pdMS_TO_TICKS(FETCH_INTERVAL_SEC * 1000));
    }

    vTaskDelete(NULL);
}

/* ======================== MAIN APP ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 HTTP Client Demo ===");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());

    /* Inisialisasi NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Koneksi WiFi */
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi gagal!");
        return;
    }

    /* Mulai HTTP client task */
    xTaskCreate(http_client_task, "http_client", 8192, NULL, 5, NULL);

    /* Main loop - monitoring */
    while (1) {
        ESP_LOGI(TAG, "[Monitor] Heap: %lu bytes | Min heap: %lu bytes",
                 (unsigned long)esp_get_free_heap_size(),
                 (unsigned long)esp_get_minimum_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
