/**
 * Multi_05_I2C_RTOS_Weather_Station - Sisi ESP32
 * 
 * Deskripsi:
 *   ESP32 menggunakan FreeRTOS dengan multiple tasks untuk weather station.
 *   Membaca BME280, BH1750, DS3231 dan menampilkan ke OLED.
 * 
 * Hardware:
 *   - ESP32 DevKit / LoLin S2 Mini / ESP32-S3 DevKitC
 *   - STM32 sebagai slave I2C (opsional)
 *   - Sensor BME280, BH1750, DS3231
 *   - OLED SSD1306 (opsional)
 * 
 * Koneksi Pin:
 *   ESP32 GPIO21 (SDA) <-> Sensor SDA / STM32 PB7
 *   ESP32 GPIO22 (SCL) <-> Sensor SCL / STM32 PB6
 *   GND ESP32 <-> GND semua device
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke ESP32
 *   2. Pastikan sensor terhubung dengan benar
 *   3. Buka serial monitor 115200 baud
 *   4. Observer multiple RTOS tasks berjalan bersamaan
 * 
 * Variabel yang bisa dicoba:
 *   - SDA_PIN: Pin SDA (default 21)
 *   - SCL_PIN: Pin SCL (default 22)
 *   - STM32_ADDR: Alamat I2C STM32 (default 0x42)
 *   - BME280_ADDR: Alamat BME280 (default 0x76)
 *   - BH1750_ADDR: Alamat BH1750 (default 0x23)
 *   - DS3231_ADDR: Alamat DS3231 (default 0x68)
 *   - READ_INTERVAL_MS: Interval baca sensor (default 1000ms)
 *   - DISPLAY_UPDATE_MS: Interval update display (default 2000ms)
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define SDA_PIN 21                   // Pin SDA
#define SCL_PIN 22                   // Pin SCL
#define STM32_ADDR 0x42              // Alamat STM32 slave
#define BME280_ADDR 0x76             // Alamat BME280
#define BH1750_ADDR 0x23             // Alamat BH1750
#define DS3231_ADDR 0x68             // Alamat DS3231
#define READ_INTERVAL_MS 1000        // Interval baca sensor
#define DISPLAY_UPDATE_MS 2000       // Interval update display

static const char *TAG = "M05_ESP32_WEATHER";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t stm32_dev, bme280_dev, bh1750_dev, ds3231_dev;
static QueueHandle_t weather_queue;   // Queue data cuaca
static SemaphoreHandle_t i2c_mutex;  // Mutex untuk akses I2C

// Struktur data cuaca
typedef struct {
    float temperature;
    float humidity;
    uint16_t lux;
    uint8_t hour, minute, second;
} weather_data_t;

// Task 1: Baca BME280
static void bme280_task(void *arg) {
    uint8_t cmd = 0xF4;  // Command untuk baca
    while (1) {
        weather_data_t data = {0};
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        // Simulasi baca BME280 (dalam praktik nyata baca register 0xFA, 0xFD)
        data.temperature = 25.5 + (rand() % 50) / 10.0;
        data.humidity = 60.0 + (rand() % 300) / 10.0;
        xSemaphoreGive(i2c_mutex);
        
        ESP_LOGI(TAG, "BME280: temp=%.1fC, hum=%.1f%%", data.temperature, data.humidity);
        xQueueSend(weather_queue, &data, 0);
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}

// Task 2: Baca BH1750
static void bh1750_task(void *arg) {
    uint8_t cmd = 0x10;  // One-time measurement
    while (1) {
        weather_data_t data = {0};
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        i2c_master_transmit(bh1750_dev, &cmd, 1, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(180));
        uint8_t raw[2] = {0};
        if (i2c_master_receive(bh1750_dev, raw, 2, pdMS_TO_TICKS(100)) == ESP_OK) {
            data.lux = ((raw[0] << 8) | raw[1]) / 1.2;
        } else {
            data.lux = 300 + (rand() % 400);
        }
        xSemaphoreGive(i2c_mutex);
        
        ESP_LOGI(TAG, "BH1750: lux=%.0f", data.lux);
        weather_data_t existing;
        if (xQueuePeek(weather_queue, &existing, 0) == pdPASS) {
            existing.lux = data.lux;
            xQueueOverwrite(weather_queue, &existing);
        }
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}

// Task 3: Baca DS3231 RTC
static void ds3231_task(void *arg) {
    uint8_t reg = 0x00;  // Register detik
    while (1) {
        weather_data_t data = {0};
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        // Simulasi baca DS3231 (dalam praktik nyata baca register 0x00-0x02)
        data.hour = 14;
        data.minute = 30;
        data.second = (xTaskGetTickCount() / 1000) % 60;
        xSemaphoreGive(i2c_mutex);
        
        ESP_LOGI(TAG, "DS3231: %02u:%02u:%02u", data.hour, data.minute, data.second);
        weather_data_t existing;
        if (xQueuePeek(weather_queue, &existing, 0) == pdPASS) {
            existing.hour = data.hour;
            existing.minute = data.minute;
            existing.second = data.second;
            xQueueOverwrite(weather_queue, &existing);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Task 4: Update display/OLED (simulasi kirim ke STM32)
static void display_task(void *arg) {
    while (1) {
        weather_data_t data;
        if (xQueueReceive(weather_queue, &data, portMAX_DELAY) == pdPASS) {
            // Kirim ke STM32 slave untuk display
            uint8_t pkt[12] = {
                0x00, 0x55,                    // Header
                (int)(data.temperature * 10) >> 8,
                (int)(data.temperature * 10) & 0xFF,
                (int)(data.humidity * 10) >> 8,
                (int)(data.humidity * 10) & 0xFF,
                data.lux >> 8,
                data.lux & 0xFF,
                data.hour,
                data.minute
            };
            xSemaphoreTake(i2c_mutex, portMAX_DELAY);
            if (i2c_master_transmit(stm32_dev, pkt, sizeof(pkt), pdMS_TO_TICKS(200)) == ESP_OK) {
                ESP_LOGI(TAG, "Display update: %.1fC, %.1f%%, %ulux, %02u:%02u",
                    data.temperature, data.humidity, data.lux, data.hour, data.minute);
            }
            xSemaphoreGive(i2c_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_MS));
    }
}

// Inisialisasi I2C
static void i2c_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
    
    i2c_device_config_t stm32_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = STM32_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(bus, &stm32_cfg, &stm32_dev);
    
    // Init sensor devices (simulasi)
    i2c_device_config_t bme280_cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = BME280_ADDR, .scl_speed_hz = 100000};
    i2c_master_bus_add_device(bus, &bme280_cfg, &bme280_dev);
    
    i2c_device_config_t bh1750_cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = BH1750_ADDR, .scl_speed_hz = 100000};
    i2c_master_bus_add_device(bus, &bh1750_cfg, &bh1750_dev);
    
    i2c_device_config_t ds3231_cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = DS3231_ADDR, .scl_speed_hz = 100000};
    i2c_master_bus_add_device(bus, &ds3231_cfg, &ds3231_dev);
}

void app_main(void) {
    i2c_init();
    weather_queue = xQueueCreate(5, sizeof(weather_data_t));
    i2c_mutex = xSemaphoreCreateMutex();
    
    // Buat multiple RTOS tasks
    xTaskCreate(bme280_task, "bme280", 4096, NULL, 3, NULL);
    xTaskCreate(bh1750_task, "bh1750", 4096, NULL, 3, NULL);
    xTaskCreate(ds3231_task, "ds3231", 2048, NULL, 2, NULL);
    xTaskCreate(display_task, "display", 4096, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "Multi_05 RTOS Weather Station started dengan 4 tasks");
}
