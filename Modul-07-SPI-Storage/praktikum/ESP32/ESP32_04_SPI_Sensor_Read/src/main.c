/**
 * ============================================================
 * ESP32_04_SPI_Sensor_Read
 * ============================================================
 * Deskripsi  : Simulasi pembacaan sensor SPI menggunakan protokol
 *              register read/write mirip BME280 (sensor suhu,
 *              tekanan, kelembaban)
 * Board      : ESP32 DOIT DevKit V1 / Wemos Lolin S2 / ESP32-S3
 * Framework  : ESP-IDF
 * 
 * Hardware yang dibutuhkan:
 *   - 1x ESP32 DevKit V1 (atau varian S2/S3)
 *   - 1x Kabel USB micro/Type-C
 *   - (Opsional) 1x Kabel jumper MOSI-MISO untuk loopback
 * 
 * CATATAN PENTING:
 *   Program ini MENSIMULASIKAN komunikasi sensor SPI.
 *   Karena tidak ada sensor fisik yang terhubung, data yang
 *   diterima akan berupa 0x00 atau 0xFF. Program ini fokus
 *   mendemonstrasikan PROTOKOL komunikasi SPI sensor:
 *   1. Cara mengirim alamat register dengan bit R/W
 *   2. Cara membaca data dari register sensor
 *   3. Cara menulis konfigurasi ke register sensor
 *   4. Urutan komunikasi (address phase + data phase)
 * 
 * Koneksi (jika menggunakan sensor BME280 asli):
 *   ESP32 GPIO13 (MOSI) ---> BME280 SDI
 *   ESP32 GPIO12 (MISO) ---> BME280 SDO
 *   ESP32 GPIO14 (SCLK) ---> BME280 SCK
 *   ESP32 GPIO15 (CS)   ---> BME280 CSB
 *   ESP32 3.3V           ---> BME280 VCC
 *   ESP32 GND            ---> BME280 GND
 * ============================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "config.h"

static const char *TAG = "SPI_SENSOR";

// Handle SPI device (mewakili sensor pada bus SPI)
static spi_device_handle_t sensor_dev;

// ============================================================
// Simulasi register map sensor (dalam memori ESP32)
// Pada sensor asli, register ini ada di dalam IC sensor.
// Di sini kita simulasikan agar bisa mendemonstrasikan
// bagaimana protokol SPI sensor bekerja.
// ============================================================
static uint8_t simulated_registers[256] = {0};

/**
 * Inisialisasi register map simulasi sensor BME280
 * Mengisi register dengan nilai default seperti pada datasheet
 */
static void init_simulated_registers(void)
{
    // Bersihkan semua register
    memset(simulated_registers, 0, sizeof(simulated_registers));

    // Chip ID - identifikasi sensor (BME280 = 0x60)
    simulated_registers[REG_CHIP_ID] = SIMULATED_CHIP_ID;

    // Status register - sensor idle
    simulated_registers[REG_STATUS] = 0x00;

    // Ctrl_meas register - default: sleep mode
    simulated_registers[REG_CTRL_MEAS] = 0x00;

    // Config register - default
    simulated_registers[REG_CONFIG] = 0x00;

    ESP_LOGI(TAG, "Register map simulasi diinisialisasi");
    ESP_LOGI(TAG, "  Chip ID (0x%02X) = 0x%02X", REG_CHIP_ID,
             simulated_registers[REG_CHIP_ID]);
}

/**
 * Update data sensor simulasi dengan nilai acak realistis
 * Menghasilkan data suhu, tekanan, dan kelembaban yang
 * mirip dengan output sensor BME280
 */
