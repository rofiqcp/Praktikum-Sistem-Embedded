/**
 * ============================================================
 * ESP32_03_SPI_DMA_Transfer
 * ============================================================
 * Deskripsi  : Demonstrasi transfer SPI dengan DMA pada ESP32
 *              Membandingkan performa transfer berbagai ukuran data
 *              menggunakan DMA (Direct Memory Access)
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
 * Konsep DMA (Direct Memory Access):
 *   DMA memungkinkan transfer data antara memori dan periferal
 *   tanpa melibatkan CPU. Ini meningkatkan efisiensi karena:
 *   1. CPU bebas mengerjakan tugas lain selama transfer
 *   2. Transfer data lebih cepat untuk blok data besar
 *   3. Mengurangi overhead interrupt per byte
 * 
 *   Pada ESP32, SPI DMA menggunakan linked-list descriptor
 *   untuk mengelola buffer data yang lebih besar dari 64 byte.
 * ============================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "config.h"

static const char *TAG = "SPI_DMA";

// Handle SPI device
static spi_device_handle_t spi_dev;

/**
 * Inisialisasi SPI bus dan device dengan konfigurasi DMA
 * DMA_CHANNEL diset ke SPI_DMA_CH_AUTO agar ESP-IDF
 * memilih channel DMA secara otomatis
 * @return ESP_OK jika berhasil
 */
static esp_err_t spi_dma_init(void)
{
    // Konfigurasi pin SPI Bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,              // Tidak digunakan
        .quadhd_io_num = -1,              // Tidak digunakan
        .max_transfer_sz = MAX_TRANSFER_SIZE,  // Max transfer = 4096 byte
    };

    // Inisialisasi SPI Bus DENGAN DMA
    // Parameter ketiga (DMA_CHANNEL) menentukan penggunaan DMA:
    //   SPI_DMA_CH_AUTO = otomatis pilih channel
    //   0 = tanpa DMA (polling/interrupt saja)
    //   1 atau 2 = channel DMA spesifik
    esp_err_t ret = spi_bus_initialize(SPI_HOST_USED, &bus_cfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI bus diinisialisasi dengan DMA (channel: AUTO)");

    // Konfigurasi SPI Device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,  // 10 MHz
        .mode = SPI_MODE,                       // Mode 0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_USED, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambahkan SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI device ditambahkan (clock: %d Hz)", SPI_CLOCK_SPEED_HZ);

    return ESP_OK;
}

/**
 * Lakukan transfer SPI full-duplex dan ukur waktu eksekusi
 * Buffer DMA harus dialokasikan dengan heap_caps_malloc(... MALLOC_CAP_DMA)
 * agar kompatibel dengan controller DMA ESP32
 * 
 * @param tx_buf   Buffer data yang dikirim (harus DMA-capable)
 * @param rx_buf   Buffer data yang diterima (harus DMA-capable)
 * @param len      Panjang data dalam byte
 * @param time_us  Pointer untuk menyimpan waktu transfer (mikro detik)
 * @return ESP_OK jika berhasil
 */
static esp_err_t spi_dma_transfer(const uint8_t *tx_buf, uint8_t *rx_buf,
                                   size_t len, int64_t *time_us)
{
    spi_transaction_t trans = {
        .length = len * 8,          // Panjang dalam bit
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    // Ukur waktu transfer menggunakan high-resolution timer
    int64_t start = esp_timer_get_time();
    esp_err_t ret = spi_device_transmit(spi_dev, &trans);
    int64_t end = esp_timer_get_time();

    if (time_us != NULL) {
        *time_us = end - start;
    }

    return ret;
}

/**
 * Isi buffer dengan pola data untuk verifikasi
 * Pola: setiap byte = (index % 256) XOR seed
 * @param buf   Buffer yang akan diisi
 * @param len   Panjang buffer
 * @param seed  Nilai seed untuk variasi pola
 */
static void fill_test_pattern(uint8_t *buf, size_t len, uint8_t seed)
{
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)((i & 0xFF) ^ seed);
    }
}

/**
 * Verifikasi data loopback (bandingkan TX dan RX)
 * @param tx     Buffer data yang dikirim
 * @param rx     Buffer data yang diterima
 * @param len    Panjang data
 * @return true jika data cocok
 */
static bool verify_data(const uint8_t *tx, const uint8_t *rx, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (tx[i] != rx[i]) {
            ESP_LOGE(TAG, "Mismatch pada byte[%d]: TX=0x%02X, RX=0x%02X",
                     (int)i, tx[i], rx[i]);
            return false;
        }
    }
    return true;
}

