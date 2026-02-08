/**
 * ============================================================================
 * PROGRAM 09: ESP32 Task Watchdog Timer (TWDT)
 * ============================================================================
 * Demonstrasi Task Watchdog Timer ESP-IDF
 * 
 * Fitur utama:
 *   - esp_task_wdt_init() untuk inisialisasi TWDT
 *   - esp_task_wdt_add() untuk mendaftarkan task ke TWDT
 *   - esp_task_wdt_reset() untuk feed/reset watchdog
 *   - esp_task_wdt_delete() untuk unsubscribe dari TWDT
 *   - Deteksi dan penanganan timeout watchdog
 * 
 * Skenario demo:
 *   1. Semua task feed WDT dengan benar
 *   2. Satu task mulai terlambat feed
 *   3. Satu task sengaja tidak feed (trigger timeout)
 *   4. Recovery - task nakal di-unsubscribe dari WDT
 * 
 * Platform: ESP32 DevKit V1
 * Framework: ESP-IDF
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "config.h"

/* Tag untuk logging */
static const char *TAG = "WATCHDOG";

/* ======================== STRUKTUR DATA ================================== */

/**
 * Struktur untuk mencatat event watchdog
 */
typedef struct {
    int64_t timestamp;          /* Waktu event (us) */
    char task_name[16];         /* Nama task */
    char event_type[16];        /* Jenis event: FEED, TIMEOUT, RECOVER */
    uint32_t feed_count;        /* Jumlah feed yang berhasil */
} wdt_event_t;

/**
 * Struktur statistik per task
 */
typedef struct {
    char name[16];              /* Nama task */
    uint32_t feed_count;        /* Jumlah feed berhasil */
    uint32_t timeout_count;     /* Jumlah timeout terjadi */
    int64_t last_feed_time;     /* Waktu feed terakhir */
    bool subscribed;            /* Apakah task terdaftar di WDT */
    bool is_blocking;           /* Apakah task sedang blocking */
} task_wdt_stats_t;

/* ======================== VARIABEL GLOBAL ================================ */

/* Handle task */
static TaskHandle_t good_task1_handle = NULL;
static TaskHandle_t good_task2_handle = NULL;
static TaskHandle_t bad_task_handle = NULL;
static TaskHandle_t monitor_handle = NULL;
static TaskHandle_t controller_handle = NULL;

/* Statistik */
static task_wdt_stats_t good1_stats = {.name = "GoodTask1"};
static task_wdt_stats_t good2_stats = {.name = "GoodTask2"};
static task_wdt_stats_t bad_stats = {.name = "BadTask"};

/* Event log */
static wdt_event_t event_log[MAX_WDT_VIOLATIONS];
static volatile int event_count = 0;

/* Skenario kontrol */
static volatile int current_scenario = 0;
static volatile bool bad_task_should_feed = true;
static volatile bool bad_task_should_block = false;
static volatile int bad_task_extra_delay_ms = 0;
static volatile uint32_t wdt_violation_count = 0;

/* LED state */
static volatile int led_blink_rate_ms = LED_BLINK_NORMAL_MS;

/* ======================== FUNGSI UTILITAS ================================ */

/**
 * Inisialisasi GPIO untuk LED
 */
static void init_gpio(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << LED_WARNING_PIN) | (1ULL << LED_STATUS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf);
    gpio_set_level(LED_WARNING_PIN, 0);
    gpio_set_level(LED_STATUS_PIN, 0);
}

/**
 * Catat event watchdog ke log
 */
static void log_wdt_event(const char *task_name, const char *event_type, uint32_t feed_count)
{
    if (event_count < MAX_WDT_VIOLATIONS) {
        wdt_event_t *evt = &event_log[event_count];
        evt->timestamp = esp_timer_get_time();
        strncpy(evt->task_name, task_name, sizeof(evt->task_name) - 1);
        strncpy(evt->event_type, event_type, sizeof(evt->event_type) - 1);
        evt->feed_count = feed_count;
        event_count++;
    }
}

/* ======================== TASK FUNCTIONS ================================= */

