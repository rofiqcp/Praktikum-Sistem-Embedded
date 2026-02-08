/**
 * ===========================================================================
 *  Program 12 : Storage Data Logger
 *  Board      : ESP32 DevKit V1
 *  Framework  : ESP-IDF
 * ===========================================================================
 *
 *  Deskripsi:
 *  Program ini mengimplementasikan data logger yang menyimpan data sensor
 *  (simulasi) ke flash internal ESP32 menggunakan SPIFFS. Menggunakan
 *  circular buffer di RAM dan FreeRTOS tasks untuk pengumpulan data
 *  dan penulisan file secara terpisah.
 *
 *  Fitur:
 *  1. Inisialisasi SPIFFS filesystem
 *  2. File CSV dengan header
 *  3. Simulasi data sensor:
 *     - Temperature: gelombang sinus
 *     - Humidity: random dalam range
 *     - ADC: gelombang sawtooth
 *  4. Circular buffer (ring buffer) di RAM
 *  5. Flush ke file setiap N entries
 *  6. File size limit + backup rotation
 *  7. Read & print entri terakhir dari file
 *  8. Statistik penyimpanan
 *  9. FreeRTOS dual-task: collector + writer
 *
 *  Koneksi Hardware:
 *  - Tidak ada koneksi eksternal (data simulasi, flash internal)
 *
 * ===========================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "esp_random.h"

#include "config.h"

static const char *TAG = "DATALOG";

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ==================== Circular Buffer ==================== */
typedef struct {
    log_entry_t entries[CIRCULAR_BUFFER_SIZE];
    volatile int head;            // Write position
    volatile int tail;            // Read position
    volatile int count;           // Current number of entries
    SemaphoreHandle_t mutex;      // Thread safety
} circular_buffer_t;

static circular_buffer_t ring_buffer;

/* ==================== Statistics ==================== */
typedef struct {
    uint32_t entries_collected;
    uint32_t entries_written;
    uint32_t flushes;
    uint32_t file_rotations;
    int64_t  start_time_us;
} logger_stats_t;

static logger_stats_t stats = {0};

/* ==================== Synchronization ==================== */
static SemaphoreHandle_t flush_semaphore;   // Signal writer to flush
static volatile bool logger_running = true;

/* ==================== Circular Buffer Operations ==================== */

static void cb_init(circular_buffer_t *cb)
{
    cb->head  = 0;
    cb->tail  = 0;
    cb->count = 0;
    cb->mutex = xSemaphoreCreateMutex();
    configASSERT(cb->mutex != NULL);
}

