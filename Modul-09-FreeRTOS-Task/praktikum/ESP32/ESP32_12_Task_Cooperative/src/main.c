/**
 * ============================================================================
 * PROGRAM 12: ESP32 Task Cooperative Scheduling
 * ============================================================================
 * Demonstrasi perbedaan cooperative vs preemptive scheduling
 * 
 * FreeRTOS pada ESP32 default menggunakan preemptive scheduling dengan
 * time slicing (round-robin) untuk task dengan prioritas sama.
 * 
 * Fase demonstrasi:
 *   1. PREEMPTIVE: Round-robin time slicing otomatis oleh scheduler
 *   2. COOPERATIVE: Task memanggil taskYIELD() secara eksplisit
 *   3. CPU HOGGING: Satu task monopoli CPU tanpa yield
 * 
 * Pengukuran:
 *   - Waktu eksekusi per task slice
 *   - Distribusi CPU time antar task
 *   - Deteksi starvation pada mode hogging
 * 
 * Platform: ESP32 DevKit V1
 * Framework: ESP-IDF
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "config.h"

/* Tag untuk logging */
static const char *TAG = "COOPERATIVE";

/* ======================== ENUMERASI FASE ================================= */

typedef enum {
    PHASE_PREEMPTIVE = 0,   /* Round-robin preemptive (default) */
    PHASE_COOPERATIVE = 1,  /* Cooperative dengan taskYIELD() */
    PHASE_HOGGING = 2       /* CPU hogging (satu task monopoli) */
} sched_phase_t;

/* ======================== STRUKTUR DATA ================================== */

/**
 * Statistik per worker task
 */
typedef struct {
    char name[16];                  /* Nama task */
    int task_id;                    /* ID task (0-2) */
    volatile uint32_t exec_count;   /* Jumlah eksekusi */
    volatile uint32_t work_units;   /* Unit kerja selesai */
    int64_t total_exec_time_us;     /* Total waktu eksekusi (us) */
    int64_t last_slice_time_us;     /* Waktu slice terakhir (us) */
    int64_t max_slice_time_us;      /* Waktu slice terpanjang */
    int64_t min_slice_time_us;      /* Waktu slice terpendek */
    int64_t last_run_time;          /* Timestamp eksekusi terakhir */
    int64_t max_gap_us;             /* Gap terlama antar eksekusi */
    volatile bool is_starved;       /* Apakah task mengalami starvation */
} worker_stats_t;

/* ======================== VARIABEL GLOBAL ================================ */

/* Fase scheduling saat ini */
static volatile sched_phase_t current_phase = PHASE_PREEMPTIVE;
static volatile int hogging_task_id = 0;  /* Task ID yang hog CPU pada fase 3 */

/* Statistik per task */
static worker_stats_t worker_stats[NUM_WORKER_TASKS];

/* Handle task */
static TaskHandle_t worker_handles[NUM_WORKER_TASKS] = {NULL};
static TaskHandle_t monitor_handle = NULL;
static TaskHandle_t controller_handle = NULL;

/* Kontrol global */
static volatile bool system_running = true;
static volatile uint32_t phase_cycle = 0;

/* ======================== FUNGSI UTILITAS ================================ */

/**
 * Inisialisasi GPIO
 */
static void init_gpio(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);
    gpio_set_level(LED1_PIN, 0);
    gpio_set_level(LED2_PIN, 0);
}

/**
 * Inisialisasi statistik worker
 */
static void init_worker_stats(void)
{
    for (int i = 0; i < NUM_WORKER_TASKS; i++) {
        snprintf(worker_stats[i].name, sizeof(worker_stats[i].name), "Worker%d", i);
        worker_stats[i].task_id = i;
        worker_stats[i].exec_count = 0;
        worker_stats[i].work_units = 0;
        worker_stats[i].total_exec_time_us = 0;
        worker_stats[i].last_slice_time_us = 0;
        worker_stats[i].max_slice_time_us = 0;
        worker_stats[i].min_slice_time_us = INT64_MAX;
        worker_stats[i].last_run_time = 0;
        worker_stats[i].max_gap_us = 0;
        worker_stats[i].is_starved = false;
    }
}

