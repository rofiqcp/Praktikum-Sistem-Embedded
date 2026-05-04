/**
 * ==========================================================
 *  Modul 07 - ESP32_03_BME280_Environmental_Sensor
 * ==========================================================
 *  Deskripsi:
 *    Membaca sensor lingkungan BME280 (suhu, tekanan, kelembaban).
 *    Mendukung alamat I2C 0x76 atau 0x77. Kalibrasi sensor dibaca
 *    dari register, data mentah dikonversi ke nilai fisik.
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
 *    I2C_FREQ_HZ 100000
 *    BME280_ADDR1 0x76
 *    BME280_ADDR2 0x77
 *    READ_INTERVAL_MS 2000
 * ==========================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// Konfigurasi pin I2C
#if defined(CONFIG_IDF_TARGET_ESP32)
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9
#else
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#endif
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000

// Alamat BME280
#define BME280_ADDR1 0x76
#define BME280_ADDR2 0x77

// Register BME280
#define BME280_REG_CHIP_ID 0xD0
#define BME280_REG_CALIB00 0x88
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_PRESS_MSB 0xF7

// Interval pembacaan
#define READ_INTERVAL_MS 2000

static const char *TAG = "BME280";

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev;
static uint8_t bme_addr;

// Kalibrasi
static uint16_t dig_T1, dig_P1;
static int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t dig_H1, dig_H3;
static int16_t dig_H2, dig_H4, dig_H5, dig_H6;

static esp_err_t bme_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, -1);
}

static esp_err_t bme_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev, buf, 2, -1);
}

static uint16_t read_u16(uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t read_s16(uint8_t *data) {
    return (int16_t)read_u16(data);
}

static void read_calibration(void) {
    uint8_t cal1[26];
    bme_read_reg(BME280_REG_CALIB00, cal1, 26);
    dig_T1 = read_u16(&cal1[0]);
    dig_T2 = read_s16(&cal1[2]);
    dig_T3 = read_s16(&cal1[4]);
    dig_P1 = read_u16(&cal1[6]);
    dig_P2 = read_s16(&cal1[8]);
    dig_P3 = read_s16(&cal1[10]);
    dig_P4 = read_s16(&cal1[12]);
    dig_P5 = read_s16(&cal1[14]);
    dig_P6 = read_s16(&cal1[16]);
    dig_P7 = read_s16(&cal1[18]);
    dig_P8 = read_s16(&cal1[20]);
    dig_P9 = read_s16(&cal1[22]);
    dig_H1 = cal1[25];
    uint8_t cal2[7];
    bme_read_reg(0xE1, cal2, 7);
    dig_H2 = read_s16(&cal2[0]);
    dig_H3 = cal2[2];
    dig_H4 = (int16_t)((cal2[3] << 4) | (cal2[4] & 0x0F));
    dig_H5 = (int16_t)((cal2[5] << 4) | (cal2[4] >> 4));
    dig_H6 = (int8_t)cal2[6];
}

static bool init_bme280(void) {
    uint8_t chip_id;
    // Coba alamat 0x76 dulu
    i2c_device_config_t dev_cfg = {
        .device_address = BME280_ADDR1,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &dev) == ESP_OK) {
        if (bme_read_reg(BME280_REG_CHIP_ID, &chip_id, 1) == ESP_OK && chip_id == 0x60) {
            bme_addr = BME280_ADDR1;
            ESP_LOGI(TAG, "BME280 ditemukan di 0x%02X", bme_addr);
            return true;
        }
        i2c_master_bus_rm_device(dev);
    }
    // Coba alamat 0x77
    dev_cfg.device_address = BME280_ADDR2;
    if (i2c_master_bus_add_device(bus, &dev_cfg, &dev) == ESP_OK) {
        if (bme_read_reg(BME280_REG_CHIP_ID, &chip_id, 1) == ESP_OK && chip_id == 0x60) {
            bme_addr = BME280_ADDR2;
            ESP_LOGI(TAG, "BME280 ditemukan di 0x%02X", bme_addr);
            return true;
        }
        i2c_master_bus_rm_device(dev);
    }
    return false;
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

    if (!init_bme280()) {
        ESP_LOGE(TAG, "BME280 tidak ditemukan");
        return;
    }

    // Konfigurasi sensor
    bme_write_reg(BME280_REG_CTRL_HUM, 0x01);   // Humidity oversampling x1
    bme_write_reg(BME280_REG_CTRL_MEAS, 0x27);  // Temp & Press oversampling x1, normal mode
    bme_write_reg(BME280_REG_CONFIG, 0xA0);     // Standby 1000ms, filter off

    read_calibration();
    ESP_LOGI(TAG, "BME280 siap, membaca setiap %d ms", READ_INTERVAL_MS);

    while (1) {
        uint8_t raw[8];
        bme_read_reg(BME280_REG_PRESS_MSB, raw, 8);

        int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
        int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
        int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

        // Kompensasi suhu
        int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * dig_T2) >> 11;
        int32_t var2 = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3) >> 14;
        int32_t t_fine = var1 + var2;
        float temp = (t_fine * 5 + 128) >> 8;
        temp /= 100.0f;

        // Kompensasi tekanan (gunakan int64_t untuk menghindari undefined behavior)
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

        printf("BME280: T=%.2fC P=%.2fPa H=%.2f%%\n", temp, pressure, humidity);
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
