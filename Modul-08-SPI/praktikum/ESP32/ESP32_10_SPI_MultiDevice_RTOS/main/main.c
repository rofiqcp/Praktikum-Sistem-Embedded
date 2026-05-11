/* ESP32_10_SPI_MultiDevice_RTOS
 * Mengoperasikan 3 perangkat SPI dengan FreeRTOS:
 *   - LCD ST7735/ILI9341 (SPI2_HOST, CS=5, DC=2, RST=4)
 *   - SD Card           (SPI2_HOST, CS=13) - shared bus with LCD
 *   - W5500 Ethernet    (SPI3_HOST, CS=14, INT=27)
 *
 * Task:
 *   coordinator_task - generate data, distribusi ke queue
 *   lcd_task         - render data ke LCD
 *   sd_task          - log data ke SD card
 *   eth_task         - kirim data via Ethernet
 *   monitor_task     - cetak statistik ke UART
 *
 * Menggunakan built-in FreeRTOS dari ESP-IDF
 * Entry point: void app_main(void)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "config.h"

static const char *TAG = "MULTI_RTOS";

/* =========================================================
 * Shared data structure
 * ========================================================= */
typedef struct {
    uint32_t timestamp_ms;
    int32_t  temperature_x100;  /* 2512 = 25.12 degC */
    int32_t  humidity_x100;     /* 6234 = 62.34 % */
    uint32_t counter;
    uint32_t free_heap;
} SensorData_t;

typedef struct {
    char     text[64];
    uint8_t  row;
} LCDMsg_t;

/* =========================================================
 * FreeRTOS objects
 * ========================================================= */
static QueueHandle_t     xLCDQueue   = NULL;
static QueueHandle_t     xSDQueue    = NULL;
static QueueHandle_t     xEthQueue   = NULL;
static SemaphoreHandle_t xSPI2Mutex  = NULL; /* LCD + SD share SPI2 */

/* =========================================================
 * Stats
 * ========================================================= */
static volatile uint32_t lcd_renders  = 0;
static volatile uint32_t sd_writes    = 0;
static volatile uint32_t eth_sends    = 0;
static volatile uint32_t sensor_ticks = 0;

/* =========================================================
 * SPI handles
 * ========================================================= */
static spi_device_handle_t lcd_spi  = NULL;
static spi_device_handle_t sd_spi   = NULL;
static spi_device_handle_t eth_spi  = NULL;

/* =========================================================
 * LCD helper functions (SPI2_HOST)
 * ========================================================= */
