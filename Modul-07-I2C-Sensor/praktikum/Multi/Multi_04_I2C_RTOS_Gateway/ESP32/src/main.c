/**
 * Multi_04_I2C_RTOS_Gateway - Sisi ESP32
 * 
 * Deskripsi:
 *   ESP32 menggunakan FreeRTOS dengan 3 task (sensor read, I2C slave response, logger).
 *   Bertindak sebagai gateway yang membaca sensor BH1750 dan merespon slave I2C.
 * 
 * Hardware:
 *   - ESP32 DevKit / LoLin S2 Mini / ESP32-S3 DevKitC
 *   - STM32 sebagai slave I2C
 *   - Sensor BH1750 di I2C bus
 * 
 * Koneksi Pin:
 *   ESP32 GPIO21 (SDA) <-> STM32 PB7
 *   ESP32 GPIO22 (SCL) <-> STM32 PB6
 *   ESP32 GPIO21 (SDA) <-> BH1750 SDA
 *   ESP32 GPIO22 (SCL) <-> BH1750 SCL
 *   GND ESP32 <-> GND STM32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke ESP32
 *   2. Pastikan STM32 sudah diupload dengan kode slave
 *   3. Buka serial monitor 115200 baud
 *   4. Observer 3 task RTOS yang berjalan bersamaan
 * 
 * Variabel yang bisa dicoba:
 *   - SDA_PIN: Pin SDA (default 21)
 *   - SCL_PIN: Pin SCL (default 22)
 *   - STM32_ADDR: Alamat I2C slave STM32 (default 0x42)
 *   - BH1750_ADDR: Alamat BH1750 (default 0x23)
 *   - SENSOR_POLL_MS: Interval poll sensor (default 500ms)
 *   - SLAVE_RESP_QUEUE_LEN: Panjang queue response (default 5)
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define SDA_PIN 21                   // Pin SDA
#define SCL_PIN 22                   // Pin SCL
#define STM32_ADDR 0x42              // Alamat STM32 slave
#define BH1750_ADDR 0x23             // Alamat sensor BH1750
#define SENSOR_POLL_MS 500           // Interval poll sensor
#define SLAVE_RESP_QUEUE_LEN 5       // Panjang queue response

static const char *TAG = "M04_ESP32_GATEWAY";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t stm32_dev, bh1750_dev;
static QueueHandle_t sensor_queue;    // Queue untuk data sensor

// Struktur data sensor
typedef struct {
    uint16_t lux;
    uint32_t timestamp;
} sensor_data_t;

// Task 1: Baca sensor BH1750
static void sensor_read_task(void *arg) {
    sensor_data_t data;
    uint8_t cmd = 0x10;  // Command untuk BH1750 one-time measurement
    while (1) {
        i2c_master_transmit(bh1750_dev, &cmd, 1, pdMS_TO_TICKS(100));
        vTaskDelay(pdMS_TO_TICKS(180));  // Tunggu measurement
        
        uint8_t raw[2] = {0};
        if (i2c_master_receive(bh1750_dev, raw, 2, pdMS_TO_TICKS(100)) == ESP_OK) {
            data.lux = ((raw[0] << 8) | raw[1]) / 1.2;
            data.timestamp = xTaskGetTickCount();
            xQueueSend(sensor_queue, &data, 0);
            ESP_LOGI(TAG, "Sensor baca: lux=%u", data.lux);
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_MS));
    }
}

// Task 2: Response ke STM32 slave request
static void i2c_slave_response_task(void *arg) {
    uint8_t frame[4];
    while (1) {
        sensor_data_t data;
        if (xQueueReceive(sensor_queue, &data, portMAX_DELAY) == pdPASS) {
            frame[0] = 0x00;          // Status register
            frame[1] = 0xA4;          // Status value
            frame[2] = data.lux >> 8; // Lux MSB
            frame[3] = data.lux & 0xFF; // Lux LSB
            
            // Kirim ke STM32 slave
            if (i2c_master_transmit(stm32_dev, frame, sizeof(frame), pdMS_TO_TICKS(100)) == ESP_OK) {
                ESP_LOGI(TAG, "Data sensor dikirim ke STM32");
            }
        }
    }
}

// Task 3: Logger - log data ke serial
static void logger_task(void *arg) {
    while (1) {
        sensor_data_t data;
        if (xQueuePeek(sensor_queue, &data, 0) == pdPASS) {
            ESP_LOGI(TAG, "[LOG] lux=%u, time=%lu ms", data.lux, data.timestamp);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    
    i2c_device_config_t bh1750_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(bus, &bh1750_cfg, &bh1750_dev);
}

void app_main(void) {
    i2c_init();
    sensor_queue = xQueueCreate(SLAVE_RESP_QUEUE_LEN, sizeof(sensor_data_t));
    
    // Buat 3 FreeRTOS tasks
    xTaskCreate(sensor_read_task, "sensor_read", 4096, NULL, 5, NULL);
    xTaskCreate(i2c_slave_response_task, "i2c_response", 4096, NULL, 4, NULL);
    xTaskCreate(logger_task, "logger", 2048, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "Multi_04 RTOS Gateway started dengan 3 tasks");
}
