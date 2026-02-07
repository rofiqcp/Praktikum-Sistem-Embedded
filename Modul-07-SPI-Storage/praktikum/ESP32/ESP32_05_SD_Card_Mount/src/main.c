/**
 * ============================================================================
 * Program     : ESP32_05_SD_Card_Mount
 * Deskripsi   : Mount SD Card via SPI menggunakan VFS FAT Filesystem pada ESP32
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
 *   1. Inisialisasi bus SPI untuk komunikasi SD Card
 *   2. Mount filesystem FAT pada SD Card menggunakan VFS
 *   3. Menampilkan informasi kartu SD (tipe, ukuran, kecepatan)
 *   4. Menampilkan detail filesystem yang di-mount
 *   5. Unmount SD Card secara bersih
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
static const char *TAG = "SD_MOUNT";

/**
 * Fungsi untuk menampilkan informasi kartu SD secara detail
 * Menampilkan: nama, tipe, kapasitas, kecepatan, dan bus width
 */
static void tampilkan_info_kartu(const sdmmc_card_t *card)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   INFORMASI KARTU SD");
    ESP_LOGI(TAG, "========================================");

    // Nama kartu SD
    ESP_LOGI(TAG, "Nama Kartu      : %s", card->cid.name);

    // Tipe kartu: SDHC, SDSC, MMC, dll.
    // Bit 30 dari OCR register menunjukkan kartu SDHC/SDXC
    if (card->ocr & (1 << 30)) {
        ESP_LOGI(TAG, "Tipe Kartu      : SDHC/SDXC");
    } else {
        ESP_LOGI(TAG, "Tipe Kartu      : SDSC");
    }

    // Kapasitas kartu dalam MB
    // card->csd.capacity = jumlah sektor, setiap sektor = 512 byte
    uint64_t kapasitas_byte = (uint64_t)card->csd.capacity * card->csd.sector_size;
    uint32_t kapasitas_mb = (uint32_t)(kapasitas_byte / (1024 * 1024));
    ESP_LOGI(TAG, "Kapasitas       : %lu MB (%.2f GB)",
             (unsigned long)kapasitas_mb,
             (float)kapasitas_mb / 1024.0f);

    // Kecepatan transfer maksimal
    ESP_LOGI(TAG, "Kecepatan Maks  : %d kHz", card->max_freq_khz);

    // Ukuran sektor
    ESP_LOGI(TAG, "Ukuran Sektor   : %d byte", card->csd.sector_size);

    // Informasi CSD (Card Specific Data)
    ESP_LOGI(TAG, "CSD Versi       : %d", card->csd.csd_ver);

    ESP_LOGI(TAG, "========================================");
}

/**
 * Fungsi untuk menampilkan informasi filesystem yang sudah di-mount
 * Menggunakan FATFS API untuk mendapatkan info volume
 */