static void lcd_send_cmd(uint8_t cmd) {
    gpio_set_level(LCD_DC_PIN, 0);
    spi_transaction_t t = {
        .length    = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(lcd_spi, &t);
}

static void lcd_send_data(uint8_t data) {
    gpio_set_level(LCD_DC_PIN, 1);
    spi_transaction_t t = {
        .length    = 8,
        .tx_buffer = &data,
    };
    spi_device_polling_transmit(lcd_spi, &t);
}

static void lcd_init(void) {
    /* Hardware reset */
    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_send_cmd(0x01); /* Software reset */
    vTaskDelay(pdMS_TO_TICKS(150));
    lcd_send_cmd(0x11); /* Sleep out */
    vTaskDelay(pdMS_TO_TICKS(200));
    lcd_send_cmd(0x3A); /* Color mode */
    lcd_send_data(0x05); /* 16-bit */
    lcd_send_cmd(0x29); /* Display on */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Backlight on */
    if (LCD_BL_PIN >= 0) {
        gpio_set_level(LCD_BL_PIN, 1);
    }

    ESP_LOGI(TAG, "[LCD] Initialized");
}

static void lcd_write_text_row(uint8_t row, const char *text) {
    /* Simplified: just send RAMWR command + dummy pixels */
    (void)row;
    (void)text;
    lcd_send_cmd(0x2C); /* RAM write */
    gpio_set_level(LCD_DC_PIN, 1);
    /* Write 32 white pixels (16-bit) */
    uint8_t pix[2] = {0xFF, 0xFF};
    for (int i = 0; i < 32; i++) {
        spi_transaction_t t = {.length = 16, .tx_buffer = pix};
        spi_device_polling_transmit(lcd_spi, &t);
    }
}

/* =========================================================
 * SD Card helper (SPI2_HOST shared with LCD)
 * ========================================================= */
static void sd_send_dummy_bytes(int n) {
    uint8_t dummy = 0xFF;
    spi_transaction_t t = {.length = 8, .tx_buffer = &dummy};
    for (int i = 0; i < n; i++) spi_device_polling_transmit(sd_spi, &t);
}

static bool sd_init(void) {
    /* Send 80 CLK pulses with CS high */
    spi_device_acquire_bus(sd_spi, portMAX_DELAY);
    gpio_set_level(SD_CS_PIN, 1);
    sd_send_dummy_bytes(10);

    /* CMD0: GO_IDLE_STATE */
    uint8_t cmd0[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    gpio_set_level(SD_CS_PIN, 0);
    spi_transaction_t t = {.length = 48, .tx_buffer = cmd0};
    spi_device_polling_transmit(sd_spi, &t);

    uint8_t resp = 0xFF;
    for (int i = 0; i < 8; i++) {
        spi_transaction_t rt = {.length = 8, .rxlength = 8, .rx_buffer = &resp};
        spi_device_polling_transmit(sd_spi, &rt);
        if (resp != 0xFF) break;
    }
    gpio_set_level(SD_CS_PIN, 1);
    spi_device_release_bus(sd_spi);

    bool ok = (resp == 0x01);
    ESP_LOGI(TAG, "[SD] Init %s (R1=0x%02X)", ok ? "OK" : "FAILED", resp);
    return ok;
}

static void sd_write_log(const SensorData_t *data) {
    char entry[64];
    snprintf(entry, sizeof(entry), "T=%lu,Temp=%ld,Hum=%ld\r\n",
             (unsigned long)data->timestamp_ms,
             (long)data->temperature_x100,
             (long)data->humidity_x100);
    /* Real implementation would use FatFS / SPIFFS */
    (void)entry;
    /* Simulate: just pulse CS */
    gpio_set_level(SD_CS_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(SD_CS_PIN, 1);
}

/* =========================================================
 * W5500 helper (SPI3_HOST)
 * ========================================================= */
static void w5500_write_reg(uint16_t addr, uint8_t bsb_raw, uint8_t data) {
    uint8_t buf[4] = {
        (uint8_t)(addr >> 8),
        (uint8_t)(addr & 0xFF),
        (uint8_t)((bsb_raw << 3) | 0x04), /* write */
        data
    };
    spi_transaction_t t = {.length = 32, .tx_buffer = buf};
    spi_device_polling_transmit(eth_spi, &t);
}

static void w5500_write_burst(uint16_t addr, uint8_t bsb_raw,
                               const uint8_t *data, size_t len) {
    uint8_t header[3] = {
        (uint8_t)(addr >> 8),
        (uint8_t)(addr & 0xFF),
        (uint8_t)((bsb_raw << 3) | 0x04)
    };
    spi_transaction_t th = {.length = 24, .tx_buffer = header};
    spi_transaction_t td = {.length = len * 8, .tx_buffer = data};
    spi_device_acquire_bus(eth_spi, portMAX_DELAY);
    gpio_set_level(ETH_CS_PIN, 0);
    spi_device_polling_transmit(eth_spi, &th);
    spi_device_polling_transmit(eth_spi, &td);
    gpio_set_level(ETH_CS_PIN, 1);
    spi_device_release_bus(eth_spi);
}

static void w5500_init(void) {
    /* Hardware reset */
    if (ETH_RST_PIN >= 0) {
        gpio_set_level(ETH_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(ETH_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    /* SW reset */
    w5500_write_reg(0x0000, 0, 0x80);
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Set MAC */
    const uint8_t mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    w5500_write_burst(0x0009, 0, mac, 6);

    /* Set IP: 192.168.1.100 */
    const uint8_t ip[4] = {192, 168, 1, 100};
    w5500_write_burst(0x000F, 0, ip, 4);

    ESP_LOGI(TAG, "[ETH] W5500 init OK, IP=192.168.1.100");
}

static void w5500_send_packet(const SensorData_t *data) {
    uint8_t pkt[8];
    pkt[0] = 0xDE; pkt[1] = 0xAD;
    pkt[2] = (uint8_t)(data->counter >> 24);
    pkt[3] = (uint8_t)(data->counter >> 16);
    pkt[4] = (uint8_t)(data->counter >> 8);
    pkt[5] = (uint8_t)(data->counter);
    pkt[6] = (uint8_t)(data->temperature_x100 >> 8);
    pkt[7] = (uint8_t)(data->temperature_x100);

    gpio_set_level(ETH_CS_PIN, 0);
    spi_transaction_t t = {.length = 64, .tx_buffer = pkt};
    spi_device_polling_transmit(eth_spi, &t);
    gpio_set_level(ETH_CS_PIN, 1);
}

/* =========================================================
 * FreeRTOS Tasks
 * ========================================================= */
static void coordinator_task(void *pvParameters)
{
    SensorData_t data;
    uint32_t count = 0;

    ESP_LOGI(TAG, "Coordinator task started");

    for (;;) {
        count++;
        data.counter          = count;
        data.timestamp_ms     = (uint32_t)(esp_timer_get_time() / 1000);
        data.temperature_x100 = 2500 + (int32_t)(count % 500);
        data.humidity_x100    = 6000 + (int32_t)(count % 3000);
        data.free_heap        = esp_get_free_heap_size();
        sensor_ticks          = count;

        /* Distribute to all queues (non-blocking) */
        LCDMsg_t lcd_msg;
        snprintf(lcd_msg.text, sizeof(lcd_msg.text),
                 "T=%ld.%02ld H=%ld.%02ld",
                 (long)(data.temperature_x100 / 100),
                 (long)(data.temperature_x100 % 100),
                 (long)(data.humidity_x100 / 100),
                 (long)(data.humidity_x100 % 100));
        lcd_msg.row = 0;
        xQueueSend(xLCDQueue, &lcd_msg, 0);

        xQueueSend(xSDQueue,  &data, 0);
        xQueueSend(xEthQueue, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void lcd_task(void *pvParameters)
{
    LCDMsg_t msg;

    ESP_LOGI(TAG, "LCD task started");
    lcd_init();

    for (;;) {
        if (xQueueReceive(xLCDQueue, &msg, pdMS_TO_TICKS(2000)) == pdTRUE) {
            if (xSemaphoreTake(xSPI2Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                lcd_write_text_row(msg.row, msg.text);
                lcd_renders++;
                xSemaphoreGive(xSPI2Mutex);
                ESP_LOGD(TAG, "[LCD] Row%d: %s", msg.row, msg.text);
            }
        }
    }
}

static void sd_task(void *pvParameters)
{
    SensorData_t data;

    ESP_LOGI(TAG, "SD task started");

    /* Wait for SPI2 mutex for SD init */
    if (xSemaphoreTake(xSPI2Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        sd_init();
        xSemaphoreGive(xSPI2Mutex);
    }

    for (;;) {
        if (xQueueReceive(xSDQueue, &data, pdMS_TO_TICKS(5000)) == pdTRUE) {
            if (xSemaphoreTake(xSPI2Mutex, pdMS_TO_TICKS(300)) == pdTRUE) {
                sd_write_log(&data);
                sd_writes++;
                xSemaphoreGive(xSPI2Mutex);
                ESP_LOGD(TAG, "[SD] Wrote entry #%lu", (unsigned long)sd_writes);
            }
        }
    }
}

static void eth_task(void *pvParameters)
{
    SensorData_t data;

    ESP_LOGI(TAG, "ETH task started");
    w5500_init();

    for (;;) {
        if (xQueueReceive(xEthQueue, &data, pdMS_TO_TICKS(3000)) == pdTRUE) {
            w5500_send_packet(&data);
            eth_sends++;
            ESP_LOGD(TAG, "[ETH] Sent packet #%lu", (unsigned long)eth_sends);
        }
    }
}

static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor task started");

    for (;;) {
        ESP_LOGI(TAG, "=== Status ===");
        ESP_LOGI(TAG, "  Sensor: %lu | LCD: %lu | SD: %lu | ETH: %lu",
                 (unsigned long)sensor_ticks,
                 (unsigned long)lcd_renders,
                 (unsigned long)sd_writes,
                 (unsigned long)eth_sends);
        ESP_LOGI(TAG, "  Free heap: %lu bytes",
                 (unsigned long)esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* =========================================================
 * Hardware initialization
 * ========================================================= */
static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    /* Output pins: LCD CS, DC, RST, BL, SD CS, ETH CS */
    uint64_t out_mask = (1ULL << LCD_CS_PIN) | (1ULL << LCD_DC_PIN) |
                        (1ULL << LCD_RST_PIN) | (1ULL << SD_CS_PIN)  |
                        (1ULL << ETH_CS_PIN);
    if (LCD_BL_PIN >= 0) out_mask |= (1ULL << (LCD_BL_PIN & 63));
    if (ETH_RST_PIN >= 0) out_mask |= (1ULL << (ETH_RST_PIN & 63));

    io_conf.pin_bit_mask = out_mask;
    gpio_config(&io_conf);

    /* Set CS pins high (inactive) */
    gpio_set_level(LCD_CS_PIN, 1);
    gpio_set_level(SD_CS_PIN,  1);
    gpio_set_level(ETH_CS_PIN, 1);
    gpio_set_level(LCD_DC_PIN, 1);
    gpio_set_level(LCD_RST_PIN, 1);
    if (ETH_RST_PIN >= 0) gpio_set_level(ETH_RST_PIN, 1);
    if (LCD_BL_PIN >= 0)  gpio_set_level(LCD_BL_PIN,  0);

    /* ETH INT as input */
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << ETH_INT_PIN);
    gpio_config(&io_conf);
}

static void spi_buses_init(void)
{
    /* SPI2_HOST: LCD + SD (shared bus) */
    spi_bus_config_t bus2 = {
        .mosi_io_num     = SPI_MOSI_PIN,
        .miso_io_num     = SPI_MISO_PIN,
        .sclk_io_num     = SPI_SCLK_PIN,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus2, SPI_DMA_CH_AUTO));

    /* LCD device on SPI2 */
    spi_device_interface_config_t lcd_cfg = {
        .clock_speed_hz  = 40 * 1000 * 1000,
        .mode            = 0,
        .spics_io_num    = LCD_CS_PIN,
        .queue_size      = 7,
        .pre_cb          = NULL,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &lcd_cfg, &lcd_spi));

    /* SD device on SPI2 (slower: 400kHz for init) */
    spi_device_interface_config_t sd_cfg = {
        .clock_speed_hz  = 400 * 1000,
        .mode            = 0,
        .spics_io_num    = SD_CS_PIN,
        .queue_size      = 4,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &sd_cfg, &sd_spi));

    /* SPI3_HOST: W5500 Ethernet */
    spi_bus_config_t bus3 = {
        .mosi_io_num     = SPI_MOSI_PIN,
        .miso_io_num     = SPI_MISO_PIN,
        .sclk_io_num     = SPI_SCLK_PIN,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_HOST, &bus3, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t eth_cfg = {
        .clock_speed_hz  = 20 * 1000 * 1000,
        .mode            = 0,
        .spics_io_num    = ETH_CS_PIN,
        .queue_size      = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(ETH_HOST, &eth_cfg, &eth_spi));

    ESP_LOGI(TAG, "SPI buses initialized");
}

/* =========================================================
 * app_main
 * ========================================================= */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 SPI MultiDevice RTOS ===");

    /* Hardware init */
    gpio_init();
    spi_buses_init();

    /* Create FreeRTOS objects */
    xSPI2Mutex = xSemaphoreCreateMutex();
    xLCDQueue  = xQueueCreate(LCD_QUEUE_SIZE, sizeof(LCDMsg_t));
    xSDQueue   = xQueueCreate(SD_QUEUE_SIZE,  sizeof(SensorData_t));
    xEthQueue  = xQueueCreate(ETH_QUEUE_SIZE, sizeof(SensorData_t));

    if (!xSPI2Mutex || !xLCDQueue || !xSDQueue || !xEthQueue) {
        ESP_LOGE(TAG, "Failed to create RTOS objects!");
        return;
    }

    /* Create tasks */
    xTaskCreate(coordinator_task, "coordinator", TASK_STACK_SIZE, NULL,
                COORDINATOR_TASK_PRIORITY, NULL);
    xTaskCreate(lcd_task,         "lcd",         TASK_STACK_SIZE, NULL,
                LCD_TASK_PRIORITY,         NULL);
    xTaskCreate(sd_task,          "sd",          TASK_STACK_SIZE, NULL,
                SD_TASK_PRIORITY,          NULL);
    xTaskCreate(eth_task,         "eth",         TASK_STACK_SIZE, NULL,
                ETH_TASK_PRIORITY,         NULL);
    xTaskCreate(monitor_task,     "monitor",     TASK_STACK_SIZE / 2, NULL,
                MONITOR_TASK_PRIORITY,     NULL);

    ESP_LOGI(TAG, "All tasks created");
}
