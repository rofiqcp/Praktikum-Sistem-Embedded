/**
 * ESP32_04_I2C_Accel_MPU6050
 * Modul 06 - I2C & Sensor
 *
 * Membaca data akselerometer dan giroskop dari MPU6050.
 * Menampilkan nilai X, Y, Z dalam g dan dps.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "MPU6050";

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

/* Alamat dan register MPU6050 */
#define MPU6050_ADDR      0x68
#define MPU6050_WHO_AM_I  0x75
#define MPU6050_PWR_MGMT1 0x6B
#define MPU6050_PWR_MGMT2 0x6C
#define MPU6050_SMPLRT    0x19
#define MPU6050_CONFIG     0x1A
#define MPU6050_GYRO_CFG  0x1B
#define MPU6050_ACCEL_CFG 0x1C
#define MPU6050_ACCEL_OUT 0x3B
#define MPU6050_TEMP_OUT  0x41
#define MPU6050_GYRO_OUT  0x43

#define MPU6050_WHO_AM_I_VAL  0x68

/* Faktor konversi */
#define ACCEL_SCALE  16384.0f  /* ±2g -> 16384 LSB/g */
#define GYRO_SCALE   131.0f    /* ±250°/s -> 131 LSB/(°/s) */

/* ---- Inisialisasi I2C Master ---- */
static void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

/* ---- Helper: tulis register ---- */
static esp_err_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* ---- Helper: baca register ---- */
static esp_err_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) i2c_master_read(cmd, buf, len - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, buf + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* ---- Inisialisasi MPU6050 ---- */
static esp_err_t mpu6050_init(void) {
    uint8_t who_am_i = 0;
    esp_err_t ret;

    /* Baca WHO_AM_I untuk verifikasi */
    ret = i2c_read_reg(MPU6050_ADDR, MPU6050_WHO_AM_I, &who_am_i, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membaca WHO_AM_I (err=%d)", ret);
        return ret;
    }
    ESP_LOGI(TAG, "WHO_AM_I: 0x%02X (diharapkan: 0x%02X)", who_am_i, MPU6050_WHO_AM_I_VAL);

    /* Wake up: tulis 0 ke PWR_MGMT_1 (clear SLEEP bit) */
    ret = i2c_write_reg(MPU6050_ADDR, MPU6050_PWR_MGMT1, 0x00);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Set sample rate divider: 1kHz / (1+9) = 100Hz */
    i2c_write_reg(MPU6050_ADDR, MPU6050_SMPLRT, 0x09);

    /* Config: DLPF = 3 (bandwidth 44Hz) */
    i2c_write_reg(MPU6050_ADDR, MPU6050_CONFIG, 0x03);

    /* Gyro config: FS_SEL=0 -> ±250°/s */
    i2c_write_reg(MPU6050_ADDR, MPU6050_GYRO_CFG, 0x00);

    /* Accel config: AFS_SEL=0 -> ±2g */
    i2c_write_reg(MPU6050_ADDR, MPU6050_ACCEL_CFG, 0x00);

    ESP_LOGI(TAG, "MPU6050 diinisialisasi (±2g, ±250°/s)");
    return ESP_OK;
}

/* ---- Baca data akselerometer & giroskop ---- */
static esp_err_t mpu6050_read(float *ax, float *ay, float *az,
                               float *gx, float *gy, float *gz,
                               float *temp) {
    uint8_t data[14];
    esp_err_t ret = i2c_read_reg(MPU6050_ADDR, MPU6050_ACCEL_OUT, data, 14);
    if (ret != ESP_OK) return ret;

    /* Akselerometer: register 0x3B-0x40 (big-endian) */
    int16_t raw_ax = (int16_t)((data[0] << 8) | data[1]);
    int16_t raw_ay = (int16_t)((data[2] << 8) | data[3]);
    int16_t raw_az = (int16_t)((data[4] << 8) | data[5]);

    /* Suhu: register 0x41-0x42 */
    int16_t raw_temp = (int16_t)((data[6] << 8) | data[7]);

    /* Giroskop: register 0x43-0x48 */
    int16_t raw_gx = (int16_t)((data[8] << 8) | data[9]);
    int16_t raw_gy = (int16_t)((data[10] << 8) | data[11]);
    int16_t raw_gz = (int16_t)((data[12] << 8) | data[13]);

    /* Konversi ke satuan fisik */
    *ax = raw_ax / ACCEL_SCALE;
    *ay = raw_ay / ACCEL_SCALE;
    *az = raw_az / ACCEL_SCALE;
    *gx = raw_gx / GYRO_SCALE;
    *gy = raw_gy / GYRO_SCALE;
    *gz = raw_gz / GYRO_SCALE;
    *temp = (raw_temp / 340.0f) + 36.53f; /* Rumus dari datasheet */

    return ESP_OK;
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    ESP_LOGI(TAG, "Inisialisasi MPU6050...");
    esp_err_t ret = mpu6050_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menginisialisasi MPU6050!");
    }

    float ax, ay, az, gx, gy, gz, temp;
    uint32_t sample = 0;

    while (1) {
        ret = mpu6050_read(&ax, &ay, &az, &gx, &gy, &gz, &temp);
        if (ret == ESP_OK) {
            printf("MPU6050_DATA: sample=%lu "
                   "ax=%.3f ay=%.3f az=%.3f "
                   "gx=%.2f gy=%.2f gz=%.2f "
                   "temp=%.1f\n",
                   (unsigned long)sample,
                   ax, ay, az, gx, gy, gz, temp);

            ESP_LOGI(TAG, "Accel: X=%.3fg Y=%.3fg Z=%.3fg", ax, ay, az);
            ESP_LOGI(TAG, "Gyro : X=%.2f°/s Y=%.2f°/s Z=%.2f°/s", gx, gy, gz);
            ESP_LOGI(TAG, "Suhu : %.1f °C", temp);
        } else {
            ESP_LOGW(TAG, "Gagal membaca sensor (err=%d)", ret);
        }
        sample++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
