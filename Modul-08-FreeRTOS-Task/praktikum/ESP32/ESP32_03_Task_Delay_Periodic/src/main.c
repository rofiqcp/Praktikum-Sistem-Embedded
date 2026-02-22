/* ============================================================
 * ESP32_03_Task_Delay_Periodic - Perbandingan Delay FreeRTOS
 * ============================================================
 * Program ini membandingkan dua metode delay di FreeRTOS:
 * 1. vTaskDelay() - delay relatif (rentan drift)
 * 2. vTaskDelayUntil() - delay absolut (timing presisi)
 *
 * Konsep yang dipelajari:
 * 1. vTaskDelay() vs vTaskDelayUntil() - perbedaan fundamental
 * 2. Timing drift pada delay relatif
 * 3. Jitter analysis - stddev, max deviation
 * 4. esp_timer untuk high-resolution timing
 * 5. Periodic task design pattern
 * 6. Statistik real-time untuk analisis performa
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1
 * - LED1 GPIO2 (vTaskDelay indicator)
 * - LED2 GPIO4 (vTaskDelayUntil indicator)
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "config.h"

static const char *TAG = "DELAY_CMP";

/* Task handles */
static TaskHandle_t xDelayTaskHandle = NULL;
static TaskHandle_t xPeriodicTaskHandle = NULL;
static TaskHandle_t xStatsTaskHandle = NULL;

/* ============================================================
 * Struktur data untuk timing statistics
 * ============================================================
 * Menyimpan sample waktu untuk analisis jitter.
 * Jitter = variasi dari periode yang diharapkan.
 * Semakin kecil jitter, semakin presisi timing task.
 * ============================================================ */
typedef struct {
    int64_t samples[MAX_SAMPLES];       // Array sample periode (us)
    uint32_t sample_count;              // Jumlah sample saat ini
    uint32_t sample_index;              // Index circular buffer
    int64_t sum;                        // Total sum untuk mean
    int64_t min_period;                 // Periode minimum
    int64_t max_period;                 // Periode maximum
    int64_t last_timestamp;             // Timestamp terakhir
    uint32_t total_iterations;          // Total iterasi
    double mean;                        // Rata-rata periode
    double variance;                    // Varians
    double stddev;                      // Standar deviasi (jitter)
    int64_t max_deviation;              // Deviasi maximum dari target
} timing_stats_t;

/* Statistik untuk kedua task */
static timing_stats_t delay_stats = {0};
static timing_stats_t periodic_stats = {0};

/* Timestamp awal program */
static int64_t start_time_us = 0;

static uint32_t get_elapsed_ms(void)
{
    return (uint32_t)((esp_timer_get_time() - start_time_us) / 1000);
}

/**
 * @brief Inisialisasi struktur timing statistics
 * @param stats Pointer ke struktur stats
 */
static void init_timing_stats(timing_stats_t *stats)
{
    memset(stats, 0, sizeof(timing_stats_t));
    stats->min_period = INT64_MAX;
    stats->max_period = 0;
    stats->last_timestamp = 0;
}

/**
 * @brief Update statistik timing dengan sample baru
 * @param stats Pointer ke struktur stats
 * @param current_time Timestamp saat ini (microseconds)
 * @param target_period_us Target periode dalam microseconds
 *
 * Menghitung:
 * - Periode aktual antara dua panggilan berurutan
 * - Min/max periode
 * - Running mean dan standard deviation
 * - Maximum deviation dari target
 */
static void update_timing_stats(timing_stats_t *stats, int64_t current_time,
                                 int64_t target_period_us)
{
    if (stats->last_timestamp != 0) {
        int64_t period = current_time - stats->last_timestamp;
        int64_t deviation = period - target_period_us;

        /* Simpan sample ke circular buffer */
        stats->samples[stats->sample_index] = period;
        stats->sample_index = (stats->sample_index + 1) % MAX_SAMPLES;
        if (stats->sample_count < MAX_SAMPLES) {
            stats->sample_count++;
        }

        /* Update min/max */
        if (period < stats->min_period) stats->min_period = period;
        if (period > stats->max_period) stats->max_period = period;

        /* Update max deviation */
        int64_t abs_dev = (deviation < 0) ? -deviation : deviation;
        if (abs_dev > stats->max_deviation) {
            stats->max_deviation = abs_dev;
        }

        stats->total_iterations++;

        /* Hitung mean dan stddev dari circular buffer */
        if (stats->sample_count > 1) {
            double sum = 0;
            for (uint32_t i = 0; i < stats->sample_count; i++) {
                sum += (double)stats->samples[i];
            }
            stats->mean = sum / stats->sample_count;

            double var_sum = 0;
            for (uint32_t i = 0; i < stats->sample_count; i++) {
                double diff = (double)stats->samples[i] - stats->mean;
                var_sum += diff * diff;
            }
            stats->variance = var_sum / (stats->sample_count - 1);
            stats->stddev = sqrt(stats->variance);
        }
    }
    stats->last_timestamp = current_time;
}

