/*
 * ESP32_05_Timer_Timeout_Monitor
 * ===============================
 * Communication timeout/heartbeat monitor.
 * Timer resets on each "heartbeat" received via serial.
 * If timer expires (no heartbeat for 5s), LED blinks warning.
 * Use xTimerReset() in heartbeat handler.
 *
 * Hardware: Serial (UART0), 1x LED on GPIO2
 * Framework: ESP-IDF with FreeRTOS
 *
 * Send "heartbeat" or "hb" via serial monitor to reset the watchdog.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "TIMEOUT_MON";

#define LED_GPIO            GPIO_NUM_2
#define TIMEOUT_MS          5000
#define WARNING_BLINK_MS    200
#define UART_BUF_SIZE       256

static TimerHandle_t xTimeoutTimer = NULL;
static volatile bool timeout_occurred = false;
static volatile uint32_t heartbeat_count = 0;
static volatile uint32_t timeout_count = 0;
static volatile TickType_t last_heartbeat_tick = 0;
static TaskHandle_t xWarningTaskHandle = NULL;

static void timeout_timer_callback(TimerHandle_t xTimer)
{
    timeout_occurred = true;
    timeout_count++;

    ESP_LOGW(TAG, "EVENT,TIMEOUT,count=%lu,no_heartbeat_for=%d_ms,last_hb_tick=%lu,tick=%lu",
             (unsigned long)timeout_count,
             TIMEOUT_MS,
             (unsigned long)last_heartbeat_tick,
             (unsigned long)xTaskGetTickCount());

    /* Notify warning task to start blinking */
    if (xWarningTaskHandle != NULL) {
        xTaskNotifyGive(xWarningTaskHandle);
    }
}

static void warning_blink_task(void *pvParameters)
{
    while (1) {
        /* Wait for timeout notification */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGW(TAG, "WARNING,LED_BLINK_START,timeout detected - blinking LED");

        /* Blink LED rapidly until heartbeat received */
        while (timeout_occurred) {
            gpio_set_level(LED_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(WARNING_BLINK_MS));
            gpio_set_level(LED_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(WARNING_BLINK_MS));
        }

        ESP_LOGI(TAG, "WARNING,LED_BLINK_STOP,heartbeat received - LED off");
        gpio_set_level(LED_GPIO, 0);
    }
}

static void heartbeat_receiver_task(void *pvParameters)
{
    /* Configure UART */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, UART_BUF_SIZE * 2, 0, 0, NULL, 0);

    uint8_t data[UART_BUF_SIZE];

    ESP_LOGI(TAG, "INFO,Send 'heartbeat' or 'hb' via serial to reset timeout");

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';

            /* Trim newlines */
            char *str = (char *)data;
            while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
                str[--len] = '\0';
            }

            if (len == 0) continue;

            ESP_LOGI(TAG, "EVENT,SERIAL_RX,data=%s,len=%d", str, len);

            /* Check for heartbeat command */
            if (strstr(str, "heartbeat") != NULL || strstr(str, "hb") != NULL) {
                heartbeat_count++;
                timeout_occurred = false;
                last_heartbeat_tick = xTaskGetTickCount();

                /* Reset the timeout timer */
                if (xTimerReset(xTimeoutTimer, pdMS_TO_TICKS(100)) == pdPASS) {
                    /* Turn LED solid ON to indicate healthy */
                    gpio_set_level(LED_GPIO, 1);

                    ESP_LOGI(TAG, "EVENT,HEARTBEAT,count=%lu,timer_reset=OK,tick=%lu",
                             (unsigned long)heartbeat_count,
                             (unsigned long)last_heartbeat_tick);
                } else {
                    ESP_LOGE(TAG, "ERROR,Failed to reset timeout timer");
                }
            } else {
                ESP_LOGW(TAG, "EVENT,UNKNOWN_CMD,data=%s", str);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Timer Timeout Monitor Demo ===");
    ESP_LOGI(TAG, "INFO,Timeout period: %d ms", TIMEOUT_MS);
    ESP_LOGI(TAG, "INFO,LED GPIO: %d", LED_GPIO);
    ESP_LOGI(TAG, "INFO,Send 'heartbeat' or 'hb' via serial to reset watchdog");
    ESP_LOGI(TAG, "INFO,If no heartbeat for %d ms, LED blinks warning", TIMEOUT_MS);

    /* Configure LED */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_GPIO, 1);  /* Start with LED ON (healthy) */

    /* Create timeout timer (one-shot, restarts on each heartbeat) */
    xTimeoutTimer = xTimerCreate(
        "TimeoutTimer",
        pdMS_TO_TICKS(TIMEOUT_MS),
        pdFALSE,       /* One-shot */
        NULL,
        timeout_timer_callback
    );

    if (xTimeoutTimer == NULL) {
        ESP_LOGE(TAG, "ERROR,Failed to create timeout timer!");
        return;
    }

    /* Start the timeout timer */
    xTimerStart(xTimeoutTimer, pdMS_TO_TICKS(100));
    last_heartbeat_tick = xTaskGetTickCount();
    ESP_LOGI(TAG, "INFO,Timeout timer started. Send heartbeat within %d ms!", TIMEOUT_MS);

    /* Create tasks */
    xTaskCreate(warning_blink_task, "warning_blink", 2048, NULL, 5, &xWarningTaskHandle);
    xTaskCreate(heartbeat_receiver_task, "hb_receiver", 4096, NULL, 10, NULL);

    /* Monitor task */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));

        TickType_t now = xTaskGetTickCount();
        TickType_t since_last = now - last_heartbeat_tick;

        ESP_LOGI(TAG, "STATUS,heartbeats=%lu,timeouts=%lu,state=%s,ms_since_last_hb=%lu,tick=%lu",
                 (unsigned long)heartbeat_count,
                 (unsigned long)timeout_count,
                 timeout_occurred ? "TIMEOUT" : "HEALTHY",
                 (unsigned long)(since_last * portTICK_PERIOD_MS),
                 (unsigned long)now);
    }
}