/**
 * Task yang rajin feed watchdog (#1)
 * 
 * Task ini selalu tepat waktu melakukan feed/reset watchdog.
 * Ini menunjukkan perilaku task yang benar - selalu merespons
 * dalam batas waktu timeout yang ditentukan.
 * 
 * @param pvParameters Tidak digunakan
 */
static void good_task_1(void *pvParameters)
{
    ESP_LOGI(TAG, "GoodTask1 dimulai pada Core %d", xPortGetCoreID());

    /* Daftarkan task ini ke watchdog */
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    good1_stats.subscribed = true;
    printf("[DATA]WDT_ADD,task=GoodTask1,status=subscribed\n");

    while (1) {
        /* Selalu feed watchdog tepat waktu */
        esp_err_t ret = esp_task_wdt_reset();
        if (ret == ESP_OK) {
            good1_stats.feed_count++;
            good1_stats.last_feed_time = esp_timer_get_time();

            /* LED status berkedip normal */
            gpio_set_level(LED_STATUS_PIN, good1_stats.feed_count % 2);

            printf("[DATA]WDT_FEED,task=GoodTask1,count=%lu,time_us=%lld\n",
                   good1_stats.feed_count, good1_stats.last_feed_time);
        }

        /* Delay yang aman (jauh di bawah timeout WDT) */
        vTaskDelay(pdMS_TO_TICKS(GOOD_FEED_INTERVAL_MS));
    }
}

/**
 * Task yang rajin feed watchdog (#2)
 * 
 * Task kedua yang juga berperilaku baik. Digunakan untuk
 * menunjukkan bahwa banyak task bisa subscribe ke WDT secara
 * bersamaan dan SEMUA harus feed tepat waktu.
 * 
 * @param pvParameters Tidak digunakan
 */
static void good_task_2(void *pvParameters)
{
    ESP_LOGI(TAG, "GoodTask2 dimulai pada Core %d", xPortGetCoreID());

    /* Daftarkan task ini ke watchdog */
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    good2_stats.subscribed = true;
    printf("[DATA]WDT_ADD,task=GoodTask2,status=subscribed\n");

    while (1) {
        /* Feed watchdog */
        esp_err_t ret = esp_task_wdt_reset();
        if (ret == ESP_OK) {
            good2_stats.feed_count++;
            good2_stats.last_feed_time = esp_timer_get_time();

            printf("[DATA]WDT_FEED,task=GoodTask2,count=%lu,time_us=%lld\n",
                   good2_stats.feed_count, good2_stats.last_feed_time);
        }

        /* Lakukan sedikit pekerjaan */
        volatile float calc = 0;
        for (int i = 0; i < 10000; i++) {
            calc += i * 0.001f;
        }

        vTaskDelay(pdMS_TO_TICKS(GOOD_FEED_INTERVAL_MS));
    }
}

/**
 * Task yang sengaja tidak feed watchdog (nakal)
 * 
 * Perilaku task ini dikendalikan oleh task controller:
 *   - Skenario 1: Feed normal
 *   - Skenario 2: Feed terlambat (delay ekstra)
 *   - Skenario 3: Tidak feed sama sekali (blocking)
 *   - Skenario 4: Di-unsubscribe dari WDT
 * 
 * @param pvParameters Tidak digunakan
 */
