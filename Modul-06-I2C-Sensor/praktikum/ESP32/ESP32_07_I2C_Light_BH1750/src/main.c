/*
 * ESP32_07_I2C_Light_BH1750
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Membaca sensor cahaya BH1750 via I2C.
 *            Mode continuous high-resolution dan one-time mode.
 *            Klasifikasi tingkat cahaya berdasarkan nilai lux.
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 *   BH1750:  ADDR=GND (0x23), VCC=3.3V
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "BH1750";

/* ======================== Konfigurasi Pin I2C ======================== */
#if CONFIG_IDF_TARGET_ESP32
#define I2C_SDA_PIN         21      /* Pin SDA untuk ESP32 */
#define I2C_SCL_PIN         22      /* Pin SCL untuk ESP32 */
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define I2C_SDA_PIN         8       /* Pin SDA untuk ESP32-S2/S3 */
#define I2C_SCL_PIN         9       /* Pin SCL untuk ESP32-S2/S3 */
#else
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#endif

#define I2C_PORT            I2C_NUM_0
#define I2C_FREQ_HZ         100000  /* Kecepatan I2C 100kHz (standard mode) */
#define I2C_TIMEOUT_MS      1000    /* Timeout operasi I2C dalam milidetik */

/* ======================== Alamat & Perintah BH1750 ======================== */
#define BH1750_ADDR         0x23    /* Alamat I2C BH1750 (ADDR pin = GND) */
#define BH1750_POWER_ON     0x01    /* Perintah power on */
#define BH1750_POWER_OFF    0x00    /* Perintah power off */
#define BH1750_RESET        0x07    /* Reset data register */
#define BH1750_CONT_HRES    0x10    /* Mode continuous high-resolution (1 lx) */
#define BH1750_CONT_HRES2   0x11    /* Mode continuous high-resolution 2 (0.5 lx) */
#define BH1750_CONT_LRES    0x13    /* Mode continuous low-resolution (4 lx) */
#define BH1750_ONETIME_HRES 0x20    /* Mode one-time high-resolution */
#define BH1750_ONETIME_HRES2 0x21   /* Mode one-time high-resolution 2 */
#define BH1750_ONETIME_LRES 0x23    /* Mode one-time low-resolution */

/* ======================== Inisialisasi I2C Master ======================== */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,            /* Mode master */
        .sda_io_num = I2C_SDA_PIN,          /* Pin SDA */
        .scl_io_num = I2C_SCL_PIN,          /* Pin SCL */
        .sda_pullup_en = GPIO_PULLUP_ENABLE,/* Pull-up internal SDA */
        .scl_pullup_en = GPIO_PULLUP_ENABLE,/* Pull-up internal SCL */
        .master.clk_speed = I2C_FREQ_HZ,   /* Kecepatan clock */
    };

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal konfigurasi I2C: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal install driver I2C: %s", esp_err_to_name(err));
    }
    return err;
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
    i2c_master_start(cmd);  /* Repeated START */
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
/* BH1750 menggunakan format perintah tanpa register address */
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
/* BH1750 mengirim 2 byte data tanpa register address */
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

/* ======================== Inisialisasi BH1750 ======================== */
static esp_err_t bh1750_init(void)
{
    esp_err_t err;

    /* Nyalakan sensor */
    err = bh1750_send_cmd(BH1750_POWER_ON);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal power on BH1750: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "BH1750 power on berhasil");

    /* Reset data register */
    err = bh1750_send_cmd(BH1750_RESET);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal reset BH1750: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "BH1750 reset berhasil");

    /* Set mode continuous high-resolution */
    err = bh1750_send_cmd(BH1750_CONT_HRES);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal set mode BH1750: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "BH1750 mode continuous high-resolution aktif");

    /* Tunggu pengukuran pertama selesai (max 180ms untuk H-res mode) */
    vTaskDelay(pdMS_TO_TICKS(200));

    return ESP_OK;
}

