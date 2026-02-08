/*
 * ESP32_10_I2C_Write_Read_Raw
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Demonstrasi protokol I2C level rendah secara eksplisit.
 *            Menunjukkan setiap tahap: START, ADDRESS, WRITE, RESTART,
 *            READ, ACK/NACK, STOP dengan logging verbose.
 *            Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

static const char *TAG = "I2C_RAW";

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

/* ======================== Konfigurasi Target ======================== */
/* Ubah alamat dan register ini sesuai perangkat yang digunakan */
#define TARGET_ADDR         0x76    /* Alamat perangkat I2C target (BMP280) */
#define TARGET_REG          0xD0    /* Register yang akan dibaca (Chip ID) */
#define READ_LENGTH         1       /* Jumlah byte yang dibaca */

/* Alamat alternatif untuk testing */
#define ALT_ADDR_1          0x23    /* BH1750 */
#define ALT_ADDR_2          0x50    /* EEPROM AT24C32 */
#define ALT_ADDR_3          0x68    /* DS3231 RTC */

/* Handle I2C bus dan device */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t target_dev_handle;
static i2c_master_dev_handle_t alt1_dev_handle;

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
    if (err != ESP_OK) return err;

    /* Tambahkan device target (BMP280) ke bus */
    i2c_device_config_t target_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TARGET_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(bus_handle, &target_cfg, &target_dev_handle);
    if (err != ESP_OK) return err;

    /* Tambahkan device alternatif 1 (BH1750) ke bus */
    i2c_device_config_t alt1_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ALT_ADDR_1,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(bus_handle, &alt1_cfg, &alt1_dev_handle);
    if (err != ESP_OK) return err;

    return ESP_OK;
}

/* ======================== Tulis Register I2C ======================== */
static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                                uint8_t *data, size_t len)
{
    uint8_t write_buf[1 + len];
    write_buf[0] = reg_addr;
    if (data != NULL && len > 0) {
        memcpy(&write_buf[1], data, len);
    }
    return i2c_master_transmit(dev_handle, write_buf, 1 + len, -1);
}

/* ======================== Baca Register I2C ======================== */
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                               uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, -1);
}

/* ======================== Interpretasi Error I2C ======================== */
static const char* i2c_error_str(esp_err_t err)
{
    switch (err) {
        case ESP_OK:              return "OK (ACK diterima)";
        case ESP_ERR_INVALID_ARG: return "Parameter tidak valid";
        case ESP_FAIL:            return "NACK - perangkat tidak merespons";
        case ESP_ERR_INVALID_STATE: return "Driver belum diinstall";
        case ESP_ERR_TIMEOUT:     return "Timeout - bus mungkin sibuk";
        default:                  return "Error tidak dikenal";
    }
}

