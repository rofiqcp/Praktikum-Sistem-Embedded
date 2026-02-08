/* ============================================================
 * ESP32_06_Task_Stack_Monitor - Monitoring Stack Task FreeRTOS
 * ============================================================
 * Program ini mendemonstrasikan monitoring stack usage pada
 * FreeRTOS tasks, termasuk deteksi stack overflow.
 *
 * Konsep yang dipelajari:
 * 1. Stack allocation untuk tasks (ukuran berbeda)
 * 2. uxTaskGetStackHighWaterMark() - cek sisa stack minimum
 * 3. Stack overflow detection (configCHECK_FOR_STACK_OVERFLOW)
 * 4. Rekursi dan konsumsi stack
 * 5. Stack sizing best practices
 * 6. Memory-safe embedded programming
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1
 * - LED1 GPIO2 (stack warning indicator)
 * - LED2 GPIO4 (normal operation indicator)
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

static const char *TAG = "STACK_MON";

/* Task handles */
static TaskHandle_t xShallowHandle = NULL;
static TaskHandle_t xMediumHandle = NULL;
static TaskHandle_t xDeepHandle = NULL;
static TaskHandle_t xMonitorHandle = NULL;

/* Stack usage tracking */
typedef struct {
    const char *name;
    TaskHandle_t *handle;
    uint32_t total_stack;
    uint32_t high_water_mark;
    uint32_t used_bytes;
    float usage_percent;
    uint32_t max_usage_percent;
    uint32_t sample_count;
    bool warning_issued;
} stack_info_t;

static stack_info_t stack_info[3] = {
    { "Shallow", &xShallowHandle, SMALL_STACK_SIZE, 0, 0, 0.0, 0, 0, false },
    { "Medium",  &xMediumHandle,  MEDIUM_STACK_SIZE, 0, 0, 0.0, 0, 0, false },
    { "Deep",    &xDeepHandle,    LARGE_STACK_SIZE, 0, 0, 0.0, 0, 0, false }
};

/* Cycle counters */
static volatile uint32_t shallow_cycles = 0;
static volatile uint32_t medium_cycles = 0;
static volatile uint32_t deep_cycles = 0;

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

/* ============================================================
 * STACK OVERFLOW HOOK
 * ============================================================
 * FreeRTOS menyediakan hook function yang dipanggil saat
 * stack overflow terdeteksi. Ini hanya aktif jika:
 * configCHECK_FOR_STACK_OVERFLOW >= 1
 *
 * Method 1 (config = 1): Cek stack pointer saat context switch
 * Method 2 (config = 2): Juga cek 16 bytes terakhir stack
 *
 * PERINGATAN: Saat hook dipanggil, state sudah corrupt!
 * Hanya gunakan untuk logging/restart, BUKAN recovery.
 *
 * Pada ESP-IDF, hook ini bisa diaktifkan via menuconfig:
 * Component config -> FreeRTOS -> Check for stack overflow
 * ============================================================ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /*
     * CRITICAL: Stack overflow terdeteksi!
     * Di sini kita hanya bisa print pesan darurat.
     * Pada production, biasanya restart sistem.
     */
    printf("\n\n!!! STACK OVERFLOW DETECTED !!!\n");
    printf("Task: %s\n", pcTaskName);
    printf("Restarting system...\n\n");

    printf("[DATA] STACK_OVERFLOW,%lu,%s\n",
           (unsigned long)get_elapsed_ms(), pcTaskName);

    /* Rapid blink LED warning */
    for (int i = 0; i < 20; i++) {
        gpio_set_level(LED1_GPIO, i % 2);
        for (volatile int j = 0; j < 100000; j++);
    }

    /* Restart ESP32 */
    esp_restart();
}

/* ============================================================
 * FUNGSI REKURSIF untuk konsumsi stack
 * ============================================================
 * Setiap pemanggilan rekursif menambah stack frame yang berisi:
 * - Return address (4 bytes)
 * - Saved registers
 * - Local variables
 * - Alignment padding
 *
 * Pada ESP32 (Xtensa LX6):
 * - Register window = 64 registers
 * - Frame size tergantung jumlah local vars
 * - Typical minimum: 32-64 bytes per frame
 * ============================================================ */

/**
 * @brief Rekursi dangkal - minimal stack usage
 * @param depth Kedalaman rekursi tersisa
 * @param buffer_size Ukuran buffer lokal per frame
 * @return Hasil komputasi (mencegah tail-call optimization)
 *
 * Stack usage per frame ≈ buffer_size + overhead (~32 bytes)
 */
