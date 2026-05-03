/**
 * Multi_02_I2C_Role_Swap_Command - Sisi ESP32
 * 
 * Deskripsi:
 *   ESP32 berperan sebagai slave I2C edukasi yang mengekspos register virtual.
 *   STM32 master akan mem-poll data dari register ESP32.
 * 
 * Hardware:
 *   - ESP32 DevKit / LoLin S2 Mini / ESP32-S3 DevKitC
 *   - STM32 sebagai master I2C
 * 
 * Koneksi Pin:
 *   ESP32 GPIO21 (SDA) <-> STM32 PB7
 *   ESP32 GPIO22 (SCL) <-> STM32 PB6
 *   GND ESP32 <-> GND STM32
 *   Pull-up 4.7k pada SDA dan SCL ke 3V3
 * 
 * Instruksi:
 *   1. Upload kode ke ESP32
 *   2. Pastikan STM32 sudah diupload dengan kode master
 *   3. Buka serial monitor 115200 baud
 *   4. Observer saat STM32 mem-poll data dari ESP32
 * 
 * Variabel yang bisa dicoba:
 *   - ESP32_SLAVE_ADDR: Alamat I2C slave ESP32 (default 0x32)
 *   - REG_COUNT: Jumlah register virtual (default 32)
 *   - REG_STATUS, REG_COUNTER, REG_TEMP_C: Register virtual
 *   - REG_CMD: Register perintah dari master
 */
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"

#define ESP32_SLAVE_ADDR 0x32
#define REG_COUNT 32
#define REG_STATUS 0x00
#define REG_COUNTER 0x01
#define REG_TEMP_C 0x02
#define REG_CMD 0x10

static const char *TAG = "M02_ESP32_SLAVE";
static uint8_t regs[REG_COUNT];

static void i2c_slave_scaffold_init(void) {
    ESP_LOGW(TAG, "Slave scaffold: aktifkan driver/i2c_slave.h sesuai ESP-IDF v5.x target");
    ESP_LOGW(TAG, "Alamat edukasi=0x%02X, SDA=GPIO21, SCL=GPIO22", ESP32_SLAVE_ADDR);
}

static void virtual_register_update(void) {
    regs[REG_STATUS] = 0x5A;
    regs[REG_COUNTER]++;
    regs[REG_TEMP_C] = 28;
    if (regs[REG_CMD]) {
        ESP_LOGI(TAG, "command dari STM32=%u", regs[REG_CMD]);
        regs[REG_CMD] = 0;
    }
}

void app_main(void) {
    i2c_slave_scaffold_init();
    while (1) {
        virtual_register_update();
        ESP_LOGI(TAG, "regs[0..2]=%u,%u,%u", regs[0], regs[1], regs[2]);
        uint64_t start_time = esp_timer_get_time();
        while (esp_timer_get_time() - start_time < 1000000);
    }
}
