/**
 * =============================================================================
 * ESP32_13_I2C_GPIO_Matrix
 * Modul 06 - I2C Sensor | Bonus: Fitur Khusus ESP32
 * =============================================================================
 *
 * FITUR KHUSUS ESP32 YANG TIDAK ADA DI STM32:
 * --------------------------------------------
 * 1. GPIO MATRIX I2C PIN REMAPPING:
 *    - ESP32 bisa assign SDA dan SCL ke HAMPIR SEMUA GPIO pin melalui
 *      GPIO matrix. Ini memberikan fleksibilitas luar biasa dalam desain PCB.
 *    - STM32 I2C TERBATAS pada pin alternate function tertentu:
 *      * I2C1_SDA hanya bisa di PB7 atau PB9 (STM32F411)
 *      * I2C1_SCL hanya bisa di PB6 atau PB8 (STM32F411)
 *      * Jika pin tersebut sudah dipakai, Anda TIDAK BISA pakai I2C1
 *
 * 2. RUNTIME PIN REMAPPING:
 *    - ESP32 bisa menghapus bus I2C dan membuat ulang di pin BERBEDA
 *      secara runtime tanpa reboot
 *    - STM32 pin mapping fixed saat compile time via alternate function
 *
 * 3. DUAL I2C BUS FLEXIBILITY:
 *    - ESP32 punya I2C_NUM_0 dan I2C_NUM_1, masing-masing bisa di pin
 *      MANA SAJA secara simultan
 *    - STM32 juga punya multiple I2C, tapi pin-nya terbatas
 *
 * MENGAPA STM32 TIDAK BISA:
 * - STM32 menggunakan system Alternate Function (AF) yang fixed
 * - Setiap peripheral hanya bisa di-route ke pin tertentu
 * - Mengubah pin I2C di STM32 = ganti konfigurasi AF, terbatas pilihannya
 * - Tidak ada "GPIO matrix" yang bisa remap ke pin mana saja
 *
 * API BARU: driver/i2c_master.h (ESP-IDF v5.x)
 * - i2c_new_master_bus() — membuat bus I2C master
 * - i2c_master_probe() — scan device di alamat tertentu
 * - i2c_del_master_bus() — hapus bus (untuk remap pin)
 *
 * Hardware: Sensor I2C (opsional) — program bisa berjalan tanpa sensor
 * =============================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "I2C_GPIO_MATRIX";

/* ==================== Konfigurasi Pin per Board ==================== */
/*
 * Kita definisikan 3 PASANG pin I2C yang berbeda untuk setiap board.
 * Ini menunjukkan bahwa ESP32 bisa menaruh I2C di pin MANA SAJA!
 *
 * Di STM32F411, I2C1 hanya bisa di:
 *   SCL: PB6 atau PB8
 *   SDA: PB7 atau PB9
 * Sangat terbatas!
 */

#if CONFIG_IDF_TARGET_ESP32
    /* ESP32 Classic - 3 pasang pin I2C yang sangat berbeda */
    #define I2C_PAIR1_SDA   21      // Default SDA
    #define I2C_PAIR1_SCL   22      // Default SCL

    #define I2C_PAIR2_SDA   18      // Pin alternatif 1
    #define I2C_PAIR2_SCL   19      // Pin alternatif 1

    #define I2C_PAIR3_SDA   25      // Pin alternatif 2
    #define I2C_PAIR3_SCL   26      // Pin alternatif 2

    #define BOARD_NAME      "ESP32 Classic"

#elif CONFIG_IDF_TARGET_ESP32S2
    /* ESP32-S2 - Lolin S2 Mini */
    #define I2C_PAIR1_SDA   33      // Default
    #define I2C_PAIR1_SCL   35

    #define I2C_PAIR2_SDA   7       // Alternatif 1
    #define I2C_PAIR2_SCL   9

    #define I2C_PAIR3_SDA   11      // Alternatif 2
    #define I2C_PAIR3_SCL   12

    #define BOARD_NAME      "ESP32-S2"

#elif CONFIG_IDF_TARGET_ESP32S3
    /* ESP32-S3 */
    #define I2C_PAIR1_SDA   8       // Default
    #define I2C_PAIR1_SCL   9

    #define I2C_PAIR2_SDA   17      // Alternatif 1
    #define I2C_PAIR2_SCL   18

    #define I2C_PAIR3_SDA   38      // Alternatif 2
    #define I2C_PAIR3_SCL   39

    #define BOARD_NAME      "ESP32-S3"

