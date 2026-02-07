/**
 * ============================================================================
 * Program     : ESP32_07_SD_Card_Write_Log
 * Deskripsi   : Menulis log data sensor (simulasi) ke SD Card via SPI
 * Platform    : ESP32 (ESP-IDF / PlatformIO)
 * ============================================================================
 *
 * Kebutuhan Hardware:
 *   - Board ESP32 (DevKit V1 atau sejenisnya)
 *   - Modul Micro SD Card (dengan antarmuka SPI)
 *   - Micro SD Card (FAT16/FAT32 terformat)
 *   - Kabel jumper secukupnya
 *
 * Koneksi Pin:
 *   ESP32 GPIO23 (MOSI) --> MOSI/DI  pada modul SD
 *   ESP32 GPIO19 (MISO) --> MISO/DO  pada modul SD
 *   ESP32 GPIO18 (SCLK) --> SCK/CLK  pada modul SD
 *   ESP32 GPIO5  (CS)   --> CS/SS    pada modul SD
 *   ESP32 3.3V          --> VCC      pada modul SD
 *   ESP32 GND           --> GND      pada modul SD
 *
 * Fitur Utama:
 *   1. Mount SD Card via SPI
 *   2. Menulis entri log bertimestamp ke file
 *   3. Mode append (tidak menimpa data lama)
 *   4. Log data sensor simulasi (suhu & kelembapan acak)
 *   5. Menulis beberapa entri log, lalu membaca kembali
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"           // Untuk esp_timer_get_time()
#include "esp_random.h"          // Untuk pembangkit angka acak hardware
#include "esp_vfs_fat.h"        // API VFS FAT filesystem
#include "driver/sdspi_host.h"   // Driver SPI host untuk SD Card
#include "driver/spi_common.h"   // Konfigurasi SPI umum
#include "sdmmc_cmd.h"           // Perintah-perintah SD/MMC
#include "config.h"

// Tag untuk logging ESP-IDF
static const char *TAG = "SD_WRITE_LOG";

/**
 * Fungsi untuk menginisialisasi dan mount SD Card via SPI
 * Mengembalikan pointer ke sdmmc_card_t jika berhasil, NULL jika gagal
 */
static sdmmc_card_t *mount_sd_card(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = MAX_FILES,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;

    ESP_LOGI(TAG, "Menginisialisasi bus SPI...");

    // Konfigurasi bus SPI
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi bus SPI: %s", esp_err_to_name(ret));
        return NULL;
    }

    // Konfigurasi device SPI untuk SD Card
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SD_SPI_HOST;

    // Konfigurasi host SD
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    host.max_freq_khz = SPI_FREQUENCY;

    // Mount filesystem
    ESP_LOGI(TAG, "Melakukan mounting SD Card ke '%s'...", MOUNT_POINT);
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config,
                                   &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal mount SD Card: %s", esp_err_to_name(ret));
        spi_bus_free(SD_SPI_HOST);
        return NULL;
    }

    ESP_LOGI(TAG, "SD Card berhasil di-mount!");
    return card;
}

/**
 * Fungsi untuk menghasilkan nilai float acak dalam rentang tertentu
 * Menggunakan hardware random number generator ESP32
 */
static float random_float(float min, float max)
{
    // esp_random() menghasilkan bilangan acak 32-bit dari hardware RNG
    uint32_t random_val = esp_random();
    // Normalisasi ke rentang [0.0, 1.0] lalu skalakan ke [min, max]
    float normalized = (float)random_val / (float)UINT32_MAX;
    return min + (normalized * (max - min));
}

/**
 * Fungsi untuk mendapatkan string timestamp berdasarkan uptime ESP32
 * Format: "HH:MM:SS" berdasarkan waktu sejak boot
 */
static void get_timestamp(char *buf, size_t buf_len, int entry_num)
{
    // Gunakan waktu uptime ESP32 (sejak boot)
    int64_t uptime_ms = esp_timer_get_time() / 1000;  // Konversi dari us ke ms
    int detik = (int)(uptime_ms / 1000) % 60;
    int menit = (int)(uptime_ms / 60000) % 60;
    int jam   = (int)(uptime_ms / 3600000) % 24;

    snprintf(buf, buf_len, "%02d:%02d:%02d", jam, menit, detik);
}

/**
 * Fungsi untuk menulis satu entri log ke file
 * Menggunakan mode append agar tidak menimpa data lama
 */
static esp_err_t tulis_entri_log(const char *path, int nomor_entri)
{
    // Buka file dalam mode append ("a") - data ditambahkan di akhir file
    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuka file untuk menulis: %s", path);
        return ESP_FAIL;
    }

    // Dapatkan timestamp
    char timestamp[16];
    get_timestamp(timestamp, sizeof(timestamp), nomor_entri);

    // Simulasikan pembacaan sensor (nilai acak)
    float suhu = random_float(TEMP_MIN, TEMP_MAX);
    float kelembapan = random_float(HUMI_MIN, HUMI_MAX);

    // Tulis entri log dengan format terstruktur
    fprintf(f, "[%s] Entri #%03d | Suhu: %.1f°C | Kelembapan: %.1f%% | Status: OK\n",
            timestamp, nomor_entri, suhu, kelembapan);

    fclose(f);

    // Tampilkan juga di serial monitor
    ESP_LOGI(TAG, "Log #%03d: Waktu=%s, Suhu=%.1f°C, Kelembapan=%.1f%%",
             nomor_entri, timestamp, suhu, kelembapan);

    return ESP_OK;
}

