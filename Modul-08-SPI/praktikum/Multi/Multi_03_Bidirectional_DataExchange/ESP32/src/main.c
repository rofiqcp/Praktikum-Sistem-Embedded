#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "BIDIR_SPI";

typedef struct {
    uint8_t cmd;
    uint8_t data[BUFFER_SIZE - 1];
} spi_packet_t;

DMA_ATTR spi_packet_t tx_packet;
DMA_ATTR spi_packet_t rx_packet;

static uint32_t sensor_value = 0;
static uint8_t led_state = 0;
static uint32_t transaction_count = 0;

void process_command(void)
{
    switch (rx_packet.cmd) {
        case CMD_READ_SENSOR:
            // Simulate sensor reading
            sensor_value = (sensor_value + 1) % 1000;
            tx_packet.cmd = CMD_READ_SENSOR;
            snprintf((char *)tx_packet.data, sizeof(tx_packet.data), "SENSOR:%lu", sensor_value);
            ESP_LOGI(TAG, "CMD: Read Sensor -> %lu", sensor_value);
            break;
            
        case CMD_WRITE_LED:
            // Control LED based on received data
            led_state = rx_packet.data[0];
            gpio_set_level(LED_PIN, led_state);
            tx_packet.cmd = CMD_WRITE_LED;
            snprintf((char *)tx_packet.data, sizeof(tx_packet.data), "LED:%s", led_state ? "ON" : "OFF");
            ESP_LOGI(TAG, "CMD: Write LED -> %s", led_state ? "ON" : "OFF");
            break;
            
        case CMD_GET_STATUS:
            tx_packet.cmd = CMD_GET_STATUS;
            snprintf((char *)tx_packet.data, sizeof(tx_packet.data), 
                     "CNT:%lu,LED:%d,SENS:%lu", transaction_count, led_state, sensor_value);
            ESP_LOGI(TAG, "CMD: Get Status");
            break;
            
        case CMD_ECHO:
            // Echo back received data
            tx_packet.cmd = CMD_ECHO;
            memcpy(tx_packet.data, rx_packet.data, sizeof(tx_packet.data));
            ESP_LOGI(TAG, "CMD: Echo -> %s", rx_packet.data);
            break;
            
        default:
            tx_packet.cmd = 0xFF;  // Error
            snprintf((char *)tx_packet.data, sizeof(tx_packet.data), "UNKNOWN_CMD:0x%02X", rx_packet.cmd);
            ESP_LOGW(TAG, "Unknown command: 0x%02X", rx_packet.cmd);
            break;
    }
}

void spi_slave_task(void *pvParameters)
{
    esp_err_t ret;
    spi_slave_transaction_t t;
    
    // Configuration for the SPI slave interface
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 3,
        .flags = 0,
    };
    
    ret = spi_slave_initialize(SPI_SLAVE_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);
    
    ESP_LOGI(TAG, "ESP32 Bidirectional SPI Slave initialized");
    
    // Initialize default response
    tx_packet.cmd = CMD_GET_STATUS;
    snprintf((char *)tx_packet.data, sizeof(tx_packet.data), "READY");
    
    while (1) {
        memset(&t, 0, sizeof(t));
        t.length = sizeof(spi_packet_t) * 8;
        t.tx_buffer = &tx_packet;
        t.rx_buffer = &rx_packet;
        
        // Signal ready to master
        gpio_set_level(PIN_NUM_HANDSHAKE, 1);
        
        // Wait for transaction
        ret = spi_slave_transmit(SPI_SLAVE_HOST, &t, portMAX_DELAY);
        
        gpio_set_level(PIN_NUM_HANDSHAKE, 0);
        
        if (ret == ESP_OK && t.trans_len > 0) {
            transaction_count++;
            ESP_LOGI(TAG, "Transaction #%lu completed", transaction_count);
            
            // Process received command
            process_command();
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    // Configure LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    // Configure handshake pin
    gpio_reset_pin(PIN_NUM_HANDSHAKE);
    gpio_set_direction(PIN_NUM_HANDSHAKE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_HANDSHAKE, 0);
    
    ESP_LOGI(TAG, "=== ESP32 Bidirectional Data Exchange ===");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MISO:      GPIO%d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  MOSI:      GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  CLK:       GPIO%d", PIN_NUM_CLK);
    ESP_LOGI(TAG, "  CS:        GPIO%d", PIN_NUM_CS);
    ESP_LOGI(TAG, "  HANDSHAKE: GPIO%d", PIN_NUM_HANDSHAKE);
    ESP_LOGI(TAG, "\nSupported Commands:");
    ESP_LOGI(TAG, "  0x01 - Read Sensor");
    ESP_LOGI(TAG, "  0x02 - Write LED");
    ESP_LOGI(TAG, "  0x03 - Get Status");
    ESP_LOGI(TAG, "  0x04 - Echo");
    
    xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 5, NULL);
}
