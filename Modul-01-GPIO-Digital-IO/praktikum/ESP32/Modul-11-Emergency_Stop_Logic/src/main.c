/**
 * @file main.c
 * @brief Program 11: Emergency Stop Logic / Fail-Safe Pattern (ESP-IDF)
 *
 * Implementasi sistem emergency stop dengan:
 * - Button E-STOP yang langsung mematikan output via ISR
 * - LED indikator status (green=running, red=stopped)
 * - Sistem harus di-reset manual setelah E-STOP
 * - Serial commands: 's'=start, 'x'=stop, 'r'=reset
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "ESTOP";

/* ==================== KONFIGURASI ==================== */
#define ESTOP_PIN       0       /* Emergency stop button (BOOT) */
#define RESET_PIN       4       /* Reset button */
#define OUTPUT_PIN      5       /* Controlled output (motor simulation) */
#define LED_RUN         2       /* Green LED - Running */
#define LED_STOP        18      /* Red LED - Stopped */

/* ==================== STATE MACHINE ==================== */
typedef enum {
    STATE_STOPPED,
    STATE_RUNNING,
    STATE_ESTOP
} system_state_t;

/* ==================== VARIABEL ==================== */
static volatile system_state_t system_state = STATE_STOPPED;
static volatile bool estop_triggered = false;
static int64_t run_start_time = 0;

/* ==================== FORWARD DECLARATIONS ==================== */
static void start_system(void);
static void stop_system(void);
static void reset_estop(void);

/* ==================== ISR ==================== */
static void IRAM_ATTR estop_isr_handler(void *arg)
{
    /* Immediate action - don't wait for main loop! */
    gpio_set_level(OUTPUT_PIN, 0);   /* Kill output immediately */
    gpio_set_level(LED_RUN, 0);
    gpio_set_level(LED_STOP, 1);
    estop_triggered = true;
    system_state = STATE_ESTOP;
}

/* ==================== SERIAL INPUT TASK ==================== */
static void serial_task(void *arg)
{
    setvbuf(stdin, NULL, _IONBF, 0);

    while (1) {
        int c = getchar();
        if (c != EOF) {
            switch (c) {
                case 's':
                case 'S':
                    if (system_state == STATE_STOPPED) {
                        start_system();
                    } else if (system_state == STATE_ESTOP) {
                        ESP_LOGW(TAG, "!!! Cannot start - E-STOP active! Reset first.");
                    } else {
                        ESP_LOGI(TAG, "System already running");
                    }
                    break;

                case 'x':
                case 'X':
                    if (system_state == STATE_RUNNING) {
                        stop_system();
                    }
                    break;

                case 'r':
                case 'R':
                    if (system_state == STATE_ESTOP) {
                        reset_estop();
                    } else {
                        ESP_LOGI(TAG, "No E-STOP to reset");
                    }
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ==================== FUNCTIONS ==================== */

static void start_system(void)
{
    system_state = STATE_RUNNING;
    run_start_time = esp_timer_get_time() / 1000;

    gpio_set_level(OUTPUT_PIN, 1);
    gpio_set_level(LED_RUN, 1);
    gpio_set_level(LED_STOP, 0);

    ESP_LOGI(TAG, ">>> SYSTEM STARTED");
    ESP_LOGI(TAG, "Output enabled, monitoring for E-STOP...");
}

static void stop_system(void)
{
    system_state = STATE_STOPPED;

    gpio_set_level(OUTPUT_PIN, 0);
    gpio_set_level(LED_RUN, 0);
    gpio_set_level(LED_STOP, 1);

    int64_t run_duration = (esp_timer_get_time() / 1000) - run_start_time;
    ESP_LOGI(TAG, ">>> SYSTEM STOPPED (normal)");
    ESP_LOGI(TAG, "Run duration: %lld ms", run_duration);
}

static void reset_estop(void)
{
    system_state = STATE_STOPPED;
    estop_triggered = false;

    gpio_set_level(LED_STOP, 1);   /* Solid red (stopped, not e-stop) */

    ESP_LOGI(TAG, ">>> E-STOP RESET");
    ESP_LOGI(TAG, "System in STOPPED state");
    ESP_LOGI(TAG, "Press 's' to start again");
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 11: Emergency Stop System");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Konfigurasi output pins */
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_PIN) | (1ULL << LED_RUN) | (1ULL << LED_STOP),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

    /* Konfigurasi E-STOP button (input + pull-up + falling edge ISR) */
    gpio_config_t estop_conf = {
        .pin_bit_mask = (1ULL << ESTOP_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&estop_conf);

    /* Konfigurasi Reset button (input + pull-up, polled) */
    gpio_config_t reset_conf = {
        .pin_bit_mask = (1ULL << RESET_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&reset_conf);

    /* Initial state: STOPPED (safe) */
    gpio_set_level(OUTPUT_PIN, 0);
    gpio_set_level(LED_RUN, 0);
    gpio_set_level(LED_STOP, 1);

    /* Install ISR service and attach E-STOP handler */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ESTOP_PIN, estop_isr_handler, NULL);

    ESP_LOGI(TAG, "Emergency Stop System Initialized");
    ESP_LOGI(TAG, "  E-STOP Button : GPIO%d (press to stop)", ESTOP_PIN);
    ESP_LOGI(TAG, "  Reset Button  : GPIO%d (press to reset)", RESET_PIN);
    ESP_LOGI(TAG, "  Output Control: GPIO%d", OUTPUT_PIN);
    ESP_LOGI(TAG, "  LED Running   : GPIO%d (Green)", LED_RUN);
    ESP_LOGI(TAG, "  LED Stopped   : GPIO%d (Red)", LED_STOP);
    ESP_LOGI(TAG, "Commands: 's'=Start, 'x'=Stop, 'r'=Reset after E-STOP");
    ESP_LOGI(TAG, "Current State: STOPPED");

    /* Start serial input task */
    xTaskCreate(serial_task, "serial_task", 4096, NULL, 5, NULL);

    int64_t last_reset_check = 0;
    int64_t last_blink = 0;

    while (1) {
        int64_t now = esp_timer_get_time() / 1000;

        /* Handle physical reset button */
        if (gpio_get_level(RESET_PIN) == 0 && now - last_reset_check > 500) {
            last_reset_check = now;
            if (system_state == STATE_ESTOP) {
                reset_estop();
            }
        }

        /* Handle E-STOP event */
        if (estop_triggered) {
            estop_triggered = false;
            int64_t run_duration = now - run_start_time;

            ESP_LOGE(TAG, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            ESP_LOGE(TAG, "!!! EMERGENCY STOP ACTIVATED !!!");
            ESP_LOGE(TAG, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            ESP_LOGE(TAG, "System was running for %lld ms", run_duration);
            ESP_LOGE(TAG, "Output KILLED immediately");
            ESP_LOGE(TAG, "Press 'r' or Reset button to clear");
        }

        /* Running indicator */
        if (system_state == STATE_RUNNING) {
            static int64_t last_status = 0;
            if (now - last_status > 500) {
                last_status = now;
                ESP_LOGI(TAG, "[RUNNING] Uptime: %lld ms", now - run_start_time);
            }
        }

        /* E-STOP state indicator (fast blink red LED) */
        if (system_state == STATE_ESTOP) {
            if (now - last_blink > 200) {
                last_blink = now;
                static bool blink_state = false;
                blink_state = !blink_state;
                gpio_set_level(LED_STOP, blink_state);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
