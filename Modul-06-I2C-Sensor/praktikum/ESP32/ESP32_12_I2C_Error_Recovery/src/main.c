/*
 * ESP32_12_I2C_Error_Recovery
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Deteksi dan recovery dari error I2C.
 *            - Deteksi SDA stuck (bus busy)
 *            - Recovery: i2c_master_bus_reset() pada API baru
 *            - Retry logic (max 3 percobaan)
 *            - Penanganan NAK dan timeout error
 *            Menggunakan ESP-IDF v5.x I2C Master API (driver/i2c_master.h)
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

static const char *TAG = "I2C_RECOVERY";

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
#define I2C_FREQ_HZ         100000

/* ======================== Konfigurasi Recovery ======================== */
#define TARGET_ADDR         0x76    /* Alamat perangkat target (BMP280) */
#define TARGET_REG          0xD0    /* Register untuk test read (Chip ID) */
#define MAX_RETRIES         3       /* Maksimum percobaan ulang */
#define INVALID_ADDR        0x7E    /* Alamat tidak valid untuk simulasi NACK */

/* ======================== Statistik Error ======================== */
typedef struct {
    uint32_t total_transactions;    /* Total transaksi */
    uint32_t success_count;         /* Jumlah berhasil */
    uint32_t nack_errors;           /* Error NACK */
    uint32_t timeout_errors;        /* Error timeout */
    uint32_t bus_busy_errors;       /* Error bus sibuk */
    uint32_t other_errors;          /* Error lainnya */
    uint32_t recovery_attempts;     /* Jumlah percobaan recovery */
    uint32_t recovery_success;      /* Recovery berhasil */
    uint32_t retry_success;         /* Berhasil setelah retry */
} error_stats_t;

static error_stats_t stats = {0};

/* Handle I2C bus dan device */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t target_dev_handle;
static i2c_master_dev_handle_t invalid_dev_handle;

/* ======================== Inisialisasi I2C Master ======================== */
static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membuat I2C master bus: %s", esp_err_to_name(err));
        return err;
    }

    /* Tambahkan device target (BMP280) ke bus */
    i2c_device_config_t target_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TARGET_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(bus_handle, &target_cfg, &target_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambah device target: %s", esp_err_to_name(err));
        return err;
    }

    /* Tambahkan device invalid untuk simulasi NACK */
    i2c_device_config_t invalid_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = INVALID_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(bus_handle, &invalid_cfg, &invalid_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal menambah device invalid: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/* ======================== Tulis Register I2C ======================== */
static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                                uint8_t *data, size_t len)
{
    uint8_t write_buf[1 + len];
    write_buf[0] = reg_addr;
    if (data != NULL && len > 0) {
        memcpy(&write_buf[1], data, len);
    }
    return i2c_master_transmit(dev_handle, write_buf, 1 + len, -1);
}

/* ======================== Baca Register I2C ======================== */
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                               uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, -1);
}

/* ======================== Cek Status Bus I2C ======================== */
static bool i2c_bus_is_busy(void)
{
    /*
     * Cek apakah SDA dalam keadaan LOW (stuck).
     * Jika SDA rendah saat tidak ada transaksi, bus mungkin stuck.
     */
    int sda_level = gpio_get_level(I2C_SDA_PIN);
    int scl_level = gpio_get_level(I2C_SCL_PIN);

    ESP_LOGI(TAG, "Status bus: SDA=%d, SCL=%d", sda_level, scl_level);

    /* Bus stuck jika SDA LOW saat SCL HIGH (menandakan slave menahan SDA) */
    if (sda_level == 0) {
        ESP_LOGW(TAG, "SDA dalam keadaan LOW - bus mungkin stuck!");
        return true;
    }
    return false;
}