static void update_simulated_sensor_data(void)
{
    // Simulasi suhu: 20.0 - 35.0 derajat Celsius
    // BME280 menyimpan suhu dalam format 20-bit (raw value)
    // Rumus konversi: T = raw / 100.0 (disederhanakan)
    uint32_t temp_raw = 2000 + (esp_random() % 1500);  // 20.00 - 35.00
    simulated_registers[REG_TEMP_MSB] = (temp_raw >> 12) & 0xFF;
    simulated_registers[REG_TEMP_LSB] = (temp_raw >> 4) & 0xFF;
    simulated_registers[REG_TEMP_XLSB] = (temp_raw << 4) & 0xF0;

    // Simulasi tekanan: 950 - 1050 hPa
    // Format 20-bit raw value
    uint32_t press_raw = 95000 + (esp_random() % 10000);
    simulated_registers[REG_PRESS_MSB] = (press_raw >> 12) & 0xFF;
    simulated_registers[REG_PRESS_LSB] = (press_raw >> 4) & 0xFF;
    simulated_registers[REG_PRESS_XLSB] = (press_raw << 4) & 0xF0;

    // Simulasi kelembaban: 30 - 80%
    // Format 16-bit raw value
    uint16_t hum_raw = 3000 + (esp_random() % 5000);
    simulated_registers[REG_HUM_MSB] = (hum_raw >> 8) & 0xFF;
    simulated_registers[REG_HUM_LSB] = hum_raw & 0xFF;

    // Status: data sudah siap (not measuring, not updating)
    simulated_registers[REG_STATUS] = 0x00;
}

/**
 * Inisialisasi SPI bus dan device untuk komunikasi sensor
 * @return ESP_OK jika berhasil
 */
static esp_err_t spi_sensor_init(void)
{
    // Konfigurasi pin SPI Bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MAX_TRANSFER_SIZE,
    };

    // Inisialisasi SPI Bus
    esp_err_t ret = spi_bus_initialize(SPI_HOST_USED, &bus_cfg, DMA_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Konfigurasi SPI Device (mewakili sensor)
    // Sensor BME280 menggunakan Mode 0 dengan clock max 10 MHz
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED_HZ,
        .mode = SPI_MODE,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_USED, &dev_cfg, &sensor_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambahkan sensor device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI sensor device berhasil diinisialisasi");
    return ESP_OK;
}

/**
 * Baca satu register dari sensor melalui SPI
 * 
 * Protokol BME280 SPI Read:
 *   1. Master kirim alamat register dengan bit 7 = 1 (READ)
 *   2. Sensor merespon dengan data register pada byte berikutnya
 *   3. Total transaksi = 2 byte (1 alamat + 1 data)
 * 
 * Pada simulasi ini, kita langsung mengambil dari register map
 * karena tidak ada sensor fisik.
 * 
 * @param reg_addr  Alamat register yang dibaca (7-bit)
 * @param data      Pointer untuk menyimpan data yang dibaca
 * @return ESP_OK jika berhasil
 */
static esp_err_t sensor_read_register(uint8_t reg_addr, uint8_t *data)
{
    // Siapkan buffer transaksi SPI
    // Byte 0: Alamat register dengan bit R/W = 1 (READ)
    // Byte 1: Dummy byte (sensor akan mengirim data pada byte ini)
    uint8_t tx_buf[2] = { reg_addr | SPI_READ_BIT, 0x00 };
    uint8_t rx_buf[2] = {0};

    // Konfigurasi transaksi SPI full-duplex
    spi_transaction_t trans = {
        .length = 16,           // 2 byte = 16 bit
        .tx_buffer = tx_buf,    // Data yang dikirim ke sensor
        .rx_buffer = rx_buf,    // Data yang diterima dari sensor
    };

    // Eksekusi transaksi SPI
    esp_err_t ret = spi_device_transmit(sensor_dev, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal baca register 0x%02X: %s", reg_addr,
                 esp_err_to_name(ret));
        return ret;
    }

    // Pada sensor asli, data ada di rx_buf[1]
    // Karena ini simulasi, kita ambil dari register map
    *data = simulated_registers[reg_addr];

    ESP_LOGD(TAG, "Read  reg[0x%02X] -> 0x%02X (TX: 0x%02X 0x%02X, RX: 0x%02X 0x%02X)",
             reg_addr, *data, tx_buf[0], tx_buf[1], rx_buf[0], rx_buf[1]);

    return ESP_OK;
}

