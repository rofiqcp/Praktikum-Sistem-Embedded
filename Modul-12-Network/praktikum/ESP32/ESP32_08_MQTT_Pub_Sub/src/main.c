/**
 * ============================================================================
 * ESP32_08_MQTT_Pub_Sub
 * Modul 13 - Network & IoT: MQTT Publish/Subscribe
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan protokol MQTT (Message Queuing Telemetry
 *   Transport) pada ESP32 menggunakan ESP-MQTT component. Fitur:
 *   - Connect ke MQTT broker (test.mosquitto.org)
 *   - Publish sensor data ke topic
 *   - Subscribe dan terima pesan dari topic
 *   - Handle MQTT events (connected, data, error, dll.)
 *   - Demonstrasi QoS level 0, 1, 2
 * 
 * Wiring Diagram:
 *   Tidak ada wiring khusus - WiFi internal + MQTT over TCP.
 *   
 *   [ESP32] ~~~WiFi~~~ [Router] --- [Internet] --- [test.mosquitto.org:1883]
 *   
 *   MQTT Topics:
 *     Publish:   esp32/sensor/data    (data sensor periodik)
 *     Publish:   esp32/status         (status device)
 *     Subscribe: esp32/command/#      (menerima perintah)
 *     Subscribe: esp32/led            (kontrol LED)
 * 
 * Expected Output:
 *   I (xxxx) MQTT: WiFi connected
 *   I (xxxx) MQTT: MQTT_EVENT_CONNECTED - Broker connected!
 *   I (xxxx) MQTT: Subscribed to: esp32/command/#
 *   I (xxxx) MQTT: Published QoS0: {"temp":25.3,"hum":60.1}
 *   I (xxxx) MQTT: MQTT_EVENT_DATA - Topic: esp32/command/led
 *   I (xxxx) MQTT: Message: {"state":"on"}
 * 
 * Public Broker:
 *   test.mosquitto.org : port 1883 (unencrypted)
 *   Bisa juga pakai broker.hivemq.com
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "esp_random.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

/* ======================== KONFIGURASI ======================== */
#define WIFI_SSID           "YourWiFiSSID"
#define WIFI_PASS           "YourWiFiPassword"
#define WIFI_MAX_RETRY      5

/* MQTT Broker Configuration */
#define MQTT_BROKER_URI     "mqtt://test.mosquitto.org"   // Public broker
#define MQTT_BROKER_PORT    1883

/* MQTT Topics */
#define TOPIC_SENSOR_DATA   "esp32/sensor/data"
#define TOPIC_STATUS        "esp32/status"
#define TOPIC_COMMAND       "esp32/command/#"    // Wildcard subscribe
#define TOPIC_LED           "esp32/led"

#define LED_GPIO            GPIO_NUM_2
#define PUBLISH_INTERVAL_MS 10000   // Publish setiap 10 detik

static const char *TAG = "MQTT";

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
static int s_retry_num = 0;

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;
static int s_publish_count = 0;

/* ======================== WIFI ======================== */
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

/* ======================== MQTT EVENT HANDLER ======================== */
/**
 * Handler untuk semua MQTT events
 * Event penting: CONNECTED, DATA, DISCONNECTED, ERROR
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        /**
         * Berhasil konek ke broker!
         * Saatnya subscribe ke topics yang diinginkan
         */
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED - Broker connected!");
        s_mqtt_connected = true;

        /* Subscribe ke topic command (wildcard #) - QoS 1 */
        int msg_id = esp_mqtt_client_subscribe(client, TOPIC_COMMAND, 1);
        ESP_LOGI(TAG, "Subscribed to: %s (msg_id=%d, QoS=1)", TOPIC_COMMAND, msg_id);

        /* Subscribe ke topic LED - QoS 0 */
        msg_id = esp_mqtt_client_subscribe(client, TOPIC_LED, 0);
        ESP_LOGI(TAG, "Subscribed to: %s (msg_id=%d, QoS=0)", TOPIC_LED, msg_id);

        /* Publish status online */
        char status_msg[128];
        snprintf(status_msg, sizeof(status_msg),
                 "{\"status\":\"online\",\"heap\":%lu}",
                 (unsigned long)esp_get_free_heap_size());
        esp_mqtt_client_publish(client, TOPIC_STATUS, status_msg, 0, 1, 1);  // QoS1, retain
        ESP_LOGI(TAG, "Published online status (retained)");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED - Broker disconnected");
        s_mqtt_connected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        /**
         * Data diterima dari topic yang di-subscribe
         * event->topic dan event->data berisi info lengkap
         */
        ESP_LOGI(TAG, "MQTT_EVENT_DATA received:");

        /* Print topic (mungkin NULL pada fragmented messages) */
        if (event->topic) {
            char topic_buf[128];
            int topic_len = event->topic_len < 127 ? event->topic_len : 127;
            memcpy(topic_buf, event->topic, topic_len);
            topic_buf[topic_len] = '\0';
            ESP_LOGI(TAG, "  Topic: %s (len=%d)", topic_buf, event->topic_len);
        }

        /* Print data/payload */
        if (event->data_len > 0) {
            char data_buf[256];
            int data_len = event->data_len < 255 ? event->data_len : 255;
            memcpy(data_buf, event->data, data_len);
            data_buf[data_len] = '\0';
            ESP_LOGI(TAG, "  Payload: %s (len=%d)", data_buf, event->data_len);

            /* Handle LED command */
            if (event->topic && strstr(event->topic, "led")) {
                if (strstr(data_buf, "on") || strstr(data_buf, "true") || strstr(data_buf, "1")) {
                    gpio_set_level(LED_GPIO, 1);
                    ESP_LOGI(TAG, "  >> LED turned ON via MQTT");
                } else if (strstr(data_buf, "off") || strstr(data_buf, "false") || strstr(data_buf, "0")) {
                    gpio_set_level(LED_GPIO, 0);
                    ESP_LOGI(TAG, "  >> LED turned OFF via MQTT");
                }
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "  TCP transport error: %s",
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI(TAG, "Other MQTT event id: %d", event->event_id);
        break;
    }
}

