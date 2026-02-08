/**
 * ===========================================================================
 *  PROGRAM 5: ESP32 Multi-Channel ADC Scan with DMA
 * ===========================================================================
 *
 *  DESKRIPSI:
 *  Multi-channel ADC scanning menggunakan DMA yang terintegrasi di
 *  peripheral ADC. ESP-IDF adc_continuous API mengelola pattern table
 *  untuk scan beberapa channel secara bergantian (interleaved sampling).
 *
 *  Fitur:
 *    - 3 channel ADC scanning via DMA (GPIO36, GPIO39, GPIO34)
 *    - De-interleave: pisahkan data per channel dari buffer gabungan
 *    - Per-channel statistics: average, RMS, min, max
 *    - Cross-channel correlation analysis
 *    - Channel comparison table
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
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_idf_version.h"
#include "soc/soc_caps.h"
#include "config.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_adc/adc_continuous.h"
#else
#include "driver/adc.h"
#endif

static const char *TAG = "ADC_MULTI";

/* Channel identifiers for de-interleaving */
static const int channel_ids[NUM_ADC_CHANNELS] = {
    ADC_CH0, ADC_CH1, ADC_CH2
};

static const int channel_gpios[NUM_ADC_CHANNELS] __attribute__((unused)) = {
    ADC_CH0_GPIO, ADC_CH1_GPIO, ADC_CH2_GPIO
};

static const char *channel_names[NUM_ADC_CHANNELS] = {
    "CH0 (GPIO36)", "CH1 (GPIO39)", "CH2 (GPIO34)"
};

/* Per-channel statistics */
typedef struct {
    uint32_t sample_count;
    double   sum;
    double   sum_sq;
    uint32_t min_val;
    uint32_t max_val;
    uint32_t last_value;
} channel_stats_t;

static channel_stats_t ch_stats[NUM_ADC_CHANNELS];

/* Correlation data */
static uint32_t correlation_buffer[NUM_ADC_CHANNELS][256]; /* Recent samples */
static int corr_idx = 0;

/* ========================= Statistics Functions ========================= */
static void stats_init_all(void)
{
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        ch_stats[i].sample_count = 0;
        ch_stats[i].sum = 0;
        ch_stats[i].sum_sq = 0;
        ch_stats[i].min_val = UINT32_MAX;
        ch_stats[i].max_val = 0;
        ch_stats[i].last_value = 0;
    }
    corr_idx = 0;
}

static void stats_update_channel(int ch, uint32_t value)
{
    if (ch < 0 || ch >= NUM_ADC_CHANNELS) return;

    channel_stats_t *s = &ch_stats[ch];
    s->sample_count++;
    s->sum += value;
    s->sum_sq += (double)value * value;
    if (value < s->min_val) s->min_val = value;
    if (value > s->max_val) s->max_val = value;
    s->last_value = value;

    /* Store for correlation */
    correlation_buffer[ch][corr_idx % 256] = value;
}

static double calculate_correlation(int ch_a, int ch_b, int n)
{
    /**
     * Pearson correlation coefficient between two channels.
     * r = Σ((xi - x̄)(yi - ȳ)) / sqrt(Σ(xi - x̄)² * Σ(yi - ȳ)²)
     */
    if (n < 2) return 0.0;
    if (n > 256) n = 256;

    double sum_a = 0, sum_b = 0;
    for (int i = 0; i < n; i++) {
        sum_a += correlation_buffer[ch_a][i];
        sum_b += correlation_buffer[ch_b][i];
    }
    double mean_a = sum_a / n;
    double mean_b = sum_b / n;

    double cov = 0, var_a = 0, var_b = 0;
    for (int i = 0; i < n; i++) {
        double da = correlation_buffer[ch_a][i] - mean_a;
        double db = correlation_buffer[ch_b][i] - mean_b;
        cov += da * db;
        var_a += da * da;
        var_b += db * db;
    }

    double denom = sqrt(var_a * var_b);
    if (denom < 1e-10) return 0.0;
    return cov / denom;
}

