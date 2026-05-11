/* ESP32_06_SPI_Ethernet_WebServer
 * HTTP Web Server menggunakan W5500 Ethernet via raw SPI
 * W5500 built-in TCP stack used directly (no LwIP)
 *
 * Hardware:
 *   W5500: MOSI=23, MISO=19, SCLK=18, CS=5, INT=4
 *   SPI Host: SPI3_HOST (VSPI)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "config.h"
#include "w5500_raw.h"

static const char *TAG = "ETH_WEBSERVER";

#define ETH_LINK_UP_BIT BIT0
static EventGroupHandle_t eth_event_group = NULL;
static spi_device_handle_t s_spi = NULL;

static uint32_t request_count = 0;
static float sensor_temp = 25.5f;
static float sensor_humidity = 60.0f;

static const uint8_t MAC_ADDR[6]   = {0x02, 0xCC, 0xDD, 0xEE, 0xFF, 0x01};
static const uint8_t STATIC_IP[4]  = {192, 168, 1, 100};
static const uint8_t GATEWAY[4]    = {192, 168, 1,   1};
static const uint8_t SUBNET[4]     = {255, 255, 255,  0};

/* W5500 socket registers (socket 0) */
#define Sn_MR       0x0000
#define Sn_CR       0x0001
#define Sn_IR       0x0002
#define Sn_SR       0x0003
#define Sn_PORT     0x0004   /* source port (2 bytes) */
#define Sn_TX_FSR   0x0020   /* TX free size (2 bytes) */
#define Sn_TX_WR    0x0024   /* TX write pointer (2 bytes) */
#define Sn_RX_RSR   0x0026   /* RX received size (2 bytes) */
#define Sn_RX_RD    0x0028   /* RX read pointer (2 bytes) */
#define Sn_MR_TCP   0x01
#define Sn_CR_OPEN    0x01
#define Sn_CR_LISTEN  0x02
#define Sn_CR_SEND    0x20
#define Sn_CR_RECV    0x40
#define Sn_CR_DISCON  0x08
#define Sn_CR_CLOSE   0x10
#define Sn_SR_SOCK_CLOSED     0x00
#define Sn_SR_SOCK_INIT       0x13
#define Sn_SR_SOCK_LISTEN     0x14
#define Sn_SR_SOCK_ESTABLISHED 0x17
#define Sn_SR_SOCK_CLOSE_WAIT  0x1C

static void w5500_set_network(void)
{
    w5500_writebuf(s_spi, W5500_SHAR, W5500_BSB_COMMON, MAC_ADDR, 6);
    w5500_writebuf(s_spi, W5500_GAR,  W5500_BSB_COMMON, GATEWAY, 4);
    w5500_writebuf(s_spi, W5500_SUBR, W5500_BSB_COMMON, SUBNET, 4);
    w5500_writebuf(s_spi, W5500_SIPR, W5500_BSB_COMMON, STATIC_IP, 4);
    ESP_LOGI(TAG, "IP=%d.%d.%d.%d GW=%d.%d.%d.%d",
             STATIC_IP[0], STATIC_IP[1], STATIC_IP[2], STATIC_IP[3],
             GATEWAY[0], GATEWAY[1], GATEWAY[2], GATEWAY[3]);
}

/* Read W5500 16-bit register */
static uint16_t w5500_read16(spi_device_handle_t spi, uint16_t addr, uint8_t bsb)
{
    return ((uint16_t)w5500_read8(spi, addr, bsb) << 8) |
            (uint16_t)w5500_read8(spi, addr + 1, bsb);
}
static void w5500_write16(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, uint16_t val)
{
    w5500_write8(spi, addr,     bsb, (uint8_t)(val >> 8));
    w5500_write8(spi, addr + 1, bsb, (uint8_t)(val & 0xFF));
}

/* W5500 TX data to socket 0 */
static void w5500_socket_send(uint8_t sock, const uint8_t *data, uint16_t len)
{
    /* Get TX write pointer */
    uint16_t ptr = w5500_read16(s_spi, Sn_TX_WR, W5500_BSB_Sn(sock));
    /* TX buffer offset (W5500 TX buffer base is 0x0000, size 2KB) */
    uint16_t offset = ptr & 0x07FF;  /* 2KB mask */
    /* Write data - simplified: single contiguous write */
    w5500_writebuf(s_spi, offset, W5500_BSB_Sn_TX(sock), data, len);
    /* Advance TX write pointer */
    w5500_write16(s_spi, Sn_TX_WR, W5500_BSB_Sn(sock), ptr + len);
    /* Issue SEND command */
    w5500_write8(s_spi, Sn_CR, W5500_BSB_Sn(sock), Sn_CR_SEND);
    /* Wait for send to complete */
    uint32_t wait = 0;
    while (w5500_read8(s_spi, Sn_CR, W5500_BSB_Sn(sock)) != 0 && wait < 100) {
        vTaskDelay(pdMS_TO_TICKS(1));
        wait++;
    }
}

