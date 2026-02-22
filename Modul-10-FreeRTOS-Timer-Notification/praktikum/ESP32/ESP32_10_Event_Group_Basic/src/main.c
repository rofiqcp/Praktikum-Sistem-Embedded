/*
 * ESP32_10_Event_Group_Basic
 * ============================
 * xEventGroupSetBits(), xEventGroupWaitBits() with pdTRUE for
 * AND logic (wait for ALL events) and pdFALSE for OR logic.
 * 3 "sensor ready" events, LED on when all ready.
 * Uses simulated sensors (tasks with different delays).
 *
 * Hardware: 1x LED on GPIO2 (3 buttons simulated via tasks)
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "EVTGRP_BASIC";

#define LED_GPIO    GPIO_NUM_2

/* Event bits for 3 sensors */
#define SENSOR1_READY_BIT   (1 << 0)    /* Bit 0 */
#define SENSOR2_READY_BIT   (1 << 1)    /* Bit 1 */
#define SENSOR3_READY_BIT   (1 << 2)    /* Bit 2 */
#define ALL_SENSORS_READY   (SENSOR1_READY_BIT | SENSOR2_READY_BIT | SENSOR3_READY_BIT)

/* Additional event bits for OR demo */
#define ALERT_TEMP_BIT      (1 << 3)
#define ALERT_HUMID_BIT     (1 << 4)
#define ALERT_PRESS_BIT     (1 << 5)
#define ANY_ALERT           (ALERT_TEMP_BIT | ALERT_HUMID_BIT | ALERT_PRESS_BIT)

static EventGroupHandle_t xEventGroup = NULL;
static volatile uint32_t all_ready_count = 0;
static volatile uint32_t any_alert_count = 0;
static volatile uint32_t cycle_count = 0;

static const char *sensor_names[] = { "Temperature", "Humidity", "Pressure" };
static const EventBits_t sensor_bits[] = { SENSOR1_READY_BIT, SENSOR2_READY_BIT, SENSOR3_READY_BIT };
static const EventBits_t alert_bits[] = { ALERT_TEMP_BIT, ALERT_HUMID_BIT, ALERT_PRESS_BIT };

/* Sensor initialization task - each takes different time */
static void sensor_init_task(void *pvParameters)
{
    int sensor_id = (int)(uintptr_t)pvParameters;
    uint32_t base_delay_ms = (sensor_id + 1) * 1000;  /* 1s, 2s, 3s */

    while (1) {
        /* Simulate sensor startup/initialization time */
        uint32_t jitter = esp_random() % 500;
        uint32_t delay = base_delay_ms + jitter;

        ESP_LOGI(TAG, "EVENT,SENSOR_INIT_START,sensor=%d,name=%s,est_time=%lu_ms,tick=%lu",
                 sensor_id, sensor_names[sensor_id],
                 (unsigned long)delay,
                 (unsigned long)xTaskGetTickCount());

        vTaskDelay(pdMS_TO_TICKS(delay));

        /* Sensor is ready - set the corresponding bit */
        EventBits_t bits_before = xEventGroupGetBits(xEventGroup);
        xEventGroupSetBits(xEventGroup, sensor_bits[sensor_id]);
        EventBits_t bits_after = xEventGroupGetBits(xEventGroup);

        ESP_LOGI(TAG, "EVENT,SENSOR_READY,sensor=%d,name=%s,bit=0x%02X,"
                 "bits_before=0x%02lX,bits_after=0x%02lX,tick=%lu",
                 sensor_id, sensor_names[sensor_id],
                 (int)sensor_bits[sensor_id],
                 (unsigned long)bits_before,
                 (unsigned long)bits_after,
                 (unsigned long)xTaskGetTickCount());

        /* Wait for reset before next cycle */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* Alert generator - randomly triggers alerts */
static void alert_generator_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(2000));  /* Initial delay */

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000 + (esp_random() % 4000)));

        /* Pick a random alert */
        int alert_id = esp_random() % 3;
        xEventGroupSetBits(xEventGroup, alert_bits[alert_id]);

        ESP_LOGW(TAG, "EVENT,ALERT_SET,type=%s,bit=0x%02X,tick=%lu",
                 sensor_names[alert_id],
                 (int)alert_bits[alert_id],
                 (unsigned long)xTaskGetTickCount());
    }
}

