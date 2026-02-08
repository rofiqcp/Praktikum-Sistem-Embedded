/**
 * ============================================================================
 * PROGRAM 11: ESP32 Task Scheduler Info
 * ============================================================================
 * Menampilkan informasi lengkap scheduler FreeRTOS
 * 
 * Fitur utama:
 *   - vTaskList()            : Daftar semua task dengan state/priority/stack
 *   - vTaskGetRunTimeStats() : Statistik CPU usage per task
 *   - uxTaskGetNumberOfTasks() : Jumlah total task
 *   - uxTaskGetSystemState() : Info detail setiap task
 * 
 * Demonstrasi:
 *   - Membuat/menghapus task secara dinamis
 *   - Menunjukkan semua state: Running, Ready, Blocked, Suspended, Deleted
 *   - Mencetak tabel terformat setiap 3 detik
 * 
 * CATATAN: Memerlukan sdkconfig.defaults dengan:
 *   CONFIG_FREERTOS_USE_TRACE_FACILITY=y
 *   CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
 *   CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
 *   CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID=y
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
static const char *TAG = "SCHED_INFO";

/* ======================== VARIABEL GLOBAL ================================ */

/* Handle task */
static TaskHandle_t monitor_handle = NULL;
static TaskHandle_t worker1_handle = NULL;
static TaskHandle_t worker2_handle = NULL;
static TaskHandle_t blocked_handle = NULL;
static TaskHandle_t suspended_handle = NULL;
static TaskHandle_t dynamic_handle = NULL;

/* Counter task */
static volatile uint32_t worker1_count = 0;
static volatile uint32_t worker2_count = 0;
static volatile uint32_t dynamic_count = 0;

/* Fase saat ini */
static volatile int current_phase = 0;
static volatile int dynamic_task_number = 0;

/* ======================== FUNGSI UTILITAS ================================ */

/**
 * Inisialisasi GPIO
 */
static void init_gpio(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << LED_STATUS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);
    gpio_set_level(LED_STATUS_PIN, 0);
}

/**
 * Konversi state task FreeRTOS ke string Indonesia
 * @param state Kode state dari eTaskGetState()
 * @return String deskripsi state
 */
static const char* state_to_string(eTaskState state)
{
    switch (state) {
        case eRunning:   return "RUNNING  ";
        case eReady:     return "READY    ";
        case eBlocked:   return "BLOCKED  ";
        case eSuspended: return "SUSPENDED";
        case eDeleted:   return "DELETED  ";
        default:         return "UNKNOWN  ";
    }
}

/**
 * Cetak tabel task list menggunakan vTaskList()
 * 
 * Format output vTaskList():
 *   Name          State  Priority  Stack  Num  Core
 *   
 * State codes:
 *   X = Running, R = Ready, B = Blocked, S = Suspended, D = Deleted
 */
static void print_task_list(void)
{
    char *task_list_buf = malloc(TASK_LIST_BUFFER_SIZE);
    if (task_list_buf == NULL) {
        ESP_LOGE(TAG, "Gagal alokasi buffer untuk vTaskList()");
        return;
    }

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║               DAFTAR TASK FreeRTOS                      ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Name            State    Prio   Stack   Num   Core      ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");

    vTaskList(task_list_buf);
    
    /* Parse dan cetak setiap baris */
    char *line = strtok(task_list_buf, "\n");
    while (line != NULL) {
        printf("║ %-56s ║\n", line);
        
        /* Parse untuk data output */
        char name[32];
        char state;
        int prio, stack, num;
        if (sscanf(line, "%31s %c %d %d %d", name, &state, &prio, &stack, &num) >= 4) {
            printf("[DATA]TASK_LIST,name=%s,state=%c,priority=%d,stack=%d,num=%d\n",
                   name, state, prio, stack, num);
        }
        
        line = strtok(NULL, "\n");
    }

    printf("╚══════════════════════════════════════════════════════════╝\n");

    free(task_list_buf);
}

/**
 * Cetak runtime statistics menggunakan vTaskGetRunTimeStats()
 * Menunjukkan berapa persen CPU time yang digunakan setiap task
 */
