/**
 * ESP32_06_I2C_RTC_DS3231
 * Modul 06 - I2C & Sensor
 *
 * Membaca dan menulis waktu ke RTC DS3231 melalui I2C.
 * Fitur: set waktu, baca waktu, baca suhu, set Alarm 1.
 * Format BCD encode/decode.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "DS3231";

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

/* Alamat dan register DS3231 */
#define DS3231_ADDR       0x68

/* Register waktu */
#define DS3231_REG_SEC    0x00
#define DS3231_REG_MIN    0x01
#define DS3231_REG_HOUR   0x02
#define DS3231_REG_DOW    0x03  /* Day of week (1-7) */
#define DS3231_REG_DATE   0x04
#define DS3231_REG_MONTH  0x05
#define DS3231_REG_YEAR   0x06

/* Register alarm 1 */
#define DS3231_REG_A1SEC  0x07
#define DS3231_REG_A1MIN  0x08
#define DS3231_REG_A1HOUR 0x09
#define DS3231_REG_A1DAY  0x0A

/* Register kontrol & status */
#define DS3231_REG_CTRL   0x0E
#define DS3231_REG_STATUS 0x0F

/* Register suhu */
#define DS3231_REG_TEMP   0x11

/* ---- Struktur data waktu ---- */
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;  /* 1=Senin .. 7=Minggu */
    uint8_t date;
    uint8_t month;
    uint8_t year;         /* 0-99 (2000-2099) */
} ds3231_time_t;

/* Nama hari dalam bahasa Indonesia */
static const char *HARI[] = {
    "", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu", "Minggu"
};

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

/* ---- BCD encode/decode ---- */
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

/* ---- Set waktu pada DS3231 ---- */
static esp_err_t ds3231_set_time(const ds3231_time_t *time) {
    uint8_t data[7];
    data[0] = dec_to_bcd(time->seconds);
    data[1] = dec_to_bcd(time->minutes);
    data[2] = dec_to_bcd(time->hours);    /* Format 24 jam */
    data[3] = dec_to_bcd(time->day_of_week);
    data[4] = dec_to_bcd(time->date);
    data[5] = dec_to_bcd(time->month);
    data[6] = dec_to_bcd(time->year);

    /* Tulis 7 byte mulai dari register 0x00 */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, DS3231_REG_SEC, true);
    i2c_master_write(cmd, data, 7, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Waktu diset: %04d-%02d-%02d %02d:%02d:%02d",
                 2000 + time->year, time->month, time->date,
                 time->hours, time->minutes, time->seconds);
    }
    return ret;
}

/* ---- Baca waktu dari DS3231 ---- */
static esp_err_t ds3231_get_time(ds3231_time_t *time) {
    uint8_t data[7];
    esp_err_t ret = i2c_read_reg(DS3231_ADDR, DS3231_REG_SEC, data, 7);
    if (ret != ESP_OK) return ret;

    time->seconds     = bcd_to_dec(data[0] & 0x7F);
    time->minutes     = bcd_to_dec(data[1] & 0x7F);
    time->hours       = bcd_to_dec(data[2] & 0x3F); /* Mask untuk 24-jam */
    time->day_of_week = bcd_to_dec(data[3] & 0x07);
    time->date        = bcd_to_dec(data[4] & 0x3F);
    time->month       = bcd_to_dec(data[5] & 0x1F);
    time->year        = bcd_to_dec(data[6]);

    return ESP_OK;
}

/* ---- Baca suhu internal DS3231 ---- */
static esp_err_t ds3231_get_temperature(float *temp) {
    uint8_t data[2];
    esp_err_t ret = i2c_read_reg(DS3231_ADDR, DS3231_REG_TEMP, data, 2);
    if (ret != ESP_OK) return ret;

    /* MSB = integer part (signed), LSB bits 7:6 = fractional (0.25°C steps) */
    int8_t integer_part = (int8_t)data[0];
    uint8_t frac_part = (data[1] >> 6) & 0x03;
    *temp = (float)integer_part + (frac_part * 0.25f);

    return ESP_OK;
}

