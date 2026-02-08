/**
 * ============================================================================
 * ESP32_12_Task_Cooperative - Konfigurasi Parameter
 * ============================================================================
 * Demonstrasi perbedaan cooperative vs preemptive scheduling
 * Round-robin, taskYIELD(), dan CPU hogging
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== KONFIGURASI LED ================================ */
#define LED1_PIN                2       /* LED1 pada GPIO2 */
#define LED2_PIN                4       /* LED2 pada GPIO4 */

/* ======================== KONFIGURASI TASK =============================== */
/* Prioritas task - semua sama untuk demonstrasi round-robin */
#define WORKER_TASK_PRIORITY    3       /* Prioritas semua worker task (sama) */
#define MONITOR_TASK_PRIORITY   5       /* Prioritas task monitor (lebih tinggi) */
#define CONTROLLER_PRIORITY     6       /* Prioritas task pengendali fase */

/* Ukuran stack (dalam bytes) */
#define WORKER_TASK_STACK       4096    /* Stack worker task */
#define MONITOR_TASK_STACK      4096    /* Stack monitor task */
#define CONTROLLER_TASK_STACK   4096    /* Stack controller task */

/* Jumlah task */
#define NUM_WORKER_TASKS        3       /* Jumlah worker task */

/* =================== KONFIGURASI TIMING ================================== */
#define PHASE_DURATION_MS       12000   /* Durasi setiap fase (ms) */
#define MONITOR_INTERVAL_MS     2000    /* Interval laporan monitor (ms) */
#define YIELD_INTERVAL_MS       50      /* Interval yield pada mode cooperative */

/* =================== KONFIGURASI TIME SLICE ============================== */
#define SLICE_MEASURE_WINDOW_US 1000000 /* Jendela pengukuran time slice (1 detik) */
#define MAX_SLICE_SAMPLES       100     /* Maksimal sampel time slice */
#define WORK_CHUNK_ITERATIONS   5000    /* Iterasi per chunk kerja */

/* =================== KONFIGURASI FASE ==================================== */
#define NUM_PHASES              3       /* Jumlah fase demonstrasi */
/* Fase 0: Preemptive (round-robin default) */
/* Fase 1: Cooperative (taskYIELD() eksplisit) */
/* Fase 2: CPU Hogging (satu task monopoli) */

/* =================== KONFIGURASI LED PATTERN ============================= */
#define LED_PREEMPTIVE_MS       500     /* Pola LED fase preemptive (ms) */
#define LED_COOPERATIVE_MS      200     /* Pola LED fase cooperative (ms) */
#define LED_HOGGING_MS          1000    /* Pola LED fase hogging (ms) */

/* =================== KONFIGURASI SERIAL ================================== */
#define SERIAL_BAUD_RATE        115200  /* Baud rate komunikasi serial */

#endif /* CONFIG_H */