/* ======================== Demonstrasi Protokol I2C Raw ======================== */
static void i2c_raw_write_read(i2c_master_dev_handle_t dev_handle, uint8_t dev_addr,
                                uint8_t reg_addr, uint8_t *read_buf, size_t read_len)
{
    esp_err_t err;
    int64_t start_time, end_time;

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "DEMONSTRASI PROTOKOL I2C RAW");
    ESP_LOGI(TAG, "Target: 0x%02X, Register: 0x%02X, Baca: %d byte",
             dev_addr, reg_addr, (int)read_len);
    ESP_LOGI(TAG, "============================================");

    start_time = esp_timer_get_time();

    /*
     * === FASE 1: WRITE (kirim alamat register) ===
     * Urutan: START -> ADDR+W -> REG_ADDR
     */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- FASE 1: WRITE PHASE ---");

    /* Step 1: START condition */
    ESP_LOGI(TAG, "[1] START condition");
    ESP_LOGI(TAG, "    SDA: HIGH -> LOW (saat SCL HIGH)");

    /* Step 2: Kirim alamat perangkat + bit WRITE (0) */
    uint8_t addr_write = (dev_addr << 1) | 0x00;
    ESP_LOGI(TAG, "[2] ALAMAT + WRITE bit");
    ESP_LOGI(TAG, "    Byte: 0x%02X = [Addr:0x%02X | W:0]", addr_write, dev_addr);
    ESP_LOGI(TAG, "    Biner: " );
    for (int i = 7; i >= 0; i--) {
        printf("%d", (addr_write >> i) & 1);
        if (i == 1) printf("|"); /* Pisahkan R/W bit */
    }
    printf("\n");
    ESP_LOGI(TAG, "    Menunggu ACK dari slave...");

    /* Step 3: Kirim alamat register yang ingin dibaca */
    ESP_LOGI(TAG, "[3] REGISTER ADDRESS");
    ESP_LOGI(TAG, "    Byte: 0x%02X (register yang akan dibaca)", reg_addr);
    ESP_LOGI(TAG, "    Menunggu ACK dari slave...");

    /*
     * === FASE 2: READ (baca data dari register) ===
     * Urutan: REPEATED START -> ADDR+R -> DATA... -> NACK -> STOP
     */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- FASE 2: READ PHASE ---");

    /* Step 4: REPEATED START condition */
    ESP_LOGI(TAG, "[4] REPEATED START condition");
    ESP_LOGI(TAG, "    SDA: HIGH -> LOW (saat SCL HIGH), tanpa STOP sebelumnya");

    /* Step 5: Kirim alamat perangkat + bit READ (1) */
    uint8_t addr_read = (dev_addr << 1) | 0x01;
    ESP_LOGI(TAG, "[5] ALAMAT + READ bit");
    ESP_LOGI(TAG, "    Byte: 0x%02X = [Addr:0x%02X | R:1]", addr_read, dev_addr);
    ESP_LOGI(TAG, "    Menunggu ACK dari slave...");

    /* Step 6: Baca data dari slave */
    if (read_len > 1) {
        ESP_LOGI(TAG, "[6] BACA %d byte data (dengan ACK antar-byte)", (int)(read_len - 1));
        ESP_LOGI(TAG, "    Master mengirim ACK setelah setiap byte (minta byte lagi)");
    }

    /* Step 7: Baca byte terakhir dengan NACK */
    ESP_LOGI(TAG, "[7] BACA byte terakhir + NACK");
    ESP_LOGI(TAG, "    Master mengirim NACK (tidak minta byte lagi)");

    /* Step 8: STOP condition */
    ESP_LOGI(TAG, "[8] STOP condition");
    ESP_LOGI(TAG, "    SDA: LOW -> HIGH (saat SCL HIGH)");

    /*
     * === FASE 3: EKSEKUSI ===
     * Menggunakan new API: i2c_master_transmit_receive() menggabungkan
     * WRITE (register addr) + REPEATED START + READ dalam satu panggilan.
     */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- FASE 3: EKSEKUSI TRANSAKSI ---");
    ESP_LOGI(TAG, "Mengirim seluruh urutan perintah ke hardware I2C...");

    err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, read_buf, read_len, -1);
    end_time = esp_timer_get_time();

    /*
     * === FASE 4: HASIL ===
     */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- FASE 4: HASIL TRANSAKSI ---");
    ESP_LOGI(TAG, "Status: %s", i2c_error_str(err));
    ESP_LOGI(TAG, "Waktu eksekusi: %lld us", (long long)(end_time - start_time));

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Data yang dibaca (%d byte):", (int)read_len);
        printf("DATA,0x%02X,0x%02X,OK,%lld,",
               dev_addr, reg_addr, (long long)(end_time - start_time));
        for (size_t i = 0; i < read_len; i++) {
            ESP_LOGI(TAG, "  Byte[%d]: 0x%02X (desimal: %d, biner: %d%d%d%d%d%d%d%d)",
                     (int)i, read_buf[i], read_buf[i],
                     (read_buf[i] >> 7) & 1, (read_buf[i] >> 6) & 1,
                     (read_buf[i] >> 5) & 1, (read_buf[i] >> 4) & 1,
                     (read_buf[i] >> 3) & 1, (read_buf[i] >> 2) & 1,
                     (read_buf[i] >> 1) & 1, (read_buf[i] >> 0) & 1);
            printf("0x%02X", read_buf[i]);
            if (i < read_len - 1) printf(" ");
        }
        printf("\n");
    } else {
        ESP_LOGE(TAG, "Transaksi GAGAL: %s", esp_err_to_name(err));
        printf("DATA,0x%02X,0x%02X,FAIL,%lld,N/A\n",
               dev_addr, reg_addr, (long long)(end_time - start_time));
    }

    ESP_LOGI(TAG, "============================================");
}

/* ======================== Demonstrasi Write-Only ======================== */
static void i2c_raw_write_only(i2c_master_dev_handle_t dev_handle, uint8_t dev_addr,
                                uint8_t reg_addr, uint8_t *write_data, size_t write_len)
{
    esp_err_t err;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== DEMONSTRASI WRITE-ONLY ===");
    ESP_LOGI(TAG, "Target: 0x%02X, Register: 0x%02X", dev_addr, reg_addr);

    uint8_t addr_w = (dev_addr << 1) | 0x00;
    ESP_LOGI(TAG, "[1] START");
    ESP_LOGI(TAG, "[2] ADDR+W: 0x%02X -> menunggu ACK", addr_w);
    ESP_LOGI(TAG, "[3] REG: 0x%02X -> menunggu ACK", reg_addr);

    for (size_t i = 0; i < write_len; i++) {
        ESP_LOGI(TAG, "[%d] DATA: 0x%02X -> menunggu ACK", (int)(4 + i), write_data[i]);
    }
    ESP_LOGI(TAG, "[%d] STOP", (int)(4 + write_len));

    /* Gabungkan reg_addr + data menjadi satu buffer dan kirim */
    uint8_t write_buf[1 + write_len];
    write_buf[0] = reg_addr;
    memcpy(&write_buf[1], write_data, write_len);

    err = i2c_master_transmit(dev_handle, write_buf, 1 + write_len, -1);

    ESP_LOGI(TAG, "Hasil Write: %s", i2c_error_str(err));
    printf("WRITE,0x%02X,0x%02X,%s\n", dev_addr, reg_addr,
           err == ESP_OK ? "OK" : "FAIL");
}

