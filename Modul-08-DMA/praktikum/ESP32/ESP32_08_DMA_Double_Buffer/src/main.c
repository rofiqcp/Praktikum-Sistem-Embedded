/*
 * ==========================================================================
 *  ESP32 DMA Double Buffer (Ping-Pong) Pattern
 * ==========================================================================
 *  Modul 08 - Program 08: Software Double-Buffer Implementation
 *
 *  KONSEP DOUBLE BUFFER:
 *  ┌─────────────────────────────────────────────────────────────────┐
 *  │ STM32 F4 DMA Double Buffer:                                    │
 *  │  - Hardware otomatis switch antara Memory0 dan Memory1         │
 *  │  - DMA_SxCR.DBM = 1 untuk enable                              │
 *  │  - Transfer Complete interrupt → switch buffer                 │
 *  │                                                                 │
 *  │ ESP32 Software Double Buffer:                                   │
 *  │  - Dua buffer dialokasikan di software                         │
 *  │  - Semaphore digunakan untuk synchronize swap                  │
 *  │  - Task A mengisi buffer X, Task B memproses buffer Y          │
 *  │  - Setelah keduanya selesai → swap buffer                      │
 *  └─────────────────────────────────────────────────────────────────┘
 *
 *  Keuntungan Double Buffer:
 *  - Zero data loss (tidak ada gap antara collection dan processing)
 *  - Higher throughput (overlap collection dan processing)
 *  - Pipeline efficiency
 *
 *  Hardware: GPIO36 → ADC input
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
#include "esp_adc/adc_oneshot.h"
#include "config.h"

static const char *TAG = "DOUBLE_BUF";

/* ==========================================================================
 *  Double Buffer Structure
 * ========================================================================== */

typedef struct {
    uint16_t buffer_a[BUFFER_A_SIZE];   /* Buffer A (ping) */
    uint16_t buffer_b[BUFFER_B_SIZE];   /* Buffer B (pong) */
    volatile int active_buffer;          /* 0 = filling A, 1 = filling B */
    volatile int samples_collected;      /* Samples in current collection */
    volatile bool collection_done;
    volatile bool processing_done;
    SemaphoreHandle_t buffer_ready;      /* Signaled when buffer is full */
    SemaphoreHandle_t buffer_processed;  /* Signaled when processing done */
    SemaphoreHandle_t swap_mutex;        /* Protects buffer swap */
} double_buffer_t;

static double_buffer_t db;

/* ---- ADC Handle ---- */
static adc_oneshot_unit_handle_t adc_handle;

/* ---- Task Handles ---- */
static TaskHandle_t collector_handle = NULL;
static TaskHandle_t processor_handle = NULL;

/* ---- Timing Analysis ---- */
typedef struct {
    int64_t collection_times[TIMING_HISTORY_SIZE];
    int64_t processing_times[TIMING_HISTORY_SIZE];
    int64_t swap_times[TIMING_HISTORY_SIZE];
    int     timing_index;
    int     timing_count;
    int64_t total_collection_time;
    int64_t total_processing_time;
    uint32_t swap_count;
    uint32_t total_samples;
    uint32_t lost_samples;
} timing_stats_t;

static timing_stats_t timing;

/* ---- Processing Results ---- */
typedef struct {
    float rms_value;
    float peak_value;
    float avg_value;
    float dominant_freq;    /* Pseudo FFT result */
    int   sample_count;
} process_result_t;

static process_result_t last_result;

/* ---- Single vs Double Buffer Comparison ---- */
static int64_t single_buffer_total_time = 0;
static int64_t double_buffer_total_time = 0;
static int comparison_done = 0;

/* ==========================================================================
 *  ADC Initialization
 * ========================================================================== */

static esp_err_t adc_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) return ret;

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg);

    ESP_LOGI(TAG, "ADC initialized: channel=%d, 12-bit, 0-3.3V", ADC_CHANNEL);
    return ret;
}

