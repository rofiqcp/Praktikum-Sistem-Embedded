/**
 * =============================================================================
 * ESP32_13_ADC_Attenuation_WiFi
 * Modul 04 - ADC | Bonus: Fitur Khusus ESP32
 * =============================================================================
 *
 * FITUR KHUSUS ESP32 YANG TIDAK ADA DI STM32:
 * --------------------------------------------
 * 1. ADC ATTENUATION CONTROL:
 *    - ESP32 memiliki attenuator internal yang dapat diprogram pada setiap
 *      channel ADC secara independen. Ada 4 level atenuasi:
 *      * 0 dB   : Rentang input 0 - 750 mV  (sensitivitas tertinggi)
 *      * 2.5 dB : Rentang input 0 - 1050 mV
 *      * 6 dB   : Rentang input 0 - 1300 mV
 *      * 12 dB  : Rentang input 0 - 3100 mV (rentang terluas, cocok 3.3V logic)
 *    - STM32 TIDAK memiliki fitur ini. STM32 ADC selalu mengukur 0 - Vref
 *      tanpa kemampuan mengubah rentang input secara hardware.
 *
 * 2. KONFLIK ADC2 + WiFi:
 *    - ESP32 memiliki 2 unit ADC: ADC1 dan ADC2
 *    - ADC2 TIDAK BISA digunakan saat WiFi aktif karena WiFi driver
 *      menggunakan ADC2 secara internal untuk kalibrasi RF
 *    - Ini adalah limitasi unik ESP32 yang perlu dipahami developer
 *    - STM32 tidak memiliki konflik semacam ini
 *
 * 3. ADC CALIBRATION SCHEME:
 *    - ESP32 mendukung kalibrasi berbasis eFuse (line fitting / curve fitting)
 *    - Setiap chip memiliki data kalibrasi unik di eFuse
 *
 * MENGAPA STM32 TIDAK BISA:
 * - STM32 ADC tidak memiliki attenuator internal
 * - Untuk mengubah rentang, STM32 memerlukan voltage divider eksternal
 * - STM32 tidak memiliki konflik ADC-WiFi (WiFi bukan built-in pada kebanyakan STM32)
 *
 * Hardware: Potensiometer/sumber tegangan pada pin ADC
 * =============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "ADC_ATTEN_WIFI";

/* ==================== Konfigurasi Pin per Board ==================== */
/*
 * Mapping pin ADC berbeda untuk setiap varian ESP32.
 * Kita pilih pin yang aman dan mudah diakses.
 */

#if CONFIG_IDF_TARGET_ESP32
    /* ESP32 Classic - ADC1 Channel 6 = GPIO34, ADC2 Channel 7 = GPIO27 */
    #define ADC1_TEST_CHANNEL   ADC_CHANNEL_6       // GPIO34 (input only, aman)
    #define ADC1_TEST_GPIO      34
    #define ADC2_TEST_CHANNEL   ADC_CHANNEL_7       // GPIO27
    #define ADC2_TEST_GPIO      27
    #define BOARD_NAME          "ESP32 Classic"

#elif CONFIG_IDF_TARGET_ESP32S2
    /* ESP32-S2 - ADC1 Channel 3 = GPIO4, ADC2 Channel 3 = GPIO13 */
    #define ADC1_TEST_CHANNEL   ADC_CHANNEL_3       // GPIO4
    #define ADC1_TEST_GPIO      4
    #define ADC2_TEST_CHANNEL   ADC_CHANNEL_3       // GPIO13
    #define ADC2_TEST_GPIO      13
    #define BOARD_NAME          "ESP32-S2"

#elif CONFIG_IDF_TARGET_ESP32S3
    /* ESP32-S3 - ADC1 Channel 3 = GPIO4, ADC2 Channel 3 = GPIO14 */
    #define ADC1_TEST_CHANNEL   ADC_CHANNEL_3       // GPIO4
    #define ADC1_TEST_GPIO      4
    #define ADC2_TEST_CHANNEL   ADC_CHANNEL_3       // GPIO14
    #define ADC2_TEST_GPIO      14
    #define BOARD_NAME          "ESP32-S3"