/* Inisialisasi GPIO */
static void init_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED1_GPIO, 0);
    gpio_set_level(LED2_GPIO, 0);
}

/**
 * @brief Simulasi kerja ringan (workload)
 *
 * Mensimulasikan beban kerja yang memakan waktu variable,
 * yang akan mempengaruhi timing vTaskDelay() tapi TIDAK
 * mempengaruhi vTaskDelayUntil().
 */
static void simulate_work(void)
{
    volatile uint32_t dummy = 0;
    /* Kerja yang memakan waktu ~WORK_TIME_MS */
    uint32_t iterations = WORK_TIME_MS * 1000;
    for (uint32_t i = 0; i < iterations; i++) {
        dummy += i;
    }
    (void)dummy;
}

/* ============================================================
 * TASK 1: vTaskDelay() - Delay Relatif
 * ============================================================
 * vTaskDelay(ticks) men-delay task selama MINIMAL 'ticks' tick
 * DARI SAAT PEMANGGILAN. Artinya:
 *
 * Total periode = waktu_kerja + delay_ticks
 *
 * Jika waktu kerja bervariasi, total periode juga bervariasi.
 * Ini menyebabkan "drift" - periode aktual lebih panjang dari
 * yang diharapkan.
 *
 * Timeline:
 * |--work--|---delay---|-work-|---delay---|
 * |<--- drift1 ------->|<--- drift2 ----->|
 *
 * Drift terakumulasi seiring waktu!
 * ============================================================ */
static void delay_task(void *pvParameters)
{
    uint8_t led_state = 0;
    uint32_t cycle = 0;
    int64_t target_period_us = TARGET_PERIOD_MS * 1000;

    ESP_LOGI(TAG, "[DELAY] Task dimulai - menggunakan vTaskDelay()");
    ESP_LOGI(TAG, "[DELAY] Target period: %d ms", TARGET_PERIOD_MS);

    init_timing_stats(&delay_stats);

    while (1) {
        int64_t now = esp_timer_get_time();
        update_timing_stats(&delay_stats, now, target_period_us);

        /* Toggle LED */
        led_state = !led_state;
        gpio_set_level(LED1_GPIO, led_state);
        cycle++;

        /* Simulasi kerja yang memakan waktu */
        simulate_work();

        /* Print data periodik */
        if (cycle % 10 == 0) {
            int64_t actual_period = (delay_stats.last_timestamp != 0 && cycle > 1) ?
                                    delay_stats.samples[(delay_stats.sample_index - 1 + MAX_SAMPLES) % MAX_SAMPLES] : 0;

            printf("[DATA] DELAY_TASK,%lu,%lu,%lld,%.1f,%.1f,%lld\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)cycle,
                   actual_period,
                   delay_stats.mean,
                   delay_stats.stddev,
                   delay_stats.max_deviation);
        }

        /*
         * vTaskDelay() - Delay RELATIF
         *
         * Delay dimulai dari SAAT INI, bukan dari awal periode.
         * Jadi total cycle time = work_time + delay_time
         * yang selalu LEBIH LAMA dari delay_time saja.
         *
         * pdMS_TO_TICKS() mengkonversi milidetik ke tick count.
         * Pada ESP-IDF default: 1 tick = 10ms (configTICK_RATE_HZ=100)
         */
        vTaskDelay(pdMS_TO_TICKS(TARGET_PERIOD_MS));
    }
}