/* ======================== Baca Nilai Lux ======================== */
static esp_err_t bh1750_read_lux(float *lux)
{
    uint8_t data[2] = {0};

    /* Baca 2 byte data dari sensor */
    esp_err_t err = bh1750_read_data(data, 2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal baca data BH1750: %s", esp_err_to_name(err));
        return err;
    }

    /* Konversi raw data ke lux: lux = raw_value / 1.2 */
    uint16_t raw = (data[0] << 8) | data[1];
    *lux = (float)raw / 1.2f;

    return ESP_OK;
}

/* ======================== Baca Lux One-Time Mode ======================== */
static esp_err_t bh1750_read_lux_onetime(float *lux)
{
    esp_err_t err;

    /* Kirim perintah one-time high-resolution mode */
    err = bh1750_send_cmd(BH1750_ONETIME_HRES);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal kirim perintah one-time: %s", esp_err_to_name(err));
        return err;
    }

    /* Tunggu pengukuran selesai (max 180ms) */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Baca hasil pengukuran */
    uint8_t data[2] = {0};
    err = bh1750_read_data(data, 2);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal baca data one-time: %s", esp_err_to_name(err));
        return err;
    }

    uint16_t raw = (data[0] << 8) | data[1];
    *lux = (float)raw / 1.2f;

    return ESP_OK;
}

/* ======================== Klasifikasi Tingkat Cahaya ======================== */
static const char* klasifikasi_cahaya(float lux)
{
    if (lux < 10.0f) {
        return "Gelap (Dark)";
    } else if (lux < 100.0f) {
        return "Redup (Dim)";
    } else if (lux < 500.0f) {
        return "Normal";
    } else if (lux < 10000.0f) {
        return "Terang (Bright)";
    } else {
        return "Sangat Terang (Very Bright)";
    }
}

/* ======================== Task Pembacaan Sensor ======================== */
static void bh1750_task(void *pvParameters)
{
    float lux_cont = 0.0f;
    float lux_onetime = 0.0f;
    uint32_t sample = 0;

    while (1) {
        sample++;

        /* Baca mode continuous */
        esp_err_t err = bh1750_read_lux(&lux_cont);
        if (err == ESP_OK) {
            printf("DATA,CONT,%lu,%.2f,%s\n",
                   (unsigned long)sample, lux_cont, klasifikasi_cahaya(lux_cont));
            ESP_LOGI(TAG, "[Continuous] Lux: %.2f - %s",
                     lux_cont, klasifikasi_cahaya(lux_cont));
        } else {
            ESP_LOGE(TAG, "Gagal baca continuous mode");
        }

        /* Baca mode one-time setiap 5 sampel */
        if (sample % 5 == 0) {
            err = bh1750_read_lux_onetime(&lux_onetime);
            if (err == ESP_OK) {
                printf("DATA,ONETIME,%lu,%.2f,%s\n",
                       (unsigned long)sample, lux_onetime, klasifikasi_cahaya(lux_onetime));
                ESP_LOGI(TAG, "[One-Time]   Lux: %.2f - %s",
                         lux_onetime, klasifikasi_cahaya(lux_onetime));
            } else {
                ESP_LOGE(TAG, "Gagal baca one-time mode");
            }

            /* Setelah one-time, aktifkan kembali continuous mode */
            bh1750_send_cmd(BH1750_POWER_ON);
            bh1750_send_cmd(BH1750_CONT_HRES);
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        /* Tunggu 1 detik sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ======================== Fungsi Utama ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 I2C Light Sensor BH1750 ===");
    ESP_LOGI(TAG, "SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
    ESP_LOGI(TAG, "Alamat BH1750: 0x%02X", BH1750_ADDR);

    /* Inisialisasi I2C master */
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C master berhasil diinisialisasi");

    /* Inisialisasi sensor BH1750 */
    esp_err_t err = bh1750_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi BH1750! Periksa koneksi.");
        return;
    }

    printf("HDR,mode,sample,lux,klasifikasi\n");

    /* Buat task pembacaan sensor */
    xTaskCreate(bh1750_task, "bh1750_task", 4096, NULL, 5, NULL);
}
