/**
 * ==========================================================
 *  Modul 07 - ESP32_06_DS3231_RTC_Alarm_Temperature
 * ==========================================================
 *  Deskripsi:
 *    Menampilkan waktu RTC DS3231 dan suhu dari sensor internal.
 *    Mengatur alarm1 setiap menit pada detik ke-10. Status alarm
 *    dibaca dan di-clear otomatis. Suhu dibaca dari register 0x11.
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
 *    DS3231_ADDR 0x68
 *    READ_INTERVAL_MS 1000
 *    ALARM_SECOND 0x10
 * ==========================================================
 */

#include <stdio.h>
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

// Alamat DS3231
#define DS3231_ADDR 0x68

// Register DS3231
#define DS3231_REG_TIME 0x00
#define DS3231_REG_ALARM1 0x07
#define DS3231_REG_CONTROL 0x0E
#define DS3231_REG_STATUS 0x0F
#define DS3231_REG_TEMP_MSB 0x11

// Interval pembacaan
#define READ_INTERVAL_MS 1000

// Alarm detik ke-10 (format BCD: 0x10 = 10 detik)
#define ALARM_SECOND 0x10

static const char *TAG = "DS3231";

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t rtc;

// Konversi decimal ke BCD
static uint8_t dec2bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// Konversi BCD ke decimal
static uint8_t bcd2dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Baca register
static esp_err_t ds_read(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(rtc, &reg, 1, data, len, -1);
}

// Tulis register
static esp_err_t ds_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(rtc, buf, 2, -1);
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

    // Add DS3231 device
    i2c_device_config_t dev_cfg = {
        .device_address = DS3231_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &rtc));

    // Set waktu awal: Sen 01/01/2024 00:00:00
    uint8_t set_time[8] = {
        0x00,           // Register address 0x00
        dec2bcd(0),     // Detik: 00
        dec2bcd(0),     // Menit: 00
        dec2bcd(0),     // Jam: 00 (24h mode)
        dec2bcd(1),     // Hari: Senin
        dec2bcd(1),     // Tanggal: 1
        dec2bcd(1),     // Bulan: Januari
        dec2bcd(24)     // Tahun: 2024
    };
    ESP_ERROR_CHECK(i2c_master_transmit(rtc, set_time, 8, -1));

    // Konfigurasi alarm1 setiap menit pada detik ke-10
    uint8_t alarm1[5] = {
        0x07,           // Register address 0x07
        ALARM_SECOND,   // Detik: 10 (dengan A1M1=0)
        0x80,           // Menit: A1M2=1 (match)
        0x80,           // Jam: A1M3=1 (match)
        0x80            // Hari: A1M4=1 (match)
    };
    ESP_ERROR_CHECK(i2c_master_transmit(rtc, alarm1, 5, -1));

    // Enable alarm1 interrupt, set A1IE=1
    ds_write(DS3231_REG_CONTROL, 0x05);  // A1IE=1, INTCN=1

    ESP_LOGI(TAG, "DS3231 siap, alarm di detik ke-10 setiap menit");

    while (1) {
        uint8_t time_data[7];
        ds_read(DS3231_REG_TIME, time_data, 7);

        uint8_t sec = bcd2dec(time_data[0] & 0x7F);
        uint8_t min = bcd2dec(time_data[1] & 0x7F);
        uint8_t hour = bcd2dec(time_data[2] & 0x3F);
        uint8_t date = bcd2dec(time_data[4] & 0x3F);
        uint8_t month = bcd2dec(time_data[5] & 0x1F);
        uint8_t year = bcd2dec(time_data[6]);

        // Baca suhu
        uint8_t temp[2];
        ds_read(DS3231_REG_TEMP_MSB, temp, 2);
        float temperature = temp[0] + ((temp[1] >> 6) * 0.25f);

        // Cek status alarm
        uint8_t status;
        ds_read(DS3231_REG_STATUS, &status, 1);
        bool alarm_active = (status & 0x01);

        printf("RTC: %02u/%02u/20%02u %02u:%02u:%02u | Temp: %.2fC | Alarm: %s\n",
               date, month, year, hour, min, sec, temperature,
               alarm_active ? "AKTIF" : "-");

        // Clear alarm flag jika aktif
        if (alarm_active) {
            ds_write(DS3231_REG_STATUS, status & ~0x01);
            ESP_LOGI(TAG, "Alarm1 cleared");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
