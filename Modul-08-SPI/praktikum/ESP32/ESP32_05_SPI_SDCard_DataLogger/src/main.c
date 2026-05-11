#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "config.h"

static const char *TAG = "SDCard_Logger";

sdmmc_card_t *card;
static uint32_t log_count = 0;

float simulate_temperature(void) {
    return TEMP_MIN + ((float)((uint32_t)rand()) / UINT32_MAX) * (TEMP_MAX - TEMP_MIN);
}

float simulate_humidity(void) {
    return HUMIDITY_MIN + ((float)((uint32_t)rand()) / UINT32_MAX) * (HUMIDITY_MAX - HUMIDITY_MIN);
}

esp_err_t init_sd_card(void) {
    ESP_LOGI(TAG, "Initializing SD card");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
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

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

esp_err_t write_log_header(void) {
    FILE *f = fopen(LOG_FILE, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    fprintf(f, "Timestamp,Count,Temperature,Humidity\n");
    fclose(f);
    ESP_LOGI(TAG, "Log header written");
    return ESP_OK;
}

esp_err_t append_log_data(float temp, float humidity) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending");
        return ESP_FAIL;
    }

    uint32_t timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
    fprintf(f, "%lu,%lu,%.2f,%.2f\n", timestamp, log_count++, temp, humidity);
    fclose(f);

    ESP_LOGI(TAG, "Data logged: T=%.2f°C, H=%.2f%%, Count=%lu", temp, humidity, log_count);
    return ESP_OK;
}

esp_err_t check_log_size(void) {
    struct stat st;
    if (stat(LOG_FILE, &st) == 0) {
        if (st.st_size > MAX_LOG_SIZE) {
            ESP_LOGW(TAG, "Log file size exceeded, creating backup");
            rename(LOG_FILE, "/sdcard/datalog_backup.csv");
            write_log_header();
            log_count = 0;
        }
    }
    return ESP_OK;
}

void data_logger_task(void *pvParameters) {
    while (1) {
        float temp = simulate_temperature();
        float humidity = simulate_humidity();

        if (append_log_data(temp, humidity) == ESP_OK) {
            check_log_size();
        }

        vTaskDelay(pdMS_TO_TICKS(LOG_INTERVAL_MS));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting SD Card Data Logger");

    if (init_sd_card() == ESP_OK) {
        write_log_header();
        xTaskCreate(data_logger_task, "logger", 4096, NULL, 5, NULL);
        ESP_LOGI(TAG, "Data logging started");
    } else {
        ESP_LOGE(TAG, "SD Card initialization failed");
    }
}