/**
 * Tulis satu register ke sensor melalui SPI
 * 
 * Protokol BME280 SPI Write:
 *   1. Master kirim alamat register dengan bit 7 = 0 (WRITE)
 *   2. Master kirim data yang akan ditulis
 *   3. Total transaksi = 2 byte (1 alamat + 1 data)
 * 
 * @param reg_addr  Alamat register yang ditulis (7-bit, tanpa R/W bit)
 * @param data      Data yang akan ditulis ke register
 * @return ESP_OK jika berhasil
 */
static esp_err_t sensor_write_register(uint8_t reg_addr, uint8_t data)
{
    // Siapkan buffer transaksi SPI
    // Byte 0: Alamat register dengan bit R/W = 0 (WRITE)
    //         Pada BME280, alamat write = (addr & 0x7F)
    // Byte 1: Data yang ditulis ke register
    uint8_t tx_buf[2] = { (reg_addr & 0x7F) | SPI_WRITE_BIT, data };
    uint8_t rx_buf[2] = {0};

    spi_transaction_t trans = {
        .length = 16,           // 2 byte = 16 bit
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(sensor_dev, &trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal tulis register 0x%02X: %s", reg_addr,
                 esp_err_to_name(ret));
        return ret;
    }

    // Simulasi: simpan data ke register map
    simulated_registers[reg_addr] = data;

    ESP_LOGD(TAG, "Write reg[0x%02X] <- 0x%02X", reg_addr, data);

    return ESP_OK;
}

/**
 * Baca beberapa register berurutan (burst read)
 * 
 * Pada BME280, setelah mengirim alamat register pertama,
 * sensor akan otomatis increment alamat untuk setiap
 * byte clock berikutnya (auto-increment mode).
 * 
 * @param start_addr  Alamat register awal
 * @param data        Buffer untuk menyimpan data
 * @param len         Jumlah register yang dibaca
 * @return ESP_OK jika berhasil
 */
static esp_err_t sensor_burst_read(uint8_t start_addr, uint8_t *data, size_t len)
{
    // Buffer: 1 byte alamat + len byte data
    size_t total_len = 1 + len;
    uint8_t *tx_buf = calloc(total_len, sizeof(uint8_t));
    uint8_t *rx_buf = calloc(total_len, sizeof(uint8_t));

    if (!tx_buf || !rx_buf) {
        ESP_LOGE(TAG, "Gagal alokasi buffer burst read");
        free(tx_buf);
        free(rx_buf);
        return ESP_ERR_NO_MEM;
    }

    // Byte pertama: alamat register dengan bit READ
    tx_buf[0] = start_addr | SPI_READ_BIT;
    // Byte selanjutnya: dummy (sensor akan mengirim data)

    spi_transaction_t trans = {
        .length = total_len * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t ret = spi_device_transmit(sensor_dev, &trans);
    if (ret == ESP_OK) {
        // Simulasi: copy data dari register map
        for (size_t i = 0; i < len; i++) {
            data[i] = simulated_registers[start_addr + i];
        }
    }

    free(tx_buf);
    free(rx_buf);

    return ret;
}

/**
 * Baca dan verifikasi Chip ID sensor
 * Langkah pertama dalam komunikasi sensor: pastikan sensor terhubung
 * dengan membaca register Chip ID
 * 
 * @return true jika chip ID sesuai dengan yang diharapkan
 */
static bool verify_chip_id(void)
{
    uint8_t chip_id = 0;
    esp_err_t ret = sensor_read_register(REG_CHIP_ID, &chip_id);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membaca Chip ID");
        return false;
    }

    printf("  Chip ID: 0x%02X (diharapkan: 0x%02X)\n", chip_id, SIMULATED_CHIP_ID);

    if (chip_id == SIMULATED_CHIP_ID) {
        printf("  Sensor teridentifikasi: BME280\n");
        return true;
    } else {
        printf("  PERINGATAN: Chip ID tidak dikenali!\n");
        return false;
    }
}

/**
 * Konfigurasi sensor untuk mode pengukuran
 * Menulis register kontrol untuk mengaktifkan pengukuran
 * suhu, tekanan, dan kelembaban
 */