/**
 * Simulasi unit kerja
 * @param iterations Jumlah iterasi komputasi
 * @return Hasil komputasi (mencegah optimisasi)
 */
static volatile float work_sink = 0;
static void do_work_chunk(uint32_t iterations)
{
    volatile float result = 1.0f;
    for (uint32_t i = 0; i < iterations; i++) {
        result += i * 0.001f;
        result *= 1.00001f;
    }
    work_sink = result;
}

/**
 * Nama fase scheduling
 */
static const char* phase_name(sched_phase_t phase)
{
    switch (phase) {
        case PHASE_PREEMPTIVE:  return "PREEMPTIVE";
        case PHASE_COOPERATIVE: return "COOPERATIVE";
        case PHASE_HOGGING:     return "HOGGING";
        default:                return "UNKNOWN";
    }
}

/* ======================== TASK FUNCTIONS ================================= */

/**
 * Worker Task - perilaku berubah berdasarkan fase saat ini
 * 
 * FASE PREEMPTIVE:
 *   Task melakukan pekerjaan dan bergantung pada scheduler untuk
 *   time slicing. FreeRTOS secara otomatis memberikan time slice
 *   yang sama ke semua task dengan prioritas yang sama.
 * 
 * FASE COOPERATIVE:
 *   Task secara eksplisit memanggil taskYIELD() untuk menyerahkan
 *   CPU ke task lain. Ini memungkinkan kontrol lebih halus atas
 *   penjadwalan dibanding time slicing otomatis.
 * 
 * FASE HOGGING:
 *   Satu task (hogging_task_id) TIDAK yield dan melakukan busy loop
 *   terus-menerus. Task lain kelaparan (starved) karena tidak
 *   mendapat giliran. Ini menunjukkan bahaya task yang monopoli CPU.
 *   
 *   CATATAN: Pada ESP32 dengan preemptive scheduling, task dengan
 *   prioritas sama tetap mendapat time slice meskipun tidak yield,
 *   berkat tick interrupt. Efek hogging lebih terlihat pada
 *   distribusi waktu eksekusi yang tidak merata.
 * 
 * @param pvParameters Task ID (0, 1, atau 2) dicast dari int
 */
