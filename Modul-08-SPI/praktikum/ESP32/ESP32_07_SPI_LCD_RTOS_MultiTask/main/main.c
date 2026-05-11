#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "LCD_RTOS";

// LCD Commands
#define LCD_CMD_SWRESET     0x01
#define LCD_CMD_SLPOUT      0x11
#define LCD_CMD_DISPON      0x29
#define LCD_CMD_CASET       0x2A
#define LCD_CMD_RASET       0x2B
#define LCD_CMD_RAMWR       0x2C
#define LCD_CMD_MADCTL      0x36
#define LCD_CMD_COLMOD      0x3A

// Global variables
static spi_device_handle_t spi_lcd;
static SemaphoreHandle_t lcd_mutex;
static QueueHandle_t sensor_data_queue;
static QueueHandle_t ui_event_queue;

typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_data_t;

typedef struct {
    uint8_t event_type;
    uint16_t x;
    uint16_t y;
    uint32_t value;
} ui_event_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t color;
} lcd_draw_cmd_t;

// LCD Functions
void lcd_cmd(uint8_t cmd) {
    gpio_set_level(LCD_DC_PIN, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_lcd, &t);
}

void lcd_data(uint8_t data) {
    gpio_set_level(LCD_DC_PIN, 1);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(spi_lcd, &t);
}

void lcd_data_buf(uint8_t *data, size_t len) {
    gpio_set_level(LCD_DC_PIN, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_lcd, &t);
}

void lcd_init(void) {
    // Reset LCD
    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Software reset
    lcd_cmd(LCD_CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    lcd_cmd(LCD_CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Color mode: 16-bit
    lcd_cmd(LCD_CMD_COLMOD);
    lcd_data(0x55);

    // Memory access control
    lcd_cmd(LCD_CMD_MADCTL);
    lcd_data(0x00);

    // Display on
    lcd_cmd(LCD_CMD_DISPON);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Backlight on
    gpio_set_level(LCD_BL_PIN, 1);

    ESP_LOGI(TAG, "LCD initialized");
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_cmd(LCD_CMD_CASET);
    lcd_data(x0 >> 8);
    lcd_data(x0 & 0xFF);
    lcd_data(x1 >> 8);
    lcd_data(x1 & 0xFF);

    lcd_cmd(LCD_CMD_RASET);
    lcd_data(y0 >> 8);
    lcd_data(y0 & 0xFF);
    lcd_data(y1 >> 8);
    lcd_data(y1 & 0xFF);

    lcd_cmd(LCD_CMD_RAMWR);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        lcd_set_window(x, y, x + w - 1, y + h - 1);
        
        uint8_t color_buf[2] = {color >> 8, color & 0xFF};
        for (uint32_t i = 0; i < w * h; i++) {
            lcd_data_buf(color_buf, 2);
        }
        
        xSemaphoreGive(lcd_mutex);
    }
}

void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color) {
    // Simple text drawing (8x8 font simulation)
    if (xSemaphoreTake(lcd_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        uint16_t offset = 0;
        while (*text) {
            lcd_fill_rect(x + offset, y, 8, 8, color);
            offset += 10;
            text++;
        }
        xSemaphoreGive(lcd_mutex);
    }
}

// Task 1: LCD Update Task (Highest Priority)
void lcd_update_task(void *pvParameters) {
    ESP_LOGI(TAG, "LCD Update Task started");
    
    while (1) {
        // This task handles critical LCD updates
        // In real application, this would process a display buffer
        
        vTaskDelay(pdMS_TO_TICKS(LCD_UPDATE_INTERVAL));
    }
}

// Task 2: Sensor Read Task
void sensor_read_task(void *pvParameters) {
    ESP_LOGI(TAG, "Sensor Read Task started");
    
    sensor_data_t sensor_data;
    uint32_t counter = 0;
    
    while (1) {
        // Simulate sensor reading
        sensor_data.temperature = 20.0f + (counter % 10);
        sensor_data.humidity = 50.0f + (counter % 20);
        sensor_data.timestamp = xTaskGetTickCount();
        
        // Send to queue
        if (xQueueSend(sensor_data_queue, &sensor_data, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Sensor queue full");
        }
        
        counter++;
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL));
    }
}

