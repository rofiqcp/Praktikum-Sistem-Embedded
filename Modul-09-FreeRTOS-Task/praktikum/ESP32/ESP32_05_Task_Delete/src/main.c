/* ============================================================
 * ESP32_05_Task_Delete - Penghapusan Task FreeRTOS
 * ============================================================
 * Program ini mendemonstrasikan vTaskDelete() dan pola
 * manajemen memori terkait pembuatan/penghapusan task.
 *
 * Konsep yang dipelajari:
 * 1. vTaskDelete() - menghapus task (dari task lain atau self)
 * 2. Self-deleting task pattern
 * 3. Memory management saat create/delete task
 * 4. Heap monitoring dengan esp_get_free_heap_size()
 * 5. Task lifecycle management
 * 6. Memory leak detection
 * 7. Proper cleanup sebelum delete
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1
 * - LED1 GPIO2 (task alive indicator)
 * - LED2 GPIO4 (self-deleting task indicator)
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "config.h"

static const char *TAG = "TASK_DEL";

/* Task handles */
static TaskHandle_t xWorkerHandle = NULL;
static TaskHandle_t xManagerHandle = NULL;
static TaskHandle_t xMonitorHandle = NULL;

/* Counter dan state tracking */
static volatile uint32_t worker_counter = 0;
static volatile uint32_t self_delete_counter = 0;
static volatile uint32_t create_count = 0;
static volatile uint32_t delete_count = 0;
static volatile bool worker_alive = false;

/* Heap tracking */
static uint32_t heap_samples[MAX_HEAP_SAMPLES];
static uint32_t heap_sample_index = 0;
static uint32_t heap_at_start = 0;

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
 * @brief Simpan sample heap ke array tracking
 * @param heap_size Ukuran free heap saat ini
 */
static void record_heap(uint32_t heap_size)
{
    if (heap_sample_index < MAX_HEAP_SAMPLES) {
        heap_samples[heap_sample_index++] = heap_size;
    }
}

/* ============================================================
 * TASK: Worker - Task yang bisa dihapus dari luar
 * ============================================================
 * Task ini berjalan terus-menerus mengedipkan LED1.
 * Akan dihapus oleh Manager task setelah beberapa waktu.
 *
 * Ketika vTaskDelete() dipanggil:
 * - Task segera dihapus dari scheduler
 * - Stack dan TCB yang dialokasikan oleh xTaskCreate()
 *   akan dibebaskan oleh IDLE task
 * - Sumber daya lain (heap alloc, file handle, dll)
 *   TIDAK otomatis dibebaskan -> potential memory leak!
 *
 * BEST PRACTICE: Jangan alokasikan resource di task yang
 * akan dihapus dari luar. Atau gunakan cleanup callback.
 * ============================================================ */
static void worker_task(void *pvParameters)
{
    uint8_t led_state = 0;
    uint32_t local_count = 0;

    /* Simulasi alokasi resource yang perlu di-cleanup */
    char *work_buffer = pvPortMalloc(256);
    if (work_buffer) {
        snprintf(work_buffer, 256, "Worker task #%lu", (unsigned long)create_count);
        ESP_LOGI(TAG, "[WORKER] Buffer allocated: '%s'", work_buffer);
    }

    ESP_LOGI(TAG, "[WORKER] Task dimulai! (instance #%lu)",
             (unsigned long)create_count);
    worker_alive = true;

    printf("[DATA] WORKER,START,%lu,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)create_count,
           (unsigned long)esp_get_free_heap_size());

    while (1) {
        led_state = !led_state;
        gpio_set_level(LED1_GPIO, led_state);
        worker_counter++;
        local_count++;

        if (local_count % 10 == 0) {
            printf("[DATA] WORKER,RUNNING,%lu,%lu,%d\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)worker_counter,
                   led_state);
        }

        vTaskDelay(pdMS_TO_TICKS(WORKER_BLINK_MS));
    }

    /*
     * CATATAN: Kode di bawah ini TIDAK PERNAH tercapai karena:
     * 1. Task dihapus dari luar via vTaskDelete() oleh manager
     * 2. Worker memiliki infinite loop tanpa exit condition
     *
     * Ini menunjukkan masalah: work_buffer yang dialokasikan
     * di atas akan LEAK karena tidak pernah di-free!
     *
     * Solusi: Gunakan self-deleting pattern dengan cleanup,
     * atau gunakan task notification untuk meminta task
     * cleanup dan self-delete.
     */
    if (work_buffer) {
        vPortFree(work_buffer);
    }
    vTaskDelete(NULL);
}