static void worker_task(void *pvParameters)
{
    int task_id = (int)(intptr_t)pvParameters;
    worker_stats_t *stats = &worker_stats[task_id];

    ESP_LOGI(TAG, "Worker%d dimulai pada Core %d", task_id, xPortGetCoreID());

    while (system_running) {
        int64_t slice_start = esp_timer_get_time();

        /* Hitung gap dari eksekusi terakhir */
        if (stats->last_run_time > 0) {
            int64_t gap = slice_start - stats->last_run_time;
            if (gap > stats->max_gap_us) {
                stats->max_gap_us = gap;
            }
            /* Deteksi starvation: gap > 5x normal */
            if (gap > 5 * SLICE_MEASURE_WINDOW_US) {
                stats->is_starved = true;
            }
        }

        switch (current_phase) {
            case PHASE_PREEMPTIVE:
                /**
                 * MODE PREEMPTIVE (Round-Robin)
                 * 
                 * Task melakukan pekerjaan dalam loop. Scheduler otomatis
                 * melakukan context switch setelah time slice habis
                 * (setiap tick interrupt = 1ms default).
                 * 
                 * Semua task dengan prioritas sama mendapat waktu yang
                 * kira-kira sama (fair scheduling).
                 */
                for (int i = 0; i < 10; i++) {
                    do_work_chunk(WORK_CHUNK_ITERATIONS);
                    stats->work_units++;
                }
                /* Tidak ada yield eksplisit - biarkan scheduler memutuskan */
                vTaskDelay(pdMS_TO_TICKS(10));  /* Delay kecil agar task lain bisa jalan */
                break;

            case PHASE_COOPERATIVE:
                /**
                 * MODE COOPERATIVE (taskYIELD)
                 * 
                 * Task secara sukarela menyerahkan CPU setelah selesai
                 * satu unit kerja. Ini lebih efisien karena context switch
                 * terjadi di titik yang logis (setelah pekerjaan selesai).
                 * 
                 * taskYIELD() memindahkan task ke akhir ready list untuk
                 * prioritas yang sama, memberikan giliran ke task lain.
                 */
                do_work_chunk(WORK_CHUNK_ITERATIONS);
                stats->work_units++;
                taskYIELD();  /* Serahkan CPU secara eksplisit */
                break;

            case PHASE_HOGGING:
                /**
                 * MODE HOGGING (CPU Monopoly)
                 * 
                 * Satu task (hogging_task_id) melakukan busy-wait tanpa
                 * yield. Task ini "menguasai" CPU dan menyebabkan task
                 * lain kelaparan (starvation).
                 * 
                 * Pada ESP32 dengan preemptive scheduler, task lain masih
                 * bisa jalan saat tick interrupt, tapi mendapat waktu
                 * yang jauh lebih sedikit.
                 */
                if (task_id == hogging_task_id) {
                    /* Task ini hog CPU - busy loop tanpa yield */
                    for (int i = 0; i < 50; i++) {
                        do_work_chunk(WORK_CHUNK_ITERATIONS * 2);
                        stats->work_units++;
                    }
                    /* Sengaja TIDAK yield dan TIDAK delay */
                    /* LED berkedip cepat saat hogging */
                    gpio_set_level(LED1_PIN, stats->exec_count % 2);
                } else {
                    /* Task lain - coba bekerja tapi kelaparan */
                    do_work_chunk(WORK_CHUNK_ITERATIONS);
                    stats->work_units++;
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                break;
        }

        /* Update statistik */
        int64_t slice_end = esp_timer_get_time();
        int64_t slice_time = slice_end - slice_start;

        stats->exec_count++;
        stats->total_exec_time_us += slice_time;
        stats->last_slice_time_us = slice_time;
        stats->last_run_time = slice_end;

        if (slice_time > stats->max_slice_time_us) {
            stats->max_slice_time_us = slice_time;
        }
        if (slice_time < stats->min_slice_time_us) {
            stats->min_slice_time_us = slice_time;
        }

        /* Cetak data setiap 50 eksekusi */
        if (stats->exec_count % 50 == 0) {
            printf("[DATA]WORKER,id=%d,phase=%s,execs=%lu,work=%lu,"
                   "slice_us=%lld,avg_us=%lld,core=%d\n",
                   task_id, phase_name(current_phase),
                   stats->exec_count, stats->work_units,
                   slice_time,
                   stats->total_exec_time_us / stats->exec_count,
                   xPortGetCoreID());
        }
    }

    vTaskDelete(NULL);
}

/**
 * Task Controller - mengatur pergantian fase scheduling
 * 
 * Berputar melalui 3 fase setiap PHASE_DURATION_MS:
 *   0 → PREEMPTIVE → 1 → COOPERATIVE → 2 → HOGGING → 0 → ...
 */
static void controller_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Controller dimulai pada Core %d", xPortGetCoreID());

    /* Mulai dengan fase preemptive */
    current_phase = PHASE_PREEMPTIVE;
    phase_cycle = 0;

    while (system_running) {
        phase_cycle++;

        /* Cetak info pergantian fase */
        printf("\n");
        printf("╔══════════════════════════════════════════════════════════╗\n");
        printf("║  FASE %d: %-48s ║\n", current_phase, phase_name(current_phase));
        printf("║  Siklus: %-47lu ║\n", phase_cycle);
        printf("╚══════════════════════════════════════════════════════════╝\n");

        printf("[DATA]PHASE,num=%d,name=%s,cycle=%lu\n",
               current_phase, phase_name(current_phase), phase_cycle);

        /* Deskripsi fase */
        switch (current_phase) {
            case PHASE_PREEMPTIVE:
                printf("[DATA]DESC,Scheduler otomatis membagi waktu (round-robin)\n");
                /* LED pattern: kedip lambat */
                gpio_set_level(LED2_PIN, 1);
                break;

            case PHASE_COOPERATIVE:
                printf("[DATA]DESC,Task memanggil taskYIELD() secara eksplisit\n");
                /* LED pattern: kedip cepat */
                gpio_set_level(LED2_PIN, 0);
                break;

            case PHASE_HOGGING:
                hogging_task_id = phase_cycle % NUM_WORKER_TASKS;
                printf("[DATA]DESC,Worker%d MONOPOLI CPU - task lain kelaparan\n",
                       hogging_task_id);
                printf("[DATA]HOG_TASK,id=%d\n", hogging_task_id);
                break;
        }

        /* Reset statistik starvation untuk fase baru */
        for (int i = 0; i < NUM_WORKER_TASKS; i++) {
            worker_stats[i].is_starved = false;
        }

        /* Tunggu durasi fase */
        vTaskDelay(pdMS_TO_TICKS(PHASE_DURATION_MS));

        /* Reset statistik per-fase untuk perbandingan yang adil */
        uint32_t phase_exec[NUM_WORKER_TASKS];
        uint32_t phase_work[NUM_WORKER_TASKS];
        for (int i = 0; i < NUM_WORKER_TASKS; i++) {
            phase_exec[i] = worker_stats[i].exec_count;
            phase_work[i] = worker_stats[i].work_units;
        }

        /* Cetak ringkasan akhir fase */
        printf("\n--- Ringkasan Fase %s ---\n", phase_name(current_phase));
        uint32_t total_work = 0;
        for (int i = 0; i < NUM_WORKER_TASKS; i++) {
            total_work += phase_work[i];
        }
        for (int i = 0; i < NUM_WORKER_TASKS; i++) {
            float pct = total_work > 0 ?
                (float)phase_work[i] / total_work * 100.0f : 0;
            printf("[DATA]PHASE_SUMMARY,phase=%s,worker=%d,execs=%lu,"
                   "work=%lu,pct=%.1f,starved=%d\n",
                   phase_name(current_phase), i,
                   phase_exec[i], phase_work[i], pct,
                   worker_stats[i].is_starved);
        }

        /* Ganti ke fase berikutnya */
        current_phase = (current_phase + 1) % NUM_PHASES;
    }

    vTaskDelete(NULL);
}

