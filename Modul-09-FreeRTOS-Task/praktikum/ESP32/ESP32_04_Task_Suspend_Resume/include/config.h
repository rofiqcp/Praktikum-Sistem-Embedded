#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * Konfigurasi Pin dan Parameter - ESP32_04_Task_Suspend_Resume
 * ESP32 DOIT DevKit V1
 * ============================================================ */

// LED GPIOs
#define LED1_GPIO           2       // Built-in LED (blinker task)
#define LED2_GPIO           4       // Status LED (suspend indicator)

// Button GPIO
#define BUTTON_GPIO         0       // Tombol BOOT (active LOW, internal pull-up)

// UART
#define UART_BAUD           115200

// Task parameters
#define TASK_STACK_SIZE     4096
#define BLINKER_PRIORITY    2       // Prioritas task blinker
#define MONITOR_PRIORITY    3       // Prioritas monitor/button task
#define SCHEDULER_PRIORITY  1       // Prioritas scheduler demo
#define ISR_DEMO_PRIORITY   2       // Prioritas ISR demo task

// Timing (ms)
#define BLINK_PERIOD_MS     250     // LED blink setiap 250ms
#define MONITOR_PERIOD_MS   100     // Button scan setiap 100ms
#define DEBOUNCE_MS         300     // Debounce time tombol
#define SCHED_SUSPEND_MS    2000    // Durasi scheduler suspend demo

// ISR config
#define ESP_INTR_FLAG_DEFAULT 0
#define BUTTON_DEBOUNCE_US  300000  // 300ms debounce dalam microseconds

#endif // CONFIG_H
