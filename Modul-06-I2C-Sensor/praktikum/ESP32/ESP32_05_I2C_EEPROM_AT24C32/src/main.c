/**
 * ESP32_05_I2C_EEPROM_AT24C32
 * Modul 06 - I2C & Sensor
 *
 * Membaca dan menulis data ke EEPROM AT24C32 melalui I2C.
 * Mendukung byte write, page write (32 byte), dan sequential read.
 * Alamat memori 2 byte (AT24C32 = 32Kbit = 4096 byte).
 *
 * Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "EEPROM";

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

#define EEPROM_ADDR       0x50
#define EEPROM_PAGE_SIZE  32     /* AT24C32 page = 32 byte */
#define EEPROM_SIZE       4096   /* 32Kbit = 4096 byte */
#define EEPROM_WRITE_TIME 5      /* Waktu tulis dalam ms */

/* Handle I2C bus dan device */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t eeprom_handle;

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

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = EEPROM_ADDR,
        .scl_speed_hz = I2C_FREQ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &eeprom_handle));
}

/* ---- EEPROM: tulis satu byte (alamat 2 byte) ---- */
static esp_err_t eeprom_write_byte(uint16_t mem_addr, uint8_t data) {
    uint8_t write_buf[3];
    write_buf[0] = (uint8_t)(mem_addr >> 8);    /* Alamat tinggi */
    write_buf[1] = (uint8_t)(mem_addr & 0xFF);  /* Alamat rendah */
    write_buf[2] = data;
    esp_err_t ret = i2c_master_transmit(eeprom_handle, write_buf, 3, -1);

    /* Tunggu siklus tulis EEPROM */
    vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_TIME));
    return ret;
}

/* ---- EEPROM: baca satu byte (alamat 2 byte) ---- */
static esp_err_t eeprom_read_byte(uint16_t mem_addr, uint8_t *data) {
    uint8_t addr_buf[2];
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFF);
    return i2c_master_transmit_receive(eeprom_handle, addr_buf, 2, data, 1, -1);
}

/* ---- EEPROM: page write (maks 32 byte, harus dalam 1 page) ---- */
static esp_err_t eeprom_page_write(uint16_t mem_addr, const uint8_t *data, size_t len) {
    if (len == 0 || len > EEPROM_PAGE_SIZE) {
        ESP_LOGE(TAG, "Panjang page write tidak valid: %d (maks %d)", (int)len, EEPROM_PAGE_SIZE);
        return ESP_ERR_INVALID_ARG;
    }

    /* Pastikan tidak melewati batas page */
    uint16_t page_start = (mem_addr / EEPROM_PAGE_SIZE) * EEPROM_PAGE_SIZE;
    uint16_t page_end = page_start + EEPROM_PAGE_SIZE;
    if (mem_addr + len > page_end) {
        ESP_LOGW(TAG, "Data melewati batas page! Memotong.");
        len = page_end - mem_addr;
    }

    /* Gabungkan alamat 2 byte + data ke satu buffer untuk transmit */
    uint8_t write_buf[2 + EEPROM_PAGE_SIZE];
    write_buf[0] = (uint8_t)(mem_addr >> 8);
    write_buf[1] = (uint8_t)(mem_addr & 0xFF);
    memcpy(&write_buf[2], data, len);

    esp_err_t ret = i2c_master_transmit(eeprom_handle, write_buf, 2 + len, -1);

    vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_TIME));
    return ret;
}

/* ---- EEPROM: sequential read ---- */
static esp_err_t eeprom_seq_read(uint16_t mem_addr, uint8_t *buf, size_t len) {
    if (len == 0) return ESP_ERR_INVALID_ARG;

    uint8_t addr_buf[2];
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFF);
    return i2c_master_transmit_receive(eeprom_handle, addr_buf, 2, buf, len, -1);
}

/* ---- Hex dump untuk debug ---- */
static void hex_dump(const char *label, uint16_t addr, const uint8_t *buf, size_t len) {
    printf("%s (0x%04X - 0x%04X):\n", label, addr, (uint16_t)(addr + len - 1));
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) printf("  %04X: ", (uint16_t)(addr + i));
        printf("%02X ", buf[i]);
        if ((i + 1) % 16 == 0 || i == len - 1) printf("\n");
    }
}

/* ---- Test 1: Byte write/read ---- */
static void test_byte_rw(void) {
    printf("\n=== TEST 1: Byte Write/Read ===\n");
    uint16_t test_addr = 0x0000;
    uint8_t test_vals[] = {0xAA, 0x55, 0x00, 0xFF, 0x42};
    int pass = 0, fail = 0;

    for (int i = 0; i < (int)(sizeof(test_vals)); i++) {
        uint16_t addr = test_addr + i;
        esp_err_t ret = eeprom_write_byte(addr, test_vals[i]);
        if (ret != ESP_OK) {
            printf("  GAGAL menulis 0x%02X ke 0x%04X\n", test_vals[i], addr);
            fail++;
            continue;
        }

        uint8_t read_val = 0;
        ret = eeprom_read_byte(addr, &read_val);
        if (ret == ESP_OK && read_val == test_vals[i]) {
            printf("  OK: addr=0x%04X wrote=0x%02X read=0x%02X\n", addr, test_vals[i], read_val);
            pass++;
        } else {
            printf("  GAGAL: addr=0x%04X wrote=0x%02X read=0x%02X\n", addr, test_vals[i], read_val);
            fail++;
        }
    }
    printf("EEPROM_TEST1: pass=%d fail=%d\n", pass, fail);
}