/**
 * Task Monitor - cetak statistik periodik
 */
static void monitor_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor dimulai pada Core %d", xPortGetCoreID());
    uint32_t cycle = 0;

    vTaskDelay(pdMS_TO_TICKS(3000));

    while (system_running) {
        cycle++;

        printf("\n========================================\n");
        printf("  SCHEDULING MONITOR - Siklus #%lu\n", cycle);
        printf("  Fase: %s (siklus %lu)\n", phase_name(current_phase), phase_cycle);
        printf("========================================\n");

        /* Statistik setiap worker */
        uint32_t total_execs = 0;
        uint32_t total_works = 0;
        for (int i = 0; i < NUM_WORKER_TASKS; i++) {
            total_execs += worker_stats[i].exec_count;
            total_works += worker_stats[i].work_units;
        }

        for (int i = 0; i < NUM_WORKER_TASKS; i++) {
            worker_stats_t *s = &worker_stats[i];
            float exec_pct = total_execs > 0 ?
                (float)s->exec_count / total_execs * 100.0f : 0;
            float work_pct = total_works > 0 ?
                (float)s->work_units / total_works * 100.0f : 0;

            printf("[DATA]STATS,worker=%d,execs=%lu,work=%lu,"
                   "exec_pct=%.1f,work_pct=%.1f,"
                   "slice_us=%lld,max_slice=%lld,max_gap=%lld,"
                   "starved=%d\n",
                   i, s->exec_count, s->work_units,
                   exec_pct, work_pct,
                   s->last_slice_time_us,
                   s->max_slice_time_us,
                   s->max_gap_us,
                   s->is_starved);
        }

        /* Fairness index (Jain's fairness) */
        if (total_works > 0) {
            float sum_x = 0, sum_x2 = 0;
            for (int i = 0; i < NUM_WORKER_TASKS; i++) {
                float x = (float)worker_stats[i].work_units;
                sum_x += x;
                sum_x2 += x * x;
            }
            float fairness = (sum_x * sum_x) / (NUM_WORKER_TASKS * sum_x2);
            printf("[DATA]FAIRNESS,index=%.4f,phase=%s\n",
                   fairness, phase_name(current_phase));

            /* Fairness = 1.0 = sempurna adil, <0.5 = sangat tidak adil */
            if (fairness > 0.95f) {
                printf("[DATA]FAIR_STATUS,ADIL - semua task mendapat waktu yang sama\n");
            } else if (fairness > 0.7f) {
                printf("[DATA]FAIR_STATUS,CUKUP ADIL - ada sedikit ketimpangan\n");
            } else {
                printf("[DATA]FAIR_STATUS,TIDAK ADIL - ada task yang kelaparan!\n");
            }
        }

        /* LED pattern sesuai fase */
        switch (current_phase) {
            case PHASE_PREEMPTIVE:
                gpio_set_level(LED1_PIN, cycle % 2);
                break;
            case PHASE_COOPERATIVE:
                gpio_set_level(LED1_PIN, (cycle / 2) % 2);
                break;
            case PHASE_HOGGING:
                gpio_set_level(LED1_PIN, 1);  /* Nyala terus */
                break;
        }

        /* Heap info */
        printf("[DATA]HEAP,free=%lu,min=%lu\n",
               (uint32_t)esp_get_free_heap_size(),
               (uint32_t)esp_get_minimum_free_heap_size());

        printf("[DATA]UPTIME,ms=%lld\n", esp_timer_get_time() / 1000);

        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}

