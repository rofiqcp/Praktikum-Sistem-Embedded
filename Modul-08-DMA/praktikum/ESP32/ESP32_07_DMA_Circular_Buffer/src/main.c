/*
 * ==========================================================================
 *  ESP32 DMA Circular Buffer - Main Program
 * ==========================================================================
 *  Modul 08 - Program 07: Circular Buffer Pattern with ADC DMA
 *
 *  KONSEP DMA di ESP32 vs STM32:
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │ STM32: DMA Controller ← → Memory ← → Peripheral               │
 *  │        General-purpose DMA channel, circular mode built-in     │
 *  │                                                                 │
 *  │ ESP32: Peripheral (ADC) → Internal DMA → Memory                │
 *  │        DMA terintegrasi di peripheral, tidak bisa diakses      │
 *  │        langsung. Circular buffer diimplementasikan di software. │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  Program ini mengimplementasikan:
 *  1. ADC continuous mode (DMA-backed) untuk akuisisi data
 *  2. Ring buffer manual dengan write_pos dan read_pos
 *  3. Producer task: baca dari ADC DMA → tulis ke ring buffer
 *  4. Consumer task: baca dari ring buffer → proses (moving average)
 *  5. Demonstrasi: producer lebih cepat dari consumer → buffer penuh
 *  6. Statistik real-time: overflow, underflow, fill level
 *
 *  Hardware:
 *  - GPIO36 (VP) → Input analog (potentiometer / sensor)
 * ==========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_continuous.h"
#include "config.h"

static const char *TAG = "CIRCULAR_BUF";

/* ==========================================================================
 *  Ring Buffer Implementation
 * ==========================================================================
 *  Di STM32, DMA circular mode otomatis wrap-around di hardware.
 *  Di ESP32, kita implementasikan ring buffer secara manual di software.
 *
 *  Layout:
 *  [0][1][2]...[write_pos]...[read_pos]...[RING_BUFFER_SIZE-1]
 *       ↑ written data ↑        ↑ data to read ↑
 * ========================================================================== */
typedef struct {
    uint16_t data[RING_BUFFER_SIZE];    /* Buffer data */
    volatile int write_pos;              /* Posisi tulis (producer) */
    volatile int read_pos;               /* Posisi baca (consumer) */
    volatile int count;                  /* Jumlah data dalam buffer */
    SemaphoreHandle_t mutex;             /* Mutex untuk akses buffer */

    /* Statistik */
    uint32_t total_written;
    uint32_t total_read;
    uint32_t overflow_count;             /* Data di-drop karena buffer penuh */
    uint32_t underflow_count;            /* Consumer membaca buffer kosong */
    uint32_t max_fill_level;             /* Maximum fill level tercapai */
    int64_t  last_write_time;
    int64_t  last_read_time;
    float    write_rate;                 /* Samples/second */
    float    read_rate;                  /* Samples/second */
} ring_buffer_t;

static ring_buffer_t rb;

/* ---- ADC Continuous Handle ---- */
static adc_continuous_handle_t adc_handle = NULL;

/* ---- Task Handles ---- */
static TaskHandle_t producer_task_handle = NULL;
static TaskHandle_t consumer_task_handle = NULL;
static TaskHandle_t stats_task_handle = NULL;

/* ---- Moving Average Filter State ---- */
static uint16_t avg_window[MOVING_AVG_WINDOW];
static int avg_index = 0;
static uint32_t avg_sum = 0;
static int avg_count = 0;

/* ---- Processing Results ---- */
static float processed_avg = 0;
static float processed_min = 4095;
static float processed_max = 0;
static uint32_t processed_samples = 0;

/* ==========================================================================
 *  Ring Buffer Functions
 * ========================================================================== */

/**
 * @brief Inisialisasi ring buffer
 *
 * Reset semua posisi dan statistik ke nol.
 */
static void ring_buffer_init(ring_buffer_t *rb)
{
    memset(rb->data, 0, sizeof(rb->data));
    rb->write_pos = 0;
    rb->read_pos = 0;
    rb->count = 0;
    rb->mutex = xSemaphoreCreateMutex();

    rb->total_written = 0;
    rb->total_read = 0;
    rb->overflow_count = 0;
    rb->underflow_count = 0;
    rb->max_fill_level = 0;
    rb->last_write_time = esp_timer_get_time();
    rb->last_read_time = esp_timer_get_time();
    rb->write_rate = 0;
    rb->read_rate = 0;

    ESP_LOGI(TAG, "Ring buffer initialized: size=%d, chunk=%d",
             RING_BUFFER_SIZE, CHUNK_SIZE);
}