static void bad_task(void *pvParameters)
{
    ESP_LOGI(TAG, "BadTask dimulai pada Core %d", xPortGetCoreID());

    /* Daftarkan task ini ke watchdog */
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    bad_stats.subscribed = true;
    printf("[DATA]WDT_ADD,task=BadTask,status=subscribed\n");

    while (1) {
        if (bad_task_should_feed && !bad_task_should_block) {
            /* Mode normal atau terlambat */
            if (bad_task_extra_delay_ms > 0) {
                /* Delay ekstra sebelum feed - bisa menyebabkan timeout */
                ESP_LOGW(TAG, "BadTask delay ekstra %d ms sebelum feed...",
                         bad_task_extra_delay_ms);
                vTaskDelay(pdMS_TO_TICKS(bad_task_extra_delay_ms));

                /* LED warning berkedip sesuai proximity ke timeout */
                int remaining = (WDT_TIMEOUT_SEC * 1000) - bad_task_extra_delay_ms;
                if (remaining < 1000) {
                    led_blink_rate_ms = LED_BLINK_DANGER_MS;
                } else if (remaining < 3000) {
                    led_blink_rate_ms = LED_BLINK_WARNING_MS;
                }
            }

            /* Feed watchdog */
            esp_err_t ret = esp_task_wdt_reset();
            if (ret == ESP_OK) {
                bad_stats.feed_count++;
                bad_stats.last_feed_time = esp_timer_get_time();
                log_wdt_event("BadTask", "FEED", bad_stats.feed_count);

                printf("[DATA]WDT_FEED,task=BadTask,count=%lu,delay_extra=%d\n",
                       bad_stats.feed_count, bad_task_extra_delay_ms);
            }
        } else if (bad_task_should_block) {
            /**
             * Mode blocking - TIDAK feed watchdog
             * Ini akan memicu timeout WDT setelah WDT_TIMEOUT_SEC detik
             * 
             * Dalam skenario nyata, ini bisa terjadi karena:
             *   - Infinite loop tanpa yield
             *   - Deadlock (menunggu resource yang tidak pernah tersedia)
             *   - Task yang memproses data terlalu lama
             */
            ESP_LOGW(TAG, "BadTask BLOCKING - tidak akan feed WDT!");
            printf("[DATA]WDT_BLOCK,task=BadTask,blocking=true\n");

            led_blink_rate_ms = LED_BLINK_DANGER_MS;

            /* Blocking selama waktu yang lama tanpa feed */
            vTaskDelay(pdMS_TO_TICKS(BAD_TASK_BLOCK_TIME_MS));

            bad_stats.timeout_count++;
            wdt_violation_count++;
            log_wdt_event("BadTask", "TIMEOUT", bad_stats.feed_count);

            printf("[DATA]WDT_TIMEOUT,task=BadTask,violations=%lu\n",
                   wdt_violation_count);
        } else {
            /* Task di-unsubscribe - tidak perlu feed */
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        vTaskDelay(pdMS_TO_TICKS(GOOD_FEED_INTERVAL_MS));
    }
}

/**
 * Task pengendali skenario demo
 * 
 * Mengatur perilaku bad_task melalui 4 skenario:
 *   1. Normal   - semua task feed WDT dengan benar
 *   2. Lambat   - bad_task mulai terlambat feed
 *   3. Blocking - bad_task tidak feed sama sekali
 *   4. Recovery - bad_task di-unsubscribe dari WDT
 * 
 * @param pvParameters Tidak digunakan
 */
static void controller_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Controller dimulai. Mengatur skenario demo...");

    while (1) {
        /* === SKENARIO 1: Semua task normal === */
        current_scenario = 1;
        bad_task_should_feed = true;
        bad_task_should_block = false;
        bad_task_extra_delay_ms = 0;
        led_blink_rate_ms = LED_BLINK_NORMAL_MS;

        printf("\n[DATA]SCENARIO,num=1,desc=Normal_Operation\n");
        ESP_LOGI(TAG, "=== SKENARIO 1: Operasi Normal ===");
        ESP_LOGI(TAG, "Semua task feed WDT tepat waktu");

        vTaskDelay(pdMS_TO_TICKS(SCENARIO_DURATION_MS));

        /* === SKENARIO 2: Bad task mulai terlambat === */
        current_scenario = 2;
        bad_task_extra_delay_ms = 2000;  /* 2 detik delay ekstra */

        printf("\n[DATA]SCENARIO,num=2,desc=Late_Feed\n");
        ESP_LOGI(TAG, "=== SKENARIO 2: Feed Terlambat ===");
        ESP_LOGI(TAG, "BadTask feed terlambat %d ms", bad_task_extra_delay_ms);

        vTaskDelay(pdMS_TO_TICKS(SCENARIO_DURATION_MS));

        /* Tingkatkan delay - semakin dekat ke timeout */
        bad_task_extra_delay_ms = 4000;  /* 4 detik - hampir timeout (5s) */

        printf("[DATA]DELAY_INCREASE,new_delay_ms=%d,timeout_sec=%d\n",
               bad_task_extra_delay_ms, WDT_TIMEOUT_SEC);

        vTaskDelay(pdMS_TO_TICKS(SCENARIO_DURATION_MS));

        /* === SKENARIO 3: Bad task blocking total === */
        current_scenario = 3;
        bad_task_should_block = true;

        printf("\n[DATA]SCENARIO,num=3,desc=Watchdog_Timeout\n");
        ESP_LOGW(TAG, "=== SKENARIO 3: WDT Timeout ===");
        ESP_LOGW(TAG, "BadTask akan TIDAK feed WDT - timeout diharapkan!");

        vTaskDelay(pdMS_TO_TICKS(SCENARIO_DURATION_MS));

        /* === SKENARIO 4: Recovery === */
        current_scenario = 4;
        bad_task_should_block = false;
        bad_task_should_feed = false;
        led_blink_rate_ms = LED_BLINK_NORMAL_MS;

        printf("\n[DATA]SCENARIO,num=4,desc=Recovery\n");
        ESP_LOGI(TAG, "=== SKENARIO 4: Recovery ===");
        ESP_LOGI(TAG, "Unsubscribe BadTask dari WDT...");

        /* Unsubscribe bad task dari WDT */
        if (bad_stats.subscribed) {
            esp_err_t ret = esp_task_wdt_delete(bad_task_handle);
            if (ret == ESP_OK) {
                bad_stats.subscribed = false;
                printf("[DATA]WDT_DELETE,task=BadTask,status=unsubscribed\n");
                ESP_LOGI(TAG, "BadTask berhasil di-unsubscribe dari WDT");
            } else {
                ESP_LOGE(TAG, "Gagal unsubscribe BadTask: %s", esp_err_to_name(ret));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SCENARIO_DURATION_MS));

        /* Re-subscribe untuk siklus berikutnya */
        ESP_LOGI(TAG, "Re-subscribe BadTask ke WDT untuk siklus berikutnya...");
        if (!bad_stats.subscribed) {
            esp_err_t ret = esp_task_wdt_add(bad_task_handle);
            if (ret == ESP_OK) {
                bad_stats.subscribed = true;
                bad_task_should_feed = true;
                printf("[DATA]WDT_ADD,task=BadTask,status=re-subscribed\n");
            }
        }

        printf("[DATA]CYCLE_COMPLETE,violations=%lu\n", wdt_violation_count);
    }
}