static uint32_t shallow_recursive(int depth, int buffer_size)
{
    /*
     * Buffer lokal di stack - mengkonsumsi stack space
     * volatile mencegah compiler optimize-out
     */
    volatile uint8_t local_buffer[SMALL_BUFFER_SIZE];

    /* Isi buffer agar compiler tidak optimize away */
    memset((void*)local_buffer, depth & 0xFF, SMALL_BUFFER_SIZE);

    if (depth <= 0) {
        /* Base case - return checksum */
        uint32_t sum = 0;
        for (int i = 0; i < SMALL_BUFFER_SIZE; i++) {
            sum += local_buffer[i];
        }
        return sum;
    }

    /* Recursive case */
    return local_buffer[0] + shallow_recursive(depth - 1, buffer_size);
}

/**
 * @brief Rekursi menengah - moderate stack usage
 */
static uint32_t medium_recursive(int depth, int buffer_size)
{
    volatile uint8_t local_buffer[MEDIUM_BUFFER_SIZE];
    memset((void*)local_buffer, depth & 0xFF, MEDIUM_BUFFER_SIZE);

    if (depth <= 0) {
        uint32_t sum = 0;
        for (int i = 0; i < MEDIUM_BUFFER_SIZE; i++) {
            sum += local_buffer[i];
        }
        return sum;
    }

    return local_buffer[0] + medium_recursive(depth - 1, buffer_size);
}

/**
 * @brief Rekursi dalam - heavy stack usage
 *
 * PERINGATAN: Dengan buffer 128 bytes dan depth 40:
 * Stack usage ≈ 40 * (128 + 32) = ~6400 bytes minimum
 * Membutuhkan stack > 8192 untuk safety margin!
 */
static uint32_t deep_recursive(int depth, int buffer_size)
{
    volatile uint8_t local_buffer[LARGE_BUFFER_SIZE];
    memset((void*)local_buffer, depth & 0xFF, LARGE_BUFFER_SIZE);

    if (depth <= 0) {
        uint32_t sum = 0;
        for (int i = 0; i < LARGE_BUFFER_SIZE; i++) {
            sum += local_buffer[i];
        }
        return sum;
    }

    return local_buffer[0] + deep_recursive(depth - 1, buffer_size);
}

/* ============================================================
 * TASK: Shallow Stack Usage
 * ============================================================
 * Task dengan stack kecil (2048 bytes) dan rekursi dangkal.
 * Menunjukkan task yang menggunakan stack minimal.
 * ============================================================ */
static void shallow_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[SHALLOW] Task dimulai - Stack: %d bytes, Depth: %d",
             SMALL_STACK_SIZE, SHALLOW_DEPTH);

    while (1) {
        shallow_cycles++;

        /* Toggle LED untuk indikasi aktivitas */
        gpio_set_level(LED2_GPIO, shallow_cycles % 2);

        /* Lakukan rekursi dangkal */
        uint32_t result = shallow_recursive(SHALLOW_DEPTH, SMALL_BUFFER_SIZE);

        /*
         * uxTaskGetStackHighWaterMark() - Cek sisa stack minimum
         *
         * Mengembalikan MINIMUM jumlah free stack space (dalam words)
         * yang pernah tersisa sejak task dibuat.
         *
         * "High water mark" = titik tertinggi penggunaan stack.
         * Semakin kecil nilainya, semakin dekat ke overflow.
         *
         * Return value dalam WORDS (4 bytes pada ESP32).
         * Kalikan dengan sizeof(StackType_t) untuk bytes.
         *
         * Best practice: HWM harus > 20% total stack
         */
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        uint32_t hwm_bytes = hwm * sizeof(StackType_t);
        uint32_t used = SMALL_STACK_SIZE - hwm_bytes;
        float usage_pct = (float)used / SMALL_STACK_SIZE * 100.0f;

        if (shallow_cycles % 5 == 0) {
            printf("[DATA] STACK,Shallow,%lu,%lu,%lu,%lu,%.1f,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)SMALL_STACK_SIZE,
                   (unsigned long)hwm_bytes,
                   (unsigned long)used,
                   usage_pct,
                   (unsigned long)result);

            ESP_LOGI(TAG, "[SHALLOW] Stack: %lu/%d bytes used (%.1f%%), HWM=%lu",
                     (unsigned long)used, SMALL_STACK_SIZE, usage_pct,
                     (unsigned long)hwm_bytes);
        }

        /* Warning jika penggunaan > threshold */
        if (usage_pct > STACK_WARN_PERCENT) {
            ESP_LOGW(TAG, "⚠ [SHALLOW] Stack usage %.1f%% > %d%% threshold!",
                     usage_pct, STACK_WARN_PERCENT);
            printf("[DATA] STACK_WARN,Shallow,%lu,%.1f\n",
                   (unsigned long)get_elapsed_ms(), usage_pct);
            gpio_set_level(LED1_GPIO, 1);  // Warning LED
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_WORK_PERIOD_MS));
    }
}

