/**
 * ===========================================================================
 *  PROGRAM 4: ESP32 ADC Continuous (DMA) Mode
 * ===========================================================================
 *
 *  DESKRIPSI:
 *  ADC DMA terintegrasi — tidak perlu setup DMA channel manual.
 *  ESP-IDF menyediakan API `adc_continuous` yang mengelola DMA buffer
 *  secara internal. Data ADC di-stream langsung ke memory via DMA yang
 *  tertanam di peripheral ADC.
 *
 *  Fitur:
 *    - Continuous sampling menggunakan DMA-backed ADC
 *    - Kalkulasi statistik: average, min, max, RMS
 *    - Deteksi frekuensi sinyal via zero-crossing
 *    - Print statistik setiap detik
 *
 *  CATATAN:
 *  Menggunakan `#if ESP_IDF_VERSION` untuk kompatibilitas API.
 *  ESP-IDF 5.x menggunakan adc_continuous_*
 *  ESP-IDF 4.x menggunakan adc_digi_* atau fallback ke adc1_get_raw()
 *
 *  PLATFORM : ESP32 (ESP-IDF / PlatformIO)
 *  AUTHOR   : Praktikum Sistem Embedded
 * ===========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_idf_version.h"
#include "config.h"

/*
 * Include appropriate headers based on ESP-IDF version.
 * ESP-IDF 5.x uses esp_adc/adc_continuous.h
 * Older versions use driver/adc.h
 */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_adc/adc_continuous.h"
#else
#include "driver/adc.h"
/* For older ESP-IDF, we'll use task-based sampling as fallback */
#endif

#include "soc/soc_caps.h"

static const char *TAG = "ADC_DMA";

/* Statistics */
typedef struct {
    uint32_t sample_count;
    uint32_t total_samples;
    double   sum;
    double   sum_sq;
    uint32_t min_val;
    uint32_t max_val;
    uint32_t zero_crossings;
    bool     last_above_threshold;
    int64_t  start_time_us;
} adc_stats_t;

static adc_stats_t stats = {0};

/* ========================= Statistics Functions ========================= */
static void stats_init(void)
{
    memset(&stats, 0, sizeof(stats));
    stats.min_val = UINT32_MAX;
    stats.max_val = 0;
    stats.last_above_threshold = false;
    stats.start_time_us = esp_timer_get_time();
}

static void stats_update(uint32_t value)
{
    stats.sample_count++;
    stats.total_samples++;
    stats.sum += value;
    stats.sum_sq += (double)value * value;

    if (value < stats.min_val) stats.min_val = value;
    if (value > stats.max_val) stats.max_val = value;

    /* Zero-crossing detection */
    bool above = (value >= ZERO_CROSSING_THRESHOLD);
    if (above != stats.last_above_threshold) {
        stats.zero_crossings++;
    }
    stats.last_above_threshold = above;
}

static void stats_print_and_reset(void)
{
    if (stats.sample_count == 0) return;

    double avg = stats.sum / stats.sample_count;
    double rms = sqrt(stats.sum_sq / stats.sample_count);
    int64_t elapsed = esp_timer_get_time() - stats.start_time_us;
    double elapsed_s = elapsed / 1e6;
    double actual_rate = stats.total_samples / elapsed_s;

    /* Estimate frequency from zero crossings */
    /* Each full cycle has 2 zero crossings */
    double freq_hz = 0;
    if (elapsed_s > 0 && stats.zero_crossings > 2) {
        freq_hz = stats.zero_crossings / (2.0 * elapsed_s);
    }

    /* Convert ADC value to voltage (12-bit, 3.3V range with 12dB attenuation) */
    double avg_voltage = (avg / 4095.0) * 3.3;
    double min_voltage = (stats.min_val / 4095.0) * 3.3;
    double max_voltage = (stats.max_val / 4095.0) * 3.3;

    printf("\n");
    printf("[ADC_STATS] Samples=%u | Rate=%.0f Hz\n",
           (unsigned)stats.sample_count, actual_rate);
    printf("[ADC_STATS] Raw  — Avg=%.1f  Min=%u  Max=%u  RMS=%.1f\n",
           avg, (unsigned)stats.min_val, (unsigned)stats.max_val, rms);
    printf("[ADC_STATS] Volt — Avg=%.3fV  Min=%.3fV  Max=%.3fV\n",
           avg_voltage, min_voltage, max_voltage);
    printf("[ADC_STATS] ZeroCrossings=%u  EstFreq=%.1f Hz\n",
           (unsigned)stats.zero_crossings, freq_hz);
    printf("[ADC_STATS] TotalSamples=%u  Uptime=%.1f s\n",
           (unsigned)stats.total_samples, elapsed_s);

    /* Reset per-interval stats (keep total and timing) */
    stats.sample_count = 0;
    stats.sum = 0;
    stats.sum_sq = 0;
    stats.min_val = UINT32_MAX;
    stats.max_val = 0;
    stats.zero_crossings = 0;
}

/* ========================= ESP-IDF 5.x: adc_continuous API ========================= */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

static adc_continuous_handle_t adc_handle = NULL;

static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata,
                                        void *user_data)
{
    /* Callback when conversion is done — we can use TaskNotify here */
    TaskHandle_t task = (TaskHandle_t)user_data;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken);
    return (xHigherPriorityTaskWoken == pdTRUE);
}