/**
 * Jalankan benchmark transfer DMA untuk ukuran tertentu
 * Melakukan beberapa iterasi dan menghitung rata-rata waktu
 * serta throughput dalam KB/s
 * 
 * @param size          Ukuran data per transfer dalam byte
 * @param label         Label untuk ditampilkan
 * @param avg_time_us   Pointer untuk menyimpan rata-rata waktu
 */
static void run_dma_benchmark(size_t size, const char *label, double *avg_time_us)
{
    // Alokasi buffer DMA-capable
    // PENTING: Buffer untuk DMA HARUS dialokasikan dengan MALLOC_CAP_DMA
    // agar berada di memori yang bisa diakses oleh controller DMA
    uint8_t *tx_buf = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_DMA);
    uint8_t *rx_buf = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_DMA);

    if (tx_buf == NULL || rx_buf == NULL) {
        ESP_LOGE(TAG, "Gagal alokasi buffer DMA untuk ukuran %d byte", (int)size);
        if (tx_buf) heap_caps_free(tx_buf);
        if (rx_buf) heap_caps_free(rx_buf);
        return;
    }

    printf("\n  --- %s (%d byte) ---\n", label, (int)size);

    // Isi TX buffer dengan pola data
    fill_test_pattern(tx_buf, size, 0x42);

    int64_t total_time = 0;
    int success_count = 0;
    bool data_valid = true;

    // Jalankan beberapa iterasi untuk mendapatkan rata-rata
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memset(rx_buf, 0, size);  // Bersihkan RX buffer

        int64_t transfer_time = 0;
        esp_err_t ret = spi_dma_transfer(tx_buf, rx_buf, size, &transfer_time);

        if (ret == ESP_OK) {
            total_time += transfer_time;
            success_count++;

            // Verifikasi data hanya pada iterasi pertama
            if (i == 0) {
                data_valid = verify_data(tx_buf, rx_buf, size);
            }
        } else {
            ESP_LOGE(TAG, "Transfer iterasi %d gagal: %s", i, esp_err_to_name(ret));
        }
    }

    // Hitung dan tampilkan statistik
    if (success_count > 0) {
        double avg = (double)total_time / success_count;
        double throughput_kbps = (size * 1000.0) / avg;  // KB/s

        printf("  Iterasi berhasil  : %d / %d\n", success_count, NUM_ITERATIONS);
        printf("  Verifikasi data   : %s\n", data_valid ? "PASS" : "FAIL");
        printf("  Waktu rata-rata   : %.1f us\n", avg);
        printf("  Throughput        : %.1f KB/s\n", throughput_kbps);
        printf("  Waktu per byte    : %.2f us/byte\n", avg / size);

        if (avg_time_us != NULL) {
            *avg_time_us = avg;
        }
    } else {
        printf("  Semua transfer GAGAL!\n");
    }

    // Bebaskan buffer DMA
    heap_caps_free(tx_buf);
    heap_caps_free(rx_buf);
}

/**
 * Fungsi utama program
 * Menjalankan benchmark transfer DMA dengan berbagai ukuran data
 * dan membandingkan performa masing-masing
 */
