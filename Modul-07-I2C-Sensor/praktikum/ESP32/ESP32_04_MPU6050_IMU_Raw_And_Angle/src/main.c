/**
 * ==========================================================
 *  Modul 07 - ESP32_04_MPU6050_IMU_Raw_And_Angle
 * ==========================================================
 *  Deskripsi:
 *    Membaca sensor IMU MPU6050 (akselerometer dan giroskop 6-axis).
 *    Menampilkan data raw register dan menghitung sudut roll/pitch
 *    menggunakan fungsi atan2. Data dibaca via I2C setiap 500ms.
 *  Hardware:
 *    ESP32 DevKit / ESP32-S2 / ESP32-S3
 *  Koneksi Pin:
 *    SDA = GPIO21 (ESP32), GPIO8 (S2/S3)
 *    SCL = GPIO22 (ESP32), GPIO9 (S2/S3)
 *  Instruksi:
 *    1. Pasang sensor sesuai alamat
 *    2. Ubah nilai #define di atas untuk mencoba variasi
 *    3. Build & upload dengan PlatformIO
 *  Variabel yang bisa dicoba (#define):
 *    I2C_SDA_GPIO GPIO_NUM_21
 *    I2C_SCL_GPIO GPIO_NUM_22
 *    I2C_PORT I2C_NUM_0
 *    I2C_FREQ_HZ 400000
 *    MPU6050_ADDR 0x68
 *    READ_INTERVAL_MS 500
 *    ACCEL_SCALE 16384.0f
 * ==========================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// Konfigurasi pin I2C
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 400000

// Alamat MPU6050
#define MPU6050_ADDR 0x68

// Register MPU6050
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

// Interval pembacaan
#define READ_INTERVAL_MS 500

// Skala akselerometer (LSB/g)
#define ACCEL_SCALE 16384.0f

static const char *TAG = "MPU6050";

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t mpu;

// Baca register 16-bit (big-endian)
static int16_t read_16bit(uint8_t reg) {
    uint8_t data[2];
    i2c_master_transmit_receive(mpu, &reg, 1, data, 2, -1);
    return (int16_t)((data[0] << 8) | data[1]);
}

// Tulis register 8-bit
static void write_8bit(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_master_transmit(mpu, buf, 2, -1);
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

    // Add MPU6050 device
    i2c_device_config_t dev_cfg = {
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &mpu));

    // Wake up MPU6050 (clear sleep bit)
    write_8bit(MPU6050_REG_PWR_MGMT_1, 0x00);
    // Config akselerometer ±2g
    write_8bit(MPU6050_REG_ACCEL_CONFIG, 0x00);
    // Config giroskop ±250°/s
    write_8bit(MPU6050_REG_GYRO_CONFIG, 0x00);

    ESP_LOGI(TAG, "MPU6050 siap, membaca setiap %d ms", READ_INTERVAL_MS);

    while (1) {
        // Baca data akselerometer (6 bytes: X, Y, Z)
        uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;
        uint8_t data[14];
        i2c_master_transmit_receive(mpu, &reg, 1, data, 14, -1);

        int16_t ax = (int16_t)((data[0] << 8) | data[1]);
        int16_t ay = (int16_t)((data[2] << 8) | data[3]);
        int16_t az = (int16_t)((data[4] << 8) | data[5]);
        int16_t gx = (int16_t)((data[8] << 8) | data[9]);
        int16_t gy = (int16_t)((data[10] << 8) | data[11]);
        int16_t gz = (int16_t)((data[12] << 8) | data[13]);

        // Konversi ke satuan g
        float ax_g = ax / ACCEL_SCALE;
        float ay_g = ay / ACCEL_SCALE;
        float az_g = az / ACCEL_SCALE;

        // Hitung sudut roll & pitch
        float roll = atan2f(ay_g, az_g) * 57.2958f;
        float pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * 57.2958f;

        printf("IMU_RAW: ax=%d ay=%d az=%d gx=%d gy=%d gz=%d | angle_roll=%.1f angle_pitch=%.1f\n",
               ax, ay, az, gx, gy, gz, roll, pitch);

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
