/*
 * ESP32_03_Stack_Overflow_Detect
 * Demonstrate configCHECK_FOR_STACK_OVERFLOW.
 * Create tasks with various stack sizes, monitor high-water marks,
 * and intentionally overflow a small-stack task.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "STK_OVF";

/* ------------------------------------------------------------------ */
/*  Stack overflow hook — called by FreeRTOS on overflow              */
/* ------------------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* This runs in ISR-like context; keep it minimal */
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!! STACK OVERFLOW in task: \"%s\" !!!\n", pcTaskName);
    printf("!!! Task handle: %p                       !!!\n", (void *)xTask);
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

    /* On ESP32, best to abort to get a backtrace */
    abort();
}

/* ------------------------------------------------------------------ */
/*  Safe task — generous stack                                        */
/* ------------------------------------------------------------------ */
static void safe_task(void *pv)
{
    char buf[64];
    uint32_t count = 0;

    while (1) {
        count++;
        snprintf(buf, sizeof(buf), "safe_task iter %lu", (unsigned long)count);

        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        printf("[SAFE]  %s | Stack HWM: %u words (%u bytes)\n",
               buf, (unsigned)hwm, (unsigned)(hwm * sizeof(StackType_t)));

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ------------------------------------------------------------------ */
/*  Monitor task — reports stack usage for all tasks                   */
/* ------------------------------------------------------------------ */
static void monitor_task(void *pv)
{
    while (1) {
        printf("\n--- Stack High-Water Marks ---\n");

        /* Use vTaskList for overview */
        char *buf = pvPortMalloc(1024);
        if (buf) {
            vTaskList(buf);
            printf("Name            State  Prio  HWM   Num\n");
            printf("--------------------------------------\n");
            printf("%s\n", buf);
            vPortFree(buf);
        }
        printf("------------------------------\n");

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ------------------------------------------------------------------ */
/*  Risky task — very small stack, will eventually overflow            */
/* ------------------------------------------------------------------ */
static volatile int g_trigger_overflow = 0;

static void risky_task(void *pv)
{
    uint32_t count = 0;

    while (1) {
        count++;
        UBaseType_t hwm = uxTaskGetStackHighWaterMark(NULL);
        printf("[RISKY] iter=%lu  HWM=%u words\n",
               (unsigned long)count, (unsigned)hwm);

        if (g_trigger_overflow) {
            printf("[RISKY] Intentionally consuming stack…\n");
            /* Recursive call to blow the stack */
            volatile char big_local[256];
            memset((void *)big_local, 0xAB, sizeof(big_local));
            printf("[RISKY] big_local @ %p (should overflow)\n", (void *)big_local);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ------------------------------------------------------------------ */
/*  Trigger task — sets the overflow flag after a delay                */
/* ------------------------------------------------------------------ */
static void trigger_task(void *pv)
{
    ESP_LOGW(TAG, "Will trigger stack overflow in 15 seconds…");
    vTaskDelay(pdMS_TO_TICKS(15000));

    ESP_LOGE(TAG, ">>> TRIGGERING STACK OVERFLOW NOW <<<");
    g_trigger_overflow = 1;

    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Stack Overflow Detection Demo ===");
    ESP_LOGI(TAG, "configCHECK_FOR_STACK_OVERFLOW = %d", configCHECK_FOR_STACK_OVERFLOW);

    /* Safe task — 4096 byte stack */
    xTaskCreate(safe_task, "safe_task", 4096, NULL, 3, NULL);

    /* Monitor task */
    xTaskCreate(monitor_task, "monitor", 4096, NULL, 2, NULL);

    /* Risky task — only 512 byte stack (very small!) */
    xTaskCreate(risky_task, "risky_task", 512, NULL, 3, NULL);

    /* Trigger task — will enable overflow after delay */
    xTaskCreate(trigger_task, "trigger", 2048, NULL, 1, NULL);
}
