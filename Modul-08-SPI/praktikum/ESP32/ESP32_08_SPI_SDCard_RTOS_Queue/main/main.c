#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "config.h"

static const char *TAG = "SD_RTOS_QUEUE";

#define MOUNT_POINT "/sdcard"

// Queue message structures
typedef struct {
    uint8_t operation;
    char filename[64];
    char data[256];
    uint32_t timestamp;
} sd_operation_t;

typedef struct {
    uint8_t status;
    char message[128];
    uint32_t bytes_processed;
} sd_result_t;

typedef struct {
    uint8_t level;
    char message[256];
    uint32_t timestamp;
} log_message_t;

// Global variables
static QueueHandle_t write_queue;
static QueueHandle_t read_queue;
static QueueHandle_t log_queue;
static SemaphoreHandle_t sd_mutex;
static sdmmc_card_t *card;
static bool sd_mounted = false;

// SD Card initialization
esp_err_t sd_card_init(void) {
    esp_err_t ret;
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus");
        return ret;
    }
    
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;
    
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    sdmmc_card_print_info(stdout, card);
    sd_mounted = true;
    
    return ESP_OK;
}

// Log helper function
void send_log(uint8_t level, const char *format, ...) {
    log_message_t log_msg;
    log_msg.level = level;
    log_msg.timestamp = xTaskGetTickCount();
    
    va_list args;
    va_start(args, format);
    vsnprintf(log_msg.message, sizeof(log_msg.message), format, args);
    va_end(args);
    
    xQueueSend(log_queue, &log_msg, 0);
}

// Writer Task - Processes write operations from queue
void writer_task(void *pvParameters) {
    ESP_LOGI(TAG, "Writer Task started");
    sd_operation_t operation;
    
    while (1) {
        if (xQueueReceive(write_queue, &operation, portMAX_DELAY) == pdTRUE) {
            if (!sd_mounted) {
                send_log(1, "SD card not mounted");
                continue;
            }
            
            if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                char filepath[128];
                snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, operation.filename);
                
                if (operation.operation == OP_WRITE) {
                    FILE *f = fopen(filepath, "a");
                    if (f == NULL) {
                        send_log(1, "Failed to open file: %s", operation.filename);
                    } else {
                        size_t written = fwrite(operation.data, 1, strlen(operation.data), f);
                        fclose(f);
                        send_log(0, "Written %d bytes to %s", written, operation.filename);
                    }
                } else if (operation.operation == OP_DELETE) {
                    if (unlink(filepath) == 0) {
                        send_log(0, "Deleted file: %s", operation.filename);
                    } else {
                        send_log(1, "Failed to delete: %s", operation.filename);
                    }
                }
                
                xSemaphoreGive(sd_mutex);
            } else {
                send_log(1, "Failed to acquire SD mutex");
            }
        }
    }
}

// Reader Task - Processes read operations from queue
void reader_task(void *pvParameters) {
    ESP_LOGI(TAG, "Reader Task started");
    sd_operation_t operation;
    
    while (1) {
        if (xQueueReceive(read_queue, &operation, portMAX_DELAY) == pdTRUE) {
            if (!sd_mounted) {
                send_log(1, "SD card not mounted");
                continue;
            }
            
            if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                char filepath[128];
                snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, operation.filename);
                
                if (operation.operation == OP_READ) {
                    FILE *f = fopen(filepath, "r");
                    if (f == NULL) {
                        send_log(1, "Failed to open file: %s", operation.filename);
                    } else {
                        char line[256];
                        int line_count = 0;
                        while (fgets(line, sizeof(line), f) != NULL && line_count < 5) {
                            send_log(0, "Read: %s", line);
                            line_count++;
                        }
                        fclose(f);
                        send_log(0, "Read %d lines from %s", line_count, operation.filename);
                    }
                } else if (operation.operation == OP_LIST) {
                    struct stat st;
                    if (stat(MOUNT_POINT, &st) == 0) {
                        send_log(0, "SD card mounted at %s", MOUNT_POINT);
                    }
                }
                
                xSemaphoreGive(sd_mutex);
            } else {
                send_log(1, "Failed to acquire SD mutex");
            }
        }
    }
}

// Logger Task - Processes log messages from queue
void logger_task(void *pvParameters) {
    ESP_LOGI(TAG, "Logger Task started");
    log_message_t log_msg;
    
    while (1) {
        if (xQueueReceive(log_queue, &log_msg, portMAX_DELAY) == pdTRUE) {
            const char *level_str[] = {"INFO", "WARN", "ERROR"};
            printf("[%lu] [%s] %s\n", log_msg.timestamp, 
                   level_str[log_msg.level], log_msg.message);
        }
    }
}

// Monitor Task - Generates periodic operations
void monitor_task(void *pvParameters) {
    ESP_LOGI(TAG, "Monitor Task started");
    
    uint32_t counter = 0;
    sd_operation_t operation;
    
    while (1) {
        // Generate write operation
        operation.operation = OP_WRITE;
        snprintf(operation.filename, sizeof(operation.filename), "data.txt");
        snprintf(operation.data, sizeof(operation.data), 
                 "Sample data %lu at %lu\n", counter, xTaskGetTickCount());
        operation.timestamp = xTaskGetTickCount();
        
        if (xQueueSend(write_queue, &operation, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "Write queue full");
        }
        
        // Every 5 iterations, read the file
        if (counter % 5 == 0) {
            operation.operation = OP_READ;
            snprintf(operation.filename, sizeof(operation.filename), "data.txt");
            
            if (xQueueSend(read_queue, &operation, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "Read queue full");
            }
        }
        
        // Every 10 iterations, list files
        if (counter % 10 == 0) {
            operation.operation = OP_LIST;
            if (xQueueSend(read_queue, &operation, pdMS_TO_TICKS(100)) != pdTRUE) {
                ESP_LOGW(TAG, "Read queue full");
            }
        }
        
        counter++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SD Card RTOS Queue Demo");
    
    // Create queues
    write_queue = xQueueCreate(WRITE_QUEUE_SIZE, sizeof(sd_operation_t));
    read_queue = xQueueCreate(READ_QUEUE_SIZE, sizeof(sd_operation_t));
    log_queue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(log_message_t));
    
    if (write_queue == NULL || read_queue == NULL || log_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }
    
    // Create mutex
    sd_mutex = xSemaphoreCreateMutex();
    if (sd_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
    
    // Initialize SD card
    esp_err_t ret = sd_card_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card initialization failed");
    } else {
        ESP_LOGI(TAG, "SD card initialized successfully");
    }
    
    // Create tasks
    xTaskCreate(writer_task, "Writer", TASK_STACK_SIZE, NULL, 
                WRITER_TASK_PRIORITY, NULL);
    
    xTaskCreate(reader_task, "Reader", TASK_STACK_SIZE, NULL, 
                READER_TASK_PRIORITY, NULL);
    
    xTaskCreate(logger_task, "Logger", TASK_STACK_SIZE, NULL, 
                LOGGER_TASK_PRIORITY, NULL);
    
    xTaskCreate(monitor_task, "Monitor", TASK_STACK_SIZE, NULL, 
                MONITOR_TASK_PRIORITY, NULL);
    
    ESP_LOGI(TAG, "All tasks created successfully");
    
    // Send initial log
    send_log(0, "System started");
}