#else
    #error "Board tidak didukung! Gunakan ESP32, ESP32-S2, atau ESP32-S3"
#endif

/* ==================== Konfigurasi I2C ==================== */
#define I2C_MASTER_FREQ_HZ      100000      // 100 kHz (Standard Mode)
#define I2C_PROBE_TIMEOUT_MS    50          // Timeout probe 50ms

/* Struktur untuk menyimpan informasi pin pair */
typedef struct {
    int sda_pin;
    int scl_pin;
    const char *deskripsi;
} i2c_pin_pair_t;

static const i2c_pin_pair_t pin_pairs[] = {
    { I2C_PAIR1_SDA, I2C_PAIR1_SCL, "Default (Pair 1)" },
    { I2C_PAIR2_SDA, I2C_PAIR2_SCL, "Alternatif (Pair 2)" },
    { I2C_PAIR3_SDA, I2C_PAIR3_SCL, "Alternatif (Pair 3)" },
};

#define NUM_PAIRS   (sizeof(pin_pairs) / sizeof(pin_pairs[0]))

/* ==================== Fungsi I2C Scanner ==================== */

/**
 * Scan bus I2C untuk menemukan device yang terhubung
 * Menggunakan i2c_master_probe() — API baru ESP-IDF v5.x
 *
 * @param bus_handle Handle bus I2C yang sudah diinisialisasi
 * @param pair_name  Nama pin pair untuk logging
 * @return Jumlah device yang ditemukan
 */
static int i2c_scan_bus(i2c_master_bus_handle_t bus_handle, const char *pair_name)
{
    int device_count = 0;

    ESP_LOGI(TAG, "  Scanning bus '%s'...", pair_name);
    ESP_LOGI(TAG, "  ┌────────┬──────────┬─────────────────────────────────┐");
    ESP_LOGI(TAG, "  │ Alamat │ Hex      │ Device Umum                     │");
    ESP_LOGI(TAG, "  ├────────┼──────────┼─────────────────────────────────┤");

    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        esp_err_t ret = i2c_master_probe(bus_handle, addr, I2C_PROBE_TIMEOUT_MS);

        if (ret == ESP_OK) {
            device_count++;

            /* Identifikasi device umum berdasarkan alamat */
            const char *device_name = "Unknown";
            switch (addr) {
                case 0x20: case 0x21: case 0x22: case 0x23:
                case 0x24: case 0x25: case 0x26: case 0x27:
                    device_name = "PCF8574 / MCP23017 (I/O Expander)";
                    break;
                case 0x38:
                    device_name = "AHT20/AHT21 (Temp/Humidity)";
                    break;
                case 0x3C: case 0x3D:
                    device_name = "SSD1306 (OLED Display)";
                    break;
                case 0x48: case 0x49: case 0x4A: case 0x4B:
                    device_name = "ADS1115/ADS1015 (ADC) / TMP102";
                    break;
                case 0x50: case 0x51: case 0x52: case 0x53:
                case 0x54: case 0x55: case 0x56: case 0x57:
                    device_name = "AT24C EEPROM";
                    break;
                case 0x68:
                    device_name = "MPU6050 (IMU) / DS3231 (RTC)";
                    break;
                case 0x69:
                    device_name = "MPU6050 (alt addr)";
                    break;
                case 0x76: case 0x77:
                    device_name = "BMP280/BME280 (Pressure/Temp)";
                    break;
            }

            ESP_LOGI(TAG, "  │  %3d   │  0x%02X    │ %-31s │",
                     addr, addr, device_name);
        }
    }

    ESP_LOGI(TAG, "  └────────┴──────────┴─────────────────────────────────┘");

    if (device_count == 0) {
        ESP_LOGI(TAG, "  Tidak ada device ditemukan (normal jika tidak ada sensor)");
    } else {
        ESP_LOGI(TAG, "  Total: %d device ditemukan", device_count);
    }

    return device_count;
}

/* ==================== Demo 1: I2C di 3 Pin Pair Berbeda ==================== */

