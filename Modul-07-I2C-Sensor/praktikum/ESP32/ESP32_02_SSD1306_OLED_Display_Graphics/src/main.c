/**
 * ==========================================================
 *  Modul 07 - ESP32_02_SSD1306_OLED_Display_Graphics
 * ==========================================================
 *  Deskripsi:
 *    Demo grafis pada OLED SSD1306 128x64 via I2C. Menampilkan animasi
 *    garis bergerak menggunakan algoritma Bresenham. Render grafis ke
 *    framebuffer 128x64 pixel (1 bit/pixel), kirim ke OLED setiap 250ms.
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
 *    I2C_SDA_PIN GPIO21 (ESP32) / GPIO8 (S2/S3)
 *    I2C_SCL_PIN GPIO22 (ESP32) / GPIO9 (S2/S3)
 *    I2C_FREQ_HZ 400000
 *    OLED_ADDR 0x3C
 *    OLED_WIDTH 128
 *    OLED_HEIGHT 64
 *    ANIM_DELAY_MS 250
 * ==========================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "SSD1306";

/* Konfigurasi I2C dan OLED - sesuaikan sesuai kebutuhan */
#if CONFIG_IDF_TARGET_ESP32
#define I2C_SDA_PIN 21           // Pin SDA untuk ESP32
#define I2C_SCL_PIN 22           // Pin SCL untuk ESP32
#else
#define I2C_SDA_PIN 8            // Pin SDA untuk ESP32-S3/C3/etc
#define I2C_SCL_PIN 9            // Pin SCL untuk ESP32-S3/C3/etc
#endif
#define I2C_FREQ_HZ 400000       // Kecepatan I2C: 400kHz untuk OLED
#define OLED_ADDR 0x3C           // Alamat I2C SSD1306
#define OLED_WIDTH 128           // Lebar display pixel
#define OLED_HEIGHT 64           // Tinggi display pixel
#define ANIM_DELAY_MS 250        // Delay antar frame animasi

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t oled;
static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8];  // Framebuffer 1-bit

/* Kirim perintah ke OLED (control byte 0x00) */
static void oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  // 0x00 = command mode
    ESP_ERROR_CHECK(i2c_master_transmit(oled, buf, sizeof(buf), -1));
}

/* Kirim data ke OLED (control byte 0x40) */
static void oled_data(const uint8_t *data, size_t len)
{
    uint8_t buf[17];
    buf[0] = 0x40;  // 0x40 = data mode
    while (len) {
        size_t chunk = len > 16 ? 16 : len;
        memcpy(&buf[1], data, chunk);
        ESP_ERROR_CHECK(i2c_master_transmit(oled, buf, chunk + 1, -1));
        data += chunk;
        len -= chunk;
    }
}

/* Set pixel pada framebuffer */
static void set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    if (on) {
        framebuffer[x + (y / 8) * OLED_WIDTH] |= 1 << (y & 7);
    } else {
        framebuffer[x + (y / 8) * OLED_WIDTH] &= ~(1 << (y & 7));
    }
}

/* Algoritma Bresenham line - menggambar garis dari (x0,y0) ke (x1,y1) */
static void draw_line(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1) {
        set_pixel(x0, y0, true);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Flush framebuffer ke OLED - kirim per halaman (8 pixel tinggi) */
static void flush_oled(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page);   // Set page address
        oled_cmd(0x00);          // Set lower column address
        oled_cmd(0x10);          // Set higher column address
        oled_data(&framebuffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}

/* Inisialisasi I2C bus dan OLED SSD1306 */
static void init_oled(void)
{
    // Konfigurasi I2C master bus
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
    
    // Tambahkan device OLED ke bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &oled));
    
    // Perintah inisialisasi SSD1306
    const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0x20, 0x00, // Memory addressing mode: horizontal
        0xB0,       // Set page start address
        0xC8,       // COM output scan direction: remap
        0x00, 0x10, // Set column address
        0x40,       // Set display start line
        0x81, 0x7F, // Set contrast
        0xA1,       // Segment re-map
        0xA6,       // Normal display
        0xA8, 0x3F, // Multiplex ratio
        0xA4,       // Display all on resume
        0xD3, 0x00, // Display offset
        0xD5, 0x80, // Display clock divide ratio
        0xD9, 0xF1, // Pre-charge period
        0xDA, 0x12, // COM pins hardware configuration
        0xDB, 0x40, // VCOMH deselect level
        0x8D, 0x14, // Charge pump setting
        0xAF        // Display ON
    };
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        oled_cmd(init_cmds[i]);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Memulai demo grafis SSD1306");
    init_oled();
    
    int offset = 0;
    // Loop utama: render animasi
    while (1) {
        memset(framebuffer, 0, sizeof(framebuffer));  // Clear framebuffer
        
        // Gambar garis-garis dari tengah ke kiri/kanan
        for (int x = 0; x < OLED_WIDTH; x += 8) {
            draw_line(OLED_WIDTH/2, OLED_HEIGHT/2, x, (x + offset) % OLED_HEIGHT);
        }
        // Gambar garis diagonal
        for (int y = 0; y < OLED_HEIGHT; y += 8) {
            draw_line(0, y, OLED_WIDTH - 1, OLED_HEIGHT - 1 - y);
        }
        
        flush_oled();  // Kirim ke OLED
        offset = (offset + 3) % OLED_HEIGHT;  // Update offset animasi
        vTaskDelay(pdMS_TO_TICKS(ANIM_DELAY_MS));
    }
}