/* ============================================================
 * TASK 2: vTaskDelayUntil() - Delay Absolut
 * ============================================================
 * vTaskDelayUntil() men-delay task sampai waktu ABSOLUT tertentu.
 * Secara otomatis mengkompensasi waktu kerja:
 *
 * Total periode = tepat 'period' ticks (terlepas dari waktu kerja)
 *
 * Fungsi ini menyimpan "last wake time" dan menghitung
 * berapa tick yang perlu di-delay untuk mencapai target.
 *
 * Timeline:
 * |--work--|--delay1---|--work--|--delay2--|
 * |<-- exact period -->|<-- exact period ->|
 *
 * Tidak ada drift! (selama work_time < period)
 *
 * PENTING: Jika work_time > period, task akan MELEWATKAN
 * deadline dan langsung ready tanpa delay (no blocking).
 * ============================================================ */
static void periodic_task(void *pvParameters)
{
    uint8_t led_state = 0;
    uint32_t cycle = 0;
    int64_t target_period_us = TARGET_PERIOD_MS * 1000;

    ESP_LOGI(TAG, "[PERIODIC] Task dimulai - menggunakan vTaskDelayUntil()");
    ESP_LOGI(TAG, "[PERIODIC] Target period: %d ms", TARGET_PERIOD_MS);

    init_timing_stats(&periodic_stats);

    /*
     * xLastWakeTime menyimpan kapan task terakhir di-unblock.
     * xTaskGetTickCount() mendapatkan tick count saat ini.
     *
     * vTaskDelayUntil() akan mengupdate xLastWakeTime secara
     * internal, menambahkan period setiap kali dipanggil.
     */
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        int64_t now = esp_timer_get_time();
        update_timing_stats(&periodic_stats, now, target_period_us);

        led_state = !led_state;
        gpio_set_level(LED2_GPIO, led_state);
        cycle++;

        /* Simulasi kerja yang sama seperti delay task */
        simulate_work();

        /* Print data periodik */
        if (cycle % 10 == 0) {
            int64_t actual_period = (periodic_stats.last_timestamp != 0 && cycle > 1) ?
                                    periodic_stats.samples[(periodic_stats.sample_index - 1 + MAX_SAMPLES) % MAX_SAMPLES] : 0;

            printf("[DATA] PERIODIC_TASK,%lu,%lu,%lld,%.1f,%.1f,%lld\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)cycle,
                   actual_period,
                   periodic_stats.mean,
                   periodic_stats.stddev,
                   periodic_stats.max_deviation);
        }

        /*
         * vTaskDelayUntil() - Delay ABSOLUT
         *
         * Parameter:
         * 1. &xLastWakeTime: pointer ke timestamp terakhir (auto-updated)
         * 2. period: jumlah ticks antara dua wake-up
         *
         * Fungsi menghitung: next_wake = xLastWakeTime + period
         * Lalu delay sampai next_wake tercapai.
         *
         * Hasilnya: periode tepat dan konsisten, tidak terpengaruh
         * oleh waktu eksekusi task (selama < period).
         */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TARGET_PERIOD_MS));
    }
}

/* ============================================================
 * STATS TASK - Perbandingan statistik kedua metode
 * ============================================================ */
