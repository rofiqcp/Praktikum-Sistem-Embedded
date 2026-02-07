/*
 * ===========================================================================
 * Modul 04 - ADC | ESP32_02_ADC_Multi_Channel
 * ===========================================================================
 * Deskripsi : Membaca 4 kanal ADC secara berurutan (sequential) dan
 *             menampilkan nilai mentah (raw) beserta tegangan terkalibrasi
 *             melalui Serial Monitor.
 * Framework : ESP-IDF
 * Hardware  : 4x Potensiometer 10kΩ terhubung ke pin ADC
 *
 * Pin Mapping:
 *   ESP32       -> ADC1_CH6 (GPIO34), ADC1_CH7 (GPIO35),
 *                  ADC1_CH4 (GPIO32), ADC1_CH5 (GPIO33)
 *   ESP32-S2/S3 -> ADC1_CH0 (GPIO1),  ADC1_CH1 (GPIO2),
 *                  ADC1_CH2 (GPIO3),  ADC1_CH3 (GPIO4)
 * ===========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

/* Tag untuk logging */
static const char *TAG = "ADC_MULTI";

/* Jumlah kanal ADC yang digunakan */
#define NUM_CHANNELS    4

/* Tegangan referensi default (mV) */
#define DEFAULT_VREF    1100

/* Atenuasi ADC: 11dB untuk rentang 0-3.3V */
#define ADC_ATTEN       ADC_ATTEN_DB_11

/* Resolusi ADC: 12-bit (0-4095) */
#define ADC_WIDTH       ADC_WIDTH_BIT_12

/* Jumlah sampel untuk rata-rata (multisampling) */
#define NUM_SAMPLES     64

/* ---- Konfigurasi kanal berdasarkan varian board ---- */
#if CONFIG_IDF_TARGET_ESP32

/* ESP32 klasik: GPIO34=CH6, GPIO35=CH7, GPIO32=CH4, GPIO33=CH5 */
static const adc1_channel_t adc_channels[NUM_CHANNELS] = {
    ADC1_CHANNEL_6,   /* GPIO 34 - Potensiometer 1 */
    ADC1_CHANNEL_7,   /* GPIO 35 - Potensiometer 2 */
    ADC1_CHANNEL_4,   /* GPIO 32 - Potensiometer 3 */
    ADC1_CHANNEL_5    /* GPIO 33 - Potensiometer 4 */
};
static const int gpio_nums[NUM_CHANNELS] = {34, 35, 32, 33};

#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3

/* ESP32-S2/S3: GPIO1=CH0, GPIO2=CH1, GPIO3=CH2, GPIO4=CH3 */
static const adc1_channel_t adc_channels[NUM_CHANNELS] = {
    ADC1_CHANNEL_0,   /* GPIO 1 - Potensiometer 1 */
    ADC1_CHANNEL_1,   /* GPIO 2 - Potensiometer 2 */
    ADC1_CHANNEL_2,   /* GPIO 3 - Potensiometer 3 */
    ADC1_CHANNEL_3    /* GPIO 4 - Potensiometer 4 */
};
static const int gpio_nums[NUM_CHANNELS] = {1, 2, 3, 4};

#else
#error "Board tidak didukung! Gunakan ESP32, ESP32-S2, atau ESP32-S3."
#endif

/* Handle untuk kalibrasi ADC */
static esp_adc_cal_characteristics_t adc_chars;

/**
 * @brief Inisialisasi ADC1 untuk semua kanal
 *
 * Mengatur lebar bit dan atenuasi untuk masing-masing kanal ADC.
 * Juga melakukan karakterisasi (kalibrasi) ADC menggunakan eFuse Vref
 * atau nilai default.
 */