/* ============================================================
 * TASK: Medium Stack Usage
 * ============================================================ */
static void medium_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[MEDIUM] Task dimulai - Stack: %d bytes, Depth: %d",
             MEDIUM_STACK_SIZE, MEDIUM_DEPTH);

    while (1) {
        medium_cycles++;
        uint32_t result = medium_recursive(MEDIUM_DEPTH, MEDIUM_BUFFER_SIZE);

        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        uint32_t hwm_bytes = hwm * sizeof(StackType_t);
        uint32_t used = MEDIUM_STACK_SIZE - hwm_bytes;
        float usage_pct = (float)used / MEDIUM_STACK_SIZE * 100.0f;

        if (medium_cycles % 5 == 0) {
            printf("[DATA] STACK,Medium,%lu,%lu,%lu,%lu,%.1f,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)MEDIUM_STACK_SIZE,
                   (unsigned long)hwm_bytes,
                   (unsigned long)used,
                   usage_pct,
                   (unsigned long)result);

            ESP_LOGI(TAG, "[MEDIUM] Stack: %lu/%d bytes used (%.1f%%)",
                     (unsigned long)used, MEDIUM_STACK_SIZE, usage_pct);
        }

        if (usage_pct > STACK_WARN_PERCENT) {
            ESP_LOGW(TAG, "⚠ [MEDIUM] Stack usage %.1f%% > %d%%!",
                     usage_pct, STACK_WARN_PERCENT);
            printf("[DATA] STACK_WARN,Medium,%lu,%.1f\n",
                   (unsigned long)get_elapsed_ms(), usage_pct);
            gpio_set_level(LED1_GPIO, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_WORK_PERIOD_MS));
    }
}

/* ============================================================
 * TASK: Deep Stack Usage
 * ============================================================
 * Task dengan stack besar dan rekursi dalam.
 * Menunjukkan penggunaan stack yang signifikan.
 *
 * Stack budget analysis:
 * - Depth: 40 levels
 * - Buffer per frame: 128 bytes
 * - Overhead per frame: ~48 bytes (registers, return addr, padding)
 * - Total per frame: ~176 bytes
 * - Total recursion: 40 * 176 = ~7040 bytes
 * - Task overhead: ~768 bytes
 * - TOTAL NEEDED: ~7808 bytes
 * - Allocated: 8192 bytes
 * - Safety margin: ~384 bytes (4.7%) <- TIGHT!
 * ============================================================ */
static void deep_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[DEEP] Task dimulai - Stack: %d bytes, Depth: %d",
             LARGE_STACK_SIZE, DEEP_DEPTH);
    ESP_LOGW(TAG, "[DEEP] Stack margin akan ketat! Observasi HWM...");

    while (1) {
        deep_cycles++;

        /*
         * Untuk safety, gunakan depth yang lebih moderat
         * saat pertama kali, lalu gradually increase
         */
        int current_depth = DEEP_DEPTH;
        if (deep_cycles <= 3) {
            current_depth = DEEP_DEPTH / 2;  // Start dengan setengah depth
            ESP_LOGI(TAG, "[DEEP] Warmup phase - using depth %d", current_depth);
        }

        uint32_t result = deep_recursive(current_depth, LARGE_BUFFER_SIZE);

        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        uint32_t hwm_bytes = hwm * sizeof(StackType_t);
        uint32_t used = LARGE_STACK_SIZE - hwm_bytes;
        float usage_pct = (float)used / LARGE_STACK_SIZE * 100.0f;

        if (deep_cycles % 5 == 0) {
            printf("[DATA] STACK,Deep,%lu,%lu,%lu,%lu,%.1f,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)LARGE_STACK_SIZE,
                   (unsigned long)hwm_bytes,
                   (unsigned long)used,
                   usage_pct,
                   (unsigned long)result);

            ESP_LOGI(TAG, "[DEEP] Stack: %lu/%d bytes used (%.1f%%), depth=%d",
                     (unsigned long)used, LARGE_STACK_SIZE, usage_pct, current_depth);
        }

        if (usage_pct > STACK_WARN_PERCENT) {
            ESP_LOGW(TAG, "⚠ [DEEP] Stack usage %.1f%% > %d%%! DANGER!",
                     usage_pct, STACK_WARN_PERCENT);
            printf("[DATA] STACK_WARN,Deep,%lu,%.1f\n",
                   (unsigned long)get_elapsed_ms(), usage_pct);

            /* Rapid blink warning LED */
            for (int i = 0; i < 6; i++) {
                gpio_set_level(LED1_GPIO, i % 2);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_WORK_PERIOD_MS * 2));
    }
}

