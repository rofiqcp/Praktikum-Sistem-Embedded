/*
 * ESP32_06_Notification_Basic
 * ============================
 * xTaskNotifyGive()/ulTaskNotifyTake() as lightweight binary semaphore.
 * Button ISR notifies task, task toggles LED.
 * Compares with binary semaphore approach.
 *
 * Hardware: 1x Button on GPIO0 (BOOT), 1x LED on GPIO2
 * Framework: ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "NOTIFY_BASIC";

#define LED_GPIO    GPIO_NUM_2
#define BTN_GPIO    GPIO_NUM_0

static TaskHandle_t xNotifyTaskHandle = NULL;
static TaskHandle_t xSemaphoreTaskHandle = NULL;
static SemaphoreHandle_t xBinarySemaphore = NULL;

static volatile uint32_t notify_count = 0;
static volatile uint32_t semaphore_count = 0;
static volatile int64_t notify_latency_sum = 0;
static volatile int64_t semaphore_latency_sum = 0;
static volatile int64_t isr_timestamp = 0;
static volatile bool use_notification = true;  /* Toggle between methods */
static bool led_state = false;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    isr_timestamp = esp_timer_get_time();

    if (use_notification) {
        /* Method 1: Task Notification (lightweight) */
        vTaskNotifyGiveFromISR(xNotifyTaskHandle, &xHigherPriorityTaskWoken);
    } else {
        /* Method 2: Binary Semaphore (traditional) */
        xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void notification_task(void *pvParameters)
{
    ESP_LOGI(TAG, "INFO,Notification task started");

    while (1) {
        /* Wait for notification (acts like binary semaphore take) */
        uint32_t value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int64_t now = esp_timer_get_time();
        int64_t latency = now - isr_timestamp;

        if (value > 0) {
            /* Simple debounce */
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BTN_GPIO) == 0) {
                notify_count++;
                notify_latency_sum += latency;
                led_state = !led_state;
                gpio_set_level(LED_GPIO, led_state ? 1 : 0);

                ESP_LOGI(TAG, "EVENT,NOTIFICATION,count=%lu,led=%s,latency=%lld_us,avg_latency=%lld_us,tick=%lu",
                         (unsigned long)notify_count,
                         led_state ? "ON" : "OFF",
                         (long long)latency,
                         (long long)(notify_latency_sum / notify_count),
                         (unsigned long)xTaskGetTickCount());

                /* After 5 presses, switch to semaphore mode */
                if (notify_count == 5) {
                    use_notification = false;
                    ESP_LOGW(TAG, "SWITCH,Switching to Binary Semaphore mode after 5 notification presses");
                    ESP_LOGW(TAG, "SWITCH,Notification avg latency: %lld us",
                             (long long)(notify_latency_sum / notify_count));
                }

                while (gpio_get_level(BTN_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
    }
}

static void semaphore_task(void *pvParameters)
{
    ESP_LOGI(TAG, "INFO,Semaphore task started");

    while (1) {
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
            int64_t now = esp_timer_get_time();
            int64_t latency = now - isr_timestamp;

            /* Simple debounce */
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BTN_GPIO) == 0) {
                semaphore_count++;
                semaphore_latency_sum += latency;
                led_state = !led_state;
                gpio_set_level(LED_GPIO, led_state ? 1 : 0);

                ESP_LOGI(TAG, "EVENT,SEMAPHORE,count=%lu,led=%s,latency=%lld_us,avg_latency=%lld_us,tick=%lu",
                         (unsigned long)semaphore_count,
                         led_state ? "ON" : "OFF",
                         (long long)latency,
                         (long long)(semaphore_latency_sum / semaphore_count),
                         (unsigned long)xTaskGetTickCount());

                /* After 5 semaphore presses, switch back */
                if (semaphore_count == 5) {
                    use_notification = true;
                    ESP_LOGW(TAG, "SWITCH,Switching back to Notification mode");
                    ESP_LOGW(TAG, "SWITCH,Semaphore avg latency: %lld us",
                             (long long)(semaphore_latency_sum / semaphore_count));
                    ESP_LOGW(TAG, "COMPARE,Notify avg=%lld us, Semaphore avg=%lld us",
                             notify_count > 0 ? (long long)(notify_latency_sum / notify_count) : 0,
                             (long long)(semaphore_latency_sum / semaphore_count));

                    /* Reset counters for next round */
                    notify_count = 0;
                    semaphore_count = 0;
                    notify_latency_sum = 0;
                    semaphore_latency_sum = 0;
                }

                while (gpio_get_level(BTN_GPIO) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Task Notification Basic Demo ===");
    ESP_LOGI(TAG, "INFO,Comparing: Task Notification vs Binary Semaphore");
    ESP_LOGI(TAG, "INFO,Button GPIO: %d, LED GPIO: %d", BTN_GPIO, LED_GPIO);
    ESP_LOGI(TAG, "INFO,Press BOOT button to toggle LED");
    ESP_LOGI(TAG, "INFO,First 5 presses: Notification mode, Next 5: Semaphore mode");

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

    /* Configure button */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    /* Create binary semaphore for comparison */
    xBinarySemaphore = xSemaphoreCreateBinary();

    /* Create tasks */
    xTaskCreate(notification_task, "notify_task", 4096, NULL, 10, &xNotifyTaskHandle);
    xTaskCreate(semaphore_task, "sema_task", 4096, NULL, 10, &xSemaphoreTaskHandle);

    /* Install ISR */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, button_isr_handler, NULL);

    ESP_LOGI(TAG, "INFO,System ready! Press BOOT button (Notification mode active)");

    /* Monitor */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "STATUS,mode=%s,notify_count=%lu,sema_count=%lu,led=%s,tick=%lu",
                 use_notification ? "NOTIFICATION" : "SEMAPHORE",
                 (unsigned long)notify_count,
                 (unsigned long)semaphore_count,
                 led_state ? "ON" : "OFF",
                 (unsigned long)xTaskGetTickCount());
    }
}
