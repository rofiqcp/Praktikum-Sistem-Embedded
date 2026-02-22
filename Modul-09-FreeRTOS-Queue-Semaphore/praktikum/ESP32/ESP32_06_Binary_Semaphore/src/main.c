/*
 * ===========================================================================
 * MODUL 10 - Percobaan 06: Binary Semaphore
 * Platform: ESP32 (ESP-IDF)
 * ===========================================================================
 * Binary semaphore: ISR gives → task takes. Event signaling pattern.
 * Hardware: 1x Push button (GPIO0) + 1x LED (GPIO2)
 * ===========================================================================
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN_PIN     GPIO_NUM_0
#define LED_PIN     GPIO_NUM_2

static const char *TAG = "BIN_SEMA";
static SemaphoreHandle_t xBinarySem;

static void IRAM_ATTR button_isr(void *arg)
{
    BaseType_t xHigherPrioWoken = pdFALSE;
    xSemaphoreGiveFromISR(xBinarySem, &xHigherPrioWoken);
    if (xHigherPrioWoken) portYIELD_FROM_ISR();
}

static void led_task(void *pvParam)
{
    int state = 0;
    uint32_t count = 0;
    
    while (1) {
        /* Block until ISR signals */
        if (xSemaphoreTake(xBinarySem, portMAX_DELAY) == pdTRUE) {
            count++;
            state = !state;
            gpio_set_level(LED_PIN, state);
            ESP_LOGI(TAG, "[TASK] Button event #%lu — LED %s",
                     (unsigned long)count, state ? "ON" : "OFF");
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_NEGEDGE);
    
    ESP_LOGI(TAG, "=== MODUL 10: Binary Semaphore (ISR → Task) ===");
    
    xBinarySem = xSemaphoreCreateBinary();
    
    xTaskCreate(led_task, "LEDTask", 2048, NULL, 3, NULL);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, button_isr, NULL);
    
    ESP_LOGI(TAG, "Press BOOT button to signal semaphore");
}
