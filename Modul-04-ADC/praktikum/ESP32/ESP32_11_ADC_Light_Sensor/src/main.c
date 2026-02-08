/**
 * ==========================================================================
 * PROGRAM 11: ADC Light Sensor (Sensor Cahaya LDR)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca nilai LDR (Light Dependent Resistor) melalui
 *   rangkaian voltage divider dengan resistor 10kΩ.
 *   
 *   Rangkaian:
 *     3.3V ──[LDR]──┬──[R=10kΩ]── GND
 *                    │
 *                GPIO34 (ADC)
 * 
 *   Semakin terang cahaya → resistansi LDR turun → tegangan ADC naik
 *   Semakin gelap → resistansi LDR naik → tegangan ADC turun
 * 
 *   Konversi ke perkiraan lux menggunakan kurva logaritmik:
 *   LDR resistance ≈ R_ref × (ADC_ref / ADC_val)
 *   Lux ≈ K / (R_ldr ^ gamma)  (perkiraan kasar)
 * 
 * Klasifikasi Level Cahaya:
 *   - Dark   : < 10 lux
 *   - Dim    : 10-100 lux  
 *   - Normal : 100-500 lux
 *   - Bright : > 500 lux
 * ==========================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "LDR_SENS";

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

#define ADC_ATTEN       ADC_ATTEN_DB_12

/* Konfigurasi Hardware */
#define R_DIVIDER       10000.0f    /* Resistor pembagi: 10kΩ */
#define V_SUPPLY        3300.0f     /* Tegangan supply: 3.3V (dalam mV) */

/* Konstanta untuk konversi LDR ke Lux (perkiraan) */
/* Nilai ini bervariasi per tipe LDR, sesuaikan sesuai datasheet */
#define LDR_K           500000.0f   /* Konstanta proporsionalitas */
#define LDR_GAMMA       0.7f        /* Eksponen gamma */

/* Ambang batas klasifikasi (dalam lux) */
#define LUX_DARK        10.0f
#define LUX_DIM         100.0f
#define LUX_NORMAL      500.0f

/* Interval pembacaan (ms) */
#define READ_INTERVAL_MS    1000

/* Jumlah sampel untuk rata-rata */
#define NUM_SAMPLES     8

/* Handle ADC dan kalibrasi */
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;

/**
 * @brief Inisialisasi kalibrasi ADC
 * 
 * Menggunakan curve fitting (ESP32, ESP32S2) atau line fitting (ESP32C3, dll.)
 * secara otomatis berdasarkan dukungan platform.
 * 
 * @return true jika kalibrasi berhasil
 */
static bool adc_calibration_init(void)
{
    esp_err_t ret;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Kalibrasi: menggunakan Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Kalibrasi: menggunakan Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
#else
    ESP_LOGW(TAG, "Kalibrasi tidak didukung pada platform ini");
    return false;
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Kalibrasi ADC berhasil");
        return true;
    } else {
        ESP_LOGW(TAG, "Kalibrasi ADC gagal: %s", esp_err_to_name(ret));
        return false;
    }
}

/**
 * @brief Hitung resistansi LDR dari tegangan ADC
 * 
 * Dari rangkaian voltage divider:
 *   V_adc = V_supply × R_divider / (R_ldr + R_divider)
 *   R_ldr = R_divider × (V_supply - V_adc) / V_adc
 * 
 * @param voltage_mv Tegangan ADC dalam mV
 * @return Resistansi LDR dalam Ohm
 */
static float calculate_ldr_resistance(float voltage_mv)
{
    if (voltage_mv <= 0) return 999999.0f;  /* Hindari pembagian nol */
    return R_DIVIDER * (V_SUPPLY - voltage_mv) / voltage_mv;
}

/**
 * @brief Perkirakan nilai lux dari resistansi LDR
 * 
 * Menggunakan model empiris: Lux ≈ K / (R_ldr ^ gamma)
 * Ini adalah perkiraan kasar, untuk akurasi tinggi diperlukan kalibrasi
 * 
 * @param resistance Resistansi LDR dalam Ohm
 * @return Perkiraan nilai lux
 */
static float estimate_lux(float resistance)
{
    if (resistance <= 0) return 9999.0f;
    return LDR_K / powf(resistance, LDR_GAMMA);
}

/**
 * @brief Klasifikasi level cahaya
 */
static const char* classify_light(float lux)
{
    if (lux < LUX_DARK)    return "GELAP  ";
    if (lux < LUX_DIM)     return "REDUP  ";
    if (lux < LUX_NORMAL)  return "NORMAL ";
    return "TERANG ";
}

/**
 * @brief Tampilkan bar visual level cahaya
 */
static void print_light_bar(float lux, int width)
{
    /* Skala logaritmik: 1-10000 lux */
    float log_lux = (lux > 0) ? log10f(lux) : 0;
    int filled = (int)(log_lux * width / 4.0f);  /* 4 = log10(10000) */
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    printf(" ☀ [");
    for (int i = 0; i < width; i++) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("] ");
}

void app_main(void)
{
    /* ====== INISIALISASI ADC ONESHOT ====== */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    /* Inisialisasi kalibrasi */
    bool cali_ok = adc_calibration_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Light Sensor (LDR) Reader");
    ESP_LOGI(TAG, "  Channel  : ADC1_CH%d (GPIO%d)", ADC_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  R divider: %.0f Ohm", R_DIVIDER);
    ESP_LOGI(TAG, "  Interval : %d ms", READ_INTERVAL_MS);
    ESP_LOGI(TAG, "  Kalibrasi: %s", cali_ok ? "Ya" : "Tidak");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Klasifikasi:");
    ESP_LOGI(TAG, "    GELAP  : < %.0f lux", LUX_DARK);
    ESP_LOGI(TAG, "    REDUP  : %.0f - %.0f lux", LUX_DARK, LUX_DIM);
    ESP_LOGI(TAG, "    NORMAL : %.0f - %.0f lux", LUX_DIM, LUX_NORMAL);
    ESP_LOGI(TAG, "    TERANG : > %.0f lux", LUX_NORMAL);
    ESP_LOGI(TAG, "========================================");

    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca ADC dengan multi-sampling */
        int raw_sum = 0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            int adc_raw;
            adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw);
            raw_sum += adc_raw;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        int raw_avg = raw_sum / NUM_SAMPLES;

        /* Konversi ke tegangan terkalibrasi */
        int voltage_mv = 0;
        if (cali_ok) {
            adc_cali_raw_to_voltage(cali_handle, raw_avg, &voltage_mv);
        }

        /* Hitung resistansi LDR */
        float r_ldr = calculate_ldr_resistance((float)voltage_mv);

        /* Estimasi lux */
        float lux = estimate_lux(r_ldr);

        /* Klasifikasi level cahaya */
        const char *level = classify_light(lux);

        /* Tampilkan hasil */
        counter++;
        printf("[%04d] Raw: %4d | %4d mV | R_LDR: %8.0f Ω | "
               "Lux: %8.1f | %s",
               counter,
               raw_avg,
               voltage_mv,
               r_ldr,
               lux,
               level);
        print_light_bar(lux, 25);
        printf("\n");

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