/**
 * Fungsi untuk menulis header log jika file baru
 * Header berisi informasi sesi logging
 */
static void tulis_header_log(const char *path)
{
    // Cek apakah file sudah ada
    struct stat st;
    bool file_baru = (stat(path, &st) != 0);

    // Buka file dalam mode append
    FILE *f = fopen(path, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuat file log: %s", path);
        return;
    }

    if (file_baru) {
        // Tulis header untuk file baru
        fprintf(f, "============================================\n");
        fprintf(f, "  LOG DATA SENSOR - ESP32\n");
        fprintf(f, "  Praktikum Sistem Embedded\n");
        fprintf(f, "  Modul 07: SPI & Storage\n");
        fprintf(f, "============================================\n");
        fprintf(f, "Format: [Waktu] Entri# | Suhu | Kelembapan | Status\n");
        fprintf(f, "--------------------------------------------\n");
        ESP_LOGI(TAG, "File log baru dibuat dengan header");
    } else {
        // Tambahkan separator untuk sesi baru
        fprintf(f, "\n--- Sesi Logging Baru ---\n");
        ESP_LOGI(TAG, "Melanjutkan penulisan ke file log yang sudah ada (ukuran: %ld byte)",
                 (long)st.st_size);
    }

    fclose(f);
}

/**
 * Fungsi untuk membaca dan menampilkan seluruh isi file log
 */
static void baca_file_log(const char *path)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  MEMBACA KEMBALI FILE LOG");
    ESP_LOGI(TAG, "========================================");

    // Cek keberadaan dan ukuran file
    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGW(TAG, "File log tidak ditemukan: %s", path);
        return;
    }
    ESP_LOGI(TAG, "File: %s (Ukuran: %ld byte)", path, (long)st.st_size);

    // Buka dan baca file
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuka file log untuk dibaca");
        return;
    }

    char buf[256];
    int nomor_baris = 0;

    ESP_LOGI(TAG, "--- Isi File Log ---");
    while (fgets(buf, sizeof(buf), f) != NULL) {
        nomor_baris++;
        // Hapus newline
        buf[strcspn(buf, "\r\n")] = 0;
        ESP_LOGI(TAG, "%s", buf);
    }
    ESP_LOGI(TAG, "--- Akhir File Log ---");
    ESP_LOGI(TAG, "Total baris: %d", nomor_baris);

    fclose(f);
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  Program ESP32_07_SD_Card_Write_Log");
    ESP_LOGI(TAG, "  Menulis Log Data Sensor ke SD Card");
    ESP_LOGI(TAG, "============================================");

    // ========================================================================
    // Langkah 1: Mount SD Card
    // ========================================================================
    sdmmc_card_t *card = mount_sd_card();
    if (card == NULL) {
        ESP_LOGE(TAG, "Gagal mount SD Card. Program berhenti.");
        return;
    }

    // Tampilkan info kartu singkat
    ESP_LOGI(TAG, "Kartu SD: %s", card->cid.name);

    // ========================================================================
    // Langkah 2: Tulis header log
    // ========================================================================
    tulis_header_log(LOG_FILE_NAME);

    // ========================================================================
    // Langkah 3: Tulis beberapa entri log dengan interval
    // ========================================================================
    ESP_LOGI(TAG, "Mulai menulis %d entri log...", LOG_ENTRIES);
    ESP_LOGI(TAG, "Interval antar entri: %d ms", LOG_INTERVAL_MS);

    int berhasil = 0;
    int gagal = 0;

    for (int i = 1; i <= LOG_ENTRIES; i++) {
        esp_err_t ret = tulis_entri_log(LOG_FILE_NAME, i);
        if (ret == ESP_OK) {
            berhasil++;
        } else {
            gagal++;
            ESP_LOGE(TAG, "Gagal menulis entri log #%d", i);
        }

        // Tunggu interval sebelum entri berikutnya
        if (i < LOG_ENTRIES) {
            vTaskDelay(pdMS_TO_TICKS(LOG_INTERVAL_MS));
        }
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  RINGKASAN PENULISAN LOG");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Total entri   : %d", LOG_ENTRIES);
    ESP_LOGI(TAG, "Berhasil      : %d", berhasil);
    ESP_LOGI(TAG, "Gagal         : %d", gagal);

    // Tampilkan ukuran file akhir
    struct stat st;
    if (stat(LOG_FILE_NAME, &st) == 0) {
        ESP_LOGI(TAG, "Ukuran file   : %ld byte", (long)st.st_size);
    }
    ESP_LOGI(TAG, "========================================");

    // ========================================================================
    // Langkah 4: Baca kembali seluruh file log
    // ========================================================================
    vTaskDelay(pdMS_TO_TICKS(500));  // Tunggu sebentar sebelum baca
    baca_file_log(LOG_FILE_NAME);

    // ========================================================================
    // Langkah 5: Unmount SD Card
    // ========================================================================
    ESP_LOGI(TAG, "Melakukan unmount SD Card...");
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(SD_SPI_HOST);
    ESP_LOGI(TAG, "SD Card berhasil di-unmount");

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  Program selesai.");
    ESP_LOGI(TAG, "============================================");

    // Loop utama - program sudah selesai
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
