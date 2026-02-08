/*
 * ESP32_07_Notification_Value
 * ============================
 * xTaskNotify() with eSetValueWithOverwrite and eSetValueWithoutOverwrite.
 * Send different command values. Receiver uses xTaskNotifyWait()
 * to get value and clear bits.
 *
 * Hardware: Serial (UART0)
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "NOTIFY_VALUE";

/* Command definitions as notification values */
#define CMD_LED_ON      0x01
#define CMD_LED_OFF     0x02
#define CMD_STATUS      0x03
#define CMD_RESET       0x04
#define CMD_SPEED_UP    0x10
#define CMD_SPEED_DOWN  0x20
#define CMD_CUSTOM      0xFF

#define UART_BUF_SIZE   256

static TaskHandle_t xReceiverTaskHandle = NULL;
static volatile uint32_t cmd_received_count = 0;
static volatile uint32_t overwrite_count = 0;
static volatile uint32_t no_overwrite_count = 0;
static volatile uint32_t no_overwrite_fail_count = 0;

static const char* cmd_to_string(uint32_t cmd)
{
    switch (cmd) {
        case CMD_LED_ON:      return "LED_ON";
        case CMD_LED_OFF:     return "LED_OFF";
        case CMD_STATUS:      return "STATUS";
        case CMD_RESET:       return "RESET";
        case CMD_SPEED_UP:    return "SPEED_UP";
        case CMD_SPEED_DOWN:  return "SPEED_DOWN";
        case CMD_CUSTOM:      return "CUSTOM";
        default:              return "UNKNOWN";
    }
}

static void receiver_task(void *pvParameters)
{
    uint32_t notification_value;

    ESP_LOGI(TAG, "INFO,Receiver task started, waiting for notifications...");

    while (1) {
        /*
         * xTaskNotifyWait():
         * - ulBitsToClearOnEntry: bits to clear before checking (0 = don't clear)
         * - ulBitsToClearOnExit: bits to clear after reading (0xFFFFFFFF = clear all)
         * - pulNotificationValue: receives the notification value
         * - xTicksToWait: how long to wait
         */
        if (xTaskNotifyWait(
                0x00,               /* Don't clear bits on entry */
                0xFFFFFFFF,         /* Clear ALL bits on exit */
                &notification_value,
                portMAX_DELAY) == pdTRUE)
        {
            cmd_received_count++;

            ESP_LOGI(TAG, "EVENT,CMD_RECEIVED,value=0x%02lX,name=%s,count=%lu,tick=%lu",
                     (unsigned long)notification_value,
                     cmd_to_string(notification_value),
                     (unsigned long)cmd_received_count,
                     (unsigned long)xTaskGetTickCount());

            /* Process the command */
            switch (notification_value) {
                case CMD_LED_ON:
                    ESP_LOGI(TAG, "ACTION,LED would turn ON");
                    break;
                case CMD_LED_OFF:
                    ESP_LOGI(TAG, "ACTION,LED would turn OFF");
                    break;
                case CMD_STATUS:
                    ESP_LOGI(TAG, "ACTION,Status report: cmds=%lu,overwrites=%lu,no_overwrite=%lu,fails=%lu",
                             (unsigned long)cmd_received_count,
                             (unsigned long)overwrite_count,
                             (unsigned long)no_overwrite_count,
                             (unsigned long)no_overwrite_fail_count);
                    break;
                case CMD_RESET:
                    ESP_LOGW(TAG, "ACTION,Resetting counters");
                    cmd_received_count = 0;
                    overwrite_count = 0;
                    no_overwrite_count = 0;
                    no_overwrite_fail_count = 0;
                    break;
                default:
                    ESP_LOGI(TAG, "ACTION,Custom command processed: 0x%02lX",
                             (unsigned long)notification_value);
                    break;
            }
        }
    }
}

