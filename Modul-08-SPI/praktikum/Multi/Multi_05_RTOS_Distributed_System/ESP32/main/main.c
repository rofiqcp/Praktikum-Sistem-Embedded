/* Multi_05_RTOS_Distributed_System - ESP32 Hub Node (ESP-IDF)
 *
 * ESP32 berfungsi sebagai hub terdistribusi:
 *   - SPI Slave: menerima data dari STM32 (SPI2_HOST/HSPI)
 *   - Ethernet (W5500): publish data via HTTP
 *
 * Task:
 *   spi_rx_task     - terima paket dari STM32 via SPI slave → xRxQueue
 *   process_task    - proses paket, simpan ke xNodeData, kirim ke xWebQueue
 *   webserver_task  - serve HTTP /  /data /node  
 *   lcd_task        - (placeholder) print data ke serial/LCD
 *   monitor_task    - print statistik sistem
 *
 * SPI Slave (HSPI = SPI2_HOST):
 *   MOSI=13, MISO=12, SCLK=14, CS=15
 * Handshake out: GPIO 2 (HIGH = ready)
 * LED: GPIO 4
 * W5500 Ethernet: SPI3_HOST MOSI=23, MISO=19, SCLK=18, CS=5, INT=4, RST=0
 *   Note: GPIO 4 reused as W5500 INT when ETH enabled; LED moved to GPIO 25
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_server.h"

static const char *TAG = "MULTI05_HUB";

/* =========================================================
 * Pin definitions
 * ========================================================= */
#define SPI_MOSI_PIN     13
#define SPI_MISO_PIN     12
#define SPI_SCLK_PIN     14
#define SPI_CS_PIN       15
#define HANDSHAKE_PIN     2
#define LED_PIN          25

/* =========================================================
 * Distributed Packet (must match STM32 config.h)
 * ========================================================= */
typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint8_t  node_id;
    uint8_t  packet_type;
    uint16_t seq_num;
    uint32_t timestamp;
    int16_t  temperature_x100;
    uint16_t humidity_x100;
    uint16_t sd_write_count;
    uint8_t  checksum;
} DistPacket_t;

#define DIST_MAGIC      0xB6
#define PKT_TYPE_SENSOR 0x01
#define PACKET_SIZE     sizeof(DistPacket_t)

/* =========================================================
 * Node data (latest reading per node)
 * ========================================================= */
#define MAX_NODES 8

typedef struct {
    uint8_t  node_id;
    uint16_t seq_num;
    int16_t  temperature_x100;
    uint16_t humidity_x100;
    uint16_t sd_write_count;
    uint64_t rx_time_us;
    uint32_t rx_count;
} NodeData_t;

static NodeData_t node_data[MAX_NODES] = {0};
static SemaphoreHandle_t xNodeDataMutex = NULL;
static volatile uint32_t rx_total   = 0;
static volatile uint32_t crc_errors = 0;

/* =========================================================
 * Queues
 * ========================================================= */
#define RX_QUEUE_LEN  16
#define WEB_QUEUE_LEN  8

static QueueHandle_t xRxQueue  = NULL;
static QueueHandle_t xWebQueue = NULL;

/* =========================================================
 * Checksum validation
 * ========================================================= */
static uint8_t calc_checksum(const DistPacket_t *p) {
    const uint8_t *b = (const uint8_t *)p;
    uint8_t sum = 0;
    for (size_t i = 0; i < PACKET_SIZE - 1; i++) sum ^= b[i];
    return sum;
}

static bool validate_packet(const DistPacket_t *p) {
    if (p->magic != DIST_MAGIC) return false;
    return (calc_checksum(p) == p->checksum);
}

/* =========================================================
 * HTTP Handlers
 * ========================================================= */
static esp_err_t root_handler(httpd_req_t *req) {
    const char *html =
        "<!DOCTYPE html><html><head><title>ESP32 Hub</title></head>"
        "<body><h1>Multi_05 Distributed Hub</h1>"
        "<p><a href='/data'>JSON Data</a></p>"
        "<p><a href='/nodes'>Node Status</a></p>"
        "</body></html>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_sendstr(req, html);
}