static esp_err_t configure_sensor(void)
{
    printf("\n  Konfigurasi sensor...\n");

    // 1. Soft reset sensor terlebih dahulu
    printf("  -> Soft reset (write 0x%02X ke reg 0x%02X)\n",
           RESET_VALUE, REG_RESET);
    esp_err_t ret = sensor_write_register(REG_RESET, RESET_VALUE);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(10));  // Tunggu reset selesai

    // Re-inisialisasi register setelah reset
    init_simulated_registers();

    // 2. Set config register
    //    Bit[7:5] = t_sb (standby time) = 000 (0.5ms)
    //    Bit[4:2] = filter = 100 (koefisien 16)
    //    Bit[0]   = spi3w_en = 0 (4-wire SPI)
    uint8_t config_val = 0x10;  // filter = 16, standby = 0.5ms
    printf("  -> Config register (write 0x%02X ke reg 0x%02X)\n",
           config_val, REG_CONFIG);
    ret = sensor_write_register(REG_CONFIG, config_val);
    if (ret != ESP_OK) return ret;

    // 3. Set ctrl_meas register (mengaktifkan pengukuran)
    //    Bit[7:5] = osrs_t (oversampling suhu) = 010 (x2)
    //    Bit[4:2] = osrs_p (oversampling tekanan) = 101 (x16)
    //    Bit[1:0] = mode = 11 (normal mode)
    uint8_t ctrl_val = 0x57;  // temp x2, press x16, normal mode
    printf("  -> Ctrl_meas register (write 0x%02X ke reg 0x%02X)\n",
           ctrl_val, REG_CTRL_MEAS);
    ret = sensor_write_register(REG_CTRL_MEAS, ctrl_val);
    if (ret != ESP_OK) return ret;

    // Verifikasi konfigurasi yang ditulis
    uint8_t readback = 0;
    sensor_read_register(REG_CTRL_MEAS, &readback);
    printf("  -> Verifikasi ctrl_meas: 0x%02X %s\n", readback,
           (readback == ctrl_val) ? "(OK)" : "(MISMATCH!)");

    sensor_read_register(REG_CONFIG, &readback);
    printf("  -> Verifikasi config   : 0x%02X %s\n", readback,
           (readback == config_val) ? "(OK)" : "(MISMATCH!)");

    printf("  Konfigurasi sensor selesai.\n");
    return ESP_OK;
}

/**
 * Baca data pengukuran dari sensor
 * Membaca register suhu, tekanan, dan kelembaban secara burst
 * kemudian konversi raw data ke nilai yang bisa dipahami
 * 
 * @param read_num  Nomor pembacaan (untuk display)
 */
