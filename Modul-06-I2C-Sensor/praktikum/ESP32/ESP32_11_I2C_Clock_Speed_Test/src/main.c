/*
 * ESP32_11_I2C_Clock_Speed_Test
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Menguji kecepatan I2C pada 100kHz dan 400kHz.
 *            Mengukur throughput transfer data untuk perbandingan.
 *            Menggunakan i2c_driver_delete() dan re-install untuk ganti kecepatan.
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
#include "driver/i2c.h"
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
#define I2C_TIMEOUT_MS      1000

/* ======================== Konfigurasi Perangkat ======================== */
#define TARGET_ADDR         0x50    /* Alamat EEPROM AT24C32 */
#define TEST_REGISTER       0x00    /* Register awal untuk test baca */
#define BULK_READ_SIZE      32      /* Ukuran bulk read per transaksi (max page EEPROM) */
#define NUM_ITERATIONS      100     /* Jumlah iterasi pengujian */

/* Kecepatan I2C yang akan diuji */
static const uint32_t test_speeds[] = {100000, 400000};
static const char *speed_names[] = {"100 kHz (Standard)", "400 kHz (Fast)"};
#define NUM_SPEEDS          (sizeof(test_speeds) / sizeof(test_speeds[0]))

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

/* ======================== Inisialisasi I2C Master ======================== */
static esp_err_t i2c_master_init(uint32_t freq_hz)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freq_hz,
    };

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal konfigurasi I2C: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal install driver I2C: %s", esp_err_to_name(err));
    }
    return err;
}

/* ======================== Tulis Register I2C ======================== */
static esp_err_t i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    if (data != NULL && len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

/* ======================== Baca Register I2C ======================== */
static esp_err_t i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

/* ======================== Ganti Kecepatan I2C ======================== */
static esp_err_t i2c_change_speed(uint32_t new_freq_hz)
{
    ESP_LOGI(TAG, "Menghapus driver I2C lama...");
    esp_err_t err = i2c_driver_delete(I2C_PORT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal hapus driver: %s", esp_err_to_name(err));
        return err;
    }

    /* Tunggu sebentar sebelum inisialisasi ulang */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "Inisialisasi ulang I2C pada %lu Hz...", (unsigned long)new_freq_hz);
    err = i2c_master_init(new_freq_hz);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi I2C baru: %s", esp_err_to_name(err));
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
    esp_err_t err = i2c_read_reg(TARGET_ADDR, TEST_REGISTER, read_buf, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Perangkat 0x%02X tidak merespons, menggunakan dummy read", TARGET_ADDR);
    }

    /* Mulai pengujian */
    start_time = esp_timer_get_time();

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        iter_start = esp_timer_get_time();

        /* Baca BULK_READ_SIZE byte dari perangkat */
        uint8_t reg = (uint8_t)((TEST_REGISTER + (i * BULK_READ_SIZE)) & 0xFF);
        err = i2c_read_reg(TARGET_ADDR, reg, read_buf, BULK_READ_SIZE);

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

            /* Ganti kecepatan I2C */
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

    /* Inisialisasi I2C master dengan kecepatan standar */
    ESP_ERROR_CHECK(i2c_master_init(100000));
    ESP_LOGI(TAG, "I2C master berhasil diinisialisasi (100 kHz)");

    printf("HDR,speed_hz,total_bytes,total_time_us,throughput_bps,avg_latency_us,success,fail\n");

    /* Buat task pengujian */
    xTaskCreate(speed_test_task, "speed_test", 4096, NULL, 5, NULL);
}
