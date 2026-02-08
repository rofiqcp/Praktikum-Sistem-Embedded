/**
 * ============================================================================
 * ESP32_11_Task_Scheduler_Info - Konfigurasi Parameter
 * ============================================================================
 * Menampilkan informasi scheduler FreeRTOS secara detail
 * vTaskList(), vTaskGetRunTimeStats(), uxTaskGetSystemState()
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== KONFIGURASI LED ================================ */
#define LED_STATUS_PIN          2       /* LED status pada GPIO2 */

/* ======================== KONFIGURASI TASK =============================== */
/* Prioritas task */
#define MONITOR_TASK_PRIORITY   5       /* Prioritas task monitor (tertinggi) */
#define DYNAMIC_TASK_PRIORITY   2       /* Prioritas task dinamis */
#define WORKER_TASK_PRIORITY    3       /* Prioritas task worker */
#define BLOCKED_TASK_PRIORITY   2       /* Prioritas task yang blocked */
#define SUSPENDED_TASK_PRIORITY 2       /* Prioritas task yang suspended */

/* Ukuran stack (dalam bytes) */
#define MONITOR_TASK_STACK      8192    /* Stack monitor (besar untuk formatting) */
#define DYNAMIC_TASK_STACK      4096    /* Stack task dinamis */
#define WORKER_TASK_STACK       4096    /* Stack task worker */
#define BLOCKED_TASK_STACK      2048    /* Stack task blocked */
#define SUSPENDED_TASK_STACK    2048    /* Stack task suspended */

/* =================== KONFIGURASI TIMING ================================== */
#define REPORT_INTERVAL_MS      3000    /* Interval cetak laporan (ms) */
#define DYNAMIC_CREATE_MS       10000   /* Interval buat task dinamis (ms) */
#define DYNAMIC_LIFETIME_MS     6000    /* Umur task dinamis (ms) */
#define WORKER_DELAY_MS         500     /* Delay worker task (ms) */

/* =================== KONFIGURASI TABEL =================================== */
#define TASK_LIST_BUFFER_SIZE   2048    /* Ukuran buffer untuk vTaskList() */
#define RUNTIME_BUFFER_SIZE     2048    /* Ukuran buffer untuk runtime stats */
#define MAX_TASKS               20      /* Maksimal task yang di-track */

/* =================== KONFIGURASI FASE ==================================== */
#define NUM_PHASES              4       /* Jumlah fase demonstrasi */
#define PHASE_DURATION_MS       15000   /* Durasi setiap fase (ms) */

/* =================== KONFIGURASI SERIAL ================================== */
#define SERIAL_BAUD_RATE        115200  /* Baud rate komunikasi serial */

#endif /* CONFIG_H */
