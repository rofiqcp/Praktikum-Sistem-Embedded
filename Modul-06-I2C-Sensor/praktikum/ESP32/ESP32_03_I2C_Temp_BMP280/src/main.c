/**
 * ESP32_03_I2C_Temp_BMP280
 * Modul 06 - I2C & Sensor
 *
 * Membaca suhu dan tekanan dari sensor BMP280 melalui I2C.
 * Menggunakan formula kompensasi resmi dari datasheet Bosch.
 *
 * Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "BMP280";

/* ---- Pin & Konfigurasi I2C ---- */
#if CONFIG_IDF_TARGET_ESP32
#define I2C_SDA 21
#define I2C_SCL 22
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define I2C_SDA 8
#define I2C_SCL 9
#else
#define I2C_SDA 21
#define I2C_SCL 22
#endif
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ 100000

#define BMP280_ADDR   0x76

/* Register BMP280 */
#define BMP280_REG_ID       0xD0
#define BMP280_REG_RESET    0xE0
#define BMP280_REG_STATUS   0xF3
#define BMP280_REG_CTRL     0xF4
#define BMP280_REG_CONFIG   0xF5
#define BMP280_REG_PRESS    0xF7
#define BMP280_REG_TEMP     0xFA
#define BMP280_REG_CALIB    0x88

#define BMP280_CHIP_ID      0x58

/* ---- Data kalibrasi BMP280 ---- */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;

static bmp280_calib_t calib;
static int32_t t_fine; /* Variabel global untuk kompensasi silang */

/* ---- Handle I2C bus dan device ---- */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t bmp280_dev_handle;

/* ---- Inisialisasi I2C Master ---- */
static void i2c_master_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    /* Tambahkan BMP280 sebagai device pada bus I2C */
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMP280_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &bmp280_dev_handle));
}

/* ---- Helper: tulis register ---- */
static esp_err_t i2c_write_reg(uint8_t reg, uint8_t val) {
    uint8_t write_buf[2] = {reg, val};
    return i2c_master_transmit(bmp280_dev_handle, write_buf, 2, -1);
}

/* ---- Helper: baca register ---- */
static esp_err_t i2c_read_reg(uint8_t reg, uint8_t *buf, size_t len) {
    return i2c_master_transmit_receive(bmp280_dev_handle, &reg, 1, buf, len, -1);
}

/* ---- Baca data kalibrasi ---- */
static esp_err_t bmp280_read_calibration(void) {
    uint8_t buf[26];
    esp_err_t ret = i2c_read_reg(BMP280_REG_CALIB, buf, 26);
    if (ret != ESP_OK) return ret;

    calib.dig_T1 = (uint16_t)(buf[1] << 8 | buf[0]);
    calib.dig_T2 = (int16_t)(buf[3] << 8 | buf[2]);
    calib.dig_T3 = (int16_t)(buf[5] << 8 | buf[4]);
    calib.dig_P1 = (uint16_t)(buf[7] << 8 | buf[6]);
    calib.dig_P2 = (int16_t)(buf[9] << 8 | buf[8]);
    calib.dig_P3 = (int16_t)(buf[11] << 8 | buf[10]);
    calib.dig_P4 = (int16_t)(buf[13] << 8 | buf[12]);
    calib.dig_P5 = (int16_t)(buf[15] << 8 | buf[14]);
    calib.dig_P6 = (int16_t)(buf[17] << 8 | buf[16]);
    calib.dig_P7 = (int16_t)(buf[19] << 8 | buf[18]);
    calib.dig_P8 = (int16_t)(buf[21] << 8 | buf[20]);
    calib.dig_P9 = (int16_t)(buf[23] << 8 | buf[22]);

    ESP_LOGI(TAG, "Kalibrasi: T1=%u T2=%d T3=%d",
             calib.dig_T1, calib.dig_T2, calib.dig_T3);
    ESP_LOGI(TAG, "Kalibrasi: P1=%u P2=%d P3=%d",
             calib.dig_P1, calib.dig_P2, calib.dig_P3);

    return ESP_OK;
}

/* ---- Kompensasi suhu (dari datasheet BMP280) ---- */
static float bmp280_compensate_temp(int32_t adc_T) {
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
            ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

/* ---- Kompensasi tekanan (dari datasheet BMP280) ---- */
static float bmp280_compensate_press(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) +
           ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0) return 0.0f; /* Hindari pembagian nol */
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (float)p / 256.0f;
}

/* ---- Inisialisasi BMP280 ---- */
static esp_err_t bmp280_init(void) {
    uint8_t chip_id = 0;
    esp_err_t ret = i2c_read_reg(BMP280_REG_ID, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membaca chip ID");
        return ret;
    }
    ESP_LOGI(TAG, "Chip ID: 0x%02X (diharapkan: 0x%02X)", chip_id, BMP280_CHIP_ID);

    if (chip_id != BMP280_CHIP_ID) {
        ESP_LOGW(TAG, "Chip ID tidak cocok! Mungkin BME280 (0x60) atau sensor lain.");
    }

    /* Soft reset */
    i2c_write_reg(BMP280_REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Baca kalibrasi */
    ret = bmp280_read_calibration();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membaca data kalibrasi");
        return ret;
    }

    /* Config: standby 0.5ms, filter coeff 16 */
    i2c_write_reg(BMP280_REG_CONFIG, 0x00);

    /* Ctrl_meas: osrs_t=x1, osrs_p=x1, mode=normal */
    i2c_write_reg(BMP280_REG_CTRL, 0x27);

    ESP_LOGI(TAG, "BMP280 diinisialisasi dalam mode normal");
    return ESP_OK;
}

/* ---- Baca suhu dan tekanan ---- */
static esp_err_t bmp280_read(float *temp, float *press) {
    uint8_t data[6];
    esp_err_t ret = i2c_read_reg(BMP280_REG_PRESS, data, 6);
    if (ret != ESP_OK) return ret;

    /* Data tekanan: 20-bit unsigned (MSB, LSB, XLSB) */
    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((data[2] >> 4) & 0x0F);

    /* Data suhu: 20-bit unsigned (MSB, LSB, XLSB) */
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((data[5] >> 4) & 0x0F);

    /* Kompensasi - suhu harus dihitung dulu untuk t_fine */
    *temp = bmp280_compensate_temp(adc_T);
    *press = bmp280_compensate_press(adc_P) / 100.0f; /* Pa -> hPa */

    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    ESP_LOGI(TAG, "Inisialisasi BMP280...");
    esp_err_t ret = bmp280_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menginisialisasi BMP280!");
    }

    float temp, press;
    uint32_t sample = 0;

    while (1) {
        ret = bmp280_read(&temp, &press);
        if (ret == ESP_OK) {
            printf("BMP280_DATA: sample=%lu temp=%.2f press=%.2f\n",
                   (unsigned long)sample, temp, press);
            ESP_LOGI(TAG, "Suhu: %.2f °C | Tekanan: %.2f hPa", temp, press);
        } else {
            ESP_LOGW(TAG, "Gagal membaca sensor (err=%d)", ret);
            printf("BMP280_DATA: sample=%lu temp=NaN press=NaN\n",
                   (unsigned long)sample);
        }
        sample++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
