/**
 * ============================================================================
 * ESP32_06_HTTP_Server
 * Modul 13 - Network & IoT: HTTP Web Server & REST API
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini membuat HTTP web server pada ESP32 menggunakan ESP-IDF
 *   `esp_http_server.h`. Fitur yang didemonstrasikan:
 *   - GET /         -> Serve halaman HTML dengan status ESP32
 *   - GET /api/status -> JSON response (heap, uptime, chip info)
 *   - POST /api/led   -> Kontrol LED via REST API
 *   - Proper URI handler registration
 * 
 * Wiring Diagram:
 *   ESP32 GPIO2 --- [330Ω] --- LED --- GND
 *   (GPIO2 biasanya sudah ada onboard LED)
 * 
 *   [ESP32:GPIO2] ---[R 330Ω]---[LED(+)]---[GND]
 * 
 *   Browser/Client ~~~WiFi~~~ [ESP32 HTTP Server]
 * 
 * Expected Output:
 *   I (xxxx) HTTP_SVR: WiFi connected, IP: 192.168.x.x
 *   I (xxxx) HTTP_SVR: HTTP Server started on port 80
 *   I (xxxx) HTTP_SVR: GET / requested
 *   I (xxxx) HTTP_SVR: GET /api/status requested
 *   I (xxxx) HTTP_SVR: POST /api/led - state: ON
 * 
 * Akses via Browser:
 *   http://192.168.x.x/           -> Halaman web
 *   http://192.168.x.x/api/status -> JSON status
 *   curl -X POST http://192.168.x.x/api/led -d '{"state":true}'
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
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "esp_chip_info.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* ======================== KONFIGURASI ======================== */
#define WIFI_SSID       "YourWiFiSSID"
#define WIFI_PASS       "YourWiFiPassword"
#define WIFI_MAX_RETRY  5
#define LED_GPIO        GPIO_NUM_2      // Onboard LED

static const char *TAG = "HTTP_SVR";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static int s_retry_num = 0;
static bool s_led_state = false;        // State LED saat ini
static httpd_handle_t s_server = NULL;  // Handle HTTP server

/* ======================== WIFI EVENT HANDLER ======================== */
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

/* ======================== WIFI INIT ======================== */
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

/* ======================== HTML PAGE ======================== */
/**
 * Halaman HTML yang di-serve oleh ESP32
 * Menampilkan status dan kontrol LED via JavaScript fetch API
 */
static const char *html_page =
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>ESP32 Web Server</title>"
    "<style>"
    "body{font-family:Arial,sans-serif;max-width:600px;margin:40px auto;"
    "padding:20px;background:#1a1a2e;color:#e0e0e0;}"
    "h1{color:#0f3460;text-align:center;}"
    ".card{background:#16213e;border-radius:10px;padding:20px;margin:15px 0;"
    "box-shadow:0 2px 8px rgba(0,0,0,0.3);}"
    ".btn{padding:12px 30px;font-size:16px;border:none;border-radius:5px;"
    "cursor:pointer;color:white;margin:5px;}"
    ".btn-on{background:#27ae60;}.btn-off{background:#e74c3c;}"
    "#status{font-size:14px;color:#a0a0a0;}"
    ".led-indicator{width:30px;height:30px;border-radius:50%;display:inline-block;"
    "margin:0 10px;vertical-align:middle;}"
    ".led-on{background:#2ecc71;box-shadow:0 0 15px #2ecc71;}"
    ".led-off{background:#555;}"
    "</style></head><body>"
    "<h1>&#x1F4E1; ESP32 Web Server</h1>"
    "<div class='card'>"
    "<h2>LED Control</h2>"
    "<p>Status: <span class='led-indicator' id='ledIcon'></span>"
    "<span id='ledText'>OFF</span></p>"
    "<button class='btn btn-on' onclick='setLed(true)'>ON</button>"
    "<button class='btn btn-off' onclick='setLed(false)'>OFF</button>"
    "</div>"
    "<div class='card'>"
    "<h2>System Info</h2>"
    "<pre id='status'>Loading...</pre>"
    "</div>"
    "<script>"
    "function updateStatus(){"
    "fetch('/api/status').then(r=>r.json()).then(d=>{"
    "document.getElementById('status').textContent=JSON.stringify(d,null,2);"
    "updateLedUI(d.led_state);"
    "}).catch(e=>console.error(e));}"
    "function setLed(state){"
    "fetch('/api/led',{method:'POST',headers:{'Content-Type':'application/json'},"
    "body:JSON.stringify({state:state})}).then(r=>r.json()).then(d=>{"
    "updateLedUI(d.led_state);}).catch(e=>console.error(e));}"
    "function updateLedUI(on){"
    "document.getElementById('ledIcon').className='led-indicator '+(on?'led-on':'led-off');"
    "document.getElementById('ledText').textContent=on?'ON':'OFF';}"
    "updateStatus();setInterval(updateStatus,3000);"
    "</script></body></html>";

