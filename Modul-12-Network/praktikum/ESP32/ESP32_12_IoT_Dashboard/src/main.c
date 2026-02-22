/**
 * ============================================================================
 * ESP32_12_IoT_Dashboard - Full IoT System: Sensor → MQTT → Dashboard
 * ============================================================================
 * 
 * Deskripsi:
 *   Program IoT lengkap yang mengintegrasikan beberapa komponen:
 *   1. Membaca data sensor (simulated / internal temp)
 *   2. Publish data ke MQTT broker dalam format JSON
 *   3. Subscribe ke topic kontrol untuk remote commands
 *   4. HTTP server menampilkan dashboard HTML live
 * 
 * Konsep Utama:
 *   - MQTT Publish/Subscribe untuk IoT communication
 *   - HTTP Server untuk web-based dashboard
 *   - JSON data formatting untuk interoperabilitas
 *   - Multi-protocol: WiFi + MQTT + HTTP simultaneously
 * 
 * Wiring Diagram:
 *   [ESP32 Dev Board]
 *   - GPIO2  -> LED built-in (status & remote control)
 *   - WiFi   -> Connect ke Access Point
 *   
 *   [Network Architecture]
 *   ESP32 --WiFi--> Router --Internet--> MQTT Broker (test.mosquitto.org)
 *                                    --> HTTP Client (Browser)
 *   
 *   Browser --> http://<ESP32_IP>/ --> Live Dashboard
 *   MQTT    --> esp32/sensor/data  --> Sensor readings
 *   MQTT    --> esp32/control/cmd  --> Remote commands
 * 
 * Expected Output:
 *   I (xxx) IOT_DASH: WiFi connected, IP: 192.168.x.x
 *   I (xxx) IOT_DASH: MQTT connected to test.mosquitto.org
 *   I (xxx) IOT_DASH: HTTP Dashboard at http://192.168.x.x/
 *   I (xxx) IOT_DASH: Published: {"temp":25,"hum":60,"light":512}
 *   I (xxx) IOT_DASH: MQTT control received: led_on
 * 
 * Author: Praktikum Sistem Embedded
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
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
#include "mqtt_client.h"

static const char *TAG = "IOT_DASH";

// ============================================================================
// Konfigurasi - GANTI sesuai setup Anda!
// ============================================================================
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASS           "YOUR_WIFI_PASS"
#define MQTT_BROKER_URI     "mqtt://test.mosquitto.org"
#define MQTT_TOPIC_DATA     "esp32/sensor/data"
#define MQTT_TOPIC_CONTROL  "esp32/control/cmd"
#define MQTT_TOPIC_STATUS   "esp32/status"

#define LED_PIN             GPIO_NUM_2
#define PUBLISH_INTERVAL_MS 5000
#define MAX_RETRY           10

// ============================================================================
// Global State
// ============================================================================
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static int retry_count = 0;

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static httpd_handle_t http_server = NULL;

// Sensor data (simulated)
typedef struct {
    float temperature;
    float humidity;
    int   light_level;
    int   reading_count;
    bool  led_state;
    uint32_t free_heap;
    int64_t  uptime_sec;
} sensor_state_t;

static sensor_state_t state = {
    .temperature = 25.0,
    .humidity = 60.0,
    .light_level = 512,
    .reading_count = 0,
    .led_state = false,
};

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
            ESP_LOGW(TAG, "WiFi reconnecting... attempt %d", retry_count);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t inst1, inst2;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, &inst1));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL, &inst2));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);
}

// ============================================================================
// MQTT Event Handler
// ============================================================================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to broker");
            mqtt_connected = true;
            // Subscribe ke control topic
            esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_CONTROL, 1);
            ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_CONTROL);
            // Publish status online
            esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS,
                                     "{\"status\":\"online\"}", 0, 1, 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA:
            // Terima pesan dari topic yang di-subscribe
            ESP_LOGI(TAG, "MQTT data received - topic: %.*s",
                     event->topic_len, event->topic);
            ESP_LOGI(TAG, "  Payload: %.*s", event->data_len, event->data);

            // Parse control commands
            if (strncmp(event->data, "led_on", event->data_len) == 0) {
                gpio_set_level(LED_PIN, 1);
                state.led_state = true;
                ESP_LOGI(TAG, "  -> LED turned ON via MQTT");
            } else if (strncmp(event->data, "led_off", event->data_len) == 0) {
                gpio_set_level(LED_PIN, 0);
                state.led_state = false;
                ESP_LOGI(TAG, "  -> LED turned OFF via MQTT");
            } else if (strncmp(event->data, "status", event->data_len) == 0) {
                char status_json[200];
                snprintf(status_json, sizeof(status_json),
                         "{\"led\":%s,\"readings\":%d,\"heap\":%lu}",
                         state.led_state ? "true" : "false",
                         state.reading_count,
                         (unsigned long)esp_get_free_heap_size());
                esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS,
                                         status_json, 0, 1, 0);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            break;

        default:
            break;
    }
}

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT client started, connecting to %s", MQTT_BROKER_URI);
}

// ============================================================================
// HTTP Dashboard - halaman HTML dengan auto-refresh
// ============================================================================
static const char *dashboard_html_template =
    "<!DOCTYPE html><html><head><title>ESP32 IoT Dashboard</title>"
    "<meta http-equiv='refresh' content='5'>"
    "<style>"
    "body{font-family:Arial;max-width:800px;margin:20px auto;background:#1a1a2e;color:#eee;}"
    ".card{background:#16213e;border-radius:10px;padding:20px;margin:10px 0;box-shadow:0 2px 8px rgba(0,0,0,0.3);}"
    ".value{font-size:48px;font-weight:bold;color:#0ff;}"
    ".label{font-size:14px;color:#888;text-transform:uppercase;}"
    ".grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:15px;}"
    ".status{padding:5px 15px;border-radius:15px;display:inline-block;}"
    ".online{background:#0f5;color:#000;}.offline{background:#f55;color:#fff;}"
    "h1{text-align:center;color:#0ff;}"
    ".controls{text-align:center;margin:15px 0;}"
    ".controls a{padding:12px 25px;margin:5px;text-decoration:none;border-radius:5px;"
    "color:#fff;font-size:16px;display:inline-block;}"
    ".btn-on{background:#0a0;}.btn-off{background:#a00;}"
    "</style></head><body>"
    "<h1>🌐 ESP32 IoT Dashboard</h1>"
    "<div class='card'>"
    "<span class='label'>System Status</span><br>"
    "<span class='status %s'>%s</span> | "
    "MQTT: <span class='status %s'>%s</span> | "
    "Uptime: %llds | Reading #%d"
    "</div>"
    "<div class='grid'>"
    "<div class='card'><div class='label'>Temperature</div><div class='value'>%.1f°C</div></div>"
    "<div class='card'><div class='label'>Humidity</div><div class='value'>%.1f%%</div></div>"
    "<div class='card'><div class='label'>Light Level</div><div class='value'>%d</div></div>"
    "</div>"
    "<div class='card'>"
    "<div class='label'>LED Control</div><br>"
    "<div class='value'>%s</div>"
    "<div class='controls'>"
    "<a href='/led?state=on' class='btn-on'>💡 LED ON</a>"
    "<a href='/led?state=off' class='btn-off'>💡 LED OFF</a>"
    "</div></div>"
    "<div class='card'><div class='label'>System Info</div>"
    "<p>Free Heap: %lu bytes | Min Heap: %lu bytes</p>"
    "</div></body></html>";

static esp_err_t dashboard_handler(httpd_req_t *req)
{
    char *html = malloc(4096);
    if (!html) return ESP_ERR_NO_MEM;

    state.uptime_sec = esp_timer_get_time() / 1000000LL;
    state.free_heap = esp_get_free_heap_size();

    snprintf(html, 4096, dashboard_html_template,
             "online", "WIFI OK",
             mqtt_connected ? "online" : "offline",
             mqtt_connected ? "CONNECTED" : "DISCONNECTED",
             (long long)state.uptime_sec, state.reading_count,
             state.temperature, state.humidity, state.light_level,
             state.led_state ? "ON 💡" : "OFF ⚫",
             (unsigned long)state.free_heap,
             (unsigned long)esp_get_minimum_free_heap_size());

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    free(html);
    return ESP_OK;
}

static esp_err_t led_handler(httpd_req_t *req)
{
    // Parse query parameter ?state=on/off
    char query[32] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[8] = {0};
        if (httpd_query_key_value(query, "state", val, sizeof(val)) == ESP_OK) {
            if (strcmp(val, "on") == 0) {
                gpio_set_level(LED_PIN, 1);
                state.led_state = true;
                ESP_LOGI(TAG, "LED ON via HTTP");
            } else if (strcmp(val, "off") == 0) {
                gpio_set_level(LED_PIN, 0);
                state.led_state = false;
                ESP_LOGI(TAG, "LED OFF via HTTP");
            }
        }
    }
    // Redirect back to dashboard
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t api_handler(httpd_req_t *req)
{
    char json[256];
    snprintf(json, sizeof(json),
             "{\"temp\":%.1f,\"hum\":%.1f,\"light\":%d,"
             "\"led\":%s,\"reading\":%d,\"uptime\":%lld,\"heap\":%lu}",
             state.temperature, state.humidity, state.light_level,
             state.led_state ? "true" : "false",
             state.reading_count,
             (long long)(esp_timer_get_time() / 1000000LL),
             (unsigned long)esp_get_free_heap_size());

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, strlen(json));
}

static void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&http_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        return;
    }

    httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = dashboard_handler };
    httpd_uri_t uri_led  = { .uri = "/led", .method = HTTP_GET, .handler = led_handler };
    httpd_uri_t uri_api  = { .uri = "/api/data", .method = HTTP_GET, .handler = api_handler };

    httpd_register_uri_handler(http_server, &uri_root);
    httpd_register_uri_handler(http_server, &uri_led);
    httpd_register_uri_handler(http_server, &uri_api);

    ESP_LOGI(TAG, "HTTP Dashboard started on port 80");
}

// ============================================================================
// Sensor Reading & MQTT Publishing Task
// ============================================================================
static void sensor_publish_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));

        // Simulate sensor readings dengan variasi realistis
        state.reading_count++;
        state.temperature = 22.0 + (float)(esp_random() % 100) / 10.0;  // 22.0-31.9
        state.humidity    = 45.0 + (float)(esp_random() % 300) / 10.0;  // 45.0-74.9
        state.light_level = 200 + (esp_random() % 800);                  // 200-999

        ESP_LOGI(TAG, "Sensor #%d: temp=%.1f°C hum=%.1f%% light=%d",
                 state.reading_count, state.temperature,
                 state.humidity, state.light_level);

        // Publish ke MQTT jika connected
        if (mqtt_connected) {
            char json_payload[256];
            snprintf(json_payload, sizeof(json_payload),
                     "{\"id\":%d,\"temp\":%.1f,\"hum\":%.1f,\"light\":%d,"
                     "\"led\":%s,\"heap\":%lu,\"uptime\":%lld}",
                     state.reading_count,
                     state.temperature, state.humidity, state.light_level,
                     state.led_state ? "true" : "false",
                     (unsigned long)esp_get_free_heap_size(),
                     (long long)(esp_timer_get_time() / 1000000LL));

            int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_DATA,
                                                  json_payload, 0, 1, 0);
            ESP_LOGI(TAG, "MQTT published (msg_id=%d): %s", msg_id, json_payload);
        } else {
            ESP_LOGW(TAG, "MQTT not connected, data not published");
        }
    }
}

// ============================================================================
// Main Application Entry Point
// ============================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 IoT Dashboard");
    ESP_LOGI(TAG, "  Sensor + MQTT + HTTP Dashboard");
    ESP_LOGI(TAG, "  Praktikum Sistem Embedded");
    ESP_LOGI(TAG, "========================================");

    // Setup LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect WiFi
    ESP_LOGI(TAG, "Step 1: Connecting to WiFi...");
    wifi_init_sta();

    // Start MQTT client
    ESP_LOGI(TAG, "Step 2: Starting MQTT client...");
    mqtt_init();

    // Start HTTP Dashboard server
    ESP_LOGI(TAG, "Step 3: Starting HTTP Dashboard...");
    start_http_server();

    // Start sensor reading & publishing task
    ESP_LOGI(TAG, "Step 4: Starting sensor task...");
    xTaskCreate(sensor_publish_task, "sensor_pub", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "=== IoT Dashboard System Running! ===");
    ESP_LOGI(TAG, "Open browser: http://<ESP32_IP>/");
    ESP_LOGI(TAG, "MQTT data topic: %s", MQTT_TOPIC_DATA);
    ESP_LOGI(TAG, "MQTT control topic: %s", MQTT_TOPIC_CONTROL);
}