/* ============================================================
 * TASK: Stack Monitor
 * ============================================================
 * Task khusus untuk monitoring stack usage semua task.
 * Menampilkan perbandingan stack usage secara periodik.
 * ============================================================ */
static void stack_monitor_task(void *pvParameters)
{
    uint32_t report_count = 0;

    ESP_LOGI(TAG, "[MONITOR] Stack monitor dimulai");
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        report_count++;

        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "============ STACK USAGE REPORT #%lu ============",
                 (unsigned long)report_count);

        /*
         * Query stack high water mark untuk setiap task.
         * Ini adalah analisis RUNTIME dari penggunaan stack aktual,
         * sangat penting untuk sizing stack yang tepat.
         */
        for (int i = 0; i < 3; i++) {
            if (*(stack_info[i].handle) != NULL) {
                UBaseType_t hwm = uxTaskGetStackHighWaterMark(*(stack_info[i].handle));
                uint32_t hwm_bytes = hwm * sizeof(StackType_t);
                uint32_t used = stack_info[i].total_stack - hwm_bytes;
                float pct = (float)used / stack_info[i].total_stack * 100.0f;

                stack_info[i].high_water_mark = hwm_bytes;
                stack_info[i].used_bytes = used;
                stack_info[i].usage_percent = pct;
                stack_info[i].sample_count++;

                if ((uint32_t)pct > stack_info[i].max_usage_percent) {
                    stack_info[i].max_usage_percent = (uint32_t)pct;
                }

                ESP_LOGI(TAG, "  [%s] Total: %lu, Used: %lu, Free: %lu (%.1f%% used, max %.0f%%)",
                         stack_info[i].name,
                         (unsigned long)stack_info[i].total_stack,
                         (unsigned long)used,
                         (unsigned long)hwm_bytes,
                         pct,
                         (float)stack_info[i].max_usage_percent);

                /* Status indicator */
                const char *status;
                if (pct >= 90) status = "CRITICAL";
                else if (pct >= STACK_WARN_PERCENT) status = "WARNING";
                else if (pct >= 50) status = "MODERATE";
                else status = "OK";

                printf("[DATA] STACK_REPORT,%s,%lu,%lu,%lu,%lu,%.1f,%s,%lu\n",
                       stack_info[i].name,
                       (unsigned long)get_elapsed_ms(),
                       (unsigned long)stack_info[i].total_stack,
                       (unsigned long)used,
                       (unsigned long)hwm_bytes,
                       pct,
                       status,
                       (unsigned long)stack_info[i].max_usage_percent);

                /* Check warning */
                if (pct > STACK_WARN_PERCENT && !stack_info[i].warning_issued) {
                    ESP_LOGW(TAG, "  ⚠ %s EXCEEDS %d%% threshold!",
                             stack_info[i].name, STACK_WARN_PERCENT);
                    stack_info[i].warning_issued = true;
                }
            }
        }

        /* Print tabel perbandingan visual (bar chart text) */
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "  Stack Usage Visualization:");
        for (int i = 0; i < 3; i++) {
            char bar[52];
            memset(bar, ' ', 51);
            bar[51] = '\0';

            int filled = (int)(stack_info[i].usage_percent / 2);
            if (filled > 50) filled = 50;
            for (int j = 0; j < filled; j++) {
                bar[j] = (j < 40) ? '#' : '!';
            }
            bar[filled] = '\0';

            ESP_LOGI(TAG, "  %-8s [%-50s] %.1f%%",
                     stack_info[i].name, bar, stack_info[i].usage_percent);
        }

        /*
         * REKOMENDASI SIZING STACK:
         *
         * Rule of thumb untuk ESP32:
         * 1. Mulai dengan 4096 bytes (cukup untuk kebanyakan task)
         * 2. Jalankan dengan beban maksimal
         * 3. Cek HWM - harus ada > 20% sisa
         * 4. Adjust: optimal_size = max_usage / 0.75
         *
         * Faktor yang mempengaruhi stack:
         * - Kedalaman call chain (termasuk library calls)
         * - Ukuran variabel lokal
         * - ISR nesting (jika task priority tertinggi)
         * - Printf/ESP_LOG menggunakan stack signifikan (~1.5KB)
         * - FreeRTOS internal context save (~200 bytes)
         */
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "  Recommended sizes (current_usage / 0.75):");
        for (int i = 0; i < 3; i++) {
            uint32_t recommended = (uint32_t)(stack_info[i].used_bytes / 0.75);
            /* Round up to nearest 256 */
            recommended = ((recommended + 255) / 256) * 256;
            ESP_LOGI(TAG, "  %-8s: Current=%lu, Recommend=%lu",
                     stack_info[i].name,
                     (unsigned long)stack_info[i].total_stack,
                     (unsigned long)recommended);
        }

        /* System info */
        printf("[DATA] SYSTEM,%lu,%lu,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)esp_get_free_heap_size(),
               (unsigned long)shallow_cycles,
               (unsigned long)medium_cycles,
               (unsigned long)deep_cycles);

        /* Heap monitoring */
        printf("[DATA] HEAP,%lu,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)esp_get_free_heap_size(),
               (unsigned long)esp_get_minimum_free_heap_size());

        ESP_LOGI(TAG, "================================================");

        /* Warning LED control */
        bool any_warning = false;
        for (int i = 0; i < 3; i++) {
            if (stack_info[i].usage_percent > STACK_WARN_PERCENT) {
                any_warning = true;
                break;
            }
        }
        gpio_set_level(LED1_GPIO, any_warning ? 1 : 0);

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
    ESP_LOGI(TAG, "  ESP32 FreeRTOS Stack Monitor Demo");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Task configurations:");
    ESP_LOGI(TAG, "  Shallow: stack=%d, depth=%d, buffer=%d",
             SMALL_STACK_SIZE, SHALLOW_DEPTH, SMALL_BUFFER_SIZE);
    ESP_LOGI(TAG, "  Medium:  stack=%d, depth=%d, buffer=%d",
             MEDIUM_STACK_SIZE, MEDIUM_DEPTH, MEDIUM_BUFFER_SIZE);
    ESP_LOGI(TAG, "  Deep:    stack=%d, depth=%d, buffer=%d",
             LARGE_STACK_SIZE, DEEP_DEPTH, LARGE_BUFFER_SIZE);
    ESP_LOGI(TAG, "Warning threshold: %d%%", STACK_WARN_PERCENT);

    printf("[DATA] INIT,%lu,%d,%d,%d,%d,%d,%d,%d\n",
           (unsigned long)get_elapsed_ms(),
           SMALL_STACK_SIZE, MEDIUM_STACK_SIZE, LARGE_STACK_SIZE,
           SHALLOW_DEPTH, MEDIUM_DEPTH, DEEP_DEPTH,
           STACK_WARN_PERCENT);

    init_gpio();

    /*
     * Membuat task dengan stack size berbeda.
     * Perhatikan bahwa pada ESP-IDF, usStackDepth dalam BYTES
     * (berbeda dengan vanilla FreeRTOS yang dalam WORDS).
     *
     * xTaskCreate mengalokasikan TCB + stack dari heap.
     * Total memory = sizeof(TCB) + usStackDepth
     * TCB ESP-IDF ≈ 350 bytes
     */
    ESP_LOGI(TAG, "Membuat tasks dengan stack berbeda...");

    /* Shallow task - stack kecil */
    xTaskCreate(shallow_task, "Shallow", SMALL_STACK_SIZE,
                NULL, SHALLOW_PRIORITY, &xShallowHandle);

    /* Medium task - stack menengah */
    xTaskCreate(medium_task, "Medium", MEDIUM_STACK_SIZE,
                NULL, MEDIUM_PRIORITY, &xMediumHandle);

    /* Deep task - stack besar */
    xTaskCreate(deep_task, "Deep", LARGE_STACK_SIZE,
                NULL, DEEP_PRIORITY, &xDeepHandle);

    /* Monitor task - stack cukup besar untuk printf */
    xTaskCreate(stack_monitor_task, "StackMon", LARGE_STACK_SIZE * 2,
                NULL, MONITOR_PRIORITY, &xMonitorHandle);

    ESP_LOGI(TAG, "Semua task dibuat. Monitoring stack usage...");
    ESP_LOGI(TAG, "============================================");
}
