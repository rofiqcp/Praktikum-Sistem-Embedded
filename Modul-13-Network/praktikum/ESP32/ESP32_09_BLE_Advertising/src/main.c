/**
 * ============================================================================
 * ESP32_09_BLE_Advertising - BLE GAP Advertising & Scan Response
 * ============================================================================
 * 
 * Deskripsi:
 *   Program ini mendemonstrasikan BLE (Bluetooth Low Energy) advertising
 *   menggunakan ESP-IDF GAP (Generic Access Profile) API. ESP32 akan
 *   melakukan broadcast advertising packets yang bisa di-scan oleh
 *   perangkat BLE lain (smartphone, laptop, dll).
 * 
 * Konsep Utama:
 *   - BLE GAP Advertising: broadcast data tanpa koneksi
 *   - Advertising Data: device name, TX power, appearance
 *   - Scan Response Data: data tambahan saat di-scan
 *   - GAP Event Handling: callback untuk event BLE
 * 
 * Wiring Diagram:
 *   Tidak ada wiring khusus - menggunakan BLE internal ESP32
 *   
 *   [ESP32 Dev Board]
 *   - Internal BLE antenna digunakan
 *   - LED_BUILTIN (GPIO2) sebagai status indicator
 * 
 * Expected Output:
 *   I (xxx) BLE_ADV: Initializing BLE...
 *   I (xxx) BLE_ADV: BT controller initialized
 *   I (xxx) BLE_ADV: Bluedroid initialized and enabled
 *   I (xxx) BLE_ADV: Device name set: ESP32_BLE_ADV
 *   I (xxx) BLE_ADV: Advertising data configured
 *   I (xxx) BLE_ADV: Advertising started successfully!
 *   I (xxx) BLE_ADV: === BLE Advertising Active ===
 * 
 * Author: Praktikum Sistem Embedded
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// BLE includes
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

static const char *TAG = "BLE_ADV";

// LED untuk status indicator
#define LED_STATUS_PIN  GPIO_NUM_2

// Nama device BLE yang akan di-advertise
#define DEVICE_NAME     "ESP32_BLE_ADV"

// Flag untuk tracking status advertising
static bool is_advertising = false;

// ============================================================================
// Konfigurasi Advertising Parameters
// ============================================================================
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,     // Min interval: 20ms (0x20 * 0.625ms)
    .adv_int_max        = 0x40,     // Max interval: 40ms (0x40 * 0.625ms)
    .adv_type           = ADV_TYPE_IND,  // Connectable undirected advertising
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,  // Advertise di semua channel (37,38,39)
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// ============================================================================
// Advertising Data - data yang dikirim saat advertising
// ============================================================================
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp       = false,
    .include_name       = true,      // Sertakan device name
    .include_txpower    = true,      // Sertakan TX power level
    .min_interval       = 0x0006,    // 7.5ms (slave connection interval)
    .max_interval       = 0x0010,    // 20ms
    .appearance         = 0x0180,    // Generic Computer appearance
    .manufacturer_len   = 0,
    .p_manufacturer_data = NULL,
    .service_data_len   = 0,
    .p_service_data     = NULL,
    .service_uuid_len   = 0,
    .p_service_uuid     = NULL,
    .flag               = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// ============================================================================
// Scan Response Data - data tambahan saat device di-scan
// ============================================================================
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp       = true,      // Ini adalah scan response
    .include_name       = true,
    .include_txpower    = true,
    .appearance         = 0x0180,
    .manufacturer_len   = 0,
    .p_manufacturer_data = NULL,
    .service_data_len   = 0,
    .p_service_data     = NULL,
    .service_uuid_len   = 0,
    .p_service_uuid     = NULL,
    .flag               = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// ============================================================================
// GAP Event Handler - menangani semua event BLE GAP
// ============================================================================
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            // Advertising data sudah di-set, sekarang set scan response
            ESP_LOGI(TAG, "Advertising data set complete");
            esp_ble_gap_config_adv_data(&scan_rsp_data);
            break;

        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            // Scan response data sudah di-set, mulai advertising
            ESP_LOGI(TAG, "Scan response data set complete");
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            // Advertising sudah dimulai
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "=== BLE Advertising Started Successfully! ===");
                is_advertising = true;
                gpio_set_level(LED_STATUS_PIN, 1);  // LED ON = advertising aktif
            } else {
                ESP_LOGE(TAG, "Advertising start failed, status: %d",
                         param->adv_start_cmpl.status);
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            // Advertising dihentikan
            if (param->adv_stop_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Advertising stopped");
                is_advertising = false;
                gpio_set_level(LED_STATUS_PIN, 0);
            } else {
                ESP_LOGE(TAG, "Advertising stop failed");
            }
            break;

        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            // Connection parameters updated
            ESP_LOGI(TAG, "Connection params update - status: %d, interval: %d",
                     param->update_conn_params.status,
                     param->update_conn_params.conn_int);
            break;

        default:
            ESP_LOGW(TAG, "Unhandled GAP event: %d", event);
            break;
    }
}

// ============================================================================
// Inisialisasi BLE Stack
// ============================================================================
static esp_err_t ble_init(void)
{
    esp_err_t ret;

    // Step 1: Release memory Classic BT (kita hanya pakai BLE)
    ESP_LOGI(TAG, "Releasing classic BT memory...");
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "BT memory release warning: %s", esp_err_to_name(ret));
    }

    // Step 2: Initialize BT controller dengan default config
    ESP_LOGI(TAG, "Initializing BT controller...");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 3: Enable BT controller dalam mode BLE only
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "BT controller initialized and enabled (BLE mode)");

    // Step 4: Initialize Bluedroid stack
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 5: Enable Bluedroid
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Bluedroid initialized and enabled");

    // Step 6: Register GAP callback
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 7: Set device name
    ret = esp_ble_gap_set_device_name(DEVICE_NAME);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set device name failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Device name set: %s", DEVICE_NAME);

    // Step 8: Configure advertising data (akan trigger callback chain)
    ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Config advertising data failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

// ============================================================================
// Status Monitoring Task
// ============================================================================
static void status_task(void *pvParameters)
{
    int counter = 0;
    while (1) {
        counter++;
        if (is_advertising) {
            ESP_LOGI(TAG, "[%d] BLE Advertising active - Device: %s", counter, DEVICE_NAME);
            // Toggle LED untuk menunjukkan aktivitas
            gpio_set_level(LED_STATUS_PIN, counter % 2);
        } else {
            ESP_LOGW(TAG, "[%d] BLE Advertising NOT active", counter);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));  // Log setiap 5 detik
    }
}

// ============================================================================
// Main Application Entry Point
// ============================================================================
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ESP32 BLE Advertising Demo");
    ESP_LOGI(TAG, "  Praktikum Sistem Embedded");
    ESP_LOGI(TAG, "========================================");

    // Konfigurasi LED status
    gpio_reset_pin(LED_STATUS_PIN);
    gpio_set_direction(LED_STATUS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_STATUS_PIN, 0);

    // Initialize NVS (diperlukan oleh BLE stack)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash erase and reinit...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized");

    // Initialize BLE
    ret = ble_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE initialization failed!");
        return;
    }

    // Buat task monitoring status
    xTaskCreate(status_task, "status_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Setup complete. BLE advertising will start shortly...");
    ESP_LOGI(TAG, "Gunakan smartphone/PC BLE scanner untuk melihat device.");
}
