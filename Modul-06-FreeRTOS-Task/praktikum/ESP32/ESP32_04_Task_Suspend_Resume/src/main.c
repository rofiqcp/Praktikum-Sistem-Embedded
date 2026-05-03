/* ============================================================
 * ESP32_04_Task_Suspend_Resume - Suspend/Resume Task FreeRTOS
 * ============================================================
 * Program ini mendemonstrasikan mekanisme suspend dan resume
 * task di FreeRTOS, termasuk penggunaan dari ISR.
 *
 * Konsep yang dipelajari:
 * 1. vTaskSuspend() - menangguhkan task tertentu
 * 2. vTaskResume() - melanjutkan task yang di-suspend
 * 3. xTaskResumeFromISR() - resume dari Interrupt Service Routine
 * 4. vTaskSuspendAll() - suspend scheduler (semua task)
 * 5. xTaskResumeAll() - resume scheduler
 * 6. Task state transitions (Running->Suspended->Ready)
 * 7. GPIO interrupt pada ESP32
 *
 * Hardware:
 * - ESP32 DOIT DevKit V1
 * - LED1 GPIO2 (blinker task)
 * - LED2 GPIO4 (status indicator)
 * - Button GPIO0 (BOOT button, active LOW)
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "config.h"

static const char *TAG = "SUSP_RES";

/* Task handles */
static TaskHandle_t xBlinkerHandle = NULL;
static TaskHandle_t xMonitorHandle = NULL;
static TaskHandle_t xSchedDemoHandle = NULL;
static TaskHandle_t xISRDemoHandle = NULL;

/* State tracking */
static volatile bool blinker_suspended = false;
static volatile uint32_t blinker_count = 0;
static volatile uint32_t suspend_count = 0;
static volatile uint32_t resume_count = 0;
static volatile uint32_t isr_resume_count = 0;
static volatile uint32_t scheduler_suspend_count = 0;

/* Debounce ISR */
static volatile int64_t last_isr_time = 0;
static volatile bool use_isr_mode = false;

/* Timestamp awal */
static int64_t start_time_us = 0;

static uint32_t get_elapsed_ms(void)
{
    return (uint32_t)((esp_timer_get_time() - start_time_us) / 1000);
}

/**
 * @brief Mendapatkan string state task
 */
static const char* task_state_str(eTaskState state)
{
    switch (state) {
        case eRunning:   return "RUNNING";
        case eReady:     return "READY";
        case eBlocked:   return "BLOCKED";
        case eSuspended: return "SUSPENDED";
        case eDeleted:   return "DELETED";
        default:         return "UNKNOWN";
    }
}

/* Inisialisasi GPIO */
static void init_gpio(void)
{
    /* LED output */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED1_GPIO, 0);
    gpio_set_level(LED2_GPIO, 0);

    /* Button input (tanpa interrupt dulu, akan di-setup untuk ISR demo) */
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_conf);
}

