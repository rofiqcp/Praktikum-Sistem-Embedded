/**
 * ============================================================================
 * PROGRAM 08: ESP32 Task Idle Hook
 * ============================================================================
 * Monitor idle task pada kedua core ESP32 untuk estimasi penggunaan CPU
 * 
 * Fitur utama:
 *   - esp_register_freertos_idle_hook_for_cpu() untuk hook per-core
 *   - Idle counter per core untuk kalkulasi CPU usage
 *   - Task beban variabel yang mengubah CPU usage seiring waktu
 *   - Monitoring free heap dari idle hook
 * 
 * ESP32 memiliki 2 idle task (satu per core). Idle hook dipanggil
 * setiap kali idle task berjalan, sehingga bisa digunakan sebagai
 * indikator seberapa sibuk CPU.
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
#include "esp_freertos_hooks.h"
#include "config.h"

/* Tag untuk logging */
static const char *TAG = "IDLE_HOOK";

/* ======================== VARIABEL GLOBAL ================================ */

/**
 * Counter idle per core
 * Diakses dari idle hook (ISR-like context) dan task monitor
 * Menggunakan volatile karena diubah dari konteks berbeda
 */
static volatile uint32_t idle_count_core0 = 0;  /* Counter idle Core 0 */
static volatile uint32_t idle_count_core1 = 0;  /* Counter idle Core 1 */

/* Snapshot counter untuk kalkulasi CPU usage */
static uint32_t prev_idle_core0 = 0;    /* Nilai sebelumnya Core 0 */
static uint32_t prev_idle_core1 = 0;    /* Nilai sebelumnya Core 1 */

/* Referensi idle count saat CPU 100% idle (kalibrasi) */
static uint32_t max_idle_core0 = 0;     /* Idle count maks Core 0 */
static uint32_t max_idle_core1 = 0;     /* Idle count maks Core 1 */

/* Flag kalibrasi */
static volatile bool calibration_done = false;

/* Histori CPU usage untuk trending */
static float cpu_history_core0[CPU_HISTORY_SIZE];
static float cpu_history_core1[CPU_HISTORY_SIZE];
static int history_index = 0;

/* Level beban saat ini */
static volatile int current_load_level = 0;
static volatile int current_load_percent = 0;

/* Handle task */
static TaskHandle_t load_task_handle = NULL;
static TaskHandle_t monitor_task_handle = NULL;

/* ======================== IDLE HOOKS ===================================== */

/**
 * Idle hook untuk Core 0
 * 
 * Fungsi ini dipanggil setiap kali idle task Core 0 berjalan.
 * PENTING: Fungsi ini berjalan dalam konteks idle task, jadi:
 *   - Tidak boleh blocking (tidak ada vTaskDelay, mutex, dll)
 *   - Harus cepat (tidak boleh komputasi berat)
 *   - Harus return true agar idle task bisa yield
 * 
 * @return true selalu (agar idle task bisa melakukan housekeeping)
 */
static bool idle_hook_core0(void)
{
    idle_count_core0++;
    return true;  /* Return true agar idle task bisa yield ke task lain */
}

/**
 * Idle hook untuk Core 1
 * Sama seperti idle hook Core 0, tetapi untuk Core 1
 * 
 * @return true selalu
 */
static bool idle_hook_core1(void)
{
    idle_count_core1++;
    return true;
}

/* ======================== FUNGSI UTILITAS ================================ */

/**
 * Inisialisasi GPIO untuk LED status
 */
static void init_gpio(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_STATUS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_STATUS_PIN, 0);
}

/**
 * Kalibrasi idle counter
 * 
 * Mengukur idle count saat tidak ada beban (100% idle) selama
 * interval tertentu. Nilai ini digunakan sebagai referensi
 * untuk menghitung persentase CPU usage.
 */
