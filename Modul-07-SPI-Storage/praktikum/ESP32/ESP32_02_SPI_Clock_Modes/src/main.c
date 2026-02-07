/**
 * ============================================================
 * ESP32_02_SPI_Clock_Modes
 * ============================================================
 * Deskripsi  : Demonstrasi 4 mode clock SPI (Mode 0-3) pada ESP32
 *              Setiap mode memiliki kombinasi CPOL dan CPHA berbeda
 *              yang menentukan kapan data di-sample dan di-shift
 * Board      : ESP32 DOIT DevKit V1 / Wemos Lolin S2 / ESP32-S3
 * Framework  : ESP-IDF
 * 
 * Hardware yang dibutuhkan:
 *   - 1x ESP32 DevKit V1 (atau varian S2/S3)
 *   - 1x Kabel jumper male-to-male (hubungkan MOSI ke MISO)
 *   - 1x Kabel USB micro/Type-C
 * 
 * Koneksi Loopback:
 *   GPIO13 (MOSI) ---jumper wire---> GPIO12 (MISO)
 *   GPIO14 = SCLK (tidak perlu dihubungkan ke luar)
 *   GPIO15 = CS   (dikelola internal oleh driver)
 * 
 * Penjelasan Mode SPI:
 *   Mode 0: CPOL=0, CPHA=0 -> Clock idle LOW,  sample pada rising edge
 *   Mode 1: CPOL=0, CPHA=1 -> Clock idle LOW,  sample pada falling edge
 *   Mode 2: CPOL=1, CPHA=0 -> Clock idle HIGH, sample pada rising edge
 *   Mode 3: CPOL=1, CPHA=1 -> Clock idle HIGH, sample pada falling edge
 * ============================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "config.h"

static const char *TAG = "SPI_CLOCK_MODES";

// Tabel nama mode SPI untuk logging
static const char *mode_names[NUM_SPI_MODES] = {
    "Mode 0 (CPOL=0, CPHA=0)",
    "Mode 1 (CPOL=0, CPHA=1)",
    "Mode 2 (CPOL=1, CPHA=0)",
    "Mode 3 (CPOL=1, CPHA=1)"
};

// Deskripsi karakteristik timing tiap mode
static const char *mode_timing[NUM_SPI_MODES] = {
    "Clock idle LOW  | Data di-sample saat rising edge  | Data di-shift saat falling edge",
    "Clock idle LOW  | Data di-sample saat falling edge | Data di-shift saat rising edge",
    "Clock idle HIGH | Data di-sample saat rising edge  | Data di-shift saat falling edge",
    "Clock idle HIGH | Data di-sample saat falling edge | Data di-shift saat rising edge"
};

/**
 * Cetak isi buffer dalam format hexadecimal
 * @param label  Label untuk ditampilkan
 * @param buf    Pointer ke buffer data
 * @param len    Panjang data dalam byte
 */
static void print_buffer(const char *label, const uint8_t *buf, size_t len)
{
    printf("  %s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X ", buf[i]);
    }
    printf("\n");
}

/**
 * Inisialisasi SPI bus (tanpa menambahkan device)
 * Bus hanya perlu diinisialisasi sekali, device bisa ditambah/hapus
 * @return ESP_OK jika berhasil
 */
static esp_err_t spi_bus_init(void)
{
    // Konfigurasi pin SPI Bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,       // Pin MOSI
        .miso_io_num = PIN_NUM_MISO,       // Pin MISO
        .sclk_io_num = PIN_NUM_CLK,        // Pin SCLK
        .quadwp_io_num = -1,               // Tidak digunakan (untuk Quad SPI)
        .quadhd_io_num = -1,               // Tidak digunakan (untuk Quad SPI)
        .max_transfer_sz = MAX_TRANSFER_SIZE,
    };

    // Inisialisasi SPI Bus dengan DMA auto
    esp_err_t ret = spi_bus_initialize(SPI_HOST_USED, &bus_cfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI bus berhasil diinisialisasi pada HSPI (SPI2)");
    return ESP_OK;
}

/**
 * Jalankan tes loopback SPI dengan mode tertentu
 * Fungsi ini menambahkan device dengan mode yang diminta,
 * melakukan transfer, verifikasi data, lalu hapus device.
 * 
 * @param mode   Nomor mode SPI (0-3)
 * @return ESP_OK jika transfer berhasil
 */
