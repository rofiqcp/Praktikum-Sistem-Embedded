/*
 * ESP32_09_I2C_Multi_Sensor
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Membaca BMP280 (suhu + tekanan) dan BH1750 (cahaya)
 *            pada bus I2C yang sama. Demonstrasi multiple device.
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 *   BMP280:  Alamat 0x76 (SDO=GND)
 *   BH1750:  Alamat 0x23 (ADDR=GND)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "MULTI_SENSOR";

/* ======================== Konfigurasi Pin I2C ======================== */
#if CONFIG_IDF_TARGET_ESP32
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define I2C_SDA_PIN         8
#define I2C_SCL_PIN         9
#else
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#endif

#define I2C_PORT            I2C_NUM_0
#define I2C_FREQ_HZ         100000
#define I2C_TIMEOUT_MS      1000

/* ======================== Alamat & Register BMP280 ======================== */
#define BMP280_ADDR         0x76    /* Alamat I2C BMP280 (SDO=GND) */
#define BMP280_REG_ID       0xD0    /* Register chip ID (harus 0x58) */
#define BMP280_REG_RESET    0xE0    /* Register soft reset */
#define BMP280_REG_STATUS   0xF3    /* Register status */
#define BMP280_REG_CTRL     0xF4    /* Register kontrol pengukuran */
#define BMP280_REG_CONFIG   0xF5    /* Register konfigurasi */
#define BMP280_REG_PRESS    0xF7    /* Register data tekanan (3 byte) */
#define BMP280_REG_TEMP     0xFA    /* Register data suhu (3 byte) */
#define BMP280_REG_CALIB    0x88    /* Awal register kalibrasi (26 byte) */

/* ======================== Alamat & Perintah BH1750 ======================== */
#define BH1750_ADDR         0x23    /* Alamat I2C BH1750 */
#define BH1750_POWER_ON     0x01    /* Power on */
#define BH1750_RESET        0x07    /* Reset */
#define BH1750_CONT_HRES    0x10    /* Continuous high-resolution mode */

/* ======================== Struktur Data Kalibrasi BMP280 ======================== */
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

static bmp280_calib_t bmp_calib;
static int32_t t_fine; /* Variabel global untuk kompensasi suhu */

/* ======================== Inisialisasi I2C Master ======================== */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
}