static void print_runtime_stats(void)
{
#if configGENERATE_RUN_TIME_STATS
    char *stats_buf = malloc(RUNTIME_BUFFER_SIZE);
    if (stats_buf == NULL) {
        ESP_LOGE(TAG, "Gagal alokasi buffer untuk runtime stats");
        return;
    }

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║            CPU RUNTIME STATISTICS                       ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Task            Abs Time       %% Time                    ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");

    vTaskGetRunTimeStats(stats_buf);

    char *line = strtok(stats_buf, "\n");
    while (line != NULL) {
        printf("║ %-56s ║\n", line);
        
        /* Parse untuk data output */
        char name[32];
        unsigned long runtime;
        unsigned long pct;
        if (sscanf(line, "%31s %lu %lu%%", name, &runtime, &pct) >= 2) {
            printf("[DATA]RUNTIME,name=%s,abs_time=%lu,pct=%lu\n",
                   name, runtime, pct);
        }
        
        line = strtok(NULL, "\n");
    }

    printf("╚══════════════════════════════════════════════════════════╝\n");

    free(stats_buf);
#else
    printf("Runtime stats tidak tersedia (configGENERATE_RUN_TIME_STATS disabled)\n");
#endif
}

/**
 * Cetak informasi detail menggunakan uxTaskGetSystemState()
 * Memberikan info lebih lengkap termasuk base priority dan core affinity
 */
static void print_system_state(void)
{
    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    printf("[DATA]TOTAL_TASKS,count=%lu\n", (uint32_t)num_tasks);

    TaskStatus_t *task_array = malloc(num_tasks * sizeof(TaskStatus_t));
    if (task_array == NULL) {
        ESP_LOGE(TAG, "Gagal alokasi buffer untuk system state");
        return;
    }

    uint32_t total_runtime;
    UBaseType_t actual = uxTaskGetSystemState(task_array, num_tasks, &total_runtime);

    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              SYSTEM STATE DETAIL (%lu tasks)                ║\n", (uint32_t)actual);
    printf("╠══════════════════════════════════════════════════════════════╣\n");

    for (UBaseType_t i = 0; i < actual; i++) {
        TaskStatus_t *t = &task_array[i];
        
        printf("║ [%2lu] %-16s %s  Prio:%lu  Stack:%5lu  RT:%8lu  ║\n",
               (uint32_t)t->xTaskNumber,
               t->pcTaskName,
               state_to_string(t->eCurrentState),
               (uint32_t)t->uxCurrentPriority,
               (uint32_t)t->usStackHighWaterMark,
               (uint32_t)t->ulRunTimeCounter);

        /* Data output untuk parser */
        printf("[DATA]SYS_STATE,num=%lu,name=%s,state=%d,prio=%lu,"
               "stack=%lu,runtime=%lu,base_prio=%lu\n",
               (uint32_t)t->xTaskNumber,
               t->pcTaskName,
               t->eCurrentState,
               (uint32_t)t->uxCurrentPriority,
               (uint32_t)t->usStackHighWaterMark,
               (uint32_t)t->ulRunTimeCounter,
               (uint32_t)t->uxBasePriority);
    }

    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Total Runtime Counter: %lu                                  ║\n", total_runtime);
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    free(task_array);
}

/* ======================== TASK FUNCTIONS ================================= */

/**
 * Worker Task 1 - task yang aktif bekerja (state: Ready/Running)
 */
static void worker_task_1(void *pvParameters)
{
    ESP_LOGI(TAG, "Worker1 dimulai pada Core %d", xPortGetCoreID());

    while (1) {
        worker1_count++;
        /* Lakukan sedikit pekerjaan */
        volatile float result = 1.0f;
        for (int i = 0; i < 10000; i++) {
            result += i * 0.001f;
        }
        vTaskDelay(pdMS_TO_TICKS(WORKER_DELAY_MS));
    }
}

/**
 * Worker Task 2 - task yang aktif bekerja
 */