static void read_sensor_data(int read_num)
{
    printf("\n  --- Pembacaan #%d ---\n", read_num);

    // Cek status sensor terlebih dahulu
    uint8_t status = 0;
    sensor_read_register(REG_STATUS, &status);
    printf("  Status register: 0x%02X", status);
    printf(" (measuring: %s, updating: %s)\n",
           (status & 0x08) ? "Ya" : "Tidak",
           (status & 0x01) ? "Ya" : "Tidak");

    // Update data simulasi (simulasikan pengukuran baru)
    update_simulated_sensor_data();

    // Baca data suhu (3 byte: MSB, LSB, XLSB)
    uint8_t temp_data[3] = {0};
    sensor_burst_read(REG_TEMP_MSB, temp_data, 3);
    // Gabungkan 3 byte menjadi 20-bit raw value
    int32_t temp_raw = ((int32_t)temp_data[0] << 12) |
                       ((int32_t)temp_data[1] << 4) |
                       ((int32_t)temp_data[2] >> 4);
    float temperature = temp_raw / 100.0f;  // Konversi sederhana

    // Baca data tekanan (3 byte: MSB, LSB, XLSB)
    uint8_t press_data[3] = {0};
    sensor_burst_read(REG_PRESS_MSB, press_data, 3);
    int32_t press_raw = ((int32_t)press_data[0] << 12) |
                        ((int32_t)press_data[1] << 4) |
                        ((int32_t)press_data[2] >> 4);
    float pressure = press_raw / 100.0f;  // Konversi ke hPa

    // Baca data kelembaban (2 byte: MSB, LSB)
    uint8_t hum_data[2] = {0};
    sensor_burst_read(REG_HUM_MSB, hum_data, 2);
    uint16_t hum_raw = ((uint16_t)hum_data[0] << 8) | hum_data[1];
    float humidity = hum_raw / 100.0f;  // Konversi ke %RH

    // Tampilkan raw data register
    printf("  Raw register data:\n");
    printf("    Temp  : MSB=0x%02X LSB=0x%02X XLSB=0x%02X (raw: %ld)\n",
           temp_data[0], temp_data[1], temp_data[2], (long)temp_raw);
    printf("    Press : MSB=0x%02X LSB=0x%02X XLSB=0x%02X (raw: %ld)\n",
           press_data[0], press_data[1], press_data[2], (long)press_raw);
    printf("    Hum   : MSB=0x%02X LSB=0x%02X            (raw: %u)\n",
           hum_data[0], hum_data[1], hum_raw);

    // Tampilkan hasil konversi
    printf("  Hasil pengukuran (simulasi):\n");
    printf("    Suhu        : %.2f C\n", temperature);
    printf("    Tekanan     : %.2f hPa\n", pressure);
    printf("    Kelembaban  : %.2f %%RH\n", humidity);
}

/**
 * Demonstrasi dump register map
 * Membaca range register dan tampilkan dalam format tabel
 */
static void dump_register_map(void)
{
    printf("\n  === Dump Register Map Sensor ===");
    printf("\n  Addr  | Nama           | Nilai  | Keterangan\n");
    printf("  ------+----------------+--------+------------------\n");

    // Baca dan tampilkan register-register penting
    struct {
        uint8_t addr;
        const char *name;
        const char *desc;
    } regs[] = {
        { REG_CHIP_ID,    "CHIP_ID   ", "Identifikasi chip"    },
        { REG_STATUS,     "STATUS    ", "Status pengukuran"    },
        { REG_CTRL_MEAS,  "CTRL_MEAS ", "Kontrol pengukuran"   },
        { REG_CONFIG,     "CONFIG    ", "Konfigurasi sensor"   },
        { REG_TEMP_MSB,   "TEMP_MSB  ", "Suhu byte atas"       },
        { REG_TEMP_LSB,   "TEMP_LSB  ", "Suhu byte bawah"      },
        { REG_TEMP_XLSB,  "TEMP_XLSB ", "Suhu byte ekstra"     },
        { REG_PRESS_MSB,  "PRESS_MSB ", "Tekanan byte atas"    },
        { REG_PRESS_LSB,  "PRESS_LSB ", "Tekanan byte bawah"   },
        { REG_PRESS_XLSB, "PRESS_XLSB", "Tekanan byte ekstra"  },
        { REG_HUM_MSB,    "HUM_MSB   ", "Kelembaban byte atas" },
        { REG_HUM_LSB,    "HUM_LSB   ", "Kelembaban byte bawah"},
    };

    for (int i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
        uint8_t val = 0;
        sensor_read_register(regs[i].addr, &val);
        printf("  0x%02X  | %s   | 0x%02X   | %s\n",
               regs[i].addr, regs[i].name, val, regs[i].desc);
    }
}

/**
 * Fungsi utama program
 * Mendemonstrasikan protokol komunikasi SPI sensor:
 * 1. Inisialisasi SPI dan verifikasi Chip ID
 * 2. Konfigurasi sensor (tulis register)
 * 3. Pembacaan data berulang
 * 4. Dump register map
 */
