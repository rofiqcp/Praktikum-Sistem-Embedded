#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "SPI_MASTER";

spi_device_handle_t spi;

void spi_master_init(void)
{
    esp_err_t ret;
    
    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BUFFER_SIZE,
    };
    
    // Configuration for the SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .flags = 0,
    };
    
    // Initialize the SPI bus
    ret = spi_bus_initialize(SPI_MASTER_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    
    // Attach the device to the SPI bus
    ret = spi_bus_add_device(SPI_MASTER_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "SPI Master initialized");
}

void spi_master_task(void *pvParameters)
{
    uint8_t txBuffer[BUFFER_SIZE];
    uint8_t rxBuffer[BUFFER_SIZE];
    uint32_t counter = 0;
    
    while (1) {
        // Prepare data to send
        snprintf((char *)txBuffer, BUFFER_SIZE, "ESP32_CMD:%lu", counter++);
        memset(rxBuffer, 0, BUFFER_SIZE);
        
        // Prepare transaction
        spi_transaction_t t = {
            .length = BUFFER_SIZE * 8,  // Length in bits
            .tx_buffer = txBuffer,
            .rx_buffer = rxBuffer,
        };
        
        // Execute transaction
        esp_err_t ret = spi_device_transmit(spi, &t);
        
        if (ret == ESP_OK) {
            rxBuffer[BUFFER_SIZE - 1] = '\0';
            ESP_LOGI(TAG, "Sent: %s", txBuffer);
            ESP_LOGI(TAG, "Received from STM32: %s", rxBuffer);
            
            // Toggle LED
            gpio_set_level(LED_PIN, counter % 2);
        } else {
            ESP_LOGE(TAG, "SPI transaction failed");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // Configure LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "ESP32 SPI Master - Controls STM32 Slave");
    ESP_LOGI(TAG, "Pin Configuration:");
    ESP_LOGI(TAG, "  MISO: GPIO%d", PIN_NUM_MISO);
    ESP_LOGI(TAG, "  MOSI: GPIO%d", PIN_NUM_MOSI);
    ESP_LOGI(TAG, "  CLK:  GPIO%d", PIN_NUM_CLK);
    ESP_LOGI(TAG, "  CS:   GPIO%d", PIN_NUM_CS);
    
    spi_master_init();
    
    xTaskCreate(spi_master_task, "spi_master_task", 4096, NULL, 5, NULL);
}
