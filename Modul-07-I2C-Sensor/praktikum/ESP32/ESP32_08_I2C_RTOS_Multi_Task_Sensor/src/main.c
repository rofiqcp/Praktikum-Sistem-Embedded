/**
 * ==========================================================
 *  Modul 07 - ESP32_08_I2C_RTOS_Multi_Task_Sensor
 * ==========================================================
 *  Deskripsi:
 *    RTOS multi-task sensor reading via I2C. 3 task sensor
 *    (BME280, BH1750, MPU6050) membaca data secara independen
 *    dan mengirim ke aggregator task via FreeRTOS queue.
 *    Menunjukkan kekuatan RTOS + I2C concurrency.
 *  Hardware:
 *    ESP32 DevKit / ESP32-S2 / ESP32-S3
 *  Koneksi Pin:
 *    SDA = GPIO21 (ESP32), GPIO8 (S2/S3)
 *    SCL = GPIO22 (ESP32), GPIO9 (S2/S3)
 *  Instruksi:
 *    1. Pasang semua sensor (BME280, BH1750, MPU6050)
 *    2. Ubah nilai #define di atas untuk mencoba variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    I2C_SDA_GPIO GPIO_NUM_21
 *    I2C_SCL_GPIO GPIO_NUM_22
 *    I2C_PORT I2C_NUM_0
 *    I2C_FREQ_HZ 100000
 *    QUEUE_LENGTH 10
 *    BME280_ADDR 0x76
 *    BH1750_ADDR 0x23
 *    MPU6050_ADDR 0x68
 *    TASK_DELAY_MS 1000
 * ==========================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// Konfigurasi pin I2C
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000

// Alamat sensor
#define BME280_ADDR 0x76
#define BH1750_ADDR 0x23
#define MPU6050_ADDR 0x68

// Queue
#define QUEUE_LENGTH 10

// Delay tiap task
#define TASK_DELAY_MS 1000

// Priority task
#define TASK_PRIO_BME 2
#define TASK_PRIO_BH  2
#define TASK_PRIO_MPU 2
#define TASK_PRIO_AGG 3

// Stack size
#define TASK_STACK 4096

static const char *TAG = "RTOS_SENSOR";

// Queue handle
static QueueHandle_t sensor_queue;

// I2C bus & devices
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev_bme, dev_bh, dev_mpu;

// Struktur data sensor untuk queue
typedef struct {
    char sensor_name[16];
    float value1;
    float value2;
    float value3;
} sensor_data_t;

// BME280 register
#define BME280_REG_CHIP_ID 0xD0
#define BME280_REG_CALIB00 0x88
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_CTRL_HUM 0xF2

// BME280 calibration data
static uint16_t dig_T1, dig_P1;
static int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t dig_H1, dig_H3;
static int16_t dig_H2, dig_H4, dig_H5, dig_H6;

// Read calibration data from BME280
static void bme280_read_calibration(void) {
    uint8_t cal1[26];
    uint8_t reg = BME280_REG_CALIB00;
    i2c_master_transmit_receive(dev_bme, &reg, 1, cal1, 26, -1);
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
    reg = 0xE1;
    i2c_master_transmit_receive(dev_bme, &reg, 1, cal2, 7, -1);
    dig_H2 = (int16_t)((uint16_t)cal2[0] | ((uint16_t)cal2[1] << 8));
    dig_H3 = cal2[2];
    dig_H4 = (int16_t)((cal2[3] << 4) | (cal2[4] & 0x0F));
    dig_H5 = (int16_t)((cal2[5] << 4) | (cal2[4] >> 4));
    dig_H6 = (int8_t)cal2[6];
}

// Init BME280
static bool init_bme280(void) {
    i2c_device_config_t dev_cfg = {
        .device_address = BME280_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &dev_bme) != ESP_OK) {
        return false;
    }
    uint8_t chip_id;
    i2c_master_transmit_receive(dev_bme, (uint8_t[]){BME280_REG_CHIP_ID}, 1, &chip_id, 1, -1);
    if (chip_id != 0x60) {
        i2c_master_bus_rm_device(dev_bme);
        return false;
    }
    i2c_master_transmit(dev_bme, (uint8_t[]){BME280_REG_CTRL_HUM, 0x01}, 2, -1);
    i2c_master_transmit(dev_bme, (uint8_t[]){BME280_REG_CTRL_MEAS, 0x27}, 2, -1);
    bme280_read_calibration();
    return true;
}

// BME280 task
static void bme280_task(void *arg) {
    ESP_LOGI(TAG, "BME280 task started");
    while (1) {
        uint8_t raw[8];
        i2c_master_transmit_receive(dev_bme, (uint8_t[]){BME280_REG_PRESS_MSB}, 1, raw, 8, -1);

        int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
        int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
        int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

        // Kompensasi suhu
        int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * dig_T2) >> 11;
        int32_t var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
        int32_t t_fine = var1 + var2;
        float temp = (t_fine * 5 + 128) >> 8;
        temp /= 100.0f;

        // Kompensasi tekanan (gunakan int64_t)
        int64_t var1_64, var2_64;
        var1_64 = (int64_t)t_fine - 128000;
        var2_64 = var1_64 * var1_64 * (int64_t)dig_P6;
        var2_64 = var2_64 + ((var1_64 * (int64_t)dig_P5) << 17);
        var2_64 = var2_64 + ((int64_t)dig_P4 << 35);
        var1_64 = ((var1_64 * var1_64 * (int64_t)dig_P3) >> 8) + ((var1_64 * (int64_t)dig_P2) << 12);
        var1_64 = (((((int64_t)1) << 47) + var1_64)) * (int64_t)dig_P1 >> 33;
        float pressure = 0;
        if (var1_64 != 0) {
            int64_t p = 1048576 - adc_P;
            p = (((p << 31) - var2_64) * 3125) / var1_64;
            int64_t var3 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
            int64_t var4 = ((int64_t)dig_P8 * p) >> 19;
            p = ((p + var3 + var4) >> 8) + (((int64_t)dig_P7) << 4);
            pressure = p / 256.0f;
        }

        // Kompensasi kelembaban
        int32_t v_x1 = t_fine - 76800;
        v_x1 = (((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1)) + 16384) >> 15;
        v_x1 = (v_x1 * (((((v_x1 * (int32_t)dig_H6) >> 10) * (((v_x1 * (int32_t)dig_H3) >> 11) + 32768)) >> 10) + 2097152) * (int32_t)dig_H2 + 8192) >> 14;
        v_x1 = v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * (int32_t)dig_H1) >> 4);
        v_x1 = v_x1 < 0 ? 0 : v_x1;
        v_x1 = v_x1 > 419430400 ? 419430400 : v_x1;
        float humidity = v_x1 / 1024.0f;
        humidity /= 1024.0f;

        sensor_data_t data = {
            .value1 = temp,
            .value2 = pressure,
            .value3 = humidity,
        };
        strcpy(data.sensor_name, "BME280");
        xQueueSend(sensor_queue, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }
}

// BH1750 task
static void bh1750_task(void *arg) {
    ESP_LOGI(TAG, "BH1750 task started");
    i2c_device_config_t dev_cfg = {
        .device_address = BH1750_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    i2c_master_bus_add_device(bus, &dev_cfg, &dev_bh);

    while (1) {
        uint8_t cmd = 0x10;
        i2c_master_transmit(dev_bh, &cmd, 1, -1);
        vTaskDelay(pdMS_TO_TICKS(180));

        uint8_t raw[2];
        i2c_master_receive(dev_bh, raw, 2, -1);
        float lux = ((raw[0] << 8) | raw[1]) / 1.2f;

        sensor_data_t data = {
            .value1 = lux,
            .value2 = 0,
            .value3 = 0,
        };
        strcpy(data.sensor_name, "BH1750");
        xQueueSend(sensor_queue, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }
}

// MPU6050 task
static void mpu6050_task(void *arg) {
    ESP_LOGI(TAG, "MPU6050 task started");
    i2c_device_config_t dev_cfg = {
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };
    i2c_master_bus_add_device(bus, &dev_cfg, &dev_mpu);

    // Wake up MPU6050
    i2c_master_transmit(dev_mpu, (uint8_t[]){0x6B, 0x00}, 2, -1);

    while (1) {
        uint8_t raw[14];
        i2c_master_transmit_receive(dev_mpu, (uint8_t[]){0x3B}, 1, raw, 14, -1);

        int16_t ax = (int16_t)((raw[0] << 8) | raw[1]);
        int16_t ay = (int16_t)((raw[2] << 8) | raw[3]);
        int16_t az = (int16_t)((raw[4] << 8) | raw[5]);

        float ax_g = ax / 16384.0f;
        float ay_g = ay / 16384.0f;
        float az_g = az / 16384.0f;

        float roll = atan2f(ay_g, az_g) * 57.2958f;
        float pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * 57.2958f;

        sensor_data_t data = {
            .value1 = roll,
            .value2 = pitch,
            .value3 = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g),
        };
        strcpy(data.sensor_name, "MPU6050");
        xQueueSend(sensor_queue, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }
}

// Aggregator task
static void aggregator_task(void *arg) {
    ESP_LOGI(TAG, "Aggregator task started");
    sensor_data_t data;

    while (1) {
        if (xQueueReceive(sensor_queue, &data, portMAX_DELAY) == pdPASS) {
            if (strcmp(data.sensor_name, "BME280") == 0) {
                printf("AGGREGATOR: [%s] T=%.2fC P=%.2fPa H=%.2f%%\n",
                       data.sensor_name, data.value1, data.value2, data.value3);
            } else if (strcmp(data.sensor_name, "BH1750") == 0) {
                printf("AGGREGATOR: [%s] Lux=%.1f\n", data.sensor_name, data.value1);
            } else if (strcmp(data.sensor_name, "MPU6050") == 0) {
                printf("AGGREGATOR: [%s] Roll=%.1f Pitch=%.1f GForce=%.2f\n",
                       data.sensor_name, data.value1, data.value2, data.value3);
            }
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

    // Init BME280
    if (!init_bme280()) {
        ESP_LOGE(TAG, "BME280 tidak ditemukan");
    }

    // Create queue
    sensor_queue = xQueueCreate(QUEUE_LENGTH, sizeof(sensor_data_t));
    if (sensor_queue == NULL) {
        ESP_LOGE(TAG, "Gagal membuat queue");
        return;
    }

    // Create tasks dengan xTaskCreate
    xTaskCreate(bme280_task, "bme280_task", TASK_STACK, NULL, TASK_PRIO_BME, NULL);
    xTaskCreate(bh1750_task, "bh1750_task", TASK_STACK, NULL, TASK_PRIO_BH, NULL);
    xTaskCreate(mpu6050_task, "mpu6050_task", TASK_STACK, NULL, TASK_PRIO_MPU, NULL);
    xTaskCreate(aggregator_task, "aggregator_task", TASK_STACK, NULL, TASK_PRIO_AGG, NULL);

    ESP_LOGI(TAG, "Semua task RTOS berjalan");
}