static void stats_task(void *pvParameters)
{
    uint32_t report = 0;

    vTaskDelay(pdMS_TO_TICKS(2000));  // Tunggu data terkumpul

    while (1) {
        report++;

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "========= TIMING COMPARISON (Report #%lu) =========",
                 (unsigned long)report);

        /* Statistik vTaskDelay() */
        ESP_LOGI(TAG, "vTaskDelay() Statistics:");
        ESP_LOGI(TAG, "  Samples: %lu", (unsigned long)delay_stats.sample_count);
        ESP_LOGI(TAG, "  Mean period: %.1f us (target: %d us)",
                 delay_stats.mean, TARGET_PERIOD_MS * 1000);
        ESP_LOGI(TAG, "  Std deviation: %.1f us", delay_stats.stddev);
        ESP_LOGI(TAG, "  Min period: %lld us", delay_stats.min_period);
        ESP_LOGI(TAG, "  Max period: %lld us", delay_stats.max_period);
        ESP_LOGI(TAG, "  Max deviation: %lld us", delay_stats.max_deviation);

        if (delay_stats.mean > 0) {
            double drift_pct = ((delay_stats.mean - (TARGET_PERIOD_MS * 1000.0)) /
                               (TARGET_PERIOD_MS * 1000.0)) * 100.0;
            ESP_LOGI(TAG, "  Drift: %.2f%%", drift_pct);
        }

        /* Statistik vTaskDelayUntil() */
        ESP_LOGI(TAG, "vTaskDelayUntil() Statistics:");
        ESP_LOGI(TAG, "  Samples: %lu", (unsigned long)periodic_stats.sample_count);
        ESP_LOGI(TAG, "  Mean period: %.1f us (target: %d us)",
                 periodic_stats.mean, TARGET_PERIOD_MS * 1000);
        ESP_LOGI(TAG, "  Std deviation: %.1f us", periodic_stats.stddev);
        ESP_LOGI(TAG, "  Min period: %lld us", periodic_stats.min_period);
        ESP_LOGI(TAG, "  Max period: %lld us", periodic_stats.max_period);
        ESP_LOGI(TAG, "  Max deviation: %lld us", periodic_stats.max_deviation);

        if (periodic_stats.mean > 0) {
            double drift_pct = ((periodic_stats.mean - (TARGET_PERIOD_MS * 1000.0)) /
                               (TARGET_PERIOD_MS * 1000.0)) * 100.0;
            ESP_LOGI(TAG, "  Drift: %.2f%%", drift_pct);
        }

        /* Perbandingan */
        if (delay_stats.stddev > 0 && periodic_stats.stddev > 0) {
            double improvement = (delay_stats.stddev - periodic_stats.stddev) /
                                delay_stats.stddev * 100.0;
            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, ">>> vTaskDelayUntil() %.1f%% lebih presisi! <<<",
                     improvement);
        }

        /* Print comparison data untuk Python */
        printf("[DATA] COMPARE,%lu,%lu,%lu,%.1f,%.1f,%.1f,%.1f,%lld,%lld\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)delay_stats.sample_count,
               (unsigned long)periodic_stats.sample_count,
               delay_stats.mean,
               periodic_stats.mean,
               delay_stats.stddev,
               periodic_stats.stddev,
               delay_stats.max_deviation,
               periodic_stats.max_deviation);

        /* Jitter warning */
        if (delay_stats.max_deviation > JITTER_WARN_US) {
            printf("[DATA] JITTER_WARN,DELAY,%lu,%lld\n",
                   (unsigned long)get_elapsed_ms(),
                   delay_stats.max_deviation);
            ESP_LOGW(TAG, "!!! vTaskDelay jitter melebihi threshold: %lld us",
                     delay_stats.max_deviation);
        }

        ESP_LOGI(TAG, "=============================================");

        /* Heap monitoring */
        printf("[DATA] HEAP,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)esp_get_free_heap_size());

        vTaskDelay(pdMS_TO_TICKS(STATS_PERIOD_MS));
    }
}

/* ============================================================
 * APP_MAIN
 * ============================================================ */
void app_main(void)
{
    start_time_us = esp_timer_get_time();

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 vTaskDelay vs vTaskDelayUntil Demo");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Target period: %d ms", TARGET_PERIOD_MS);
    ESP_LOGI(TAG, "Work simulation: %d ms per cycle", WORK_TIME_MS);
    ESP_LOGI(TAG, "Tick rate: %d Hz (resolution: %d ms)",
             configTICK_RATE_HZ, 1000 / configTICK_RATE_HZ);

    printf("[DATA] INIT,%lu,%d,%d,%d\n",
           (unsigned long)get_elapsed_ms(),
           TARGET_PERIOD_MS,
           WORK_TIME_MS,
           configTICK_RATE_HZ);

    init_gpio();

    /* Membuat task delay (vTaskDelay) */
    xTaskCreate(delay_task, "DelayTask", TASK_STACK_SIZE,
                NULL, DELAY_TASK_PRIORITY, &xDelayTaskHandle);

    /* Membuat task periodic (vTaskDelayUntil) */
    xTaskCreate(periodic_task, "PeriodicTask", TASK_STACK_SIZE,
                NULL, PERIODIC_TASK_PRIORITY, &xPeriodicTaskHandle);

    /* Membuat task statistics */
    xTaskCreate(stats_task, "StatsTask", TASK_STACK_SIZE * 2,
                NULL, STATS_PRIORITY, &xStatsTaskHandle);

    ESP_LOGI(TAG, "Semua task dimulai. Observasi drift vTaskDelay...");
    ESP_LOGI(TAG, "============================================");
}
