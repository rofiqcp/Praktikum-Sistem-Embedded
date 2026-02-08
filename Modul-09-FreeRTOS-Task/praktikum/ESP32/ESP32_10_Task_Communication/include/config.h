/**
 * ============================================================================
 * ESP32_10_Task_Communication - Konfigurasi Parameter
 * ============================================================================
 * Demonstrasi komunikasi TIDAK AMAN antar task menggunakan global variable
 * Menunjukkan mengapa mutex/semaphore dibutuhkan (setup untuk Modul 10)
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== KONFIGURASI LED ================================ */
#define LED_CORRUPTION_PIN      2       /* LED indikator data korupsi pada GPIO2 */
#define LED_STATUS_PIN          4       /* LED status normal pada GPIO4 */

/* ======================== KONFIGURASI TASK =============================== */
/* Prioritas task */
#define PRODUCER_TASK_PRIORITY  3       /* Prioritas task producer */
#define CONSUMER_TASK_PRIORITY  3       /* Prioritas task consumer */
#define MONITOR_TASK_PRIORITY   5       /* Prioritas task monitor */
#define FAST_PROD_PRIORITY      4       /* Prioritas producer cepat */
#define FAST_CONS_PRIORITY      4       /* Prioritas consumer cepat */

/* Ukuran stack (dalam bytes) */
#define PRODUCER_TASK_STACK     4096    /* Stack task producer */
#define CONSUMER_TASK_STACK     4096    /* Stack task consumer */
#define MONITOR_TASK_STACK      4096    /* Stack task monitor */

/* =================== KONFIGURASI SHARED DATA ============================= */
#define SHARED_BUFFER_SIZE      8       /* Ukuran buffer shared (elemen) */
#define SHARED_ARRAY_SIZE       16      /* Ukuran array dalam shared struct */
#define MAGIC_VALUE             0xDEADBEEF  /* Nilai sentinel untuk deteksi korupsi */

/* =================== KONFIGURASI TIMING ================================== */
#define PRODUCER_DELAY_MS       0       /* Delay producer (0 = secepat mungkin) */
#define CONSUMER_DELAY_MS       0       /* Delay consumer (0 = secepat mungkin) */
#define MONITOR_INTERVAL_MS     2000    /* Interval laporan monitor (ms) */
#define TEST_DURATION_SEC       60      /* Durasi tes keseluruhan (detik) */

/* =================== KONFIGURASI DETEKSI ================================= */
#define MAX_CORRUPTION_LOG      100     /* Maksimal log event korupsi */
#define CORRUPTION_LED_BLINK_MS 50      /* Kedip LED saat korupsi terdeteksi */

/* =================== KONFIGURASI SERIAL ================================== */
#define SERIAL_BAUD_RATE        115200  /* Baud rate komunikasi serial */

#endif /* CONFIG_H */
