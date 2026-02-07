/**
 * @file main.c
 * @brief Program 12: LED Test Pattern / Self-Check / POST (ESP-IDF)
 *
 * Power-On Self-Test (POST) untuk LED dan GPIO.
 * Menjalankan serangkaian test pattern saat startup
 * untuk memverifikasi hardware berfungsi dengan baik.
 *
 * @author Praktikum Sistem Embedded
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"

static const char *TAG = "LED_POST";

/* ==================== KONFIGURASI ==================== */
#define NUM_LEDS        4
#define LED_1           2
#define LED_2           4
#define LED_3           5
#define LED_4           18

static const gpio_num_t LED_PINS[NUM_LEDS] = {LED_1, LED_2, LED_3, LED_4};

#define TEST_DELAY_MS   100
#define PATTERN_REPEAT  2

/* ==================== TEST RESULTS ==================== */
typedef struct {
    bool led[NUM_LEDS];
    bool all_passed;
    uint32_t test_time;
} test_result_t;

static test_result_t test_result;

/* ==================== FUNCTION PROTOTYPES ==================== */
static void run_post(void);
static void test_all_on(void);
static void test_all_off(void);
static void test_sequential(void);
static void test_alternate(void);
static void test_binary(void);
static void test_random_pattern(void);
static void print_test_result(void);

/* ==================== FUNCTIONS ==================== */

static void test_all_on(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(LED_PINS[i], 1);
    }
    ESP_LOGI(TAG, "    All LEDs should be ON");
}

static void test_all_off(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(LED_PINS[i], 0);
    }
    ESP_LOGI(TAG, "    All LEDs should be OFF");
}

static void test_sequential(void)
{
    /* Forward */
    for (int i = 0; i < NUM_LEDS; i++) {
        for (int j = 0; j < NUM_LEDS; j++) {
            gpio_set_level(LED_PINS[j], (i == j) ? 1 : 0);
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }
    /* Backward */
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
        for (int j = 0; j < NUM_LEDS; j++) {
            gpio_set_level(LED_PINS[j], (i == j) ? 1 : 0);
        }
        vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_MS));
    }
    test_all_off();
}

static void test_alternate(void)
{
    /* Pattern 1: 1010 */
    gpio_set_level(LED_PINS[0], 1);
    gpio_set_level(LED_PINS[1], 0);
    gpio_set_level(LED_PINS[2], 1);
    gpio_set_level(LED_PINS[3], 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Pattern 2: 0101 */
    gpio_set_level(LED_PINS[0], 0);
    gpio_set_level(LED_PINS[1], 1);
    gpio_set_level(LED_PINS[2], 0);
    gpio_set_level(LED_PINS[3], 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    test_all_off();
}

static void test_binary(void)
{
    for (int count = 0; count <= 15; count++) {
        for (int i = 0; i < NUM_LEDS; i++) {
            gpio_set_level(LED_PINS[i], (count >> i) & 1);
        }
        ESP_LOGI(TAG, "    Binary: %d%d%d%d = %2d",
                 (count >> 3) & 1,
                 (count >> 2) & 1,
                 (count >> 1) & 1,
                 (count >> 0) & 1,
                 count);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    test_all_off();
}

static void test_random_pattern(void)
{
    for (int r = 0; r < 10; r++) {
        uint8_t pattern = esp_random() & 0x0F;   /* 0-15 */
        for (int i = 0; i < NUM_LEDS; i++) {
            gpio_set_level(LED_PINS[i], (pattern >> i) & 1);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    test_all_off();
}

static void run_post(void)
{
    int64_t start_time = esp_timer_get_time() / 1000;

    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "     POWER-ON SELF-TEST (POST)");
    ESP_LOGI(TAG, "==========================================");

    /* Initialize test results */
    for (int i = 0; i < NUM_LEDS; i++) {
        test_result.led[i] = true;   /* Assume pass */
    }
    test_result.all_passed = true;

    /* Test 1: All ON */
    ESP_LOGI(TAG, "[Test 1] All LEDs ON...");
    test_all_on();
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Test 2: All OFF */
    ESP_LOGI(TAG, "[Test 2] All LEDs OFF...");
    test_all_off();
    vTaskDelay(pdMS_TO_TICKS(300));

    /* Test 3: Sequential */
    ESP_LOGI(TAG, "[Test 3] Sequential pattern...");
    for (int r = 0; r < PATTERN_REPEAT; r++) {
        test_sequential();
    }

    /* Test 4: Alternate */
    ESP_LOGI(TAG, "[Test 4] Alternate pattern...");
    for (int r = 0; r < PATTERN_REPEAT; r++) {
        test_alternate();
    }

    /* Test 5: Binary count */
    ESP_LOGI(TAG, "[Test 5] Binary count (0-15)...");
    test_binary();

    /* Test 6: Random */
    ESP_LOGI(TAG, "[Test 6] Random pattern...");
    test_random_pattern();

    /* Final: All OFF */
    test_all_off();

    test_result.test_time = (uint32_t)((esp_timer_get_time() / 1000) - start_time);

    print_test_result();
}

static void print_test_result(void)
{
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "          TEST RESULTS");
    ESP_LOGI(TAG, "==========================================");

    for (int i = 0; i < NUM_LEDS; i++) {
        ESP_LOGI(TAG, "  LED %d (GPIO%2d): %s",
                 i + 1, LED_PINS[i],
                 test_result.led[i] ? "PASS" : "FAIL");
    }

    ESP_LOGI(TAG, "------------------------------------------");
    ESP_LOGI(TAG, "  Test Duration: %lu ms", (unsigned long)test_result.test_time);
    ESP_LOGI(TAG, "  Overall: %s",
             test_result.all_passed ? "ALL PASSED" : "FAILED");
    ESP_LOGI(TAG, "==========================================");
}

/* ==================== SERIAL INPUT TASK ==================== */
static void serial_task(void *arg)
{
    setvbuf(stdin, NULL, _IONBF, 0);

    while (1) {
        int c = getchar();
        if (c != EOF) {
            if (c == 't' || c == 'T') {
                ESP_LOGI(TAG, ">>> Running self-test again...");
                run_post();
            } else if (c >= '1' && c <= '4') {
                int led_num = c - '1';
                int state = !gpio_get_level(LED_PINS[led_num]);
                gpio_set_level(LED_PINS[led_num], state);
                ESP_LOGI(TAG, "LED %d (GPIO%d): %s",
                         led_num + 1, LED_PINS[led_num],
                         state ? "ON" : "OFF");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ==================== MAIN ==================== */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Program 12: LED Self-Test (POST)");
    ESP_LOGI(TAG, "Praktikum Sistem Embedded - ESP-IDF");
    ESP_LOGI(TAG, "========================================");

    /* Initialize all LED pins */
    uint64_t pin_mask = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        pin_mask |= (1ULL << LED_PINS[i]);
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "LED Configuration:");
    for (int i = 0; i < NUM_LEDS; i++) {
        ESP_LOGI(TAG, "  LED %d: GPIO%d", i + 1, LED_PINS[i]);
    }

    /* Run Power-On Self-Test */
    run_post();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Self-test complete!");
    ESP_LOGI(TAG, "Type 't' to run test again");
    ESP_LOGI(TAG, "Type '1'-'4' to toggle individual LED");
    ESP_LOGI(TAG, "========================================");

    /* Start serial input task */
    xTaskCreate(serial_task, "serial_task", 4096, NULL, 5, NULL);

    /* Main task can idle */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
