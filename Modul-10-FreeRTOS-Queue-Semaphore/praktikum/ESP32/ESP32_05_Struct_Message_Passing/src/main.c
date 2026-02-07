/**
 * ESP32_05: Struct Message Passing via Queue
 * ============================================
 * Modul 10 - FreeRTOS Queue dan Semaphore
 * Framework: ESP-IDF
 *
 * Konsep: Mengirim struct kompleks (SensorData) melalui Queue.
 *         Multiple producer (sensor tasks) ke single consumer.
 *
 * Hardware: ESP32 DevKit V1 + Potensiometer di GPIO34 (ADC)
 *   - Potensiometer 10K: VCC-Wiper-GND, Wiper ke GPIO34
 *   - USB Serial Monitor (115200)
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "STRUCT_Q";

typedef enum { SENSOR_TEMP, SENSOR_LIGHT, SENSOR_POT } SensorType_t;
typedef struct {
    SensorType_t type;
    float        value;
    uint32_t     timestamp;
    uint8_t      sensor_id;
} SensorData_t;

static QueueHandle_t xSensorQueue = NULL;
static SemaphoreHandle_t xPrintMutex = NULL;
static adc_oneshot_unit_handle_t adc_handle = NULL;

static const char *sensor_names[] = {"TEMP", "LIGHT", "POT"};

static void vSensorTask(void *pv)
{
    int id = (int)(intptr_t)pv;
    SensorData_t data;
    data.sensor_id = id;
    for (;;) {
        data.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
        switch (id) {
            case 0: /* Internal temp sensor (simulated) */
                data.type = SENSOR_TEMP;
                data.value = 25.0f + (float)(esp_random() % 100) / 10.0f;
                break;
            case 1: /* Light sensor (simulated) */
                data.type = SENSOR_LIGHT;
                data.value = (float)(esp_random() % 1000);
                break;
            case 2: /* Potentiometer via ADC */
                data.type = SENSOR_POT;
                int raw = 0;
                if (adc_handle) adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &raw);
                data.value = (float)raw * 3.3f / 4095.0f;
                break;
        }
        if (xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(TAG, "Sensor %d: queue full!", id);
        }
        vTaskDelay(pdMS_TO_TICKS(500 + id * 200));
    }
}

static void vConsumerTask(void *pv)
{
    SensorData_t data;
    uint32_t counts[3] = {0};
    for (;;) {
        if (xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdPASS) {
            counts[data.type]++;
            xSemaphoreTake(xPrintMutex, portMAX_DELAY);
            printf("[Consumer] %-5s | val=%.2f | t=%lu ms | #%lu\n",
                   sensor_names[data.type], data.value,
                   (unsigned long)data.timestamp,
                   (unsigned long)counts[data.type]);
            xSemaphoreGive(xPrintMutex);
        }
    }
}

void app_main(void)
{
    printf("\n=========================================================\n");
    printf("  ESP32_05: Struct Message Passing via Queue\n");
    printf("  Multiple producers -> single consumer\n");
    printf("=========================================================\n\n");

    /* ADC init for potentiometer on GPIO34 (ADC1_CH6) */
    adc_oneshot_unit_init_cfg_t adc_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&adc_cfg, &adc_handle);
    adc_oneshot_chan_cfg_t ch_cfg = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &ch_cfg);

    xSensorQueue = xQueueCreate(15, sizeof(SensorData_t));
    xPrintMutex  = xSemaphoreCreateMutex();

    for (int i = 0; i < 3; i++)
        xTaskCreate(vSensorTask, "Sensor", 4096, (void*)(intptr_t)i, 2, NULL);
    xTaskCreate(vConsumerTask, "Consumer", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "3 sensors -> 1 consumer via queue.");
}