void app_main(void)
{
    printf("\n============================================================\n");
    printf("  ESP32 SPI Sensor Read - Simulasi BME280\n");
    printf("  Demonstrasi protokol komunikasi sensor via SPI\n");
    printf("============================================================\n");

    // Tampilkan informasi konfigurasi
    ESP_LOGI(TAG, "Konfigurasi SPI Sensor:");
    ESP_LOGI(TAG, "  Host     : SPI2_HOST (HSPI)");
    ESP_LOGI(TAG, "  MOSI     : GPIO %d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  MISO     : GPIO %d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  SCLK     : GPIO %d", PIN_NUM_CLK);
    ESP_LOGI(TAG, "  CS       : GPIO %d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  Clock    : %d Hz", SPI_CLOCK_SPEED_HZ);
    ESP_LOGI(TAG, "  Mode     : %d", SPI_MODE);

    // Penjelasan protokol SPI sensor
    printf("\n  Protokol SPI Sensor (BME280-like):\n");
    printf("  - Setiap transaksi dimulai dengan CS LOW\n");
    printf("  - Byte pertama: alamat register + bit R/W\n");
    printf("    * Bit 7 = 1: operasi READ\n");
    printf("    * Bit 7 = 0: operasi WRITE\n");
    printf("  - Byte berikutnya: data (dari sensor atau ke sensor)\n");
    printf("  - Burst read: alamat otomatis increment\n");
    printf("  - Transaksi selesai saat CS HIGH\n");

    // Inisialisasi register map simulasi
    init_simulated_registers();

    // Inisialisasi SPI bus dan device
    if (spi_sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "Inisialisasi SPI sensor gagal, program berhenti");
        return;
    }

    // Langkah 1: Verifikasi Chip ID
    printf("\n============================================================\n");
    printf("  LANGKAH 1: Verifikasi Chip ID\n");
    printf("============================================================\n");
    if (!verify_chip_id()) {
        ESP_LOGE(TAG, "Sensor tidak teridentifikasi!");
        // Lanjutkan saja karena ini simulasi
    }

    // Langkah 2: Konfigurasi sensor
    printf("\n============================================================\n");
    printf("  LANGKAH 2: Konfigurasi Sensor\n");
    printf("============================================================\n");
    if (configure_sensor() != ESP_OK) {
        ESP_LOGE(TAG, "Konfigurasi sensor gagal");
        return;
    }

    // Langkah 3: Dump register map setelah konfigurasi
    printf("\n============================================================\n");
    printf("  LANGKAH 3: Dump Register Map\n");
    printf("============================================================\n");
    dump_register_map();

    // Langkah 4: Baca data sensor secara periodik
    printf("\n============================================================\n");
    printf("  LANGKAH 4: Pembacaan Data Sensor Periodik\n");
    printf("============================================================\n");

    for (int i = 1; i <= SENSOR_READ_COUNT; i++) {
        read_sensor_data(i);

        if (i < SENSOR_READ_COUNT) {
            printf("\n  Menunggu %d ms sebelum pembacaan berikutnya...\n",
                   SENSOR_READ_INTERVAL_MS);
            vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
        }
    }

    // Ringkasan akhir
    printf("\n============================================================\n");
    printf("  RINGKASAN\n");
    printf("============================================================\n");
    printf("  Program ini mendemonstrasikan:\n");
    printf("  1. Inisialisasi SPI bus dan device untuk sensor\n");
    printf("  2. Pembacaan register tunggal (single read)\n");
    printf("  3. Penulisan register (single write)\n");
    printf("  4. Pembacaan register berurutan (burst read)\n");
    printf("  5. Protokol alamat register dengan bit R/W\n");
    printf("  6. Konversi raw data ke nilai terukur\n");
    printf("\n  Untuk menggunakan sensor BME280 asli:\n");
    printf("  - Hubungkan pin SPI sesuai konfigurasi\n");
    printf("  - Hapus fungsi simulasi (init/update_simulated_*)\n");
    printf("  - Data rx_buf[1] dari SPI akan berisi data sensor asli\n");
    printf("============================================================\n");

    // De-inisialisasi
    spi_bus_remove_device(sensor_dev);
    spi_bus_free(SPI_HOST_USED);
    ESP_LOGI(TAG, "SPI bus dibebaskan. Program selesai.");

    // Loop selamanya
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