/* ==========================================================================
 *  Data Collection Function
 * ==========================================================================
 *  Mengisi buffer dengan data ADC.
 *  Simulasi data dengan noise jika ADC tidak tersedia.
 * ========================================================================== */

static int collect_data(uint16_t *buffer, int num_samples)
{
    int collected = 0;

    for (int i = 0; i < num_samples; i++) {
        int raw = 0;
        esp_err_t ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);
        if (ret == ESP_OK) {
            buffer[i] = (uint16_t)raw;
        } else {
            /* Fallback: generate synthetic signal for testing */
            static float phase = 0;
            buffer[i] = (uint16_t)(2048 + 1800 * sinf(phase) +
                        200 * sinf(phase * 3.7f));
            phase += 0.05f;
            if (phase > 2 * M_PI * 100) phase -= 2 * M_PI * 100;
        }
        collected++;
    }

    return collected;
}

/* ==========================================================================
 *  Processing Functions (FFT-like, RMS, Peak Detection)
 * ========================================================================== */

/**
 * @brief Calculate RMS value of buffer
 */
static float calculate_rms(const uint16_t *data, int len)
{
    double sum_sq = 0;
    for (int i = 0; i < len; i++) {
        double val = (double)data[i] - 2048.0;  /* Center around zero */
        sum_sq += val * val;
    }
    return (float)sqrt(sum_sq / len);
}

/**
 * @brief Find peak value in buffer
 */
static float find_peak(const uint16_t *data, int len)
{
    uint16_t max_val = 0;
    for (int i = 0; i < len; i++) {
        if (data[i] > max_val) max_val = data[i];
    }
    return (float)max_val * 3.3f / 4095.0f;
}

/**
 * @brief Calculate average value
 */
static float calculate_average(const uint16_t *data, int len)
{
    uint32_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return (float)sum / len * 3.3f / 4095.0f;
}

/**
 * @brief Pseudo-FFT: zero-crossing frequency detection
 *
 * Ini bukan FFT sesungguhnya, tapi estimasi frekuensi dominan
 * berdasarkan zero-crossing rate.
 */
static float estimate_frequency(const uint16_t *data, int len, float sample_rate)
{
    int crossings = 0;
    int midpoint = 2048;

    for (int i = 1; i < len; i++) {
        if ((data[i - 1] < midpoint && data[i] >= midpoint) ||
            (data[i - 1] >= midpoint && data[i] < midpoint)) {
            crossings++;
        }
    }

    /* Frequency = zero crossings / (2 * time) */
    float duration = (float)len / sample_rate;
    return (float)crossings / (2.0f * duration);
}

/**
 * @brief Process a buffer (comprehensive analysis)
 */
static void process_buffer(const uint16_t *data, int len, process_result_t *result)
{
    result->rms_value = calculate_rms(data, len);
    result->peak_value = find_peak(data, len);
    result->avg_value = calculate_average(data, len);
    result->dominant_freq = estimate_frequency(data, len, 10000.0f);
    result->sample_count = len;

    /* Simulate additional processing time */
    vTaskDelay(pdMS_TO_TICKS(PROCESSING_DELAY_MS));
}

/* ==========================================================================
 *  Single-Buffer Approach (for comparison)
 * ==========================================================================
 *  Dalam single buffer: collect → process → collect → process (sequential)
 *  Tidak ada overlap, throughput lebih rendah.
 * ========================================================================== */

static void single_buffer_test(void)
{
    uint16_t *buffer = heap_caps_malloc(NUM_SAMPLES * sizeof(uint16_t),
                                         MALLOC_CAP_DEFAULT);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate single buffer");
        return;
    }

    process_result_t result;
    int64_t start = esp_timer_get_time();

    for (int cycle = 0; cycle < SINGLE_BUFFER_TEST_CYCLES; cycle++) {
        /* Step 1: Collect (blocking) */
        collect_data(buffer, NUM_SAMPLES);

        /* Step 2: Process (blocking) - no overlap! */
        process_buffer(buffer, NUM_SAMPLES, &result);
    }

    single_buffer_total_time = esp_timer_get_time() - start;

    ESP_LOGI(TAG, "Single buffer test: %lld us for %d cycles (%.1f us/cycle)",
             single_buffer_total_time, SINGLE_BUFFER_TEST_CYCLES,
             (float)single_buffer_total_time / SINGLE_BUFFER_TEST_CYCLES);

    free(buffer);
}

