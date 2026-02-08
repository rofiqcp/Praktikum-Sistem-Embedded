/**
 * ==========================================================================
 * ESP32_06_Sleep_Data_Retention - RTC Memory Data Persistence
 * ==========================================================================
 * 
 * Modul 14 - Power Management
 * Program 6: RTC memory usage to persist data across deep sleep
 * 
 * KONSEP:
 * - RTC_DATA_ATTR: Data bertahan selama deep sleep, hilang saat power off
 * - RTC_NOINIT_ATTR: Data bertahan saat software reset, TAPI tetap hilang saat power off
 * - Regular variable: SELALU hilang setelah deep sleep (cold boot)
 * 
 * MEMORY MAP ESP32:
 * ┌──────────────────────────────────────────────┐
 * │ Main RAM (520KB)                              │
 * │  → .data, .bss, heap                         │
 * │  → HILANG saat deep sleep                    │
 * ├──────────────────────────────────────────────┤
 * │ RTC FAST Memory (8KB)                         │
 * │  → RTC_DATA_ATTR variables                   │
 * │  → BERTAHAN saat deep sleep                  │
 * │  → Hilang saat power off                     │
 * ├──────────────────────────────────────────────┤
 * │ RTC SLOW Memory (8KB)                         │
 * │  → ULP program & data                        │
 * │  → BERTAHAN saat deep sleep                  │
 * └──────────────────────────────────────────────┘
 * 
 * WIRING / KONEKSI:
 * ┌─────────────────────────────────────────────┐
 * │  ESP32          Komponen                     │
 * │  GPIO2  ──────► LED (+) ──► R(220Ω) ──► GND │
 * │  EN     ──────► Reset Button ──► GND         │
 * │  (untuk test software reset)                 │
 * └─────────────────────────────────────────────┘
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * [RTC_MEM] === Boot #3 ===
 * [RTC_MEM] --- RTC_DATA_ATTR (survives deep sleep) ---
 * [RTC_MEM] Boot count: 3 ✓
 * [RTC_MEM] Readings[0]: 1234
 * [RTC_MEM] Readings[1]: 2567  
 * [RTC_MEM] --- Regular variable (lost after deep sleep) ---
 * [RTC_MEM] Regular var: 0 (always 0!)
 * [RTC_MEM] --- RTC_NOINIT_ATTR (survives sw reset) ---
 * [RTC_MEM] Noinit counter: 5
 * ========================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_random.h"

static const char *TAG = "RTC_MEM";

/* Konfigurasi */
#define LED_PIN             GPIO_NUM_2
#define MAX_STORED_READINGS 10              // Maks sensor readings di RTC
#define SLEEP_DURATION_US   (10ULL * 1000000ULL) // 10 detik
#define MAGIC_NUMBER        0xDEADBEEF      // Untuk validasi RTC memory

/**
 * ============================================================
 * RTC_DATA_ATTR: Bertahan selama deep sleep
 * Disimpan di RTC Fast Memory (8KB max)
 * ============================================================
 */
RTC_DATA_ATTR static uint32_t rtc_magic = 0;              // Validasi magic number
RTC_DATA_ATTR static int rtc_boot_count = 0;               // Boot counter
RTC_DATA_ATTR static int rtc_readings[MAX_STORED_READINGS]; // Sensor data array
RTC_DATA_ATTR static int rtc_reading_count = 0;             // Jumlah reading tersimpan
RTC_DATA_ATTR static int64_t rtc_first_boot_time = 0;       // Timestamp boot pertama
RTC_DATA_ATTR static int64_t rtc_timestamps[MAX_STORED_READINGS]; // Timestamp per reading
RTC_DATA_ATTR static uint32_t rtc_checksum = 0;             // Checksum validasi data

/**
 * ============================================================
 * RTC_NOINIT_ATTR: Bertahan saat software reset
 * TIDAK di-inisialisasi ulang saat boot, tapi hilang saat power off
 * Berguna untuk mendeteksi crash/restart
 * ============================================================
 */
RTC_NOINIT_ATTR static int noinit_counter;
RTC_NOINIT_ATTR static uint32_t noinit_magic;
RTC_NOINIT_ATTR static int noinit_crash_count;

/**
 * Regular variable: SELALU 0 setelah deep sleep
 * Ini untuk membuktikan bahwa main RAM hilang
 */
static int regular_variable = 0;
static int regular_array[5] = {0};

/**
 * Inisialisasi LED
 */
