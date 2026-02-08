/*
 * ESP32_12_Timer_vs_Notification_Benchmark
 * ==========================================
 * Compare latency/overhead: timer callback vs task notification
 * vs binary semaphore. Run 1000 iterations of each.
 * Measure ticks and microseconds. Print comparison table.
 *
 * Hardware: Serial only (no GPIO needed)
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BENCHMARK";

#define ITERATIONS  1000

static TaskHandle_t xBenchmarkTaskHandle = NULL;
static TaskHandle_t xNotifyRecvTaskHandle = NULL;
static TaskHandle_t xSemaRecvTaskHandle = NULL;
static SemaphoreHandle_t xBinarySema = NULL;
static TimerHandle_t xBenchTimer = NULL;

/* Benchmark results */
typedef struct {
    char name[32];
    int64_t latencies[ITERATIONS];
    uint32_t count;
    int64_t min_us;
    int64_t max_us;
    int64_t sum_us;
    double std_dev;
} BenchResult;

static BenchResult results[3];
static volatile int64_t send_time_us = 0;
static volatile int64_t recv_time_us = 0;
static volatile bool benchmark_running = false;
static volatile bool timer_callback_fired = false;

/* ===== Timer Callback Benchmark ===== */
static void bench_timer_callback(TimerHandle_t xTimer)
{
    recv_time_us = esp_timer_get_time();
    timer_callback_fired = true;
}

static void run_timer_benchmark(BenchResult *result)
{
    strcpy(result->name, "SW_Timer_Callback");
    result->count = 0;
    result->min_us = INT64_MAX;
    result->max_us = 0;
    result->sum_us = 0;

    ESP_LOGI(TAG, "BENCH,START,%s,%d iterations", result->name, ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        timer_callback_fired = false;

        /* Measure time from start to callback execution */
        send_time_us = esp_timer_get_time();
        xTimerStart(xBenchTimer, pdMS_TO_TICKS(100));

        /* Wait for callback */
        while (!timer_callback_fired) {
            taskYIELD();
        }

        int64_t latency = recv_time_us - send_time_us;
        result->latencies[result->count] = latency;
        result->count++;
        result->sum_us += latency;
        if (latency < result->min_us) result->min_us = latency;
        if (latency > result->max_us) result->max_us = latency;

        /* Stop timer to reset for next iteration */
        xTimerStop(xBenchTimer, pdMS_TO_TICKS(100));

        if ((i + 1) % 200 == 0) {
            ESP_LOGI(TAG, "BENCH,PROGRESS,%s,%d/%d,avg=%lld_us",
                     result->name, i + 1, ITERATIONS,
                     (long long)(result->sum_us / result->count));
        }
    }

    /* Calculate std deviation */
    double avg = (double)result->sum_us / result->count;
    double sum_sq = 0;
    for (uint32_t i = 0; i < result->count; i++) {
        double diff = (double)result->latencies[i] - avg;
        sum_sq += diff * diff;
    }
    result->std_dev = sqrt(sum_sq / result->count);

    ESP_LOGI(TAG, "BENCH,DONE,%s,count=%lu,min=%lld,max=%lld,avg=%lld,stddev=%.2f",
             result->name,
             (unsigned long)result->count,
             (long long)result->min_us,
             (long long)result->max_us,
             (long long)(result->sum_us / result->count),
             result->std_dev);
}

/* ===== Notification Benchmark ===== */
static volatile bool notify_recv_ready = false;

static void notify_receiver_task(void *pvParameters)
{
    while (1) {
        notify_recv_ready = true;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        recv_time_us = esp_timer_get_time();
        notify_recv_ready = false;

        /* Signal back to benchmark task */
        xTaskNotifyGive(xBenchmarkTaskHandle);
    }
}