/* ============================================================
 * ISR Handler untuk Button GPIO
 * ============================================================
 * Interrupt Service Routine dipanggil saat button ditekan.
 * PENTING: Di dalam ISR, kita HANYA boleh menggunakan API
 * FreeRTOS yang aman untuk ISR (berakhiran "FromISR").
 *
 * xTaskResumeFromISR() men-resume task dari dalam ISR.
 * Return pdTRUE jika perlu context switch setelah ISR.
 *
 * CATATAN ESP32: Pada ESP32, ISR harus di-register dengan
 * gpio_install_isr_service() dan gpio_isr_handler_add().
 * ============================================================ */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    /*
     * Debounce: abaikan interrupt jika terlalu cepat
     * PENTING: Di ISR, gunakan esp_timer_get_time() karena
     * aman dipanggil dari ISR pada ESP32.
     */
    int64_t now = esp_timer_get_time();
    if ((now - last_isr_time) < BUTTON_DEBOUNCE_US) {
        return;
    }
    last_isr_time = now;

    /*
     * xTaskResumeFromISR() - Resume task dari ISR
     *
     * Berbeda dengan vTaskResume() yang dipanggil dari task,
     * fungsi ini aman digunakan di dalam ISR context.
     *
     * Return value:
     * - pdTRUE: task yang di-resume memiliki prioritas lebih tinggi
     *   dari task yang interrupted -> perlu context switch
     * - pdFALSE: tidak perlu context switch
     *
     * PENTING: Harus menggunakan portYIELD_FROM_ISR() jika
     * return pdTRUE, agar context switch segera terjadi.
     */
    if (xISRDemoHandle != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (use_isr_mode && eTaskGetState(xBlinkerHandle) == eSuspended) {
            xHigherPriorityTaskWoken = xTaskResumeFromISR(xBlinkerHandle);
            isr_resume_count++;
        }

        /*
         * portYIELD_FROM_ISR() memaksa context switch jika
         * task yang baru ready memiliki prioritas lebih tinggi.
         * Tanpa ini, task baru ready harus menunggu scheduler
         * tick berikutnya.
         */
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}

/**
 * @brief Setup GPIO interrupt untuk button
 *
 * ESP32 GPIO interrupt menggunakan service model:
 * 1. gpio_install_isr_service() - install ISR service
 * 2. gpio_set_intr_type() - set tipe interrupt
 * 3. gpio_isr_handler_add() - register callback per pin
 */
static void setup_button_isr(void)
{
    /* Set interrupt type: falling edge (button ditekan = LOW) */
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);

    /* Install GPIO ISR service dengan default flags */
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    /* Register ISR handler untuk button pin */
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    ESP_LOGI(TAG, "Button ISR terpasang pada GPIO%d", BUTTON_GPIO);
}

/* ============================================================
 * TASK: Blinker - LED yang bisa di-suspend/resume
 * ============================================================
 * Task ini mengedipkan LED1 secara continuous.
 * Bisa di-suspend dari task lain atau dari ISR.
 *
 * Ketika di-suspend:
 * - Task masuk state SUSPENDED
 * - Tidak menggunakan CPU sama sekali
 * - LED berhenti pada state terakhir
 * - Semua context (variabel lokal, stack) dipertahankan
 *
 * Ketika di-resume:
 * - Task kembali ke state READY
 * - Melanjutkan dari titik terakhir (setelah vTaskDelay)
 * - Variabel lokal tetap utuh
 * ============================================================ */