/* ---- Set Alarm 1 (detik cocok) ---- */
static esp_err_t ds3231_set_alarm1(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    /* A1M4=1, A1M3=0, A1M2=0, A1M1=0: Alarm when hours, minutes, seconds match */
    i2c_write_reg(DS3231_ADDR, DS3231_REG_A1SEC,  dec_to_bcd(seconds) & 0x7F);  /* A1M1=0 */
    i2c_write_reg(DS3231_ADDR, DS3231_REG_A1MIN,  dec_to_bcd(minutes) & 0x7F);  /* A1M2=0 */
    i2c_write_reg(DS3231_ADDR, DS3231_REG_A1HOUR, dec_to_bcd(hours) & 0x3F);    /* A1M3=0 */
    i2c_write_reg(DS3231_ADDR, DS3231_REG_A1DAY,  0x80);                        /* A1M4=1 */

    /* Enable Alarm 1 interrupt: set A1IE bit di control register */
    uint8_t ctrl = 0;
    i2c_read_reg(DS3231_ADDR, DS3231_REG_CTRL, &ctrl, 1);
    ctrl |= 0x05;  /* INTCN=1, A1IE=1 */
    i2c_write_reg(DS3231_ADDR, DS3231_REG_CTRL, ctrl);

    /* Clear alarm flag */
    uint8_t status = 0;
    i2c_read_reg(DS3231_ADDR, DS3231_REG_STATUS, &status, 1);
    status &= ~0x01;  /* Clear A1F */
    i2c_write_reg(DS3231_ADDR, DS3231_REG_STATUS, status);

    ESP_LOGI(TAG, "Alarm 1 diset: %02d:%02d:%02d", hours, minutes, seconds);
    return ESP_OK;
}

/* ---- Cek apakah Alarm 1 terpicu ---- */
static bool ds3231_check_alarm1(void) {
    uint8_t status = 0;
    i2c_read_reg(DS3231_ADDR, DS3231_REG_STATUS, &status, 1);
    if (status & 0x01) {
        /* Clear flag */
        status &= ~0x01;
        i2c_write_reg(DS3231_ADDR, DS3231_REG_STATUS, status);
        return true;
    }
    return false;
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    ESP_LOGI(TAG, "=== DS3231 RTC Demo ===");

    /* Set waktu awal: 2026-02-07 (Sabtu) 10:30:00 */
    ds3231_time_t set_time = {
        .seconds = 0,
        .minutes = 30,
        .hours = 10,
        .day_of_week = 6,  /* Sabtu */
        .date = 7,
        .month = 2,
        .year = 26         /* 2026 */
    };
    ds3231_set_time(&set_time);

    /* Set Alarm 1: 10:30:30 (30 detik kemudian) */
    ds3231_set_alarm1(10, 30, 30);

    ds3231_time_t now;
    float temperature;
    uint32_t sample = 0;

    while (1) {
        /* Baca waktu */
        esp_err_t ret = ds3231_get_time(&now);
        if (ret == ESP_OK) {
            const char *day_name = "";
            if (now.day_of_week >= 1 && now.day_of_week <= 7) {
                day_name = HARI[now.day_of_week];
            }

            printf("RTC_TIME: sample=%lu "
                   "date=20%02d-%02d-%02d "
                   "time=%02d:%02d:%02d "
                   "day=%s",
                   (unsigned long)sample,
                   now.year, now.month, now.date,
                   now.hours, now.minutes, now.seconds,
                   day_name);

            /* Baca suhu */
            ret = ds3231_get_temperature(&temperature);
            if (ret == ESP_OK) {
                printf(" temp=%.2f", temperature);
            }

            /* Cek alarm */
            bool alarm = ds3231_check_alarm1();
            if (alarm) {
                printf(" ALARM=1");
                ESP_LOGW(TAG, "*** ALARM 1 TERPICU! ***");
            }

            printf("\n");

            ESP_LOGI(TAG, "%s, 20%02d-%02d-%02d %02d:%02d:%02d | Suhu: %.2f°C",
                     day_name, now.year, now.month, now.date,
                     now.hours, now.minutes, now.seconds, temperature);
        } else {
            ESP_LOGW(TAG, "Gagal membaca RTC (err=%d)", ret);
            printf("RTC_TIME: sample=%lu ERROR\n", (unsigned long)sample);
        }

        sample++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
