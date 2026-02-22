/**
 * ============================================================================
 * ESP32_08_Task_Idle_Hook - Konfigurasi Parameter
 * ============================================================================
 * Monitor idle task pada kedua core ESP32 untuk estimasi penggunaan CPU
 * Menggunakan esp_register_freertos_idle_hook_for_cpu()
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== KONFIGURASI TASK =============================== */
/* Prioritas task */
#define LOAD_TASK_PRIORITY      2       /* Prioritas task pemberi beban */
#define MONITOR_TASK_PRIORITY   5       /* Prioritas task monitor (tertinggi) */

/* Ukuran stack (dalam bytes) */
#define LOAD_TASK_STACK         4096    /* Stack size task beban */
#define MONITOR_TASK_STACK      4096    /* Stack size task monitor */

/* ================== KONFIGURASI PENGUKURAN =============================== */
#define MEASUREMENT_INTERVAL_MS 2000    /* Interval pengukuran CPU usage (ms) */
#define IDLE_SAMPLE_WINDOW_MS   1000    /* Jendela sampling idle counter (ms) */

/* ================ KONFIGURASI BEBAN VARIABEL ============================= */
/* Task beban akan bervariasi antara ringan dan berat secara siklis */
#define LOAD_PHASE_DURATION_MS  5000    /* Durasi setiap fase beban (ms) */
#define LOAD_LEVEL_COUNT        5       /* Jumlah level beban berbeda */

/* Level beban dalam persen (0-100) */
#define LOAD_LEVEL_0            0       /* Tanpa beban (idle) */
#define LOAD_LEVEL_1            25      /* Beban 25% */
#define LOAD_LEVEL_2            50      /* Beban 50% */
#define LOAD_LEVEL_3            75      /* Beban 75% */
#define LOAD_LEVEL_4            95      /* Beban 95% (hampir penuh) */

/* ================ KONFIGURASI BUSY-WAIT ================================== */
#define BUSY_LOOP_ITERATIONS    10000   /* Iterasi per siklus busy-wait */
#define BUSY_LOOP_DELAY_US      10      /* Delay antar iterasi (mikro detik) */

/* ================ KONFIGURASI LED ======================================== */
#define LED_STATUS_PIN          2       /* LED status pada GPIO2 */

/* ================ KONFIGURASI SERIAL ===================================== */
#define SERIAL_BAUD_RATE        115200  /* Baud rate komunikasi serial */

/* ================ KONFIGURASI HISTOGRAM ================================== */
#define CPU_HISTORY_SIZE        30      /* Jumlah sampel history CPU usage */

#endif /* CONFIG_H */