/**
 * Demonstrasi utama: Membuat bus I2C pada 3 pasang pin yang berbeda
 * dan menjalankan scanner pada masing-masing.
 *
 * Ini MUSTAHIL di STM32 — STM32 tidak bisa remap I2C ke pin sembarang!
 */
static void demo_multi_pin_pair(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 1: I2C pada 3 Pin Pair Berbeda (GPIO Matrix)        ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP32 GPIO Matrix memungkinkan I2C di pin MANA SAJA!");
    ESP_LOGI(TAG, "Kita akan membuat bus I2C pada 3 pasang pin berbeda.");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Perbandingan dengan STM32F411:");
    ESP_LOGI(TAG, "  STM32 I2C1: SCL=PB6|PB8, SDA=PB7|PB9 (hanya 2 opsi!)");
    ESP_LOGI(TAG, "  ESP32:      SCL/SDA = hampir semua GPIO (puluhan opsi!)");
    ESP_LOGI(TAG, "");

    for (int pair = 0; pair < NUM_PAIRS; pair++) {
        ESP_LOGI(TAG, "══════════════════════════════════════════════════════════");
        ESP_LOGI(TAG, "Pin Pair %d: SDA=GPIO%d, SCL=GPIO%d — %s",
                 pair + 1,
                 pin_pairs[pair].sda_pin,
                 pin_pairs[pair].scl_pin,
                 pin_pairs[pair].deskripsi);
        ESP_LOGI(TAG, "══════════════════════════════════════════════════════════");

        /* Konfigurasi bus I2C master pada pin pair ini */
        i2c_master_bus_config_t bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .scl_io_num = pin_pairs[pair].scl_pin,
            .sda_io_num = pin_pairs[pair].sda_pin,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,   // Gunakan pull-up internal
        };

        i2c_master_bus_handle_t bus_handle = NULL;
        esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "  Gagal membuat bus I2C: %s", esp_err_to_name(ret));
            ESP_LOGE(TAG, "  (Pin mungkin sedang digunakan peripheral lain)");
            continue;
        }

        ESP_LOGI(TAG, "  ✓ Bus I2C berhasil dibuat pada pin pair ini!");

        /* Scan bus untuk device */
        i2c_scan_bus(bus_handle, pin_pairs[pair].deskripsi);

        /* Hapus bus — ini membebaskan pin untuk digunakan ulang */
        ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
        ESP_LOGI(TAG, "  ✓ Bus dihapus, pin dibebaskan");
        ESP_LOGI(TAG, "");

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Kesimpulan Demo 1:");
    ESP_LOGI(TAG, "  Bus I2C berhasil dibuat di 3 pasang pin BERBEDA!");
    ESP_LOGI(TAG, "  Di STM32, ini MUSTAHIL — hanya bisa di pin AF tertentu.");
}

/* ==================== Demo 2: Runtime Pin Remapping ==================== */

/**
 * Demonstrasi runtime pin remapping
 * Buat bus → hapus → buat lagi di pin baru, TANPA reboot!
 *
 * Di STM32, mengubah pin I2C memerlukan:
 * 1. Reconfigure GPIO AF
 * 2. Reinit I2C peripheral
 * 3. Terbatas pada pin yang tersedia di AF table
 */