static bool cb_push(circular_buffer_t *cb, const log_entry_t *entry)
{
    if (xSemaphoreTake(cb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    if (cb->count >= CIRCULAR_BUFFER_SIZE) {
        // Overwrite oldest entry
        cb->tail = (cb->tail + 1) % CIRCULAR_BUFFER_SIZE;
        cb->count--;
        ESP_LOGW(TAG, "Ring buffer overflow! Oldest entry overwritten.");
    }

    memcpy(&cb->entries[cb->head], entry, sizeof(log_entry_t));
    cb->head = (cb->head + 1) % CIRCULAR_BUFFER_SIZE;
    cb->count++;

    xSemaphoreGive(cb->mutex);
    return true;
}

static bool cb_pop(circular_buffer_t *cb, log_entry_t *entry)
{
    if (xSemaphoreTake(cb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    if (cb->count == 0) {
        xSemaphoreGive(cb->mutex);
        return false;
    }

    memcpy(entry, &cb->entries[cb->tail], sizeof(log_entry_t));
    cb->tail = (cb->tail + 1) % CIRCULAR_BUFFER_SIZE;
    cb->count--;

    xSemaphoreGive(cb->mutex);
    return true;
}

static int cb_count(circular_buffer_t *cb)
{
    int count;
    if (xSemaphoreTake(cb->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }
    count = cb->count;
    xSemaphoreGive(cb->mutex);
    return count;
}

/* ==================== SPIFFS Operations ==================== */

static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path              = SPIFFS_BASE_PATH,
        .partition_label        = SPIFFS_PARTITION_LABEL,
        .max_files              = SPIFFS_MAX_FILES,
        .format_if_mount_failed = SPIFFS_FORMAT_IF_FAILED,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPIFFS mounted at '%s'", SPIFFS_BASE_PATH);
    return ESP_OK;
}

static void print_storage_stats(void)
{
    size_t total = 0, used = 0;
    if (esp_spiffs_info(SPIFFS_PARTITION_LABEL, &total, &used) == ESP_OK) {
        uint32_t uptime_s = (uint32_t)((esp_timer_get_time() - stats.start_time_us) / 1000000);
        ESP_LOGI(TAG, "=== Storage Statistics ===");
        ESP_LOGI(TAG, "  Total space : %d bytes (%d KB)", total, total / 1024);
        ESP_LOGI(TAG, "  Used space  : %d bytes (%d KB)", used, used / 1024);
        ESP_LOGI(TAG, "  Free space  : %d bytes (%d KB)", total - used, (total - used) / 1024);
        ESP_LOGI(TAG, "  Usage       : %.1f%%", (float)used / total * 100.0f);
        ESP_LOGI(TAG, "  Entries logged  : %lu", (unsigned long)stats.entries_written);
        ESP_LOGI(TAG, "  Flushes         : %lu", (unsigned long)stats.flushes);
        ESP_LOGI(TAG, "  File rotations  : %lu", (unsigned long)stats.file_rotations);
        ESP_LOGI(TAG, "  Uptime          : %lus", (unsigned long)uptime_s);
        if (uptime_s > 0) {
            ESP_LOGI(TAG, "  Data rate       : %.1f entries/min",
                     (float)stats.entries_written / ((float)uptime_s / 60.0f));
        }
        ESP_LOGI(TAG, "  Buffer entries  : %d / %d", cb_count(&ring_buffer), CIRCULAR_BUFFER_SIZE);
        ESP_LOGI(TAG, "=========================");
    }
}

static long get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

static void rotate_log_file(void)
{
    ESP_LOGW(TAG, "Log file reached max size (%d KB). Rotating...", MAX_FILE_SIZE / 1024);

    // Delete old backup if exists
    struct stat st;
    if (stat(LOG_FILE_BACKUP, &st) == 0) {
        unlink(LOG_FILE_BACKUP);
    }

    // Rename current to backup
    if (rename(LOG_FILE_PATH, LOG_FILE_BACKUP) == 0) {
        ESP_LOGI(TAG, "Renamed %s -> %s", LOG_FILE_PATH, LOG_FILE_BACKUP);
    } else {
        ESP_LOGE(TAG, "Failed to rename log file: %s", strerror(errno));
        // Delete and start fresh
        unlink(LOG_FILE_PATH);
    }

    // Create new file with header
    FILE *f = fopen(LOG_FILE_PATH, "w");
    if (f) {
        fprintf(f, LOG_FILE_HEADER);
        fclose(f);
        ESP_LOGI(TAG, "New log file created with header");
    }

    stats.file_rotations++;
}

static void create_log_file_if_needed(void)
{
    struct stat st;
    if (stat(LOG_FILE_PATH, &st) != 0) {
        // File doesn't exist, create with header
        FILE *f = fopen(LOG_FILE_PATH, "w");
        if (f) {
            fprintf(f, LOG_FILE_HEADER);
            fclose(f);
            ESP_LOGI(TAG, "Created log file: %s", LOG_FILE_PATH);
        } else {
            ESP_LOGE(TAG, "Failed to create log file: %s", strerror(errno));
        }
    } else {
        ESP_LOGI(TAG, "Log file exists: %s (%ld bytes)", LOG_FILE_PATH, (long)st.st_size);
    }
}

static void read_last_n_entries(int n)
{
    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "Cannot open log file for reading");
        return;
    }

    // Count total lines
    int total_lines = 0;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        total_lines++;
    }

    // Subtract header
    int data_lines = total_lines - 1;
    int skip = (data_lines > n) ? (data_lines - n) : 0;

    rewind(f);
    fgets(line, sizeof(line), f); // Skip header

    int current = 0;
    ESP_LOGI(TAG, "--- Last %d entries (of %d total) ---", 
             (data_lines < n ? data_lines : n), data_lines);
    ESP_LOGI(TAG, "  %-12s %-10s %-10s %-10s", "Timestamp", "Temp(°C)", "Hum(%)", "ADC");

    while (fgets(line, sizeof(line), f)) {
        if (current >= skip) {
            uint32_t ts;
            float temp, hum;
            uint16_t adc;
            if (sscanf(line, "%lu,%f,%f,%hu", (unsigned long *)&ts, &temp, &hum, &adc) == 4) {
                ESP_LOGI(TAG, "  %-12lu %-10.2f %-10.2f %-10u",
                         (unsigned long)ts, temp, hum, adc);
            }
        }
        current++;
    }

    fclose(f);
}

/* ==================== Sensor Data Simulation ==================== */

static log_entry_t simulate_sensor_data(void)
{
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    log_entry_t entry;

    entry.timestamp = now_ms;

    // Temperature: sine wave
    float phase = (2.0f * M_PI * (float)now_ms) / (float)TEMP_PERIOD_MS;
    entry.temperature = TEMP_BASE + TEMP_AMPLITUDE * sinf(phase);

    // Humidity: random within range
    uint32_t rand_val = esp_random();
    float rand_frac = (float)(rand_val % 10000) / 10000.0f;
    entry.humidity = HUMIDITY_MIN + rand_frac * (HUMIDITY_MAX - HUMIDITY_MIN);

    // ADC: sawtooth wave
    uint32_t saw_pos = now_ms % ADC_SAW_PERIOD_MS;
    entry.adc_value = (uint16_t)((uint32_t)saw_pos * ADC_MAX_VALUE / ADC_SAW_PERIOD_MS);

    return entry;
}

/* ==================== FreeRTOS Tasks ==================== */

/**
 * @brief Data collector task: periodically samples sensor data and pushes to ring buffer
 */
static void data_collector_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[Collector] Task started (interval: %d ms)", LOG_INTERVAL_MS);
    TickType_t last_wake = xTaskGetTickCount();

    while (logger_running) {
        // Simulate sensor reading
        log_entry_t entry = simulate_sensor_data();

        // Push to circular buffer
        if (cb_push(&ring_buffer, &entry)) {
            stats.entries_collected++;

            if (stats.entries_collected % 10 == 0) {
                ESP_LOGI(TAG, "[Collector] Sample #%lu: T=%.1f°C, H=%.1f%%, ADC=%u (buf: %d/%d)",
                         (unsigned long)stats.entries_collected,
                         entry.temperature, entry.humidity, entry.adc_value,
                         cb_count(&ring_buffer), CIRCULAR_BUFFER_SIZE);
            }

            // Signal writer if buffer has enough entries
            if (cb_count(&ring_buffer) >= FLUSH_THRESHOLD) {
                xSemaphoreGive(flush_semaphore);
            }
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(LOG_INTERVAL_MS));
    }

    ESP_LOGI(TAG, "[Collector] Task ended. Total collected: %lu",
             (unsigned long)stats.entries_collected);
    vTaskDelete(NULL);
}

