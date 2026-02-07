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
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

static const char *TAG = "ADC_MULTI";

/* Konfigurasi Channel berdasarkan target */
#if CONFIG_IDF_TARGET_ESP32
    #define CH1_CHANNEL     ADC1_CHANNEL_6   /* GPIO34 */
    #define CH2_CHANNEL     ADC1_CHANNEL_7   /* GPIO35 */
    #define CH1_GPIO        34
    #define CH2_GPIO        35
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define CH1_CHANNEL     ADC1_CHANNEL_3   /* GPIO4 */
    #define CH2_CHANNEL     ADC1_CHANNEL_4   /* GPIO5 */
    #define CH1_GPIO        4
    #define CH2_GPIO        5
#else
    #define CH1_CHANNEL     ADC1_CHANNEL_6
    #define CH2_CHANNEL     ADC1_CHANNEL_7
    #define CH1_GPIO        34
    #define CH2_GPIO        35
#endif

#define ADC_WIDTH       ADC_WIDTH_BIT_12
#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_UNIT        ADC_UNIT_1
#define DEFAULT_VREF    1100
#define READ_INTERVAL_MS    500

/* Jumlah channel yang digunakan */
#define NUM_CHANNELS    2

/* Struktur data channel */
typedef struct {
    adc1_channel_t channel;
    int gpio_num;
    const char *label;
    esp_adc_cal_characteristics_t *cal_chars;
} adc_channel_info_t;

void app_main(void)
{
    /* ====== KONFIGURASI ADC ====== */
    adc1_config_width(ADC_WIDTH);

    /* Definisi informasi channel */
    adc_channel_info_t channels[NUM_CHANNELS] = {
        {
            .channel = CH1_CHANNEL,
            .gpio_num = CH1_GPIO,
            .label = "POT-1",
            .cal_chars = NULL
        },
        {
            .channel = CH2_CHANNEL,
            .gpio_num = CH2_GPIO,
            .label = "POT-2",
            .cal_chars = NULL
        }
    };

    /* Konfigurasi setiap channel */
    for (int i = 0; i < NUM_CHANNELS; i++) {
        /* Atur atenuasi untuk masing-masing channel */
        adc1_config_channel_atten(channels[i].channel, ADC_ATTEN);

        /* Alokasi dan konfigurasi kalibrasi */
        channels[i].cal_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
        esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN, ADC_WIDTH,
                                 DEFAULT_VREF, channels[i].cal_chars);

        ESP_LOGI(TAG, "Channel %d (%s): ADC1_CH%d pada GPIO%d - Dikonfigurasi",
                 i + 1, channels[i].label, channels[i].channel, channels[i].gpio_num);
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
        uint32_t voltage[NUM_CHANNELS];

        /* Baca semua channel secara sekuensial */
        for (int i = 0; i < NUM_CHANNELS; i++) {
            /* Baca nilai mentah */
            raw[i] = adc1_get_raw(channels[i].channel);

            /* Konversi ke tegangan terkalibrasi */
            voltage[i] = esp_adc_cal_raw_to_voltage(raw[i], channels[i].cal_chars);
        }

        /* Hitung selisih antar channel */
        int32_t diff_raw = raw[0] - raw[1];
        int32_t diff_mv = (int32_t)voltage[0] - (int32_t)voltage[1];

        /* Tampilkan hasil secara berdampingan */
        counter++;
        printf("[%04d] | %4d / %4lu mV      | %4d / %4lu mV      | %+4ld mV\n",
               counter,
               raw[0], (unsigned long)voltage[0],
               raw[1], (unsigned long)voltage[1],
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