static void run_notification_benchmark(BenchResult *result)
{
    strcpy(result->name, "Task_Notification");
    result->count = 0;
    result->min_us = INT64_MAX;
    result->max_us = 0;
    result->sum_us = 0;

    ESP_LOGI(TAG, "BENCH,START,%s,%d iterations", result->name, ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        /* Wait for receiver to be ready */
        while (!notify_recv_ready) {
            taskYIELD();
        }

        send_time_us = esp_timer_get_time();
        xTaskNotifyGive(xNotifyRecvTaskHandle);

        /* Wait for receiver to process */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        int64_t latency = recv_time_us - send_time_us;
        result->latencies[result->count] = latency;
        result->count++;
        result->sum_us += latency;
        if (latency < result->min_us) result->min_us = latency;
        if (latency > result->max_us) result->max_us = latency;

        if ((i + 1) % 200 == 0) {
            ESP_LOGI(TAG, "BENCH,PROGRESS,%s,%d/%d,avg=%lld_us",
                     result->name, i + 1, ITERATIONS,
                     (long long)(result->sum_us / result->count));
        }
    }

    double avg = (double)result->sum_us / result->count;
    double sum_sq = 0;
    for (uint32_t i = 0; i < result->count; i++) {
        double diff = (double)result->latencies[i] - avg;
        sum_sq += diff * diff;
    }
    result->std_dev = sqrt(sum_sq / result->count);

    ESP_LOGI(TAG, "BENCH,DONE,%s,count=%lu,min=%lld,max=%lld,avg=%lld,stddev=%.2f",
             result->name,
             (unsigned long)result->count,
             (long long)result->min_us,
             (long long)result->max_us,
             (long long)(result->sum_us / result->count),
             result->std_dev);
}

/* ===== Semaphore Benchmark ===== */
static volatile bool sema_recv_ready = false;

static void sema_receiver_task(void *pvParameters)
{
    while (1) {
        sema_recv_ready = true;
        xSemaphoreTake(xBinarySema, portMAX_DELAY);
        recv_time_us = esp_timer_get_time();
        sema_recv_ready = false;

        xTaskNotifyGive(xBenchmarkTaskHandle);
    }
}

static void run_semaphore_benchmark(BenchResult *result)
{
    strcpy(result->name, "Binary_Semaphore");
    result->count = 0;
    result->min_us = INT64_MAX;
    result->max_us = 0;
    result->sum_us = 0;

    ESP_LOGI(TAG, "BENCH,START,%s,%d iterations", result->name, ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        while (!sema_recv_ready) {
            taskYIELD();
        }

        send_time_us = esp_timer_get_time();
        xSemaphoreGive(xBinarySema);

        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        int64_t latency = recv_time_us - send_time_us;
        result->latencies[result->count] = latency;
        result->count++;
        result->sum_us += latency;
        if (latency < result->min_us) result->min_us = latency;
        if (latency > result->max_us) result->max_us = latency;

        if ((i + 1) % 200 == 0) {
            ESP_LOGI(TAG, "BENCH,PROGRESS,%s,%d/%d,avg=%lld_us",
                     result->name, i + 1, ITERATIONS,
                     (long long)(result->sum_us / result->count));
        }
    }

    double avg = (double)result->sum_us / result->count;
    double sum_sq = 0;
    for (uint32_t i = 0; i < result->count; i++) {
        double diff = (double)result->latencies[i] - avg;
        sum_sq += diff * diff;
    }
    result->std_dev = sqrt(sum_sq / result->count);

    ESP_LOGI(TAG, "BENCH,DONE,%s,count=%lu,min=%lld,max=%lld,avg=%lld,stddev=%.2f",
             result->name,
             (unsigned long)result->count,
             (long long)result->min_us,
             (long long)result->max_us,
             (long long)(result->sum_us / result->count),
             result->std_dev);
}