#else
    #error "Board tidak didukung! Gunakan ESP32, ESP32-S2, atau ESP32-S3"
#endif

/* ==================== Struktur Data Atenuasi ==================== */

/**
 * Tabel informasi untuk setiap level atenuasi
 * Ini yang membuat ESP32 unik — bisa mengubah rentang ADC secara hardware
 */
typedef struct {
    adc_atten_t atten;          // Nilai enum atenuasi
    const char *nama;           // Nama deskriptif
    const char *rentang;        // Rentang tegangan yang bisa diukur
    int mv_max;                 // Tegangan maksimum (mV)
} atten_info_t;

static const atten_info_t atten_table[] = {
    { ADC_ATTEN_DB_0,   "0 dB",   "0 - 750 mV",  750  },
    { ADC_ATTEN_DB_2_5, "2.5 dB", "0 - 1050 mV", 1050 },
    { ADC_ATTEN_DB_6,   "6 dB",   "0 - 1300 mV", 1300 },
    { ADC_ATTEN_DB_12,  "12 dB",  "0 - 3100 mV", 3100 },
};

#define ATTEN_COUNT  (sizeof(atten_table) / sizeof(atten_table[0]))

/* ==================== Variabel Global ==================== */
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool cali_enabled = false;

/* ==================== Fungsi Inisialisasi Kalibrasi ==================== */

/**
 * Inisialisasi kalibrasi ADC
 * ESP32 mendukung kalibrasi berbasis eFuse — setiap chip unik
 */
static bool adc_kalibrasi_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten)
{
    esp_err_t ret;

    /* Hapus handle kalibrasi lama jika ada */
    if (adc_cali_handle != NULL) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_delete_scheme_curve_fitting(adc_cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(adc_cali_handle);
#endif
        adc_cali_handle = NULL;
    }

    /* Coba Curve Fitting dulu (lebih akurat, tersedia di ESP32-S2/S3) */
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Menggunakan kalibrasi Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi Curve Fitting berhasil");
        return true;
    }
#endif

    /* Fallback ke Line Fitting (tersedia di ESP32 Classic) */
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Menggunakan kalibrasi Line Fitting");
    adc_cali_line_fitting_config_t cali_config_line = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config_line, &adc_cali_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi Line Fitting berhasil");
        return true;
    }
#endif

    ESP_LOGW(TAG, "Kalibrasi tidak tersedia, menggunakan raw value");
    (void)ret;
    return false;
}

/* ==================== Demo 1: Test Semua Level Atenuasi ==================== */

/**
 * Demonstrasi utama: Membaca ADC1 dengan 4 level atenuasi berbeda
 * pada channel yang SAMA dan membandingkan hasilnya.
 *
 * Ini menunjukkan kemampuan unik ESP32:
 * - Mengubah sensitivitas ADC secara runtime
 * - Memilih rentang yang optimal untuk sinyal yang diukur
 */
