/**
 * ESP32_02_I2C_OLED_SSD1306
 * Modul 06 - I2C & Sensor
 *
 * Mengendalikan OLED SSD1306 128x64 melalui I2C.
 * Menampilkan teks "HELLO ESP32" dan counter yang berjalan.
 *
 * Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "SSD1306";

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
#define I2C_PORT  I2C_NUM_0
#define I2C_FREQ  400000  /* SSD1306 mendukung 400kHz */

#define SSD1306_ADDR   0x3C
#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES  (SSD1306_HEIGHT / 8)

/* ---- Buffer framebuffer ---- */
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_PAGES];

/* ---- Handle I2C bus dan device ---- */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t ssd1306_dev_handle;

/* ---- Inisialisasi I2C Master ---- */
static void i2c_master_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    /* Tambahkan SSD1306 sebagai device pada bus I2C */
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SSD1306_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &ssd1306_dev_handle));
}

/* ---- Helper: tulis register ---- */
static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t val) {
    uint8_t write_buf[2] = {reg, val};
    return i2c_master_transmit(dev_handle, write_buf, 2, -1);
}

/* ---- Helper: baca register ---- */
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *buf, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, buf, len, -1);
}

/* ---- Kirim perintah ke SSD1306 ---- */
static esp_err_t ssd1306_send_cmd(uint8_t cmd_byte) {
    uint8_t write_buf[2] = {0x00, cmd_byte};  /* Co=0, D/C#=0 : command */
    return i2c_master_transmit(ssd1306_dev_handle, write_buf, 2, -1);
}

/* ---- Kirim data ke SSD1306 ---- */
static esp_err_t ssd1306_send_data(const uint8_t *data, size_t len) {
    /* Alokasi buffer: 1 byte control + len byte data */
    uint8_t *write_buf = malloc(len + 1);
    if (!write_buf) return ESP_ERR_NO_MEM;
    write_buf[0] = 0x40;  /* Co=0, D/C#=1 : data */
    memcpy(write_buf + 1, data, len);
    esp_err_t ret = i2c_master_transmit(ssd1306_dev_handle, write_buf, len + 1, -1);
    free(write_buf);
    return ret;
}

/* ---- Font 5x7 - Karakter ASCII 32..90 (spasi hingga Z) + beberapa ---- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 32: Space */
    {0x00,0x00,0x5F,0x00,0x00}, /* 33: ! */
    {0x00,0x07,0x00,0x07,0x00}, /* 34: " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* 35: # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* 36: $ */
    {0x23,0x13,0x08,0x64,0x62}, /* 37: % */
    {0x36,0x49,0x55,0x22,0x50}, /* 38: & */
    {0x00,0x05,0x03,0x00,0x00}, /* 39: ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* 40: ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* 41: ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* 42: * */
    {0x08,0x08,0x3E,0x08,0x08}, /* 43: + */
    {0x00,0x50,0x30,0x00,0x00}, /* 44: , */
    {0x08,0x08,0x08,0x08,0x08}, /* 45: - */
    {0x00,0x60,0x60,0x00,0x00}, /* 46: . */
    {0x20,0x10,0x08,0x04,0x02}, /* 47: / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 48: 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 49: 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 50: 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 51: 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 52: 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 53: 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 54: 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 55: 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 56: 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 57: 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* 58: : */
    {0x00,0x56,0x36,0x00,0x00}, /* 59: ; */
    {0x00,0x08,0x14,0x22,0x41}, /* 60: < */
    {0x14,0x14,0x14,0x14,0x14}, /* 61: = */
    {0x41,0x22,0x14,0x08,0x00}, /* 62: > */
    {0x02,0x01,0x51,0x09,0x06}, /* 63: ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* 64: @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 65: A */
    {0x7F,0x49,0x49,0x49,0x36}, /* 66: B */
    {0x3E,0x41,0x41,0x41,0x22}, /* 67: C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 68: D */
    {0x7F,0x49,0x49,0x49,0x41}, /* 69: E */
    {0x7F,0x09,0x09,0x01,0x01}, /* 70: F */
    {0x3E,0x41,0x41,0x51,0x32}, /* 71: G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 72: H */
    {0x00,0x41,0x7F,0x41,0x00}, /* 73: I */
    {0x20,0x40,0x41,0x3F,0x01}, /* 74: J */
    {0x7F,0x08,0x14,0x22,0x41}, /* 75: K */
    {0x7F,0x40,0x40,0x40,0x40}, /* 76: L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* 77: M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 78: N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 79: O */
    {0x7F,0x09,0x09,0x09,0x06}, /* 80: P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 81: Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* 82: R */
    {0x46,0x49,0x49,0x49,0x31}, /* 83: S */
    {0x01,0x01,0x7F,0x01,0x01}, /* 84: T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 85: U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 86: V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* 87: W */
    {0x63,0x14,0x08,0x14,0x63}, /* 88: X */
    {0x03,0x04,0x78,0x04,0x03}, /* 89: Y */
    {0x61,0x51,0x49,0x45,0x43}, /* 90: Z */
};

