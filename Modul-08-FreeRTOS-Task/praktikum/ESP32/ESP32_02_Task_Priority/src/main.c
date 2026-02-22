/* ============================================================
 * ESP32_02_Task_Priority - Prioritas Task FreeRTOS
 * ============================================================
 * Program ini mendemonstrasikan mekanisme prioritas pada
 * FreeRTOS scheduler. Task dengan prioritas lebih tinggi akan
 * dieksekusi terlebih dahulu (preemptive scheduling).
 *
 * Konsep yang dipelajari:
 * 1. Preemptive scheduling - task prioritas tinggi mendahului
 * 2. Priority inversion - masalah klasik RTOS
 * 3. vTaskPrioritySet() - mengubah prioritas runtime
 * 4. uxTaskPriorityGet() - membaca prioritas task
 * 5. CPU-bound vs I/O-bound task behavior
 * 6. Time-slicing antara task prioritas sama
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1
 * - LED1 GPIO2, LED2 GPIO4, LED3 GPIO5
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

static const char *TAG = "TASK_PRIO";

/* Task handles */
static TaskHandle_t xLowTaskHandle = NULL;
static TaskHandle_t xMedTaskHandle = NULL;
static TaskHandle_t xHighTaskHandle = NULL;
static TaskHandle_t xMonitorHandle = NULL;

/* Timestamps untuk tracking urutan eksekusi */
static volatile int64_t low_start_us = 0;
static volatile int64_t med_start_us = 0;
static volatile int64_t high_start_us = 0;
static volatile int64_t low_end_us = 0;
static volatile int64_t med_end_us = 0;
static volatile int64_t high_end_us = 0;

/* Counter kerja setiap task */
static volatile uint32_t low_work_count = 0;
static volatile uint32_t med_work_count = 0;
static volatile uint32_t high_work_count = 0;

/* Round tracker */
static volatile uint32_t low_round = 0;
static volatile uint32_t med_round = 0;
static volatile uint32_t high_round = 0;

/* Flag untuk sinkronisasi demo */
static volatile bool demo_running = false;
static volatile uint32_t demo_phase = 0;

/* Timestamp awal */
static int64_t start_time_us = 0;

static uint32_t get_elapsed_ms(void)
{
    return (uint32_t)((esp_timer_get_time() - start_time_us) / 1000);
}

/* Inisialisasi GPIO */
static void init_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO) | (1ULL << LED3_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED1_GPIO, 0);
    gpio_set_level(LED2_GPIO, 0);
    gpio_set_level(LED3_GPIO, 0);
}

/**
 * @brief Simulasi kerja CPU-intensive
 * @param iterations Jumlah iterasi loop
 * @return Hasil komputasi (mencegah optimisasi compiler)
 *
 * Fungsi ini melakukan komputasi intensif untuk menunjukkan
 * bagaimana task CPU-bound berperilaku dengan scheduler.
 * Variabel volatile mencegah compiler meng-optimize-out loop.
 */
static volatile uint32_t cpu_intensive_work(uint32_t iterations)
{
    volatile uint32_t result = 0;
    for (uint32_t i = 0; i < iterations; i++) {
        result += i;
        result ^= (result << 3);
        result += (result >> 5);
    }
    return result;
}

/* ============================================================
 * TASK: Low Priority (Priority = 1)
 * ============================================================
 * Task ini memiliki prioritas terendah. Pada preemptive scheduler,
 * task ini HANYA berjalan ketika tidak ada task prioritas lebih
 * tinggi yang READY.
 *
 * Observasi yang diharapkan:
 * - Task ini akan di-preempt saat task prioritas lebih tinggi READY
 * - Execution time akan lebih lama karena preemption
 * - Pada dual-core ESP32, bisa berjalan parallel jika ada core free
 * ============================================================ */