static void tampilkan_info_filesystem(void)
{
    FATFS *fs;
    DWORD fre_clust;

    // Mendapatkan informasi volume filesystem
    // Menggunakan drive number "0:" untuk partisi pertama
    FRESULT res = f_getfree("0:", &fre_clust, &fs);
    if (res != FR_OK) {
        ESP_LOGE(TAG, "Gagal mendapatkan info filesystem (error: %d)", res);
        return;
    }

    // Menghitung kapasitas total dan ruang kosong
    // Total sektor = (total cluster - 2) * sektor per cluster
    uint64_t total_bytes = (uint64_t)(fs->n_fatent - 2) * fs->csize * 512;
    uint64_t free_bytes  = (uint64_t)fre_clust * fs->csize * 512;
    uint64_t used_bytes  = total_bytes - free_bytes;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   INFORMASI FILESYSTEM");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Mount Point     : %s", MOUNT_POINT);
    ESP_LOGI(TAG, "Tipe Filesystem : FAT");
    ESP_LOGI(TAG, "Ukuran Cluster  : %d sektor", fs->csize);
    ESP_LOGI(TAG, "Total Kapasitas : %llu byte (%.2f MB)",
             (unsigned long long)total_bytes, (float)total_bytes / (1024.0f * 1024.0f));
    ESP_LOGI(TAG, "Ruang Terpakai  : %llu byte (%.2f MB)",
             (unsigned long long)used_bytes, (float)used_bytes / (1024.0f * 1024.0f));
    ESP_LOGI(TAG, "Ruang Kosong    : %llu byte (%.2f MB)",
             (unsigned long long)free_bytes, (float)free_bytes / (1024.0f * 1024.0f));
    ESP_LOGI(TAG, "========================================");
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  Program ESP32_05_SD_Card_Mount");
    ESP_LOGI(TAG, "  Mount SD Card via SPI + VFS FAT");
    ESP_LOGI(TAG, "============================================");

    // ========================================================================
    // LANGKAH 1: Konfigurasi opsi mount filesystem
    // ========================================================================
    // Struktur konfigurasi untuk mounting FAT filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,    // Jangan format otomatis jika mount gagal
        .max_files = MAX_FILES,             // Jumlah file maksimal yang bisa dibuka
        .allocation_unit_size = 16 * 1024   // Ukuran unit alokasi (16 KB)
    };

    sdmmc_card_t *card;     // Pointer untuk menyimpan info kartu SD

    ESP_LOGI(TAG, "Menginisialisasi SD Card via SPI...");

    // ========================================================================
    // LANGKAH 2: Inisialisasi bus SPI
    // ========================================================================
    // Konfigurasi bus SPI (MOSI, MISO, SCLK)
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,        // Pin MOSI (Master Out Slave In)
        .miso_io_num = PIN_NUM_MISO,        // Pin MISO (Master In Slave Out)
        .sclk_io_num = PIN_NUM_CLK,         // Pin Serial Clock
        .quadwp_io_num = -1,                // Tidak menggunakan Quad SPI WP
        .quadhd_io_num = -1,                // Tidak menggunakan Quad SPI HD
        .max_transfer_sz = 4000,            // Ukuran transfer maksimal (byte)
    };

    // Inisialisasi bus SPI dengan DMA
    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi bus SPI: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Bus SPI berhasil diinisialisasi");

    // ========================================================================
    // LANGKAH 3: Konfigurasi device SPI untuk SD Card
    // ========================================================================
    // Konfigurasi slot SPI untuk SD Card (pin CS dan parameter lainnya)
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;       // Pin Chip Select
    slot_config.host_id = SD_SPI_HOST;      // Host SPI yang digunakan

    // ========================================================================
    // LANGKAH 4: Konfigurasi host SD
    // ========================================================================
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    host.max_freq_khz = SPI_FREQUENCY;      // Frekuensi SPI maksimal

    // ========================================================================
    // LANGKAH 5: Mount filesystem FAT pada SD Card
    // ========================================================================
    ESP_LOGI(TAG, "Melakukan mounting SD Card ke '%s'...", MOUNT_POINT);

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config,
                                   &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Gagal mount filesystem!");
            ESP_LOGE(TAG, "Pastikan SD Card sudah diformat FAT32.");
        } else {
            ESP_LOGE(TAG, "Gagal inisialisasi SD Card: %s", esp_err_to_name(ret));
            ESP_LOGE(TAG, "Periksa koneksi kabel dan pastikan SD Card terpasang.");
        }
        // Bebaskan bus SPI jika mount gagal
        spi_bus_free(SD_SPI_HOST);
        return;
    }

    ESP_LOGI(TAG, "SD Card berhasil di-mount!");

    // ========================================================================
    // LANGKAH 6: Tampilkan informasi kartu SD
    // ========================================================================
    tampilkan_info_kartu(card);

    // ========================================================================
    // LANGKAH 7: Tampilkan informasi filesystem
    // ========================================================================
    tampilkan_info_filesystem();

    // ========================================================================
    // LANGKAH 8: Tes sederhana - buat file kecil untuk verifikasi
    // ========================================================================
    ESP_LOGI(TAG, "Melakukan tes tulis/baca sederhana...");

    // Membuat file tes
    const char *file_path = MOUNT_POINT "/test_mount.txt";
    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Gagal membuat file tes: %s", file_path);
    } else {
        fprintf(f, "SD Card berhasil di-mount pada ESP32!\n");
        fprintf(f, "Filesystem: FAT32 via VFS\n");
        fprintf(f, "Mount point: %s\n", MOUNT_POINT);
        fclose(f);
        ESP_LOGI(TAG, "File tes berhasil dibuat: %s", file_path);

        // Baca kembali file tes untuk verifikasi
        f = fopen(file_path, "r");
        if (f != NULL) {
            char buf[128];
            ESP_LOGI(TAG, "--- Isi file tes ---");
            while (fgets(buf, sizeof(buf), f) != NULL) {
                // Hapus newline di akhir baris
                buf[strcspn(buf, "\n")] = 0;
                ESP_LOGI(TAG, "%s", buf);
            }
            fclose(f);
            ESP_LOGI(TAG, "--- Akhir file tes ---");
        }

        // Dapatkan info ukuran file
        struct stat st;
        if (stat(file_path, &st) == 0) {
            ESP_LOGI(TAG, "Ukuran file tes: %ld byte", (long)st.st_size);
        }
    }

    // ========================================================================
    // LANGKAH 9: Unmount SD Card secara bersih
    // ========================================================================
    ESP_LOGI(TAG, "Melakukan unmount SD Card...");

    // Unmount filesystem dan bebaskan resource
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    ESP_LOGI(TAG, "SD Card berhasil di-unmount");

    // Bebaskan bus SPI
    spi_bus_free(SD_SPI_HOST);
    ESP_LOGI(TAG, "Bus SPI dibebaskan");

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  Program selesai. SD Card aman dicabut.");
    ESP_LOGI(TAG, "============================================");

    // Loop utama - program sudah selesai
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