/**
 * @brief Tulis data ke ring buffer
 *
 * @param rb Pointer ke ring buffer
 * @param data Data yang akan ditulis
 * @param len Jumlah sample yang akan ditulis
 * @return Jumlah sample yang berhasil ditulis
 *
 * Jika buffer penuh, data lama akan di-overwrite (drop oldest).
 * Ini berbeda dengan STM32 DMA circular mode yang otomatis overwrite.
 */
static int ring_buffer_write(ring_buffer_t *rb, const uint16_t *data, int len)
{
    int written = 0;

    xSemaphoreTake(rb->mutex, portMAX_DELAY);

    for (int i = 0; i < len; i++) {
        if (rb->count >= RING_BUFFER_SIZE) {
            /* Buffer penuh - drop oldest data (overwrite) */
            rb->overflow_count++;
            rb->read_pos = (rb->read_pos + 1) % RING_BUFFER_SIZE;
            rb->count--;
        }

        rb->data[rb->write_pos] = data[i];
        rb->write_pos = (rb->write_pos + 1) % RING_BUFFER_SIZE;
        rb->count++;
        written++;
    }

    rb->total_written += written;

    /* Update max fill level */
    if ((uint32_t)rb->count > rb->max_fill_level) {
        rb->max_fill_level = rb->count;
    }

    /* Calculate write rate */
    int64_t now = esp_timer_get_time();
    int64_t elapsed = now - rb->last_write_time;
    if (elapsed > 0) {
        rb->write_rate = (float)written * 1000000.0f / (float)elapsed;
    }
    rb->last_write_time = now;

    xSemaphoreGive(rb->mutex);
    return written;
}

/**
 * @brief Baca data dari ring buffer
 *
 * @param rb Pointer ke ring buffer
 * @param data Buffer output
 * @param len Jumlah sample yang ingin dibaca
 * @return Jumlah sample yang berhasil dibaca
 */
static int ring_buffer_read(ring_buffer_t *rb, uint16_t *data, int len)
{
    int read_count = 0;

    xSemaphoreTake(rb->mutex, portMAX_DELAY);

    if (rb->count == 0) {
        rb->underflow_count++;
        xSemaphoreGive(rb->mutex);
        return 0;
    }

    int available = rb->count;
    int to_read = (len < available) ? len : available;

    for (int i = 0; i < to_read; i++) {
        data[i] = rb->data[rb->read_pos];
        rb->read_pos = (rb->read_pos + 1) % RING_BUFFER_SIZE;
        read_count++;
    }

    rb->count -= read_count;
    rb->total_read += read_count;

    /* Calculate read rate */
    int64_t now = esp_timer_get_time();
    int64_t elapsed = now - rb->last_read_time;
    if (elapsed > 0) {
        rb->read_rate = (float)read_count * 1000000.0f / (float)elapsed;
    }
    rb->last_read_time = now;

    xSemaphoreGive(rb->mutex);
    return read_count;
}

/**
 * @brief Dapatkan fill level ring buffer (percentage)
 */
static float ring_buffer_fill_pct(ring_buffer_t *rb)
{
    return (float)rb->count * 100.0f / (float)RING_BUFFER_SIZE;
}

/* ==========================================================================
 *  ADC Continuous Mode Initialization (DMA-backed)
 * ==========================================================================
 *  ESP32 ADC continuous mode menggunakan DMA internal:
 *  - Data ADC ditransfer ke memory via DMA tanpa intervensi CPU
 *  - Mirip dengan STM32 ADC + DMA, tapi DMA tidak bisa dikonfigurasi
 *    secara terpisah - semuanya terintegrasi dalam driver ADC
 * ========================================================================== */

/**
 * @brief Callback ketika ADC DMA selesai konversi satu frame
 */