static void blinker_task(void *pvParameters)
{
    uint8_t led_state = 0;

    ESP_LOGI(TAG, "[BLINKER] Task dimulai - GPIO%d, period=%dms",
             LED1_GPIO, BLINK_PERIOD_MS);

    while (1) {
        led_state = !led_state;
        gpio_set_level(LED1_GPIO, led_state);
        blinker_count++;

        if (blinker_count % 10 == 0) {
            printf("[DATA] BLINK,%lu,%d,%lu,%s\n",
                   (unsigned long)get_elapsed_ms(),
                   led_state,
                   (unsigned long)blinker_count,
                   blinker_suspended ? "SUSPENDED" : "RUNNING");
        }

        /*
         * Saat vTaskDelay() dipanggil dan task di-suspend,
         * task akan tetap SUSPENDED bahkan setelah delay selesai.
         * Task hanya bisa keluar dari SUSPENDED via vTaskResume().
         */
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}

/* ============================================================
 * TASK: Monitor - Memantau button dan mengontrol blinker
 * ============================================================
 * Task ini memiliki dua mode:
 * 1. Polling mode: baca button via GPIO, suspend/resume manual
 * 2. ISR mode: button menggunakan interrupt
 *
 * Mendemonstrasikan:
 * - vTaskSuspend(handle) - suspend task tertentu
 * - vTaskResume(handle) - resume task tertentu
 * - eTaskGetState(handle) - cek state task
 * ============================================================ */
static void monitor_task(void *pvParameters)
{
    uint32_t report_count = 0;
    int last_button = 1;  // Button pull-up, default HIGH
    int64_t last_press_time = 0;

    ESP_LOGI(TAG, "[MONITOR] Task dimulai - Button pada GPIO%d", BUTTON_GPIO);

    while (1) {
        report_count++;

        /* Baca button (active LOW pada ESP32 BOOT button) */
        int button = gpio_get_level(BUTTON_GPIO);

        /* Deteksi falling edge (button pressed) dengan debounce */
        if (!use_isr_mode && button == 0 && last_button == 1) {
            int64_t now = esp_timer_get_time();
            if ((now - last_press_time) > (DEBOUNCE_MS * 1000)) {
                last_press_time = now;

                if (!blinker_suspended) {
                    /*
                     * vTaskSuspend() - Suspend task
                     *
                     * Parameter: TaskHandle dari task yang akan di-suspend
                     * - Task masuk state SUSPENDED
                     * - Task TIDAK akan di-schedule lagi sampai di-resume
                     * - Jika task sedang blocked (delay), tetap menjadi SUSPENDED
                     * - Passing NULL akan suspend calling task sendiri
                     *
                     * PERINGATAN: vTaskSuspend() bukan mekanisme sinkronisasi!
                     * Jangan gunakan untuk mutual exclusion. Gunakan mutex/semaphore.
                     */
                    vTaskSuspend(xBlinkerHandle);
                    blinker_suspended = true;
                    suspend_count++;

                    gpio_set_level(LED2_GPIO, 1);  // Status LED ON = suspended
                    gpio_set_level(LED1_GPIO, 0);  // Matikan blinker LED

                    ESP_LOGW(TAG, "[MONITOR] Blinker SUSPENDED! (count=%lu)",
                             (unsigned long)suspend_count);
                    printf("[DATA] STATE_CHANGE,SUSPEND,%lu,%lu\n",
                           (unsigned long)get_elapsed_ms(),
                           (unsigned long)suspend_count);

                } else {
                    /*
                     * vTaskResume() - Resume task yang di-suspend
                     *
                     * Parameter: TaskHandle dari task yang akan di-resume
                     * - Task kembali ke state READY
                     * - Jika prioritas task lebih tinggi dari current,
                     *   context switch akan terjadi segera
                     * - Memanggil resume pada task yang tidak suspended
                     *   tidak memiliki efek (aman)
                     */
                    vTaskResume(xBlinkerHandle);
                    blinker_suspended = false;
                    resume_count++;

                    gpio_set_level(LED2_GPIO, 0);  // Status LED OFF = running

                    ESP_LOGI(TAG, "[MONITOR] Blinker RESUMED! (count=%lu)",
                             (unsigned long)resume_count);
                    printf("[DATA] STATE_CHANGE,RESUME,%lu,%lu\n",
                           (unsigned long)get_elapsed_ms(),
                           (unsigned long)resume_count);
                }
            }
        }
        last_button = button;

        /* Periodik report */
        if (report_count % 20 == 0) {
            eTaskState blinker_state = eTaskGetState(xBlinkerHandle);

            printf("[DATA] STATUS,%lu,%s,%lu,%lu,%lu,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   task_state_str(blinker_state),
                   (unsigned long)blinker_count,
                   (unsigned long)suspend_count,
                   (unsigned long)resume_count,
                   (unsigned long)isr_resume_count);

            ESP_LOGI(TAG, "Blinker: %s, blinks=%lu, suspend=%lu, resume=%lu, isr=%lu",
                     task_state_str(blinker_state),
                     (unsigned long)blinker_count,
                     (unsigned long)suspend_count,
                     (unsigned long)resume_count,
                     (unsigned long)isr_resume_count);
        }

        vTaskDelay(pdMS_TO_TICKS(MONITOR_PERIOD_MS));
    }
}

/* ============================================================
 * TASK: Scheduler Suspend Demo
 * ============================================================
 * Mendemonstrasikan vTaskSuspendAll() dan xTaskResumeAll().
 *
 * vTaskSuspendAll() men-suspend SELURUH scheduler:
 * - SEMUA task berhenti di-schedule
 * - Interrupt tetap aktif
 * - Tick count tidak bertambah
 * - Calling task tetap berjalan (karena tidak ada context switch)
 *
 * Ini berguna untuk critical section yang tidak boleh interrupted
 * oleh task lain, tapi masih perlu handle hardware interrupts.
 *
 * xTaskResumeAll() mengembalikan scheduler:
 * - Return pdTRUE jika ada pending context switch
 * - Semua task kembali normal
 *
 * PERINGATAN: Jangan panggil API FreeRTOS yang blocking
 * (vTaskDelay, xQueueSend, dll) saat scheduler suspended!
 * ============================================================ */
static void scheduler_demo_task(void *pvParameters)
{
    uint32_t demo_round = 0;

    ESP_LOGI(TAG, "[SCHED_DEMO] Task dimulai");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Tunggu 5 detik sebelum demo

    while (1) {
        demo_round++;

        ESP_LOGW(TAG, "[SCHED_DEMO] === Round %lu: Scheduler Suspend Demo ===",
                 (unsigned long)demo_round);

        printf("[DATA] SCHED_DEMO,START,%lu,%lu\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)demo_round);

        /* Catat timestamp sebelum suspend */
        int64_t before_suspend = esp_timer_get_time();

        /*
         * vTaskSuspendAll() - Suspend seluruh scheduler
         *
         * Setelah ini, TIDAK ADA task switch yang terjadi.
         * Calling task terus berjalan tanpa preemption.
         * Bisa di-nest (panggil berkali-kali), harus
         * xTaskResumeAll() sebanyak panggilan suspend.
         */
        vTaskSuspendAll();
        scheduler_suspend_count++;

        ESP_LOGW(TAG, "[SCHED_DEMO] Scheduler SUSPENDED! Melakukan critical work...");

        /*
         * Critical section - tidak ada task switch
         * Gunakan ini untuk operasi yang harus atomik
         * terhadap task lain (tapi bukan terhadap ISR)
         */
        volatile uint32_t critical_work = 0;
        for (uint32_t i = 0; i < 100000; i++) {
            critical_work += i;
        }

        /* Simulated non-blocking delay using busy wait
         * (karena vTaskDelay TIDAK boleh dipanggil saat scheduler suspended) */
        int64_t wait_until = esp_timer_get_time() + 500000; // 500ms
        while (esp_timer_get_time() < wait_until) {
            // Busy wait - hanya untuk demo!
        }

        /*
         * xTaskResumeAll() - Resume scheduler
         *
         * Return: pdTRUE jika ada context switch yang pending
         * (task prioritas lebih tinggi ready selama suspend)
         */
        BaseType_t xNeedSwitch = xTaskResumeAll();

        int64_t after_resume = esp_timer_get_time();
        int64_t suspend_duration = after_resume - before_suspend;

        ESP_LOGI(TAG, "[SCHED_DEMO] Scheduler RESUMED! Duration=%lld us, NeedSwitch=%d",
                 suspend_duration, xNeedSwitch);

        printf("[DATA] SCHED_DEMO,END,%lu,%lu,%lld,%d\n",
               (unsigned long)get_elapsed_ms(),
               (unsigned long)demo_round,
               suspend_duration,
               xNeedSwitch);

        /* Tunggu sebelum demo berikutnya */
        vTaskDelay(pdMS_TO_TICKS(SCHED_SUSPEND_MS * 5));
    }
}

/* ============================================================
 * TASK: ISR Resume Demo
 * ============================================================
 * Mendemonstrasikan xTaskResumeFromISR().
 * Task ini suspend dirinya sendiri dan menunggu
 * button ISR untuk men-resume.
 * ============================================================ */
static void isr_demo_task(void *pvParameters)
{
    ESP_LOGI(TAG, "[ISR_DEMO] Task dimulai, menunggu aktivasi ISR mode...");

    /* Tunggu 15 detik, lalu aktifkan ISR mode */
    vTaskDelay(pdMS_TO_TICKS(15000));

    ESP_LOGW(TAG, "[ISR_DEMO] Mengaktifkan ISR mode!");
    ESP_LOGW(TAG, "[ISR_DEMO] Tekan BOOT button untuk resume blinker via ISR");

    setup_button_isr();
    use_isr_mode = true;

    printf("[DATA] ISR_MODE,ENABLED,%lu\n", (unsigned long)get_elapsed_ms());

    uint32_t isr_demo_round = 0;

    while (1) {
        isr_demo_round++;

        /* Suspend blinker dan tunggu ISR */
        if (!blinker_suspended) {
            vTaskSuspend(xBlinkerHandle);
            blinker_suspended = true;
            gpio_set_level(LED2_GPIO, 1);

            ESP_LOGW(TAG, "[ISR_DEMO] Blinker suspended. Tekan button untuk resume via ISR!");
            printf("[DATA] ISR_SUSPEND,%lu,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)isr_demo_round);
        }

        /* Tunggu beberapa detik, lalu check apakah ISR sudah resume */
        vTaskDelay(pdMS_TO_TICKS(3000));

        if (!blinker_suspended) {
            /* ISR sudah resume blinker */
            ESP_LOGI(TAG, "[ISR_DEMO] Blinker di-resume oleh ISR! Total ISR resume=%lu",
                     (unsigned long)isr_resume_count);
            printf("[DATA] ISR_RESUMED,%lu,%lu\n",
                   (unsigned long)get_elapsed_ms(),
                   (unsigned long)isr_resume_count);

            gpio_set_level(LED2_GPIO, 0);

            /* Biarkan blinker berjalan beberapa detik */
            vTaskDelay(pdMS_TO_TICKS(5000));
        } else {
            /* Blinker masih suspended, cek state */
            eTaskState state = eTaskGetState(xBlinkerHandle);
            if (state == eSuspended) {
                blinker_suspended = true;
            } else {
                blinker_suspended = false;
                gpio_set_level(LED2_GPIO, 0);
            }
        }
    }
}

/* ============================================================
 * APP_MAIN
 * ============================================================ */
void app_main(void)
{
    start_time_us = esp_timer_get_time();

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "  ESP32 Task Suspend/Resume Demo");
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Phase 1 (0-15s): Polling mode - button toggle");
    ESP_LOGI(TAG, "Phase 2 (15s+): ISR mode - button ISR resume");
    ESP_LOGI(TAG, "Button: GPIO%d (BOOT)", BUTTON_GPIO);

    printf("[DATA] INIT,%lu,%d,%d\n",
           (unsigned long)get_elapsed_ms(),
           LED1_GPIO, BUTTON_GPIO);

    init_gpio();

    /* Buat semua task */
    xTaskCreate(blinker_task, "Blinker", TASK_STACK_SIZE,
                NULL, BLINKER_PRIORITY, &xBlinkerHandle);

    xTaskCreate(monitor_task, "Monitor", TASK_STACK_SIZE,
                NULL, MONITOR_PRIORITY, &xMonitorHandle);

    xTaskCreate(scheduler_demo_task, "SchedDemo", TASK_STACK_SIZE,
                NULL, SCHEDULER_PRIORITY, &xSchedDemoHandle);

    xTaskCreate(isr_demo_task, "ISRDemo", TASK_STACK_SIZE,
                NULL, ISR_DEMO_PRIORITY, &xISRDemoHandle);

    ESP_LOGI(TAG, "Semua task dibuat.");
    ESP_LOGI(TAG, "Tekan BOOT button untuk suspend/resume blinker");
    ESP_LOGI(TAG, "============================================");
}