/* AND logic: wait for ALL sensors to be ready */
static void and_wait_task(void *pvParameters)
{
    while (1) {
        cycle_count++;
        ESP_LOGI(TAG, "EVENT,AND_WAIT_START,cycle=%lu,waiting for ALL 3 sensors,tick=%lu",
                 (unsigned long)cycle_count,
                 (unsigned long)xTaskGetTickCount());

        TickType_t start_tick = xTaskGetTickCount();

        /*
         * xEventGroupWaitBits():
         * - uxBitsToWaitFor: ALL_SENSORS_READY (bits 0,1,2)
         * - xClearOnExit: pdTRUE (clear the bits after unblocking)
         * - xWaitForAllBits: pdTRUE (AND logic - ALL bits must be set)
         * - xTicksToWait: max 15 seconds
         */
        EventBits_t bits = xEventGroupWaitBits(
            xEventGroup,
            ALL_SENSORS_READY,
            pdTRUE,             /* Clear bits on exit */
            pdTRUE,             /* AND: wait for ALL bits */
            pdMS_TO_TICKS(15000)
        );

        TickType_t elapsed = xTaskGetTickCount() - start_tick;

        if ((bits & ALL_SENSORS_READY) == ALL_SENSORS_READY) {
            all_ready_count++;
            gpio_set_level(LED_GPIO, 1);  /* LED ON */

            ESP_LOGW(TAG, "EVENT,ALL_SENSORS_READY,count=%lu,bits=0x%02lX,wait_time=%lu_ms,tick=%lu",
                     (unsigned long)all_ready_count,
                     (unsigned long)bits,
                     (unsigned long)(elapsed * portTICK_PERIOD_MS),
                     (unsigned long)xTaskGetTickCount());

            /* LED on for 2 seconds */
            vTaskDelay(pdMS_TO_TICKS(2000));
            gpio_set_level(LED_GPIO, 0);
        } else {
            ESP_LOGE(TAG, "EVENT,AND_WAIT_TIMEOUT,bits=0x%02lX,missing=0x%02lX,tick=%lu",
                     (unsigned long)bits,
                     (unsigned long)(ALL_SENSORS_READY & ~bits),
                     (unsigned long)xTaskGetTickCount());
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* OR logic: wait for ANY alert */
static void or_wait_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "EVENT,OR_WAIT_START,waiting for ANY alert,tick=%lu",
                 (unsigned long)xTaskGetTickCount());

        /*
         * xWaitForAllBits: pdFALSE (OR logic - ANY bit triggers)
         */
        EventBits_t bits = xEventGroupWaitBits(
            xEventGroup,
            ANY_ALERT,
            pdTRUE,             /* Clear on exit */
            pdFALSE,            /* OR: wait for ANY bit */
            portMAX_DELAY
        );

        any_alert_count++;

        const char *alert_name = "UNKNOWN";
        if (bits & ALERT_TEMP_BIT) alert_name = "Temperature";
        else if (bits & ALERT_HUMID_BIT) alert_name = "Humidity";
        else if (bits & ALERT_PRESS_BIT) alert_name = "Pressure";

        ESP_LOGW(TAG, "EVENT,ALERT_RECEIVED,type=%s,bits=0x%02lX,count=%lu,tick=%lu",
                 alert_name,
                 (unsigned long)bits,
                 (unsigned long)any_alert_count,
                 (unsigned long)xTaskGetTickCount());

        /* Quick LED blink for alert */
        for (int i = 0; i < 3; i++) {
            gpio_set_level(LED_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Event Group Basic Demo ===");
    ESP_LOGI(TAG, "INFO,AND logic: Wait for ALL 3 sensors ready -> LED ON");
    ESP_LOGI(TAG, "INFO,OR logic: Wait for ANY alert -> LED blink");
    ESP_LOGI(TAG, "INFO,Sensor bits: 0x01=Temp, 0x02=Humid, 0x04=Press");
    ESP_LOGI(TAG, "INFO,Alert bits: 0x08=TempAlert, 0x10=HumidAlert, 0x20=PressAlert");

    /* Configure LED */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_GPIO, 0);

    /* Create event group */
    xEventGroup = xEventGroupCreate();
    if (xEventGroup == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create event group!");
        return;
    }

    /* Create sensor tasks */
    for (int i = 0; i < 3; i++) {
        char name[20];
        snprintf(name, sizeof(name), "sensor_%d", i);
        xTaskCreate(sensor_init_task, name, 4096, (void *)(uintptr_t)i, 5, NULL);
    }

    /* Create AND/OR wait tasks */
    xTaskCreate(and_wait_task, "and_wait", 4096, NULL, 8, NULL);
    xTaskCreate(or_wait_task, "or_wait", 4096, NULL, 8, NULL);

    /* Create alert generator */
    xTaskCreate(alert_generator_task, "alert_gen", 2048, NULL, 3, NULL);

    /* Monitor */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        EventBits_t current = xEventGroupGetBits(xEventGroup);
        ESP_LOGI(TAG, "STATUS,current_bits=0x%02lX,all_ready_count=%lu,alerts=%lu,cycles=%lu,tick=%lu",
                 (unsigned long)current,
                 (unsigned long)all_ready_count,
                 (unsigned long)any_alert_count,
                 (unsigned long)cycle_count,
                 (unsigned long)xTaskGetTickCount());
    }
}
