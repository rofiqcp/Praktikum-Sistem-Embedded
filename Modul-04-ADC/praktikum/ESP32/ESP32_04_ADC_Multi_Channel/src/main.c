/**
 * ==========================================================================
 * PROGRAM 04: ADC Multi Channel (Pembacaan Multi Channel ADC)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini membaca dua potensiometer pada dua channel ADC1 secara
 *   berurutan (sekuensial). Kedua nilai ditampilkan secara berdampingan
 *   beserta konversi ke tegangan.
 * 
 * Koneksi Hardware:
 *   ESP32:
 *     - Pot 1: Wiper → GPIO34 (ADC1_CHANNEL_6)
 *     - Pot 2: Wiper → GPIO35 (ADC1_CHANNEL_7)
 *   ESP32-S2/S3:
 *     - Pot 1: Wiper → GPIO4 (ADC1_CHANNEL_3)
 *     - Pot 2: Wiper → GPIO5 (ADC1_CHANNEL_4)
 * 
 * API yang digunakan (ESP-IDF v5.x Oneshot + Calibration API):
 *   - adc_oneshot_new_unit()                    : Membuat unit handle ADC
 *   - adc_oneshot_config_channel()              : Konfigurasi channel
 *   - adc_oneshot_read()                        : Membaca nilai mentah ADC
 *   - adc_cali_create_scheme_curve_fitting()    : Buat handle kalibrasi (ESP32)
 *   - adc_cali_create_scheme_line_fitting()     : Buat handle kalibrasi (ESP32-S2/S3/C3)
 *   - adc_cali_raw_to_voltage()                 : Konversi raw ke tegangan terkalibrasi
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "ADC_MULTI";

/* Konfigurasi Channel berdasarkan target */
#if CONFIG_IDF_TARGET_ESP32
    #define CH1_CHANNEL     ADC_CHANNEL_6    /* GPIO34 */
    #define CH2_CHANNEL     ADC_CHANNEL_7    /* GPIO35 */
    #define CH1_GPIO        34
    #define CH2_GPIO        35
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define CH1_CHANNEL     ADC_CHANNEL_3    /* GPIO4 */
    #define CH2_CHANNEL     ADC_CHANNEL_4    /* GPIO5 */
    #define CH1_GPIO        4
    #define CH2_GPIO        5
#else
    #define CH1_CHANNEL     ADC_CHANNEL_6
    #define CH2_CHANNEL     ADC_CHANNEL_7
    #define CH1_GPIO        34
    #define CH2_GPIO        35
#endif

#define ADC_ATTEN       ADC_ATTEN_DB_12
#define READ_INTERVAL_MS    500

/* Jumlah channel yang digunakan */
#define NUM_CHANNELS    2

/* Struktur data channel */
typedef struct {
    adc_channel_t channel;
    int gpio_num;
    const char *label;
    adc_cali_handle_t cali_handle;
    bool cali_enabled;
} adc_channel_info_t;

/**
 * @brief Inisialisasi kalibrasi ADC untuk satu channel
 * 
 * Otomatis memilih skema kalibrasi berdasarkan target:
 * - ESP32: Curve Fitting
 * - ESP32-S2/S3/C3: Line Fitting
 * 
 * @param unit Unit ADC
 * @param atten Atenuasi
 * @param out_handle Pointer ke handle kalibrasi (output)
 * @return true jika kalibrasi berhasil
 */
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    /* Skema Curve Fitting (tersedia di ESP32, ESP32-S2) */
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    /* Skema Line Fitting (tersedia di ESP32-C3, ESP32-S3, dll.) */
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
#endif

    if (ret == ESP_OK) {
        *out_handle = handle;
        return true;
    }

    ESP_LOGW(TAG, "Kalibrasi gagal atau tidak didukung: %s", esp_err_to_name(ret));
    return false;
}

void app_main(void)
{
    /* ====== KONFIGURASI ADC (Oneshot) ====== */
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    /* Definisi informasi channel */
    adc_channel_info_t channels[NUM_CHANNELS] = {
        {
            .channel = CH1_CHANNEL,
            .gpio_num = CH1_GPIO,
            .label = "POT-1",
            .cali_handle = NULL,
            .cali_enabled = false
        },
        {
            .channel = CH2_CHANNEL,
            .gpio_num = CH2_GPIO,
            .label = "POT-2",
            .cali_handle = NULL,
            .cali_enabled = false
        }
    };

    /* Konfigurasi setiap channel */
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };

    for (int i = 0; i < NUM_CHANNELS; i++) {
        /* Konfigurasi channel (atenuasi + bitwidth) */
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channels[i].channel, &chan_config));

        /* Inisialisasi kalibrasi untuk masing-masing channel */
        channels[i].cali_enabled = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN, &channels[i].cali_handle);

        ESP_LOGI(TAG, "Channel %d (%s): ADC1_CH%d pada GPIO%d - Dikonfigurasi (kalibrasi: %s)",
                 i + 1, channels[i].label, channels[i].channel, channels[i].gpio_num,
                 channels[i].cali_enabled ? "YA" : "TIDAK");
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Multi Channel - 2 Potensiometer");
    ESP_LOGI(TAG, "  CH1: GPIO%d (%s)", CH1_GPIO, channels[0].label);
    ESP_LOGI(TAG, "  CH2: GPIO%d (%s)", CH2_GPIO, channels[1].label);
    ESP_LOGI(TAG, "========================================");

    /* Cetak header tabel */
    printf("\n%-6s | %-20s | %-20s | %-10s\n",
           "No", "POT-1 (Raw / mV)", "POT-2 (Raw / mV)", "Selisih");
    printf("-------+----------------------+----------------------+-----------\n");

    int counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        int raw[NUM_CHANNELS];
        int voltage[NUM_CHANNELS];

        /* Baca semua channel secara sekuensial */
        for (int i = 0; i < NUM_CHANNELS; i++) {
            /* Baca nilai mentah */
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channels[i].channel, &raw[i]));

            /* Konversi ke tegangan terkalibrasi */
            voltage[i] = 0;
            if (channels[i].cali_enabled) {
                ESP_ERROR_CHECK(adc_cali_raw_to_voltage(channels[i].cali_handle, raw[i], &voltage[i]));
            }
        }

        /* Hitung selisih antar channel */
        int32_t diff_mv = (int32_t)voltage[0] - (int32_t)voltage[1];

        /* Tampilkan hasil secara berdampingan */
        counter++;
        printf("[%04d] | %4d / %4d mV      | %4d / %4d mV      | %+4ld mV\n",
               counter,
               raw[0], voltage[0],
               raw[1], voltage[1],
               (long)diff_mv);

        /* Tampilkan bar visual setiap 10 pembacaan */
        if (counter % 10 == 0) {
            int bar1 = raw[0] * 30 / 4095;
            int bar2 = raw[1] * 30 / 4095;
            printf("  POT1: [");
            for (int i = 0; i < 30; i++) printf(i < bar1 ? "#" : ".");
            printf("] %d%%\n", (int)(raw[0] * 100 / 4095));
            printf("  POT2: [");
            for (int i = 0; i < 30; i++) printf(i < bar2 ? "#" : ".");
            printf("] %d%%\n\n", (int)(raw[1] * 100 / 4095));
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }
}
