/**
 * ==========================================================
 *  Modul 07 - ESP32_01_I2C_Bus_Scanner_Address_Map
 * ==========================================================
 *  Deskripsi:
 *    Scan I2C bus alamat 0x01 hingga 0x7F, identifikasi perangkat yang dikenal (SSD1306, BME280, MPU6050, dsb). Tampilkan hasil scan ke serial monitor setiap 5 detik.
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
 *    I2C_SCAN_START 0x01
 *    I2C_SCAN_END 0x7F
 *    SCAN_DELAY_MS 5000
 *    I2C_SDA_GPIO GPIO21
 *    I2C_SCL_GPIO GPIO22
 *    I2C_PORT I2C_NUM_0
 *    I2C_FREQ_HZ 100000
 * ==========================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

// Konfigurasi scan I2C
#define I2C_SCAN_START 0x01       // Alamat awal scan
#define I2C_SCAN_END 0x7F         // Alamat akhir scan
#define SCAN_DELAY_MS 5000        // Delay antar scan (ms)

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
#define I2C_PORT I2C_NUM_0        // Port I2C yang digunakan
#define I2C_FREQ_HZ 100000        // Frekuensi I2C (100kHz)

static const char *TAG = "I2C_SCAN";

void i2c_scan(i2c_master_bus_handle_t bus_handle) {
    ESP_LOGI(TAG, "Memulai scan I2C bus 0x%02X - 0x%02X", I2C_SCAN_START, I2C_SCAN_END);
    for (uint8_t addr = I2C_SCAN_START; addr <= I2C_SCAN_END; addr++) {
        // Konfigurasi device I2C untuk alamat tertentu
        i2c_device_config_t dev_cfg = {
            .device_address = addr,
            .scl_speed_hz = I2C_FREQ_HZ,
        };
        i2c_master_dev_handle_t dev_handle;
        esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
        if (err == ESP_OK) {
            // Coba kirim data kosong untuk cek ACK
            err = i2c_master_transmit(dev_handle, NULL, 0, 100);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Ditemukan device di 0x%02X", addr);
                // Identifikasi perangkat yang dikenal
                switch(addr) {
                    case 0x3C: ESP_LOGI(TAG, "  -> SSD1306 OLED"); break;
                    case 0x76: case 0x77: ESP_LOGI(TAG, "  -> BME280"); break;
                    case 0x68: case 0x69: ESP_LOGI(TAG, "  -> MPU6050 / DS3231"); break;
                    case 0x50: ESP_LOGI(TAG, "  -> AT24C32 EEPROM"); break;
                    case 0x23: case 0x5C: ESP_LOGI(TAG, "  -> BH1750 Light Sensor"); break;
                    default: ESP_LOGI(TAG, "  -> Perangkat tidak dikenal"); break;
                }
            }
            // Hapus device dari bus
            i2c_master_bus_rm_device(dev_handle);
        }
    }
    ESP_LOGI(TAG, "Scan selesai\n");
}

void app_main(void) {
    // Inisialisasi I2C master bus
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    // Loop utama scan I2C setiap SCAN_DELAY_MS
    while (1) {
        i2c_scan(bus_handle);
        vTaskDelay(SCAN_DELAY_MS / portTICK_PERIOD_MS);
    }

    // Tidak akan sampai sini, tapi untuk best practice
    i2c_del_master_bus(bus_handle);
}
