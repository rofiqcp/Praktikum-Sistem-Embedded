#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * Konfigurasi Pin dan Parameter - ESP32_06_Task_Stack_Monitor
 * ESP32 DOIT DevKit V1
 * ============================================================ */

// LED GPIOs
#define LED1_GPIO           2       // Built-in LED (stack warning)
#define LED2_GPIO           4       // External LED (normal operation)

// Button GPIO
#define BUTTON_GPIO         0       // Tombol BOOT

// UART
#define UART_BAUD           115200

// Task parameters - berbeda stack size untuk demonstrasi
#define SMALL_STACK_SIZE    2048    // Stack kecil (shallow task)
#define MEDIUM_STACK_SIZE   4096    // Stack menengah (medium task)
#define LARGE_STACK_SIZE    8192    // Stack besar (deep recursion)

#define SHALLOW_PRIORITY    2       // Prioritas task shallow
#define MEDIUM_PRIORITY     2       // Prioritas task medium
#define DEEP_PRIORITY       2       // Prioritas task deep
#define MONITOR_PRIORITY    4       // Prioritas monitor (tertinggi)

// Stack usage parameters
#define SHALLOW_DEPTH       5       // Kedalaman rekursi shallow
#define MEDIUM_DEPTH        20      // Kedalaman rekursi medium
#define DEEP_DEPTH          40      // Kedalaman rekursi deep

// Monitoring
#define MONITOR_PERIOD_MS   2000    // Report setiap 2 detik
#define STACK_WARN_PERCENT  80      // Warning jika penggunaan > 80%
#define TASK_WORK_PERIOD_MS 500     // Periode kerja setiap task

// Buffer sizes for stack consumption test
#define SMALL_BUFFER_SIZE   32      // Buffer kecil per frame
#define MEDIUM_BUFFER_SIZE  64      // Buffer medium per frame
#define LARGE_BUFFER_SIZE   128     // Buffer besar per frame

#endif // CONFIG_H
