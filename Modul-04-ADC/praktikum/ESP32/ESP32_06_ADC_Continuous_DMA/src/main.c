/**
 * ==========================================================================
 * PROGRAM 06: ADC Continuous DMA (ADC Berkelanjutan dengan DMA)
 * ==========================================================================
 * Modul 04 - ADC | Praktikum Sistem Embedded
 * 
 * Deskripsi:
 *   Program ini menggunakan mode ADC continuous (berkelanjutan) dengan
 *   DMA (Direct Memory Access) untuk pengambilan sampel berkecepatan tinggi.
 *   DMA memungkinkan transfer data ADC ke memori tanpa intervensi CPU.
 * 
 * Cara Kerja DMA ADC:
 *   1. Konfigurasi handle ADC continuous
 *   2. Konfigurasi pattern (channel + atenuasi)
 *   3. Mulai konversi berkelanjutan
 *   4. Baca hasil dari buffer DMA
 *   5. Proses dan tampilkan data
 * 
 * API yang digunakan:
 *   - adc_continuous_new_handle()  : Buat handle ADC continuous
 *   - adc_continuous_config()      : Konfigurasi konversi
 *   - adc_continuous_start()       : Mulai konversi
 *   - adc_continuous_read()        : Baca data dari buffer
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"

static const char *TAG = "ADC_DMA";

/* Konfigurasi Channel */
#if CONFIG_IDF_TARGET_ESP32
    #define ADC_CONV_CHANNEL    ADC_CHANNEL_6   /* GPIO34 */
    #define ADC_GPIO_NUM        34
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    #define ADC_CONV_CHANNEL    ADC_CHANNEL_3   /* GPIO4 */
    #define ADC_GPIO_NUM        4
#else
    #define ADC_CONV_CHANNEL    ADC_CHANNEL_6
    #define ADC_GPIO_NUM        34
#endif

#define ADC_UNIT_USED           ADC_UNIT_1
#define ADC_CONV_MODE           ADC_CONV_SINGLE_UNIT_1
#define ADC_OUTPUT_TYPE         ADC_DIGI_OUTPUT_FORMAT_TYPE1

/* Konfigurasi DMA */
#define DMA_BUFFER_SIZE         256     /* Ukuran buffer DMA dalam byte */
#define SAMPLE_FREQ_HZ          20000   /* Frekuensi sampling: 20kHz */
#define READ_LEN                256     /* Jumlah byte per pembacaan */

/* Handle ADC continuous */
static adc_continuous_handle_t adc_handle = NULL;

/**
 * @brief Callback ketika konversi selesai (opsional)
 * 
 * Callback ini dipanggil ketika satu frame konversi selesai.
 * Bisa digunakan untuk memberi notifikasi ke task utama.
 */
static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata,
                                        void *user_data)
{
    /* Bisa digunakan untuk memberi sinyal ke task via semaphore/notification */
    return false;  /* Tidak perlu yield dari ISR */
}

/**
 * @brief Inisialisasi ADC continuous mode
 */