/* ======================== ENTRY POINT ==================================== */

void app_main(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("  ESP32 FreeRTOS - Cooperative vs Preemptive Scheduling\n");
    printf("  Round-Robin, taskYIELD(), dan CPU Hogging\n");
    printf("============================================================\n");
    printf("  Fase 0: PREEMPTIVE - Round-robin time slicing otomatis\n");
    printf("  Fase 1: COOPERATIVE - taskYIELD() eksplisit\n");
    printf("  Fase 2: HOGGING - Satu task monopoli CPU\n");
    printf("  Durasi per fase: %d ms\n", PHASE_DURATION_MS);
    printf("  Jumlah worker: %d (semua prioritas %d)\n",
           NUM_WORKER_TASKS, WORKER_TASK_PRIORITY);
    printf("  Free Heap: %lu bytes\n", (uint32_t)esp_get_free_heap_size());
    printf("============================================================\n\n");

    /* Inisialisasi */
    init_gpio();
    init_worker_stats();

    /* Buat worker tasks - semua di Core 1 agar bersaing */
    for (int i = 0; i < NUM_WORKER_TASKS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Worker%d", i);

        BaseType_t ret = xTaskCreatePinnedToCore(
            worker_task,
            name,
            WORKER_TASK_STACK,
            (void *)(intptr_t)i,
            WORKER_TASK_PRIORITY,    /* Semua prioritas SAMA */
            &worker_handles[i],
            1                        /* Semua di Core 1 */
        );

        if (ret == pdPASS) {
            ESP_LOGI(TAG, "Worker%d berhasil dibuat (prioritas %d, Core 1)",
                     i, WORKER_TASK_PRIORITY);
        }
    }

    /* Buat controller task (prioritas lebih tinggi, Core 0) */
    xTaskCreatePinnedToCore(
        controller_task, "Controller", CONTROLLER_TASK_STACK,
        NULL, CONTROLLER_PRIORITY, &controller_handle, 0);

    /* Buat monitor task (Core 0) */
    xTaskCreatePinnedToCore(
        monitor_task_fn, "Monitor", MONITOR_TASK_STACK,
        NULL, MONITOR_TASK_PRIORITY, &monitor_handle, 0);

    ESP_LOGI(TAG, "Semua task dimulai. Demo scheduling berjalan...");
    printf("[DATA]STATUS,system=started,workers=%d,phases=%d\n",
           NUM_WORKER_TASKS, NUM_PHASES);
}