static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata,
                                        void *user_data)
{
    BaseType_t must_yield = pdFALSE;

    /* Notify producer task bahwa data DMA sudah siap */
    vTaskNotifyGiveFromISR(producer_task_handle, &must_yield);

    return (must_yield == pdTRUE);
}

/**
 * @brief Inisialisasi ADC continuous mode dengan DMA
 */
static esp_err_t adc_continuous_init(void)
{
    esp_err_t ret;

    /* Konfigurasi ADC continuous handle */
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = DMA_POOL_SIZE * SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
        .conv_frame_size = DMA_CONV_FRAME_SIZE * SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
    };

    ret = adc_continuous_new_handle(&adc_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC continuous handle: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* Konfigurasi ADC digital controller
     *
     * Di STM32: kita konfigurasi ADC channel, lalu DMA channel terpisah
     * Di ESP32: konfigurasi ADC sudah termasuk DMA internal
     */
    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN,
        .channel = ADC_CHANNEL,
        .unit = ADC_UNIT,
        .bit_width = ADC_BIT_WIDTH,
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_RATE_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 1,
        .adc_pattern = &adc_pattern,
    };

    ret = adc_continuous_config(adc_handle, &dig_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC continuous: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* Register callback untuk DMA completion */
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = adc_conv_done_cb,
    };
    ret = adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callbacks: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ADC continuous mode initialized:");
    ESP_LOGI(TAG, "  Channel: GPIO36 (ADC1_CH0)");
    ESP_LOGI(TAG, "  Sample rate: %d Hz", SAMPLE_RATE_HZ);
    ESP_LOGI(TAG, "  DMA frame size: %d samples", DMA_CONV_FRAME_SIZE);
    ESP_LOGI(TAG, "  DMA pool size: %d samples", DMA_POOL_SIZE);

    return ESP_OK;
}

/* ==========================================================================
 *  Moving Average Filter
 * ==========================================================================
 *  Filter sederhana yang dijalankan oleh consumer task.
 *  Window size = MOVING_AVG_WINDOW samples.
 * ========================================================================== */

static uint16_t moving_average_process(uint16_t new_sample)
{
    /* Subtract oldest value from sum */
    avg_sum -= avg_window[avg_index];

    /* Add new value */
    avg_window[avg_index] = new_sample;
    avg_sum += new_sample;

    /* Advance index with wrap-around */
    avg_index = (avg_index + 1) % MOVING_AVG_WINDOW;

    if (avg_count < MOVING_AVG_WINDOW) {
        avg_count++;
    }

    return (uint16_t)(avg_sum / avg_count);
}

/* ==========================================================================
 *  Producer Task
 * ==========================================================================
 *  Membaca data dari ADC DMA buffer dan menulis ke ring buffer.
 *  Task ini berjalan lebih cepat (PRODUCER_PERIOD_MS) dari consumer
 *  untuk mendemonstrasikan buffer fill-up.
 * ========================================================================== */