static void demo_adc_attenuation(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 1: ADC Attenuation Control (Fitur Khusus ESP32)      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "Board: %s | ADC1 Channel: %d (GPIO%d)",
             BOARD_NAME, ADC1_TEST_CHANNEL, ADC1_TEST_GPIO);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP32 memiliki attenuator internal yang TIDAK dimiliki STM32!");
    ESP_LOGI(TAG, "Kita akan membaca channel yang SAMA dengan 4 atenuasi berbeda.");
    ESP_LOGI(TAG, "");

    /* Inisialisasi ADC1 */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    /* Header tabel */
    ESP_LOGI(TAG, "┌──────────┬─────────────────┬───────────┬──────────────┬─────────────┐");
    ESP_LOGI(TAG, "│ Atenuasi │ Rentang Input   │ Raw Value │ Tegangan(mV) │ Status      │");
    ESP_LOGI(TAG, "├──────────┼─────────────────┼───────────┼──────────────┼─────────────┤");

    /* Test setiap level atenuasi */
    for (int i = 0; i < ATTEN_COUNT; i++) {
        /* Konfigurasi channel dengan atenuasi yang berbeda */
        adc_oneshot_chan_cfg_t chan_config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = atten_table[i].atten,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_TEST_CHANNEL, &chan_config));

        /* Setup kalibrasi untuk atenuasi ini */
        cali_enabled = adc_kalibrasi_init(ADC_UNIT_1, ADC1_TEST_CHANNEL, atten_table[i].atten);

        /* Baca beberapa sampel dan rata-ratakan */
        int raw_total = 0;
        int mv_total = 0;
        const int num_samples = 16;

        for (int s = 0; s < num_samples; s++) {
            int raw;
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_TEST_CHANNEL, &raw));
            raw_total += raw;

            if (cali_enabled) {
                int mv;
                adc_cali_raw_to_voltage(adc_cali_handle, raw, &mv);
                mv_total += mv;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        int raw_avg = raw_total / num_samples;
        int mv_avg = (cali_enabled) ? (mv_total / num_samples) : -1;

        /* Tentukan status berdasarkan apakah nilai dalam rentang */
        const char *status = "OK";
        if (cali_enabled && mv_avg > atten_table[i].mv_max) {
            status = "SATURASI!";  // Tegangan melebihi rentang atenuasi
        }

        if (cali_enabled) {
            ESP_LOGI(TAG, "│ %-8s │ %-15s │ %9d │ %10d   │ %-11s │",
                     atten_table[i].nama, atten_table[i].rentang,
                     raw_avg, mv_avg, status);
        } else {
            ESP_LOGI(TAG, "│ %-8s │ %-15s │ %9d │ (no cali)    │ %-11s │",
                     atten_table[i].nama, atten_table[i].rentang,
                     raw_avg, status);
        }
    }

    ESP_LOGI(TAG, "└──────────┴─────────────────┴───────────┴──────────────┴─────────────┘");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ANALISIS:");
    ESP_LOGI(TAG, "- Atenuasi RENDAH (0dB): Lebih sensitif, cocok untuk sinyal kecil");
    ESP_LOGI(TAG, "- Atenuasi TINGGI (12dB): Rentang lebar, cocok untuk 0-3.3V");
    ESP_LOGI(TAG, "- Jika tegangan input > rentang atenuasi → nilai SATURASI (4095)");
    ESP_LOGI(TAG, "- STM32 tidak bisa melakukan ini — hanya bisa 0 sampai Vref");

    /* Cleanup */
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    adc1_handle = NULL;
}

/* ==================== Demo 2: Monitoring Berkelanjutan ==================== */

/**
 * Monitoring ADC secara berkelanjutan dengan atenuasi optimal
 * Menunjukkan auto-ranging: pilih atenuasi terbaik berdasarkan tegangan
 */
static void demo_monitoring_berkelanjutan(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 2: Monitoring Berkelanjutan dengan Auto-Ranging      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "Membaca ADC1 GPIO%d dengan atenuasi 12dB selama 10 detik...", ADC1_TEST_GPIO);
    ESP_LOGI(TAG, "");

    /* Inisialisasi ADC1 dengan atenuasi 12dB (rentang terluas) */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_TEST_CHANNEL, &chan_config));

    cali_enabled = adc_kalibrasi_init(ADC_UNIT_1, ADC1_TEST_CHANNEL, ADC_ATTEN_DB_12);

    /* Monitoring loop */
    int min_val = 4095, max_val = 0;
    int64_t sum = 0;
    int count = 0;

    for (int i = 0; i < 20; i++) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_TEST_CHANNEL, &raw));

        int mv = 0;
        if (cali_enabled) {
            adc_cali_raw_to_voltage(adc_cali_handle, raw, &mv);
        }

        /* Track statistik */
        if (raw < min_val) min_val = raw;
        if (raw > max_val) max_val = raw;
        sum += raw;
        count++;

        /* Rekomendasi atenuasi berdasarkan tegangan */
        const char *rekomendasi = "12dB";
        if (cali_enabled) {
            if (mv < 700)       rekomendasi = "0dB (lebih presisi)";
            else if (mv < 1000) rekomendasi = "2.5dB (optimal)";
            else if (mv < 1250) rekomendasi = "6dB (optimal)";
            else                rekomendasi = "12dB (saat ini)";
        }

        ESP_LOGI(TAG, "[%2d] Raw: %4d | %4d mV | Rekomendasi atenuasi: %s",
                 i + 1, raw, mv, rekomendasi);

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* Tampilkan statistik */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "--- Statistik Monitoring ---");
    ESP_LOGI(TAG, "Min: %d | Max: %d | Rata-rata: %lld | Noise: %d LSB",
             min_val, max_val, sum / count, max_val - min_val);

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    adc1_handle = NULL;
}