/**
 * @brief File writer task: flushes ring buffer to CSV file
 */
static void file_writer_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[Writer] Task started (flush threshold: %d entries)", FLUSH_THRESHOLD);
    TickType_t last_stats = xTaskGetTickCount();

    while (logger_running) {
        // Wait for signal or timeout
        if (xSemaphoreTake(flush_semaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
            // Check file size before writing
            long fsize = get_file_size(LOG_FILE_PATH);
            if (fsize >= MAX_FILE_SIZE) {
                rotate_log_file();
            }

            // Flush all entries from ring buffer to file
            FILE *f = fopen(LOG_FILE_PATH, "a");
            if (f) {
                int flushed = 0;
                log_entry_t entry;

                while (cb_pop(&ring_buffer, &entry)) {
                    fprintf(f, "%lu,%.2f,%.2f,%u\n",
                            (unsigned long)entry.timestamp,
                            entry.temperature,
                            entry.humidity,
                            entry.adc_value);
                    flushed++;
                    stats.entries_written++;
                }

                fclose(f);
                stats.flushes++;

                if (flushed > 0) {
                    long new_size = get_file_size(LOG_FILE_PATH);
                    ESP_LOGI(TAG, "[Writer] Flushed %d entries (file: %ld bytes)",
                             flushed, new_size);
                }
            } else {
                ESP_LOGE(TAG, "[Writer] Failed to open log file: %s", strerror(errno));
            }
        }

        // Periodic statistics
        if ((xTaskGetTickCount() - last_stats) * portTICK_PERIOD_MS >= STATS_INTERVAL_MS) {
            last_stats = xTaskGetTickCount();
            print_storage_stats();
            read_last_n_entries(PRINT_LAST_N_ENTRIES);
        }
    }

    // Final flush
    ESP_LOGI(TAG, "[Writer] Final flush...");
    FILE *f = fopen(LOG_FILE_PATH, "a");
    if (f) {
        log_entry_t entry;
        int flushed = 0;
        while (cb_pop(&ring_buffer, &entry)) {
            fprintf(f, "%lu,%.2f,%.2f,%u\n",
                    (unsigned long)entry.timestamp,
                    entry.temperature,
                    entry.humidity,
                    entry.adc_value);
            flushed++;
            stats.entries_written++;
        }
        fclose(f);
        ESP_LOGI(TAG, "[Writer] Final flush: %d entries", flushed);
    }

    ESP_LOGI(TAG, "[Writer] Task ended. Total written: %lu",
             (unsigned long)stats.entries_written);
    vTaskDelete(NULL);
}

