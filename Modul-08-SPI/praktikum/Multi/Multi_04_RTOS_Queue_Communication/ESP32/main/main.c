/* Multi_04_RTOS_Queue_Communication - ESP32 (SPI Slave)
 * Menerima data sensor dari STM32 via SPI, proses dengan FreeRTOS Queue
 *
 * Task:
 *   rx_task      - tunggu SPI transfer dari STM32, parse paket, kirim ke queue
 *   process_task - terima dari queue, proses dan cetak data
 *   display_task - tampilkan statistik ringkas setiap 5 detik
 *   monitor_task - print sistem status setiap 10 detik
 *
 * Hardware:
 *   SPI Slave (HSPI_HOST/SPI2_HOST):
 *     MOSI=13, MISO=12, SCLK=14, CS=15
 *   Handshake out (ready signal): GPIO 2 (HIGH = ready)
 *   LED: GPIO 4
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MULTI04_ESP32";

/* =========================================================
 * Pin definitions
 * ========================================================= */
#define SPI_MOSI_PIN    13
#define SPI_MISO_PIN    12
#define SPI_SCLK_PIN    14
#define SPI_CS_PIN      15
#define HANDSHAKE_PIN    2  /* GPIO output: HIGH = slave ready */
#define LED_PIN          4

/* =========================================================
 * Packet structure (must match STM32 config.h)
 * ========================================================= */
typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  cmd;
    int16_t  temperature_x100;
    uint16_t humidity_x100;
    uint32_t timestamp;
    uint8_t  checksum;
} SpiPacket_t;

#define PACKET_MAGIC    0xA5
#define CMD_SENSOR_DATA 0x01
#define CMD_ACK         0xAA
#define PACKET_SIZE     sizeof(SpiPacket_t)

/* =========================================================
 * FreeRTOS objects
 * ========================================================= */
#define DATA_QUEUE_LEN  16

static QueueHandle_t xDataQueue = NULL;

/* =========================================================
 * Statistics
 * ========================================================= */
static volatile uint32_t rx_count    = 0;
static volatile uint32_t proc_count  = 0;
static volatile uint32_t crc_errors  = 0;

/* =========================================================
 * Packet validation
 * ========================================================= */
static uint8_t calc_checksum(const SpiPacket_t *p) {
    const uint8_t *b = (const uint8_t *)p;
    uint8_t sum = 0;
    for (size_t i = 0; i < PACKET_SIZE - 1; i++) sum ^= b[i];
    return sum;
}

static bool validate_packet(const SpiPacket_t *p) {
    if (p->magic != PACKET_MAGIC) return false;
    return (calc_checksum(p) == p->checksum);
}

/* =========================================================
 * SPI Slave Tasks
 * ========================================================= */
WORD_ALIGNED_ATTR static uint8_t tx_buf[PACKET_SIZE];
WORD_ALIGNED_ATTR static uint8_t rx_buf[PACKET_SIZE];

static void rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "RX task started");

    /* Prepare ACK response */
    memset(tx_buf, 0xFF, sizeof(tx_buf));
    tx_buf[0] = CMD_ACK;

    for (;;) {
        /* Signal ready */
        gpio_set_level(HANDSHAKE_PIN, 1);

        spi_slave_transaction_t t = {
            .length    = PACKET_SIZE * 8,
            .tx_buffer = tx_buf,
            .rx_buffer = rx_buf,
        };

        /* Wait for master to start transaction */
        esp_err_t ret = spi_slave_transmit(SPI2_HOST, &t, pdMS_TO_TICKS(5000));

        /* Lower handshake while processing */
        gpio_set_level(HANDSHAKE_PIN, 0);

        if (ret == ESP_OK) {
            rx_count++;
            const SpiPacket_t *pkt = (const SpiPacket_t *)rx_buf;

            if (validate_packet(pkt)) {
                if (xQueueSend(xDataQueue, pkt, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "Queue full, dropping packet");
                }
            } else {
                crc_errors++;
                ESP_LOGW(TAG, "Checksum error (rx=%lu)", (unsigned long)rx_count);
            }
        } else if (ret == ESP_ERR_TIMEOUT) {
            /* No packet, continue */
        } else {
            ESP_LOGE(TAG, "SPI slave error: %d", ret);
        }
    }
}

static void process_task(void *pvParameters)
{
    SpiPacket_t pkt;
    ESP_LOGI(TAG, "Process task started");

    for (;;) {
        if (xQueueReceive(xDataQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            proc_count++;

            if (pkt.cmd == CMD_SENSOR_DATA) {
                ESP_LOGI(TAG, "[DATA] #%lu T=%d.%02d C, H=%d.%02d%%, ts=%lu",
                         (unsigned long)proc_count,
                         pkt.temperature_x100 / 100, pkt.temperature_x100 % 100,
                         pkt.humidity_x100 / 100, pkt.humidity_x100 % 100,
                         (unsigned long)pkt.timestamp);
            } else {
                ESP_LOGI(TAG, "[PKT] cmd=0x%02X ts=%lu", pkt.cmd, (unsigned long)pkt.timestamp);
            }

            /* Toggle LED */
            gpio_set_level(LED_PIN, (proc_count % 2) ? 1 : 0);
        }
    }
}

static void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task started");

    for (;;) {
        ESP_LOGI(TAG, "=== Display ===");
        ESP_LOGI(TAG, "  Received: %lu | Processed: %lu | CRC errors: %lu",
                 (unsigned long)rx_count,
                 (unsigned long)proc_count,
                 (unsigned long)crc_errors);
        ESP_LOGI(TAG, "  Queue: %u/%d items",
                 (unsigned)uxQueueMessagesWaiting(xDataQueue), DATA_QUEUE_LEN);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor task started");

    for (;;) {
        ESP_LOGI(TAG, "=== System Monitor ===");
        ESP_LOGI(TAG, "  Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "  Uptime: %llu s", esp_timer_get_time() / 1000000);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* =========================================================
 * app_main
 * ========================================================= */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Multi_04 ESP32 SPI Slave RTOS Queue ===");

    /* GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << HANDSHAKE_PIN) | (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(HANDSHAKE_PIN, 0);
    gpio_set_level(LED_PIN, 0);

    /* SPI Slave init */
    spi_bus_config_t buscfg = {
        .mosi_io_num   = SPI_MOSI_PIN,
        .miso_io_num   = SPI_MISO_PIN,
        .sclk_io_num   = SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_slave_interface_config_t slvcfg = {
        .mode          = 0,
        .spics_io_num  = SPI_CS_PIN,
        .queue_size    = 4,
        .flags         = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL,
    };
    ESP_ERROR_CHECK(spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI Slave initialized on SPI2_HOST");

    /* Create queue */
    xDataQueue = xQueueCreate(DATA_QUEUE_LEN, sizeof(SpiPacket_t));
    if (!xDataQueue) {
        ESP_LOGE(TAG, "Failed to create queue!");
        return;
    }

    /* Create tasks */
    xTaskCreate(rx_task,      "spi_rx",   4096, NULL, 6, NULL);
    xTaskCreate(process_task, "process",  4096, NULL, 5, NULL);
    xTaskCreate(display_task, "display",  2048, NULL, 2, NULL);
    xTaskCreate(monitor_task, "monitor",  2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "All tasks created");
}