static void init_led(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

/**
 * Hitung simple checksum dari data RTC
 */
static uint32_t calculate_checksum(void)
{
    uint32_t sum = 0;
    sum += rtc_boot_count;
    sum += rtc_reading_count;
    for (int i = 0; i < rtc_reading_count && i < MAX_STORED_READINGS; i++) {
        sum += (uint32_t)rtc_readings[i];
    }
    return sum ^ 0xA5A5A5A5;
}

/**
 * Simulasi sensor reading (menggunakan random + boot_count)
 */
static int simulate_sensor_reading(void)
{
    /* Gunakan esp_random() untuk nilai pseudo-random */
    uint32_t rand_val = esp_random();
    return (int)(rand_val % 4096); // 12-bit ADC range
}

/**
 * Simpan reading ke RTC memory array
 */
static void store_rtc_reading(int value, int64_t timestamp)
{
    if (rtc_reading_count < MAX_STORED_READINGS) {
        rtc_readings[rtc_reading_count] = value;
        rtc_timestamps[rtc_reading_count] = timestamp;
        rtc_reading_count++;
    } else {
        /* Buffer penuh - shift data (FIFO) */
        for (int i = 0; i < MAX_STORED_READINGS - 1; i++) {
            rtc_readings[i] = rtc_readings[i + 1];
            rtc_timestamps[i] = rtc_timestamps[i + 1];
        }
        rtc_readings[MAX_STORED_READINGS - 1] = value;
        rtc_timestamps[MAX_STORED_READINGS - 1] = timestamp;
    }
}

/**
 * Cetak semua data RTC memory
 */
static void print_rtc_data(void)
{
    int count = (rtc_reading_count < MAX_STORED_READINGS) ?
                 rtc_reading_count : MAX_STORED_READINGS;

    ESP_LOGI(TAG, "--- RTC_DATA_ATTR Contents (%d readings) ---", count);
    for (int i = 0; i < count; i++) {
        float voltage = rtc_readings[i] * 3.3f / 4095.0f;
        int64_t relative_ts = rtc_timestamps[i] - rtc_first_boot_time;
        ESP_LOGI(TAG, "  [%2d] ADC=%4d (%.2fV) | Time offset: %lld ms",
                 i, rtc_readings[i], voltage, relative_ts / 1000);
    }
}

/**
 * Demonstrasi perbandingan tipe memori
 */
static void demonstrate_memory_types(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  MEMORY TYPE COMPARISON");
    ESP_LOGI(TAG, "============================================");

    /* 1. Regular variable - selalu 0 */
    regular_variable = 999; // Set ke 999, tapi setelah deep sleep akan 0 lagi
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "1. REGULAR VARIABLE (main RAM):");
    ESP_LOGI(TAG, "   Value: %d", regular_variable);
    ESP_LOGW(TAG, "   → Set ke 999 di boot ini, tapi akan 0 setelah deep sleep!");
    ESP_LOGW(TAG, "   → Main RAM HILANG saat deep sleep (cold boot)");

    /* 2. RTC_DATA_ATTR - bertahan */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "2. RTC_DATA_ATTR (RTC Fast Memory):");
    ESP_LOGI(TAG, "   Boot count: %d ✓ (persists!)", rtc_boot_count);
    ESP_LOGI(TAG, "   Readings stored: %d", rtc_reading_count);
    ESP_LOGI(TAG, "   → Bertahan selama deep sleep");
    ESP_LOGI(TAG, "   → Hilang saat power off");

    /* 3. RTC_NOINIT_ATTR - bertahan saat sw reset */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "3. RTC_NOINIT_ATTR:");
    if (noinit_magic == MAGIC_NUMBER) {
        ESP_LOGI(TAG, "   Noinit counter: %d ✓ (survived reset!)", noinit_counter);
        ESP_LOGI(TAG, "   Crash count: %d", noinit_crash_count);
    } else {
        ESP_LOGW(TAG, "   Magic number invalid → fresh power on");
        ESP_LOGW(TAG, "   Initializing noinit variables...");
        noinit_magic = MAGIC_NUMBER;
        noinit_counter = 0;
        noinit_crash_count = 0;
    }
    noinit_counter++;
    ESP_LOGI(TAG, "   → Bertahan saat software reset & deep sleep");
    ESP_LOGI(TAG, "   → TIDAK di-init ulang oleh startup code");
    ESP_LOGI(TAG, "   → Hilang saat power off");

    /* Tabel ringkasan */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "  ┌─────────────────┬───────────┬──────────┬───────────┐");
    ESP_LOGI(TAG, "  │ Memory Type     │Deep Sleep │SW Reset  │Power Off  │");
    ESP_LOGI(TAG, "  ├─────────────────┼───────────┼──────────┼───────────┤");
    ESP_LOGI(TAG, "  │ Regular (RAM)   │ LOST      │ LOST     │ LOST      │");
    ESP_LOGI(TAG, "  │ RTC_DATA_ATTR   │ PRESERVED │ RESET    │ LOST      │");
    ESP_LOGI(TAG, "  │ RTC_NOINIT_ATTR │ PRESERVED │PRESERVED │ LOST      │");
    ESP_LOGI(TAG, "  │ NVS (Flash)     │ PRESERVED │PRESERVED │ PRESERVED │");
    ESP_LOGI(TAG, "  └─────────────────┴───────────┴──────────┴───────────┘");
}