static esp_err_t data_handler(httpd_req_t *req) {
    char buf[512];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "{\"hub\":\"multi05\",\"rx_total\":%lu,\"crc_errors\":%lu,\"nodes\":[",
                    (unsigned long)rx_total, (unsigned long)crc_errors);

    if (xSemaphoreTake(xNodeDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool first = true;
        for (int i = 0; i < MAX_NODES; i++) {
            if (node_data[i].node_id == 0) continue;
            if (!first) pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
            first = false;
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                            "{\"id\":%u,\"seq\":%u,\"temp\":%d.%02d,\"hum\":%d.%02d,\"rx\":%lu}",
                            node_data[i].node_id,
                            node_data[i].seq_num,
                            node_data[i].temperature_x100 / 100,
                            node_data[i].temperature_x100 % 100,
                            node_data[i].humidity_x100 / 100,
                            node_data[i].humidity_x100 % 100,
                            (unsigned long)node_data[i].rx_count);
        }
        xSemaphoreGive(xNodeDataMutex);
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

static esp_err_t nodes_handler(httpd_req_t *req) {
    char buf[512];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "<!DOCTYPE html><html><body><h2>Node List</h2><pre>");

    if (xSemaphoreTake(xNodeDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < MAX_NODES; i++) {
            if (node_data[i].node_id == 0) continue;
            pos += snprintf(buf + pos, sizeof(buf) - pos,
                            "Node %u: seq=%u T=%d.%02dC H=%d.%02d%% rx=%lu\n",
                            node_data[i].node_id,
                            node_data[i].seq_num,
                            node_data[i].temperature_x100 / 100,
                            node_data[i].temperature_x100 % 100,
                            node_data[i].humidity_x100 / 100,
                            node_data[i].humidity_x100 % 100,
                            (unsigned long)node_data[i].rx_count);
        }
        xSemaphoreGive(xNodeDataMutex);
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos, "</pre></body></html>");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_sendstr(req, buf);
}

/* =========================================================
 * Tasks
 * ========================================================= */
WORD_ALIGNED_ATTR static uint8_t spi_tx_buf[PACKET_SIZE];
WORD_ALIGNED_ATTR static uint8_t spi_rx_buf[PACKET_SIZE];

static void spi_rx_task(void *pvParameters) {
    ESP_LOGI(TAG, "SPI RX task started");

    spi_tx_buf[0] = 0xAA;  /* ACK */
    memset(spi_tx_buf + 1, 0xFF, PACKET_SIZE - 1);

    for (;;) {
        gpio_set_level(HANDSHAKE_PIN, 1);

        spi_slave_transaction_t t = {
            .length    = PACKET_SIZE * 8,
            .tx_buffer = spi_tx_buf,
            .rx_buffer = spi_rx_buf,
        };

        esp_err_t ret = spi_slave_transmit(SPI2_HOST, &t, pdMS_TO_TICKS(5000));
        gpio_set_level(HANDSHAKE_PIN, 0);

        if (ret == ESP_OK) {
            rx_total++;
            const DistPacket_t *pkt = (const DistPacket_t *)spi_rx_buf;
            if (validate_packet(pkt)) {
                if (xQueueSend(xRxQueue, pkt, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "RX queue full");
                }
            } else {
                crc_errors++;
                ESP_LOGW(TAG, "CRC error (rx=%lu)", (unsigned long)rx_total);
            }
        }
    }
}