/* ======================== MQTT INIT ======================== */
/**
 * Inisialisasi MQTT client dan mulai koneksi ke broker
 */
static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Initializing MQTT client...");
    ESP_LOGI(TAG, "Broker: %s:%d", MQTT_BROKER_URI, MQTT_BROKER_PORT);

    /* Konfigurasi MQTT client */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.address.port = MQTT_BROKER_PORT,
        .credentials.client_id = "esp32_praktikum_modul13",
        .session.last_will.topic = TOPIC_STATUS,
        .session.last_will.msg = "{\"status\":\"offline\"}",
        .session.last_will.msg_len = 0,
        .session.last_will.qos = 1,
        .session.last_will.retain = 1,
        .network.reconnect_timeout_ms = 5000,
        .session.keepalive = 60,
    };

    /* Buat dan mulai MQTT client */
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Gagal init MQTT client!");
        return;
    }

    /* Register event handler */
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);

    /* Start MQTT client - akan auto-connect ke broker */
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
    ESP_LOGI(TAG, "MQTT client started, connecting to broker...");
}

/* ======================== SENSOR PUBLISH TASK ======================== */
/**
 * Task untuk publish data sensor secara periodik
 * Mendemonstrasikan QoS 0, 1, dan 2
 */
static void sensor_publish_task(void *pvParameters)
{
    /* Tunggu sampai MQTT connected */
    while (!s_mqtt_connected) {
        ESP_LOGI(TAG, "Menunggu MQTT connected...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (1) {
        if (!s_mqtt_connected) {
            ESP_LOGW(TAG, "MQTT disconnected, menunggu reconnect...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        s_publish_count++;

        /* Simulasi data sensor (dalam praktik nyata, baca dari ADC/I2C) */
        float temperature = 20.0 + (float)(esp_random() % 150) / 10.0;  // 20-35°C
        float humidity = 40.0 + (float)(esp_random() % 400) / 10.0;     // 40-80%
        int light = esp_random() % 4096;  // Simulasi ADC 12-bit

        /* Format JSON payload */
        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"seq\":%d,\"temp\":%.1f,\"hum\":%.1f,\"light\":%d,"
                 "\"heap\":%lu,\"uptime\":%lld}",
                 s_publish_count, temperature, humidity, light,
                 (unsigned long)esp_get_free_heap_size(),
                 esp_timer_get_time() / 1000000LL);

        /**
         * Publish dengan berbagai QoS level:
         * - QoS 0: Fire and forget (cepat, tidak dijamin sampai)
         * - QoS 1: At least once (dijamin sampai, bisa duplikat)
         * - QoS 2: Exactly once (paling reliable, paling lambat)
         */
        int qos = s_publish_count % 3;  // Rotasi QoS 0, 1, 2

        int msg_id = esp_mqtt_client_publish(s_mqtt_client, TOPIC_SENSOR_DATA,
                                             payload, 0, qos, 0);
        ESP_LOGI(TAG, "Published #%d (QoS%d, msg_id=%d): %s",
                 s_publish_count, qos, msg_id, payload);

        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}

/* ======================== MAIN APP ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 MQTT Publish/Subscribe Demo ===");
    ESP_LOGI(TAG, "Broker: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());

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
    gpio_set_level(LED_GPIO, 0);

    /* Koneksi WiFi */
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi gagal!");
        return;
    }

    /* Start MQTT */
    mqtt_app_start();

    /* Start sensor publish task */
    xTaskCreate(sensor_publish_task, "mqtt_publish", 4096, NULL, 5, NULL);

    /* Main loop - monitoring */
    while (1) {
        ESP_LOGI(TAG, "[Monitor] MQTT: %s | Published: %d | Heap: %lu bytes",
                 s_mqtt_connected ? "CONNECTED" : "DISCONNECTED",
                 s_publish_count,
                 (unsigned long)esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