static void demo_runtime_remap(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 2: Runtime Pin Remapping                             ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Memindahkan bus I2C dari satu pin pair ke pin pair lain");
    ESP_LOGI(TAG, "secara runtime — TANPA reboot!");
    ESP_LOGI(TAG, "");

    for (int pair = 0; pair < NUM_PAIRS; pair++) {
        ESP_LOGI(TAG, "--- Remap ke Pair %d: SDA=GPIO%d, SCL=GPIO%d ---",
                 pair + 1,
                 pin_pairs[pair].sda_pin,
                 pin_pairs[pair].scl_pin);

        /* Buat bus di pin pair saat ini */
        i2c_master_bus_config_t bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .scl_io_num = pin_pairs[pair].scl_pin,
            .sda_io_num = pin_pairs[pair].sda_pin,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        i2c_master_bus_handle_t bus_handle = NULL;
        esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  ✓ I2C aktif di GPIO%d/GPIO%d",
                     pin_pairs[pair].sda_pin, pin_pairs[pair].scl_pin);

            /* Coba probe beberapa alamat umum */
            uint8_t common_addrs[] = { 0x3C, 0x68, 0x76, 0x50, 0x38 };
            const char *common_names[] = { "OLED", "MPU6050", "BMP280", "EEPROM", "AHT20" };

            for (int a = 0; a < 5; a++) {
                ret = i2c_master_probe(bus_handle, common_addrs[a], I2C_PROBE_TIMEOUT_MS);
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "  → %s ditemukan di 0x%02X pada pair ini!",
                             common_names[a], common_addrs[a]);
                }
            }

            /* Hapus bus untuk remap ke pin berikutnya */
            ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
            ESP_LOGI(TAG, "  ✓ Bus dihapus — siap remap ke pin lain");
        } else {
            ESP_LOGW(TAG, "  Gagal: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "✓ Pin remapping berhasil! Bus I2C berpindah antar pin");
    ESP_LOGI(TAG, "  tanpa reboot — ini keunggulan GPIO Matrix ESP32");
}

/* ==================== Demo 3: Dual I2C Bus Simultan ==================== */

/**
 * Demonstrasi dual I2C bus yang berjalan BERSAMAAN
 * I2C_NUM_0 di pin pair 1, I2C_NUM_1 di pin pair 2 — simultan!
 *
 * Ini berguna saat:
 * - Ada device dengan alamat yang sama (perlu bus terpisah)
 * - Perlu bandwidth lebih (2 bus = 2x throughput)
 * - Isolasi bus untuk reliability
 */
static void demo_dual_bus_simultan(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 3: Dual I2C Bus Simultan                             ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Menjalankan 2 bus I2C secara BERSAMAAN di pin berbeda!");
    ESP_LOGI(TAG, "  Bus 0: SDA=GPIO%d, SCL=GPIO%d", I2C_PAIR1_SDA, I2C_PAIR1_SCL);
    ESP_LOGI(TAG, "  Bus 1: SDA=GPIO%d, SCL=GPIO%d", I2C_PAIR2_SDA, I2C_PAIR2_SCL);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Kegunaan dual bus:");
    ESP_LOGI(TAG, "  1. Device dengan alamat sama di bus berbeda");
    ESP_LOGI(TAG, "  2. Throughput 2x lipat (paralel)");
    ESP_LOGI(TAG, "  3. Isolasi bus untuk reliability");
    ESP_LOGI(TAG, "");

    /* Konfigurasi Bus 0 */
    i2c_master_bus_config_t bus0_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_PAIR1_SCL,
        .sda_io_num = I2C_PAIR1_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    /* Konfigurasi Bus 1 */
    i2c_master_bus_config_t bus1_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_1,
        .scl_io_num = I2C_PAIR2_SCL,
        .sda_io_num = I2C_PAIR2_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus0_handle = NULL;
    i2c_master_bus_handle_t bus1_handle = NULL;

    /* Buat kedua bus */
    esp_err_t ret0 = i2c_new_master_bus(&bus0_config, &bus0_handle);
    esp_err_t ret1 = i2c_new_master_bus(&bus1_config, &bus1_handle);

    if (ret0 != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membuat Bus 0: %s", esp_err_to_name(ret0));
    }
    if (ret1 != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membuat Bus 1: %s", esp_err_to_name(ret1));
    }

    if (ret0 == ESP_OK && ret1 == ESP_OK) {
        ESP_LOGI(TAG, "✓ Kedua bus I2C aktif secara simultan!");
        ESP_LOGI(TAG, "");

        /* Scan kedua bus secara bergantian */
        ESP_LOGI(TAG, "--- Scanning Bus 0 (GPIO%d/GPIO%d) ---",
                 I2C_PAIR1_SDA, I2C_PAIR1_SCL);
        int count0 = i2c_scan_bus(bus0_handle, "Bus 0");

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "--- Scanning Bus 1 (GPIO%d/GPIO%d) ---",
                 I2C_PAIR2_SDA, I2C_PAIR2_SCL);
        int count1 = i2c_scan_bus(bus1_handle, "Bus 1");

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "Hasil dual bus scan:");
        ESP_LOGI(TAG, "  Bus 0: %d device", count0);
        ESP_LOGI(TAG, "  Bus 1: %d device", count1);
        ESP_LOGI(TAG, "  (Hubungkan sensor ke salah satu pair untuk melihat hasilnya)");
    }

    /* Cleanup */
    if (bus0_handle) {
        ESP_ERROR_CHECK(i2c_del_master_bus(bus0_handle));
        ESP_LOGI(TAG, "✓ Bus 0 dihapus");
    }
    if (bus1_handle) {
        ESP_ERROR_CHECK(i2c_del_master_bus(bus1_handle));
        ESP_LOGI(TAG, "✓ Bus 1 dihapus");
    }
}