/* ---- Test 2: Page write/read ---- */
static void test_page_rw(void) {
    printf("\n=== TEST 2: Page Write/Read ===\n");
    uint16_t page_addr = 0x0020; /* Mulai dari page 1 */
    uint8_t write_buf[EEPROM_PAGE_SIZE];
    uint8_t read_buf[EEPROM_PAGE_SIZE];

    /* Isi buffer dengan pola */
    for (int i = 0; i < EEPROM_PAGE_SIZE; i++) {
        write_buf[i] = (uint8_t)(i * 3 + 0x10);
    }

    /* Page write */
    esp_err_t ret = eeprom_page_write(page_addr, write_buf, EEPROM_PAGE_SIZE);
    if (ret != ESP_OK) {
        printf("  GAGAL page write! (err=%d)\n", ret);
        printf("EEPROM_TEST2: pass=0 fail=1\n");
        return;
    }
    printf("  Page write OK (%d byte ke 0x%04X)\n", EEPROM_PAGE_SIZE, page_addr);

    /* Sequential read */
    memset(read_buf, 0, sizeof(read_buf));
    ret = eeprom_seq_read(page_addr, read_buf, EEPROM_PAGE_SIZE);
    if (ret != ESP_OK) {
        printf("  GAGAL sequential read! (err=%d)\n", ret);
        printf("EEPROM_TEST2: pass=0 fail=1\n");
        return;
    }

    /* Verifikasi */
    int errors = 0;
    for (int i = 0; i < EEPROM_PAGE_SIZE; i++) {
        if (read_buf[i] != write_buf[i]) {
            printf("  MISMATCH: addr=0x%04X expected=0x%02X got=0x%02X\n",
                   (uint16_t)(page_addr + i), write_buf[i], read_buf[i]);
            errors++;
        }
    }

    hex_dump("  Data terbaca", page_addr, read_buf, EEPROM_PAGE_SIZE);

    if (errors == 0) {
        printf("  Verifikasi BERHASIL! %d byte cocok.\n", EEPROM_PAGE_SIZE);
        printf("EEPROM_TEST2: pass=1 fail=0\n");
    } else {
        printf("  Verifikasi GAGAL: %d byte tidak cocok.\n", errors);
        printf("EEPROM_TEST2: pass=0 fail=1\n");
    }
}

/* ---- Test 3: String write/read ---- */
static void test_string_rw(void) {
    printf("\n=== TEST 3: String Write/Read ===\n");
    uint16_t str_addr = 0x0100;
    const char *test_str = "Hello EEPROM ESP32!";
    size_t str_len = strlen(test_str) + 1; /* Termasuk null terminator */
    char read_str[32];

    /* Tulis string per page */
    size_t written = 0;
    while (written < str_len) {
        uint16_t cur_addr = str_addr + written;
        /* Hitung sisa byte dalam page saat ini */
        uint16_t page_boundary = ((cur_addr / EEPROM_PAGE_SIZE) + 1) * EEPROM_PAGE_SIZE;
        size_t chunk = page_boundary - cur_addr;
        if (chunk > str_len - written) chunk = str_len - written;
        if (chunk > EEPROM_PAGE_SIZE) chunk = EEPROM_PAGE_SIZE;

        esp_err_t ret = eeprom_page_write(cur_addr, (const uint8_t *)(test_str + written), chunk);
        if (ret != ESP_OK) {
            printf("  GAGAL menulis chunk ke 0x%04X\n", cur_addr);
            printf("EEPROM_TEST3: pass=0 fail=1\n");
            return;
        }
        written += chunk;
    }
    printf("  String tertulis: \"%s\" (%d byte)\n", test_str, (int)str_len);

    /* Baca balik */
    memset(read_str, 0, sizeof(read_str));
    esp_err_t ret = eeprom_seq_read(str_addr, (uint8_t *)read_str, str_len);
    if (ret != ESP_OK) {
        printf("  GAGAL membaca string!\n");
        printf("EEPROM_TEST3: pass=0 fail=1\n");
        return;
    }

    printf("  String terbaca : \"%s\"\n", read_str);

    if (strcmp(test_str, read_str) == 0) {
        printf("  Verifikasi BERHASIL!\n");
        printf("EEPROM_TEST3: pass=1 fail=0\n");
    } else {
        printf("  Verifikasi GAGAL!\n");
        printf("EEPROM_TEST3: pass=0 fail=1\n");
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Inisialisasi I2C Master...");
    i2c_master_init();

    ESP_LOGI(TAG, "=== EEPROM AT24C32 Test Suite ===");
    ESP_LOGI(TAG, "Alamat: 0x%02X, Page: %d byte, Total: %d byte",
             EEPROM_ADDR, EEPROM_PAGE_SIZE, EEPROM_SIZE);

    uint32_t cycle = 0;

    while (1) {
        printf("\n########################################\n");
        printf("# EEPROM Test - Siklus %lu\n", (unsigned long)cycle);
        printf("########################################\n");

        test_byte_rw();
        test_page_rw();
        test_string_rw();

        printf("\nEEPROM_CYCLE: cycle=%lu status=complete\n", (unsigned long)cycle);

        cycle++;
        ESP_LOGI(TAG, "Test berikutnya dalam 10 detik...");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