static void process_task(void *pvParameters) {
    DistPacket_t pkt;
    ESP_LOGI(TAG, "Process task started");

    for (;;) {
        if (xQueueReceive(xRxQueue, &pkt, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "[PKT] Node=%u Seq=%u T=%d.%02dC H=%d.%02d%%",
                     pkt.node_id, pkt.seq_num,
                     pkt.temperature_x100 / 100, pkt.temperature_x100 % 100,
                     pkt.humidity_x100 / 100, pkt.humidity_x100 % 100);

            /* Update node data */
            if (xSemaphoreTake(xNodeDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                for (int i = 0; i < MAX_NODES; i++) {
                    if (node_data[i].node_id == 0 ||
                        node_data[i].node_id == pkt.node_id) {
                        node_data[i].node_id          = pkt.node_id;
                        node_data[i].seq_num          = pkt.seq_num;
                        node_data[i].temperature_x100 = pkt.temperature_x100;
                        node_data[i].humidity_x100    = pkt.humidity_x100;
                        node_data[i].sd_write_count   = pkt.sd_write_count;
                        node_data[i].rx_time_us       = esp_timer_get_time();
                        node_data[i].rx_count++;
                        break;
                    }
                }
                xSemaphoreGive(xNodeDataMutex);
            }

            gpio_set_level(LED_PIN, (rx_total % 2) ? 1 : 0);
        }
    }
}

static void webserver_task(void *pvParameters) {
    ESP_LOGI(TAG, "Webserver task starting...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port    = 80;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start httpd");
        vTaskDelete(NULL);
        return;
    }

    httpd_uri_t uri_root  = { .uri = "/",      .method = HTTP_GET, .handler = root_handler  };
    httpd_uri_t uri_data  = { .uri = "/data",  .method = HTTP_GET, .handler = data_handler  };
    httpd_uri_t uri_nodes = { .uri = "/nodes", .method = HTTP_GET, .handler = nodes_handler };

    httpd_register_uri_handler(server, &uri_root);
    httpd_register_uri_handler(server, &uri_data);
    httpd_register_uri_handler(server, &uri_nodes);

    ESP_LOGI(TAG, "HTTP server on port 80");

    /* Task stays alive to hold server context */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

static void lcd_task(void *pvParameters) {
    ESP_LOGI(TAG, "LCD/Display task started (UART output mode)");
    for (;;) {
        if (xSemaphoreTake(xNodeDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            for (int i = 0; i < MAX_NODES; i++) {
                if (node_data[i].node_id == 0) continue;
                ESP_LOGI(TAG, "[LCD] Node%u: T=%d.%02dC H=%d.%02d%%",
                         node_data[i].node_id,
                         node_data[i].temperature_x100 / 100,
                         node_data[i].temperature_x100 % 100,
                         node_data[i].humidity_x100 / 100,
                         node_data[i].humidity_x100 % 100);
            }
            xSemaphoreGive(xNodeDataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void monitor_task(void *pvParameters) {
    ESP_LOGI(TAG, "Monitor task started");
    for (;;) {
        ESP_LOGI(TAG, "=== Monitor ===");
        ESP_LOGI(TAG, "  RX total: %lu | CRC errors: %lu", (unsigned long)rx_total, (unsigned long)crc_errors);
        ESP_LOGI(TAG, "  Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
        ESP_LOGI(TAG, "  Uptime: %llu s", esp_timer_get_time() / 1000000ULL);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* =========================================================
 * app_main
 * ========================================================= */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Multi_05 ESP32 Distributed Hub ===");

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

    /* SPI Slave (SPI2_HOST = HSPI) */
    spi_bus_config_t buscfg = {
        .mosi_io_num   = SPI_MOSI_PIN,
        .miso_io_num   = SPI_MISO_PIN,
        .sclk_io_num   = SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_slave_interface_config_t slvcfg = {
        .mode         = 0,
        .spics_io_num = SPI_CS_PIN,
        .queue_size   = 4,
        .flags        = 0,
        .post_setup_cb = NULL,
        .post_trans_cb = NULL,
    };
    ESP_ERROR_CHECK(spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI2 slave initialized");

    /* Mutex */
    xNodeDataMutex = xSemaphoreCreateMutex();
    if (!xNodeDataMutex) {
        ESP_LOGE(TAG, "Mutex creation failed!");
        return;
    }

    /* Queues */
    xRxQueue  = xQueueCreate(RX_QUEUE_LEN, sizeof(DistPacket_t));
    xWebQueue = xQueueCreate(WEB_QUEUE_LEN, sizeof(DistPacket_t));
    if (!xRxQueue || !xWebQueue) {
        ESP_LOGE(TAG, "Queue creation failed!");
        return;
    }

    /* Tasks */
    xTaskCreate(spi_rx_task,   "spi_rx",  4096, NULL, 6, NULL);
    xTaskCreate(process_task,  "process", 4096, NULL, 5, NULL);
    xTaskCreate(webserver_task,"webserv", 8192, NULL, 3, NULL);
    xTaskCreate(lcd_task,      "lcd",     2048, NULL, 2, NULL);
    xTaskCreate(monitor_task,  "monitor", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "All tasks created. Hub running.");
}
