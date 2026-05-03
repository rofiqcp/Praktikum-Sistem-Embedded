/**
 * Multi_03_I2C_Shared_Sensor - Sisi ESP32
 * 
 * Deskripsi:
 *   ESP32 dan STM32 bergantian peran sebagai master/slave.
 *   Handshake GPIO BUS_OWNER menentukan siapa yang memegang kendali bus.
 * 
 * Hardware:
 *   - ESP32 DevKit / LoLin S2 Mini / ESP32-S3 DevKitC
 *   - STM32 sebagai peer di bus I2C
 * 
 * Koneksi Pin:
 *   ESP32 GPIO21 (SDA) <-> STM32 PB7
 *   ESP32 GPIO22 (SCL) <-> STM32 PB6
 *   ESP32 GPIO4 (BUS_OWNER) <-> STM32 PA0
 *   GND ESP32 <-> GND STM32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 *   Pull-down 10k pada BUS_OWNER
 * 
 * Instruksi:
 *   1. Upload kode ke ESP32 dan STM32
 *   2. Buka serial monitor kedua device (115200 baud)
 *   3. Amati pergantian peran master/slave setiap beberapa detik
 * 
 * Variabel yang bisa dicoba:
 *   - SDA_PIN: Pin SDA (default 21)
 *   - SCL_PIN: Pin SCL (default 22)
 *   - BUS_OWNER_PIN: GPIO handshake (default 4)
 *   - STM32_ADDR: Alamat I2C STM32 (default 0x42)
 *   - ESP32_ADDR: Alamat I2C ESP32 (default 0x32)
 */
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define BUS_OWNER_PIN 4
#define STM32_ADDR 0x42
#define ESP32_ADDR 0x32

static const char *TAG = "M03_ESP32_SWAP";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t stm32;

static void bus_owner_init(void) {
    gpio_config_t cfg = {.pin_bit_mask = 1ULL << BUS_OWNER_PIN, .mode = GPIO_MODE_OUTPUT, .pull_down_en = true};
    gpio_config(&cfg);
}

static void master_start(void) {
    i2c_master_bus_config_t bus_cfg = {.i2c_port = I2C_NUM_0, .sda_io_num = SDA_PIN, .scl_io_num = SCL_PIN, .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7, .flags.enable_internal_pullup = true};
    i2c_new_master_bus(&bus_cfg, &bus);
    i2c_device_config_t dev_cfg = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = STM32_ADDR, .scl_speed_hz = 100000};
    i2c_master_bus_add_device(bus, &dev_cfg, &stm32);
}

static void master_stop(void) {
    if (stm32) i2c_master_bus_rm_device(stm32);
    if (bus) i2c_del_master_bus(bus);
    stm32 = NULL;
    bus = NULL;
}

static void slave_scaffold_start(void) {
    ESP_LOGW(TAG, "ESP32 slave scaffold addr=0x%02X; gunakan driver/i2c_slave.h bila tersedia", ESP32_ADDR);
}

void app_main(void) {
    bus_owner_init();
    uint8_t command = 1;
    while (1) {
        gpio_set_level(BUS_OWNER_PIN, 1);
        uint64_t start_time_50 = esp_timer_get_time();
        while (esp_timer_get_time() - start_time_50 < 50000);
        master_start();
        uint8_t frame[2] = {0x10, command++};
        esp_err_t err = i2c_master_transmit(stm32, frame, sizeof(frame), 200);
        ESP_LOGI(TAG, "ESP32 master kirim command err=%s", esp_err_to_name(err));
        master_stop();
        gpio_set_level(BUS_OWNER_PIN, 0);
        slave_scaffold_start();
        uint64_t start_time_3000 = esp_timer_get_time();
        while (esp_timer_get_time() - start_time_3000 < 3000000);
    }
}
