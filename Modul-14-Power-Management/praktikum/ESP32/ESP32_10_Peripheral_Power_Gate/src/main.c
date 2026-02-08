/**
 * ESP32_10_Peripheral_Power_Gate
 * 
 * Konsep: Disable peripheral yang tidak terpakai untuk menghemat daya.
 * - Demonstrasi disable/enable WiFi, BT, clock peripheral
 * - Gunakan periph_module_disable() untuk LEDC, I2C, SPI, dll.
 * - Tampilkan dampak arus setiap peripheral enable/disable
 * - Print status peripheral yang aktif/nonaktif
 * 
 * Framework: ESP-IDF (bukan Arduino)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_private/periph_ctrl.h"
#include "soc/periph_defs.h"
#include "hal/clk_gate_ll.h"

static const char *TAG = "PERIPH_GATE";

#define LED_PIN GPIO_NUM_2

// Struktur info peripheral
typedef struct {
    const char *name;
    periph_module_t module;
    float typical_current_ma;  // Estimasi arus saat aktif
    bool currently_enabled;
} peripheral_info_t;

// Daftar peripheral yang bisa dikelola
static peripheral_info_t peripherals[] = {
    {"LEDC (PWM)",    PERIPH_LEDC_MODULE,    0.5,  false},
    {"I2C_0",         PERIPH_I2C0_MODULE,    0.3,  false},
    {"SPI_2",         PERIPH_HSPI_MODULE,    1.0,  false},
    {"SPI_3",         PERIPH_VSPI_MODULE,    1.0,  false},
    {"UART_1",        PERIPH_UART1_MODULE,   0.5,  false},
    {"UART_2",        PERIPH_UART2_MODULE,   0.5,  false},
    {"I2S_0",         PERIPH_I2S0_MODULE,    1.5,  false},
    {"I2S_1",         PERIPH_I2S1_MODULE,    1.5,  false},
    {"Timer Group 0", PERIPH_TIMG0_MODULE,   0.3,  false},
    {"Timer Group 1", PERIPH_TIMG1_MODULE,   0.3,  false},
    {"RMT",           PERIPH_RMT_MODULE,     0.3,  false},
    {"PCNT",          PERIPH_PCNT_MODULE,    0.2,  false},
};

static const int num_peripherals = sizeof(peripherals) / sizeof(peripherals[0]);

// Statistik
static float total_saved_ma = 0.0f;
static int disabled_count = 0;
static int enabled_count = 0;

/**
 * Inisialisasi LED indikator
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
 * Tampilkan status semua peripheral
 */
static void print_peripheral_status(void)
{
    ESP_LOGI(TAG, "\n--- Status Peripheral ---");
    ESP_LOGI(TAG, " %-16s | Status  | Est. Arus", "Peripheral");
    ESP_LOGI(TAG, "-----------------|---------|----------");

    float total_active = 0;
    float total_inactive = 0;

    for (int i = 0; i < num_peripherals; i++) {
        const char *status = peripherals[i].currently_enabled ? "AKTIF" : "OFF";
        ESP_LOGI(TAG, " %-16s | %-7s | %.1f mA",
                 peripherals[i].name, status,
                 peripherals[i].typical_current_ma);

        if (peripherals[i].currently_enabled) {
            total_active += peripherals[i].typical_current_ma;
        } else {
            total_inactive += peripherals[i].typical_current_ma;
        }
    }

    ESP_LOGI(TAG, "-----------------|---------|----------");
    ESP_LOGI(TAG, " Total aktif     : %.1f mA", total_active);
    ESP_LOGI(TAG, " Total hemat     : %.1f mA", total_inactive);
}

/**
 * Aktifkan semua peripheral (simulasi kondisi awal)
 */
static void enable_all_peripherals(void)
{
    ESP_LOGI(TAG, "\n>>> Mengaktifkan SEMUA peripheral...");

    for (int i = 0; i < num_peripherals; i++) {
        periph_module_enable(peripherals[i].module);
        peripherals[i].currently_enabled = true;
        enabled_count++;
        ESP_LOGI(TAG, "  [ON]  %s (+%.1f mA)",
                 peripherals[i].name, peripherals[i].typical_current_ma);
    }

    ESP_LOGW(TAG, "Semua peripheral AKTIF - konsumsi daya maksimum!");
}

/**
 * Nonaktifkan peripheral satu per satu (demonstrasi penghematan)
 */
static void disable_peripherals_sequentially(void)
{
    ESP_LOGI(TAG, "\n>>> Menonaktifkan peripheral satu per satu...");
    total_saved_ma = 0;

    for (int i = 0; i < num_peripherals; i++) {
        if (peripherals[i].currently_enabled) {
            // Ukur waktu disable
            int64_t start = esp_timer_get_time();
            periph_module_disable(peripherals[i].module);
            int64_t elapsed = esp_timer_get_time() - start;

            peripherals[i].currently_enabled = false;
            total_saved_ma += peripherals[i].typical_current_ma;
            disabled_count++;

            ESP_LOGI(TAG, "  [OFF] %s (hemat %.1f mA, waktu %lld us, total hemat: %.1f mA)",
                     peripherals[i].name,
                     peripherals[i].typical_current_ma,
                     elapsed,
                     total_saved_ma);

            vTaskDelay(pdMS_TO_TICKS(500));  // Delay untuk observasi
        }
    }

    ESP_LOGW(TAG, "Semua peripheral NONAKTIF - penghematan total: %.1f mA", total_saved_ma);
}

