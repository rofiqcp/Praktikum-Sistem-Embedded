/*
 * ESP32_12_I2C_Error_Recovery
 * Modul 06 - I2C & Sensor
 *
 * Deskripsi: Deteksi dan recovery dari error I2C.
 *            - Deteksi SDA stuck (bus busy)
 *            - Recovery: toggle SCL 9x via GPIO, kirim STOP
 *            - Retry logic (max 3 percobaan)
 *            - Penanganan NAK dan timeout error
 *
 * Koneksi Pin:
 *   ESP32:    SDA=GPIO21, SCL=GPIO22
 *   S2/S3:   SDA=GPIO8,  SCL=GPIO9
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
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
#define I2C_TIMEOUT_MS      1000

/* ======================== Konfigurasi Recovery ======================== */
#define TARGET_ADDR         0x76    /* Alamat perangkat target (BMP280) */
#define TARGET_REG          0xD0    /* Register untuk test read (Chip ID) */
#define MAX_RETRIES         3       /* Maksimum percobaan ulang */
#define SCL_TOGGLE_COUNT    9       /* Jumlah toggle SCL untuk recovery */
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
static bool i2c_driver_installed = false;

/* ======================== Inisialisasi I2C Master ======================== */
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal konfigurasi I2C: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal install driver I2C: %s", esp_err_to_name(err));
    } else {
        i2c_driver_installed = true;
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

/* ======================== Cek Status Bus I2C ======================== */
static bool i2c_bus_is_busy(void)
{
    /*
     * Cek apakah SDA dalam keadaan LOW (stuck).
     * Jika SDA rendah saat tidak ada transaksi, bus mungkin stuck.
     */
    /* Sementara matikan driver untuk baca langsung GPIO */
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
     * Prosedur recovery:
     * 1. Hapus driver I2C
     * 2. Konfigurasi SCL sebagai GPIO output
     * 3. Konfigurasi SDA sebagai GPIO input (dengan pull-up)
     * 4. Toggle SCL 9 kali (untuk melepas slave yang stuck)
     * 5. Kirim kondisi STOP manual
     * 6. Inisialisasi ulang driver I2C
     */

    /* Step 1: Hapus driver I2C */
    if (i2c_driver_installed) {
        ESP_LOGI(TAG, "[1/6] Menghapus driver I2C...");
        esp_err_t err = i2c_driver_delete(I2C_PORT);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Gagal hapus driver: %s", esp_err_to_name(err));
            return err;
        }
        i2c_driver_installed = false;
    }

    /* Step 2: Konfigurasi SCL sebagai GPIO output */
    ESP_LOGI(TAG, "[2/6] Konfigurasi SCL (GPIO%d) sebagai output...", I2C_SCL_PIN);
    gpio_config_t scl_conf = {
        .pin_bit_mask = (1ULL << I2C_SCL_PIN),
        .mode = GPIO_MODE_OUTPUT_OD,  /* Open-drain seperti I2C */
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&scl_conf);

    /* Step 3: Konfigurasi SDA sebagai GPIO input */
    ESP_LOGI(TAG, "[3/6] Konfigurasi SDA (GPIO%d) sebagai input...", I2C_SDA_PIN);
    gpio_config_t sda_conf = {
        .pin_bit_mask = (1ULL << I2C_SDA_PIN),
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sda_conf);
    gpio_set_level(I2C_SDA_PIN, 1);  /* Lepas SDA (high) */

    /* Step 4: Toggle SCL 9 kali */
    ESP_LOGI(TAG, "[4/6] Toggle SCL %d kali untuk melepas slave...", SCL_TOGGLE_COUNT);
    for (int i = 0; i < SCL_TOGGLE_COUNT; i++) {
        gpio_set_level(I2C_SCL_PIN, 0);  /* SCL LOW */
        esp_rom_delay_us(5);              /* Tahan 5us */
        gpio_set_level(I2C_SCL_PIN, 1);  /* SCL HIGH */
        esp_rom_delay_us(5);              /* Tahan 5us */

        /* Cek apakah SDA sudah lepas */
        int sda = gpio_get_level(I2C_SDA_PIN);
        ESP_LOGI(TAG, "  Toggle %d: SCL↑ SDA=%d", i + 1, sda);

        if (sda == 1 && i > 0) {
            ESP_LOGI(TAG, "  SDA sudah HIGH setelah %d toggle!", i + 1);
            break;
        }
    }

    /* Step 5: Kirim kondisi STOP manual */
    ESP_LOGI(TAG, "[5/6] Mengirim kondisi STOP manual...");
    /* STOP = SDA LOW->HIGH saat SCL HIGH */
    gpio_set_level(I2C_SDA_PIN, 0);  /* SDA LOW */
    esp_rom_delay_us(5);
    gpio_set_level(I2C_SCL_PIN, 1);  /* SCL HIGH */
    esp_rom_delay_us(5);
    gpio_set_level(I2C_SDA_PIN, 1);  /* SDA HIGH (STOP condition) */
    esp_rom_delay_us(5);

    /* Verifikasi SDA sudah lepas */
    int sda_level = gpio_get_level(I2C_SDA_PIN);
    int scl_level = gpio_get_level(I2C_SCL_PIN);
    ESP_LOGI(TAG, "  Setelah STOP: SDA=%d, SCL=%d", sda_level, scl_level);

    /* Step 6: Inisialisasi ulang driver I2C */
    ESP_LOGI(TAG, "[6/6] Inisialisasi ulang driver I2C...");
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Reset konfigurasi GPIO sebelum re-init I2C */
    gpio_reset_pin(I2C_SDA_PIN);
    gpio_reset_pin(I2C_SCL_PIN);

    esp_err_t err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Gagal inisialisasi ulang I2C: %s", esp_err_to_name(err));
        return err;
    }

    /* Verifikasi recovery berhasil */
    vTaskDelay(pdMS_TO_TICKS(50));
    uint8_t test_data = 0;
    err = i2c_read_reg(TARGET_ADDR, TARGET_REG, &test_data, 1);
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
static esp_err_t i2c_read_with_retry(uint8_t dev_addr, uint8_t reg_addr,
                                      uint8_t *data, size_t len)
{
    esp_err_t err;

    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        stats.total_transactions++;

        err = i2c_read_reg(dev_addr, reg_addr, data, len);

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
        printf("ERROR,%s,0x%02X,0x%02X,%d\n", error_type, dev_addr, reg_addr, attempt + 1);

        /* Tunggu sebelum retry */
        vTaskDelay(pdMS_TO_TICKS(50 * (attempt + 1))); /* Backoff eksponensial sederhana */

        /* Jika timeout, coba recovery */
        if (err == ESP_ERR_TIMEOUT && attempt < MAX_RETRIES - 1) {
            ESP_LOGW(TAG, "Timeout terdeteksi, mencoba bus recovery...");
            i2c_bus_recovery();
        }
    }

    ESP_LOGE(TAG, "Semua %d percobaan GAGAL untuk addr 0x%02X reg 0x%02X",
             MAX_RETRIES, dev_addr, reg_addr);
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
        esp_err_t err = i2c_read_with_retry(TARGET_ADDR, TARGET_REG, data, 1);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Berhasil baca: 0x%02X", data[0]);
            printf("DATA,NORMAL,0x%02X,0x%02X,OK,0x%02X\n",
                   TARGET_ADDR, TARGET_REG, data[0]);
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Test 2: Baca dari alamat tidak valid (simulasi NACK) */
        ESP_LOGI(TAG, "--- Test 2: Simulasi NACK (addr=0x%02X) ---", INVALID_ADDR);
        err = i2c_read_with_retry(INVALID_ADDR, 0x00, data, 1);
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
        err = i2c_read_with_retry(TARGET_ADDR, 0x88, data, 4);
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