/* ---- Inisialisasi SSD1306 ---- */
static void ssd1306_init(void) {
    /* Urutan perintah inisialisasi SSD1306 128x64 */
    ssd1306_send_cmd(0xAE); /* Display OFF */
    ssd1306_send_cmd(0xD5); /* Set display clock */
    ssd1306_send_cmd(0x80); /* Rasio default */
    ssd1306_send_cmd(0xA8); /* Set multiplex ratio */
    ssd1306_send_cmd(0x3F); /* 64 baris (0x3F = 63) */
    ssd1306_send_cmd(0xD3); /* Set display offset */
    ssd1306_send_cmd(0x00); /* Tidak ada offset */
    ssd1306_send_cmd(0x40); /* Set start line = 0 */
    ssd1306_send_cmd(0x8D); /* Charge pump */
    ssd1306_send_cmd(0x14); /* Enable charge pump */
    ssd1306_send_cmd(0x20); /* Memory addressing mode */
    ssd1306_send_cmd(0x00); /* Horizontal addressing */
    ssd1306_send_cmd(0xA1); /* Segment remap (flip horizontal) */
    ssd1306_send_cmd(0xC8); /* COM scan direction (flip vertical) */
    ssd1306_send_cmd(0xDA); /* Set COM pins */
    ssd1306_send_cmd(0x12); /* Alternatif COM pins */
    ssd1306_send_cmd(0x81); /* Set contrast */
    ssd1306_send_cmd(0xCF); /* Kontras tinggi */
    ssd1306_send_cmd(0xD9); /* Set pre-charge period */
    ssd1306_send_cmd(0xF1);
    ssd1306_send_cmd(0xDB); /* Set VCOMH deselect level */
    ssd1306_send_cmd(0x40);
    ssd1306_send_cmd(0xA4); /* Display dari RAM */
    ssd1306_send_cmd(0xA6); /* Normal display (bukan inverted) */
    ssd1306_send_cmd(0xAF); /* Display ON */

    ESP_LOGI(TAG, "SSD1306 diinisialisasi");
}

/* ---- Bersihkan framebuffer dan display ---- */
static void ssd1306_clear(void) {
    memset(ssd1306_buffer, 0x00, sizeof(ssd1306_buffer));
}

/* ---- Update display dari framebuffer ---- */
static void ssd1306_update(void) {
    ssd1306_send_cmd(0x21); /* Set column address */
    ssd1306_send_cmd(0x00); /* Kolom awal */
    ssd1306_send_cmd(0x7F); /* Kolom akhir (127) */
    ssd1306_send_cmd(0x22); /* Set page address */
    ssd1306_send_cmd(0x00); /* Page awal */
    ssd1306_send_cmd(0x07); /* Page akhir (7) */

    /* Kirim per page agar tidak melebihi buffer I2C */
    for (int page = 0; page < SSD1306_PAGES; page++) {
        ssd1306_send_data(&ssd1306_buffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
    }
}

/* ---- Set pixel di framebuffer ---- */
static void ssd1306_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    int page = y / 8;
    int bit = y % 8;
    if (on)
        ssd1306_buffer[page * SSD1306_WIDTH + x] |= (1 << bit);
    else
        ssd1306_buffer[page * SSD1306_WIDTH + x] &= ~(1 << bit);
}

/* ---- Tulis satu karakter pada posisi (x, y) ---- */
static void ssd1306_write_char(int x, int y, char c) {
    if (c < 32 || c > 90) {
        /* Konversi huruf kecil ke besar */
        if (c >= 'a' && c <= 'z') c = c - 32;
        else return;
    }
    int idx = c - 32;
    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            ssd1306_set_pixel(x + col, y + row, (line >> row) & 0x01);
        }
    }
}

/* ---- Tulis string pada posisi (x, y) ---- */
static void ssd1306_write_string(int x, int y, const char *str) {
    int cx = x;
    while (*str) {
        ssd1306_write_char(cx, y, *str);
        cx += 6; /* 5 pixel karakter + 1 pixel spasi */
        if (cx > SSD1306_WIDTH - 5) {
            cx = 0;
            y += 9; /* 7 pixel tinggi + 2 pixel spasi baris */
        }
        str++;
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    /* Suppress unused warnings */
    (void)i2c_write_reg;
    (void)i2c_read_reg;

    ESP_LOGI(TAG, "Inisialisasi OLED SSD1306...");
    ssd1306_init();

    uint32_t counter = 0;
    char counter_str[32];

    while (1) {
        ssd1306_clear();

        /* Baris 1: Judul */
        ssd1306_write_string(10, 2, "HELLO ESP32");

        /* Baris 2: Garis pemisah */
        for (int x = 0; x < SSD1306_WIDTH; x++) {
            ssd1306_set_pixel(x, 14, true);
        }

        /* Baris 3: Label modul */
        ssd1306_write_string(4, 18, "MODUL 06 I2C");

        /* Baris 4: Counter */
        snprintf(counter_str, sizeof(counter_str), "COUNT: %lu", (unsigned long)counter);
        ssd1306_write_string(4, 30, counter_str);

        /* Baris 5: Info SDA/SCL */
        snprintf(counter_str, sizeof(counter_str), "SDA=%d SCL=%d", I2C_SDA, I2C_SCL);
        ssd1306_write_string(4, 42, counter_str);

        /* Update ke display */
        ssd1306_update();

        /* Output serial untuk monitoring */
        printf("OLED_UPDATE: counter=%lu\n", (unsigned long)counter);

        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