// Task 3: UI Task
void ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "UI Task started");
    
    sensor_data_t sensor_data;
    char text_buf[32];
    uint16_t y_pos = 10;
    
    // Colors (RGB565)
    const uint16_t COLOR_BLACK = 0x0000;
    const uint16_t COLOR_WHITE = 0xFFFF;
    const uint16_t COLOR_RED = 0xF800;
    const uint16_t COLOR_GREEN = 0x07E0;
    const uint16_t COLOR_BLUE = 0x001F;
    
    // Clear screen
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_BLACK);
    
    // Draw header
    lcd_fill_rect(0, 0, LCD_WIDTH, 30, COLOR_BLUE);
    lcd_draw_text(10, 10, "ESP32 LCD RTOS", COLOR_WHITE);
    
    while (1) {
        // Check for sensor data
        if (xQueueReceive(sensor_data_queue, &sensor_data, 0) == pdTRUE) {
            // Update temperature display
            lcd_fill_rect(10, 50, 220, 20, COLOR_BLACK);
            snprintf(text_buf, sizeof(text_buf), "Temp: %.1fC", sensor_data.temperature);
            lcd_draw_text(10, 50, text_buf, COLOR_GREEN);
            
            // Update humidity display
            lcd_fill_rect(10, 80, 220, 20, COLOR_BLACK);
            snprintf(text_buf, sizeof(text_buf), "Hum: %.1f%%", sensor_data.humidity);
            lcd_draw_text(10, 80, text_buf, COLOR_GREEN);
            
            ESP_LOGI(TAG, "UI updated: T=%.1f H=%.1f", 
                     sensor_data.temperature, sensor_data.humidity);
        }
        
        // Draw status bar
        lcd_fill_rect(0, LCD_HEIGHT - 20, LCD_WIDTH, 20, COLOR_BLUE);
        snprintf(text_buf, sizeof(text_buf), "Time: %lu", xTaskGetTickCount() / 1000);
        lcd_draw_text(10, LCD_HEIGHT - 15, text_buf, COLOR_WHITE);
        
        vTaskDelay(pdMS_TO_TICKS(UI_UPDATE_INTERVAL));
    }
}

// Task 4: Background Task (Lowest Priority)
void background_task(void *pvParameters) {
    ESP_LOGI(TAG, "Background Task started");
    
    uint32_t counter = 0;
    
    while (1) {
        // Perform background operations
        ESP_LOGI(TAG, "Background task running: %lu", counter++);
        
        // Simulate some work
        vTaskDelay(pdMS_TO_TICKS(BACKGROUND_INTERVAL));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32 LCD RTOS Multi-Task Demo");
    
    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LCD_DC_PIN) | (1ULL << LCD_RST_PIN) | (1ULL << LCD_BL_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_MOSI_PIN,
        .miso_io_num = -1,
        .sclk_io_num = LCD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = LCD_CS_PIN,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi_lcd));
    
    // Initialize LCD
    lcd_init();
    
    // Create synchronization objects
    lcd_mutex = xSemaphoreCreateMutex();
    sensor_data_queue = xQueueCreate(10, sizeof(sensor_data_t));
    ui_event_queue = xQueueCreate(5, sizeof(ui_event_t));
    
    if (lcd_mutex == NULL || sensor_data_queue == NULL || ui_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create synchronization objects");
        return;
    }
    
    // Create tasks with different priorities
    xTaskCreate(lcd_update_task, "LCD_Update", LCD_UPDATE_STACK_SIZE, NULL, 
                LCD_UPDATE_TASK_PRIORITY, NULL);
    
    xTaskCreate(sensor_read_task, "Sensor_Read", SENSOR_READ_STACK_SIZE, NULL, 
                SENSOR_READ_TASK_PRIORITY, NULL);
    
    xTaskCreate(ui_task, "UI_Task", UI_STACK_SIZE, NULL, 
                UI_TASK_PRIORITY, NULL);
    
    xTaskCreate(background_task, "Background", BACKGROUND_STACK_SIZE, NULL, 
                BACKGROUND_TASK_PRIORITY, NULL);
    
    ESP_LOGI(TAG, "All tasks created successfully");
}