/* Build and send HTTP response */
static void send_http_response(uint8_t sock)
{
    /* Simulate sensor update */
    sensor_temp = 20.0f + (float)(((uint32_t)rand() % 1500)) / 100.0f;
    sensor_humidity = 40.0f + (float)(((uint32_t)rand() % 4000)) / 100.0f;
    request_count++;

    static char body[512];
    snprintf(body, sizeof(body),
        "<!DOCTYPE html><html><head><title>ESP32 W5500 Server</title></head><body>"
        "<h1>ESP32 Ethernet Web Server</h1>"
        "<p>Temperature: %.1f C</p>"
        "<p>Humidity: %.1f %%</p>"
        "<p>Requests: %lu</p>"
        "<p>Uptime: %lu s</p>"
        "</body></html>",
        sensor_temp, sensor_humidity,
        (unsigned long)request_count,
        (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000));

    static char hdr[256];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        (int)strlen(body));

    w5500_socket_send(sock, (uint8_t *)hdr, strlen(hdr));
    w5500_socket_send(sock, (uint8_t *)body, strlen(body));
    ESP_LOGI(TAG, "Served request #%lu (%.1fC, %.1f%%)",
             (unsigned long)request_count, sensor_temp, sensor_humidity);
}

/* =========================================================
 * Link monitor task
 * ========================================================= */
static void eth_link_task(void *pvParameters)
{
    bool last = false;
    for (;;) {
        bool up = w5500_link_up(s_spi);
        if (up && !last) {
            ESP_LOGI(TAG, "Link UP - setting network config");
            w5500_set_network();
            xEventGroupSetBits(eth_event_group, ETH_LINK_UP_BIT);
        } else if (!up && last) {
            ESP_LOGW(TAG, "Link DOWN");
            xEventGroupClearBits(eth_event_group, ETH_LINK_UP_BIT);
        }
        last = up;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* =========================================================
 * HTTP Server task using W5500 socket 0
 * ========================================================= */
static void webserver_task(void *pvParameters)
{
    xEventGroupWaitBits(eth_event_group, ETH_LINK_UP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "=== HTTP Server started on http://%d.%d.%d.%d/ ===",
             STATIC_IP[0], STATIC_IP[1], STATIC_IP[2], STATIC_IP[3]);

    for (;;) {
        /* Open TCP socket on port 80 */
        w5500_write8(s_spi, Sn_MR,     W5500_BSB_Sn(0), Sn_MR_TCP);
        w5500_write8(s_spi, Sn_PORT,   W5500_BSB_Sn(0), 0x00);  /* port 80 = 0x0050 */
        w5500_write8(s_spi, Sn_PORT+1, W5500_BSB_Sn(0), 0x50);
        w5500_write8(s_spi, Sn_CR,     W5500_BSB_Sn(0), Sn_CR_OPEN);
        vTaskDelay(pdMS_TO_TICKS(5));
        w5500_write8(s_spi, Sn_CR,     W5500_BSB_Sn(0), Sn_CR_LISTEN);

        /* Wait for incoming connection */
        uint8_t sr;
        do {
            sr = w5500_read8(s_spi, Sn_SR, W5500_BSB_Sn(0));
            vTaskDelay(pdMS_TO_TICKS(10));
        } while (sr != Sn_SR_SOCK_ESTABLISHED && sr != Sn_SR_SOCK_CLOSE_WAIT);

        if (sr == Sn_SR_SOCK_ESTABLISHED) {
            /* Wait for some data */
            vTaskDelay(pdMS_TO_TICKS(50));
            /* Send HTTP response */
            send_http_response(0);
        }

        /* Disconnect and close */
        w5500_write8(s_spi, Sn_CR, W5500_BSB_Sn(0), Sn_CR_DISCON);
        vTaskDelay(pdMS_TO_TICKS(100));
        w5500_write8(s_spi, Sn_CR, W5500_BSB_Sn(0), Sn_CR_CLOSE);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Ethernet WebServer (W5500 raw SPI) ===");
    eth_event_group = xEventGroupCreate();

    /* ETH_RST_PIN = -1 in config, will skip reset */
    esp_err_t ret = w5500_spi_init(&s_spi, SPI3_HOST,
        ETH_MOSI_PIN, ETH_MISO_PIN, ETH_SCLK_PIN,
        ETH_CS_PIN, ETH_RST_PIN, ETH_INT_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 init failed!");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    xTaskCreate(eth_link_task,  "eth_link",   4096, NULL, 6, NULL);
    xTaskCreate(webserver_task, "webserver",  8192, NULL, 5, NULL);
    ESP_LOGI(TAG, "Tasks created");
}
