/**
 * ============================================================================
 * Program     : ESP32_06_SD_Card_Read
 * Deskripsi   : Membaca file dari SD Card via SPI pada ESP32
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
 *   2. Membuat file tes jika belum ada
 *   3. Membaca isi file dan menampilkan di serial monitor
 *   4. Menangani error file tidak ditemukan secara graceful
 *   5. Menampilkan ukuran file dan konten
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"        // API VFS FAT filesystem
#include "driver/sdspi_host.h"   // Driver SPI host untuk SD Card
#include "driver/spi_common.h"   // Konfigurasi SPI umum
#include "sdmmc_cmd.h"           // Perintah-perintah SD/MMC
#include "config.h"

// Tag untuk logging ESP-IDF
static const char *TAG = "SD_READ";

/**
 * Fungsi untuk menginisialisasi dan mount SD Card via SPI
 * Mengembalikan pointer ke sdmmc_card_t jika berhasil, NULL jika gagal
 */
static sdmmc_card_t *mount_sd_card(void)
{
    // Konfigurasi opsi mount filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,    // Jangan format jika mount gagal
        .max_files = MAX_FILES,             // Maksimal file terbuka
        .allocation_unit_size = 16 * 1024   // Unit alokasi 16 KB
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
 * Fungsi untuk membuat file tes dengan konten contoh
 * Hanya membuat file jika belum ada
 */
static void buat_file_tes_jika_belum_ada(const char *path)
{
    // Cek apakah file sudah ada menggunakan stat()
    struct stat st;
    if (stat(path, &st) == 0) {
        ESP_LOGI(TAG, "File '%s' sudah ada (ukuran: %ld byte)", path, (long)st.st_size);
        ESP_LOGI(TAG, "Tidak perlu membuat file baru.");
        return;
    }

    // File belum ada, buat file baru dengan konten contoh
    ESP_LOGI(TAG, "File '%s' belum ada, membuat file baru...", path);

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuat file: %s", path);
        return;
    }

    // Tulis konten contoh ke file
    fprintf(f, "=== Data Pengujian SD Card ESP32 ===\n");
    fprintf(f, "Baris 1: Halo dari ESP32!\n");
    fprintf(f, "Baris 2: Praktikum Sistem Embedded\n");
    fprintf(f, "Baris 3: Modul 07 - SPI & Storage\n");
    fprintf(f, "Baris 4: Membaca file dari SD Card\n");
    fprintf(f, "Baris 5: Suhu=25.5C Kelembapan=60%%\n");
    fprintf(f, "Baris 6: Data sensor berhasil disimpan\n");
    fprintf(f, "=== Akhir Data Pengujian ===\n");

    fclose(f);
    ESP_LOGI(TAG, "File tes berhasil dibuat!");
}

/**
 * Fungsi untuk membaca dan menampilkan isi file
 * Menangani error file tidak ditemukan secara graceful
 */
static void baca_file(const char *path)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  MEMBACA FILE: %s", path);
    ESP_LOGI(TAG, "========================================");

    // Cek keberadaan file terlebih dahulu
    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGW(TAG, "File '%s' tidak ditemukan!", path);
        ESP_LOGW(TAG, "Pastikan file sudah ada di SD Card.");
        return;
    }

    // Tampilkan informasi file
    ESP_LOGI(TAG, "Ukuran file: %ld byte", (long)st.st_size);

    // Buka file untuk dibaca
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuka file untuk dibaca: %s", path);
        return;
    }

    // Baca file baris per baris
    char buf[READ_BUF_SIZE];
    int nomor_baris = 0;

    ESP_LOGI(TAG, "--- Isi File ---");
    while (fgets(buf, sizeof(buf), f) != NULL) {
        nomor_baris++;
        // Hapus karakter newline di akhir baris
        buf[strcspn(buf, "\r\n")] = 0;
        ESP_LOGI(TAG, "[%03d] %s", nomor_baris, buf);
    }
    ESP_LOGI(TAG, "--- Akhir File ---");
    ESP_LOGI(TAG, "Total baris yang dibaca: %d", nomor_baris);

    fclose(f);
}

/**
 * Fungsi untuk mencoba membaca file yang tidak ada
 * Demonstrasi penanganan error secara graceful
 */
static void demo_file_tidak_ada(void)
{
    const char *path_tidak_ada = MOUNT_POINT "/file_tidak_ada.txt";

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  DEMO: Membaca File yang Tidak Ada");
    ESP_LOGI(TAG, "========================================");

    // Cek keberadaan file
    struct stat st;
    if (stat(path_tidak_ada, &st) != 0) {
        ESP_LOGW(TAG, "File '%s' TIDAK DITEMUKAN (expected)", path_tidak_ada);
        ESP_LOGI(TAG, "Penanganan error berhasil - program tetap berjalan normal");
    }

    // Coba buka file - ini akan mengembalikan NULL
    FILE *f = fopen(path_tidak_ada, "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "fopen() mengembalikan NULL untuk file yang tidak ada");
        ESP_LOGI(TAG, "Ini adalah perilaku yang diharapkan (graceful handling)");
    } else {
        fclose(f);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  Program ESP32_06_SD_Card_Read");
    ESP_LOGI(TAG, "  Membaca File dari SD Card");
    ESP_LOGI(TAG, "============================================");

    // Langkah 1: Mount SD Card
    sdmmc_card_t *card = mount_sd_card();
    if (card == NULL) {
        ESP_LOGE(TAG, "Gagal mount SD Card. Program berhenti.");
        return;
    }

    // Tampilkan info kartu singkat
    ESP_LOGI(TAG, "Kartu SD: %s, Kapasitas: %llu MB",
             card->cid.name,
             (unsigned long long)((uint64_t)card->csd.capacity * card->csd.sector_size / (1024 * 1024)));

    // Langkah 2: Buat file tes jika belum ada
    buat_file_tes_jika_belum_ada(TEST_FILE_NAME);

    // Langkah 3: Baca dan tampilkan isi file
    baca_file(TEST_FILE_NAME);

    // Langkah 4: Demo penanganan file yang tidak ada
    demo_file_tidak_ada();

    // Langkah 5: Unmount SD Card
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
