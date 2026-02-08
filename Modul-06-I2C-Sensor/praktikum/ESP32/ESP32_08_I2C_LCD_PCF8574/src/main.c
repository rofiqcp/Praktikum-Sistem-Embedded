/*
 * ESP32_08_I2C_LCD_PCF8574
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Mengendalikan LCD 16x2 via PCF8574 I2C backpack.
 *            Mode 4-bit. Menampilkan teks dan counter.
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 *   PCF8574: P0=RS, P1=RW, P2=EN, P3=BL, P4-P7=D4-D7
 *
 * Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "LCD_I2C";

/* ======================== Konfigurasi Pin I2C ======================== */
#if CONFIG_IDF_TARGET_ESP32
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define I2C_SDA_PIN         8
#define I2C_SCL_PIN         9
#else
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#endif

#define I2C_PORT            I2C_NUM_0
#define I2C_FREQ_HZ         100000

/* ======================== Konfigurasi PCF8574 & LCD ======================== */
#define PCF8574_ADDR        0x27    /* Alamat I2C PCF8574 (A0=A1=A2=1) */

/* Bit mapping pada PCF8574 */
#define LCD_RS              (1 << 0)  /* P0 = RS (Register Select) */
#define LCD_RW              (1 << 1)  /* P1 = RW (Read/Write) */
#define LCD_EN              (1 << 2)  /* P2 = EN (Enable) */
#define LCD_BL              (1 << 3)  /* P3 = Backlight */
/* P4-P7 = D4-D7 (data nibble atas) */

/* Perintah LCD HD44780 */
#define LCD_CMD_CLEAR       0x01    /* Bersihkan layar */
#define LCD_CMD_HOME        0x02    /* Kursor ke posisi awal */
#define LCD_CMD_ENTRY_MODE  0x06    /* Mode entry: increment, no shift */
#define LCD_CMD_DISPLAY_ON  0x0C    /* Display on, cursor off, blink off */
#define LCD_CMD_DISPLAY_OFF 0x08    /* Display off */
#define LCD_CMD_FUNC_SET_4  0x28    /* 4-bit mode, 2 baris, 5x8 dots */
#define LCD_CMD_FUNC_SET_8  0x30    /* 8-bit mode (untuk inisialisasi) */
#define LCD_CMD_SET_DDRAM   0x80    /* Set alamat DDRAM */
#define LCD_LINE1_ADDR      0x00    /* Alamat awal baris 1 */
#define LCD_LINE2_ADDR      0x40    /* Alamat awal baris 2 */

static uint8_t lcd_backlight = LCD_BL; /* Status backlight (on) */

/* Handle I2C bus dan device */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t pcf8574_handle;

/* ======================== Inisialisasi I2C Master ======================== */
static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membuat I2C master bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCF8574_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(bus_handle, &dev_config, &pcf8574_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambahkan device PCF8574: %s", esp_err_to_name(err));
    }
    return err;
}

/* ======================== Tulis Byte ke PCF8574 ======================== */
static esp_err_t pcf8574_write(uint8_t byte_val)
{
    return i2c_master_transmit(pcf8574_handle, &byte_val, 1, -1);
}

/* ======================== Pulse Enable ======================== */
/* Membuat pulse pada pin EN untuk latch data ke LCD */
static void lcd_pulse_enable(uint8_t byte_val)
{
    /* EN high - data di-latch pada falling edge */
    pcf8574_write(byte_val | LCD_EN);
    esp_rom_delay_us(1);  /* Minimal 450ns */

    /* EN low - latch data */
    pcf8574_write(byte_val & ~LCD_EN);
    esp_rom_delay_us(50); /* Tunggu proses perintah */
}

/* ======================== Kirim 4 Bit ke LCD ======================== */
static void lcd_send_nibble(uint8_t nibble, uint8_t mode)
{
    /* Nibble ditempatkan di P4-P7, mode (RS) di P0 */
    uint8_t byte_val = ((nibble & 0x0F) << 4) | mode | lcd_backlight;
    pcf8574_write(byte_val);
    lcd_pulse_enable(byte_val);
}

/* ======================== Kirim Byte ke LCD (4-bit mode) ======================== */
static void lcd_send_byte(uint8_t byte_val, uint8_t mode)
{
    /* Kirim nibble atas terlebih dahulu */
    lcd_send_nibble((byte_val >> 4) & 0x0F, mode);
    /* Kirim nibble bawah */
    lcd_send_nibble(byte_val & 0x0F, mode);
}

/* ======================== Kirim Perintah ke LCD ======================== */
static void lcd_send_cmd(uint8_t cmd_val)
{
    lcd_send_byte(cmd_val, 0); /* RS = 0 untuk perintah */
    if (cmd_val == LCD_CMD_CLEAR || cmd_val == LCD_CMD_HOME) {
        vTaskDelay(pdMS_TO_TICKS(2)); /* Perintah clear/home butuh waktu lebih lama */
    }
}

/* ======================== Kirim Data Karakter ke LCD ======================== */
static void lcd_send_data(uint8_t data)
{
    lcd_send_byte(data, LCD_RS); /* RS = 1 untuk data */
}

