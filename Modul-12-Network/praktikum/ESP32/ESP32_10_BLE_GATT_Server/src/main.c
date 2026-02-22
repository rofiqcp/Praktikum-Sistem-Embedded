/**
 * ============================================================================
 * ESP32_10_BLE_GATT_Server - GATT Server with Service & Characteristics
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mengimplementasikan BLE GATT (Generic Attribute Profile)
 *   Server pada ESP32. GATT Server menyediakan service dengan beberapa
 *   characteristic: read (sensor data), write (LED control), dan notify
 *   (periodic sensor updates).
 * 
 * Konsep Utama:
 *   - GATT Server: menyediakan data via service/characteristic
 *   - Service UUID: identifikasi unik untuk layanan
 *   - Characteristics: read, write, notify properties
 *   - Notifications: push data ke client secara periodik
 * 
 * Wiring Diagram:
 *   [ESP32 Dev Board]
 *   - GPIO2  -> LED (built-in) untuk kontrol via BLE write
 *   - Internal temp sensor / simulated sensor untuk read/notify
 * 
 * GATT Structure:
 *   Service (UUID: 0x00FF)
 *     ├── Char A - Read  (UUID: 0xFF01) - Sensor value
 *     ├── Char B - Write (UUID: 0xFF02) - LED control  
 *     └── Char C - Notify(UUID: 0xFF03) - Periodic data
 * 
 * Expected Output:
 *   I (xxx) GATT_SVR: BLE GATT Server initialized
 *   I (xxx) GATT_SVR: Service created, handle: XX
 *   I (xxx) GATT_SVR: Characteristics added
 *   I (xxx) GATT_SVR: Advertising started, waiting for connection...
 *   I (xxx) GATT_SVR: Client connected! conn_id=0
 *   I (xxx) GATT_SVR: Notification sent: sensor=XX
 * 
 * Author: Praktikum Sistem Embedded
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// BLE includes
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"

static const char *TAG = "GATT_SVR";

#define LED_PIN             GPIO_NUM_2
#define DEVICE_NAME         "ESP32_GATT"
#define GATTS_APP_ID        0
#define PROFILE_NUM         1

// Custom Service UUID = 0x00FF
#define SERVICE_UUID        0x00FF
#define CHAR_READ_UUID      0xFF01   // Characteristic A: Read sensor
#define CHAR_WRITE_UUID     0xFF02   // Characteristic B: Write LED
#define CHAR_NOTIFY_UUID    0xFF03   // Characteristic C: Notify

// GATT handles
#define GATTS_NUM_HANDLE    8   // Service + Char decl + Char val + CCCD (x3 chars)

// ============================================================================
// Application Profile Structure
// ============================================================================
typedef struct {
    uint16_t gatts_if;
    uint16_t conn_id;
    bool     is_connected;
    uint16_t service_handle;
    uint16_t char_read_handle;
    uint16_t char_write_handle;
    uint16_t char_notify_handle;
    uint16_t descr_notify_handle;
    bool     notify_enabled;
} gatt_profile_t;

static gatt_profile_t gatt_profile = {
    .gatts_if = ESP_GATT_IF_NONE,
    .is_connected = false,
    .notify_enabled = false,
};

// Sensor data (simulated)
static uint8_t sensor_value = 25;

// ============================================================================
// Advertising Parameters
// ============================================================================
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp       = false,
    .include_name       = true,
    .include_txpower    = true,
    .flag               = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// ============================================================================
// GAP Event Handler
// ============================================================================
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Advertising started, waiting for connection...");
            }
            break;
        default:
            break;
    }
}

// ============================================================================
// GATTS Event Handler - inti dari GATT Server
// ============================================================================
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                 esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            // Aplikasi terdaftar, buat service
            ESP_LOGI(TAG, "GATT app registered, creating service...");
            gatt_profile.gatts_if = gatts_if;

            esp_ble_gap_set_device_name(DEVICE_NAME);
            esp_ble_gap_config_adv_data(&adv_data);

            // Buat service
            esp_gatt_srvc_id_t service_id = {
                .is_primary = true,
                .id = {
                    .inst_id = 0,
                    .uuid = {
                        .len = ESP_UUID_LEN_16,
                        .uuid = { .uuid16 = SERVICE_UUID },
                    },
                },
            };
            esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
            break;
        }

        case ESP_GATTS_CREATE_EVT: {
            // Service sudah dibuat, tambahkan characteristics
            ESP_LOGI(TAG, "Service created, handle: %d", param->create.service_handle);
            gatt_profile.service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(gatt_profile.service_handle);

            // Add READ characteristic (sensor value)
            esp_bt_uuid_t char_uuid_read = {
                .len = ESP_UUID_LEN_16,
                .uuid = { .uuid16 = CHAR_READ_UUID },
            };
            esp_ble_gatts_add_char(gatt_profile.service_handle, &char_uuid_read,
                                   ESP_GATT_PERM_READ,
                                   ESP_GATT_CHAR_PROP_BIT_READ,
                                   NULL, NULL);
            break;
        }

        case ESP_GATTS_ADD_CHAR_EVT: {
            // Characteristic ditambahkan
            uint16_t char_uuid = param->add_char.char_uuid.uuid.uuid16;
            ESP_LOGI(TAG, "Characteristic added, UUID: 0x%04X, handle: %d",
                     char_uuid, param->add_char.attr_handle);

            if (char_uuid == CHAR_READ_UUID) {
                gatt_profile.char_read_handle = param->add_char.attr_handle;
                // Tambah WRITE characteristic
                esp_bt_uuid_t uuid_write = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = { .uuid16 = CHAR_WRITE_UUID },
                };
                esp_ble_gatts_add_char(gatt_profile.service_handle, &uuid_write,
                                       ESP_GATT_PERM_WRITE,
                                       ESP_GATT_CHAR_PROP_BIT_WRITE,
                                       NULL, NULL);
            } else if (char_uuid == CHAR_WRITE_UUID) {
                gatt_profile.char_write_handle = param->add_char.attr_handle;
                // Tambah NOTIFY characteristic
                esp_bt_uuid_t uuid_notify = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = { .uuid16 = CHAR_NOTIFY_UUID },
                };
                esp_ble_gatts_add_char(gatt_profile.service_handle, &uuid_notify,
                                       ESP_GATT_PERM_READ,
                                       ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                       NULL, NULL);
            } else if (char_uuid == CHAR_NOTIFY_UUID) {
                gatt_profile.char_notify_handle = param->add_char.attr_handle;
                ESP_LOGI(TAG, "All characteristics added successfully!");
            }
            break;
        }

        case ESP_GATTS_CONNECT_EVT:
            // Client terhubung
            ESP_LOGI(TAG, ">>> Client connected! conn_id=%d, addr=%02x:%02x:%02x:%02x:%02x:%02x",
                     param->connect.conn_id,
                     param->connect.remote_bda[0], param->connect.remote_bda[1],
                     param->connect.remote_bda[2], param->connect.remote_bda[3],
                     param->connect.remote_bda[4], param->connect.remote_bda[5]);
            gatt_profile.conn_id = param->connect.conn_id;
            gatt_profile.is_connected = true;
            gpio_set_level(LED_PIN, 1);  // LED ON saat connected
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            // Client disconnect
            ESP_LOGW(TAG, "<<< Client disconnected, reason: 0x%x", param->disconnect.reason);
            gatt_profile.is_connected = false;
            gatt_profile.notify_enabled = false;
            gpio_set_level(LED_PIN, 0);
            // Restart advertising
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GATTS_READ_EVT:
            // Client membaca characteristic
            ESP_LOGI(TAG, "READ request - handle: %d", param->read.handle);
            if (param->read.handle == gatt_profile.char_read_handle ||
                param->read.handle == gatt_profile.char_notify_handle) {
                esp_gatt_rsp_t rsp;
                memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
                rsp.attr_value.handle = param->read.handle;
                rsp.attr_value.len = 1;
                rsp.attr_value.value[0] = sensor_value;
                esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                                             param->read.trans_id,
                                             ESP_GATT_OK, &rsp);
                ESP_LOGI(TAG, "  Responded with sensor value: %d", sensor_value);
            }
            break;

        case ESP_GATTS_WRITE_EVT:
            // Client menulis ke characteristic
            ESP_LOGI(TAG, "WRITE request - handle: %d, len: %d",
                     param->write.handle, param->write.len);
            if (param->write.handle == gatt_profile.char_write_handle) {
                uint8_t led_state = param->write.value[0];
                gpio_set_level(LED_PIN, led_state ? 1 : 0);
                ESP_LOGI(TAG, "  LED set to: %s", led_state ? "ON" : "OFF");
            }
            // Check if client enabling notifications (CCCD write)
            if (param->write.len == 2 && param->write.value[0] == 0x01 && param->write.value[1] == 0x00) {
                gatt_profile.notify_enabled = true;
                ESP_LOGI(TAG, "  Notifications ENABLED by client");
            } else if (param->write.len == 2 && param->write.value[0] == 0x00 && param->write.value[1] == 0x00) {
                gatt_profile.notify_enabled = false;
                ESP_LOGI(TAG, "  Notifications DISABLED by client");
            }
            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                             param->write.trans_id,
                                             ESP_GATT_OK, NULL);
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// Notification Task - kirim data periodik ke client
// ============================================================================
static void notify_task(void *pvParameters)
{
    while (1) {
        if (gatt_profile.is_connected && gatt_profile.notify_enabled) {
            // Simulasi perubahan sensor
            sensor_value = 20 + (esp_random() % 20);  // 20-39

            // Kirim notification
            esp_ble_gatts_send_indicate(gatt_profile.gatts_if,
                                         gatt_profile.conn_id,
                                         gatt_profile.char_notify_handle,
                                         sizeof(sensor_value),
                                         &sensor_value,
                                         false);  // false = notification, true = indication
            ESP_LOGI(TAG, "Notification sent: sensor=%d", sensor_value);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));  // Kirim setiap 2 detik
    }
}

// ============================================================================
// Main Application Entry Point
// ============================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 BLE GATT Server Demo");
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

    // Release classic BT memory
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Init BT controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    // Init Bluedroid
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Register callbacks
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));

    // Register GATT application
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(GATTS_APP_ID));
    ESP_LOGI(TAG, "BLE GATT Server initialized");

    // Start notification task
    xTaskCreate(notify_task, "notify_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Ready! Connect with BLE client to interact.");
}