/**
 * Entry point utama
 */
void app_main(void)
{
    int64_t current_time = esp_timer_get_time();

    /* Dapatkan wakeup cause */
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    /* Cek apakah ini boot pertama (RTC memory belum di-init) */
    if (rtc_magic != MAGIC_NUMBER) {
        /* First boot - inisialisasi RTC memory */
        rtc_magic = MAGIC_NUMBER;
        rtc_boot_count = 0;
        rtc_reading_count = 0;
        rtc_first_boot_time = current_time;
        memset(rtc_readings, 0, sizeof(rtc_readings));
        memset(rtc_timestamps, 0, sizeof(rtc_timestamps));
        ESP_LOGI(TAG, "First boot detected - RTC memory initialized");
    }

    rtc_boot_count++;

    init_led();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "  RTC MEMORY DATA RETENTION - Boot #%d", rtc_boot_count);
    ESP_LOGI(TAG, "================================================");

    const char *cause_str = "POWER_ON";
    if (cause == ESP_SLEEP_WAKEUP_TIMER) cause_str = "TIMER";
    else if (cause == ESP_SLEEP_WAKEUP_EXT0) cause_str = "EXT0";
    ESP_LOGI(TAG, "Wakeup cause: %s", cause_str);

    /* Blink LED sesuai boot count (max 5) */
    int blinks = (rtc_boot_count > 5) ? 5 : rtc_boot_count;
    for (int i = 0; i < blinks; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* Baca "sensor" dan simpan ke RTC */
    int sensor_value = simulate_sensor_reading();
    store_rtc_reading(sensor_value, current_time);
    ESP_LOGI(TAG, "New reading: ADC=%d (%.2fV)", sensor_value, sensor_value * 3.3f / 4095.0f);

    /* Tampilkan semua data RTC */
    print_rtc_data();

    /* Demonstrasi perbandingan memori */
    demonstrate_memory_types();

    /* Validasi integritas data */
    uint32_t new_checksum = calculate_checksum();
    if (rtc_boot_count > 1 && rtc_checksum != 0) {
        /* Checksum berubah karena data baru ditambahkan - ini normal */
        ESP_LOGI(TAG, "Previous checksum: 0x%08lx", (unsigned long)rtc_checksum);
    }
    rtc_checksum = new_checksum;
    ESP_LOGI(TAG, "Current checksum: 0x%08lx", (unsigned long)rtc_checksum);

    /* RTC memory usage estimation */
    int rtc_used = sizeof(rtc_magic) + sizeof(rtc_boot_count) +
                   sizeof(rtc_readings) + sizeof(rtc_reading_count) +
                   sizeof(rtc_first_boot_time) + sizeof(rtc_timestamps) +
                   sizeof(rtc_checksum);
    ESP_LOGI(TAG, "RTC memory used: ~%d bytes / 8192 bytes (%.1f%%)",
             rtc_used, (float)rtc_used / 8192.0f * 100.0f);

    /* Data terformat untuk Python */
    ESP_LOGI(TAG, "DATA,%d,%d,%d,%s,%d,0x%08lx,%d,%d",
             rtc_boot_count, sensor_value, rtc_reading_count,
             cause_str, regular_variable,
             (unsigned long)rtc_checksum,
             noinit_counter, rtc_used);

    /* Masuk deep sleep */
    gpio_set_level(LED_PIN, 0);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Entering deep sleep for %lld seconds...",
             SLEEP_DURATION_US / 1000000ULL);
    ESP_LOGI(TAG, "RTC data will be PRESERVED, regular vars will be LOST");
    ESP_LOGI(TAG, "================================================");

    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(200));

    esp_deep_sleep(SLEEP_DURATION_US);
}
