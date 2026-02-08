/**
 * ==========================================================================
 * PROGRAM 08: ADC Battery Monitor (Monitor Baterai)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca tegangan baterai Li-Ion/Li-Po melalui voltage
 *   divider (pembagi tegangan) menggunakan dua resistor 10kΩ.
 *   
 *   Rangkaian Voltage Divider:
 *     V_bat ──[R1=10kΩ]──┬──[R2=10kΩ]── GND
 *                         │
 *                     GPIO34 (ADC)
 *   
 *   Rumus: V_bat = V_adc × (R1 + R2) / R2 = V_adc × 2
 *   
 *   Estimasi persentase baterai Li-Ion:
 *     - 4.2V = 100% (penuh)
 *     - 3.7V = ~50% (nominal)
 *     - 3.0V = 0% (kosong)
 * 
 * Koneksi Hardware:
 *   - Baterai → R1 (10kΩ) → titik tengah → R2 (10kΩ) → GND
 *   - Titik tengah → GPIO34
 * ==========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "BATT_MON";

/* Konfigurasi ADC */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CHANNEL     ADC_CHANNEL_6
    #define ADC_GPIO_NUM    34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CHANNEL     ADC_CHANNEL_3
    #define ADC_GPIO_NUM    4
#else
    #define ADC_CHANNEL     ADC_CHANNEL_6
    #define ADC_GPIO_NUM    34
#endif

#define ADC_ATTEN_LEVEL ADC_ATTEN_DB_12
#define ADC_UNIT        ADC_UNIT_1

/* Konfigurasi Voltage Divider */
#define R1_OHM          10000       /* Resistor atas: 10kΩ */
#define R2_OHM          10000       /* Resistor bawah: 10kΩ */
/* Rasio pembagi = (R1 + R2) / R2 = 2.0 */
#define DIVIDER_RATIO   ((float)(R1_OHM + R2_OHM) / R2_OHM)

/* Batas tegangan baterai Li-Ion (dalam mV) */
#define BATT_FULL_MV    4200        /* 4.2V = 100% */
#define BATT_NOMINAL_MV 3700        /* 3.7V = nominal */
#define BATT_EMPTY_MV   3000        /* 3.0V = 0% */

/* Jumlah sampel untuk rata-rata (noise reduction) */
#define NUM_SAMPLES     16

/* Interval pembacaan (ms) */
#define READ_INTERVAL_MS    2000

/* Handle ADC dan kalibrasi */
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;
static bool calibrated = false;

/**
 * @brief Estimasi persentase baterai dari tegangan
 * 
 * Menggunakan interpolasi linear sederhana antara BATT_EMPTY (0%)
 * dan BATT_FULL (100%).
 * 
 * @param voltage_mv Tegangan baterai dalam mV
 * @return Persentase baterai (0-100)
 */
static int estimate_battery_percent(uint32_t voltage_mv)
{
    if (voltage_mv >= BATT_FULL_MV) {
        return 100;
    }
    if (voltage_mv <= BATT_EMPTY_MV) {
        return 0;
    }
    /* Interpolasi linear: 3000mV=0%, 4200mV=100% */
    return (int)((voltage_mv - BATT_EMPTY_MV) * 100 / (BATT_FULL_MV - BATT_EMPTY_MV));
}

/**
 * @brief Mengembalikan label status baterai
 */
static const char* get_battery_status(int percent)
{
    if (percent >= 80) return "PENUH    ";
    if (percent >= 50) return "BAIK     ";
    if (percent >= 20) return "SEDANG   ";
    if (percent >= 10) return "RENDAH   ";
    return "KRITIS   ";
}

/**
 * @brief Menampilkan ikon baterai ASCII
 */
static void print_battery_icon(int percent)
{
    int bars = percent / 10;  /* 0-10 bar */
    printf(" [");
    for (int i = 0; i < 10; i++) {
        printf(i < bars ? "█" : "░");
    }
    printf("] ");
}

/**
 * @brief Baca ADC dengan multi-sampling (rata-rata N sampel)
 */
static int read_adc_averaged(int num_samples)
{
    int sum = 0;
    for (int i = 0; i < num_samples; i++) {
        int raw = 0;
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);
        sum += raw;
        vTaskDelay(pdMS_TO_TICKS(2));  /* Delay kecil antar sampel */
    }
    return sum / num_samples;
}

void app_main(void)
{
    /* ====== INISIALISASI ADC ONESHOT ====== */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_LEVEL,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config));

    /* Kalibrasi */
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN_LEVEL,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle) == ESP_OK) {
        calibrated = true;
        ESP_LOGI(TAG, "Kalibrasi: Curve Fitting");
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT,
            .atten = ADC_ATTEN_LEVEL,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if (adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle) == ESP_OK) {
            calibrated = true;
            ESP_LOGI(TAG, "Kalibrasi: Line Fitting");
        }
    }
#endif

    if (!calibrated) {
        ESP_LOGW(TAG, "Kalibrasi tidak tersedia, tegangan tidak akan akurat");
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Battery Monitor - Pemantau Baterai");
    ESP_LOGI(TAG, "  Channel  : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Divider  : R1=%dΩ, R2=%dΩ (Rasio=%.1f)", 
             R1_OHM, R2_OHM, DIVIDER_RATIO);
    ESP_LOGI(TAG, "  Batt Full: %d mV, Empty: %d mV", BATT_FULL_MV, BATT_EMPTY_MV);
    ESP_LOGI(TAG, "  Sampling : %d sampel per pembacaan", NUM_SAMPLES);
    ESP_LOGI(TAG, "========================================");

    int counter = 0;

    /* ====== LOOP MONITORING ====== */
    while (1) {
        /* Baca ADC dengan rata-rata multi-sampel */
        int raw_avg = read_adc_averaged(NUM_SAMPLES);

        /* Konversi ke tegangan ADC (terkalibrasi) */
        int voltage_adc = 0;
        if (calibrated) {
            adc_cali_raw_to_voltage(cali_handle, raw_avg, &voltage_adc);
        }

        /* Hitung tegangan baterai aktual (melalui voltage divider) */
        uint32_t voltage_battery = (uint32_t)(voltage_adc * DIVIDER_RATIO);

        /* Estimasi persentase baterai */
        int percent = estimate_battery_percent(voltage_battery);

        /* Dapatkan status */
        const char *status = get_battery_status(percent);

        /* Tampilkan hasil */
        counter++;
        printf("[%04d] Raw: %4d | V_ADC: %4d mV | V_Batt: %4lu mV (%d.%03d V) | "
               "%3d%% %s",
               counter,
               raw_avg,
               voltage_adc,
               (unsigned long)voltage_battery,
               (int)(voltage_battery / 1000),
               (int)(voltage_battery % 1000),
               percent,
               status);
        print_battery_icon(percent);
        printf("\n");

        /* Peringatan baterai rendah */
        if (percent <= 10) {
            printf("  ⚠️  PERINGATAN: Baterai kritis! Segera isi ulang.\n");
        } else if (percent <= 20) {
            printf("  ℹ️  Baterai rendah, pertimbangkan untuk mengisi ulang.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
