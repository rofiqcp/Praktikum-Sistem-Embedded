/* ESP32_09_SPI_Ethernet_RTOS_Semaphore
 * Ethernet W5500 dengan FreeRTOS Semaphore untuk sinkronisasi TX/RX
 * Menggunakan raw SPI register access (tidak memerlukan managed component)
 *
 * Hardware:
 *   W5500: MOSI=23, MISO=19, SCLK=18, CS=5, INT=4
 *   SPI Host: SPI2_HOST
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "config.h"
#include "w5500_raw.h"

static const char *TAG = "ETH_RTOS_SEM";

/* Event bits */
#define ETH_CONNECTED_BIT   BIT0
#define ETH_TX_READY_BIT    BIT1
#define ETH_RX_READY_BIT    BIT2

/* Global variables */
static EventGroupHandle_t eth_event_group;
static SemaphoreHandle_t  tx_semaphore;
static SemaphoreHandle_t  rx_semaphore;
static SemaphoreHandle_t  eth_mutex;
static spi_device_handle_t s_spi = NULL;
static bool eth_connected = false;

/* Statistics */
typedef struct {
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t errors;
} eth_stats_t;
static eth_stats_t eth_stats = {0};

/* Static IP */
static const uint8_t MAC_ADDR[6]  = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};
static const uint8_t STATIC_IP[4] = {192, 168, 1, 101};
static const uint8_t GATEWAY[4]   = {192, 168, 1,   1};
static const uint8_t SUBNET[4]    = {255, 255, 255,  0};

/* =========================================================
 * Network Task - monitors link and manages state
 * ========================================================= */
static void network_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Network Task started");
    bool last_link = false;

    while (1) {
        bool link = w5500_link_up(s_spi);

        if (link && !last_link) {
            ESP_LOGI(TAG, "Ethernet Link UP");
            /* Configure W5500 network */
            w5500_writebuf(s_spi, W5500_SHAR, W5500_BSB_COMMON, MAC_ADDR, 6);
            w5500_writebuf(s_spi, W5500_GAR,  W5500_BSB_COMMON, GATEWAY, 4);
            w5500_writebuf(s_spi, W5500_SUBR, W5500_BSB_COMMON, SUBNET, 4);
            w5500_writebuf(s_spi, W5500_SIPR, W5500_BSB_COMMON, STATIC_IP, 4);
            ESP_LOGI(TAG, "IP=%d.%d.%d.%d GW=%d.%d.%d.%d",
                STATIC_IP[0],STATIC_IP[1],STATIC_IP[2],STATIC_IP[3],
                GATEWAY[0],GATEWAY[1],GATEWAY[2],GATEWAY[3]);
            eth_connected = true;
            xEventGroupSetBits(eth_event_group, ETH_CONNECTED_BIT);
            xSemaphoreGive(tx_semaphore);
            xSemaphoreGive(rx_semaphore);
        } else if (!link && last_link) {
            ESP_LOGW(TAG, "Ethernet Link DOWN");
            eth_connected = false;
            xEventGroupClearBits(eth_event_group, ETH_CONNECTED_BIT | ETH_TX_READY_BIT | ETH_RX_READY_BIT);
        }
        last_link = link;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* =========================================================
 * TX Task - simulates packet transmission with semaphore
 * ========================================================= */
static void tx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "TX Task started");
    uint32_t packet_count = 0;

    while (1) {
        if (xSemaphoreTake(tx_semaphore, pdMS_TO_TICKS(SEMAPHORE_TIMEOUT_MS)) == pdTRUE) {
            if (eth_connected) {
                if (xSemaphoreTake(eth_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    uint32_t packet_size = 64 + (packet_count % 1400);
                    eth_stats.tx_packets++;
                    eth_stats.tx_bytes += packet_size;
                    ESP_LOGI(TAG, "TX: Packet %lu, Size: %lu bytes",
                             (unsigned long)packet_count, (unsigned long)packet_size);
                    xSemaphoreGive(eth_mutex);
                }
                packet_count++;
            }
            xSemaphoreGive(tx_semaphore);
        } else {
            ESP_LOGW(TAG, "TX: Timeout waiting for semaphore");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* =========================================================
 * RX Task - simulates packet reception with semaphore
 * ========================================================= */
static void rx_task(void *pvParameters)
{
    ESP_LOGI(TAG, "RX Task started");
    uint32_t packet_count = 0;

    while (1) {
        if (xSemaphoreTake(rx_semaphore, pdMS_TO_TICKS(SEMAPHORE_TIMEOUT_MS)) == pdTRUE) {
            if (eth_connected) {
                if (xSemaphoreTake(eth_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    uint32_t packet_size = 64 + (packet_count % 1400);
                    eth_stats.rx_packets++;
                    eth_stats.rx_bytes += packet_size;
                    ESP_LOGI(TAG, "RX: Packet %lu, Size: %lu bytes",
                             (unsigned long)packet_count, (unsigned long)packet_size);
                    xSemaphoreGive(eth_mutex);
                }
                packet_count++;
            }
            xSemaphoreGive(rx_semaphore);
        } else {
            ESP_LOGD(TAG, "RX: No data");
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

/* =========================================================
 * Monitor Task - prints statistics
 * ========================================================= */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor Task started");

    while (1) {
        if (xSemaphoreTake(eth_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint8_t phycfg = w5500_read8(s_spi, W5500_PHYCFGR, W5500_BSB_COMMON);
            ESP_LOGI(TAG, "=== Stats === Link:%s TX:%lu pkts/%lu B  RX:%lu pkts/%lu B  Errors:%lu  PHYCFGR:0x%02X",
                     eth_connected ? "UP" : "DOWN",
                     (unsigned long)eth_stats.tx_packets, (unsigned long)eth_stats.tx_bytes,
                     (unsigned long)eth_stats.rx_packets, (unsigned long)eth_stats.rx_bytes,
                     (unsigned long)eth_stats.errors, phycfg);
            xSemaphoreGive(eth_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* =========================================================
 * app_main
 * ========================================================= */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 SPI Ethernet RTOS Semaphore (W5500 raw SPI) ===");

    /* Create synchronization primitives */
    eth_event_group = xEventGroupCreate();
    tx_semaphore    = xSemaphoreCreateBinary();
    rx_semaphore    = xSemaphoreCreateBinary();
    eth_mutex       = xSemaphoreCreateMutex();

    /* Initialize W5500 */
    esp_err_t ret = w5500_spi_init(&s_spi, SPI2_HOST,
        ETH_MOSI_PIN, ETH_MISO_PIN, ETH_SCLK_PIN,
        ETH_CS_PIN, ETH_RST_PIN, ETH_INT_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 init failed!");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Create FreeRTOS tasks */
    xTaskCreate(network_task, "net_task", TASK_STACK_SIZE, NULL, NETWORK_TASK_PRIORITY, NULL);
    xTaskCreate(tx_task,      "tx_task",  TASK_STACK_SIZE, NULL, TX_TASK_PRIORITY,      NULL);
    xTaskCreate(rx_task,      "rx_task",  TASK_STACK_SIZE, NULL, RX_TASK_PRIORITY,      NULL);
    xTaskCreate(monitor_task, "monitor",  TASK_STACK_SIZE, NULL, MONITOR_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "All tasks created - waiting for Ethernet link...");
}