static void calibrate_idle_counter(void)
{
    ESP_LOGI(TAG, "Memulai kalibrasi idle counter (tidak ada beban)...");

    /* Reset counter */
    idle_count_core0 = 0;
    idle_count_core1 = 0;

    /* Tunggu selama interval pengukuran tanpa beban */
    vTaskDelay(pdMS_TO_TICKS(MEASUREMENT_INTERVAL_MS));

    /* Simpan nilai referensi (idle count saat 100% idle) */
    max_idle_core0 = idle_count_core0;
    max_idle_core1 = idle_count_core1;

    ESP_LOGI(TAG, "Kalibrasi selesai - Max idle Core0: %lu, Core1: %lu",
             max_idle_core0, max_idle_core1);

    printf("[DATA]CALIBRATION,max_idle_core0=%lu,max_idle_core1=%lu\n",
           max_idle_core0, max_idle_core1);

    calibration_done = true;
}

/**
 * Hitung CPU usage berdasarkan idle counter
 * 
 * Formula: CPU_Usage = 100% - (idle_count / max_idle_count * 100%)
 * Jika idle_count == max_idle_count, CPU usage = 0% (full idle)
 * Jika idle_count == 0, CPU usage = 100% (full load)
 * 
 * @param current_idle Counter idle saat ini
 * @param prev_idle Counter idle sebelumnya
 * @param max_idle Counter idle referensi (saat 100% idle)
 * @return Persentase CPU usage (0-100)
 */
static float calculate_cpu_usage(uint32_t current_idle, uint32_t prev_idle, uint32_t max_idle)
{
    if (max_idle == 0) return 0.0f;

    uint32_t delta_idle = current_idle - prev_idle;
    float idle_ratio = (float)delta_idle / (float)max_idle;

    /* Clamp ke range 0-100% */
    float cpu_usage = (1.0f - idle_ratio) * 100.0f;
    if (cpu_usage < 0.0f) cpu_usage = 0.0f;
    if (cpu_usage > 100.0f) cpu_usage = 100.0f;

    return cpu_usage;
}

/* ======================== TASK FUNCTIONS ================================= */

/**
 * Task pemberi beban variabel
 * 
 * Task ini mengubah level beban CPU secara siklis untuk mendemonstrasikan
 * bagaimana idle hook bisa mendeteksi perubahan CPU usage. Beban diberikan
 * dengan busy-wait loop yang mengonsumsi waktu CPU.
 * 
 * Level beban berputar: 0% → 25% → 50% → 75% → 95% → 0% → ...
 * 
 * @param pvParameters Tidak digunakan
 */
