/**
 * ==========================================================
 *  Modul 07 - ESP32_05_AT24C32_EEPROM_Data_Log
 * ==========================================================
 *  Deskripsi:
 *    Menulis dan membaca data ke EEPROM AT24C32 via I2C.
 *    Mencatat timestamp (tick count) ke EEPROM, lalu membaca
 *    kembali untuk verifikasi. EEPROM 4KB (32kbit) dengan
 *    alamat I2C 0x50. Data ditulis setiap 2 detik.
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
 *    EEPROM_ADDR 0x50
 *    EEPROM_SIZE 4096
 *    LOG_ENTRY_SIZE 32
 *    READ_INTERVAL_MS 2000
 * ==========================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// Konfigurasi pin I2C
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000

// Alamat EEPROM AT24C32
#define EEPROM_ADDR 0x50

// Ukuran EEPROM (4KB = 32kbit)
#define EEPROM_SIZE 4096

// Ukuran setiap entri log
#define LOG_ENTRY_SIZE 32

// Interval penulisan
#define READ_INTERVAL_MS 2000

static const char *TAG = "EEPROM";

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t eeprom;

// Tulis data ke EEPROM (alamat 16-bit)
static esp_err_t eeprom_write(uint16_t addr, const uint8_t *data, size_t len) {
    uint8_t buf[LOG_ENTRY_SIZE + 2];
    if (len > LOG_ENTRY_SIZE) len = LOG_ENTRY_SIZE;
    buf[0] = addr >> 8;
    buf[1] = addr & 0xFF;
    memcpy(&buf[2], data, len);
    esp_err_t ret = i2c_master_transmit(eeprom, buf, len + 2, -1);
    vTaskDelay(pdMS_TO_TICKS(10));  // Tunggu write cycle
    return ret;
}

// Baca data dari EEPROM (alamat 16-bit)
static esp_err_t eeprom_read(uint16_t addr, uint8_t *data, size_t len) {
    uint8_t cmd[2] = {addr >> 8, addr & 0xFF};
    return i2c_master_transmit_receive(eeprom, cmd, 2, data, len, -1);
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

    // Add EEPROM device
    i2c_device_config_t dev_cfg = {
        .device_address = EEPROM_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &eeprom));

    ESP_LOGI(TAG, "AT24C32 EEPROM siap, ukuran %d bytes", EEPROM_SIZE);

    uint16_t write_ptr = 0;

    while (1) {
        char log_entry[LOG_ENTRY_SIZE];
        char read_back[LOG_ENTRY_SIZE] = {0};

        // Format log entry dengan timestamp
        snprintf(log_entry, sizeof(log_entry), "LOG %05lu\n",
                 (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));

        size_t len = strlen(log_entry);

        // Wrap around jika mencapai akhir EEPROM
        if (write_ptr + len >= EEPROM_SIZE) {
            write_ptr = 0;
        }

        // Tulis ke EEPROM
        ESP_ERROR_CHECK(eeprom_write(write_ptr, (uint8_t *)log_entry, len));
        ESP_LOGI(TAG, "Write @ 0x%03X: '%s'", write_ptr, log_entry);

        // Baca kembali untuk verifikasi
        ESP_ERROR_CHECK(eeprom_read(write_ptr, (uint8_t *)read_back, len));
        ESP_LOGI(TAG, "Read  @ 0x%03X: '%s'", write_ptr, read_back);

        // Cek apakah data sama
        if (strncmp(log_entry, read_back, len) == 0) {
            ESP_LOGI(TAG, "Verify: OK");
        } else {
            ESP_LOGW(TAG, "Verify: MISMATCH");
        }

        write_ptr += LOG_ENTRY_SIZE;
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