static void print_channel_stats(int64_t elapsed_us)
{
    double elapsed_s = elapsed_us / 1e6;

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           ESP32 Multi-Channel ADC — Statistics (%.1f s)        ║\n", elapsed_s);
    printf("╠═══════════════╦════════════╦════════╦════════╦════════╦═════════════╣\n");
    printf("║ Channel       ║  Samples   ║  Avg   ║  Min   ║  Max   ║  RMS        ║\n");
    printf("╠═══════════════╬════════════╬════════╬════════╬════════╬═════════════╣\n");

    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        channel_stats_t *s = &ch_stats[i];
        if (s->sample_count > 0) {
            double avg = s->sum / s->sample_count;
            double rms = sqrt(s->sum_sq / s->sample_count);
            double avg_v = (avg / 4095.0) * 3.3;

            printf("║ %-13s ║ %10u ║ %6.1f ║ %6u ║ %6u ║ %8.1f    ║\n",
                   channel_names[i], (unsigned)s->sample_count,
                   avg, (unsigned)s->min_val, (unsigned)s->max_val, rms);

            /* Also print voltage */
            printf("[ADC_MULTI] CH%d: avg=%.1f min=%u max=%u rms=%.1f voltage=%.3fV samples=%u\n",
                   i, avg, (unsigned)s->min_val, (unsigned)s->max_val,
                   rms, avg_v, (unsigned)s->sample_count);
        } else {
            printf("║ %-13s ║          0 ║   N/A  ║   N/A  ║   N/A  ║   N/A       ║\n",
                   channel_names[i]);
        }
    }

    printf("╚═══════════════╩════════════╩════════╩════════╩════════╩═════════════╝\n");

    /* Cross-channel correlation */
    int n = corr_idx;
    if (n > 256) n = 256;
    if (n >= 10) {
        printf("\n  Cross-Channel Correlation (Pearson r, n=%d):\n", n);
        printf("  CH0-CH1: r = %+.4f\n", calculate_correlation(0, 1, n));
        printf("  CH0-CH2: r = %+.4f\n", calculate_correlation(0, 2, n));
        printf("  CH1-CH2: r = %+.4f\n", calculate_correlation(1, 2, n));
        printf("  (r ≈ +1: positively correlated, r ≈ 0: uncorrelated, r ≈ -1: anti-correlated)\n");

        printf("[CORR] CH0_CH1=%.4f CH0_CH2=%.4f CH1_CH2=%.4f\n",
               calculate_correlation(0, 1, n),
               calculate_correlation(0, 2, n),
               calculate_correlation(1, 2, n));
    }

    printf("\n");
}

/* ========================= ESP-IDF 5.x Implementation ========================= */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

static adc_continuous_handle_t adc_handle = NULL;

static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata,
                                        void *user_data)
{
    TaskHandle_t task = (TaskHandle_t)user_data;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken);
    return (xHigherPriorityTaskWoken == pdTRUE);
}

static esp_err_t adc_multi_init(TaskHandle_t notify_task)
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

    /* Configure multi-channel pattern */
    adc_digi_pattern_config_t patterns[NUM_ADC_CHANNELS];
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        patterns[i].atten = ADC_ATTEN;
        patterns[i].channel = channel_ids[i];
        patterns[i].unit = ADC_UNIT;
        patterns[i].bit_width = ADC_BITWIDTH;
    }

    adc_continuous_config_t config = {
        .pattern_num = NUM_ADC_CHANNELS,
        .adc_pattern = patterns,
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
    adc_continuous_register_event_callbacks(adc_handle, &cbs, notify_task);

    return ESP_OK;
}

