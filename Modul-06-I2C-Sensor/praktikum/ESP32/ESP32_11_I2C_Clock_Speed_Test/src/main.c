/*
 * ESP32_11_I2C_Clock_Speed_Test
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Menguji kecepatan I2C pada 100kHz dan 400kHz.
 *            Mengukur throughput transfer data untuk perbandingan.
 *            Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h).
 *            Kecepatan diatur per-device, jadi ganti speed dengan
 *            i2c_master_bus_rm_device() + i2c_master_bus_add_device().
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 *   Perangkat: EEPROM AT24C32 (0x50) atau sensor lainnya
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

static const char *TAG = "I2C_SPEED";

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

/* ======================== Konfigurasi Perangkat ======================== */
#define TARGET_ADDR         0x50    /* Alamat EEPROM AT24C32 */
#define TEST_REGISTER       0x00    /* Register awal untuk test baca */
#define BULK_READ_SIZE      32      /* Ukuran bulk read per transaksi (max page EEPROM) */
#define NUM_ITERATIONS      100     /* Jumlah iterasi pengujian */

/* Kecepatan I2C yang akan diuji */
static const uint32_t test_speeds[] = {100000, 400000};
static const char *speed_names[] = {"100 kHz (Standard)", "400 kHz (Fast)"};
#define NUM_SPEEDS          (sizeof(test_speeds) / sizeof(test_speeds[0]))

/* Handle I2C bus dan device */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

/* ======================== Hasil Pengujian ======================== */
typedef struct {
    uint32_t speed_hz;          /* Kecepatan clock I2C */
    uint32_t total_bytes;       /* Total byte yang ditransfer */
    int64_t  total_time_us;     /* Total waktu dalam mikrodetik */
    float    throughput_bps;    /* Throughput dalam byte per detik */
    float    avg_latency_us;    /* Rata-rata latency per transaksi */
    uint32_t success_count;     /* Jumlah transaksi berhasil */
    uint32_t fail_count;        /* Jumlah transaksi gagal */
} speed_test_result_t;

/* ======================== Inisialisasi I2C Master Bus ======================== */
static esp_err_t i2c_bus_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&bus_config, &bus_handle);
}

/* ======================== Tambah Device dengan Kecepatan Tertentu ======================== */
static esp_err_t i2c_add_device(uint32_t freq_hz)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TARGET_ADDR,
        .scl_speed_hz = freq_hz,
    };

    return i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
}

/* ======================== Baca Register I2C ======================== */
static esp_err_t i2c_read_reg(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, -1);
}

/* ======================== Ganti Kecepatan I2C ======================== */
static esp_err_t i2c_change_speed(uint32_t new_freq_hz)
{
    ESP_LOGI(TAG, "Menghapus device I2C lama...");

    /* Hapus device lama (kecepatan diatur per-device pada API baru) */
    esp_err_t err = i2c_master_bus_rm_device(dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal hapus device: %s", esp_err_to_name(err));
        return err;
    }

    /* Tunggu sebentar sebelum menambah device baru */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Menambah device baru pada %lu Hz...", (unsigned long)new_freq_hz);
    err = i2c_add_device(new_freq_hz);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambah device baru: %s", esp_err_to_name(err));
        return err;
    }

    /* Tunggu stabilisasi */
    vTaskDelay(pdMS_TO_TICKS(50));
    return ESP_OK;
}