/* ============================================================
 * TASK: Self-Deleting Task
 * ============================================================
 * Pola task yang menghapus dirinya sendiri setelah selesai.
 * Ini adalah pola yang LEBIH AMAN karena task bisa melakukan
 * cleanup sebelum menghapus dirinya.
 *
 * vTaskDelete(NULL) menghapus calling task sendiri:
 * - NULL sebagai parameter berarti "hapus diri sendiri"
 * - Task langsung dihapus, kode setelahnya TIDAK dieksekusi
 * - IDLE task akan membersihkan TCB dan stack
 *
 * Pola ini cocok untuk:
 * - One-shot task (jalankan sekali lalu selesai)
 * - Initialization task
 * - Task dengan masa hidup terbatas
 * ============================================================ */
static void self_deleting_task(void *pvParameters)
{
    uint32_t task_id = (uint32_t)(intptr_t)pvParameters;
    uint8_t led_state = 0;
    uint32_t work_iterations = 0;

    ESP_LOGI(TAG, "[SELF_DEL #%lu] Task dimulai! Akan bekerja selama %d ms",
             (unsigned long)task_id, SELFDELETE_WORK_MS);

    /* Catat heap saat task mulai */
    uint32_t heap_at_start_local = esp_get_free_heap_size();

    printf("[DATA] SELFDELETE,START,%lu,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)task_id,
           (unsigned long)heap_at_start_local);

    /* Alokasi buffer kerja */
    uint8_t *work_data = pvPortMalloc(512);
    if (work_data == NULL) {
        ESP_LOGE(TAG, "[SELF_DEL #%lu] Gagal alokasi buffer!", (unsigned long)task_id);
        printf("[DATA] SELFDELETE,ERROR,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(), (unsigned long)task_id);
        vTaskDelete(NULL);  // Delete diri sendiri jika gagal
        return;  // Tidak akan pernah tercapai, tapi good practice
    }

    /* Kerja selama SELFDELETE_WORK_MS */
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t work_ticks = pdMS_TO_TICKS(SELFDELETE_WORK_MS);

    while ((xTaskGetTickCount() - start_tick) < work_ticks) {
        /* Simulasi kerja dengan LED toggle */
        led_state = !led_state;
        gpio_set_level(LED2_GPIO, led_state);
        work_iterations++;

        /* Gunakan buffer (mencegah optimisasi) */
        memset(work_data, (uint8_t)(work_iterations & 0xFF), 512);

        if (work_iterations % 5 == 0) {
            printf("[DATA] SELFDELETE,WORKING,%lu,%lu,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)task_id,
                   (unsigned long)work_iterations);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    /* === CLEANUP sebelum self-delete === */
    ESP_LOGI(TAG, "[SELF_DEL #%lu] Kerja selesai! Iterations=%lu. Cleanup...",
             (unsigned long)task_id, (unsigned long)work_iterations);

    /* Free buffer yang dialokasikan */
    vPortFree(work_data);
    work_data = NULL;

    /* Matikan LED */
    gpio_set_level(LED2_GPIO, 0);

    /* Update counter global */
    self_delete_counter++;

    printf("[DATA] SELFDELETE,DONE,%lu,%lu,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)task_id,
           (unsigned long)work_iterations,
           (unsigned long)esp_get_free_heap_size());

    ESP_LOGI(TAG, "[SELF_DEL #%lu] Selamat tinggal! Menghapus diri sendiri...",
             (unsigned long)task_id);

    /*
     * vTaskDelete(NULL) - Self-delete
     *
     * Setelah pemanggilan ini, task TIDAK LAGI ADA.
     * - Kode setelah vTaskDelete(NULL) TIDAK akan dieksekusi
     * - IDLE task akan free TCB dan stack nanti
     * - Handle task tidak lagi valid setelah ini
     */
    vTaskDelete(NULL);

    /* DEAD CODE - tidak akan pernah dieksekusi */
    ESP_LOGE(TAG, "INI TIDAK BOLEH MUNCUL!");
}

/* ============================================================
 * TASK: Manager - Mengelola lifecycle worker tasks
 * ============================================================
 * Task ini mendemonstrasikan cycle create-delete:
 * 1. Buat worker task
 * 2. Biarkan worker berjalan beberapa detik
 * 3. Hapus worker dari luar
 * 4. Monitor heap
 * 5. Buat self-deleting task
 * 6. Tunggu task selesai sendiri
 * 7. Ulangi
 * ============================================================ */
static void manager_task(void *pvParameters)
{
    uint32_t cycle = 0;

    ESP_LOGI(TAG, "[MANAGER] Task dimulai - mengelola lifecycle tasks");
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        cycle++;

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "[MANAGER] === Cycle #%lu ===", (unsigned long)cycle);
        ESP_LOGI(TAG, "========================================");

        /* === FASE 1: Create dan Delete Worker dari luar === */
        uint32_t heap_before_create = esp_get_free_heap_size();
        record_heap(heap_before_create);

        ESP_LOGI(TAG, "[MANAGER] Fase 1: Membuat Worker Task");
        ESP_LOGI(TAG, "[MANAGER] Heap sebelum create: %lu bytes",
                 (unsigned long)heap_before_create);

        printf("[DATA] CYCLE,START,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)cycle,
               (unsigned long)heap_before_create);

        /* Buat worker task */
        create_count++;
        BaseType_t ret = xTaskCreate(
            worker_task,
            "Worker",
            TASK_STACK_SIZE,
            NULL,
            WORKER_PRIORITY,
            &xWorkerHandle
        );

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "[MANAGER] GAGAL membuat worker! Heap=%lu",
                     (unsigned long)esp_get_free_heap_size());
            printf("[DATA] CREATE_FAIL,%lu,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)esp_get_free_heap_size());
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        uint32_t heap_after_create = esp_get_free_heap_size();
        uint32_t task_memory_cost = heap_before_create - heap_after_create;

        ESP_LOGI(TAG, "[MANAGER] Worker dibuat! Memory cost: %lu bytes",
                 (unsigned long)task_memory_cost);

        printf("[DATA] CREATED,%lu,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)create_count,
               (unsigned long)heap_after_create,
               (unsigned long)task_memory_cost);

        /* Biarkan worker berjalan */
        ESP_LOGI(TAG, "[MANAGER] Worker berjalan selama %d ms...", WORKER_LIFETIME_MS);
        vTaskDelay(pdMS_TO_TICKS(WORKER_LIFETIME_MS));

        /* Hapus worker dari luar */
        if (xWorkerHandle != NULL && worker_alive) {
            uint32_t heap_before_delete = esp_get_free_heap_size();

            ESP_LOGW(TAG, "[MANAGER] Menghapus Worker task...");

            /*
             * vTaskDelete(handle) - Delete task dari luar
             *
             * Parameter: TaskHandle dari task yang akan dihapus
             *
             * PERINGATAN:
             * 1. Task langsung dihapus, tidak ada kesempatan cleanup
             * 2. Resources yang dialokasikan task (malloc, dll) LEAK!
             * 3. TCB dan stack di-free oleh IDLE task (bukan segera)
             * 4. Handle tidak lagi valid setelah delete
             *
             * Best practice: Lebih baik gunakan task notification
             * untuk meminta task melakukan cleanup dan self-delete.
             */
            vTaskDelete(xWorkerHandle);
            delete_count++;
            worker_alive = false;
            xWorkerHandle = NULL;

            /* Matikan LED worker */
            gpio_set_level(LED1_GPIO, 0);

            /* IDLE task perlu waktu untuk free memory */
            vTaskDelay(pdMS_TO_TICKS(100));

            uint32_t heap_after_delete = esp_get_free_heap_size();
            int32_t recovered = (int32_t)heap_after_delete - (int32_t)heap_before_delete;

            ESP_LOGI(TAG, "[MANAGER] Worker dihapus! Memory recovered: %ld bytes",
                     (long)recovered);

            printf("[DATA] DELETED,%lu,%lu,%lu,%ld,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)delete_count,
                   (unsigned long)heap_after_delete,
                   (long)recovered,
                   (unsigned long)task_memory_cost);

            if ((uint32_t)recovered < task_memory_cost) {
                ESP_LOGW(TAG, "[MANAGER] ⚠ Memory leak detected! "
                         "Allocated: %lu, Recovered: %ld",
                         (unsigned long)task_memory_cost, (long)recovered);
                printf("[DATA] LEAK_WARN,%lu,%lu,%ld\n",
                       (unsigned long)get_elapsed_ms(),
                       (unsigned long)task_memory_cost,
                       (long)recovered);
            }
        }

        record_heap(esp_get_free_heap_size());

        /* === FASE 2: Self-deleting task === */
        ESP_LOGI(TAG, "[MANAGER] Fase 2: Membuat Self-Deleting Task");

        uint32_t heap_before_self = esp_get_free_heap_size();

        TaskHandle_t xSelfDelHandle = NULL;
        xTaskCreate(
            self_deleting_task,
            "SelfDel",
            TASK_STACK_SIZE,
            (void *)(intptr_t)cycle,
            SELFDELETE_PRIORITY,
            &xSelfDelHandle
        );

        uint32_t heap_after_self_create = esp_get_free_heap_size();
        ESP_LOGI(TAG, "[MANAGER] Self-deleting task dibuat. Cost: %lu bytes",
                 (unsigned long)(heap_before_self - heap_after_self_create));

        /* Tunggu self-deleting task selesai */
        ESP_LOGI(TAG, "[MANAGER] Menunggu self-deleting task selesai...");
        vTaskDelay(pdMS_TO_TICKS(SELFDELETE_WORK_MS + 2000));

        /* Cek apakah task sudah dihapus */
        if (xSelfDelHandle != NULL) {
            eTaskState state = eTaskGetState(xSelfDelHandle);
            if (state == eDeleted || state == eReady) {
                ESP_LOGI(TAG, "[MANAGER] Self-deleting task sudah selesai");
            }
        }

        /* IDLE task perlu waktu cleanup */
        vTaskDelay(pdMS_TO_TICKS(200));

        uint32_t heap_after_self_done = esp_get_free_heap_size();
        int32_t self_recovered = (int32_t)heap_after_self_done - (int32_t)heap_after_self_create;

        ESP_LOGI(TAG, "[MANAGER] Self-del cleanup. Heap now: %lu, recovered: %ld",
                 (unsigned long)heap_after_self_done, (long)self_recovered);

        printf("[DATA] SELFDELETE_CLEANUP,%lu,%lu,%ld\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)heap_after_self_done,
               (long)self_recovered);

        record_heap(heap_after_self_done);

        /* Summary */
        printf("[DATA] CYCLE,END,%lu,%lu,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)cycle,
               (unsigned long)create_count,
               (unsigned long)delete_count,
               (unsigned long)heap_after_self_done);

        /* Delay sebelum cycle berikutnya */
        ESP_LOGI(TAG, "[MANAGER] Cycle #%lu selesai. Delay %d ms...",
                 (unsigned long)cycle, CREATE_DELETE_CYCLE);
        vTaskDelay(pdMS_TO_TICKS(CREATE_DELETE_CYCLE));
    }
}

/* ============================================================
 * TASK: Heap Monitor - Memantau penggunaan memori
 * ============================================================
 * Monitoring heap secara periodik untuk deteksi memory leak.
 * Pada ESP32, heap terdiri dari beberapa region:
 * - DRAM: data RAM utama (~300KB)
 * - IRAM: instruction RAM (bisa digunakan untuk data)
 * - PSRAM: external PSRAM (jika ada, hingga 4MB)
 * ============================================================ */
static void heap_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[HEAP_MON] Task dimulai");

    while (1) {
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_heap = esp_get_minimum_free_heap_size();
        UBaseType_t num_tasks = uxTaskGetNumberOfTasks();

        printf("[DATA] HEAP,%lu,%lu,%lu,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)free_heap,
               (unsigned long)min_heap,
               (unsigned long)num_tasks,
               (unsigned long)create_count,
               (unsigned long)delete_count);

        ESP_LOGI(TAG, "[HEAP] Free: %lu, Min: %lu, Tasks: %d, Created: %lu, Deleted: %lu",
                 (unsigned long)free_heap,
                 (unsigned long)min_heap,
                 (int)num_tasks,
                 (unsigned long)create_count,
                 (unsigned long)delete_count);

        /* Warning jika heap rendah */
        if (free_heap < HEAP_WARN_THRESHOLD) {
            ESP_LOGW(TAG, "⚠ LOW HEAP WARNING! Free: %lu < %d bytes",
                     (unsigned long)free_heap, HEAP_WARN_THRESHOLD);
            printf("[DATA] HEAP_LOW,%lu,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)free_heap);

            /* Rapid blink LED sebagai warning */
            for (int i = 0; i < 10; i++) {
                gpio_set_level(LED1_GPIO, i % 2);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        /* Deteksi memory leak trend */
        if (heap_sample_index > 5) {
            int32_t trend = (int32_t)heap_samples[heap_sample_index - 1] -
                           (int32_t)heap_samples[0];
            if (trend < -1000) {
                ESP_LOGW(TAG, "⚠ Memory leak trend detected! Lost %ld bytes total",
                         (long)(-trend));
                printf("[DATA] LEAK_TREND,%lu,%ld\n",
                       (unsigned long)get_elapsed_ms(), (long)trend);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* ============================================================
 * APP_MAIN
 * ============================================================ */
void app_main(void)
{
    start_time_us = esp_timer_get_time();
    heap_at_start = esp_get_free_heap_size();
    memset(heap_samples, 0, sizeof(heap_samples));

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 FreeRTOS Task Delete Demo");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Demonstrasi vTaskDelete() behavior:");
    ESP_LOGI(TAG, "1. Delete task dari luar (external delete)");
    ESP_LOGI(TAG, "2. Self-deleting task pattern");
    ESP_LOGI(TAG, "3. Heap monitoring untuk deteksi memory leak");
    ESP_LOGI(TAG, "Starting heap: %lu bytes", (unsigned long)heap_at_start);

    printf("[DATA] INIT,%lu,%lu\n",
           (unsigned long)get_elapsed_ms(),
           (unsigned long)heap_at_start);

    init_gpio();

    /* Buat task manager (mengontrol lifecycle task lain) */
    xTaskCreate(manager_task, "Manager", TASK_STACK_SIZE * 2,
                NULL, MANAGER_PRIORITY, &xManagerHandle);

    /* Buat heap monitor */
    xTaskCreate(heap_monitor_task, "HeapMon", TASK_STACK_SIZE,
                NULL, MONITOR_PRIORITY, &xMonitorHandle);

    ESP_LOGI(TAG, "Manager dan Monitor task dibuat.");
    ESP_LOGI(TAG, "============================================");
}