static void adc_init(void)
{
    /* Atur resolusi ADC1 ke 12-bit */
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH));
    ESP_LOGI(TAG, "ADC1 dikonfigurasi dengan resolusi 12-bit");

    /* Konfigurasi atenuasi untuk setiap kanal */
    for (int i = 0; i < NUM_CHANNELS; i++) {
        ESP_ERROR_CHECK(adc1_config_channel_atten(adc_channels[i], ADC_ATTEN));
        ESP_LOGI(TAG, "  Kanal %d (GPIO %d) - Atenuasi 11dB (0-3.3V)",
                 i, gpio_nums[i]);
    }

    /* Karakterisasi ADC untuk konversi tegangan yang akurat */
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
        ADC_UNIT_1,         /* Unit ADC1 */
        ADC_ATTEN,          /* Atenuasi 11dB */
        ADC_WIDTH,          /* Resolusi 12-bit */
        DEFAULT_VREF,       /* Tegangan referensi default */
        &adc_chars          /* Output: karakteristik kalibrasi */
    );

    /* Tampilkan jenis kalibrasi yang digunakan */
    switch (cal_type) {
        case ESP_ADC_CAL_VAL_EFUSE_VREF:
            ESP_LOGI(TAG, "Kalibrasi: eFuse Vref");
            break;
        case ESP_ADC_CAL_VAL_EFUSE_TP:
            ESP_LOGI(TAG, "Kalibrasi: eFuse Two Point");
            break;
        default:
            ESP_LOGI(TAG, "Kalibrasi: Nilai Default Vref (%d mV)", DEFAULT_VREF);
            break;
    }
}

/**
 * @brief Membaca nilai ADC dengan multisampling untuk mengurangi noise
 *
 * @param channel Kanal ADC1 yang akan dibaca
 * @return int Nilai rata-rata ADC (0-4095)
 */
static int adc_read_multisampled(adc1_channel_t channel)
{
    int total = 0;

    /* Ambil beberapa sampel dan hitung rata-rata */
    for (int i = 0; i < NUM_SAMPLES; i++) {
        total += adc1_get_raw(channel);
    }

    return total / NUM_SAMPLES;
}

/**
 * @brief Task utama untuk membaca ADC multi-kanal secara periodik
 *
 * Task ini membaca keempat kanal ADC secara berurutan setiap 500ms,
 * menghitung tegangan terkalibrasi, dan menampilkan hasilnya.
 *
 * @param pvParameters Parameter task (tidak digunakan)
 */
static void adc_multi_channel_task(void *pvParameters)
{
    int raw_values[NUM_CHANNELS];
    uint32_t voltage_mv[NUM_CHANNELS];

    ESP_LOGI(TAG, "Mulai pembacaan ADC multi-kanal...\n");

    while (1) {
        /* Baca semua kanal secara berurutan */
        for (int i = 0; i < NUM_CHANNELS; i++) {
            /* Baca nilai mentah dengan multisampling */
            raw_values[i] = adc_read_multisampled(adc_channels[i]);

            /* Konversi ke tegangan (mV) menggunakan kalibrasi */
            voltage_mv[i] = 0;
            esp_adc_cal_get_voltage(adc_channels[i], &adc_chars, &voltage_mv[i]);
        }

        /* Tampilkan hasil pembacaan semua kanal */
        ESP_LOGI(TAG, "---- Pembacaan ADC Multi-Kanal ----");
        for (int i = 0; i < NUM_CHANNELS; i++) {
            ESP_LOGI(TAG, "  CH%d (GPIO%2d): Raw = %4d | Tegangan = %4lu mV (%.2f V)",
                     i, gpio_nums[i],
                     raw_values[i],
                     (unsigned long)voltage_mv[i],
                     voltage_mv[i] / 1000.0f);
        }
        ESP_LOGI(TAG, "-----------------------------------\n");

        /* Tunda 500ms sebelum pembacaan berikutnya */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Fungsi utama (entry point) aplikasi ESP-IDF
 */
void app_main(void)
{
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Program 02: ADC Multi-Channel - ESP-IDF");
    ESP_LOGI(TAG, "Hardware : 4x Potensiometer 10kΩ");
    ESP_LOGI(TAG, "==========================================\n");

    /* Inisialisasi ADC */
    adc_init();

    /* Buat task untuk pembacaan ADC */
    xTaskCreate(
        adc_multi_channel_task,   /* Fungsi task */
        "adc_multi_ch",           /* Nama task */
        4096,                     /* Ukuran stack (bytes) */
        NULL,                     /* Parameter task */
        5,                        /* Prioritas task */
        NULL                      /* Handle task (tidak disimpan) */
    );
}