static void low_priority_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[LOW] Task dimulai - Prioritas=%d, Core=%d",
             (int)uxTaskPriorityGet(NULL), (int)xPortGetCoreID());

    while (1) {
        low_round++;
        gpio_set_level(LED1_GPIO, 1);  // LED ON saat bekerja

        low_start_us = esp_timer_get_time();
        ESP_LOGI(TAG, "[LOW] Round %lu mulai", (unsigned long)low_round);

        printf("[DATA] PRIO_START,LOW,%lu,%lu,1\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)low_round);

        /* Kerja CPU-intensive */
        cpu_intensive_work(CPU_WORK_ITERATIONS);
        low_work_count++;

        low_end_us = esp_timer_get_time();
        int64_t duration_us = low_end_us - low_start_us;

        gpio_set_level(LED1_GPIO, 0);  // LED OFF setelah kerja

        printf("[DATA] PRIO_END,LOW,%lu,%lu,%lld,%d\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)low_round,
               duration_us,
               (int)xPortGetCoreID());

        ESP_LOGI(TAG, "[LOW] Round %lu selesai dalam %lld us",
                 (unsigned long)low_round, duration_us);

        /* Delay pendek sebelum ronde berikutnya */
        vTaskDelay(pdMS_TO_TICKS(ROUND_DELAY_MS * 3));
    }
}

/* ============================================================
 * TASK: Medium Priority (Priority = 3)
 * ============================================================
 * Task ini memiliki prioritas menengah.
 * - Dapat di-preempt oleh task HIGH
 * - Dapat men-preempt task LOW
 * - Menunjukkan "middle ground" scheduling behavior
 * ============================================================ */
static void med_priority_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[MED] Task dimulai - Prioritas=%d, Core=%d",
             (int)uxTaskPriorityGet(NULL), (int)xPortGetCoreID());

    while (1) {
        med_round++;
        gpio_set_level(LED2_GPIO, 1);

        med_start_us = esp_timer_get_time();

        printf("[DATA] PRIO_START,MED,%lu,%lu,3\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)med_round);

        cpu_intensive_work(CPU_WORK_ITERATIONS);
        med_work_count++;

        med_end_us = esp_timer_get_time();
        int64_t duration_us = med_end_us - med_start_us;

        gpio_set_level(LED2_GPIO, 0);

        printf("[DATA] PRIO_END,MED,%lu,%lu,%lld,%d\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)med_round,
               duration_us,
               (int)xPortGetCoreID());

        ESP_LOGI(TAG, "[MED] Round %lu selesai dalam %lld us",
                 (unsigned long)med_round, duration_us);

        vTaskDelay(pdMS_TO_TICKS(ROUND_DELAY_MS * 2));
    }
}

/* ============================================================
 * TASK: High Priority (Priority = 5)
 * ============================================================
 * Task ini memiliki prioritas tertinggi.
 * - Akan selalu di-eksekusi pertama ketika READY
 * - Men-preempt semua task lain
 * - Execution time akan paling konsisten
 *
 * PERINGATAN: Task CPU-bound dengan prioritas tinggi yang tidak
 * pernah block akan STARVE semua task prioritas lebih rendah!
 * Selalu pastikan task high-priority memiliki blocking point.
 * ============================================================ */
static void high_priority_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[HIGH] Task dimulai - Prioritas=%d, Core=%d",
             (int)uxTaskPriorityGet(NULL), (int)xPortGetCoreID());

    while (1) {
        high_round++;
        gpio_set_level(LED3_GPIO, 1);

        high_start_us = esp_timer_get_time();

        printf("[DATA] PRIO_START,HIGH,%lu,%lu,5\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)high_round);

        cpu_intensive_work(CPU_WORK_ITERATIONS);
        high_work_count++;

        high_end_us = esp_timer_get_time();
        int64_t duration_us = high_end_us - high_start_us;

        gpio_set_level(LED3_GPIO, 0);

        printf("[DATA] PRIO_END,HIGH,%lu,%lu,%lld,%d\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)high_round,
               duration_us,
               (int)xPortGetCoreID());

        ESP_LOGI(TAG, "[HIGH] Round %lu selesai dalam %lld us",
                 (unsigned long)high_round, duration_us);

        vTaskDelay(pdMS_TO_TICKS(ROUND_DELAY_MS));
    }
}