/* ===== Print Final Comparison ===== */
static void print_final_results(void)
{
    ESP_LOGW(TAG, "");
    ESP_LOGW(TAG, "╔══════════════════════════════════════════════════════════════════════╗");
    ESP_LOGW(TAG, "║           FREERTOS MECHANISM LATENCY BENCHMARK RESULTS              ║");
    ESP_LOGW(TAG, "╠══════════════════════════════════════════════════════════════════════╣");
    ESP_LOGW(TAG, "RESULT,Method,Iterations,Min_us,Max_us,Avg_us,StdDev_us");

    for (int i = 0; i < 3; i++) {
        int64_t avg = results[i].count > 0 ? results[i].sum_us / results[i].count : 0;
        ESP_LOGI(TAG, "RESULT,%s,%lu,%lld,%lld,%lld,%.2f",
                 results[i].name,
                 (unsigned long)results[i].count,
                 (long long)results[i].min_us,
                 (long long)results[i].max_us,
                 (long long)avg,
                 results[i].std_dev);
    }

    ESP_LOGW(TAG, "╠══════════════════════════════════════════════════════════════════════╣");

    /* Find fastest */
    int fastest = 0;
    int64_t fastest_avg = results[0].count > 0 ? results[0].sum_us / results[0].count : INT64_MAX;
    for (int i = 1; i < 3; i++) {
        int64_t avg = results[i].count > 0 ? results[i].sum_us / results[i].count : INT64_MAX;
        if (avg < fastest_avg) {
            fastest_avg = avg;
            fastest = i;
        }
    }

    ESP_LOGW(TAG, "WINNER,%s with avg latency of %lld us", results[fastest].name, (long long)fastest_avg);

    /* Print relative comparisons */
    for (int i = 0; i < 3; i++) {
        if (i != fastest && results[i].count > 0) {
            int64_t avg = results[i].sum_us / results[i].count;
            double ratio = (double)avg / (double)fastest_avg;
            ESP_LOGI(TAG, "COMPARE,%s is %.2fx slower than %s",
                     results[i].name, ratio, results[fastest].name);
        }
    }

    ESP_LOGW(TAG, "╚══════════════════════════════════════════════════════════════════════╝");
}

static void benchmark_task(void *pvParameters)
{
    ESP_LOGI(TAG, "INFO,Starting benchmark in 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    benchmark_running = true;

    /* Run each benchmark */
    ESP_LOGW(TAG, "===== BENCHMARK 1/3: Software Timer =====");
    run_timer_benchmark(&results[0]);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGW(TAG, "===== BENCHMARK 2/3: Task Notification =====");
    run_notification_benchmark(&results[1]);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGW(TAG, "===== BENCHMARK 3/3: Binary Semaphore =====");
    run_semaphore_benchmark(&results[2]);
    vTaskDelay(pdMS_TO_TICKS(1000));

    benchmark_running = false;

    /* Print results */
    print_final_results();

    /* Repeat every 30 seconds */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        ESP_LOGI(TAG, "INFO,Re-running benchmark...");
        benchmark_running = true;

        run_timer_benchmark(&results[0]);
        vTaskDelay(pdMS_TO_TICKS(500));
        run_notification_benchmark(&results[1]);
        vTaskDelay(pdMS_TO_TICKS(500));
        run_semaphore_benchmark(&results[2]);

        benchmark_running = false;
        print_final_results();
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Timer vs Notification vs Semaphore Benchmark ===");
    ESP_LOGI(TAG, "INFO,Iterations per test: %d", ITERATIONS);
    ESP_LOGI(TAG, "INFO,Measuring task-to-task latency for each mechanism");
    ESP_LOGI(TAG, "INFO,Mechanisms: SW Timer callback, Task Notification, Binary Semaphore");

    /* Create sync primitives */
    xBinarySema = xSemaphoreCreateBinary();

    /* Create timer for benchmark (one-shot, 1 tick minimum period) */
    xBenchTimer = xTimerCreate(
        "BenchTimer",
        1,          /* Minimum period = 1 tick */
        pdFALSE,    /* One-shot */
        NULL,
        bench_timer_callback
    );

    if (xBenchTimer == NULL || xBinarySema == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create timer or semaphore!");
        return;
    }

    /* Create receiver tasks */
    xTaskCreate(notify_receiver_task, "notify_recv", 4096, NULL, 12, &xNotifyRecvTaskHandle);
    xTaskCreate(sema_receiver_task, "sema_recv", 4096, NULL, 12, &xSemaRecvTaskHandle);

    /* Create benchmark task (higher priority so it can yield properly) */
    xTaskCreate(benchmark_task, "benchmark", 8192, NULL, 10, &xBenchmarkTaskHandle);

    /* app_main just monitors */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        if (!benchmark_running) {
            ESP_LOGI(TAG, "STATUS,benchmark_idle,tick=%lu",
                     (unsigned long)xTaskGetTickCount());
        }
    }
}