/**
 * Task LED warning - berkedip sesuai status watchdog
 * 
 * @param pvParameters Tidak digunakan
 */
static void led_warning_task(void *pvParameters)
{
    while (1) {
        gpio_set_level(LED_WARNING_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(led_blink_rate_ms));
        gpio_set_level(LED_WARNING_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(led_blink_rate_ms));
    }
}

/**
 * Task monitor - cetak ringkasan statistik
 * 
 * @param pvParameters Tidak digunakan
 */
static void monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Monitor dimulai pada Core %d", xPortGetCoreID());

    vTaskDelay(pdMS_TO_TICKS(2000));

    uint32_t cycle = 0;
    while (1) {
        cycle++;

        printf("\n========================================\n");
        printf("  WATCHDOG MONITOR - Siklus #%lu\n", cycle);
        printf("  Skenario: %d\n", current_scenario);
        printf("========================================\n");

        /* Statistik setiap task */
        printf("[DATA]MONITOR,task=GoodTask1,feeds=%lu,subscribed=%d\n",
               good1_stats.feed_count, good1_stats.subscribed);
        printf("[DATA]MONITOR,task=GoodTask2,feeds=%lu,subscribed=%d\n",
               good2_stats.feed_count, good2_stats.subscribed);
        printf("[DATA]MONITOR,task=BadTask,feeds=%lu,timeouts=%lu,subscribed=%d,blocking=%d\n",
               bad_stats.feed_count, bad_stats.timeout_count,
               bad_stats.subscribed, bad_task_should_block);

        /* Total violation */
        printf("[DATA]VIOLATIONS,total=%lu,scenario=%d\n",
               wdt_violation_count, current_scenario);

        /* Heap info */
        printf("[DATA]HEAP,free=%lu,min=%lu\n",
               (uint32_t)esp_get_free_heap_size(),
               (uint32_t)esp_get_minimum_free_heap_size());

        /* Event log terakhir */
        if (event_count > 0) {
            int start = (event_count > 5) ? event_count - 5 : 0;
            for (int i = start; i < event_count; i++) {
                printf("[DATA]EVENT,ts=%lld,task=%s,type=%s,feeds=%lu\n",
                       event_log[i].timestamp / 1000,
                       event_log[i].task_name,
                       event_log[i].event_type,
                       event_log[i].feed_count);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
    }
}

/* ======================== ENTRY POINT ==================================== */

/**
 * Fungsi utama aplikasi
 */
void app_main(void)
{
    printf("\n");
    printf("============================================================\n");
    printf("  ESP32 FreeRTOS - Task Watchdog Timer (TWDT) Demo\n");
    printf("  Demonstrasi esp_task_wdt_init/add/reset/delete\n");
    printf("============================================================\n");
    printf("  WDT Timeout: %d detik\n", WDT_TIMEOUT_SEC);
    printf("  Panic on timeout: %s\n", WDT_PANIC_ENABLE ? "YES" : "NO");
    printf("  Free Heap: %lu bytes\n", (uint32_t)esp_get_free_heap_size());
    printf("============================================================\n\n");

    /* 1. Inisialisasi GPIO */
    init_gpio();

    /* 2. Inisialisasi Task Watchdog Timer */
    ESP_LOGI(TAG, "Inisialisasi Task Watchdog Timer...");
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,            /* Jangan subscribe idle task */
        .trigger_panic = WDT_PANIC_ENABLE
    };
    esp_err_t ret = esp_task_wdt_reconfigure(&wdt_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WDT reconfigure: %s, mencoba init...", esp_err_to_name(ret));
        ret = esp_task_wdt_init(&wdt_config);
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGI(TAG, "WDT sudah diinisialisasi, melanjutkan...");
        }
    }

    printf("[DATA]WDT_INIT,timeout_sec=%d,panic=%d\n",
           WDT_TIMEOUT_SEC, WDT_PANIC_ENABLE);

    /* 3. Buat task yang rajin feed WDT */
    xTaskCreatePinnedToCore(good_task_1, "GoodTask1", GOOD_TASK_STACK, NULL,
                            GOOD_TASK_PRIORITY, &good_task1_handle, 1);
    xTaskCreatePinnedToCore(good_task_2, "GoodTask2", GOOD_TASK_STACK, NULL,
                            GOOD_TASK_PRIORITY, &good_task2_handle, 1);

    /* 4. Buat task nakal */
    xTaskCreatePinnedToCore(bad_task, "BadTask", BAD_TASK_STACK, NULL,
                            BAD_TASK_PRIORITY, &bad_task_handle, 1);

    /* 5. Buat task LED warning */
    xTaskCreatePinnedToCore(led_warning_task, "LEDWarn", 2048, NULL,
                            1, NULL, 0);

    /* 6. Buat task monitor */
    xTaskCreatePinnedToCore(monitor_task, "Monitor", MONITOR_TASK_STACK, NULL,
                            MONITOR_TASK_PRIORITY, &monitor_handle, 0);

    /* 7. Buat task pengendali skenario */
    xTaskCreatePinnedToCore(controller_task, "Controller", CONTROLLER_TASK_STACK, NULL,
                            CONTROLLER_TASK_PRIORITY, &controller_handle, 0);

    ESP_LOGI(TAG, "Semua task dimulai. Demo WDT berjalan...");
    printf("[DATA]STATUS,system=started,tasks=6\n");
}
