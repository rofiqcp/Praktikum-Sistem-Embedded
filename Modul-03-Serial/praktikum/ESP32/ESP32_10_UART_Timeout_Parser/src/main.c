/**
 * @file main.c
 * @brief UART Timeout-based Packet Parser (mirip Modbus RTU)
 *
 * @details
 * Modul       : Modul 03 - Serial UART
 * Board       : ESP32 DevKit / ESP32-S2 / ESP32-S3
 * Framework   : ESP-IDF
 *
 * Koneksi:
 *   - USB Serial (UART0) untuk komunikasi terminal
 *
 * Cara Kerja:
 *   1. Menerima byte dari UART dan menyimpan ke buffer
 *   2. Saat byte pertama diterima, mulai timer timeout
 *   3. Setiap byte baru yang diterima, reset timer timeout
 *   4. Jika tidak ada byte masuk selama TIMEOUT_MS, paket dianggap lengkap
 *   5. Paket yang lengkap ditampilkan sebagai hex dump dan ASCII
 *   6. Menggunakan esp_timer untuk tracking timeout yang presisi
 *   7. Statistik: total paket diterima, total bytes, rata-rata ukuran paket
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "TIMEOUT_PARSER";

/* Packet buffer */
static uint8_t packet_buf[MAX_PACKET_SIZE];
static volatile int packet_pos = 0;
static volatile bool packet_receiving = false;

/* Timer handle */
static esp_timer_handle_t timeout_timer;

/* Statistics */
static uint32_t total_packets = 0;
static uint32_t total_bytes = 0;

/* Mutex for buffer access */
static SemaphoreHandle_t buf_mutex;

/**
 * @brief Print hex dump dan ASCII dari paket
 */
static void process_packet(const uint8_t *data, int len)
{
    total_packets++;
    total_bytes += len;

    ESP_LOGI(TAG, "=== Paket #%lu diterima (%d bytes) ===",
             (unsigned long)total_packets, len);

    /* Hex dump */
    printf("  HEX: ");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < len) {
            printf("\r\n       ");
        }
    }
    printf("\r\n");

    /* ASCII representation */
    printf("  ASCII: ");
    for (int i = 0; i < len; i++) {
        if (data[i] >= 0x20 && data[i] < 0x7F) {
            printf("%c", data[i]);
        } else {
            printf(".");
        }
    }
    printf("\r\n");

    /* Rata-rata ukuran paket */
    float avg = (float)total_bytes / (float)total_packets;
    ESP_LOGI(TAG, "Stats: total_pkt=%lu, total_bytes=%lu, avg_size=%.1f",
             (unsigned long)total_packets, (unsigned long)total_bytes, avg);
}

/**
 * @brief Callback timeout timer - dipanggil saat timeout terjadi
 */
static void timeout_callback(void *arg)
{
    if (xSemaphoreTake(buf_mutex, 0) == pdTRUE) {
        if (packet_receiving && packet_pos > 0) {
            /* Timeout terjadi, paket lengkap */
            process_packet(packet_buf, packet_pos);
            packet_pos = 0;
            packet_receiving = false;
        }
        xSemaphoreGive(buf_mutex);
    }
}

/**
 * @brief Inisialisasi UART
 */
static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);

    ESP_LOGI(TAG, "UART%d diinisialisasi pada %d baud", UART_PORT, UART_BAUD_RATE);
}

/**
 * @brief Inisialisasi timeout timer
 */
static void timer_init(void)
{
    esp_timer_create_args_t timer_args = {
        .callback = timeout_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "uart_timeout",
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timeout_timer));
    ESP_LOGI(TAG, "Timeout timer dibuat (timeout=%d ms)", TIMEOUT_MS);
}

/**
 * @brief Restart timeout timer
 */
static void restart_timeout(void)
{
    /* Stop timer jika sedang berjalan */
    esp_timer_stop(timeout_timer);

    /* Start timer dengan timeout baru */
    esp_timer_start_once(timeout_timer, TIMEOUT_MS * 1000);  /* us */
}

/**
 * @brief Task utama penerima UART dengan timeout detection
 */
static void uart_receiver_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Receiver task dimulai");
    ESP_LOGI(TAG, "Kirim data via serial, paket terdeteksi setelah %d ms silence", TIMEOUT_MS);

    while (1) {
        uint8_t byte;
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(10));
        if (len <= 0) continue;

        if (xSemaphoreTake(buf_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (packet_pos < MAX_PACKET_SIZE) {
                packet_buf[packet_pos++] = byte;
                packet_receiving = true;

                /* Restart timeout timer setiap byte diterima */
                restart_timeout();
            } else {
                ESP_LOGW(TAG, "Packet buffer penuh (%d bytes), proses paksa",
                         MAX_PACKET_SIZE);
                process_packet(packet_buf, packet_pos);
                packet_pos = 0;
                packet_receiving = false;
                esp_timer_stop(timeout_timer);
            }
            xSemaphoreGive(buf_mutex);
        }
    }
}

/**
 * @brief Task untuk kirim paket test secara periodik
 */
static void test_sender_task(void *pvParameters)
{
    int test_num = 0;

    /* Tunggu sebentar agar output boot selesai */
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        test_num++;
        ESP_LOGI(TAG, "--- Sending test packet #%d ---", test_num);

        /* Kirim beberapa byte cepat (akan jadi 1 paket karena timeout) */
        uint8_t test_data[8];
        for (int i = 0; i < 8; i++) {
            test_data[i] = (uint8_t)(test_num * 10 + i);
        }

        /* Kirim semua byte sekaligus - akan terdeteksi sebagai 1 paket */
        uart_write_bytes(UART_PORT, test_data, sizeof(test_data));

        /* Tunggu lebih dari timeout agar paket terdeteksi */
        vTaskDelay(pdMS_TO_TICKS(TIMEOUT_MS * 3));

        /* Kirim paket kedua dengan delay antar byte */
        ESP_LOGI(TAG, "--- Sending split test ---");
        uint8_t byte1 = 0xAA;
        uart_write_bytes(UART_PORT, &byte1, 1);
        vTaskDelay(pdMS_TO_TICKS(10));  /* Kurang dari timeout */
        uint8_t byte2 = 0xBB;
        uart_write_bytes(UART_PORT, &byte2, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t byte3 = 0xCC;
        uart_write_bytes(UART_PORT, &byte3, 1);

        vTaskDelay(pdMS_TO_TICKS(STATS_INTERVAL_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Modul 03: UART Timeout Parser ===");
    ESP_LOGI(TAG, "Timeout: %d ms, Max packet: %d bytes", TIMEOUT_MS, MAX_PACKET_SIZE);

    buf_mutex = xSemaphoreCreateMutex();
    configASSERT(buf_mutex != NULL);

    uart_init();
    timer_init();

    xTaskCreate(uart_receiver_task, "uart_rx", 4096, NULL, 6, NULL);
    xTaskCreate(test_sender_task, "test_tx", 4096, NULL, 4, NULL);
}