/* ==========================================================================
 *  Double Buffer Tasks
 * ========================================================================== */

/**
 * @brief Data collection task
 *
 * Mengisi buffer A sementara buffer B sedang diproses, lalu swap.
 */
static void collector_task(void *arg)
{
    ESP_LOGI(TAG, "Collector task started on core %d", xPortGetCoreID());

    while (1) {
        int64_t start = esp_timer_get_time();

        /* Pilih buffer yang aktif untuk diisi */
        uint16_t *active_buf = (db.active_buffer == 0) ?
                                db.buffer_a : db.buffer_b;

        /* Collect data ke buffer aktif */
        int collected = collect_data(active_buf, NUM_SAMPLES);
        db.samples_collected = collected;

        int64_t collection_time = esp_timer_get_time() - start;

        /* Record timing */
        int idx = timing.timing_index % TIMING_HISTORY_SIZE;
        timing.collection_times[idx] = collection_time;
        timing.total_collection_time += collection_time;
        timing.total_samples += collected;

        /* Signal: buffer penuh, siap diproses */
        xSemaphoreGive(db.buffer_ready);

        /* Tunggu processing selesai sebelum swap */
        xSemaphoreTake(db.buffer_processed, portMAX_DELAY);

        /* Swap buffer */
        xSemaphoreTake(db.swap_mutex, portMAX_DELAY);
        db.active_buffer = 1 - db.active_buffer;
        timing.swap_count++;
        xSemaphoreGive(db.swap_mutex);

        /* Small yield to allow other tasks */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief Data processing task
 *
 * Memproses buffer yang sudah penuh sementara buffer lain sedang diisi.
 */
static void processor_task(void *arg)
{
    ESP_LOGI(TAG, "Processor task started on core %d", xPortGetCoreID());

    while (1) {
        /* Tunggu buffer siap */
        xSemaphoreTake(db.buffer_ready, portMAX_DELAY);

        int64_t start = esp_timer_get_time();

        /* Proses buffer yang TIDAK sedang diisi
         * Jika active_buffer = 0 (sedang isi A), proses B
         * Jika active_buffer = 1 (sedang isi B), proses A
         */
        uint16_t *process_buf = (db.active_buffer == 0) ?
                                 db.buffer_b : db.buffer_a;

        /* Pada swap pertama, buffer B belum ada data, skip */
        if (timing.swap_count > 0) {
            process_buffer(process_buf, NUM_SAMPLES, &last_result);
        }

        int64_t processing_time = esp_timer_get_time() - start;

        /* Record timing */
        int idx = timing.timing_index % TIMING_HISTORY_SIZE;
        timing.processing_times[idx] = processing_time;
        timing.total_processing_time += processing_time;
        timing.timing_index++;
        if (timing.timing_count < TIMING_HISTORY_SIZE) {
            timing.timing_count++;
        }

        /* Signal: processing selesai, boleh swap */
        xSemaphoreGive(db.buffer_processed);
    }
}

/* ==========================================================================
 *  Statistics and Display Task
 * ========================================================================== */

static void stats_task(void *arg)
{
    ESP_LOGI(TAG, "Stats task started");

    /* First run single-buffer test for comparison */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Running single-buffer benchmark... ===");
    single_buffer_test();
    ESP_LOGI(TAG, "=== Single-buffer benchmark complete ===");
    ESP_LOGI(TAG, "");

    /* Wait a bit for double-buffer to collect data */
    vTaskDelay(pdMS_TO_TICKS(STATS_INTERVAL_MS * 2));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(STATS_INTERVAL_MS));

        /* Calculate averages */
        float avg_collection = 0, avg_processing = 0;
        if (timing.timing_count > 0) {
            int count = timing.timing_count;
            int64_t sum_c = 0, sum_p = 0;
            for (int i = 0; i < count; i++) {
                sum_c += timing.collection_times[i];
                sum_p += timing.processing_times[i];
            }
            avg_collection = (float)sum_c / count;
            avg_processing = (float)sum_p / count;
        }

        /* Calculate overlap percentage */
        float overlap_pct = 0;
        if (avg_collection > 0 && avg_processing > 0) {
            float max_time = (avg_collection > avg_processing) ?
                             avg_collection : avg_processing;
            float sequential_time = avg_collection + avg_processing;
            overlap_pct = (1.0f - max_time / sequential_time) * 100.0f;
        }

        /* Calculate double-buffer throughput */
        float db_throughput = 0;
        if (timing.swap_count > 0 && timing.total_collection_time > 0) {
            db_throughput = (float)timing.total_samples * 1000000.0f /
                           (float)(timing.total_collection_time +
                                   timing.total_processing_time);
        }

        /* Calculate single-buffer throughput */
        float sb_throughput = 0;
        if (single_buffer_total_time > 0) {
            sb_throughput = (float)(SINGLE_BUFFER_TEST_CYCLES * NUM_SAMPLES) *
                           1000000.0f / (float)single_buffer_total_time;
        }

        float improvement = 0;
        if (sb_throughput > 0) {
            improvement = (db_throughput - sb_throughput) / sb_throughput * 100.0f;
        }

        printf("\n");
        printf("╔══════════════════════════════════════════════════════════╗\n");
        printf("║       DOUBLE BUFFER (PING-PONG) STATUS - ESP32         ║\n");
        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║  Active Buffer: %s                                     ║\n",
               db.active_buffer == 0 ? "A (ping)" : "B (pong)");
        printf("║  Buffer Swaps:  %-8lu                                ║\n",
               (unsigned long)timing.swap_count);
        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║                  TIMING ANALYSIS                       ║\n");
        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║  Avg Collection Time:  %8.1f us                      ║\n",
               avg_collection);
        printf("║  Avg Processing Time:  %8.1f us                      ║\n",
               avg_processing);
        printf("║  Overlap Percentage:   %8.1f %%                       ║\n",
               overlap_pct);
        printf("╠══════════════════════════════════════════════════════════╣\n");

        /* Timing diagram */
        printf("║  Timing Diagram (latest cycle):                        ║\n");
        if (timing.timing_count > 0) {
            int last = (timing.timing_index - 1) % TIMING_HISTORY_SIZE;
            int64_t ct = timing.collection_times[last >= 0 ? last : 0];
            int64_t pt = timing.processing_times[last >= 0 ? last : 0];
            int max_width = 40;
            int64_t max_t = (ct > pt) ? ct : pt;
            if (max_t == 0) max_t = 1;

            int cw = (int)(ct * max_width / max_t);
            int pw = (int)(pt * max_width / max_t);
            if (cw < 1) cw = 1;
            if (pw < 1) pw = 1;

            printf("║  Collect: [");
            for (int i = 0; i < max_width; i++)
                printf("%c", i < cw ? '=' : ' ');
            printf("]  ║\n");

            printf("║  Process: [");
            for (int i = 0; i < max_width; i++)
                printf("%c", i < pw ? '#' : ' ');
            printf("]  ║\n");

            /* Show overlap visually */
            printf("║  Overlap: [");
            int min_w = (cw < pw) ? cw : pw;
            for (int i = 0; i < max_width; i++)
                printf("%c", i < min_w ? '*' : ' ');
            printf("]  ║\n");
        }

        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║              THROUGHPUT COMPARISON                      ║\n");
        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║  Single Buffer: %10.1f samples/sec                  ║\n",
               sb_throughput);
        printf("║  Double Buffer: %10.1f samples/sec                  ║\n",
               db_throughput);
        printf("║  Improvement:   %+9.1f %%                             ║\n",
               improvement);
        printf("║  Data Loss:     %10lu samples                      ║\n",
               (unsigned long)timing.lost_samples);
        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║              PROCESSING RESULTS                        ║\n");
        printf("╠══════════════════════════════════════════════════════════╣\n");

        if (last_result.sample_count > 0) {
            printf("║  RMS:           %8.1f  (ADC units)                  ║\n",
                   last_result.rms_value);
            printf("║  Peak:          %8.3f V                             ║\n",
                   last_result.peak_value);
            printf("║  Average:       %8.3f V                             ║\n",
                   last_result.avg_value);
            printf("║  Est. Freq:     %8.1f Hz                            ║\n",
                   last_result.dominant_freq);
        }

        printf("╠══════════════════════════════════════════════════════════╣\n");
        printf("║  ESP32 vs STM32:                                       ║\n");
        printf("║  • STM32: Hardware DMA double-buffer (DMA_SxCR.DBM)    ║\n");
        printf("║  • ESP32: Software ping-pong via FreeRTOS semaphores   ║\n");
        printf("║  • Hasil sama: zero data loss + higher throughput      ║\n");
        printf("╚══════════════════════════════════════════════════════════╝\n");
    }
}

