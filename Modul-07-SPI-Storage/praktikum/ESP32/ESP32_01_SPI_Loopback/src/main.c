/**
 * ============================================================
 * ESP32_01_SPI_Loopback
 * ============================================================
 * Deskripsi  : Tes loopback SPI pada ESP32 menggunakan ESP-IDF
 *              MOSI dihubungkan ke MISO untuk verifikasi data
 * Board      : ESP32 DOIT DevKit V1 / Wemos Lolin S2 / ESP32-S3
 * Framework  : ESP-IDF
 * 
 * Hardware yang dibutuhkan:
 *   - 1x ESP32 DevKit V1 (atau varian S2/S3)
 *   - 1x Kabel jumper male-to-male (untuk hubungkan MOSI ke MISO)
 *   - 1x Kabel USB micro/Type-C
 * 
 * Koneksi:
 *   GPIO13 (MOSI) ---jumper wire---> GPIO12 (MISO)
 *   GPIO14 = SCLK (tidak perlu dihubungkan ke luar)
 *   GPIO15 = CS   (dikelola internal oleh driver)
 * ============================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "SPI_LOOPBACK";

// Handle SPI device
static spi_device_handle_t spi_dev;

/**
 * Inisialisasi SPI bus dan device
 */
static esp_err_t spi_init(void)
{
    // Konfigurasi SPI Bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,       // Tidak digunakan
        .quadhd_io_num = -1,       // Tidak digunakan
        .max_transfer_sz = MAX_TRANSFER_SIZE,
    };

    // Inisialisasi SPI Bus dengan DMA
    esp_err_t ret = spi_bus_initialize(SPI_HOST_USED, &bus_cfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI bus berhasil diinisialisasi");

    // Konfigurasi SPI Device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,
        .mode = SPI_MODE,           // CPOL=0, CPHA=0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_USED, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambahkan SPI device: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI device berhasil ditambahkan");

    return ESP_OK;
}

/**
 * Mengirim dan menerima data SPI (full-duplex)
 * @param tx_buf  Buffer data yang dikirim
 * @param rx_buf  Buffer data yang diterima
 * @param len     Panjang data dalam byte
 */
static esp_err_t spi_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
    spi_transaction_t trans = {
        .length = len * 8,          // Panjang dalam bit
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    return spi_device_transmit(spi_dev, &trans);
}

/**
 * Cetak data buffer dalam format hex
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
 * Test loopback: kirim data via MOSI, terima balik via MISO
 */
static void spi_loopback_test(int test_num, const uint8_t *test_data, size_t len)
{
    uint8_t rx_buffer[MAX_TRANSFER_SIZE] = {0};

    printf("\n--- Test #%d: Kirim %d byte ---\n", test_num, (int)len);
    print_buffer("TX", test_data, len);

    esp_err_t ret = spi_transfer(test_data, rx_buffer, len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Transfer gagal: %s", esp_err_to_name(ret));
        return;
    }

    print_buffer("RX", rx_buffer, len);

    // Verifikasi: bandingkan TX dan RX
    bool match = (memcmp(test_data, rx_buffer, len) == 0);
    printf("  Hasil: %s\n", match ? "✅ PASS - Data cocok!" : "❌ FAIL - Data tidak cocok!");

    if (!match) {
        printf("  Detail perbedaan:\n");
        for (size_t i = 0; i < len; i++) {
            if (test_data[i] != rx_buffer[i]) {
                printf("    Byte[%d]: TX=0x%02X, RX=0x%02X\n", (int)i, test_data[i], rx_buffer[i]);
            }
        }
    }
}

void app_main(void)
{
    printf("\n============================================================\n");
    printf("  ESP32 SPI Loopback Test\n");
    printf("  Hubungkan MOSI (GPIO%d) ke MISO (GPIO%d)\n", PIN_NUM_MOSI, PIN_NUM_MISO);
    printf("============================================================\n");

    // Inisialisasi SPI
    if (spi_init() != ESP_OK) {
        ESP_LOGE(TAG, "Inisialisasi SPI gagal, program berhenti");
        return;
    }

    // Test 1: Data ascending
    uint8_t test1[TEST_DATA_LEN];
    for (int i = 0; i < TEST_DATA_LEN; i++) {
        test1[i] = i;
    }
    spi_loopback_test(1, test1, TEST_DATA_LEN);

    // Test 2: Data 0xAA 0x55 pattern (alternating bits)
    uint8_t test2[TEST_DATA_LEN];
    for (int i = 0; i < TEST_DATA_LEN; i++) {
        test2[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }
    spi_loopback_test(2, test2, TEST_DATA_LEN);

    // Test 3: Data 0xFF (semua bit high)
    uint8_t test3[TEST_DATA_LEN];
    memset(test3, 0xFF, TEST_DATA_LEN);
    spi_loopback_test(3, test3, TEST_DATA_LEN);

    // Test 4: Data 0x00 (semua bit low)
    uint8_t test4[TEST_DATA_LEN];
    memset(test4, 0x00, TEST_DATA_LEN);
    spi_loopback_test(4, test4, TEST_DATA_LEN);

    // Test 5: Data string ASCII
    const char *msg = "Hello SPI ESP32!";
    spi_loopback_test(5, (const uint8_t *)msg, strlen(msg));

    printf("\n============================================================\n");
    printf("  Semua test selesai!\n");
    printf("  Jika semua PASS: koneksi loopback benar\n");
    printf("  Jika ada FAIL: periksa koneksi MOSI -> MISO\n");
    printf("============================================================\n");

    // Loop forever
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