/* ======================== Recovery Bus I2C ======================== */
static esp_err_t i2c_bus_recovery(void)
{
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "========== MEMULAI RECOVERY BUS I2C ==========");
    stats.recovery_attempts++;

    /*
     * Prosedur recovery menggunakan API baru:
     * 1. Panggil i2c_master_bus_reset() untuk reset bus
     *    (API baru menangani toggle SCL dan STOP secara internal)
     * 2. Tunggu stabilisasi
     * 3. Verifikasi recovery berhasil dengan test read
     */

    /* Step 1: Reset bus I2C menggunakan API baru */
    ESP_LOGI(TAG, "[1/3] Melakukan reset bus I2C via i2c_master_bus_reset()...");
    ESP_LOGI(TAG, "  (API baru menangani toggle SCL 9x dan STOP secara internal)");
    esp_err_t err = i2c_master_bus_reset(bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal reset bus: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "  Reset bus berhasil dikirim");

    /* Step 2: Tunggu stabilisasi */
    ESP_LOGI(TAG, "[2/3] Menunggu stabilisasi...");
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Step 3: Verifikasi recovery berhasil */
    ESP_LOGI(TAG, "[3/3] Verifikasi recovery dengan test read...");
    uint8_t test_data = 0;
    err = i2c_read_reg(target_dev_handle, TARGET_REG, &test_data, 1);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Recovery BERHASIL! Test read: 0x%02X", test_data);
        stats.recovery_success++;
        printf("RECOVERY,SUCCESS,%lu,%lu\n",
               (unsigned long)stats.recovery_attempts,
               (unsigned long)stats.recovery_success);
    } else {
        ESP_LOGE(TAG, "Recovery GAGAL! Test read error: %s", esp_err_to_name(err));
        printf("RECOVERY,FAILED,%lu,%lu\n",
               (unsigned long)stats.recovery_attempts,
               (unsigned long)stats.recovery_success);
    }

    ESP_LOGW(TAG, "========== RECOVERY SELESAI ==========\n");
    return err;
}

/* ======================== Baca dengan Retry Logic ======================== */
static esp_err_t i2c_read_with_retry(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr,
                                      uint8_t *data, size_t len, uint8_t dev_addr_log)
{
    esp_err_t err;

    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        stats.total_transactions++;

        err = i2c_read_reg(dev_handle, reg_addr, data, len);

        if (err == ESP_OK) {
            stats.success_count++;
            if (attempt > 0) {
                stats.retry_success++;
                ESP_LOGI(TAG, "Berhasil pada percobaan ke-%d", attempt + 1);
            }
            return ESP_OK;
        }

        /* Klasifikasi error */
        const char *error_type;
        switch (err) {
            case ESP_FAIL:
                stats.nack_errors++;
                error_type = "NACK";
                break;
            case ESP_ERR_TIMEOUT:
                stats.timeout_errors++;
                error_type = "TIMEOUT";
                break;
            default:
                stats.other_errors++;
                error_type = "OTHER";
                break;
        }

        ESP_LOGW(TAG, "Percobaan %d/%d GAGAL: %s (%s)",
                 attempt + 1, MAX_RETRIES, error_type, esp_err_to_name(err));
        printf("ERROR,%s,0x%02X,0x%02X,%d\n", error_type, dev_addr_log, reg_addr, attempt + 1);

        /* Tunggu sebelum retry */
        vTaskDelay(pdMS_TO_TICKS(50 * (attempt + 1))); /* Backoff eksponensial sederhana */

        /* Jika timeout, coba recovery */
        if (err == ESP_ERR_TIMEOUT && attempt < MAX_RETRIES - 1) {
            ESP_LOGW(TAG, "Timeout terdeteksi, mencoba bus recovery...");
            i2c_bus_recovery();
        }
    }

    ESP_LOGE(TAG, "Semua %d percobaan GAGAL untuk addr 0x%02X reg 0x%02X",
             MAX_RETRIES, dev_addr_log, reg_addr);
    return err;
}

/* ======================== Cetak Statistik ======================== */
static void print_stats(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║        STATISTIK ERROR I2C               ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ Total transaksi    : %-10lu          ║", (unsigned long)stats.total_transactions);
    ESP_LOGI(TAG, "║ Berhasil           : %-10lu          ║", (unsigned long)stats.success_count);
    ESP_LOGI(TAG, "║ Error NACK         : %-10lu          ║", (unsigned long)stats.nack_errors);
    ESP_LOGI(TAG, "║ Error timeout      : %-10lu          ║", (unsigned long)stats.timeout_errors);
    ESP_LOGI(TAG, "║ Error bus busy     : %-10lu          ║", (unsigned long)stats.bus_busy_errors);
    ESP_LOGI(TAG, "║ Error lainnya      : %-10lu          ║", (unsigned long)stats.other_errors);
    ESP_LOGI(TAG, "║ Recovery attempts  : %-10lu          ║", (unsigned long)stats.recovery_attempts);
    ESP_LOGI(TAG, "║ Recovery berhasil  : %-10lu          ║", (unsigned long)stats.recovery_success);
    ESP_LOGI(TAG, "║ Retry berhasil     : %-10lu          ║", (unsigned long)stats.retry_success);
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");

    float success_rate = 0;
    if (stats.total_transactions > 0) {
        success_rate = (float)stats.success_count / (float)stats.total_transactions * 100.0f;
    }
    ESP_LOGI(TAG, "Tingkat keberhasilan: %.1f%%", success_rate);

    printf("STATS,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.1f\n",
           (unsigned long)stats.total_transactions,
           (unsigned long)stats.success_count,
           (unsigned long)stats.nack_errors,
           (unsigned long)stats.timeout_errors,
           (unsigned long)stats.bus_busy_errors,
           (unsigned long)stats.other_errors,
           (unsigned long)stats.recovery_attempts,
           (unsigned long)stats.recovery_success,
           (unsigned long)stats.retry_success,
           success_rate);
}