/* ======================== Tulis Register I2C ======================== */
static esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    if (data != NULL && len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

/* ======================== Baca Register I2C ======================== */
static esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

/* ======================== Kirim Perintah BH1750 ======================== */
static esp_err_t bh1750_send_cmd(uint8_t command)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BH1750_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

/* ======================== Baca Data BH1750 ======================== */
static esp_err_t bh1750_read_data(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BH1750_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

/* ======================== Inisialisasi BMP280 ======================== */
static esp_err_t bmp280_init(void)
{
    uint8_t chip_id = 0;
    esp_err_t err;

    /* Baca chip ID untuk verifikasi */
    err = i2c_read_reg(BMP280_ADDR, BMP280_REG_ID, &chip_id, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membaca chip ID BMP280: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "BMP280 Chip ID: 0x%02X (diharapkan 0x58)", chip_id);

    if (chip_id != 0x58) {
        ESP_LOGW(TAG, "Chip ID tidak sesuai! Mungkin BME280 (0x60) atau lainnya");
    }

    /* Baca data kalibrasi (26 byte mulai dari 0x88) */
    uint8_t calib_data[26];
    err = i2c_read_reg(BMP280_ADDR, BMP280_REG_CALIB, calib_data, 26);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal baca data kalibrasi BMP280");
        return err;
    }

    /* Parse data kalibrasi (little-endian) */
    bmp_calib.dig_T1 = (uint16_t)(calib_data[1] << 8 | calib_data[0]);
    bmp_calib.dig_T2 = (int16_t)(calib_data[3] << 8 | calib_data[2]);
    bmp_calib.dig_T3 = (int16_t)(calib_data[5] << 8 | calib_data[4]);
    bmp_calib.dig_P1 = (uint16_t)(calib_data[7] << 8 | calib_data[6]);
    bmp_calib.dig_P2 = (int16_t)(calib_data[9] << 8 | calib_data[8]);
    bmp_calib.dig_P3 = (int16_t)(calib_data[11] << 8 | calib_data[10]);
    bmp_calib.dig_P4 = (int16_t)(calib_data[13] << 8 | calib_data[12]);
    bmp_calib.dig_P5 = (int16_t)(calib_data[15] << 8 | calib_data[14]);
    bmp_calib.dig_P6 = (int16_t)(calib_data[17] << 8 | calib_data[16]);
    bmp_calib.dig_P7 = (int16_t)(calib_data[19] << 8 | calib_data[18]);
    bmp_calib.dig_P8 = (int16_t)(calib_data[21] << 8 | calib_data[20]);
    bmp_calib.dig_P9 = (int16_t)(calib_data[23] << 8 | calib_data[22]);

    ESP_LOGI(TAG, "Data kalibrasi BMP280 berhasil dibaca");
    ESP_LOGI(TAG, "  T1=%u T2=%d T3=%d", bmp_calib.dig_T1, bmp_calib.dig_T2, bmp_calib.dig_T3);

    /* Konfigurasi: standby 500ms, filter coeff 4 */
    uint8_t config_val = (0x04 << 5) | (0x02 << 2); /* t_sb=500ms, filter=x4 */
    err = i2c_write_reg(BMP280_ADDR, BMP280_REG_CONFIG, &config_val, 1);
    if (err != ESP_OK) return err;

    /* Kontrol: osrs_t=x2, osrs_p=x16, mode=normal */
    uint8_t ctrl_val = (0x02 << 5) | (0x05 << 2) | 0x03;
    err = i2c_write_reg(BMP280_ADDR, BMP280_REG_CTRL, &ctrl_val, 1);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "BMP280 berhasil diinisialisasi (mode normal)");
    return ESP_OK;
}

/* ======================== Inisialisasi BH1750 ======================== */
static esp_err_t bh1750_init(void)
{
    esp_err_t err;

    err = bh1750_send_cmd(BH1750_POWER_ON);
    if (err != ESP_OK) return err;

    err = bh1750_send_cmd(BH1750_RESET);
    if (err != ESP_OK) return err;

    err = bh1750_send_cmd(BH1750_CONT_HRES);
    if (err != ESP_OK) return err;

    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "BH1750 berhasil diinisialisasi");
    return ESP_OK;
}

/* ======================== Kompensasi Suhu BMP280 ======================== */
static float bmp280_compensate_temp(int32_t adc_T)
{
    int32_t var1, var2;

    var1 = ((((adc_T >> 3) - ((int32_t)bmp_calib.dig_T1 << 1))) *
            ((int32_t)bmp_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp_calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)bmp_calib.dig_T1))) >> 12) *
            ((int32_t)bmp_calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    float temp = (float)((t_fine * 5 + 128) >> 8) / 100.0f;
    return temp;
}

/* ======================== Kompensasi Tekanan BMP280 ======================== */
static float bmp280_compensate_press(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)bmp_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp_calib.dig_P1) >> 33;

    if (var1 == 0) {
        return 0.0f; /* Hindari pembagian nol */
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp_calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp_calib.dig_P7) << 4);
    float press = (float)p / 25600.0f; /* Dalam hPa */
    return press;
}

/* ======================== Baca BMP280 ======================== */
static esp_err_t bmp280_read(float *temp, float *press)
{
    uint8_t data[6];
    esp_err_t err;

    /* Baca 6 byte data: tekanan (3 byte) + suhu (3 byte) */
    err = i2c_read_reg(BMP280_ADDR, BMP280_REG_PRESS, data, 6);
    if (err != ESP_OK) return err;

    /* Parse raw data (20-bit) */
    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);

    /* Kompensasi - suhu harus dihitung terlebih dahulu (untuk t_fine) */
    *temp = bmp280_compensate_temp(adc_T);
    *press = bmp280_compensate_press(adc_P);

    return ESP_OK;
}