static esp_err_t test_spi_mode(uint8_t mode)
{
    spi_device_handle_t spi_dev;
    esp_err_t ret;

    printf("\n------------------------------------------------------------\n");
    printf("  Menguji %s\n", mode_names[mode]);
    printf("  Timing: %s\n", mode_timing[mode]);
    printf("------------------------------------------------------------\n");

    // Konfigurasi SPI Device dengan mode yang diminta
    // Setiap mode memiliki kombinasi CPOL dan CPHA berbeda
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,  // Kecepatan clock 1 MHz
        .mode = mode,                           // Mode SPI (0-3)
        .spics_io_num = PIN_NUM_CS,            // Pin Chip Select
        .queue_size = 7,                        // Ukuran antrian transaksi
        .flags = 0,                             // Tidak ada flag khusus
    };

    // Tambahkan device ke SPI bus dengan mode tertentu
    ret = spi_bus_add_device(SPI_HOST_USED, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambahkan device mode %d: %s", mode, esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Device ditambahkan dengan %s", mode_names[mode]);

    // === Transfer 1: Data pola incrementing ===
    uint8_t tx_data1[TEST_DATA_LEN];
    uint8_t rx_data1[TEST_DATA_LEN] = {0};
    for (int i = 0; i < TEST_DATA_LEN; i++) {
        tx_data1[i] = (mode * 0x10) + i;  // Data unik per mode: Mode0=0x00.., Mode1=0x10..
    }

    // Konfigurasi transaksi SPI full-duplex
    spi_transaction_t trans1 = {
        .length = TEST_DATA_LEN * 8,    // Panjang dalam bit
        .tx_buffer = tx_data1,           // Buffer kirim
        .rx_buffer = rx_data1,           // Buffer terima
    };

    // Ukur waktu transfer
    int64_t start_time = esp_timer_get_time();
    ret = spi_device_transmit(spi_dev, &trans1);
    int64_t elapsed = esp_timer_get_time() - start_time;

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Transfer gagal pada mode %d: %s", mode, esp_err_to_name(ret));
        spi_bus_remove_device(spi_dev);
        return ret;
    }

    printf("  Transfer 1: Pola Incrementing (%d byte)\n", TEST_DATA_LEN);
    print_buffer("TX", tx_data1, TEST_DATA_LEN);
    print_buffer("RX", rx_data1, TEST_DATA_LEN);
    printf("  Waktu transfer: %lld us\n", (long long)elapsed);

    // Verifikasi data loopback (TX harus sama dengan RX)
    bool match1 = (memcmp(tx_data1, rx_data1, TEST_DATA_LEN) == 0);
    printf("  Hasil: %s\n", match1 ? "PASS - Data cocok!" : "FAIL - Data tidak cocok!");

    // === Transfer 2: Data pola alternating bits ===
    uint8_t tx_data2[TEST_DATA_LEN];
    uint8_t rx_data2[TEST_DATA_LEN] = {0};
    for (int i = 0; i < TEST_DATA_LEN; i++) {
        tx_data2[i] = (i % 2 == 0) ? 0xAA : 0x55;  // Pola bit bergantian
    }

    spi_transaction_t trans2 = {
        .length = TEST_DATA_LEN * 8,
        .tx_buffer = tx_data2,
        .rx_buffer = rx_data2,
    };

    start_time = esp_timer_get_time();
    ret = spi_device_transmit(spi_dev, &trans2);
    elapsed = esp_timer_get_time() - start_time;

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Transfer 2 gagal pada mode %d: %s", mode, esp_err_to_name(ret));
        spi_bus_remove_device(spi_dev);
        return ret;
    }

    printf("\n  Transfer 2: Pola Alternating Bits (%d byte)\n", TEST_DATA_LEN);
    print_buffer("TX", tx_data2, TEST_DATA_LEN);
    print_buffer("RX", rx_data2, TEST_DATA_LEN);
    printf("  Waktu transfer: %lld us\n", (long long)elapsed);

    bool match2 = (memcmp(tx_data2, rx_data2, TEST_DATA_LEN) == 0);
    printf("  Hasil: %s\n", match2 ? "PASS - Data cocok!" : "FAIL - Data tidak cocok!");

    // === Transfer 3: Data semua 0xFF ===
    uint8_t tx_data3[TEST_DATA_LEN];
    uint8_t rx_data3[TEST_DATA_LEN] = {0};
    memset(tx_data3, 0xFF, TEST_DATA_LEN);

    spi_transaction_t trans3 = {
        .length = TEST_DATA_LEN * 8,
        .tx_buffer = tx_data3,
        .rx_buffer = rx_data3,
    };

    start_time = esp_timer_get_time();
    ret = spi_device_transmit(spi_dev, &trans3);
    elapsed = esp_timer_get_time() - start_time;

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Transfer 3 gagal pada mode %d: %s", mode, esp_err_to_name(ret));
        spi_bus_remove_device(spi_dev);
        return ret;
    }

    printf("\n  Transfer 3: Pola All-High 0xFF (%d byte)\n", TEST_DATA_LEN);
    print_buffer("TX", tx_data3, TEST_DATA_LEN);
    print_buffer("RX", rx_data3, TEST_DATA_LEN);
    printf("  Waktu transfer: %lld us\n", (long long)elapsed);

    bool match3 = (memcmp(tx_data3, rx_data3, TEST_DATA_LEN) == 0);
    printf("  Hasil: %s\n", match3 ? "PASS - Data cocok!" : "FAIL - Data tidak cocok!");

    // Ringkasan hasil untuk mode ini
    printf("\n  === Ringkasan %s ===", mode_names[mode]);
    printf("\n  Transfer 1 (Incrementing) : %s", match1 ? "PASS" : "FAIL");
    printf("\n  Transfer 2 (Alternating)  : %s", match2 ? "PASS" : "FAIL");
    printf("\n  Transfer 3 (All-High)     : %s\n", match3 ? "PASS" : "FAIL");

    // Hapus device dari bus sebelum menambahkan dengan mode baru
    ret = spi_bus_remove_device(spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menghapus device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Tunggu sebentar agar bus stabil sebelum mode berikutnya
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

/**
 * Fungsi utama program
 * Menjalankan tes SPI loopback pada keempat mode clock
 */
void app_main(void)
{
    printf("\n============================================================\n");
    printf("  ESP32 SPI Clock Modes - Demonstrasi 4 Mode SPI\n");
    printf("  Hubungkan MOSI (GPIO%d) ke MISO (GPIO%d) untuk loopback\n", PIN_NUM_MOSI, PIN_NUM_MISO);
    printf("============================================================\n");

    // Tampilkan informasi konfigurasi
    ESP_LOGI(TAG, "Konfigurasi SPI:");
    ESP_LOGI(TAG, "  Host     : SPI2_HOST (HSPI)");
    ESP_LOGI(TAG, "  MOSI     : GPIO %d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  MISO     : GPIO %d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  SCLK     : GPIO %d", PIN_NUM_CLK);
    ESP_LOGI(TAG, "  CS       : GPIO %d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  Clock    : %d Hz", SPI_CLOCK_SPEED_HZ);
    ESP_LOGI(TAG, "  Data Len : %d byte", TEST_DATA_LEN);

    // Penjelasan teori SPI modes
    printf("\n  Teori SPI Clock Modes:\n");
    printf("  CPOL (Clock Polarity) = kondisi clock saat idle\n");
    printf("    CPOL=0: Clock idle pada logika LOW\n");
    printf("    CPOL=1: Clock idle pada logika HIGH\n");
    printf("  CPHA (Clock Phase) = kapan data di-sample\n");
    printf("    CPHA=0: Data di-sample pada edge pertama clock\n");
    printf("    CPHA=1: Data di-sample pada edge kedua clock\n");
    printf("\n");

    // Inisialisasi SPI bus (satu kali saja)
    if (spi_bus_init() != ESP_OK) {
        ESP_LOGE(TAG, "Inisialisasi SPI bus gagal, program berhenti");
        return;
    }

    // Array untuk menyimpan hasil setiap mode
    bool mode_results[NUM_SPI_MODES] = {false};

    // Uji semua 4 mode SPI secara berurutan
    for (int mode = 0; mode < NUM_SPI_MODES; mode++) {
        esp_err_t ret = test_spi_mode(mode);
        mode_results[mode] = (ret == ESP_OK);
    }

    // Tampilkan ringkasan akhir semua mode
    printf("\n============================================================\n");
    printf("  RINGKASAN AKHIR - Semua Mode SPI\n");
    printf("============================================================\n");
    for (int mode = 0; mode < NUM_SPI_MODES; mode++) {
        printf("  %s : %s\n", mode_names[mode],
               mode_results[mode] ? "BERHASIL" : "GAGAL");
    }
    printf("\n  Catatan: Pada loopback test, semua mode seharusnya BERHASIL\n");
    printf("  karena MOSI langsung terhubung ke MISO (tidak ada slave).\n");
    printf("  Perbedaan mode terlihat pada sinyal clock (perlu oscilloscope).\n");
    printf("============================================================\n");

    // De-inisialisasi SPI bus
    spi_bus_free(SPI_HOST_USED);
    ESP_LOGI(TAG, "SPI bus dibebaskan. Program selesai.");

    // Loop selamanya
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
