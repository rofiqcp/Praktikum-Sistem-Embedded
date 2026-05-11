#include "w5500_raw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "W5500_RAW";

esp_err_t w5500_spi_init(spi_device_handle_t *spi, spi_host_device_t host,
                          int mosi, int miso, int sclk, int cs, int rst_pin, int int_pin)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num     = mosi,
        .miso_io_num     = miso,
        .sclk_io_num     = sclk,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "spi_bus_initialize failed: %d", ret); return ret; }

    spi_device_interface_config_t devcfg = {
        .command_bits    = 0,
        .address_bits    = 0,
        .dummy_bits      = 0,
        .mode            = 0,
        .clock_speed_hz  = 20000000,
        .spics_io_num    = cs,
        .queue_size      = 7,
    };
    ret = spi_bus_add_device(host, &devcfg, spi);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "spi_bus_add_device failed: %d", ret); return ret; }

    /* Reset W5500 */
    if (rst_pin >= 0) {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << rst_pin),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = 0,
            .pull_down_en = 0,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
        gpio_set_level(rst_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(rst_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* Configure INT pin as input */
    if (int_pin >= 0) {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << int_pin),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = 1,
            .pull_down_en = 0,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
    }

    /* Software reset */
    w5500_write8(*spi, W5500_MR, W5500_BSB_COMMON, 0x80);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Check chip version */
    uint8_t ver = w5500_read8(*spi, W5500_VERSIONR, W5500_BSB_COMMON);
    if (ver == 0x04) {
        ESP_LOGI(TAG, "W5500 detected (version 0x%02X)", ver);
    } else {
        ESP_LOGW(TAG, "W5500 version unexpected: 0x%02X (expected 0x04)", ver);
    }
    return ESP_OK;
}

/* W5500 SPI read: [addr_hi][addr_lo][ctrl] + data */
uint8_t w5500_read8(spi_device_handle_t spi, uint16_t addr, uint8_t bsb)
{
    uint8_t tx[4] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), W5500_RD(bsb), 0x00 };
    uint8_t rx[4] = { 0 };
    spi_transaction_t t = {
        .length    = 32,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    spi_device_polling_transmit(spi, &t);
    return rx[3];
}

void w5500_write8(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, uint8_t val)
{
    uint8_t tx[4] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), W5500_WR(bsb), val };
    spi_transaction_t t = {
        .length    = 32,
        .tx_buffer = tx,
        .rx_buffer = NULL,
    };
    spi_device_polling_transmit(spi, &t);
}

void w5500_readbuf(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, uint8_t *buf, size_t len)
{
    /* Send 3-byte header first, then read len bytes */
    uint8_t hdr[3] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), W5500_RD(bsb) };
    spi_transaction_t t_hdr = { .length = 24, .tx_buffer = hdr, .rx_buffer = NULL };
    spi_device_polling_transmit(spi, &t_hdr);

    spi_transaction_t t_data = {
        .length    = len * 8,
        .tx_buffer = NULL,
        .rx_buffer = buf,
    };
    spi_device_polling_transmit(spi, &t_data);
}

void w5500_writebuf(spi_device_handle_t spi, uint16_t addr, uint8_t bsb, const uint8_t *buf, size_t len)
{
    uint8_t hdr[3] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), W5500_WR(bsb) };
    spi_transaction_t t_hdr = { .length = 24, .tx_buffer = hdr, .rx_buffer = NULL };
    spi_device_polling_transmit(spi, &t_hdr);

    spi_transaction_t t_data = {
        .length    = len * 8,
        .tx_buffer = buf,
        .rx_buffer = NULL,
    };
    spi_device_polling_transmit(spi, &t_data);
}

bool w5500_link_up(spi_device_handle_t spi)
{
    uint8_t phycfg = w5500_read8(spi, W5500_PHYCFGR, W5500_BSB_COMMON);
    return (phycfg & W5500_PHYCFGR_LNK) != 0;
}