/* ======================== Uji Throughput ======================== */
static void run_speed_test(uint32_t speed_hz, speed_test_result_t *result)
{
    uint8_t read_buf[BULK_READ_SIZE];
    int64_t start_time, end_time, iter_start, iter_end;
    int64_t total_latency = 0;

    /* Inisialisasi hasil */
    memset(result, 0, sizeof(speed_test_result_t));
    result->speed_hz = speed_hz;

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "PENGUJIAN KECEPATAN: %lu Hz", (unsigned long)speed_hz);
    ESP_LOGI(TAG, "Iterasi: %d, Byte per read: %d", NUM_ITERATIONS, BULK_READ_SIZE);
    ESP_LOGI(TAG, "========================================");

    /* Verifikasi perangkat tersedia */
    esp_err_t err = i2c_read_reg(TEST_REGISTER, read_buf, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Perangkat 0x%02X tidak merespons, menggunakan dummy read", TARGET_ADDR);
    }

    /* Mulai pengujian */
    start_time = esp_timer_get_time();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        iter_start = esp_timer_get_time();

        /* Baca BULK_READ_SIZE byte dari perangkat */
        uint8_t reg = (uint8_t)((TEST_REGISTER + (i * BULK_READ_SIZE)) & 0xFF);
        err = i2c_read_reg(reg, read_buf, BULK_READ_SIZE);

        iter_end = esp_timer_get_time();

        if (err == ESP_OK) {
            result->success_count++;
            result->total_bytes += BULK_READ_SIZE;
            total_latency += (iter_end - iter_start);
        } else {
            result->fail_count++;
        }

        /* Progress setiap 25% */
        if ((i + 1) % (NUM_ITERATIONS / 4) == 0) {
            ESP_LOGI(TAG, "  Progress: %d/%d (%d%%) - OK:%lu FAIL:%lu",
                     i + 1, NUM_ITERATIONS, ((i + 1) * 100) / NUM_ITERATIONS,
                     (unsigned long)result->success_count,
                     (unsigned long)result->fail_count);
        }
    }

    end_time = esp_timer_get_time();
    result->total_time_us = end_time - start_time;

    /* Hitung throughput dan latency */
    if (result->total_time_us > 0) {
        result->throughput_bps = (float)result->total_bytes * 1000000.0f /
                                 (float)result->total_time_us;
    }
    if (result->success_count > 0) {
        result->avg_latency_us = (float)total_latency / (float)result->success_count;
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Hasil kecepatan %lu Hz:", (unsigned long)speed_hz);
    ESP_LOGI(TAG, "  Total byte     : %lu", (unsigned long)result->total_bytes);
    ESP_LOGI(TAG, "  Total waktu    : %lld us (%.2f ms)",
             (long long)result->total_time_us,
             (float)result->total_time_us / 1000.0f);
    ESP_LOGI(TAG, "  Throughput     : %.2f byte/s (%.2f KB/s)",
             result->throughput_bps, result->throughput_bps / 1024.0f);
    ESP_LOGI(TAG, "  Latency rata2  : %.2f us", result->avg_latency_us);
    ESP_LOGI(TAG, "  Berhasil/Gagal : %lu/%lu",
             (unsigned long)result->success_count,
             (unsigned long)result->fail_count);
}