/* ==================== Demo 3: Konflik ADC2 + WiFi ==================== */

/**
 * Demonstrasi konflik ADC2 dan WiFi
 *
 * PENTING: Ini adalah limitasi khusus ESP32!
 * - ADC2 digunakan oleh WiFi driver untuk kalibrasi RF
 * - Saat WiFi aktif, pembacaan ADC2 akan GAGAL
 * - Solusi: Gunakan ADC1 saja jika WiFi dibutuhkan
 * - STM32 tidak memiliki konflik semacam ini
 */
static void demo_adc2_wifi_conflict(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 3: Konflik ADC2 + WiFi (Limitasi Khusus ESP32)       ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP32 memiliki 2 unit ADC:");
    ESP_LOGI(TAG, "  ADC1: Selalu tersedia, aman digunakan kapan saja");
    ESP_LOGI(TAG, "  ADC2: KONFLIK dengan WiFi! Tidak bisa dipakai saat WiFi aktif");
    ESP_LOGI(TAG, "");

    /* --- Langkah 1: Baca ADC2 SEBELUM WiFi aktif (harus berhasil) --- */
    ESP_LOGI(TAG, "=== Langkah 1: Membaca ADC2 TANPA WiFi ===");

    adc_oneshot_unit_handle_t adc2_handle = NULL;
    adc_oneshot_unit_init_cfg_t adc2_init = {
        .unit_id = ADC_UNIT_2,
    };

    esp_err_t ret = adc_oneshot_new_unit(&adc2_init, &adc2_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC2 tidak tersedia di board ini (err: %s)", esp_err_to_name(ret));
        ESP_LOGI(TAG, "Melewati demo ADC2+WiFi conflict");
        return;
    }

    adc_oneshot_chan_cfg_t adc2_chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC2_TEST_CHANNEL, &adc2_chan_config));

    /* Baca ADC2 — seharusnya berhasil */
    int raw_before = 0;
    ret = adc_oneshot_read(adc2_handle, ADC2_TEST_CHANNEL, &raw_before);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ ADC2 (GPIO%d) berhasil dibaca: raw = %d", ADC2_TEST_GPIO, raw_before);
    } else {
        ESP_LOGW(TAG, "✗ ADC2 gagal dibaca: %s", esp_err_to_name(ret));
    }

    /* Hapus unit ADC2 sebelum init WiFi */
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2_handle));
    adc2_handle = NULL;

    /* --- Langkah 2: Aktifkan WiFi --- */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Langkah 2: Mengaktifkan WiFi... ===");

    /* Inisialisasi NVS (diperlukan WiFi) */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "✓ WiFi diaktifkan (mode STA)");
    vTaskDelay(pdMS_TO_TICKS(1000));  // Tunggu WiFi stabil

    /* --- Langkah 3: Coba baca ADC2 SAAT WiFi aktif (akan gagal!) --- */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Langkah 3: Mencoba ADC2 SAAT WiFi Aktif ===");

    ret = adc_oneshot_new_unit(&adc2_init, &adc2_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ GAGAL membuat unit ADC2: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "  → WiFi sedang menggunakan ADC2!");
    } else {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC2_TEST_CHANNEL, &adc2_chan_config));

        int raw_during = 0;
        ret = adc_oneshot_read(adc2_handle, ADC2_TEST_CHANNEL, &raw_during);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ADC2 berhasil dibaca (mungkin beruntung): raw = %d", raw_during);
            ESP_LOGW(TAG, "Catatan: Pada beberapa versi IDF, ADC2 bisa timeout secara acak");
        } else {
            ESP_LOGE(TAG, "✗ ADC2 GAGAL dibaca: %s", esp_err_to_name(ret));
            ESP_LOGE(TAG, "  → Ini adalah KONFLIK ADC2+WiFi yang terkenal!");
        }

        ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2_handle));
        adc2_handle = NULL;
    }

    /* --- Langkah 4: ADC1 tetap berfungsi saat WiFi aktif --- */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Langkah 4: ADC1 Tetap Berfungsi Saat WiFi Aktif ===");

    adc_oneshot_unit_init_cfg_t adc1_init = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_init, &adc1_handle));

    adc_oneshot_chan_cfg_t adc1_chan = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_TEST_CHANNEL, &adc1_chan));

    int raw_adc1 = 0;
    ret = adc_oneshot_read(adc1_handle, ADC1_TEST_CHANNEL, &raw_adc1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ ADC1 (GPIO%d) TETAP berhasil: raw = %d", ADC1_TEST_GPIO, raw_adc1);
        ESP_LOGI(TAG, "  → ADC1 tidak terpengaruh oleh WiFi!");
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    adc1_handle = NULL;

    /* --- Langkah 5: Matikan WiFi, ADC2 kembali tersedia --- */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Langkah 5: Matikan WiFi, Test ADC2 Lagi ===");

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    vTaskDelay(pdMS_TO_TICKS(500));

    ret = adc_oneshot_new_unit(&adc2_init, &adc2_handle);
    if (ret == ESP_OK) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC2_TEST_CHANNEL, &adc2_chan_config));

        int raw_after = 0;
        ret = adc_oneshot_read(adc2_handle, ADC2_TEST_CHANNEL, &raw_after);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ ADC2 kembali berfungsi setelah WiFi dimatikan: raw = %d", raw_after);
        }
        ESP_ERROR_CHECK(adc_oneshot_del_unit(adc2_handle));
    }

    /* Tampilkan rangkuman */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  RANGKUMAN KONFLIK ADC2 + WiFi                             ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  • ADC2 tidak bisa digunakan saat WiFi aktif               ║");
    ESP_LOGI(TAG, "║  • ADC1 selalu aman digunakan                              ║");
    ESP_LOGI(TAG, "║  • Solusi: Pakai ADC1 untuk proyek dengan WiFi             ║");
    ESP_LOGI(TAG, "║  • STM32 tidak memiliki limitasi ini                       ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
}

/* ==================== Main Application ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32_13: ADC Attenuation & WiFi Conflict Demo            ║");
    ESP_LOGI(TAG, "║  Fitur Khusus ESP32 — Tidak Tersedia di STM32              ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "");

    /* Demo 1: Test semua level atenuasi */
    demo_adc_attenuation();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Demo 2: Monitoring berkelanjutan dengan auto-ranging */
    demo_monitoring_berkelanjutan();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Demo 3: Konflik ADC2 + WiFi */
    demo_adc2_wifi_conflict();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "Semua demo selesai!");
    ESP_LOGI(TAG, "Kesimpulan: ESP32 ADC lebih fleksibel dari STM32 karena:");
    ESP_LOGI(TAG, "1. Atenuator internal yang bisa diprogram (4 level)");
    ESP_LOGI(TAG, "2. Kalibrasi berbasis eFuse per-chip");
    ESP_LOGI(TAG, "3. Tapi ada limitasi ADC2+WiFi yang harus dipahami");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════════");
}