static esp_err_t adc_continuous_init(void)
{
    /* ====== LANGKAH 1: Buat handle ADC continuous ====== */
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = 1024,         /* Ukuran buffer ring internal */
        .conv_frame_size = DMA_BUFFER_SIZE, /* Ukuran frame konversi */
    };
    
    esp_err_t ret = adc_continuous_new_handle(&handle_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal membuat handle ADC continuous: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Handle ADC continuous berhasil dibuat");

    /* ====== LANGKAH 2: Konfigurasi pattern (channel yang di-scan) ====== */
    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN_DB_11,           /* Atenuasi 11dB (0-3.3V) */
        .channel = ADC_CONV_CHANNEL,        /* Channel ADC */
        .unit = ADC_UNIT_USED,              /* Unit ADC1 */
        .bit_width = ADC_BITWIDTH_12,       /* Resolusi 12-bit */
    };

    /* ====== LANGKAH 3: Konfigurasi konversi ====== */
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_FREQ_HZ,  /* Frekuensi sampling */
        .conv_mode = ADC_CONV_MODE,         /* Mode: single unit */
        .format = ADC_OUTPUT_TYPE,          /* Format output */
        .pattern_num = 1,                   /* Jumlah pattern/channel */
        .adc_pattern = &adc_pattern,        /* Pointer ke pattern */
    };

    ret = adc_continuous_config(adc_handle, &dig_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal mengkonfigurasi ADC continuous: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Konfigurasi ADC continuous berhasil");

    /* ====== LANGKAH 4: Daftarkan callback (opsional) ====== */
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = adc_conv_done_cb,
    };
    ret = adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Gagal mendaftarkan callback: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ADC Continuous DMA Mode");
    ESP_LOGI(TAG, "  Channel   : ADC1_CH%d (GPIO%d)", ADC_CONV_CHANNEL, ADC_GPIO_NUM);
    ESP_LOGI(TAG, "  Sampling  : %d Hz", SAMPLE_FREQ_HZ);
    ESP_LOGI(TAG, "  Buffer DMA: %d bytes", DMA_BUFFER_SIZE);
    ESP_LOGI(TAG, "========================================");

    /* ====== INISIALISASI ====== */
    esp_err_t ret = adc_continuous_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Inisialisasi gagal, program berhenti.");
        return;
    }

    /* ====== MULAI KONVERSI ====== */
    ret = adc_continuous_start(adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal memulai konversi: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Konversi ADC continuous dimulai!");

    /* Buffer untuk menerima data */
    uint8_t result[READ_LEN] = {0};
    uint32_t ret_num = 0;
    int batch_counter = 0;

    /* ====== LOOP PEMBACAAN ====== */
    while (1) {
        /* Baca data dari buffer DMA */
        ret = adc_continuous_read(adc_handle, result, READ_LEN, &ret_num, 1000);

        if (ret == ESP_OK) {
            batch_counter++;

            /* Hitung jumlah sampel yang diterima */
            /* Setiap sampel = 4 byte (format TYPE1) */
            int num_samples = ret_num / SOC_ADC_DIGI_RESULT_BYTES;

            /* Proses setiap sampel dalam frame */
            uint32_t sum = 0;
            uint32_t min_val = 4095, max_val = 0;
            int valid_samples = 0;

            for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                /* Parse data sesuai format output */
                adc_digi_output_data_t *data_ptr = (adc_digi_output_data_t *)&result[i];

#if ADC_OUTPUT_TYPE == ADC_DIGI_OUTPUT_FORMAT_TYPE1
                uint32_t channel = data_ptr->type1.channel;
                uint32_t raw_val = data_ptr->type1.data;
#else
                uint32_t channel = data_ptr->type2.channel;
                uint32_t raw_val = data_ptr->type2.data;
#endif

                /* Validasi channel */
                if (channel == ADC_CONV_CHANNEL) {
                    sum += raw_val;
                    if (raw_val < min_val) min_val = raw_val;
                    if (raw_val > max_val) max_val = raw_val;
                    valid_samples++;
                }
            }

            /* Tampilkan ringkasan batch */
            if (valid_samples > 0) {
                uint32_t avg = sum / valid_samples;
                float voltage = avg * 3300.0f / 4095.0f;

                printf("[Batch %04d] Samples: %3d | Avg: %4lu | "
                       "Min: %4lu | Max: %4lu | Voltage: %7.1f mV | "
                       "Bytes: %lu\n",
                       batch_counter,
                       valid_samples,
                       (unsigned long)avg,
                       (unsigned long)min_val,
                       (unsigned long)max_val,
                       voltage,
                       (unsigned long)ret_num);
            }
        } else if (ret == ESP_ERR_TIMEOUT) {
            /* Timeout - belum ada data baru */
            ESP_LOGD(TAG, "Timeout menunggu data DMA");
        } else {
            ESP_LOGE(TAG, "Error membaca DMA: %s", esp_err_to_name(ret));
            break;
        }

        /* Delay kecil agar tidak terlalu cepat mencetak */
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* ====== CLEANUP ====== */
    adc_continuous_stop(adc_handle);
    adc_continuous_deinit(adc_handle);
    ESP_LOGI(TAG, "ADC continuous dihentikan dan di-deinit.");
}
