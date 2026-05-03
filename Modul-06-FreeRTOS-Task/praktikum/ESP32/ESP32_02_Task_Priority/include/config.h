#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * Konfigurasi Pin dan Parameter - ESP32_02_Task_Priority
 * ESP32 DOIT DevKit V1
 * ============================================================ */

// LED GPIOs
#define LED1_GPIO           2       // Built-in LED
#define LED2_GPIO           4       // External LED
#define LED3_GPIO           5       // LED ketiga untuk task prioritas tinggi

// Button GPIO
#define BUTTON_GPIO         0       // Tombol BOOT

// UART
#define UART_BAUD           115200

// Task parameters
#define TASK_STACK_SIZE     4096
#define LOW_PRIORITY        1       // Prioritas rendah
#define MED_PRIORITY        3       // Prioritas menengah
#define HIGH_PRIORITY       5       // Prioritas tinggi
#define MONITOR_PRIORITY    6       // Monitor harus paling tinggi

// CPU intensive work parameters
#define CPU_WORK_ITERATIONS 500000  // Jumlah iterasi kerja CPU
#define WORK_ROUNDS         10      // Jumlah ronde kerja per task

// Timing (ms)
#define ROUND_DELAY_MS      100     // Delay antar ronde
#define MONITOR_PERIOD_MS   1000    // Monitor report setiap 1 detik

#endif // CONFIG_H
