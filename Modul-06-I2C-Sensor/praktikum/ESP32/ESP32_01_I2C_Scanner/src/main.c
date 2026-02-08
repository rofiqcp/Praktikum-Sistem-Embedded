/**
 * ESP32_01_I2C_Scanner
 * Modul 06 - I2C & Sensor
 *
 * Memindai bus I2C dari alamat 0x01 hingga 0x7F,
 * mendeteksi perangkat yang terhubung dan menampilkan
 * tabel dengan nama-nama perangkat umum.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "I2C_SCANNER";

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

/* ---- Nama perangkat I2C yang umum ---- */
static const char* get_device_name(uint8_t addr) {
    switch (addr) {
        case 0x20: return "PCF8574 / MCP23017";
        case 0x21: return "PCF8574 (A0=1)";
        case 0x22: return "PCF8574 (A1=1)";
        case 0x23: return "PCF8574 / BH1750";
        case 0x27: return "PCF8574 (LCD I2C)";
        case 0x3C: return "SSD1306 OLED";
        case 0x3D: return "SSD1306 OLED (alt)";
        case 0x40: return "INA219 / HDC1080 / PCA9685";
        case 0x48: return "ADS1115 / PCF8591 / TMP102";
        case 0x49: return "ADS1115 (A0=1)";
        case 0x50: return "AT24C32 EEPROM";
        case 0x51: return "AT24C32 EEPROM (A0=1)";
        case 0x52: return "AT24C32 EEPROM (A1=1)";
        case 0x57: return "AT24C32 (DS3231 board)";
        case 0x5A: return "MLX90614";
        case 0x5B: return "CCS811";
        case 0x60: return "SI1145 / MCP4725";
        case 0x68: return "MPU6050 / DS3231 RTC";
        case 0x69: return "MPU6050 (AD0=1)";
        case 0x76: return "BMP280 / BME280";
        case 0x77: return "BMP280 / BME280 (alt)";
        default:   return "Tidak dikenal";
    }
}

/* ---- Probe satu alamat I2C ---- */
static esp_err_t i2c_probe(uint8_t addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* ---- Scan seluruh bus I2C ---- */
static void i2c_scan(void) {
    uint8_t found[128];
    int count = 0;

    printf("\n======================================\n");
    printf("       I2C BUS SCANNER\n");
    printf("======================================\n");
    printf("SDA=GPIO%d  SCL=GPIO%d  Freq=%dHz\n", I2C_SDA, I2C_SCL, I2C_FREQ);
    printf("--------------------------------------\n");

    /* Tabel header */
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    for (int row = 0; row < 8; row++) {
        printf("%02X: ", row * 16);
        for (int col = 0; col < 16; col++) {
            uint8_t addr = (uint8_t)(row * 16 + col);
            if (addr < 0x01 || addr > 0x7F) {
                printf("   ");
                continue;
            }
            esp_err_t ret = i2c_probe(addr);
            if (ret == ESP_OK) {
                printf("%02X ", addr);
                found[count++] = addr;
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }

    printf("--------------------------------------\n");
    printf("Ditemukan %d perangkat.\n\n", count);

    if (count > 0) {
        printf("Daftar perangkat:\n");
        printf("  Alamat  | Nama\n");
        printf("  --------|---------------------------\n");
        for (int i = 0; i < count; i++) {
            printf("  0x%02X    | %s\n", found[i], get_device_name(found[i]));
            /* Format output untuk parsing Python */
            printf("DEVICE_FOUND: addr=0x%02X name=%s\n", found[i], get_device_name(found[i]));
        }
    }
    printf("SCAN_COMPLETE: total=%d\n\n", count);
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    /* Suppress unused warnings */
    (void)i2c_write_reg;
    (void)i2c_read_reg;

    while (1) {
        i2c_scan();
        ESP_LOGI(TAG, "Scan berikutnya dalam 5 detik...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
