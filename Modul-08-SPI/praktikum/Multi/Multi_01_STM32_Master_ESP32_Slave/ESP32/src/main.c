#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "SPI_SLAVE";

// SPI transaction buffers
DMA_ATTR uint8_t sendbuf[BUFFER_SIZE] = {0};
DMA_ATTR uint8_t recvbuf[BUFFER_SIZE] = {0};

void spi_slave_task(void *pvParameters)
{
    esp_err_t ret;
    spi_slave_transaction_t t;
    uint32_t counter = 0;
    
    // Configuration for the SPI slave interface
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    
    // Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 3,
        .flags = 0,
    };
    
    // Initialize SPI slave interface
    ret = spi_slave_initialize(SPI_SLAVE_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);
    
    ESP_LOGI(TAG, "SPI Slave initialized");
    
    while (1) {
        // Prepare send buffer with counter value
        snprintf((char *)sendbuf, BUFFER_SIZE, "ESP32_CNT:%lu", counter++);
        
        memset(&t, 0, sizeof(t));
        t.length = BUFFER_SIZE * 8;  // Length in bits
        t.tx_buffer = sendbuf;
        t.rx_buffer = recvbuf;
        
        // Wait for transaction
        ret = spi_slave_transmit(SPI_SLAVE_HOST, &t, portMAX_DELAY);
        
        if (ret == ESP_OK) {
            // Check if we received data
            if (t.trans_len > 0) {
                recvbuf[BUFFER_SIZE - 1] = '\0';  // Ensure null termination
                ESP_LOGI(TAG, "Received from STM32: %s", recvbuf);
                
                // Toggle LED on successful transaction
                gpio_set_level(LED_PIN, counter % 2);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    // Configure LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "ESP32 SPI Slave - Controlled by STM32 Master");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MISO: GPIO%d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  MOSI: GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  CLK:  GPIO%d", PIN_NUM_CLK);
    ESP_LOGI(TAG, "  CS:   GPIO%d", PIN_NUM_CS);
    
    xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 5, NULL);
}
