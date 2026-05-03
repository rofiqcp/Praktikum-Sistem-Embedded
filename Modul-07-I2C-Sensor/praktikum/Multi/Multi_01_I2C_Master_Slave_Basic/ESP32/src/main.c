/**
 * Multi_01_I2C_Master_Slave_Basic - Sisi ESP32
 * 
 * Deskripsi:
 *   ESP32 berperan sebagai master I2C yang mem-poll register virtual STM32 slave.
 *   Data ditampilkan melalui serial monitor untuk edukasi komunikasi I2C.
 * 
 * Hardware:
 *   - ESP32 DevKit / LoLin S2 Mini / ESP32-S3 DevKitC
 *   - STM32 sebagai slave I2C
 * 
 * Koneksi Pin:
 *   ESP32 GPIO21 (SDA) <-> STM32 PB7
 *   ESP32 GPIO22 (SCL) <-> STM32 PB6
 *   GND ESP32 <-> GND STM32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke ESP32
 *   2. Pastikan STM32 sudah diupload dengan kode slave
 *   3. Buka serial monitor 115200 baud
 *   4. Observer data yang dipoll dari STM32
 * 
 * Variabel yang bisa dicoba:
 *   - SDA_PIN: Pin SDA (default 21)
 *   - SCL_PIN: Pin SCL (default 22)
 *   - I2C_HZ: Kecepatan I2C (default 100000)
 *   - STM32_ADDR: Alamat I2C slave STM32 (default 0x42)
 *   - REG_STATUS, REG_COUNTER, REG_TEMP_C: Register yang dibaca
 *   - REG_LED_CMD: Register perintah LED
 */
#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_HZ 100000
#define STM32_ADDR 0x42
#define REG_STATUS 0x00
#define REG_COUNTER 0x01
#define REG_TEMP_C 0x02
#define REG_LED_CMD 0x10

static const char *TAG = "M01_ESP32_MASTER";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t stm32;

static void i2c_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = STM32_ADDR,
        .scl_speed_hz = I2C_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &stm32));
}

static esp_err_t reg_read(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(stm32, &reg, 1, data, len, pdMS_TO_TICKS(200));
}

static esp_err_t reg_write(uint8_t reg, uint8_t value) {
    uint8_t frame[2] = {reg, value};
    return i2c_master_transmit(stm32, frame, sizeof(frame), pdMS_TO_TICKS(200));
}

void app_main(void) {
    i2c_init();
    uint8_t led = 0;
    while (1) {
        uint8_t buf[3] = {0};
        if (reg_read(REG_STATUS, buf, sizeof(buf)) == ESP_OK) {
            ESP_LOGI(TAG, "status=%u counter=%u tempC=%u", buf[0], buf[1], buf[2]);
        } else {
            ESP_LOGW(TAG, "STM32 slave tidak ACK; cek GND/pull-up/alamat 0x42");
        }
        led ^= 1;
        reg_write(REG_LED_CMD, led);
        uint64_t start_time = esp_timer_get_time();
        while (esp_timer_get_time() - start_time < 1000000);
    }
}