/**
 * Demonstrasi selective enable - hanya aktifkan yang diperlukan
 */
static void demonstrate_selective_enable(void)
{
    ESP_LOGI(TAG, "\n>>> Demonstrasi Selective Enable...");
    ESP_LOGI(TAG, "Skenario: Hanya perlu I2C dan Timer untuk sensor reading");

    // Aktifkan hanya I2C_0 dan Timer Group 0
    for (int i = 0; i < num_peripherals; i++) {
        if (peripherals[i].module == PERIPH_I2C0_MODULE ||
            peripherals[i].module == PERIPH_TIMG0_MODULE) {
            periph_module_enable(peripherals[i].module);
            peripherals[i].currently_enabled = true;
            ESP_LOGI(TAG, "  [ON]  %s (diperlukan)", peripherals[i].name);
        }
    }

    ESP_LOGI(TAG, "\nPeripheral aktif hanya yang diperlukan:");
    print_peripheral_status();

    // Kembali disable
    vTaskDelay(pdMS_TO_TICKS(2000));
    for (int i = 0; i < num_peripherals; i++) {
        if (peripherals[i].currently_enabled) {
            periph_module_disable(peripherals[i].module);
            peripherals[i].currently_enabled = false;
        }
    }
}

/**
 * Tampilkan laporan power budget
 */
static void print_power_report(void)
{
    // Estimasi konsumsi daya sistem
    float cpu_current = 30.0;    // CPU 80MHz typical
    float flash_current = 8.0;   // Flash idle
    float base_current = 5.0;    // Sistem dasar

    float peripheral_current = 0;
    for (int i = 0; i < num_peripherals; i++) {
        if (peripherals[i].currently_enabled) {
            peripheral_current += peripherals[i].typical_current_ma;
        }
    }

    float total = cpu_current + flash_current + base_current + peripheral_current;

    ESP_LOGI(TAG, "\n============================================");
    ESP_LOGI(TAG, "  LAPORAN POWER BUDGET");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "  Komponen         | Arus (mA) | Persen");
    ESP_LOGI(TAG, "  ------------------|-----------|-------");
    ESP_LOGI(TAG, "  CPU (80MHz)       | %6.1f    | %5.1f%%", cpu_current, cpu_current / total * 100);
    ESP_LOGI(TAG, "  Flash (idle)      | %6.1f    | %5.1f%%", flash_current, flash_current / total * 100);
    ESP_LOGI(TAG, "  Sistem dasar      | %6.1f    | %5.1f%%", base_current, base_current / total * 100);
    ESP_LOGI(TAG, "  Peripheral (aktif)| %6.1f    | %5.1f%%", peripheral_current, peripheral_current / total * 100);
    ESP_LOGI(TAG, "  ------------------|-----------|-------");
    ESP_LOGI(TAG, "  TOTAL             | %6.1f    | 100.0%%", total);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "  Total hemat dari peripheral: %.1f mA", total_saved_ma);
    ESP_LOGI(TAG, "  Operasi enable/disable: %d/%d", enabled_count, disabled_count);
    ESP_LOGI(TAG, "");

    // Estimasi battery life
    float battery_mah = 2000.0f;
    float hours_full = battery_mah / (total + total_saved_ma);
    float hours_optimized = battery_mah / total;

    ESP_LOGI(TAG, "  Estimasi Battery Life (2000mAh):");
    ESP_LOGI(TAG, "    Semua peripheral ON  : %.1f jam (%.1f hari)", hours_full, hours_full / 24);
    ESP_LOGI(TAG, "    Peripheral dioptimasi: %.1f jam (%.1f hari)", hours_optimized, hours_optimized / 24);
    ESP_LOGI(TAG, "    Penghematan          : %.1f jam (%.1f%%)",
             hours_optimized - hours_full,
             (hours_optimized - hours_full) / hours_full * 100);
    ESP_LOGI(TAG, "============================================");
}

void app_main(void)
{
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 Peripheral Power Gating");
    ESP_LOGI(TAG, "============================================");

    init_led();
    gpio_set_level(LED_PIN, 1);

    // Tahap 1: Aktifkan semua peripheral
    enable_all_peripherals();
    print_peripheral_status();
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Tahap 2: Nonaktifkan satu per satu
    disable_peripherals_sequentially();
    print_peripheral_status();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Tahap 3: Selective enable
    demonstrate_selective_enable();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Tahap 4: Laporan akhir
    print_power_report();

    gpio_set_level(LED_PIN, 0);

    ESP_LOGI(TAG, "\nProgram selesai. Restart untuk mengulangi.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