/* ======================== Scan I2C Bus Verbose ======================== */
static void i2c_scan_verbose(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "===== SCAN I2C BUS (VERBOSE) =====");
    ESP_LOGI(TAG, "Mengirim START + ADDR+W ke setiap alamat 0x01-0x7E");
    ESP_LOGI(TAG, "ACK = perangkat ada, NACK = tidak ada perangkat");
    ESP_LOGI(TAG, "");

    uint8_t found = 0;
    int64_t scan_start = esp_timer_get_time();

    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
    for (uint8_t row = 0; row < 8; row++) {
        printf("%02X: ", row * 16);
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = row * 16 + col;
            if (addr < 0x03 || addr > 0x77) {
                printf("   ");
                continue;
            }

            esp_err_t err = i2c_master_probe(bus_handle, addr, -1);

            if (err == ESP_OK) {
                printf("%02X ", addr);
                found++;
                ESP_LOGI(TAG, "  -> ACK diterima dari 0x%02X", addr);
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }

    int64_t scan_end = esp_timer_get_time();
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Total perangkat ditemukan: %d", found);
    ESP_LOGI(TAG, "Waktu scan: %lld ms", (long long)(scan_end - scan_start) / 1000);
    printf("SCAN,found,%d,time_ms,%lld\n", found, (long long)(scan_end - scan_start) / 1000);
}

/* ======================== Demonstrasi Multi-Byte Read ======================== */
static void i2c_raw_multi_read(i2c_master_dev_handle_t dev_handle, uint8_t dev_addr,
                                uint8_t start_reg, size_t count)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== MULTI-BYTE READ ===");
    ESP_LOGI(TAG, "Alamat: 0x%02X, Mulai register: 0x%02X, Jumlah: %d byte",
             dev_addr, start_reg, (int)count);

    uint8_t data[32];
    if (count > sizeof(data)) count = sizeof(data);

    esp_err_t err = i2c_read_reg(dev_handle, start_reg, data, count);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Berhasil membaca %d byte:", (int)count);
        printf("MULTI_READ,0x%02X,0x%02X,%d,OK,", dev_addr, start_reg, (int)count);
        for (size_t i = 0; i < count; i++) {
            ESP_LOGI(TAG, "  Reg[0x%02X] = 0x%02X (%d)",
                     (uint8_t)(start_reg + i), data[i], data[i]);
            printf("0x%02X", data[i]);
            if (i < count - 1) printf(" ");
        }
        printf("\n");
    } else {
        ESP_LOGE(TAG, "Gagal membaca: %s", i2c_error_str(err));
        printf("MULTI_READ,0x%02X,0x%02X,%d,FAIL\n", dev_addr, start_reg, (int)count);
    }
}

/* ======================== Task Utama ======================== */
static void raw_i2c_task(void *pvParameters)
{
    uint32_t cycle = 0;

    while (1) {
        cycle++;
        ESP_LOGI(TAG, "\n\n>>>>>>> SIKLUS #%lu <<<<<<<", (unsigned long)cycle);

        /* 1. Scan bus I2C */
        if (cycle == 1) {
            i2c_scan_verbose();
        }

        /* 2. Demonstrasi write-read raw (baca 1 register) */
        uint8_t read_data[READ_LENGTH] = {0};
        i2c_raw_write_read(target_dev_handle, TARGET_ADDR, TARGET_REG,
                           read_data, READ_LENGTH);

        /* 3. Demonstrasi multi-byte read */
        i2c_raw_multi_read(target_dev_handle, TARGET_ADDR, 0x88, 6);

        /* 4. Coba alamat alternatif */
        uint8_t alt_data[1] = {0};
        ESP_LOGI(TAG, "\n--- Coba alamat alternatif ---");
        i2c_raw_write_read(alt1_dev_handle, ALT_ADDR_1, 0x00, alt_data, 1);

        /* Tunggu 5 detik sebelum siklus berikutnya */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ======================== Fungsi Utama ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 I2C Write/Read Raw Protocol Demo ===");
    ESP_LOGI(TAG, "SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
    ESP_LOGI(TAG, "Target: 0x%02X, Register: 0x%02X", TARGET_ADDR, TARGET_REG);

    /* Inisialisasi I2C master */
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C master berhasil diinisialisasi");

    printf("HDR,addr,reg,status,time_us,data\n");

    /* Buat task demonstrasi */
    xTaskCreate(raw_i2c_task, "raw_i2c_task", 4096, NULL, 5, NULL);
}