static void producer_task(void *arg)
{
    esp_err_t ret;
    uint8_t result[DMA_CONV_FRAME_SIZE * SOC_ADC_DIGI_DATA_BYTES_PER_CONV];
    uint32_t ret_num = 0;
    uint16_t adc_values[DMA_CONV_FRAME_SIZE];

    ESP_LOGI(TAG, "Producer task started (period=%dms)", PRODUCER_PERIOD_MS);

    /* Start ADC continuous mode */
    ret = adc_continuous_start(adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ADC continuous: %s",
                 esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        /* Tunggu notifikasi dari DMA callback */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        /* Baca data dari ADC DMA buffer
         *
         * Di STM32: DMA langsung menulis ke memory, kita akses langsung
         * Di ESP32: kita gunakan adc_continuous_read() untuk mendapatkan
         *           data dari DMA buffer internal
         */
        ret = adc_continuous_read(adc_handle, result, sizeof(result),
                                   &ret_num, 0);

        if (ret == ESP_OK && ret_num > 0) {
            int num_samples = ret_num / SOC_ADC_DIGI_DATA_BYTES_PER_CONV;

            /* Parse ADC output data */
            for (int i = 0; i < num_samples; i++) {
                adc_digi_output_data_t *p =
                    (adc_digi_output_data_t *)&result[i * SOC_ADC_DIGI_DATA_BYTES_PER_CONV];
                adc_values[i] = p->type1.data;
            }

            /* Tulis ke ring buffer */
            int written = ring_buffer_write(&rb, adc_values, num_samples);

            if (written < num_samples) {
                /* Sebagian data tidak bisa ditulis (seharusnya tidak terjadi
                 * karena kita menggunakan overwrite strategy) */
                ESP_LOGD(TAG, "Producer: partial write %d/%d",
                         written, num_samples);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(PRODUCER_PERIOD_MS));
    }
}

/* ==========================================================================
 *  Consumer Task
 * ==========================================================================
 *  Membaca data dari ring buffer dan memproses (moving average filter).
 *  Task ini berjalan lebih lambat dari producer untuk mendemonstrasikan
 *  bahwa buffer bisa terisi penuh.
 * ========================================================================== */

static void consumer_task(void *arg)
{
    uint16_t chunk[CHUNK_SIZE];

    ESP_LOGI(TAG, "Consumer task started (period=%dms, chunk=%d)",
             CONSUMER_PERIOD_MS, CHUNK_SIZE);

    while (1) {
        /* Baca dari ring buffer */
        int num_read = ring_buffer_read(&rb, chunk, CHUNK_SIZE);

        if (num_read > 0) {
            /* Proses data dengan moving average filter */
            for (int i = 0; i < num_read; i++) {
                uint16_t filtered = moving_average_process(chunk[i]);

                /* Update processing statistics */
                float voltage = (float)filtered * 3.3f / 4095.0f;
                processed_avg = (processed_avg * processed_samples + voltage) /
                                (processed_samples + 1);
                if (voltage < processed_min) processed_min = voltage;
                if (voltage > processed_max) processed_max = voltage;
                processed_samples++;
            }
        }

        /* Consumer delay lebih lama dari producer → buffer terisi */
        vTaskDelay(pdMS_TO_TICKS(CONSUMER_PERIOD_MS));
    }
}

/* ==========================================================================
 *  Statistics Task
 * ==========================================================================
 *  Menampilkan status ring buffer secara real-time termasuk visual bar.
 * ========================================================================== */

/**
 * @brief Print visual bar menunjukkan fill level buffer
 *
 * [████████████████░░░░░░░░░░░░░░░░░░░░░░░░]  45.2%
 */
static void print_visual_bar(float fill_pct)
{
    int filled = (int)(fill_pct * VISUAL_BAR_WIDTH / 100.0f);
    if (filled > VISUAL_BAR_WIDTH) filled = VISUAL_BAR_WIDTH;

    char bar[VISUAL_BAR_WIDTH + 3];
    bar[0] = '[';
    for (int i = 0; i < VISUAL_BAR_WIDTH; i++) {
        if (i < filled) {
            bar[i + 1] = '#';
        } else {
            bar[i + 1] = '.';
        }
    }
    bar[VISUAL_BAR_WIDTH + 1] = ']';
    bar[VISUAL_BAR_WIDTH + 2] = '\0';

    printf("  Buffer: %s %5.1f%%\n", bar, fill_pct);
}

static void stats_task(void *arg)
{
    ESP_LOGI(TAG, "Statistics task started (interval=%dms)",
             STATS_PRINT_INTERVAL_MS);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STATS_PRINT_INTERVAL_MS));

        float fill_pct = ring_buffer_fill_pct(&rb);

        printf("\n");
        printf("╔════════════════════════════════════════════════════════╗\n");
        printf("║          CIRCULAR BUFFER STATUS (ESP32 DMA)           ║\n");
        printf("╠════════════════════════════════════════════════════════╣\n");

        /* Visual bar */
        print_visual_bar(fill_pct);

        printf("  ──────────────────────────────────────────────────\n");

        /* Buffer positions */
        printf("  Write pos:   %4d  |  Read pos:   %4d\n",
               rb.write_pos, rb.read_pos);
        printf("  Count:       %4d  |  Capacity:   %4d\n",
               rb.count, RING_BUFFER_SIZE);
        printf("  ──────────────────────────────────────────────────\n");

        /* Transfer statistics */
        printf("  Total written: %8lu  |  Write rate: %8.1f sps\n",
               (unsigned long)rb.total_written, rb.write_rate);
        printf("  Total read:    %8lu  |  Read rate:  %8.1f sps\n",
               (unsigned long)rb.total_read, rb.read_rate);
        printf("  ──────────────────────────────────────────────────\n");

        /* Error statistics */
        printf("  Overflows:  %6lu  (data dropped)\n",
               (unsigned long)rb.overflow_count);
        printf("  Underflows: %6lu  (empty reads)\n",
               (unsigned long)rb.underflow_count);
        printf("  Max fill:   %6lu  (%.1f%%)\n",
               (unsigned long)rb.max_fill_level,
               (float)rb.max_fill_level * 100.0f / RING_BUFFER_SIZE);
        printf("  ──────────────────────────────────────────────────\n");

        /* Processing results */
        if (processed_samples > 0) {
            printf("  Processed: %lu samples\n",
                   (unsigned long)processed_samples);
            printf("  Avg: %.3fV  Min: %.3fV  Max: %.3fV\n",
                   processed_avg, processed_min, processed_max);
        }

        printf("╚════════════════════════════════════════════════════════╝\n");

        /* Warning if buffer is getting full */
        if (fill_pct > 80.0f) {
            printf("  ⚠ WARNING: Buffer fill > 80%%! Consumer terlalu lambat.\n");
        }
        if (rb.overflow_count > 0) {
            printf("  ⚠ OVERFLOW detected: %lu samples dropped.\n",
                   (unsigned long)rb.overflow_count);
            printf("    → Producer lebih cepat dari consumer.\n");
            printf("    → Solusi: tingkatkan consumer speed atau perbesar buffer.\n");
        }

        /*
         * PENJELASAN ESP32 vs STM32 DMA Circular Mode:
         *
         * STM32:
         * - DMA_Mode_Circular: hardware otomatis wrap pointer
         * - Half-Transfer dan Transfer-Complete interrupt
         * - Tidak perlu mutex karena hardware yang handle
         *
         * ESP32:
         * - Tidak ada DMA circular mode di hardware
         * - Ring buffer diimplementasikan di software
         * - Perlu mutex untuk synchronize producer-consumer
         * - ADC continuous mode menggunakan DMA internal
         *   tapi kita perlu buffer software di atasnya
         */
    }
}

