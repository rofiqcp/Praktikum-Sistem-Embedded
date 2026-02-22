/**
 * ============================================================================
 * ESP32_09_Task_Watchdog - Konfigurasi Parameter
 * ============================================================================
 * Demonstrasi Task Watchdog Timer (TWDT) ESP-IDF
 * esp_task_wdt_init(), esp_task_wdt_add(), esp_task_wdt_reset()
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ======================== KONFIGURASI LED ================================ */
#define LED_WARNING_PIN         2       /* LED peringatan pada GPIO2 */
#define LED_STATUS_PIN          4       /* LED status pada GPIO4 */

/* ======================== KONFIGURASI WATCHDOG ============================ */
#define WDT_TIMEOUT_SEC         5       /* Timeout watchdog (detik) */
#define WDT_PANIC_ENABLE        false   /* true = reset ESP32 saat timeout */

/* ======================== KONFIGURASI TASK =============================== */
/* Prioritas task */
#define GOOD_TASK_PRIORITY      3       /* Task yang rajin feed WDT */
#define BAD_TASK_PRIORITY        3       /* Task yang sengaja tidak feed WDT */
#define RECOVERY_TASK_PRIORITY  4       /* Task pemulihan */
#define MONITOR_TASK_PRIORITY   5       /* Task monitor */
#define CONTROLLER_TASK_PRIORITY 5      /* Task pengendali skenario */

/* Ukuran stack (dalam bytes) */
#define GOOD_TASK_STACK         4096    /* Stack task yang baik */
#define BAD_TASK_STACK          4096    /* Stack task yang buruk */
#define RECOVERY_TASK_STACK     4096    /* Stack task pemulihan */
#define MONITOR_TASK_STACK      4096    /* Stack task monitor */
#define CONTROLLER_TASK_STACK   4096    /* Stack task pengendali */

/* =================== KONFIGURASI TIMING ================================== */
#define GOOD_FEED_INTERVAL_MS   1000    /* Interval feed WDT task baik (ms) */
#define BAD_TASK_BLOCK_TIME_MS  8000    /* Waktu task buruk memblokir (ms) */
#define MONITOR_INTERVAL_MS     1000    /* Interval laporan monitor (ms) */
#define SCENARIO_DURATION_MS    15000   /* Durasi setiap skenario demo (ms) */

/* =================== KONFIGURASI LED BLINK =============================== */
#define LED_BLINK_NORMAL_MS     500     /* Kedip normal (ms) */
#define LED_BLINK_WARNING_MS    100     /* Kedip peringatan (ms) */
#define LED_BLINK_DANGER_MS     50      /* Kedip bahaya/cepat (ms) */

/* =================== KONFIGURASI SKENARIO ================================ */
#define NUM_SCENARIOS           4       /* Jumlah skenario demo */
#define MAX_WDT_VIOLATIONS      10      /* Maksimal pelanggaran sebelum recovery */

/* =================== KONFIGURASI SERIAL ================================== */
#define SERIAL_BAUD_RATE        115200  /* Baud rate komunikasi serial */

#endif /* CONFIG_H */
