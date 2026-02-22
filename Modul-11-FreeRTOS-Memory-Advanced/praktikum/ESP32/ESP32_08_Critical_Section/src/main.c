/*
 * ESP32_08_Critical_Section
 * taskENTER_CRITICAL / taskEXIT_CRITICAL demo for ESP32.
 * ESP32 uses portMUX_TYPE spinlock for critical sections.
 * Compare critical section vs mutex. LEDs on GPIO2 and GPIO4.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

static const char *TAG = "CRIT_SEC";

#define LED1_PIN  GPIO_NUM_2
#define LED2_PIN  GPIO_NUM_4

/* ------------------------------------------------------------------ */
/*  Shared resource protected by critical section / mutex             */
/* ------------------------------------------------------------------ */
static volatile uint32_t shared_counter = 0;
static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t xMutex;

/* Timing accumulators */
static volatile int64_t critical_total_us = 0;
static volatile uint32_t critical_count = 0;
static volatile int64_t mutex_total_us = 0;
static volatile uint32_t mutex_count = 0;

/* ------------------------------------------------------------------ */
/*  Task using critical section (spinlock)                            */
/* ------------------------------------------------------------------ */
static void critical_section_task(void *pv)
{
    int task_id = (int)(intptr_t)pv;
    while (1) {
        int64_t t0 = esp_timer_get_time();

        portENTER_CRITICAL(&my_spinlock);
        /* --- critical region: interrupts disabled on this core --- */
        shared_counter++;
        uint32_t val = shared_counter;
        /* Simulate some work */
        for (volatile int i = 0; i < 100; i++) {}
        portEXIT_CRITICAL(&my_spinlock);

        int64_t t1 = esp_timer_get_time();
        int64_t elapsed = t1 - t0;

        /* Accumulate timing (not inside critical section) */
        portENTER_CRITICAL(&my_spinlock);
        critical_total_us += elapsed;
        critical_count++;
        portEXIT_CRITICAL(&my_spinlock);

        if (val % 50 == 0) {
            printf("[CRIT T%d] counter=%lu  crit_time=%lld us\n",
                   task_id, (unsigned long)val, (long long)elapsed);
            gpio_set_level(LED1_PIN, val % 2);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ------------------------------------------------------------------ */
/*  Task using mutex                                                  */
/* ------------------------------------------------------------------ */
static volatile uint32_t mutex_counter = 0;

static void mutex_task(void *pv)
{
    int task_id = (int)(intptr_t)pv;
    while (1) {
        int64_t t0 = esp_timer_get_time();

        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            mutex_counter++;
            uint32_t val = mutex_counter;
            /* Simulate some work */
            for (volatile int i = 0; i < 100; i++) {}
            xSemaphoreGive(xMutex);

            int64_t t1 = esp_timer_get_time();
            int64_t elapsed = t1 - t0;

            portENTER_CRITICAL(&my_spinlock);
            mutex_total_us += elapsed;
            mutex_count++;
            portEXIT_CRITICAL(&my_spinlock);

            if (val % 50 == 0) {
                printf("[MUTEX T%d] counter=%lu  mutex_time=%lld us\n",
                       task_id, (unsigned long)val, (long long)elapsed);
                gpio_set_level(LED2_PIN, val % 2);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ------------------------------------------------------------------ */
/*  Stats reporting task                                              */
/* ------------------------------------------------------------------ */
static void stats_task(void *pv)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        portENTER_CRITICAL(&my_spinlock);
        uint32_t cc = critical_count;
        int64_t  ct = critical_total_us;
        uint32_t mc = mutex_count;
        int64_t  mt = mutex_total_us;
        uint32_t sc = shared_counter;
        uint32_t mx = mutex_counter;
        portEXIT_CRITICAL(&my_spinlock);

        printf("\n===== CRITICAL vs MUTEX COMPARISON =====\n");
        printf("[CRITICAL] counter=%lu  ops=%lu  avg=%lld us/op\n",
               (unsigned long)sc, (unsigned long)cc,
               cc ? (long long)(ct / cc) : 0);
        printf("[MUTEX]    counter=%lu  ops=%lu  avg=%lld us/op\n",
               (unsigned long)mx, (unsigned long)mc,
               mc ? (long long)(mt / mc) : 0);
        printf("========================================\n\n");
    }
}

/* ------------------------------------------------------------------ */
/*  Interrupt disable/enable demo                                     */
/* ------------------------------------------------------------------ */
static void interrupt_demo_task(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(3000));

    printf("\n--- portDISABLE_INTERRUPTS demo ---\n");
    int64_t t0 = esp_timer_get_time();
    portENTER_CRITICAL(&my_spinlock);
    /* All maskable interrupts disabled */
    volatile uint32_t sum = 0;
    for (volatile int i = 0; i < 1000; i++) sum += i;
    portEXIT_CRITICAL(&my_spinlock);
    int64_t t1 = esp_timer_get_time();

    printf("  1000 iterations with interrupts disabled: %lld us  sum=%lu\n",
           (long long)(t1 - t0), (unsigned long)sum);
    printf("  WARNING: Keep critical sections SHORT!\n");
    printf("-----------------------------------\n");

    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Critical Section Demo (ESP32 spinlock) ===");

    /* LEDs */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);

    /* Mutex */
    xMutex = xSemaphoreCreateMutex();

    /* Two critical-section tasks */
    xTaskCreate(critical_section_task, "crit_A", 2048, (void *)1, 5, NULL);
    xTaskCreate(critical_section_task, "crit_B", 2048, (void *)2, 5, NULL);

    /* Two mutex tasks */
    xTaskCreate(mutex_task, "mutex_A", 2048, (void *)1, 5, NULL);
    xTaskCreate(mutex_task, "mutex_B", 2048, (void *)2, 5, NULL);

    /* Stats */
    xTaskCreate(stats_task, "stats", 3072, NULL, 3, NULL);

    /* One-shot interrupt demo */
    xTaskCreate(interrupt_demo_task, "int_demo", 2048, NULL, 4, NULL);
}