/* ==================== Main Entry Point ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Program 12: Storage Data Logger         ║");
    ESP_LOGI(TAG, "║   Modul 07 - SPI & Storage               ║");
    ESP_LOGI(TAG, "║   Framework: ESP-IDF                      ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "Configuration:");
    ESP_LOGI(TAG, "  Log file        : %s", LOG_FILE_PATH);
    ESP_LOGI(TAG, "  Max file size   : %d KB", MAX_FILE_SIZE / 1024);
    ESP_LOGI(TAG, "  Log interval    : %d ms", LOG_INTERVAL_MS);
    ESP_LOGI(TAG, "  Buffer size     : %d entries", CIRCULAR_BUFFER_SIZE);
    ESP_LOGI(TAG, "  Flush threshold : %d entries", FLUSH_THRESHOLD);
    ESP_LOGI(TAG, "");

    // Record start time
    stats.start_time_us = esp_timer_get_time();

    // Initialize SPIFFS
    if (init_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS init failed! Cannot proceed.");
        return;
    }

    // Print initial storage info
    print_storage_stats();

    // Create log file with header if needed
    create_log_file_if_needed();

    // Initialize circular buffer
    cb_init(&ring_buffer);
    ESP_LOGI(TAG, "Circular buffer initialized (capacity: %d entries, entry size: %d bytes)",
             CIRCULAR_BUFFER_SIZE, sizeof(log_entry_t));

    // Create flush semaphore
    flush_semaphore = xSemaphoreCreateBinary();
    configASSERT(flush_semaphore != NULL);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Starting data logger tasks...");
    ESP_LOGI(TAG, "  Simulated sensors: Temperature (sine), Humidity (random), ADC (sawtooth)");
    ESP_LOGI(TAG, "");

    // Create FreeRTOS tasks
    BaseType_t ret;

    ret = xTaskCreate(data_collector_task, "collector",
                      COLLECTOR_TASK_STACK, NULL,
                      COLLECTOR_TASK_PRIORITY, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create collector task!");
        return;
    }

    ret = xTaskCreate(file_writer_task, "writer",
                      WRITER_TASK_STACK, NULL,
                      WRITER_TASK_PRIORITY, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create writer task!");
        return;
    }

    ESP_LOGI(TAG, "Data logger running. Tasks created successfully.");
    ESP_LOGI(TAG, "Logging will continue indefinitely. Monitor serial for updates.");
    ESP_LOGI(TAG, "Statistics printed every %d seconds.", STATS_INTERVAL_MS / 1000);

    // app_main returns, tasks continue running
}