/**
 * @brief Monitor task - melaporkan status dan melakukan demo perubahan prioritas
 *
 * Task ini juga mendemonstrasikan priority inversion scenario:
 * 1. Fase 1: Normal operation - observe execution order
 * 2. Fase 2: Swap priorities LOW <-> HIGH
 * 3. Fase 3: Restore original priorities
 * 4. Fase 4: Set all equal priority (time-slicing)
 */
static void monitor_task(void *pvParameters)
{
    uint32_t report_count = 0;

    vTaskDelay(pdMS_TO_TICKS(1000));  // Tunggu task lain stabil

    while (1) {
        report_count++;

        printf("[DATA] MONITOR,%lu,%lu,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)report_count,
               (unsigned long)low_work_count,
               (unsigned long)med_work_count,
               (unsigned long)high_work_count);

        /* Print prioritas saat ini */
        printf("[DATA] PRIORITIES,%lu,%d,%d,%d\n",
               (unsigned long)get_elapsed_ms(),
               (int)uxTaskPriorityGet(xLowTaskHandle),
               (int)uxTaskPriorityGet(xMedTaskHandle),
               (int)uxTaskPriorityGet(xHighTaskHandle));

        ESP_LOGI(TAG, "=== Report #%lu ===", (unsigned long)report_count);
        ESP_LOGI(TAG, "Work counts: LOW=%lu, MED=%lu, HIGH=%lu",
                 (unsigned long)low_work_count,
                 (unsigned long)med_work_count,
                 (unsigned long)high_work_count);
        ESP_LOGI(TAG, "Priorities: LOW=%d, MED=%d, HIGH=%d",
                 (int)uxTaskPriorityGet(xLowTaskHandle),
                 (int)uxTaskPriorityGet(xMedTaskHandle),
                 (int)uxTaskPriorityGet(xHighTaskHandle));

        /*
         * Demo perubahan prioritas setiap 10 report
         *
         * vTaskPrioritySet() mengubah prioritas task secara runtime.
         * Jika prioritas baru lebih tinggi dari task yang sedang running,
         * context switch SEGERA terjadi (preemption).
         */
        if (report_count == 10) {
            ESP_LOGW(TAG, ">>> DEMO: Menukar prioritas LOW <-> HIGH <<<");
            printf("[DATA] PHASE,SWAP_PRIO,%lu\n", (unsigned long)get_elapsed_ms());

            /*
             * Priority Inversion Demo:
             * Menukar prioritas LOW dan HIGH menunjukkan bagaimana
             * behavior berubah saat prioritas di-swap runtime.
             *
             * vTaskPrioritySet(TaskHandle, NewPriority)
             * - Jika handle NULL, mengubah prioritas calling task
             * - Perubahan langsung efektif pada next scheduling point
             */
            vTaskPrioritySet(xLowTaskHandle, HIGH_PRIORITY);
            vTaskPrioritySet(xHighTaskHandle, LOW_PRIORITY);

            ESP_LOGW(TAG, "Prioritas di-swap! LOW->%d, HIGH->%d",
                     HIGH_PRIORITY, LOW_PRIORITY);
        }

        if (report_count == 20) {
            ESP_LOGW(TAG, ">>> DEMO: Mengembalikan prioritas original <<<");
            printf("[DATA] PHASE,RESTORE_PRIO,%lu\n", (unsigned long)get_elapsed_ms());

            vTaskPrioritySet(xLowTaskHandle, LOW_PRIORITY);
            vTaskPrioritySet(xHighTaskHandle, HIGH_PRIORITY);
        }

        if (report_count == 30) {
            ESP_LOGW(TAG, ">>> DEMO: Semua prioritas sama (time-slicing) <<<");
            printf("[DATA] PHASE,EQUAL_PRIO,%lu\n", (unsigned long)get_elapsed_ms());

            /*
             * Ketika semua task memiliki prioritas sama, FreeRTOS
             * menggunakan round-robin time-slicing. Setiap task
             * mendapat slot waktu yang sama (1 tick period).
             *
             * configUSE_TIME_SLICING harus 1 (default di ESP-IDF).
             */
            vTaskPrioritySet(xLowTaskHandle, MED_PRIORITY);
            vTaskPrioritySet(xMedTaskHandle, MED_PRIORITY);
            vTaskPrioritySet(xHighTaskHandle, MED_PRIORITY);
        }

        if (report_count == 40) {
            ESP_LOGW(TAG, ">>> DEMO: Kembali ke prioritas awal <<<");
            printf("[DATA] PHASE,FINAL_RESTORE,%lu\n", (unsigned long)get_elapsed_ms());

            vTaskPrioritySet(xLowTaskHandle, LOW_PRIORITY);
            vTaskPrioritySet(xMedTaskHandle, MED_PRIORITY);
            vTaskPrioritySet(xHighTaskHandle, HIGH_PRIORITY);
            report_count = 0;  // Reset cycle
        }

        /* Free heap monitoring */
        printf("[DATA] HEAP,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)esp_get_free_heap_size());

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* ============================================================
 * APP_MAIN
 * ============================================================ */
void app_main(void)
{
    start_time_us = esp_timer_get_time();

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 FreeRTOS Task Priority Demo");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Konfigurasi prioritas:");
    ESP_LOGI(TAG, "  LOW  = %d", LOW_PRIORITY);
    ESP_LOGI(TAG, "  MED  = %d", MED_PRIORITY);
    ESP_LOGI(TAG, "  HIGH = %d", HIGH_PRIORITY);
    ESP_LOGI(TAG, "CPU Work Iterations: %d", CPU_WORK_ITERATIONS);

    printf("[DATA] INIT,%lu,%d,%d,%d,%d\n",
           (unsigned long)get_elapsed_ms(),
           LOW_PRIORITY, MED_PRIORITY, HIGH_PRIORITY,
           CPU_WORK_ITERATIONS);

    init_gpio();

    /*
     * Membuat task dari prioritas rendah ke tinggi.
     * Task prioritas lebih tinggi bisa langsung preempt app_main()
     * (yang berjalan di prioritas 1) saat dibuat.
     *
     * Pada ESP32 dual-core, task bisa langsung mulai di Core 1
     * sementara app_main() masih berjalan di Core 0.
     */
    ESP_LOGI(TAG, "Membuat LOW priority task...");
    xTaskCreate(low_priority_task, "LowPrio", TASK_STACK_SIZE,
                NULL, LOW_PRIORITY, &xLowTaskHandle);

    ESP_LOGI(TAG, "Membuat MED priority task...");
    xTaskCreate(med_priority_task, "MedPrio", TASK_STACK_SIZE,
                NULL, MED_PRIORITY, &xMedTaskHandle);

    ESP_LOGI(TAG, "Membuat HIGH priority task...");
    xTaskCreate(high_priority_task, "HighPrio", TASK_STACK_SIZE,
                NULL, HIGH_PRIORITY, &xHighTaskHandle);

    ESP_LOGI(TAG, "Membuat Monitor task...");
    xTaskCreate(monitor_task, "Monitor", TASK_STACK_SIZE,
                NULL, MONITOR_PRIORITY, &xMonitorHandle);

    ESP_LOGI(TAG, "Semua task berhasil dibuat. Observasi urutan eksekusi...");
    ESP_LOGI(TAG, "============================================");
}