/* ======================== Inisialisasi LCD 4-bit Mode ======================== */
static esp_err_t lcd_init(void)
{
    ESP_LOGI(TAG, "Memulai inisialisasi LCD 16x2...");

    /* Tunggu LCD siap setelah power-on (minimal 40ms) */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Matikan semua sinyal, backlight on */
    pcf8574_write(lcd_backlight);
    vTaskDelay(pdMS_TO_TICKS(100));

    /*
     * Urutan inisialisasi sesuai datasheet HD44780:
     * 1. Kirim 0x03 tiga kali (force ke 8-bit mode)
     * 2. Kirim 0x02 (pindah ke 4-bit mode)
     */

    /* Step 1: Kirim 0x03 (8-bit mode), tunggu >4.1ms */
    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));

    /* Step 2: Kirim 0x03 lagi, tunggu >100us */
    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(1));

    /* Step 3: Kirim 0x03 ketiga kali */
    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(1));

    /* Step 4: Pindah ke 4-bit mode */
    lcd_send_nibble(0x02, 0);
    vTaskDelay(pdMS_TO_TICKS(1));

    /* Sekarang dalam 4-bit mode, kirim perintah konfigurasi */
    lcd_send_cmd(LCD_CMD_FUNC_SET_4); /* Function set: 4-bit, 2 baris, 5x8 */
    lcd_send_cmd(LCD_CMD_DISPLAY_ON); /* Display ON, cursor OFF, blink OFF */
    lcd_send_cmd(LCD_CMD_CLEAR);      /* Clear display */
    lcd_send_cmd(LCD_CMD_ENTRY_MODE); /* Entry mode: increment, no shift */

    ESP_LOGI(TAG, "LCD berhasil diinisialisasi (4-bit mode, 2 baris)");
    return ESP_OK;
}

/* ======================== Set Posisi Kursor ======================== */
static void lcd_set_cursor(uint8_t baris, uint8_t kolom)
{
    uint8_t addr;
    if (baris == 0) {
        addr = LCD_LINE1_ADDR + kolom; /* Baris 1 */
    } else {
        addr = LCD_LINE2_ADDR + kolom; /* Baris 2 */
    }
    lcd_send_cmd(LCD_CMD_SET_DDRAM | addr);
}

/* ======================== Cetak String ke LCD ======================== */
static void lcd_print(const char *str)
{
    while (*str) {
        lcd_send_data((uint8_t)*str);
        str++;
    }
}

/* ======================== Bersihkan LCD ======================== */
static void lcd_clear(void)
{
    lcd_send_cmd(LCD_CMD_CLEAR);
}

/* ======================== Kontrol Backlight ======================== */
static void lcd_backlight_on(void)
{
    lcd_backlight = LCD_BL;
    pcf8574_write(lcd_backlight);
}

static void lcd_backlight_off(void)
{
    lcd_backlight = 0;
    pcf8574_write(lcd_backlight);
}

/* ======================== Task Utama LCD ======================== */
static void lcd_task(void *pvParameters)
{
    uint32_t counter = 0;
    char buf[17]; /* Buffer untuk 16 karakter + null terminator */

    /* Tampilkan pesan selamat datang */
    lcd_set_cursor(0, 0);
    lcd_print("Hello ESP32!");
    ESP_LOGI(TAG, "Menampilkan 'Hello ESP32!' di baris 1");

    lcd_set_cursor(1, 0);
    lcd_print("I2C LCD Ready");
    ESP_LOGI(TAG, "Menampilkan 'I2C LCD Ready' di baris 2");

    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Suppress unused warning */
    (void)lcd_clear;

    while (1) {
        counter++;

        /* Baris 1: Pesan tetap */
        lcd_set_cursor(0, 0);
        lcd_print("Hello ESP32!    ");

        /* Baris 2: Counter */
        snprintf(buf, sizeof(buf), "Counter: %-6lu", (unsigned long)counter);
        lcd_set_cursor(1, 0);
        lcd_print(buf);

        /* Output serial untuk monitoring */
        printf("DATA,%lu,Hello ESP32!,Counter: %lu\n", (unsigned long)counter, (unsigned long)counter);
        ESP_LOGI(TAG, "Counter: %lu", (unsigned long)counter);

        /* Kedip backlight setiap 10 hitungan untuk demonstrasi */
        if (counter % 10 == 0) {
            lcd_backlight_off();
            vTaskDelay(pdMS_TO_TICKS(200));
            lcd_backlight_on();
            ESP_LOGI(TAG, "Backlight toggle pada counter %lu", (unsigned long)counter);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ======================== Fungsi Utama ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 I2C LCD PCF8574 ===");
    ESP_LOGI(TAG, "SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
    ESP_LOGI(TAG, "Alamat PCF8574: 0x%02X", PCF8574_ADDR);

    /* Inisialisasi I2C master */
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C master berhasil diinisialisasi");

    /* Inisialisasi LCD */
    esp_err_t err = lcd_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi LCD!");
        return;
    }

    printf("HDR,counter,baris1,baris2\n");

    /* Buat task LCD */
    xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 5, NULL);
}
