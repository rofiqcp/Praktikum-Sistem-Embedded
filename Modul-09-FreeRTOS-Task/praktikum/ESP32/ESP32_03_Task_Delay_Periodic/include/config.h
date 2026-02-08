#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * Konfigurasi Pin dan Parameter - ESP32_03_Task_Delay_Periodic
 * ESP32 DOIT DevKit V1
 * ============================================================ */

// LED GPIOs
#define LED1_GPIO           2       // LED untuk vTaskDelay
#define LED2_GPIO           4       // LED untuk vTaskDelayUntil

// Button GPIO
#define BUTTON_GPIO         0       // Tombol BOOT

// UART
#define UART_BAUD           115200

// Task parameters
#define TASK_STACK_SIZE     4096
#define DELAY_TASK_PRIORITY     3
#define PERIODIC_TASK_PRIORITY  3
#define MONITOR_PRIORITY        4
#define STATS_PRIORITY          5

// Timing (ms)
#define TARGET_PERIOD_MS    100     // Target periode 100ms
#define WORK_TIME_MS        15      // Simulasi kerja 15ms setiap cycle
#define MONITOR_PERIOD_MS   5000    // Report setiap 5 detik
#define STATS_PERIOD_MS     3000    // Statistik setiap 3 detik

// Statistik
#define MAX_SAMPLES         200     // Jumlah sample untuk jitter analysis
#define JITTER_WARN_US      5000    // Warning jika jitter > 5ms

#endif // CONFIG_H