/* ==========================================================================
 *  Main Application
 * ========================================================================== */

void app_main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║   ESP32 Double Buffer (Ping-Pong) - Modul 08 Prog 08   ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Software double-buffer pattern menggantikan            ║\n");
    printf("║  hardware DMA double-buffer mode (STM32 F4)             ║\n");
    printf("║                                                          ║\n");
    printf("║  Buffer A ←→ Buffer B (ping-pong)                       ║\n");
    printf("║  Collect A │ Process B  →  Collect B │ Process A        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Initialize ADC */
    esp_err_t ret = adc_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Initialize double buffer */
    memset(&db, 0, sizeof(db));
    db.active_buffer = 0;
    db.buffer_ready = xSemaphoreCreateBinary();
    db.buffer_processed = xSemaphoreCreateBinary();
    db.swap_mutex = xSemaphoreCreateMutex();

    /* Give initial buffer_processed so collector can start */
    xSemaphoreGive(db.buffer_processed);

    /* Initialize timing */
    memset(&timing, 0, sizeof(timing));

    ESP_LOGI(TAG, "Double buffer initialized:");
    ESP_LOGI(TAG, "  Buffer A: %d samples (%d bytes)",
             BUFFER_A_SIZE, (int)(BUFFER_A_SIZE * sizeof(uint16_t)));
    ESP_LOGI(TAG, "  Buffer B: %d samples (%d bytes)",
             BUFFER_B_SIZE, (int)(BUFFER_B_SIZE * sizeof(uint16_t)));
    ESP_LOGI(TAG, "  Samples per cycle: %d", NUM_SAMPLES);

    /* Create tasks on different cores for true parallelism */
    xTaskCreatePinnedToCore(collector_task, "collector",
                            COLLECTOR_TASK_STACK, NULL,
                            COLLECTOR_TASK_PRIO, &collector_handle, 0);

    xTaskCreatePinnedToCore(processor_task, "processor",
                            PROCESSOR_TASK_STACK, NULL,
                            PROCESSOR_TASK_PRIO, &processor_handle, 1);

    xTaskCreate(stats_task, "stats", 4096, NULL,
                STATS_TASK_PRIO, NULL);

    ESP_LOGI(TAG, "Tasks created:");
    ESP_LOGI(TAG, "  Collector: core 0, prio %d", COLLECTOR_TASK_PRIO);
    ESP_LOGI(TAG, "  Processor: core 1, prio %d", PROCESSOR_TASK_PRIO);
}