static void variable_load_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task beban variabel dimulai pada Core %d", xPortGetCoreID());

    /* Array level beban */
    const int load_levels[] = {
        LOAD_LEVEL_0, LOAD_LEVEL_1, LOAD_LEVEL_2,
        LOAD_LEVEL_3, LOAD_LEVEL_4
    };
    int level_index = 0;

    /* Tunggu kalibrasi selesai */
    while (!calibration_done) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    TickType_t phase_start = xTaskGetTickCount();

    while (1) {
        /* Ganti level beban setiap LOAD_PHASE_DURATION_MS */
        TickType_t now = xTaskGetTickCount();
        if ((now - phase_start) * portTICK_PERIOD_MS >= LOAD_PHASE_DURATION_MS) {
            level_index = (level_index + 1) % LOAD_LEVEL_COUNT;
            current_load_level = level_index;
            current_load_percent = load_levels[level_index];
            phase_start = now;

            printf("[DATA]LOAD_CHANGE,level=%d,percent=%d\n",
                   level_index, load_levels[level_index]);
        }

        int load_pct = load_levels[level_index];

        if (load_pct > 0) {
            /**
             * Simulasi beban CPU dengan busy-wait
             * Rasio busy/idle menentukan persentase CPU usage
             * 
             * Contoh: load_pct=50 → 50% waktu busy, 50% waktu idle
             */
            int busy_ticks = load_pct;
            int idle_ticks = 100 - load_pct;

            /* Fase busy: komputasi intensif */
            for (int i = 0; i < busy_ticks; i++) {
                volatile float result = 1.0f;
                for (int j = 0; j < BUSY_LOOP_ITERATIONS; j++) {
                    result *= 1.00001f;
                }
            }

            /* Fase idle: biarkan CPU istirahat */
            if (idle_ticks > 0) {
                vTaskDelay(pdMS_TO_TICKS(idle_ticks / 10));
            }
        } else {
            /* Tanpa beban - yield sepenuhnya */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/**
 * Task monitor - menghitung dan mencetak CPU usage periodik
 * 
 * Membaca idle counter secara periodik, menghitung delta dari
 * pembacaan sebelumnya, dan mengkonversi ke persentase CPU usage
 * menggunakan nilai kalibrasi sebagai referensi.
 * 
 * @param pvParameters Tidak digunakan
 */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor dimulai pada Core %d", xPortGetCoreID());

    /* Kalibrasi dulu sebelum mulai monitoring */
    calibrate_idle_counter();

    /* Inisialisasi histori */
    memset(cpu_history_core0, 0, sizeof(cpu_history_core0));
    memset(cpu_history_core1, 0, sizeof(cpu_history_core1));

    uint32_t cycle = 0;

    while (1) {
        cycle++;

        /* Ambil snapshot counter idle saat ini */
        uint32_t current_idle0 = idle_count_core0;
        uint32_t current_idle1 = idle_count_core1;

        /* Hitung CPU usage berdasarkan delta idle count */
        float cpu0 = calculate_cpu_usage(current_idle0, prev_idle_core0, max_idle_core0);
        float cpu1 = calculate_cpu_usage(current_idle1, prev_idle_core1, max_idle_core1);

        /* Simpan snapshot untuk pengukuran berikutnya */
        prev_idle_core0 = current_idle0;
        prev_idle_core1 = current_idle1;

        /* Reset counter untuk mencegah overflow */
        if (current_idle0 > 4000000000UL) {
            idle_count_core0 = 0;
            prev_idle_core0 = 0;
        }
        if (current_idle1 > 4000000000UL) {
            idle_count_core1 = 0;
            prev_idle_core1 = 0;
        }

        /* Simpan ke histori */
        cpu_history_core0[history_index] = cpu0;
        cpu_history_core1[history_index] = cpu1;
        history_index = (history_index + 1) % CPU_HISTORY_SIZE;

        /* Hitung rata-rata dari histori */
        float avg_cpu0 = 0, avg_cpu1 = 0;
        int valid_samples = 0;
        for (int i = 0; i < CPU_HISTORY_SIZE; i++) {
            if (cpu_history_core0[i] > 0 || cpu_history_core1[i] > 0) {
                avg_cpu0 += cpu_history_core0[i];
                avg_cpu1 += cpu_history_core1[i];
                valid_samples++;
            }
        }
        if (valid_samples > 0) {
            avg_cpu0 /= valid_samples;
            avg_cpu1 /= valid_samples;
        }

        /* Cetak laporan */
        printf("\n========================================\n");
        printf("  CPU USAGE MONITOR - Siklus #%lu\n", cycle);
        printf("========================================\n");

        /* Data untuk serial parser */
        printf("[DATA]CPU_USAGE,core0=%.1f,core1=%.1f,avg0=%.1f,avg1=%.1f\n",
               cpu0, cpu1, avg_cpu0, avg_cpu1);

        printf("[DATA]IDLE_COUNT,core0=%lu,core1=%lu\n",
               current_idle0, current_idle1);

        printf("[DATA]LOAD_INFO,level=%d,target_percent=%d\n",
               current_load_level, current_load_percent);

        /* Informasi heap */
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_heap = esp_get_minimum_free_heap_size();
        printf("[DATA]HEAP,free=%lu,min=%lu\n", free_heap, min_heap);

        /* Status LED berdasarkan CPU usage rata-rata */
        float total_cpu = (cpu0 + cpu1) / 2.0f;
        if (total_cpu > 80.0f) {
            /* Kedip cepat - beban tinggi */
            for (int i = 0; i < 5; i++) {
                gpio_set_level(LED_STATUS_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(50));
                gpio_set_level(LED_STATUS_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        } else if (total_cpu > 40.0f) {
            /* Kedip sedang */
            gpio_set_level(LED_STATUS_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_STATUS_PIN, 0);
        } else {
            /* LED mati - idle */
            gpio_set_level(LED_STATUS_PIN, 0);
        }

        /* Cetak visual bar CPU usage */
        printf("[DATA]BAR_CORE0,");
        int bar_len0 = (int)(cpu0 / 5.0f);
        for (int i = 0; i < 20; i++) {
            printf("%c", i < bar_len0 ? '#' : '.');
        }
        printf(",%.1f%%\n", cpu0);

        printf("[DATA]BAR_CORE1,");
        int bar_len1 = (int)(cpu1 / 5.0f);
        for (int i = 0; i < 20; i++) {
            printf("%c", i < bar_len1 ? '#' : '.');
        }
        printf(",%.1f%%\n", cpu1);

        /* Uptime */
        printf("[DATA]UPTIME,ms=%lld\n", esp_timer_get_time() / 1000);

        vTaskDelay(pdMS_TO_TICKS(MEASUREMENT_INTERVAL_MS));
    }
}

/* ======================== ENTRY POINT ==================================== */

/**
 * Fungsi utama aplikasi
 * 
 * Urutan:
 *   1. Inisialisasi GPIO
 *   2. Registrasi idle hook untuk kedua core
 *   3. Buat task monitor (melakukan kalibrasi)
 *   4. Buat task pemberi beban variabel
 */
void app_main(void)
{
    /* Cetak banner program */
    printf("\n");
    printf("============================================================\n");
    printf("  ESP32 FreeRTOS - Task Idle Hook & CPU Usage Monitor\n");
    printf("  Monitoring Dual-Core CPU Utilization\n");
    printf("============================================================\n");
    printf("  Platform: ESP32 (%d core)\n", portNUM_PROCESSORS);
    printf("  Free Heap: %lu bytes\n", (uint32_t)esp_get_free_heap_size());
    printf("  app_main() berjalan di Core %d\n", xPortGetCoreID());
    printf("============================================================\n\n");

    /* 1. Inisialisasi GPIO */
    ESP_LOGI(TAG, "Inisialisasi GPIO...");
    init_gpio();

    /* 2. Registrasi idle hook untuk Core 0 */
    ESP_LOGI(TAG, "Registrasi idle hook Core 0...");
    esp_err_t ret = esp_register_freertos_idle_hook_for_cpu(idle_hook_core0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal registrasi idle hook Core 0: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Idle hook Core 0 berhasil diregistrasi");
    }

    /* 3. Registrasi idle hook untuk Core 1 */
    ESP_LOGI(TAG, "Registrasi idle hook Core 1...");
    ret = esp_register_freertos_idle_hook_for_cpu(idle_hook_core1, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Gagal registrasi idle hook Core 1: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Idle hook Core 1 berhasil diregistrasi");
    }

    printf("[DATA]HOOKS,core0=registered,core1=registered\n");

    /* 4. Buat task monitor (pinned ke Core 0 untuk mengukur Core 1 secara akurat) */
    ESP_LOGI(TAG, "Membuat task monitor...");
    xTaskCreatePinnedToCore(
        monitor_task,
        "MonitorTask",
        MONITOR_TASK_STACK,
        NULL,
        MONITOR_TASK_PRIORITY,
        &monitor_task_handle,
        0  /* Pin ke Core 0 */
    );

    /* 5. Buat task beban variabel (pinned ke Core 1 untuk demo) */
    ESP_LOGI(TAG, "Membuat task beban variabel...");
    xTaskCreatePinnedToCore(
        variable_load_task,
        "LoadTask",
        LOAD_TASK_STACK,
        NULL,
        LOAD_TASK_PRIORITY,
        &load_task_handle,
        1  /* Pin ke Core 1 */
    );

    ESP_LOGI(TAG, "Semua task dimulai. Monitoring CPU usage...");
    printf("[DATA]STATUS,system=started\n");
}