static void adc_multi_task(void *pvParameters)
{
    uint8_t *result_buf = malloc(READ_LEN_BYTES);
    if (!result_buf) {
        ESP_LOGE(TAG, "Failed to allocate read buffer!");
        vTaskDelete(NULL);
        return;
    }

    /* Start ADC continuous mode */
    esp_err_t ret = adc_continuous_start(adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_continuous_start failed: %s", esp_err_to_name(ret));
        free(result_buf);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Multi-channel ADC DMA started!");
    ESP_LOGI(TAG, "Scanning %d channels at %d Hz total", NUM_ADC_CHANNELS, SAMPLE_RATE_HZ);

    int64_t start_time = esp_timer_get_time();
    int64_t last_print = start_time;

    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        uint32_t out_length = 0;
        ret = adc_continuous_read(adc_handle, result_buf, READ_LEN_BYTES,
                                  &out_length, pdMS_TO_TICKS(100));

        if (ret == ESP_OK && out_length > 0) {
            adc_digi_output_data_t *data = (adc_digi_output_data_t *)result_buf;
            int num_samples = out_length / sizeof(adc_digi_output_data_t);

            /* De-interleave: separate channels from combined buffer */
            for (int i = 0; i < num_samples; i++) {
                uint32_t chan = data[i].type1.channel;
                uint32_t val  = data[i].type1.data;

                /* Map channel to index */
                for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
                    if ((int)chan == channel_ids[ch]) {
                        stats_update_channel(ch, val);
                        break;
                    }
                }
            }
            corr_idx++;
        }

        /* Periodic statistics */
        int64_t now = esp_timer_get_time();
        if ((now - last_print) >= (STATS_INTERVAL_MS * 1000LL)) {
            print_channel_stats(now - start_time);

            /* Reset per-interval stats */
            for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
                ch_stats[i].sample_count = 0;
                ch_stats[i].sum = 0;
                ch_stats[i].sum_sq = 0;
                ch_stats[i].min_val = UINT32_MAX;
                ch_stats[i].max_val = 0;
            }
            last_print = now;
        }
    }

    adc_continuous_stop(adc_handle);
    adc_continuous_deinit(adc_handle);
    free(result_buf);
    vTaskDelete(NULL);
}

#else /* Fallback for ESP-IDF < 5.0 */

static void adc_multi_polling_task(void *pvParameters)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CH0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_CH1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC_CH2, ADC_ATTEN_DB_11);

    ESP_LOGW(TAG, "Using polling fallback (no adc_continuous API)");

    int64_t start_time = esp_timer_get_time();
    int64_t last_print = start_time;
    int sample_delay_us = 1000000 / (SAMPLE_RATE_HZ / NUM_ADC_CHANNELS);

    adc1_channel_t channels[] = {ADC_CH0, ADC_CH1, ADC_CH2};

    while (1) {
        for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
            int raw = adc1_get_raw(channels[ch]);
            if (raw >= 0) {
                stats_update_channel(ch, (uint32_t)raw);
            }
        }
        corr_idx++;
        esp_rom_delay_us(sample_delay_us);

        int64_t now = esp_timer_get_time();
        if ((now - last_print) >= (STATS_INTERVAL_MS * 1000LL)) {
            print_channel_stats(now - start_time);
            for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
                ch_stats[i].sample_count = 0;
                ch_stats[i].sum = 0;
                ch_stats[i].sum_sq = 0;
                ch_stats[i].min_val = UINT32_MAX;
                ch_stats[i].max_val = 0;
            }
            last_print = now;
        }
    }

    vTaskDelete(NULL);
}

#endif /* ESP_IDF_VERSION */

void app_main(void)
{
    printf("\n\n");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " ESP32 Multi-Channel ADC Scan (DMA)");
    ESP_LOGI(TAG, " Modul 08 — DMA (ADC Multi-Channel)");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Channels:");
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        ESP_LOGI(TAG, "  %s (ADC1_CH%d)", channel_names[i], channel_ids[i]);
    }
    ESP_LOGI(TAG, "Sample rate: %d Hz per channel", SAMPLE_RATE_HZ / NUM_ADC_CHANNELS);
    ESP_LOGI(TAG, "");

    stats_init_all();

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ESP_LOGI(TAG, "Using adc_continuous API (DMA-backed multi-channel)");

    TaskHandle_t task_handle = NULL;
    xTaskCreate(adc_multi_task, "adc_multi_task", ADC_TASK_STACK_SIZE,
                NULL, ADC_TASK_PRIORITY, &task_handle);

    esp_err_t ret = adc_multi_init(task_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC multi-channel init failed!");
        return;
    }
#else
    ESP_LOGW(TAG, "ESP-IDF < 5.0 — using polling fallback");
    xTaskCreate(adc_multi_polling_task, "adc_poll_task", ADC_TASK_STACK_SIZE,
                NULL, ADC_TASK_PRIORITY, NULL);
#endif

    ESP_LOGI(TAG, "Connect analog signals to GPIO36, GPIO39, GPIO34.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
