/* ESP32_03_SPI_Ethernet_Basic
 * Koneksi Ethernet dasar dengan W5500 via raw SPI register access
 *
 * Hardware:
 *   W5500: MOSI=13, MISO=12, SCLK=14, CS=15, INT=4, RST=5
 *   SPI Host: SPI2_HOST (HSPI)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "config.h"
#include "w5500_raw.h"

static const char *TAG = "ETH_BASIC";

#define ETH_LINK_UP_BIT   BIT0
#define ETH_LINK_DOWN_BIT BIT1

static EventGroupHandle_t eth_event_group = NULL;
static spi_device_handle_t s_spi = NULL;

static const uint8_t MAC_ADDR[6] = {0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
static const uint8_t STATIC_IP[4]  = {192, 168, 1, 100};
static const uint8_t GATEWAY[4]    = {192, 168, 1,   1};
static const uint8_t SUBNET[4]     = {255, 255, 255, 0};

static void w5500_set_network(void)
{
    w5500_writebuf(s_spi, W5500_SHAR, W5500_BSB_COMMON, MAC_ADDR, 6);
    w5500_writebuf(s_spi, W5500_GAR,  W5500_BSB_COMMON, GATEWAY, 4);
    w5500_writebuf(s_spi, W5500_SUBR, W5500_BSB_COMMON, SUBNET, 4);
    w5500_writebuf(s_spi, W5500_SIPR, W5500_BSB_COMMON, STATIC_IP, 4);
    ESP_LOGI(TAG, "Network config: IP=%d.%d.%d.%d GW=%d.%d.%d.%d",
             STATIC_IP[0], STATIC_IP[1], STATIC_IP[2], STATIC_IP[3],
             GATEWAY[0], GATEWAY[1], GATEWAY[2], GATEWAY[3]);
}

static void ethernet_status_task(void *pvParameters)
{
    bool last_link = false;
    uint32_t count = 0;
    for (;;) {
        bool link = w5500_link_up(s_spi);
        if (link && !last_link) {
            ESP_LOGI(TAG, "Ethernet Link UP");
            w5500_set_network();
            xEventGroupSetBits(eth_event_group, ETH_LINK_UP_BIT);
            xEventGroupClearBits(eth_event_group, ETH_LINK_DOWN_BIT);
        } else if (!link && last_link) {
            ESP_LOGW(TAG, "Ethernet Link DOWN");
            xEventGroupClearBits(eth_event_group, ETH_LINK_UP_BIT);
            xEventGroupSetBits(eth_event_group, ETH_LINK_DOWN_BIT);
        }
        last_link = link;
        count++;
        if (count % 6 == 0) {
            uint8_t phycfg = w5500_read8(s_spi, W5500_PHYCFGR, W5500_BSB_COMMON);
            ESP_LOGI(TAG, "[%lu] Link=%s PHYCFGR=0x%02X Heap=%lu",
                (unsigned long)count, link ? "UP" : "DOWN",
                phycfg, (unsigned long)esp_get_free_heap_size());
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* W5500 socket registers */
#define Sn_MR     0x0000
#define Sn_CR     0x0001
#define Sn_SR     0x0003
#define Sn_PORT   0x0004
#define Sn_MR_TCP 0x01
#define Sn_CR_OPEN    0x01
#define Sn_CR_LISTEN  0x02
#define Sn_CR_DISCON  0x08
#define Sn_CR_CLOSE   0x10
#define Sn_SR_ESTABLISHED 0x17

static void socket_test_task(void *pvParameters)
{
    xEventGroupWaitBits(eth_event_group, ETH_LINK_UP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "=== TCP Server on port 1234 (IP=%d.%d.%d.%d) ===",
             STATIC_IP[0], STATIC_IP[1], STATIC_IP[2], STATIC_IP[3]);
    for (;;) {
        w5500_write8(s_spi, Sn_MR,     W5500_BSB_Sn(0), Sn_MR_TCP);
        w5500_write8(s_spi, Sn_PORT,   W5500_BSB_Sn(0), 0x04);
        w5500_write8(s_spi, Sn_PORT+1, W5500_BSB_Sn(0), 0xD2);
        w5500_write8(s_spi, Sn_CR,     W5500_BSB_Sn(0), Sn_CR_OPEN);
        vTaskDelay(pdMS_TO_TICKS(5));
        w5500_write8(s_spi, Sn_CR,     W5500_BSB_Sn(0), Sn_CR_LISTEN);
        ESP_LOGI(TAG, "Listening on port 1234...");
        uint32_t wait = 0;
        uint8_t sr = 0;
        while (wait < 60) {
            sr = w5500_read8(s_spi, Sn_SR, W5500_BSB_Sn(0));
            if (sr == Sn_SR_ESTABLISHED) break;
            wait++;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        if (sr == Sn_SR_ESTABLISHED) {
            ESP_LOGI(TAG, "TCP connection established!");
            vTaskDelay(pdMS_TO_TICKS(1000));
            w5500_write8(s_spi, Sn_CR, W5500_BSB_Sn(0), Sn_CR_DISCON);
        } else {
            ESP_LOGI(TAG, "No connection (30s), retrying");
            w5500_write8(s_spi, Sn_CR, W5500_BSB_Sn(0), Sn_CR_CLOSE);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 SPI Ethernet Basic (W5500 raw SPI) ===");
    eth_event_group = xEventGroupCreate();
    esp_err_t ret = w5500_spi_init(&s_spi, ETH_SPI_HOST,
        ETH_MOSI_PIN, ETH_MISO_PIN, ETH_SCLK_PIN,
        ETH_CS_PIN, ETH_RST_PIN, ETH_INT_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 init failed!");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    xTaskCreate(ethernet_status_task, "eth_monitor", 4096, NULL, 5, NULL);
    xTaskCreate(socket_test_task,     "sock_test",   4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "Tasks created - waiting for Ethernet link...");
}