/* ======================== Task Recovery Demo ======================== */
static void recovery_task(void *pvParameters)
{
    uint8_t data[4] = {0};
    uint32_t cycle = 0;

    while (1) {
        cycle++;
        ESP_LOGI(TAG, "\n====== SIKLUS #%lu ======", (unsigned long)cycle);

        /* Test 1: Baca normal dari perangkat yang valid */
        ESP_LOGI(TAG, "--- Test 1: Pembacaan normal (addr=0x%02X) ---", TARGET_ADDR);
        esp_err_t err = i2c_read_with_retry(target_dev_handle, TARGET_REG, data, 1, TARGET_ADDR);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Berhasil baca: 0x%02X", data[0]);
            printf("DATA,NORMAL,0x%02X,0x%02X,OK,0x%02X\n",
                   TARGET_ADDR, TARGET_REG, data[0]);
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Test 2: Baca dari alamat tidak valid (simulasi NACK) */
        ESP_LOGI(TAG, "--- Test 2: Simulasi NACK (addr=0x%02X) ---", INVALID_ADDR);
        err = i2c_read_with_retry(invalid_dev_handle, 0x00, data, 1, INVALID_ADDR);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error yang diharapkan: NACK pada alamat tidak valid");
            printf("DATA,NACK_TEST,0x%02X,0x00,NACK,N/A\n", INVALID_ADDR);
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Test 3: Cek status bus */
        ESP_LOGI(TAG, "--- Test 3: Pengecekan status bus ---");
        bool busy = i2c_bus_is_busy();
        if (busy) {
            stats.bus_busy_errors++;
            ESP_LOGW(TAG, "Bus sibuk terdeteksi! Memulai recovery...");
            i2c_bus_recovery();
        } else {
            ESP_LOGI(TAG, "Bus dalam kondisi normal (idle)");
        }
        printf("DATA,BUS_CHECK,%s\n", busy ? "BUSY" : "IDLE");

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Test 4: Multi-byte read dengan retry */
        ESP_LOGI(TAG, "--- Test 4: Multi-byte read ---");
        err = i2c_read_with_retry(target_dev_handle, 0x88, data, 4, TARGET_ADDR);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Berhasil baca 4 byte: 0x%02X 0x%02X 0x%02X 0x%02X",
                     data[0], data[1], data[2], data[3]);
            printf("DATA,MULTI_READ,0x%02X,0x88,OK,0x%02X 0x%02X 0x%02X 0x%02X\n",
                   TARGET_ADDR, data[0], data[1], data[2], data[3]);
        }

        /* Test 5: Recovery paksa setiap 10 siklus (untuk demonstrasi) */
        if (cycle % 10 == 0) {
            ESP_LOGI(TAG, "--- Test 5: Recovery paksa (demonstrasi) ---");
            i2c_bus_recovery();
        }

        /* Cetak statistik setiap 5 siklus */
        if (cycle % 5 == 0) {
            print_stats();
        }

        /* Tunggu 3 detik sebelum siklus berikutnya */
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ======================== Fungsi Utama ======================== */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 I2C Error Recovery Demo ===");
    ESP_LOGI(TAG, "SDA=GPIO%d, SCL=GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
    ESP_LOGI(TAG, "Target: 0x%02X, Max retry: %d", TARGET_ADDR, MAX_RETRIES);

    /* Inisialisasi I2C master */
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C master berhasil diinisialisasi");

    printf("HDR,type,addr,reg,status,data\n");

    /* Buat task recovery demo */
    xTaskCreate(recovery_task, "recovery_task", 4096, NULL, 5, NULL);
}
