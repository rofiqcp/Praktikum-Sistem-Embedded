#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * Konfigurasi Pin dan Parameter - ESP32_05_Task_Delete
 * ESP32 DOIT DevKit V1
 * ============================================================ */

// LED GPIOs
#define LED1_GPIO           2       // Built-in LED (task alive indicator)
#define LED2_GPIO           4       // External LED (self-deleting task)

// Button GPIO
#define BUTTON_GPIO         0       // Tombol BOOT

// UART
#define UART_BAUD           115200

// Task parameters
#define TASK_STACK_SIZE     4096
#define WORKER_PRIORITY     2       // Prioritas worker task
#define SELFDELETE_PRIORITY 3       // Prioritas self-deleting task
#define MANAGER_PRIORITY    4       // Prioritas manager task
#define MONITOR_PRIORITY    5       // Prioritas heap monitor

// Timing (ms)
#define WORKER_BLINK_MS     200     // Worker LED blink period
#define WORKER_LIFETIME_MS  5000    // Worker hidup selama 5 detik
#define SELFDELETE_WORK_MS  3000    // Self-delete task bekerja 3 detik
#define CREATE_DELETE_CYCLE  8000   // Cycle create-delete 8 detik
#define MONITOR_PERIOD_MS   1000    // Heap report setiap 1 detik

// Memory monitoring
#define HEAP_WARN_THRESHOLD 50000   // Warning jika free heap < 50KB
#define MAX_HEAP_SAMPLES    100     // Jumlah sample heap untuk tracking

#endif // CONFIG_H