static void worker_task_2(void *pvParameters)
{
    ESP_LOGI(TAG, "Worker2 dimulai pada Core %d", xPortGetCoreID());

    while (1) {
        worker2_count++;
        volatile float result = 1.0f;
        for (int i = 0; i < 5000; i++) {
            result *= 1.0001f;
        }
        vTaskDelay(pdMS_TO_TICKS(WORKER_DELAY_MS * 2));
    }
}

/**
 * Blocked Task - task yang selalu dalam state Blocked (menunggu delay lama)
 */
static void blocked_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "BlockedTask dimulai - akan selalu Blocked (delay panjang)");

    while (1) {
        /* Delay sangat lama - task ini akan hampir selalu dalam state Blocked */
        vTaskDelay(pdMS_TO_TICKS(30000));  /* 30 detik */
    }
}

/**
 * Suspended Task - task yang bisa di-suspend dan di-resume
 */
static void suspended_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "SuspendedTask dimulai");

    while (1) {
        /* Task ini akan di-suspend oleh monitor */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * Dynamic Task - task yang dibuat dan dihapus secara dinamis
 * Umurnya singkat untuk menunjukkan lifecycle task
 */
static void dynamic_task_fn(void *pvParameters)
{
    int task_num = (int)(intptr_t)pvParameters;
    ESP_LOGI(TAG, "DynamicTask #%d dimulai pada Core %d", task_num, xPortGetCoreID());

    printf("[DATA]DYNAMIC_CREATE,num=%d,core=%d\n", task_num, xPortGetCoreID());

    /* Bekerja selama DYNAMIC_LIFETIME_MS lalu self-delete */
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) * portTICK_PERIOD_MS < DYNAMIC_LIFETIME_MS) {
        dynamic_count++;
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    printf("[DATA]DYNAMIC_DELETE,num=%d,total_work=%lu\n", task_num, dynamic_count);
    ESP_LOGI(TAG, "DynamicTask #%d selesai, menghapus diri", task_num);

    dynamic_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * Monitor Task - mencetak semua info scheduler secara periodik
 * 
 * Fase demonstrasi:
 *   1. Semua task aktif
 *   2. Suspend satu task
 *   3. Buat task dinamis
 *   4. Resume suspended task, hapus task dinamis
 */
static void monitor_task_fn(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor dimulai pada Core %d", xPortGetCoreID());
    uint32_t cycle = 0;
    TickType_t phase_start = xTaskGetTickCount();

    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        cycle++;
        TickType_t now = xTaskGetTickCount();

        /* Ganti fase setiap PHASE_DURATION_MS */
        if ((now - phase_start) * portTICK_PERIOD_MS >= PHASE_DURATION_MS) {
            current_phase = (current_phase + 1) % NUM_PHASES;
            phase_start = now;

            printf("\n[DATA]PHASE_CHANGE,phase=%d\n", current_phase);

            switch (current_phase) {
                case 0:
                    /* Fase 0: Semua task aktif */
                    ESP_LOGI(TAG, "=== FASE 0: Semua task aktif ===");
                    if (suspended_handle) {
                        vTaskResume(suspended_handle);
                        printf("[DATA]TASK_RESUME,name=Suspended\n");
                    }
                    break;

                case 1:
                    /* Fase 1: Suspend satu task */
                    ESP_LOGI(TAG, "=== FASE 1: Suspend satu task ===");
                    if (suspended_handle) {
                        vTaskSuspend(suspended_handle);
                        printf("[DATA]TASK_SUSPEND,name=Suspended\n");
                    }
                    break;

                case 2:
                    /* Fase 2: Buat task dinamis */
                    ESP_LOGI(TAG, "=== FASE 2: Buat task dinamis ===");
                    dynamic_task_number++;
                    if (dynamic_handle == NULL) {
                        xTaskCreatePinnedToCore(
                            dynamic_task_fn, "DynTask",
                            DYNAMIC_TASK_STACK,
                            (void *)(intptr_t)dynamic_task_number,
                            DYNAMIC_TASK_PRIORITY,
                            &dynamic_handle, tskNO_AFFINITY);
                    }
                    break;

                case 3:
                    /* Fase 3: Resume dan bersihkan */
                    ESP_LOGI(TAG, "=== FASE 3: Resume & cleanup ===");
                    if (suspended_handle) {
                        vTaskResume(suspended_handle);
                        printf("[DATA]TASK_RESUME,name=Suspended\n");
                    }
                    break;
            }
        }

        /* LED toggle */
        gpio_set_level(LED_STATUS_PIN, cycle % 2);

        printf("\n████████████████████████████████████████████████████████████\n");
        printf("  SCHEDULER INFO REPORT - Siklus #%lu (Fase %d)\n", cycle, current_phase);
        printf("████████████████████████████████████████████████████████████\n");

        /* Jumlah total task */
        UBaseType_t total_tasks = uxTaskGetNumberOfTasks();
        printf("[DATA]TASK_COUNT,total=%lu,phase=%d\n", (uint32_t)total_tasks, current_phase);

        /* 1. Cetak vTaskList() */
        print_task_list();

        /* 2. Cetak runtime stats */
        print_runtime_stats();

        /* 3. Cetak system state detail */
        print_system_state();

        /* 4. Cetak info tambahan */
        printf("[DATA]WORKER_COUNTS,w1=%lu,w2=%lu,dyn=%lu\n",
               worker1_count, worker2_count, dynamic_count);

        /* Stack high water marks */
        if (worker1_handle) {
            printf("[DATA]WATERMARK,task=Worker1,hwm=%u\n",
                   (unsigned)uxTaskGetStackHighWaterMark(worker1_handle));
        }
        if (worker2_handle) {
            printf("[DATA]WATERMARK,task=Worker2,hwm=%u\n",
                   (unsigned)uxTaskGetStackHighWaterMark(worker2_handle));
        }

        /* Heap info */
        printf("[DATA]HEAP,free=%lu,min=%lu\n",
               (uint32_t)esp_get_free_heap_size(),
               (uint32_t)esp_get_minimum_free_heap_size());

        printf("[DATA]UPTIME,ms=%lld\n", esp_timer_get_time() / 1000);

        vTaskDelay(pdMS_TO_TICKS(REPORT_INTERVAL_MS));
    }
}

