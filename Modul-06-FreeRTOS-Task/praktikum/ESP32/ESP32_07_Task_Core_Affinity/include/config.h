/**
 * ============================================================================
 * ESP32_07_Task_Core_Affinity - Konfigurasi Pin dan Parameter
 * ============================================================================
 * Demonstrasi penggunaan dual-core ESP32 dengan xTaskCreatePinnedToCore()
 * Core 0 = protocol (WiFi/BT), Core 1 = application
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ========================== KONFIGURASI LED ============================== */
#define LED1_PIN                2       /* LED1 pada GPIO2 (built-in) */
#define LED2_PIN                4       /* LED2 pada GPIO4 (eksternal) */

/* ======================== KONFIGURASI TASK =============================== */
/* Prioritas task - semakin tinggi nilainya semakin prioritas */
#define TASK_CORE0_PRIORITY     3       /* Prioritas task yang di-pin ke Core 0 */
#define TASK_CORE1_PRIORITY     3       /* Prioritas task yang di-pin ke Core 1 */
#define TASK_FLOAT_PRIORITY     3       /* Prioritas task floating (tanpa afinitas) */
#define MONITOR_TASK_PRIORITY   5       /* Prioritas task monitor (tertinggi) */

/* Ukuran stack untuk masing-masing task (dalam words) */
#define TASK_CORE0_STACK        4096    /* Stack size task Core 0 */
#define TASK_CORE1_STACK        4096    /* Stack size task Core 1 */
#define TASK_FLOAT_STACK        4096    /* Stack size task floating */
#define MONITOR_TASK_STACK      4096    /* Stack size task monitor */

/* =================== KONFIGURASI WORKLOAD ================================ */
#define WORK_ITERATIONS_LIGHT   50000   /* Iterasi untuk beban ringan */
#define WORK_ITERATIONS_MEDIUM  200000  /* Iterasi untuk beban sedang */
#define WORK_ITERATIONS_HEAVY   500000  /* Iterasi untuk beban berat */

/* =================== KONFIGURASI TIMING ================================== */
#define TASK_DELAY_MS           1000    /* Delay antar eksekusi task (ms) */
#define MONITOR_INTERVAL_MS     3000    /* Interval laporan monitor (ms) */
#define LED_BLINK_FAST_MS       100     /* Kedip LED cepat (ms) */
#define LED_BLINK_SLOW_MS       500     /* Kedip LED lambat (ms) */

/* =================== KONFIGURASI PENGUKURAN ============================== */
#define MAX_SAMPLES             50      /* Jumlah sampel waktu eksekusi */
#define WARMUP_CYCLES           5       /* Siklus pemanasan sebelum pengukuran */

/* =================== KONFIGURASI SERIAL ================================== */
#define SERIAL_BAUD_RATE        115200  /* Baud rate komunikasi serial */

#endif /* CONFIG_H */