static void sender_task(void *pvParameters)
{
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

    ESP_LOGI(TAG, "INFO,Available commands via serial:");
    ESP_LOGI(TAG, "INFO,  'on'     - LED_ON  (eSetValueWithOverwrite)");
    ESP_LOGI(TAG, "INFO,  'off'    - LED_OFF (eSetValueWithOverwrite)");
    ESP_LOGI(TAG, "INFO,  'status' - STATUS  (eSetValueWithoutOverwrite)");
    ESP_LOGI(TAG, "INFO,  'reset'  - RESET   (eSetValueWithOverwrite)");
    ESP_LOGI(TAG, "INFO,  'up'     - SPEED_UP  (eSetValueWithoutOverwrite)");
    ESP_LOGI(TAG, "INFO,  'down'   - SPEED_DOWN (eSetValueWithoutOverwrite)");
    ESP_LOGI(TAG, "INFO,  'auto'   - Run automatic demo sequence");

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            char *str = (char *)data;
            while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
                str[--len] = '\0';
            }
            if (len == 0) continue;

            ESP_LOGI(TAG, "EVENT,SERIAL_RX,cmd=%s", str);

            if (strcmp(str, "on") == 0) {
                overwrite_count++;
                xTaskNotify(xReceiverTaskHandle, CMD_LED_ON, eSetValueWithOverwrite);
                ESP_LOGI(TAG, "SEND,value=0x%02X,mode=eSetValueWithOverwrite", CMD_LED_ON);

            } else if (strcmp(str, "off") == 0) {
                overwrite_count++;
                xTaskNotify(xReceiverTaskHandle, CMD_LED_OFF, eSetValueWithOverwrite);
                ESP_LOGI(TAG, "SEND,value=0x%02X,mode=eSetValueWithOverwrite", CMD_LED_OFF);

            } else if (strcmp(str, "status") == 0) {
                no_overwrite_count++;
                BaseType_t result = xTaskNotify(xReceiverTaskHandle, CMD_STATUS,
                                                eSetValueWithoutOverwrite);
                if (result != pdPASS) {
                    no_overwrite_fail_count++;
                    ESP_LOGW(TAG, "SEND,value=0x%02X,mode=eSetValueWithoutOverwrite,result=FAILED(pending)",
                             CMD_STATUS);
                } else {
                    ESP_LOGI(TAG, "SEND,value=0x%02X,mode=eSetValueWithoutOverwrite,result=OK", CMD_STATUS);
                }

            } else if (strcmp(str, "reset") == 0) {
                overwrite_count++;
                xTaskNotify(xReceiverTaskHandle, CMD_RESET, eSetValueWithOverwrite);
                ESP_LOGI(TAG, "SEND,value=0x%02X,mode=eSetValueWithOverwrite", CMD_RESET);

            } else if (strcmp(str, "up") == 0) {
                no_overwrite_count++;
                BaseType_t result = xTaskNotify(xReceiverTaskHandle, CMD_SPEED_UP,
                                                eSetValueWithoutOverwrite);
                ESP_LOGI(TAG, "SEND,value=0x%02X,mode=eSetValueWithoutOverwrite,result=%s",
                         CMD_SPEED_UP, result == pdPASS ? "OK" : "FAILED");
                if (result != pdPASS) no_overwrite_fail_count++;

            } else if (strcmp(str, "down") == 0) {
                no_overwrite_count++;
                BaseType_t result = xTaskNotify(xReceiverTaskHandle, CMD_SPEED_DOWN,
                                                eSetValueWithoutOverwrite);
                ESP_LOGI(TAG, "SEND,value=0x%02X,mode=eSetValueWithoutOverwrite,result=%s",
                         CMD_SPEED_DOWN, result == pdPASS ? "OK" : "FAILED");
                if (result != pdPASS) no_overwrite_fail_count++;

            } else if (strcmp(str, "auto") == 0) {
                ESP_LOGI(TAG, "AUTO,Starting automatic demo sequence...");

                /* Demo: eSetValueWithOverwrite always succeeds */
                uint32_t cmds[] = { CMD_LED_ON, CMD_STATUS, CMD_LED_OFF, CMD_SPEED_UP, CMD_RESET };
                const char *names[] = { "LED_ON", "STATUS", "LED_OFF", "SPEED_UP", "RESET" };

                for (int i = 0; i < 5; i++) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    overwrite_count++;
                    xTaskNotify(xReceiverTaskHandle, cmds[i], eSetValueWithOverwrite);
                    ESP_LOGI(TAG, "AUTO,Sent %s (0x%02lX) with eSetValueWithOverwrite",
                             names[i], (unsigned long)cmds[i]);
                }

                vTaskDelay(pdMS_TO_TICKS(2000));

                /* Demo: eSetValueWithoutOverwrite may fail if pending */
                ESP_LOGW(TAG, "AUTO,Now testing eSetValueWithoutOverwrite rapid fire...");
                int success = 0, fail = 0;
                for (int i = 0; i < 5; i++) {
                    BaseType_t result = xTaskNotify(xReceiverTaskHandle,
                                                    CMD_CUSTOM, eSetValueWithoutOverwrite);
                    if (result == pdPASS) {
                        success++;
                    } else {
                        fail++;
                        no_overwrite_fail_count++;
                    }
                    no_overwrite_count++;
                    /* Small delay between sends */
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                ESP_LOGI(TAG, "AUTO,Rapid fire results: success=%d, fail=%d", success, fail);

            } else {
                ESP_LOGW(TAG, "UNKNOWN,cmd=%s", str);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Task Notification Value Demo ===");
    ESP_LOGI(TAG, "INFO,Demonstrates xTaskNotify with value passing");
    ESP_LOGI(TAG, "INFO,eSetValueWithOverwrite vs eSetValueWithoutOverwrite");

    /* Create receiver task first */
    xTaskCreate(receiver_task, "receiver", 4096, NULL, 10, &xReceiverTaskHandle);

    /* Create sender task */
    xTaskCreate(sender_task, "sender", 4096, NULL, 5, NULL);

    /* Monitor */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "STATUS,cmds=%lu,overwrites=%lu,no_overwrites=%lu,no_overwrite_fails=%lu,tick=%lu",
                 (unsigned long)cmd_received_count,
                 (unsigned long)overwrite_count,
                 (unsigned long)no_overwrite_count,
                 (unsigned long)no_overwrite_fail_count,
                 (unsigned long)xTaskGetTickCount());
    }
}
