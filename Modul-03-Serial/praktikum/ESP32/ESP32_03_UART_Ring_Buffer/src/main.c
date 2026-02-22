/**
 * @file    main.c
 * @brief   ESP32_03 – UART dengan Custom Circular/Ring Buffer
 *
 * @details
 * Modul       : Modul-03 Serial UART
 * Board       : ESP32 DevKit / Lolin S2 Mini / ESP32-S3 DevKitC
 * Framework   : ESP-IDF
 *
 * Koneksi     :
 *   - UART0 menggunakan pin default (TX=GPIO1, RX=GPIO3 pada ESP32 DevKit)
 *   - Hubungkan USB ke PC, gunakan serial monitor 115200 baud
 *
 * Cara Kerja  :
 *   1. Implementasi struct ring_buffer_t sebagai circular buffer manual.
 *   2. Fungsi rb_init, rb_push, rb_pop, rb_is_full, rb_is_empty disediakan.
 *   3. Data dari UART dibaca dan dimasukkan ke ring buffer via rb_push().
 *   4. Data diproses (echo) dengan membaca dari ring buffer via rb_pop().
 *   5. Statistik (jumlah push, pop, overflow) ditampilkan setiap 5 detik.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "UART_RINGBUF";

/* ════════════════════════════════════════════════════
 * Ring Buffer Implementation
 * ════════════════════════════════════════════════════ */

/**
 * @brief Struktur data ring buffer (circular buffer)
 */
typedef struct {
    uint8_t  buffer[RING_BUF_SIZE]; /**< Array penyimpan data        */
    uint16_t head;                  /**< Indeks tulis (push)         */
    uint16_t tail;                  /**< Indeks baca  (pop)          */
    uint16_t count;                 /**< Jumlah elemen saat ini      */
    uint16_t size;                  /**< Kapasitas maksimum          */
    uint32_t overflow_count;        /**< Statistik: jumlah overflow  */
} ring_buffer_t;

/**
 * @brief Inisialisasi ring buffer
 * @param rb Pointer ke ring_buffer_t
 */
static void rb_init(ring_buffer_t *rb)
{
    memset(rb->buffer, 0, sizeof(rb->buffer));
    rb->head           = 0;
    rb->tail           = 0;
    rb->count          = 0;
    rb->size           = RING_BUF_SIZE;
    rb->overflow_count = 0;
}

/**
 * @brief Cek apakah ring buffer penuh
 * @return true jika penuh
 */
static bool rb_is_full(const ring_buffer_t *rb)
{
    return rb->count >= rb->size;
}

/**
 * @brief Cek apakah ring buffer kosong
 * @return true jika kosong
 */
static bool rb_is_empty(const ring_buffer_t *rb)
{
    return rb->count == 0;
}

/**
 * @brief Push satu byte ke ring buffer
 * @return true jika berhasil, false jika buffer penuh
 */
static bool rb_push(ring_buffer_t *rb, uint8_t byte)
{
    if (rb_is_full(rb)) {
        rb->overflow_count++;
        return false;
    }
    rb->buffer[rb->head] = byte;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    return true;
}

/**
 * @brief Pop satu byte dari ring buffer
 * @param[out] byte Pointer untuk menyimpan byte yang dibaca
 * @return true jika berhasil, false jika buffer kosong
 */
static bool rb_pop(ring_buffer_t *rb, uint8_t *byte)
{
    if (rb_is_empty(rb)) {
        return false;
    }
    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    return true;
}

/* ════════════════════════════════════════════════════
 * UART
 * ════════════════════════════════════════════════════ */

static ring_buffer_t rx_ring;          /**< Ring buffer untuk RX data   */
static uint32_t total_pushed = 0;      /**< Statistik total push        */
static uint32_t total_popped = 0;      /**< Statistik total pop         */

/**
 * @brief Inisialisasi UART
 */
static void uart_init(void)
{
    const uart_config_t uart_cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE, BUF_SIZE,
                                        0, NULL, 0));
}

/**
 * @brief Task untuk membaca UART → push ke ring buffer → pop & echo
 */
static void uart_ring_task(void *pvParameters)
{
    uint8_t tmp[128];

    while (1) {
        /* ── Baca dari UART dan push ke ring buffer ── */
        int len = uart_read_bytes(UART_PORT, tmp, sizeof(tmp),
                                  pdMS_TO_TICKS(20));
        for (int i = 0; i < len; i++) {
            if (rb_push(&rx_ring, tmp[i])) {
                total_pushed++;
            } else {
                ESP_LOGW(TAG, "Ring buffer penuh! Byte 0x%02X dibuang", tmp[i]);
            }
        }

        /* ── Pop dari ring buffer dan echo kembali ── */
        uint8_t byte;
        while (rb_pop(&rx_ring, &byte)) {
            uart_write_bytes(UART_PORT, (const char *)&byte, 1);
            total_popped++;
        }
    }
}

/**
 * @brief Task untuk menampilkan statistik ring buffer
 */
static void stats_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "── Statistik Ring Buffer ──");
        ESP_LOGI(TAG, "  Kapasitas   : %u", rx_ring.size);
        ESP_LOGI(TAG, "  Isi saat ini: %u", rx_ring.count);
        ESP_LOGI(TAG, "  Total push  : %lu", (unsigned long)total_pushed);
        ESP_LOGI(TAG, "  Total pop   : %lu", (unsigned long)total_popped);
        ESP_LOGI(TAG, "  Overflow    : %lu", (unsigned long)rx_ring.overflow_count);
    }
}

/**
 * @brief Entry point aplikasi
 */
void app_main(void)
{
    rb_init(&rx_ring);
    uart_init();

    ESP_LOGI(TAG, "UART Ring Buffer siap (kapasitas=%u)", RING_BUF_SIZE);

    xTaskCreate(uart_ring_task, "uart_ring_task", 4096, NULL, 12, NULL);
    xTaskCreate(stats_task,     "stats_task",     2048, NULL,  5, NULL);
}
