/*
 * ESP32_09_Notification_ISR
 * ==========================
 * vTaskNotifyGiveFromISR() — ISR to task signaling.
 * Compare latency with binary semaphore ISR approach.
 * Print tick counts for timing comparison.
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

static const char *TAG = "NOTIFY_ISR";

#define LED_GPIO        GPIO_NUM_2
#define BTN_GPIO        GPIO_NUM_0
#define TEST_ITERATIONS 20

static TaskHandle_t xNotifyTaskHandle = NULL;
static TaskHandle_t xSemaTaskHandle = NULL;
static SemaphoreHandle_t xBinarySemaphore = NULL;

static volatile int64_t isr_time_us = 0;
static volatile bool test_mode_notify = true;
static volatile uint32_t press_count = 0;

/* Latency tracking */
typedef struct {
    int64_t latencies[TEST_ITERATIONS];
    uint32_t count;
    int64_t min;
    int64_t max;
    int64_t sum;
} LatencyStats;

static LatencyStats notify_stats = { .min = INT64_MAX, .max = 0, .sum = 0, .count = 0 };
static LatencyStats sema_stats = { .min = INT64_MAX, .max = 0, .sum = 0, .count = 0 };

static bool led_state = false;

static void update_stats(LatencyStats *stats, int64_t latency)
{
    if (stats->count < TEST_ITERATIONS) {
        stats->latencies[stats->count] = latency;
    }
    stats->count++;
    stats->sum += latency;
    if (latency < stats->min) stats->min = latency;
    if (latency > stats->max) stats->max = latency;
}

static void print_comparison(void)
{
    ESP_LOGW(TAG, "========== LATENCY COMPARISON ==========");
    ESP_LOGW(TAG, "COMPARE,Method,Count,Min_us,Max_us,Avg_us");

    if (notify_stats.count > 0) {
        ESP_LOGI(TAG, "COMPARE,NOTIFICATION,%lu,%lld,%lld,%lld",
                 (unsigned long)notify_stats.count,
                 (long long)notify_stats.min,
                 (long long)notify_stats.max,
                 (long long)(notify_stats.sum / notify_stats.count));
    }
    if (sema_stats.count > 0) {
        ESP_LOGI(TAG, "COMPARE,SEMAPHORE,%lu,%lld,%lld,%lld",
                 (unsigned long)sema_stats.count,
                 (long long)sema_stats.min,
                 (long long)sema_stats.max,
                 (long long)(sema_stats.sum / sema_stats.count));
    }

    if (notify_stats.count > 0 && sema_stats.count > 0) {
        int64_t notify_avg = notify_stats.sum / notify_stats.count;
        int64_t sema_avg = sema_stats.sum / sema_stats.count;
        int64_t diff = sema_avg - notify_avg;
        ESP_LOGW(TAG, "COMPARE,DIFFERENCE,%lld_us,notification is %s by %lld us",
                 (long long)diff,
                 diff > 0 ? "FASTER" : "SLOWER",
                 (long long)(diff > 0 ? diff : -diff));
    }
    ESP_LOGW(TAG, "=========================================");
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    isr_time_us = esp_timer_get_time();

    if (test_mode_notify) {
        vTaskNotifyGiveFromISR(xNotifyTaskHandle, &xHigherPriorityTaskWoken);
    } else {
        xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void notification_handler_task(void *pvParameters)
{
    while (1) {
        uint32_t value = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int64_t now = esp_timer_get_time();
        int64_t latency = now - isr_time_us;

        /* Debounce */
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(BTN_GPIO) == 0) {
            press_count++;
            update_stats(&notify_stats, latency);

            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state ? 1 : 0);

            ESP_LOGI(TAG, "EVENT,NOTIFY_ISR,press=%lu,latency=%lld_us,min=%lld,max=%lld,avg=%lld,tick=%lu",
                     (unsigned long)press_count,
                     (long long)latency,
                     (long long)notify_stats.min,
                     (long long)notify_stats.max,
                     (long long)(notify_stats.sum / notify_stats.count),
                     (unsigned long)xTaskGetTickCount());

            if (notify_stats.count >= TEST_ITERATIONS / 2) {
                test_mode_notify = false;
                ESP_LOGW(TAG, "SWITCH,Switching to SEMAPHORE mode for comparison");
                ESP_LOGW(TAG, "SWITCH,Notification stats: min=%lld, max=%lld, avg=%lld us",
                         (long long)notify_stats.min,
                         (long long)notify_stats.max,
                         (long long)(notify_stats.sum / notify_stats.count));
            }

            while (gpio_get_level(BTN_GPIO) == 0) vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static void semaphore_handler_task(void *pvParameters)
{
    while (1) {
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
            int64_t now = esp_timer_get_time();
            int64_t latency = now - isr_time_us;

            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BTN_GPIO) == 0) {
                press_count++;
                update_stats(&sema_stats, latency);

                led_state = !led_state;
                gpio_set_level(LED_GPIO, led_state ? 1 : 0);

                ESP_LOGI(TAG, "EVENT,SEMA_ISR,press=%lu,latency=%lld_us,min=%lld,max=%lld,avg=%lld,tick=%lu",
                         (unsigned long)press_count,
                         (long long)latency,
                         (long long)sema_stats.min,
                         (long long)sema_stats.max,
                         (long long)(sema_stats.sum / sema_stats.count),
                         (unsigned long)xTaskGetTickCount());

                if (sema_stats.count >= TEST_ITERATIONS / 2) {
                    print_comparison();
                    /* Reset for next round */
                    test_mode_notify = true;
                    notify_stats = (LatencyStats){ .min = INT64_MAX, .max = 0, .sum = 0, .count = 0 };
                    sema_stats = (LatencyStats){ .min = INT64_MAX, .max = 0, .sum = 0, .count = 0 };
                    press_count = 0;
                    ESP_LOGW(TAG, "SWITCH,Back to NOTIFICATION mode. Press button to continue.");
                }

                while (gpio_get_level(BTN_GPIO) == 0) vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Notification ISR Latency Demo ===");
    ESP_LOGI(TAG, "INFO,Comparing ISR-to-task latency:");
    ESP_LOGI(TAG, "INFO,  vTaskNotifyGiveFromISR() vs xSemaphoreGiveFromISR()");
    ESP_LOGI(TAG, "INFO,Button GPIO: %d, LED GPIO: %d", BTN_GPIO, LED_GPIO);
    ESP_LOGI(TAG, "INFO,Press BOOT button %d times for Notification, then %d for Semaphore",
             TEST_ITERATIONS / 2, TEST_ITERATIONS / 2);

    /* Configure GPIO */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_GPIO, 0);

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&btn_conf);

    xBinarySemaphore = xSemaphoreCreateBinary();

    xTaskCreate(notification_handler_task, "notify_handler", 4096, NULL, 12, &xNotifyTaskHandle);
    xTaskCreate(semaphore_handler_task, "sema_handler", 4096, NULL, 12, &xSemaTaskHandle);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, button_isr_handler, NULL);

    ESP_LOGI(TAG, "INFO,System ready! Mode: NOTIFICATION. Press BOOT button.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "STATUS,mode=%s,presses=%lu,notify_samples=%lu,sema_samples=%lu,tick=%lu",
                 test_mode_notify ? "NOTIFICATION" : "SEMAPHORE",
                 (unsigned long)press_count,
                 (unsigned long)notify_stats.count,
                 (unsigned long)sema_stats.count,
                 (unsigned long)xTaskGetTickCount());
    }
}