/* ======================== ENTRY POINT ==================================== */

void app_main(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("  ESP32 FreeRTOS - Task Scheduler Info Demo\n");
    printf("  vTaskList, vTaskGetRunTimeStats, uxTaskGetSystemState\n");
    printf("============================================================\n");
    printf("  Total task saat boot: %lu\n", (uint32_t)uxTaskGetNumberOfTasks());
    printf("  Free Heap: %lu bytes\n", (uint32_t)esp_get_free_heap_size());
    printf("  app_main() berjalan di Core %d\n", xPortGetCoreID());
    printf("============================================================\n\n");

    /* Inisialisasi GPIO */
    init_gpio();

    /* Buat task-task demo */
    ESP_LOGI(TAG, "Membuat task-task demo...");

    xTaskCreatePinnedToCore(worker_task_1, "Worker1", WORKER_TASK_STACK,
                            NULL, WORKER_TASK_PRIORITY, &worker1_handle, 1);

    xTaskCreatePinnedToCore(worker_task_2, "Worker2", WORKER_TASK_STACK,
                            NULL, WORKER_TASK_PRIORITY, &worker2_handle, 0);

    xTaskCreatePinnedToCore(blocked_task_fn, "Blocked", BLOCKED_TASK_STACK,
                            NULL, BLOCKED_TASK_PRIORITY, &blocked_handle, 1);

    xTaskCreatePinnedToCore(suspended_task_fn, "Suspended", SUSPENDED_TASK_STACK,
                            NULL, SUSPENDED_TASK_PRIORITY, &suspended_handle, 1);

    /* Buat monitor task */
    xTaskCreatePinnedToCore(monitor_task_fn, "Monitor", MONITOR_TASK_STACK,
                            NULL, MONITOR_TASK_PRIORITY, &monitor_handle, 0);

    ESP_LOGI(TAG, "Semua task dibuat. Total: %lu", (uint32_t)uxTaskGetNumberOfTasks());
    printf("[DATA]STATUS,system=started,initial_tasks=%lu\n",
           (uint32_t)uxTaskGetNumberOfTasks());
}