/* ==========================================================================
 *  Main Application
 * ========================================================================== */

void app_main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║   ESP32 DMA Circular Buffer - Modul 08 Program 07    ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  Ring buffer pattern dengan ADC continuous mode (DMA) ║\n");
    printf("║                                                       ║\n");
    printf("║  ESP32 DMA vs STM32 DMA:                              ║\n");
    printf("║  • STM32: DMA circular mode di hardware               ║\n");
    printf("║  • ESP32: Ring buffer software + ADC DMA internal     ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    /* Inisialisasi ring buffer */
    ring_buffer_init(&rb);

    /* Inisialisasi ADC continuous mode (DMA-backed) */
    esp_err_t ret = adc_continuous_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed! Aborting.");
        return;
    }

    /* Buat tasks
     *
     * Producer berjalan lebih cepat (10ms) dari consumer (50ms)
     * untuk mendemonstrasikan buffer filling up.
     */
    xTaskCreatePinnedToCore(producer_task, "producer",
                            PRODUCER_TASK_STACK, NULL,
                            PRODUCER_TASK_PRIORITY, &producer_task_handle, 0);

    xTaskCreatePinnedToCore(consumer_task, "consumer",
                            CONSUMER_TASK_STACK, NULL,
                            CONSUMER_TASK_PRIORITY, &consumer_task_handle, 1);

    xTaskCreate(stats_task, "stats",
                4096, NULL, 3, &stats_task_handle);

    ESP_LOGI(TAG, "All tasks created:");
    ESP_LOGI(TAG, "  Producer: core 0, priority %d, period %dms",
             PRODUCER_TASK_PRIORITY, PRODUCER_PERIOD_MS);
    ESP_LOGI(TAG, "  Consumer: core 1, priority %d, period %dms",
             CONSUMER_TASK_PRIORITY, CONSUMER_PERIOD_MS);
    ESP_LOGI(TAG, "  Stats:    any core, priority 3, period %dms",
             STATS_PRINT_INTERVAL_MS);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Producer lebih cepat → buffer akan terisi → overflow!");
}
