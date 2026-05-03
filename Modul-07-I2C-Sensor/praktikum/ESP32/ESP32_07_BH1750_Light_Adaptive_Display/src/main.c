/**
 * ==========================================================
 *  Modul 07 - ESP32_07_BH1750_Light_Adaptive_Display
 * ==========================================================
 *  Deskripsi:
 *    Membaca sensor cahaya BH1750 dan menampilkan bar grafik
 *    adaptif berdasarkan intensitas cahaya. Mode: gelap (<50 lux),
 *    normal (50-500 lux), terang (>500 lux). Pembacaan via I2C
 *    dengan resolusi tinggi (0x10 command).
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
 *    BH1750_ADDR 0x23
 *    READ_INTERVAL_MS 1000
 *    LUX_MODE 0x10
 *    LUX_DELAY_MS 180
 *    BAR_LENGTH 20
 * ==========================================================
 */

#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"

// Konfigurasi pin I2C
#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22
#define I2C_PORT I2C_NUM_0
#define I2C_FREQ_HZ 100000

// Alamat BH1750 (0x23 jika ADDR LOW, 0x5C jika ADDR HIGH)
#define BH1750_ADDR 0x23

// Mode pengukuran: 0x10 = Continuously H-Resolution Mode (1 lx resolution)
#define LUX_MODE 0x10

// Delay untuk konversi (180ms untuk H-Resolution)
#define LUX_DELAY_MS 180

// Interval pembacaan
#define READ_INTERVAL_MS 1000

// Panjang bar grafik
#define BAR_LENGTH 20

static const char *TAG = "BH1750";

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t bh1750;

// Baca lux dari BH1750
static float read_lux(void) {
    // Kirim command mode pengukuran
    uint8_t cmd = LUX_MODE;
    ESP_ERROR_CHECK(i2c_master_transmit(bh1750, &cmd, 1, -1));

    // Tunggu konversi selesai (non-RTOS)
    uint64_t start = esp_timer_get_time();
    while (esp_timer_get_time() - start < LUX_DELAY_MS * 1000) {}

    // Baca 2 byte data (MSB first)
    uint8_t data[2];
    ESP_ERROR_CHECK(i2c_master_receive(bh1750, data, 2, -1));

    // Konversi ke lux: (data[0] << 8 | data[1]) / 1.2
    uint16_t raw = (uint16_t)((data[0] << 8) | data[1]);
    return raw / 1.2f;
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

    // Add BH1750 device
    i2c_device_config_t dev_cfg = {
        .device_address = BH1750_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &bh1750));

    ESP_LOGI(TAG, "BH1750 siap, membaca setiap %d ms", READ_INTERVAL_MS);

    while (1) {
        float lux = read_lux();

        // Tentukan mode berdasarkan lux
        const char *mode;
        if (lux < 50) {
            mode = "gelap";
        } else if (lux < 500) {
            mode = "normal";
        } else {
            mode = "terang";
        }

        // Buat bar grafik
        int bar_fill = (lux > 1000) ? BAR_LENGTH : (int)(lux / 50);
        if (bar_fill > BAR_LENGTH) bar_fill = BAR_LENGTH;

        printf("LUX: %.1f |", lux);
        for (int i = 0; i < BAR_LENGTH; i++) {
            putchar(i < bar_fill ? '#' : '-');
        }
        printf("| mode=%s\n", mode);

        // Delay non-RTOS
        uint64_t start = esp_timer_get_time();
        while (esp_timer_get_time() - start < READ_INTERVAL_MS * 1000) {}
    }
}