/* ======================== Baca BH1750 Lux ======================== */
static esp_err_t bh1750_read_lux(float *lux)
{
    uint8_t data[2] = {0};
    esp_err_t err = bh1750_read_data(data, 2);
    if (err != ESP_OK) return err;

    uint16_t raw = (data[0] << 8) | data[1];
    *lux = (float)raw / 1.2f;
    return ESP_OK;
}

/* ======================== Klasifikasi Cahaya ======================== */
static const char* klasifikasi_cahaya(float lux)
{
    if (lux < 10.0f) return "Gelap";
    else if (lux < 100.0f) return "Redup";
    else if (lux < 500.0f) return "Normal";
    else if (lux < 10000.0f) return "Terang";
    else return "Sangat Terang";
}

/* ======================== Task Multi Sensor ======================== */
static void multi_sensor_task(void *pvParameters)
{
    float temperature = 0.0f;
    float pressure = 0.0f;
    float lux = 0.0f;
    uint32_t sample = 0;

    while (1) {
        sample++;

        /* Baca BMP280 (suhu & tekanan) */
        esp_err_t err_bmp = bmp280_read(&temperature, &pressure);
        if (err_bmp == ESP_OK) {
            ESP_LOGI(TAG, "[BMP280] Suhu: %.2f°C, Tekanan: %.2f hPa",
                     temperature, pressure);
        } else {
            ESP_LOGE(TAG, "Gagal baca BMP280: %s", esp_err_to_name(err_bmp));
        }

        /* Baca BH1750 (cahaya) */
        esp_err_t err_bh = bh1750_read_lux(&lux);
        if (err_bh == ESP_OK) {
            ESP_LOGI(TAG, "[BH1750] Cahaya: %.2f lux - %s", lux, klasifikasi_cahaya(lux));
        } else {
            ESP_LOGE(TAG, "Gagal baca BH1750: %s", esp_err_to_name(err_bh));
        }

        /* Output gabungan untuk analisis */
        if (err_bmp == ESP_OK && err_bh == ESP_OK) {
            printf("DATA,%lu,%.2f,%.2f,%.2f,%s\n",
                   (unsigned long)sample, temperature, pressure,
                   lux, klasifikasi_cahaya(lux));
            ESP_LOGI(TAG, "--- Sampel #%lu: T=%.2f°C P=%.2f hPa L=%.2f lux ---",
                     (unsigned long)sample, temperature, pressure, lux);
        } else {
            printf("DATA,%lu,ERR,ERR,ERR,ERR\n", (unsigned long)sample);
        }

        /* Tunggu 2 detik sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ======================== Scan I2C Bus ======================== */
static void i2c_scan(void)
{
    ESP_LOGI(TAG, "Memulai scan I2C bus...");
    uint8_t device_count = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "  Perangkat ditemukan di alamat 0x%02X", addr);
            device_count++;
        }
    }
    ESP_LOGI(TAG, "Scan selesai. Total perangkat: %u", device_count);
}

/* ======================== Fungsi Utama ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 I2C Multi Sensor (BMP280 + BH1750) ===");
    ESP_LOGI(TAG, "SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);

    /* Inisialisasi I2C master */
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C master berhasil diinisialisasi");

    /* Scan bus untuk mendeteksi perangkat */
    i2c_scan();

    /* Inisialisasi BMP280 */
    esp_err_t err = bmp280_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi BMP280! Periksa koneksi.");
    } else {
        ESP_LOGI(TAG, "BMP280 siap di alamat 0x%02X", BMP280_ADDR);
    }

    /* Inisialisasi BH1750 */
    err = bh1750_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi BH1750! Periksa koneksi.");
    } else {
        ESP_LOGI(TAG, "BH1750 siap di alamat 0x%02X", BH1750_ADDR);
    }

    printf("HDR,sample,suhu_C,tekanan_hPa,cahaya_lux,klasifikasi\n");

    /* Buat task pembacaan multi-sensor */
    xTaskCreate(multi_sensor_task, "multi_sensor", 4096, NULL, 5, NULL);
}
