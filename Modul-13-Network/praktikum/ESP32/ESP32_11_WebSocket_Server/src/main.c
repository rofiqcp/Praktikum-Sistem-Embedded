/**
 * ============================================================================
 * ESP32_11_WebSocket_Server - Real-time Bidirectional Communication
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mengimplementasikan WebSocket server pada ESP32
 *   menggunakan ESP-IDF HTTP server dengan upgrade ke WebSocket.
 *   Mendukung komunikasi real-time bidirectional antara ESP32
 *   dan web browser atau client Python.
 * 
 * Konsep Utama:
 *   - WebSocket Protocol: full-duplex communication over TCP
 *   - HTTP Server upgrade ke WebSocket
 *   - Frame types: text, binary, ping/pong, close
 *   - Broadcast ke semua connected clients
 *   - Async data push dari server ke client
 * 
 * Wiring Diagram:
 *   [ESP32 Dev Board]
 *   - GPIO2  -> LED built-in (dikontrol via WebSocket command)
 *   - WiFi   -> Connect ke Access Point
 *   
 *   [Network]
 *   ESP32 (WebSocket Server, port 80) <--ws://--> Client(s)
 * 
 * Expected Output:
 *   I (xxx) WS_SRV: Connected to WiFi, IP: 192.168.x.x
 *   I (xxx) WS_SRV: WebSocket server started on port 80
 *   I (xxx) WS_SRV: ws://192.168.x.x/ws
 *   I (xxx) WS_SRV: Client connected (fd=X)
 *   I (xxx) WS_SRV: Received: "hello"
 *   I (xxx) WS_SRV: Broadcasting sensor data to 2 clients
 * 
 * Author: Praktikum Sistem Embedded
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
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"

static const char *TAG = "WS_SRV";

// ============================================================================
// Konfigurasi WiFi - GANTI sesuai jaringan Anda!
// ============================================================================
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASS       "YOUR_WIFI_PASS"
#define MAX_RETRY       10
#define LED_PIN         GPIO_NUM_2

// WebSocket client tracking
#define MAX_WS_CLIENTS  4

static httpd_handle_t server = NULL;
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static int retry_count = 0;

// Track connected WebSocket clients
static int ws_client_fds[MAX_WS_CLIENTS];
static int ws_client_count = 0;

// Simulated sensor counter
static int sensor_counter = 0;

// ============================================================================
// WiFi Event Handler
// ============================================================================
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGW(TAG, "WiFi reconnecting... attempt %d/%d", retry_count, MAX_RETRY);
        } else {
            ESP_LOGE(TAG, "WiFi connection failed after %d attempts", MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ============================================================================
// WiFi Initialization
// ============================================================================
static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL, &instance_got_ip));

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

    ESP_LOGI(TAG, "WiFi STA init complete, connecting to: %s", WIFI_SSID);

    // Tunggu sampai connected
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);
}

// ============================================================================
// WebSocket Handler - menangani koneksi dan pesan WebSocket
// ============================================================================
static esp_err_t ws_handler(httpd_req_t *req)
{
    // Handle new WebSocket connection (upgrade request)
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, ">>> WebSocket client connected (fd=%d)", fd);

        // Track client
        if (ws_client_count < MAX_WS_CLIENTS) {
            ws_client_fds[ws_client_count++] = fd;
        }

        // Kirim welcome message
        const char *welcome = "{\"type\":\"welcome\",\"msg\":\"Connected to ESP32 WebSocket!\"}";
        httpd_ws_frame_t ws_pkt = {
            .payload = (uint8_t *)welcome,
            .len = strlen(welcome),
            .type = HTTPD_WS_TYPE_TEXT,
        };
        httpd_ws_send_frame(req, &ws_pkt);
        return ESP_OK;
    }

    // Receive WebSocket frame
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // Get frame length first
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ws recv frame len failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (ws_pkt.len > 0) {
        // Allocate buffer dan terima data
        uint8_t *buf = calloc(1, ws_pkt.len + 1);
        if (!buf) {
            ESP_LOGE(TAG, "Memory allocation failed!");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            free(buf);
            return ret;
        }

        ESP_LOGI(TAG, "Received [%s]: %s", 
                 ws_pkt.type == HTTPD_WS_TYPE_TEXT ? "TEXT" : "BIN",
                 (char *)ws_pkt.payload);

        // Process commands dari client
        char *msg = (char *)ws_pkt.payload;
        char response[256];

        if (strcmp(msg, "led_on") == 0) {
            gpio_set_level(LED_PIN, 1);
            snprintf(response, sizeof(response),
                     "{\"type\":\"response\",\"cmd\":\"led\",\"status\":\"ON\"}");
        } else if (strcmp(msg, "led_off") == 0) {
            gpio_set_level(LED_PIN, 0);
            snprintf(response, sizeof(response),
                     "{\"type\":\"response\",\"cmd\":\"led\",\"status\":\"OFF\"}");
        } else if (strcmp(msg, "status") == 0) {
            snprintf(response, sizeof(response),
                     "{\"type\":\"status\",\"uptime\":%lld,\"clients\":%d,\"heap\":%lu}",
                     esp_timer_get_time() / 1000000LL,
                     ws_client_count,
                     (unsigned long)esp_get_free_heap_size());
        } else if (strcmp(msg, "ping") == 0) {
            snprintf(response, sizeof(response),
                     "{\"type\":\"pong\",\"time\":%lld}",
                     esp_timer_get_time() / 1000LL);
        } else {
            snprintf(response, sizeof(response),
                     "{\"type\":\"echo\",\"data\":\"%s\"}", msg);
        }

        // Kirim response
        httpd_ws_frame_t rsp_pkt = {
            .payload = (uint8_t *)response,
            .len = strlen(response),
            .type = HTTPD_WS_TYPE_TEXT,
        };
        httpd_ws_send_frame(req, &rsp_pkt);

        free(buf);
    }

    return ESP_OK;
}

// ============================================================================
// Root HTTP Handler - serve halaman HTML sederhana
// ============================================================================
static const char *html_page =
    "<!DOCTYPE html><html><head><title>ESP32 WebSocket</title>"
    "<style>body{font-family:monospace;max-width:600px;margin:50px auto;}"
    "button{padding:10px 20px;margin:5px;font-size:16px;cursor:pointer;}"
    "#log{background:#111;color:#0f0;padding:15px;height:300px;"
    "overflow-y:scroll;font-size:14px;}</style></head><body>"
    "<h2>ESP32 WebSocket Dashboard</h2>"
    "<button onclick=\"ws.send('led_on')\">LED ON</button>"
    "<button onclick=\"ws.send('led_off')\">LED OFF</button>"
    "<button onclick=\"ws.send('status')\">Status</button>"
    "<button onclick=\"ws.send('ping')\">Ping</button>"
    "<h3>Log:</h3><div id='log'></div>"
    "<script>"
    "var ws=new WebSocket('ws://'+location.host+'/ws');"
    "var log=document.getElementById('log');"
    "ws.onopen=function(){addLog('Connected!');};"
    "ws.onmessage=function(e){addLog('RX: '+e.data);};"
    "ws.onclose=function(){addLog('Disconnected');};"
    "function addLog(m){log.innerHTML+=m+'<br>';log.scrollTop=log.scrollHeight;}"
    "</script></body></html>";

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_page, strlen(html_page));
}

// ============================================================================
// Start WebSocket Server
// ============================================================================
static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;

    ESP_LOGI(TAG, "Starting WebSocket server on port %d", config.server_port);

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        return NULL;
    }

    // Register root page handler
    httpd_uri_t root_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_handler,
    };
    httpd_register_uri_handler(server, &root_uri);

    // Register WebSocket handler
    httpd_uri_t ws_uri = {
        .uri       = "/ws",
        .method    = HTTP_GET,
        .handler   = ws_handler,
        .is_websocket = true,
    };
    httpd_register_uri_handler(server, &ws_uri);

    ESP_LOGI(TAG, "WebSocket endpoint: ws://<IP>/ws");
    ESP_LOGI(TAG, "Dashboard: http://<IP>/");
    return server;
}

// ============================================================================
// Broadcast Task - kirim sensor data ke semua clients
// ============================================================================
static void broadcast_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));  // Broadcast setiap 3 detik

        if (server == NULL || ws_client_count == 0) continue;

        sensor_counter++;
        int simulated_temp = 20 + (esp_random() % 15);  // 20-34°C
        int simulated_hum  = 40 + (esp_random() % 40);  // 40-79%

        char json_msg[200];
        snprintf(json_msg, sizeof(json_msg),
                 "{\"type\":\"sensor\",\"id\":%d,\"temp\":%d,\"hum\":%d,\"heap\":%lu}",
                 sensor_counter, simulated_temp, simulated_hum,
                 (unsigned long)esp_get_free_heap_size());

        // Broadcast ke semua connected clients menggunakan async send
        int active = 0;
        for (int i = 0; i < ws_client_count; i++) {
            httpd_ws_frame_t ws_pkt = {
                .final = true,
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)json_msg,
                .len = strlen(json_msg),
            };

            if (httpd_ws_send_frame_async(server, ws_client_fds[i], &ws_pkt) == ESP_OK) {
                active++;
            } else {
                // Client mungkin sudah disconnect, hapus dari list
                ESP_LOGW(TAG, "Client fd=%d unreachable, removing", ws_client_fds[i]);
                for (int j = i; j < ws_client_count - 1; j++) {
                    ws_client_fds[j] = ws_client_fds[j + 1];
                }
                ws_client_count--;
                i--;
            }
        }

        if (active > 0) {
            ESP_LOGI(TAG, "Broadcast #%d to %d client(s): temp=%d hum=%d",
                     sensor_counter, active, simulated_temp, simulated_hum);
        }
    }
}

// ============================================================================
// Main Application Entry Point
// ============================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 WebSocket Server Demo");
    ESP_LOGI(TAG, "  Praktikum Sistem Embedded");
    ESP_LOGI(TAG, "========================================");

    // Setup LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init WiFi
    ESP_LOGI(TAG, "Connecting to WiFi...");
    wifi_init_sta();

    // Start WebSocket server
    start_webserver();

    // Start broadcast task
    xTaskCreate(broadcast_task, "broadcast", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "System ready! Open browser to http://<ESP32_IP>/");
}