static esp_err_t adc_continuous_init(TaskHandle_t notify_task)
{
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = READ_LEN_BYTES * 4,
        .conv_frame_size = READ_LEN_BYTES,
    };

    esp_err_t ret = adc_continuous_new_handle(&handle_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_new_handle failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure ADC pattern */
    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN,
        .channel = ADC_CHANNEL,
        .unit = ADC_UNIT,
        .bit_width = ADC_BITWIDTH,
    };

    adc_continuous_config_t config = {
        .pattern_num = 1,
        .adc_pattern = &adc_pattern,
        .sample_freq_hz = SAMPLE_RATE_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };

    ret = adc_continuous_config(adc_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register callback */
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = adc_conv_done_cb,
    };
    ret = adc_continuous_register_event_callbacks(adc_handle, &cbs, notify_task);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Callback registration failed: %s", esp_err_to_name(ret));
    }

    return ESP_OK;
}

static void adc_continuous_task(void *pvParameters)
{
    uint8_t *result_buf = malloc(READ_LEN_BYTES);
    if (!result_buf) {
        ESP_LOGE(TAG, "Failed to allocate ADC read buffer!");
        vTaskDelete(NULL);
        return;
    }

    /* Start continuous ADC */
    esp_err_t ret = adc_continuous_start(adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_start failed: %s", esp_err_to_name(ret));
        free(result_buf);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "ADC continuous DMA started! Rate=%d Hz", SAMPLE_RATE_HZ);
    ESP_LOGI(TAG, "ADC DMA terintegrasi — tidak perlu setup DMA channel manual");

    int64_t last_print = esp_timer_get_time();

    while (1) {
        /* Wait for conversion done notification */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        uint32_t out_length = 0;
        ret = adc_continuous_read(adc_handle, result_buf, READ_LEN_BYTES,
                                  &out_length, pdMS_TO_TICKS(100));

        if (ret == ESP_OK && out_length > 0) {
            /* Parse ADC results */
            adc_digi_output_data_t *data = (adc_digi_output_data_t *)result_buf;
            int num_samples = out_length / sizeof(adc_digi_output_data_t);

            for (int i = 0; i < num_samples; i++) {
                /* TYPE1 format for ESP32 */
                uint32_t chan = data[i].type1.channel;
                uint32_t val  = data[i].type1.data;

                if (chan == ADC_CHANNEL) {
                    stats_update(val);
                }
            }
        } else if (ret == ESP_ERR_TIMEOUT) {
            /* No data available — normal during low activity */
        } else if (ret != ESP_OK) {
            ESP_LOGW(TAG, "adc_continuous_read error: %s", esp_err_to_name(ret));
        }

        /* Periodic statistics */
        int64_t now = esp_timer_get_time();
        if ((now - last_print) >= (STATS_INTERVAL_MS * 1000LL)) {
            stats_print_and_reset();
            last_print = now;
        }
    }

    adc_continuous_stop(adc_handle);
    adc_continuous_deinit(adc_handle);
    free(result_buf);
    vTaskDelete(NULL);
}

#else /* ESP-IDF < 5.0 — Fallback to task-based adc1_get_raw() */

/*
 * FALLBACK: Untuk ESP-IDF versi lama yang tidak mendukung adc_continuous API,
 * gunakan adc1_get_raw() dengan task-based sampling.
 * Ini BUKAN DMA — ini adalah polling approach sebagai alternatif.
 */

#include "driver/adc.h"
#include "esp_adc_cal.h"

static void adc_polling_task(void *pvParameters)
{
    /* Configure ADC1 */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);

    ESP_LOGI(TAG, "ADC polling mode (fallback — no continuous DMA API in this ESP-IDF)");
    ESP_LOGW(TAG, "Upgrade to ESP-IDF 5.x for true DMA-backed ADC continuous mode");

    int64_t last_print = esp_timer_get_time();
    int sample_delay_us = 1000000 / SAMPLE_RATE_HZ;

    while (1) {
        int raw = adc1_get_raw(ADC_CHANNEL);
        if (raw >= 0) {
            stats_update((uint32_t)raw);
        }

        /* Delay for approximate sample rate */
        esp_rom_delay_us(sample_delay_us);

        /* Periodic statistics */
        int64_t now = esp_timer_get_time();
        if ((now - last_print) >= (STATS_INTERVAL_MS * 1000LL)) {
            stats_print_and_reset();
            last_print = now;
        }
    }

    vTaskDelete(NULL);
}

#endif /* ESP_IDF_VERSION check */

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 ADC Continuous (DMA) Mode");
    ESP_LOGI(TAG, " Modul 08 — DMA (ADC)");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Arsitektur DMA ESP32 vs STM32:");
    ESP_LOGI(TAG, "  STM32: DMA controller terpisah (DMA1/DMA2)");
    ESP_LOGI(TAG, "         ADC → DMA Channel → Memory");
    ESP_LOGI(TAG, "  ESP32: DMA terintegrasi di ADC peripheral");
    ESP_LOGI(TAG, "         ADC internal DMA → Memory");
    ESP_LOGI(TAG, "  Hasilnya sama: sampling kontinu tanpa CPU intervention");
    ESP_LOGI(TAG, "");

    stats_init();

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ESP_LOGI(TAG, "Using ESP-IDF 5.x adc_continuous API (DMA-backed)");

    TaskHandle_t adc_task_handle = NULL;
    xTaskCreate(adc_continuous_task, "adc_dma_task", ADC_TASK_STACK_SIZE,
                NULL, ADC_TASK_PRIORITY, &adc_task_handle);

    /* Initialize ADC continuous with task handle for notification */
    esp_err_t ret = adc_continuous_init(adc_task_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC continuous init failed!");
        return;
    }
#else
    ESP_LOGW(TAG, "ESP-IDF < 5.0 detected — using polling fallback");
    ESP_LOGW(TAG, "(adc_continuous API not available in this version)");

    xTaskCreate(adc_polling_task, "adc_poll_task", ADC_TASK_STACK_SIZE,
                NULL, ADC_TASK_PRIORITY, NULL);
#endif

    ESP_LOGI(TAG, "ADC task started. Connect signal to GPIO36.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
