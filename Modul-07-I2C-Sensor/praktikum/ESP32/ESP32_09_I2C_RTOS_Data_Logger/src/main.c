/**
 * ==========================================================
 *  Modul 07 - ESP32_09_I2C_RTOS_Data_Logger
 * ==========================================================
 *  Deskripsi:
 *    RTOS data logger: producer task baca BME280 & MPU6050,
 *    kirim via FreeRTOS queue ke consumer task untuk ditulis
 *    ke EEPROM AT24C32. Gunakan semaphore untuk akses I2C
 *    bersama. Menunjukkan RTOS concurrency + I2C.
 *  Hardware:
 *    ESP32 DevKit / ESP32-S2 / ESP32-S3
 *  Koneksi Pin:
 *    SDA = GPIO21 (ESP32), GPIO8 (S2/S3)
 *    SCL = GPIO22 (ESP32), GPIO9 (S2/S3)
 *  Instruksi:
 *    1. Pasang BME280, MPU6050, AT24C32 EEPROM
 *    2. Ubah nilai #define di atas untuk mencoba variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    I2C_SDA_GPIO GPIO_NUM_21
 *    I2C_SCL_GPIO GPIO_NUM_22
 *    I2C_PORT I2C_NUM_0
 *    I2C_FREQ_HZ 100000
 *    BME280_ADDR 0x76
 *    MPU6050_ADDR 0x68
 *    EEPROM_ADDR 0x50
 *    QUEUE_LENGTH 10
 *    TASK_DELAY_MS 2000
 *    EEPROM_LOG_START 0x0000
 * ==========================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// Konfigurasi pin I2C
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000

// Alamat sensor
#define BME280_ADDR 0x76
#define MPU6050_ADDR 0x68
#define EEPROM_ADDR 0x50

// Queue & Semaphore
#define QUEUE_LENGTH 10

// Delay task
#define TASK_DELAY_MS 2000

// EEPROM log start address
#define EEPROM_LOG_START 0x0000

// Priority & stack
#define TASK_PRIO_PRODUCER 2
#define TASK_PRIO_CONSUMER 3
#define TASK_STACK 4096

static const char *TAG = "RTOS_LOGGER";

// Queue & Semaphore handle
static QueueHandle_t sensor_queue;
static SemaphoreHandle_t i2c_mutex;

// I2C bus & devices
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev_bme, dev_mpu, dev_eeprom;

// BME280 calibration data
static uint16_t dig_T1, dig_P1;
static int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t dig_H1, dig_H3;
static int16_t dig_H2, dig_H4, dig_H5, dig_H6;

// Read calibration data from BME280
static void bme280_read_calibration(void) {
    uint8_t cal1[26];
    i2c_master_transmit_receive(dev_bme, (uint8_t[]){0x88}, 1, cal1, 26, -1);
    dig_T1 = (uint16_t)cal1[0] | ((uint16_t)cal1[1] << 8);
    dig_T2 = (int16_t)((uint16_t)cal1[2] | ((uint16_t)cal1[3] << 8));
    dig_T3 = (int16_t)((uint16_t)cal1[4] | ((uint16_t)cal1[5] << 8));
    dig_P1 = (uint16_t)cal1[6] | ((uint16_t)cal1[7] << 8);
    dig_P2 = (int16_t)((uint16_t)cal1[8] | ((uint16_t)cal1[9] << 8));
    dig_P3 = (int16_t)((uint16_t)cal1[10] | ((uint16_t)cal1[11] << 8));
    dig_P4 = (int16_t)((uint16_t)cal1[12] | ((uint16_t)cal1[13] << 8));
    dig_P5 = (int16_t)((uint16_t)cal1[14] | ((uint16_t)cal1[15] << 8));
    dig_P6 = (int16_t)((uint16_t)cal1[16] | ((uint16_t)cal1[17] << 8));
    dig_P7 = (int16_t)((uint16_t)cal1[18] | ((uint16_t)cal1[19] << 8));
    dig_P8 = (int16_t)((uint16_t)cal1[20] | ((uint16_t)cal1[21] << 8));
    dig_P9 = (int16_t)((uint16_t)cal1[22] | ((uint16_t)cal1[23] << 8));
    dig_H1 = cal1[25];
    uint8_t cal2[7];
    i2c_master_transmit_receive(dev_bme, (uint8_t[]){0xE1}, 1, cal2, 7, -1);
    dig_H2 = (int16_t)((uint16_t)cal2[0] | ((uint16_t)cal2[1] << 8));
    dig_H3 = cal2[2];
    dig_H4 = (int16_t)((cal2[3] << 4) | (cal2[4] & 0x0F));
    dig_H5 = (int16_t)((cal2[5] << 4) | (cal2[4] >> 4));
    dig_H6 = (int8_t)cal2[6];
}

// Struktur data untuk queue
typedef struct {
    uint32_t timestamp;
    float bme_temp;
    float bme_press;
    float mpu_roll;
    float mpu_pitch;
} log_data_t;

// Init BME280
static bool init_bme280(void) {
    i2c_device_config_t dev_cfg = {
        .device_address = BME280_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &dev_bme) != ESP_OK) return false;
    uint8_t chip_id;
    i2c_master_transmit_receive(dev_bme, (uint8_t[]){0xD0}, 1, &chip_id, 1, -1);
    if (chip_id != 0x60) { i2c_master_bus_rm_device(dev_bme); return false; }
    i2c_master_transmit(dev_bme, (uint8_t[]){0xF2, 0x01}, 2, -1);
    i2c_master_transmit(dev_bme, (uint8_t[]){0xF4, 0x27}, 2, -1);
    bme280_read_calibration();
    return true;
}

// Init MPU6050
static bool init_mpu6050(void) {
    i2c_device_config_t dev_cfg = {
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &dev_mpu) != ESP_OK) return false;
    i2c_master_transmit(dev_mpu, (uint8_t[]){0x6B, 0x00}, 2, -1);
    return true;
}

// Init EEPROM
static bool init_eeprom(void) {
    i2c_device_config_t dev_cfg = {
        .device_address = EEPROM_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    return i2c_master_bus_add_device(bus, &dev_cfg, &dev_eeprom) == ESP_OK;
}

// Tulis EEPROM (alamat 16-bit)
static void eeprom_write(uint16_t addr, const uint8_t *data, size_t len) {
    uint8_t buf[34];
    buf[0] = addr >> 8;
    buf[1] = addr & 0xFF;
    memcpy(&buf[2], data, len);
    i2c_master_transmit(dev_eeprom, buf, len + 2, -1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Producer task: baca sensor, kirim ke queue
static void producer_task(void *arg) {
    ESP_LOGI(TAG, "Producer task started");
    log_data_t data;
    uint16_t eeprom_ptr = EEPROM_LOG_START;

    while (1) {
        // Ambil mutex untuk akses I2C
        if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
            // Baca BME280
            uint8_t raw[8];
            i2c_master_transmit_receive(dev_bme, (uint8_t[]){0xF7}, 1, raw, 8, -1);
            int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
            int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
            int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

            // Kompensasi suhu
            int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * dig_T2) >> 11;
            int32_t var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
            int32_t t_fine = var1 + var2;
            data.bme_temp = (t_fine * 5 + 128) >> 8;
            data.bme_temp /= 100.0f;

            // Kompensasi tekanan (gunakan int64_t)
            int64_t var1_64, var2_64;
            var1_64 = (int64_t)t_fine - 128000;
            var2_64 = var1_64 * var1_64 * (int64_t)dig_P6;
            var2_64 = var2_64 + ((var1_64 * (int64_t)dig_P5) << 17);
            var2_64 = var2_64 + ((int64_t)dig_P4 << 35);
            var1_64 = ((var1_64 * var1_64 * (int64_t)dig_P3) >> 8) + ((var1_64 * (int64_t)dig_P2) << 12);
            var1_64 = (((((int64_t)1) << 47) + var1_64)) * (int64_t)dig_P1 >> 33;
            if (var1_64 != 0) {
                int64_t p = 1048576 - adc_P;
                p = (((p << 31) - var2_64) * 3125) / var1_64;
                int64_t var3 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
                int64_t var4 = ((int64_t)dig_P8 * p) >> 19;
                p = ((p + var3 + var4) >> 8) + (((int64_t)dig_P7) << 4);
                data.bme_press = p / 256.0f;
            } else {
                data.bme_press = 0;
            }

            // Baca MPU6050
            uint8_t mpu_raw[14];
            i2c_master_transmit_receive(dev_mpu, (uint8_t[]){0x3B}, 1, mpu_raw, 14, -1);
            int16_t ax = (int16_t)((mpu_raw[0] << 8) | mpu_raw[1]);
            int16_t ay = (int16_t)((mpu_raw[2] << 8) | mpu_raw[3]);
            int16_t az = (int16_t)((mpu_raw[4] << 8) | mpu_raw[5]);
            float ax_g = ax / 16384.0f, ay_g = ay / 16384.0f, az_g = az / 16384.0f;
            data.mpu_roll = atan2f(ay_g, az_g) * 57.2958f;
            data.mpu_pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * 57.2958f;

            data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;

            xSemaphoreGive(i2c_mutex);
        }

        // Kirim ke queue
        if (xQueueSend(sensor_queue, &data, 0) != pdPASS) {
            ESP_LOGW(TAG, "Queue penuh, data dibuang");
        } else {
            ESP_LOGI(TAG, "Producer: data dikirim ke queue");
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }
}

// Consumer task: terima dari queue, tulis ke EEPROM
static void consumer_task(void *arg) {
    ESP_LOGI(TAG, "Consumer task started");
    log_data_t data;
    uint16_t eeprom_ptr = EEPROM_LOG_START;

    while (1) {
        if (xQueueReceive(sensor_queue, &data, portMAX_DELAY) == pdPASS) {
            char log_str[64];
            int len = snprintf(log_str, sizeof(log_str), "%lu,%.1f,%.1f,%.1f,%.1f\n",
                             (unsigned long)data.timestamp, data.bme_temp, data.bme_press,
                             data.mpu_roll, data.mpu_pitch);

            // Ambil mutex untuk akses EEPROM
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                eeprom_write(eeprom_ptr, (uint8_t *)log_str, len);
                ESP_LOGI(TAG, "Consumer: tulis EEPROM @ 0x%04X: '%s'", eeprom_ptr, log_str);
                xSemaphoreGive(i2c_mutex);
            }

            eeprom_ptr += len;
            if (eeprom_ptr >= 0x1000) eeprom_ptr = EEPROM_LOG_START;  // Wrap 4KB
        }
    }
}

void app_main(void) {
    // Init I2C
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    // Init sensor & EEPROM
    if (!init_bme280()) ESP_LOGE(TAG, "BME280 tidak ditemukan");
    if (!init_mpu6050()) ESP_LOGE(TAG, "MPU6050 tidak ditemukan");
    if (!init_eeprom()) ESP_LOGE(TAG, "EEPROM tidak ditemukan");

    // Create mutex & queue
    i2c_mutex = xSemaphoreCreateMutex();
    sensor_queue = xQueueCreate(QUEUE_LENGTH, sizeof(log_data_t));

    if (i2c_mutex == NULL || sensor_queue == NULL) {
        ESP_LOGE(TAG, "Gagal membuat RTOS objects");
        return;
    }

    // Create tasks
    xTaskCreate(producer_task, "producer", TASK_STACK, NULL, TASK_PRIO_PRODUCER, NULL);
    xTaskCreate(consumer_task, "consumer", TASK_STACK, NULL, TASK_PRIO_CONSUMER, NULL);

    ESP_LOGI(TAG, "Producer-Consumer RTOS berjalan");
}
