/**
 * ============================================================================
 * PROGRAM 07: ESP32 Task Core Affinity
 * ============================================================================
 * Demonstrasi fitur dual-core ESP32 dengan FreeRTOS
 * 
 * ESP32 memiliki 2 core:
 *   - Core 0: Protocol CPU (WiFi, BT, sistem)
 *   - Core 1: Application CPU (default untuk app_main)
 * 
 * Fitur utama:
 *   - xTaskCreatePinnedToCore() untuk pin task ke core tertentu
 *   - xPortGetCoreID() untuk cek core mana yang menjalankan task
 *   - Perbandingan performa: pinned vs unpinned (floating) task
 *   - Pengukuran waktu eksekusi dengan esp_timer_get_time()
 * 
 * Koneksi Hardware:
 *   - LED1: GPIO2 (built-in LED)
 *   - LED2: GPIO4 (LED eksternal)
 * 
 * Platform: ESP32 DevKit V1
 * Framework: ESP-IDF
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "config.h"

/* Tag untuk logging */
static const char *TAG = "CORE_AFFINITY";

/* ======================== STRUKTUR DATA ================================== */

/**
 * Struktur untuk menyimpan statistik performa setiap task
 * Menyimpan waktu eksekusi, core ID, dan hitungan eksekusi
 */
typedef struct {
    char name[32];                      /* Nama task */
    int pinned_core;                    /* Core yang ditugaskan (-1 = floating) */
    int actual_core;                    /* Core aktual saat berjalan */
    int64_t exec_times[MAX_SAMPLES];    /* Array waktu eksekusi (us) */
    int64_t min_time;                   /* Waktu eksekusi minimum */
    int64_t max_time;                   /* Waktu eksekusi maksimum */
    int64_t total_time;                 /* Total waktu eksekusi kumulatif */
    uint32_t exec_count;                /* Jumlah total eksekusi */
    uint32_t sample_index;              /* Indeks sampel saat ini */
    uint32_t core0_count;              /* Berapa kali jalan di Core 0 */
    uint32_t core1_count;              /* Berapa kali jalan di Core 1 */
} task_stats_t;

/* ======================== VARIABEL GLOBAL ================================ */

/* Statistik untuk 3 task utama */
static task_stats_t stats_core0;        /* Statistik task pinned ke Core 0 */
static task_stats_t stats_core1;        /* Statistik task pinned ke Core 1 */
static task_stats_t stats_float;        /* Statistik task floating */

/* Handle task untuk kontrol */
static TaskHandle_t task_core0_handle = NULL;
static TaskHandle_t task_core1_handle = NULL;
static TaskHandle_t task_float_handle = NULL;
static TaskHandle_t monitor_handle = NULL;

/* Flag kontrol */
static volatile bool system_running = true;
static volatile uint32_t work_iterations = WORK_ITERATIONS_MEDIUM;

/* ======================== FUNGSI UTILITAS ================================ */

/**
 * Inisialisasi GPIO untuk LED
 * Mengkonfigurasi LED1 dan LED2 sebagai output digital
 */