void app_main(void)
{
    printf("\n============================================================\n");
    printf("  ESP32 SPI DMA Transfer - Benchmark\n");
    printf("  Hubungkan MOSI (GPIO%d) ke MISO (GPIO%d) untuk loopback\n",
           PIN_NUM_MOSI, PIN_NUM_MISO);
    printf("============================================================\n");

    // Tampilkan informasi konfigurasi
    ESP_LOGI(TAG, "Konfigurasi SPI DMA:");
    ESP_LOGI(TAG, "  Host       : SPI2_HOST (HSPI)");
    ESP_LOGI(TAG, "  MOSI       : GPIO %d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  MISO       : GPIO %d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  SCLK       : GPIO %d", PIN_NUM_CLK);
    ESP_LOGI(TAG, "  CS         : GPIO %d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  Clock      : %d Hz (%d MHz)", SPI_CLOCK_SPEED_HZ,
             SPI_CLOCK_SPEED_HZ / 1000000);
    ESP_LOGI(TAG, "  DMA        : AUTO");
    ESP_LOGI(TAG, "  Iterasi    : %d per ukuran", NUM_ITERATIONS);

    // Tampilkan informasi memori DMA yang tersedia
    size_t dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
    ESP_LOGI(TAG, "  DMA memory : %d byte tersedia", (int)dma_free);

    // Penjelasan DMA pada SPI
    printf("\n  Penjelasan DMA (Direct Memory Access):\n");
    printf("  - DMA memungkinkan transfer data tanpa keterlibatan CPU\n");
    printf("  - CPU bebas mengerjakan tugas lain selama transfer\n");
    printf("  - Semakin besar data, semakin terasa keuntungan DMA\n");
    printf("  - Buffer DMA harus dialokasikan di memori DMA-capable\n");
    printf("  - ESP32 SPI DMA mendukung transfer hingga 64KB per transaksi\n");

    // Inisialisasi SPI dengan DMA
    if (spi_dma_init() != ESP_OK) {
        ESP_LOGE(TAG, "Inisialisasi SPI DMA gagal, program berhenti");
        return;
    }

    // Variabel untuk menyimpan hasil benchmark
    double time_small = 0, time_medium = 0, time_large = 0, time_xlarge = 0;

    printf("\n============================================================\n");
    printf("  BENCHMARK TRANSFER DMA - Berbagai Ukuran Data\n");
    printf("============================================================\n");

    // Benchmark 1: Transfer kecil (16 byte)
    run_dma_benchmark(SMALL_TRANSFER_SIZE, "Transfer Kecil", &time_small);

    // Benchmark 2: Transfer medium (256 byte)
    run_dma_benchmark(MEDIUM_TRANSFER_SIZE, "Transfer Medium", &time_medium);

    // Benchmark 3: Transfer besar (1024 byte)
    run_dma_benchmark(LARGE_TRANSFER_SIZE, "Transfer Besar", &time_large);

    // Benchmark 4: Transfer sangat besar (4096 byte)
    run_dma_benchmark(XLARGE_TRANSFER_SIZE, "Transfer Sangat Besar", &time_xlarge);

    // Tampilkan perbandingan hasil
    printf("\n============================================================\n");
    printf("  PERBANDINGAN PERFORMA DMA TRANSFER\n");
    printf("============================================================\n");
    printf("  %-25s | %-12s | %-12s | %-12s\n", "Ukuran", "Waktu (us)", "KB/s", "us/byte");
    printf("  %-25s-+-%-12s-+-%-12s-+-%-12s\n", "-------------------------",
           "------------", "------------", "------------");

    if (time_small > 0) {
        printf("  %-25s | %10.1f   | %10.1f   | %10.2f\n",
               "16 byte (kecil)", time_small,
               (SMALL_TRANSFER_SIZE * 1000.0) / time_small,
               time_small / SMALL_TRANSFER_SIZE);
    }
    if (time_medium > 0) {
        printf("  %-25s | %10.1f   | %10.1f   | %10.2f\n",
               "256 byte (medium)", time_medium,
               (MEDIUM_TRANSFER_SIZE * 1000.0) / time_medium,
               time_medium / MEDIUM_TRANSFER_SIZE);
    }
    if (time_large > 0) {
        printf("  %-25s | %10.1f   | %10.1f   | %10.2f\n",
               "1024 byte (besar)", time_large,
               (LARGE_TRANSFER_SIZE * 1000.0) / time_large,
               time_large / LARGE_TRANSFER_SIZE);
    }
    if (time_xlarge > 0) {
        printf("  %-25s | %10.1f   | %10.1f   | %10.2f\n",
               "4096 byte (sangat besar)", time_xlarge,
               (XLARGE_TRANSFER_SIZE * 1000.0) / time_xlarge,
               time_xlarge / XLARGE_TRANSFER_SIZE);
    }

    // Hitung dan tampilkan rasio speedup
    printf("\n  Analisis Performa DMA:\n");
    if (time_small > 0 && time_xlarge > 0) {
        double ratio_byte = (time_small / SMALL_TRANSFER_SIZE) /
                            (time_xlarge / XLARGE_TRANSFER_SIZE);
        printf("  - Rasio efisiensi (kecil vs sangat besar): %.1fx\n", ratio_byte);
        printf("  - Overhead per-byte MENURUN seiring bertambahnya ukuran data\n");
        printf("  - Ini menunjukkan keuntungan DMA untuk transfer besar\n");
    }
    printf("  - DMA mengurangi beban CPU untuk transfer data besar\n");
    printf("  - Untuk transfer kecil (<32 byte), overhead DMA setup\n");
    printf("    mungkin lebih besar dari transfer itu sendiri\n");

    printf("\n============================================================\n");
    printf("  Benchmark selesai!\n");
    printf("============================================================\n");

    // De-inisialisasi SPI
    spi_bus_remove_device(spi_dev);
    spi_bus_free(SPI_HOST_USED);
    ESP_LOGI(TAG, "SPI bus dibebaskan. Program selesai.");

    // Loop selamanya
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
