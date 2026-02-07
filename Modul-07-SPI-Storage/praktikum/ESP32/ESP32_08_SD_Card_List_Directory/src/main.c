/**
 * ============================================================================
 * Program     : ESP32_08_SD_Card_List_Directory
 * Deskripsi   : Listing direktori dan file secara rekursif pada SD Card via SPI
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
 *   2. Membuat struktur direktori contoh dengan file-file
 *   3. Listing rekursif semua file dan direktori
 *   4. Menampilkan ukuran file
 *   5. Unmount secara bersih
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>              // Untuk operasi direktori (opendir, readdir, dll)
#include <errno.h>
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
static const char *TAG = "SD_LIST_DIR";

// Variabel penghitung untuk statistik
static int total_file = 0;
static int total_dir = 0;
static uint64_t total_ukuran = 0;

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
 * Fungsi helper untuk membuat direktori jika belum ada
 * Menangani error EEXIST secara graceful
 */
static void buat_direktori(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        // Direktori sudah ada
        return;
    }

    int ret = mkdir(path, 0775);
    if (ret != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "Gagal membuat direktori '%s': %s", path, strerror(errno));
    } else {
        ESP_LOGI(TAG, "Direktori dibuat: %s", path);
    }
}

/**
 * Fungsi helper untuk membuat file dengan konten
 * Hanya membuat jika file belum ada
 */
static void buat_file(const char *path, const char *konten)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        // File sudah ada, lewati
        return;
    }

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuat file '%s'", path);
        return;
    }
    fprintf(f, "%s", konten);
    fclose(f);
    ESP_LOGI(TAG, "File dibuat: %s", path);
}

/**
 * Fungsi untuk membuat struktur direktori contoh
 * Membuat beberapa folder dan file untuk demonstrasi listing
 */
static void buat_struktur_contoh(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Membuat Struktur Direktori Contoh");
    ESP_LOGI(TAG, "========================================");

    // Buat direktori-direktori
    buat_direktori(MOUNT_POINT "/data");
    buat_direktori(MOUNT_POINT "/data/sensor");
    buat_direktori(MOUNT_POINT "/data/log");
    buat_direktori(MOUNT_POINT "/config");
    buat_direktori(MOUNT_POINT "/backup");

    // Buat file-file di direktori root
    buat_file(MOUNT_POINT "/readme.txt",
             "ESP32 SD Card - Praktikum Sistem Embedded\n"
             "Modul 07: SPI & Storage\n");

    buat_file(MOUNT_POINT "/info.txt",
             "Board: ESP32 DevKit V1\n"
             "Storage: Micro SD Card via SPI\n");

    // Buat file-file di direktori data/sensor
    buat_file(MOUNT_POINT "/data/sensor/suhu.csv",
             "waktu,suhu_c\n"
             "00:00,25.3\n"
             "00:01,25.5\n"
             "00:02,25.1\n");

    buat_file(MOUNT_POINT "/data/sensor/kelembapan.csv",
             "waktu,kelembapan_persen\n"
             "00:00,65.2\n"
             "00:01,64.8\n");

    // Buat file di direktori data/log
    buat_file(MOUNT_POINT "/data/log/system.log",
             "[INFO] Sistem dimulai\n"
             "[INFO] SD Card terdeteksi\n"
             "[INFO] Sensor aktif\n");

    // Buat file di direktori config
    buat_file(MOUNT_POINT "/config/wifi.cfg",
             "ssid=ESP32_Network\n"
             "password=12345678\n");

    buat_file(MOUNT_POINT "/config/sensor.cfg",
             "interval_ms=1000\n"
             "sensor_type=DHT22\n");

    // Buat file di direktori backup
    buat_file(MOUNT_POINT "/backup/data_backup.txt",
             "Backup data sensor terakhir\n"
             "Tanggal: 2026-02-07\n");

    ESP_LOGI(TAG, "Struktur direktori contoh selesai dibuat!");
}

/**
 * Fungsi untuk menampilkan indentasi berdasarkan kedalaman
 * Membantu visualisasi hierarki direktori
 */
static void cetak_indentasi(int kedalaman)
{
    for (int i = 0; i < kedalaman; i++) {
        printf("  ");  // 2 spasi per level kedalaman
    }
}