static void init_gpio(void)
{
    /* Konfigurasi LED1 (GPIO2) */
    gpio_config_t led1_conf = {
        .pin_bit_mask = (1ULL << LED1_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led1_conf);

    /* Konfigurasi LED2 (GPIO4) */
    gpio_config_t led2_conf = {
        .pin_bit_mask = (1ULL << LED2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led2_conf);

    /* Matikan kedua LED */
    gpio_set_level(LED1_PIN, 0);
    gpio_set_level(LED2_PIN, 0);
}

/**
 * Inisialisasi struktur statistik task
 * @param stats Pointer ke struktur statistik
 * @param name Nama task
 * @param pinned_core Core yang ditugaskan (-1 untuk floating)
 */
static void init_stats(task_stats_t *stats, const char *name, int pinned_core)
{
    strncpy(stats->name, name, sizeof(stats->name) - 1);
    stats->pinned_core = pinned_core;
    stats->actual_core = -1;
    stats->min_time = INT64_MAX;
    stats->max_time = 0;
    stats->total_time = 0;
    stats->exec_count = 0;
    stats->sample_index = 0;
    stats->core0_count = 0;
    stats->core1_count = 0;
    memset(stats->exec_times, 0, sizeof(stats->exec_times));
}

/**
 * Simulasi beban kerja CPU-intensive
 * Melakukan operasi matematika untuk membebani CPU
 * @param iterations Jumlah iterasi komputasi
 * @return Hasil komputasi (untuk mencegah optimisasi compiler)
 */
static volatile float cpu_work_result = 0;
static void do_cpu_work(uint32_t iterations)
{
    volatile float result = 1.0f;
    for (uint32_t i = 1; i <= iterations; i++) {
        /* Operasi matematika yang cukup berat */
        result += (float)i * 0.001f;
        result = result * 1.00001f;
        if (i % 1000 == 0) {
            result = result / 1.001f;  /* Mencegah overflow */
        }
    }
    cpu_work_result = result;  /* Mencegah compiler menghapus kode */
}

/**
 * Update statistik setelah satu eksekusi
 * @param stats Pointer ke struktur statistik
 * @param exec_time Waktu eksekusi dalam mikrodetik
 * @param core_id Core ID tempat task berjalan
 */
static void update_stats(task_stats_t *stats, int64_t exec_time, int core_id)
{
    stats->actual_core = core_id;
    stats->exec_count++;
    stats->total_time += exec_time;

    /* Update min/max */
    if (exec_time < stats->min_time) stats->min_time = exec_time;
    if (exec_time > stats->max_time) stats->max_time = exec_time;

    /* Simpan sampel waktu eksekusi */
    if (stats->sample_index < MAX_SAMPLES) {
        stats->exec_times[stats->sample_index] = exec_time;
        stats->sample_index++;
    }

    /* Hitung distribusi core */
    if (core_id == 0) {
        stats->core0_count++;
    } else {
        stats->core1_count++;
    }
}

/* ======================== TASK FUNCTIONS ================================= */

/**
 * Task yang di-pin ke Core 0 (Protocol CPU)
 * 
 * Task ini berjalan secara eksklusif di Core 0, yang juga digunakan
 * oleh WiFi dan Bluetooth protocol stack. Hal ini dapat menyebabkan
 * interferensi dengan protocol stack jika beban terlalu berat.
 * 
 * @param pvParameters Tidak digunakan
 */
static void task_pinned_core0(void *pvParameters)
{
    ESP_LOGI(TAG, "Task Core0 dimulai pada Core %d", xPortGetCoreID());

    /* Fase pemanasan - biarkan sistem stabil */
    for (int i = 0; i < WARMUP_CYCLES; i++) {
        do_cpu_work(WORK_ITERATIONS_LIGHT);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (system_running) {
        int core_id = xPortGetCoreID();

        /* Nyalakan LED1 saat bekerja di Core 0 */
        gpio_set_level(LED1_PIN, 1);

        /* Ukur waktu eksekusi beban kerja */
        int64_t start = esp_timer_get_time();
        do_cpu_work(work_iterations);
        int64_t end = esp_timer_get_time();
        int64_t elapsed = end - start;

        gpio_set_level(LED1_PIN, 0);

        /* Update statistik */
        update_stats(&stats_core0, elapsed, core_id);

        /* Cetak data untuk serial parser */
        printf("[DATA]CORE0_TASK,core=%d,time_us=%lld,count=%lu,iter=%lu\n",
               core_id, elapsed, stats_core0.exec_count, work_iterations);

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }

    ESP_LOGI(TAG, "Task Core0 selesai");
    vTaskDelete(NULL);
}

/**
 * Task yang di-pin ke Core 1 (Application CPU)
 * 
 * Task ini berjalan di Core 1 yang merupakan core default untuk
 * aplikasi. Core 1 biasanya lebih bebas dari interferensi karena
 * tidak menjalankan protocol stack.
 * 
 * @param pvParameters Tidak digunakan
 */
static void task_pinned_core1(void *pvParameters)
{
    ESP_LOGI(TAG, "Task Core1 dimulai pada Core %d", xPortGetCoreID());

    /* Fase pemanasan */
    for (int i = 0; i < WARMUP_CYCLES; i++) {
        do_cpu_work(WORK_ITERATIONS_LIGHT);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (system_running) {
        int core_id = xPortGetCoreID();

        /* Nyalakan LED2 saat bekerja di Core 1 */
        gpio_set_level(LED2_PIN, 1);

        /* Ukur waktu eksekusi beban kerja */
        int64_t start = esp_timer_get_time();
        do_cpu_work(work_iterations);
        int64_t end = esp_timer_get_time();
        int64_t elapsed = end - start;

        gpio_set_level(LED2_PIN, 0);

        /* Update statistik */
        update_stats(&stats_core1, elapsed, core_id);

        /* Cetak data untuk serial parser */
        printf("[DATA]CORE1_TASK,core=%d,time_us=%lld,count=%lu,iter=%lu\n",
               core_id, elapsed, stats_core1.exec_count, work_iterations);

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }

    ESP_LOGI(TAG, "Task Core1 selesai");
    vTaskDelete(NULL);
}

/**
 * Task floating (tskNO_AFFINITY) - bisa berjalan di core manapun
 * 
 * Task ini tidak di-pin ke core tertentu sehingga FreeRTOS scheduler
 * bebas menjadwalkannya di core manapun yang tersedia. Ini memungkinkan
 * load balancing otomatis tetapi bisa menyebabkan cache thrashing
 * jika sering berpindah core.
 * 
 * @param pvParameters Tidak digunakan
 */
static void task_floating(void *pvParameters)
{
    ESP_LOGI(TAG, "Task Float dimulai pada Core %d (floating)", xPortGetCoreID());

    /* Fase pemanasan */
    for (int i = 0; i < WARMUP_CYCLES; i++) {
        do_cpu_work(WORK_ITERATIONS_LIGHT);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (system_running) {
        int core_id = xPortGetCoreID();

        /* Toggle kedua LED saat floating task aktif */
        gpio_set_level(LED1_PIN, 1);
        gpio_set_level(LED2_PIN, 1);

        /* Ukur waktu eksekusi beban kerja */
        int64_t start = esp_timer_get_time();
        do_cpu_work(work_iterations);
        int64_t end = esp_timer_get_time();
        int64_t elapsed = end - start;

        gpio_set_level(LED1_PIN, 0);
        gpio_set_level(LED2_PIN, 0);

        /* Update statistik */
        update_stats(&stats_float, elapsed, core_id);

        /* Cetak data untuk serial parser */
        printf("[DATA]FLOAT_TASK,core=%d,time_us=%lld,count=%lu,c0=%lu,c1=%lu\n",
               core_id, elapsed, stats_float.exec_count,
               stats_float.core0_count, stats_float.core1_count);

        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }

    ESP_LOGI(TAG, "Task Float selesai");
    vTaskDelete(NULL);
}

/**
 * Cetak laporan statistik untuk satu task
 * @param stats Pointer ke struktur statistik
 */
static void print_task_stats(task_stats_t *stats)
{
    if (stats->exec_count == 0) return;

    int64_t avg_time = stats->total_time / stats->exec_count;
    float core0_pct = (float)stats->core0_count / stats->exec_count * 100.0f;
    float core1_pct = (float)stats->core1_count / stats->exec_count * 100.0f;

    printf("[DATA]STATS,%s,pinned=%d,avg_us=%lld,min_us=%lld,max_us=%lld,"
           "count=%lu,core0_pct=%.1f,core1_pct=%.1f\n",
           stats->name, stats->pinned_core, avg_time,
           stats->min_time, stats->max_time,
           stats->exec_count, core0_pct, core1_pct);
}

/**
 * Task monitor - mengumpulkan dan mencetak statistik periodik
 * 
 * Task ini berjalan dengan prioritas tinggi dan mencetak:
 *   - Statistik waktu eksekusi setiap task
 *   - Distribusi core untuk floating task
 *   - Informasi heap memory
 *   - Perbandingan performa antar core
 * 
 * @param pvParameters Tidak digunakan
 */
static void task_monitor(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor dimulai pada Core %d", xPortGetCoreID());
    uint32_t cycle = 0;

    /* Tunggu task lain stabil */
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (system_running) {
        cycle++;

        printf("\n========================================\n");
        printf("  CORE AFFINITY MONITOR - Siklus #%lu\n", cycle);
        printf("========================================\n");

        /* Cetak statistik setiap task */
        print_task_stats(&stats_core0);
        print_task_stats(&stats_core1);
        print_task_stats(&stats_float);

        /* Informasi sistem */
        printf("[DATA]SYSTEM,heap_free=%lu,heap_min=%lu,uptime_ms=%lld\n",
               (uint32_t)esp_get_free_heap_size(),
               (uint32_t)esp_get_minimum_free_heap_size(),
               esp_timer_get_time() / 1000);

        /* Perbandingan performa Core 0 vs Core 1 */
        if (stats_core0.exec_count > 0 && stats_core1.exec_count > 0) {
            int64_t avg0 = stats_core0.total_time / stats_core0.exec_count;
            int64_t avg1 = stats_core1.total_time / stats_core1.exec_count;
            float ratio = (avg0 > 0) ? (float)avg1 / avg0 * 100.0f : 0;

            printf("[DATA]COMPARE,avg_core0_us=%lld,avg_core1_us=%lld,ratio=%.1f%%\n",
                   avg0, avg1, ratio);

            /* Core 0 biasanya lebih lambat karena protocol stack */
            if (avg0 > avg1) {
                printf("[DATA]INSIGHT,Core0 %.1f%% lebih lambat (protocol stack overhead)\n",
                       (float)(avg0 - avg1) / avg1 * 100.0f);
            }
        }

        /* Info distribusi floating task */
        if (stats_float.exec_count > 0) {
            printf("[DATA]FLOAT_DIST,core0=%lu,core1=%lu,total=%lu\n",
                   stats_float.core0_count, stats_float.core1_count,
                   stats_float.exec_count);
        }

        /* Variasi beban kerja setiap 5 siklus */
        if (cycle % 5 == 0) {
            if (work_iterations == WORK_ITERATIONS_MEDIUM) {
                work_iterations = WORK_ITERATIONS_HEAVY;
                printf("[DATA]WORKLOAD,level=HEAVY,iterations=%lu\n", work_iterations);
            } else if (work_iterations == WORK_ITERATIONS_HEAVY) {
                work_iterations = WORK_ITERATIONS_LIGHT;
                printf("[DATA]WORKLOAD,level=LIGHT,iterations=%lu\n", work_iterations);
            } else {
                work_iterations = WORK_ITERATIONS_MEDIUM;
                printf("[DATA]WORKLOAD,level=MEDIUM,iterations=%lu\n", work_iterations);
            }
        }

        /* Stack watermark check */
        if (task_core0_handle) {
            printf("[DATA]STACK,task=Core0,watermark=%u\n",
                   (unsigned)uxTaskGetStackHighWaterMark(task_core0_handle));
        }
        if (task_core1_handle) {
            printf("[DATA]STACK,task=Core1,watermark=%u\n",
                   (unsigned)uxTaskGetStackHighWaterMark(task_core1_handle));
        }
        if (task_float_handle) {
            printf("[DATA]STACK,task=Float,watermark=%u\n",
                   (unsigned)uxTaskGetStackHighWaterMark(task_float_handle));
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}

/* ======================== ENTRY POINT ==================================== */

/**
 * Fungsi utama aplikasi ESP-IDF
 * 
 * Urutan inisialisasi:
 *   1. Konfigurasi GPIO untuk LED
 *   2. Inisialisasi struktur statistik
 *   3. Cetak informasi sistem (jumlah core, frekuensi, dll)
 *   4. Buat task pinned ke Core 0
 *   5. Buat task pinned ke Core 1
 *   6. Buat task floating (tskNO_AFFINITY)
 *   7. Buat task monitor
 */
void app_main(void)
{
    /* Cetak banner program */
    printf("\n");
    printf("============================================================\n");
    printf("  ESP32 FreeRTOS - Task Core Affinity Demo\n");
    printf("  Dual-Core Task Pinning & Performance Comparison\n");
    printf("============================================================\n");
    printf("  Platform: ESP32 (%d core)\n", portNUM_PROCESSORS);
    printf("  CPU Freq: %d MHz\n", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    printf("  Free Heap: %lu bytes\n", (uint32_t)esp_get_free_heap_size());
    printf("  FreeRTOS Tick Rate: %d Hz\n", configTICK_RATE_HZ);
    printf("============================================================\n\n");

    /* 1. Inisialisasi GPIO */
    ESP_LOGI(TAG, "Inisialisasi GPIO...");
    init_gpio();

    /* 2. Inisialisasi struktur statistik */
    init_stats(&stats_core0, "Core0_Task", 0);
    init_stats(&stats_core1, "Core1_Task", 1);
    init_stats(&stats_float, "Float_Task", -1);

    /* 3. Informasi tentang dual-core ESP32 */
    printf("[DATA]INFO,num_cores=%d,tick_rate=%d\n",
           portNUM_PROCESSORS, configTICK_RATE_HZ);

    /* Catatan tentang Core 0 */
    printf("\n--- INFORMASI CORE ESP32 ---\n");
    printf("Core 0: Protocol CPU - WiFi, BT, sistem task berjalan di sini\n");
    printf("Core 1: Application CPU - app_main() default berjalan di sini\n");
    printf("app_main() saat ini berjalan di Core %d\n", xPortGetCoreID());
    printf("----------------------------\n\n");

    /* 4. Buat task yang di-pin ke Core 0 */
    ESP_LOGI(TAG, "Membuat task pinned ke Core 0...");
    BaseType_t ret = xTaskCreatePinnedToCore(
        task_pinned_core0,          /* Fungsi task */
        "Core0Task",                /* Nama task (untuk debug) */
        TASK_CORE0_STACK,           /* Ukuran stack */
        NULL,                       /* Parameter (tidak ada) */
        TASK_CORE0_PRIORITY,        /* Prioritas */
        &task_core0_handle,         /* Handle output */
        0                           /* Core ID = 0 */
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Gagal membuat task Core 0!");
    }

    /* 5. Buat task yang di-pin ke Core 1 */
    ESP_LOGI(TAG, "Membuat task pinned ke Core 1...");
    ret = xTaskCreatePinnedToCore(
        task_pinned_core1,          /* Fungsi task */
        "Core1Task",                /* Nama task */
        TASK_CORE1_STACK,           /* Ukuran stack */
        NULL,                       /* Parameter */
        TASK_CORE1_PRIORITY,        /* Prioritas */
        &task_core1_handle,         /* Handle output */
        1                           /* Core ID = 1 */
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Gagal membuat task Core 1!");
    }

    /* 6. Buat task floating (bisa berjalan di core manapun) */
    ESP_LOGI(TAG, "Membuat task floating (tskNO_AFFINITY)...");
    ret = xTaskCreatePinnedToCore(
        task_floating,              /* Fungsi task */
        "FloatTask",                /* Nama task */
        TASK_FLOAT_STACK,           /* Ukuran stack */
        NULL,                       /* Parameter */
        TASK_FLOAT_PRIORITY,        /* Prioritas */
        &task_float_handle,         /* Handle output */
        tskNO_AFFINITY              /* Bisa di core manapun */
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Gagal membuat task floating!");
    }

    /* 7. Buat task monitor (pinned ke Core 1 agar tidak ganggu Core 0) */
    ESP_LOGI(TAG, "Membuat task monitor...");
    ret = xTaskCreatePinnedToCore(
        task_monitor,               /* Fungsi task */
        "MonitorTask",              /* Nama task */
        MONITOR_TASK_STACK,         /* Ukuran stack */
        NULL,                       /* Parameter */
        MONITOR_TASK_PRIORITY,      /* Prioritas (tertinggi) */
        &monitor_handle,            /* Handle output */
        1                           /* Pin ke Core 1 */
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Gagal membuat task monitor!");
    }

    ESP_LOGI(TAG, "Semua task berhasil dibuat. Sistem berjalan...");
    printf("[DATA]STATUS,all_tasks_created=true\n");

    /* app_main() selesai - FreeRTOS scheduler menangani task-task */
    /* app_main() berjalan sebagai task sendiri dan bisa dihapus */
}