/* ======================== URI HANDLERS ======================== */

/**
 * GET / - Serve halaman HTML utama
 */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET / requested from client");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, strlen(html_page));
    return ESP_OK;
}

/**
 * GET /api/status - Response JSON dengan informasi sistem ESP32
 */
static esp_err_t api_status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /api/status requested");

    /* Kumpulkan informasi sistem */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    int64_t uptime_us = esp_timer_get_time();
    int uptime_sec = (int)(uptime_us / 1000000);

    /* Format JSON response */
    char json_resp[512];
    snprintf(json_resp, sizeof(json_resp),
        "{\"free_heap\":%lu,"
        "\"min_heap\":%lu,"
        "\"uptime_sec\":%d,"
        "\"chip_model\":\"ESP32\","
        "\"chip_cores\":%d,"
        "\"chip_revision\":%d,"
        "\"led_state\":%s,"
        "\"led_gpio\":%d,"
        "\"idf_version\":\"%s\"}",
        (unsigned long)esp_get_free_heap_size(),
        (unsigned long)esp_get_minimum_free_heap_size(),
        uptime_sec,
        chip_info.cores,
        chip_info.revision,
        s_led_state ? "true" : "false",
        LED_GPIO,
        esp_get_idf_version());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_resp, strlen(json_resp));
    return ESP_OK;
}

/**
 * POST /api/led - Kontrol LED via REST API
 * Body: {"state": true} atau {"state": false}
 */
static esp_err_t api_led_handler(httpd_req_t *req)
{
    char buf[128];
    int ret, remaining = req->content_len;

    if (remaining > sizeof(buf) - 1) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }

    /* Baca body request */
    ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) - 1));
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read body");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    ESP_LOGI(TAG, "POST /api/led - body: %s", buf);

    /* Parse sederhana untuk {"state":true/false} */
    if (strstr(buf, "true") != NULL) {
        s_led_state = true;
        gpio_set_level(LED_GPIO, 1);
        ESP_LOGI(TAG, "LED turned ON");
    } else if (strstr(buf, "false") != NULL) {
        s_led_state = false;
        gpio_set_level(LED_GPIO, 0);
        ESP_LOGI(TAG, "LED turned OFF");
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON: use {\"state\":true/false}");
        return ESP_FAIL;
    }

    /* Response dengan state terkini */
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"led_state\":%s}", s_led_state ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* ======================== HTTP SERVER START/STOP ======================== */
/**
 * Inisialisasi dan mulai HTTP server
 * Register semua URI handlers
 */
static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;     // Bersihkan koneksi lama

    ESP_LOGI(TAG, "Starting HTTP server on port %d...", config.server_port);

    if (httpd_start(&s_server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_uri_t root_uri = {
            .uri = "/", .method = HTTP_GET, .handler = root_get_handler
        };
        httpd_uri_t status_uri = {
            .uri = "/api/status", .method = HTTP_GET, .handler = api_status_handler
        };
        httpd_uri_t led_uri = {
            .uri = "/api/led", .method = HTTP_POST, .handler = api_led_handler
        };

        httpd_register_uri_handler(s_server, &root_uri);
        httpd_register_uri_handler(s_server, &status_uri);
        httpd_register_uri_handler(s_server, &led_uri);

        ESP_LOGI(TAG, "HTTP Server started. Registered URIs: /, /api/status, /api/led");
        return s_server;
    }

    ESP_LOGE(TAG, "Failed to start HTTP server!");
    return NULL;
}

/* ======================== MAIN APP ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 HTTP Web Server Demo ===");

    /* Inisialisasi NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Konfigurasi LED GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);  // LED off awal

    /* Koneksi WiFi */
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi gagal! Server tidak bisa dimulai.");
        return;
    }

    /* Mulai HTTP Server */
    start_webserver();

    /* Main loop - monitoring */
    while (1) {
        ESP_LOGI(TAG, "[Monitor] Heap: %lu bytes | LED: %s | Uptime: %lld s",
                 (unsigned long)esp_get_free_heap_size(),
                 s_led_state ? "ON" : "OFF",
                 esp_timer_get_time() / 1000000LL);
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}
