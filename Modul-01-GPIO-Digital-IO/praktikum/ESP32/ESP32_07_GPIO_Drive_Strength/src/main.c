/**
 * ==========================================================================
 *  ESP32_07 - GPIO Drive Strength Configuration
 * ==========================================================================
 *  Demonstrates configuring GPIO drive strength (output current capability).
 *  The ESP32 GPIO can be set to 4 drive strength levels that control
 *  maximum output current. This affects LED brightness and can be
 *  measured with a multimeter on the output pin.
 *
 *  Hardware:
 *    - 1x LED on GPIO2 with 220Ω series resistor
 *    - 1x Multimeter (measure voltage across LED or current through it)
 *
 *  Wiring:
 *    GPIO2 ──►|── 220Ω ── GND
 *            LED
 *
 *  API Used:
 *    - gpio_config()
 *    - gpio_set_level()
 *    - gpio_set_drive_capability()
 *    - gpio_get_drive_capability()
 *
 *  Expected behaviour:
 *    Cycles through 4 drive strength levels every 3 seconds.
 *    Observe slight brightness / voltage difference on multimeter.
 * ==========================================================================
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"

static const char *TAG = "DRIVE_STR";

/* Human-readable labels for each drive strength level */
static const char *drive_labels[NUM_DRIVE_LEVELS] = {
    "CAP_0 (~5 mA  - weakest)",
    "CAP_1 (~10 mA - weak)",
    "CAP_2 (~20 mA - default)",
    "CAP_3 (~40 mA - strongest)"
};

/* Drive capability enum values in order */
static const gpio_drive_cap_t drive_caps[NUM_DRIVE_LEVELS] = {
    DRIVE_LEVEL_0,
    DRIVE_LEVEL_1,
    DRIVE_LEVEL_2,
    DRIVE_LEVEL_3
};

/**
 * @brief Initialise LED GPIO as output with default drive strength.
 */
static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* Start with LED ON so the effect is visible */
    gpio_set_level(LED_PIN, 1);
    ESP_LOGI(TAG, "LED GPIO%d configured as output, LED ON", LED_PIN);
}

/**
 * @brief Read back and log the current drive capability.
 */
static void log_current_drive(void)
{
    gpio_drive_cap_t cap;
    ESP_ERROR_CHECK(gpio_get_drive_capability(LED_PIN, &cap));
    ESP_LOGI(TAG, "  Readback → drive capability = %d", (int)cap);
}

void app_main(void)
{
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " ESP32_07 - GPIO Drive Strength Demo");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "LED pin  : GPIO%d", LED_PIN);
    ESP_LOGI(TAG, "Hold time: %d ms per level", HOLD_TIME_MS);
    ESP_LOGI(TAG, "Measure voltage/current with multimeter on LED");
    ESP_LOGI(TAG, "");

    gpio_init();

    int cycle = 0;
    while (1) {
        cycle++;
        ESP_LOGI(TAG, "---------- Cycle %d ----------", cycle);

        for (int i = 0; i < NUM_DRIVE_LEVELS; i++) {
            /* Set drive strength */
            ESP_ERROR_CHECK(gpio_set_drive_capability(LED_PIN, drive_caps[i]));
            ESP_LOGI(TAG, "Level %d: %s", i, drive_labels[i]);
            log_current_drive();

            /* Hold so user can observe / measure */
            vTaskDelay(pdMS_TO_TICKS(HOLD_TIME_MS));

            /* Brief OFF pause between transitions */
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(TRANSITION_PAUSE_MS));
            gpio_set_level(LED_PIN, 1);
        }

        ESP_LOGI(TAG, "");
    }
}