/**
 * Fungsi untuk memformat ukuran file ke format yang mudah dibaca
 * Contoh: 1024 -> "1.00 KB", 1048576 -> "1.00 MB"
 */
static void format_ukuran(uint64_t ukuran, char *buf, size_t buf_len)
{
    if (ukuran < 1024) {
        snprintf(buf, buf_len, "%llu B", (unsigned long long)ukuran);
    } else if (ukuran < 1024 * 1024) {
        snprintf(buf, buf_len, "%.2f KB", (float)ukuran / 1024.0f);
    } else {
        snprintf(buf, buf_len, "%.2f MB", (float)ukuran / (1024.0f * 1024.0f));
    }
}

/**
 * Fungsi utama untuk listing direktori secara rekursif
 * Menampilkan semua file dan subdirektori dengan informasi ukuran
 *
 * Parameter:
 *   - path: path absolut direktori yang akan di-list
 *   - kedalaman: level kedalaman saat ini (untuk indentasi)
 */
static void list_direktori_rekursif(const char *path, int kedalaman)
{
    // Batasi kedalaman rekursi untuk mencegah stack overflow
    if (kedalaman > MAX_DEPTH) {
        cetak_indentasi(kedalaman);
        ESP_LOGW(TAG, "Kedalaman maksimum tercapai, berhenti di: %s", path);
        return;
    }

    // Buka direktori
    DIR *dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Gagal membuka direktori '%s': %s", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    struct stat entry_stat;
    char full_path[PATH_MAX_LEN];

    // Iterasi melalui semua entri dalam direktori
    while ((entry = readdir(dir)) != NULL) {
        // Lewati entri "." (direktori saat ini) dan ".." (direktori parent)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Bangun path lengkap untuk entri
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        // Dapatkan informasi statistik entri
        if (stat(full_path, &entry_stat) != 0) {
            ESP_LOGE(TAG, "Gagal mendapatkan info: %s", full_path);
            continue;
        }

        // Cek apakah entri adalah direktori atau file
        if (S_ISDIR(entry_stat.st_mode)) {
            // === DIREKTORI ===
            total_dir++;
            cetak_indentasi(kedalaman);
            printf("📁 [DIR]  %s/\n", entry->d_name);

            // Rekursi ke subdirektori
            list_direktori_rekursif(full_path, kedalaman + 1);
        } else {
            // === FILE ===
            total_file++;
            total_ukuran += (uint64_t)entry_stat.st_size;

            // Format ukuran file
            char ukuran_str[32];
            format_ukuran((uint64_t)entry_stat.st_size, ukuran_str, sizeof(ukuran_str));

            cetak_indentasi(kedalaman);
            printf("📄 [FILE] %s (%s)\n", entry->d_name, ukuran_str);
        }
    }

    closedir(dir);
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  Program ESP32_08_SD_Card_List_Directory");
    ESP_LOGI(TAG, "  Listing Rekursif Direktori SD Card");
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
    ESP_LOGI(TAG, "Kartu SD: %s, Kapasitas: %llu MB",
             card->cid.name,
             (unsigned long long)((uint64_t)card->csd.capacity * card->csd.sector_size / (1024 * 1024)));

    // ========================================================================
    // Langkah 2: Buat struktur direktori contoh
    // ========================================================================
    buat_struktur_contoh();

    // ========================================================================
    // Langkah 3: Listing rekursif seluruh isi SD Card
    // ========================================================================
    // Reset penghitung
    total_file = 0;
    total_dir = 0;
    total_ukuran = 0;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  LISTING ISI SD CARD");
    ESP_LOGI(TAG, "========================================");
    printf("\n📂 %s/ (root)\n", MOUNT_POINT);

    // Mulai listing dari root mount point
    list_direktori_rekursif(MOUNT_POINT, 1);

    // ========================================================================
    // Langkah 4: Tampilkan statistik
    // ========================================================================
    char total_ukuran_str[32];
    format_ukuran(total_ukuran, total_ukuran_str, sizeof(total_ukuran_str));

    printf("\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  STATISTIK SD CARD");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Total Direktori : %d", total_dir);
    ESP_LOGI(TAG, "Total File      : %d", total_file);
    ESP_LOGI(TAG, "Total Ukuran    : %s", total_ukuran_str);
    ESP_LOGI(TAG, "========================================");

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