/* ==================== Demo 4: Perbandingan dengan STM32 ==================== */

/**
 * Rangkuman perbandingan kemampuan I2C pin mapping
 * ESP32 vs STM32
 */
static void demo_perbandingan(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  PERBANDINGAN I2C PIN MAPPING: ESP32 vs STM32                  ║");
    ESP_LOGI(TAG, "╠═══════════════════════════╦════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Fitur                    ║ ESP32          │ STM32           ║");
    ESP_LOGI(TAG, "╠═══════════════════════════╬════════════════╪═════════════════╣");
    ESP_LOGI(TAG, "║  SDA/SCL pin              ║ Hampir semua   │ Fixed AF pins   ║");
    ESP_LOGI(TAG, "║  Runtime remap            ║ Ya (delete +   │ Terbatas        ║");
    ESP_LOGI(TAG, "║                           ║  recreate)     │                 ║");
    ESP_LOGI(TAG, "║  Pin options per bus      ║ 20+ GPIO       │ 2-4 pin pair    ║");
    ESP_LOGI(TAG, "║  PCB design freedom       ║ Sangat tinggi  │ Terbatas        ║");
    ESP_LOGI(TAG, "║  Internal pull-up         ║ Ya (weak)      │ Tidak (biasanya)║");
    ESP_LOGI(TAG, "║  Multiple bus di pin      ║ Mana saja      │ Fixed per I2Cx  ║");
    ESP_LOGI(TAG, "║  berbeda                  ║                │                 ║");
    ESP_LOGI(TAG, "╚═══════════════════════════╩════════════════╧═════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Contoh batasan STM32F411:");
    ESP_LOGI(TAG, "  I2C1: SCL = PB6|PB8     SDA = PB7|PB9     (hanya 2 opsi)");
    ESP_LOGI(TAG, "  I2C2: SCL = PB10        SDA = PB3|PB9     (sangat terbatas)");
    ESP_LOGI(TAG, "  I2C3: SCL = PA8         SDA = PB4|PC9     (sangat terbatas)");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP32 (%s):", BOARD_NAME);
    ESP_LOGI(TAG, "  I2C bisa di pin MANA SAJA yang tersedia sebagai GPIO!");
    ESP_LOGI(TAG, "  Demo ini menggunakan 3 pair yang sangat berbeda:");

    for (int i = 0; i < NUM_PAIRS; i++) {
        ESP_LOGI(TAG, "    Pair %d: SDA=GPIO%d, SCL=GPIO%d — %s",
                 i + 1, pin_pairs[i].sda_pin, pin_pairs[i].scl_pin,
                 pin_pairs[i].deskripsi);
    }
}

/* ==================== Main Application ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32_13: I2C GPIO Matrix Pin Remapping Demo              ║");
    ESP_LOGI(TAG, "║  Fitur Khusus ESP32 — Tidak Tersedia di STM32              ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "");

    /* Demo 1: I2C pada 3 pin pair berbeda */
    demo_multi_pin_pair();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Demo 2: Runtime pin remapping */
    demo_runtime_remap();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Demo 3: Dual I2C bus simultan */
    demo_dual_bus_simultan();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Demo 4: Perbandingan ESP32 vs STM32 */
    demo_perbandingan();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "Semua demo selesai!");
    ESP_LOGI(TAG, "Kesimpulan: ESP32 I2C jauh lebih fleksibel dari STM32:");
    ESP_LOGI(TAG, "1. GPIO Matrix — I2C di pin mana saja");
    ESP_LOGI(TAG, "2. Runtime remapping — pindah pin tanpa reboot");
    ESP_LOGI(TAG, "3. Dual bus simultan di pin yang bebas dipilih");
    ESP_LOGI(TAG, "4. Sangat memudahkan desain PCB dan prototyping");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════════");
}