/* ======================== Cetak Tabel Perbandingan ======================== */
static void print_comparison_table(speed_test_result_t *results, size_t count)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║              TABEL PERBANDINGAN KECEPATAN I2C                       ║");
    ESP_LOGI(TAG, "╠══════════════╦═══════════╦═══════════╦══════════╦════════╦═══════════╣");
    ESP_LOGI(TAG, "║ Kecepatan    ║ Byte      ║ Waktu(ms) ║ KB/s     ║ OK     ║ Lat.(us)  ║");
    ESP_LOGI(TAG, "╠══════════════╬═══════════╬═══════════╬══════════╬════════╬═══════════╣");

    for (size_t i = 0; i < count; i++) {
        ESP_LOGI(TAG, "║ %6lu Hz   ║ %7lu   ║ %7.1f   ║ %6.2f   ║ %4lu   ║ %7.1f   ║",
                 (unsigned long)results[i].speed_hz,
                 (unsigned long)results[i].total_bytes,
                 (float)results[i].total_time_us / 1000.0f,
                 results[i].throughput_bps / 1024.0f,
                 (unsigned long)results[i].success_count,
                 results[i].avg_latency_us);

        /* Output CSV */
        printf("RESULT,%lu,%lu,%lld,%.2f,%.2f,%lu,%lu\n",
               (unsigned long)results[i].speed_hz,
               (unsigned long)results[i].total_bytes,
               (long long)results[i].total_time_us,
               results[i].throughput_bps,
               results[i].avg_latency_us,
               (unsigned long)results[i].success_count,
               (unsigned long)results[i].fail_count);
    }

    ESP_LOGI(TAG, "╚══════════════╩═══════════╩═══════════╩══════════╩════════╩═══════════╝");

    /* Hitung rasio percepatan */
    if (count >= 2 && results[0].throughput_bps > 0) {
        float speedup = results[1].throughput_bps / results[0].throughput_bps;
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Percepatan 400kHz vs 100kHz: %.2fx", speedup);
        ESP_LOGI(TAG, "Percepatan teoritis: 4.00x");
        ESP_LOGI(TAG, "Efisiensi: %.1f%%", (speedup / 4.0f) * 100.0f);
        printf("SPEEDUP,%.2f,%.1f\n", speedup, (speedup / 4.0f) * 100.0f);
    }
}

/* ======================== Task Pengujian ======================== */
static void speed_test_task(void *pvParameters)
{
    speed_test_result_t results[NUM_SPEEDS];

    while (1) {
        ESP_LOGI(TAG, "\n\n====== MEMULAI PENGUJIAN KECEPATAN I2C ======");
        ESP_LOGI(TAG, "Perangkat target: 0x%02X", TARGET_ADDR);
        ESP_LOGI(TAG, "Jumlah kecepatan: %d", (int)NUM_SPEEDS);

        for (size_t i = 0; i < NUM_SPEEDS; i++) {
            ESP_LOGI(TAG, "\n--- Test %d/%d: %s ---",
                     (int)(i + 1), (int)NUM_SPEEDS, speed_names[i]);

            /* Ganti kecepatan I2C (hapus device lama, tambah device baru) */
            esp_err_t err = i2c_change_speed(test_speeds[i]);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Gagal ganti kecepatan ke %lu Hz!",
                         (unsigned long)test_speeds[i]);
                memset(&results[i], 0, sizeof(speed_test_result_t));
                results[i].speed_hz = test_speeds[i];
                continue;
            }

            /* Jalankan pengujian throughput */
            run_speed_test(test_speeds[i], &results[i]);

            vTaskDelay(pdMS_TO_TICKS(500));
        }

        /* Cetak tabel perbandingan */
        print_comparison_table(results, NUM_SPEEDS);

        /* Kembalikan ke kecepatan standar 100kHz */
        i2c_change_speed(100000);

        ESP_LOGI(TAG, "\n====== PENGUJIAN SELESAI ======");
        ESP_LOGI(TAG, "Mengulangi dalam 30 detik...\n");

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/* ======================== Fungsi Utama ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 I2C Clock Speed Test ===");
    ESP_LOGI(TAG, "SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
    ESP_LOGI(TAG, "Target: 0x%02X, Bulk size: %d byte", TARGET_ADDR, BULK_READ_SIZE);

    /* Inisialisasi I2C master bus */
    ESP_ERROR_CHECK(i2c_bus_init());
    ESP_LOGI(TAG, "I2C master bus berhasil diinisialisasi");

    /* Tambah device awal dengan kecepatan standar 100kHz */
    ESP_ERROR_CHECK(i2c_add_device(100000));
    ESP_LOGI(TAG, "Device 0x%02X ditambahkan (100 kHz)", TARGET_ADDR);

    printf("HDR,speed_hz,total_bytes,total_time_us,throughput_bps,avg_latency_us,success,fail\n");

    /* Buat task pengujian */
    xTaskCreate(speed_test_task, "speed_test", 4096, NULL, 5, NULL);
}
