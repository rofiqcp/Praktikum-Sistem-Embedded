/**
 * =============================================================================
 * ESP32_13_PWM_LEDC_Fade
 * Modul 05 - DAC/PWM | Bonus: Fitur Khusus ESP32
 * =============================================================================
 *
 * FITUR KHUSUS ESP32 YANG TIDAK ADA DI STM32:
 * --------------------------------------------
 * 1. LEDC HARDWARE FADE:
 *    - ESP32 memiliki peripheral LEDC (LED Control) dengan fitur fade
 *      bawaan di hardware. LED bisa fade dari satu brightness ke brightness
 *      lain secara OTOMATIS tanpa intervensi CPU.
 *    - ledc_set_fade_with_time() → set target duty & durasi, hardware
 *      mengerjakan sisanya
 *    - ledc_fade_start() dengan LEDC_FADE_NO_WAIT → CPU bebas melakukan
 *      hal lain saat fade berjalan
 *    - STM32 TIDAK memiliki fitur ini. Untuk fade LED di STM32:
 *      * Harus update duty cycle secara manual di loop/timer ISR
 *      * Atau menggunakan DMA untuk update CCR register
 *      * Membutuhkan CPU time atau DMA channel
 *
 * 2. GPIO MATRIX FLEXIBILITY:
 *    - ESP32 bisa assign output LEDC ke GPIO PIN MANA SAJA melalui
 *      GPIO matrix. Tidak ada batasan "alternate function" seperti STM32.
 *    - STM32 PWM output HARUS pada pin tertentu (misal TIM1_CH1 hanya
 *      bisa di PA8 atau PE9, tidak bisa di sembarang pin)
 *
 * 3. LEDC TIMER INDEPENDENCE:
 *    - ESP32 memiliki 8 channel LEDC (4 high-speed + 4 low-speed)
 *    - Setiap channel bisa di-bind ke timer berbeda
 *    - Memungkinkan frekuensi berbeda untuk setiap channel
 *
 * MENGAPA STM32 TIDAK BISA:
 * - STM32 Timer tidak memiliki hardware fade — harus software
 * - STM32 PWM output fixed ke pin tertentu via AF mapping
 * - STM32 perlu DMA + timer untuk achieve smooth fade tanpa CPU
 *
 * Hardware: 3 LED pada pin GPIO yang berbeda
 * =============================================================================
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LEDC_FADE";

/* ==================== Konfigurasi Pin per Board ==================== */
/*
 * Keunggulan ESP32: Bisa assign LEDC ke pin MANA SAJA!
 * STM32 terbatas pada pin alternate function tertentu.
 */

#if CONFIG_IDF_TARGET_ESP32
    /* ESP32 Classic - pilih 3 pin GPIO yang mudah diakses */
    #define LED_PIN_1       2       // Built-in LED pada banyak board
    #define LED_PIN_2       4       // GPIO4
    #define LED_PIN_3       5       // GPIO5
    #define BOARD_NAME      "ESP32 Classic"

#elif CONFIG_IDF_TARGET_ESP32S2
    /* ESP32-S2 - Lolin S2 Mini */
    #define LED_PIN_1       15      // Built-in LED Lolin S2 Mini
    #define LED_PIN_2       7       // GPIO7
    #define LED_PIN_3       9       // GPIO9
    #define BOARD_NAME      "ESP32-S2"

#elif CONFIG_IDF_TARGET_ESP32S3
    /* ESP32-S3 */
    #define LED_PIN_1       2       // GPIO2
    #define LED_PIN_2       4       // GPIO4
    #define LED_PIN_3       5       // GPIO5
    #define BOARD_NAME      "ESP32-S3"

#else
    #error "Board tidak didukung! Gunakan ESP32, ESP32-S2, atau ESP32-S3"
#endif

/* ==================== Konstanta LEDC ==================== */
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT       // Resolusi 13-bit (0-8191)
#define LEDC_FREQUENCY      5000                     // 5 kHz
#define LEDC_MAX_DUTY       ((1 << 13) - 1)          // 8191

/* Channel assignment */
#define LEDC_CHANNEL_1      LEDC_CHANNEL_0
#define LEDC_CHANNEL_2      LEDC_CHANNEL_1
#define LEDC_CHANNEL_3      LEDC_CHANNEL_2

/* ==================== Inisialisasi LEDC ==================== */

/**
 * Inisialisasi LEDC timer dan 3 channel
 * Perhatikan: ESP32 bisa assign channel ke pin MANA SAJA (GPIO Matrix)
 */
static void ledc_init(void)
{
    ESP_LOGI(TAG, "Inisialisasi LEDC...");
    ESP_LOGI(TAG, "GPIO Matrix: LED1=GPIO%d, LED2=GPIO%d, LED3=GPIO%d",
             LED_PIN_1, LED_PIN_2, LED_PIN_3);
    ESP_LOGI(TAG, "(Di STM32, PWM pin tidak bisa dipilih sebebas ini!)");

    /* Konfigurasi timer LEDC */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* Konfigurasi 3 channel — masing-masing di pin berbeda */
    ledc_channel_config_t channels[] = {
        {
            .speed_mode = LEDC_MODE,
            .channel    = LEDC_CHANNEL_1,
            .timer_sel  = LEDC_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = LED_PIN_1,
            .duty       = 0,
            .hpoint     = 0,
        },
        {
            .speed_mode = LEDC_MODE,
            .channel    = LEDC_CHANNEL_2,
            .timer_sel  = LEDC_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = LED_PIN_2,
            .duty       = 0,
            .hpoint     = 0,
        },
        {
            .speed_mode = LEDC_MODE,
            .channel    = LEDC_CHANNEL_3,
            .timer_sel  = LEDC_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = LED_PIN_3,
            .duty       = 0,
            .hpoint     = 0,
        },
    };

    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(ledc_channel_config(&channels[i]));
    }

    /* Install fade function — ini yang memungkinkan hardware fade */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
    ESP_LOGI(TAG, "✓ LEDC fade ISR service terinstall");
    ESP_LOGI(TAG, "  (Fitur hardware fade ini TIDAK ada di STM32!)");
}

/* ==================== Demo 1: Linear Fade ==================== */

/**
 * Demonstrasi hardware fade linear
 * LED fade dari OFF ke ON dan kembali, sepenuhnya oleh hardware!
 */
static void demo_linear_fade(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 1: Hardware Linear Fade (Fitur Khusus ESP32)         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "LEDC Hardware Fade — CPU TIDAK terlibat selama fade!");
    ESP_LOGI(TAG, "Di STM32, Anda harus update duty secara manual di timer ISR.");
    ESP_LOGI(TAG, "");

    for (int cycle = 0; cycle < 3; cycle++) {
        ESP_LOGI(TAG, "--- Siklus %d/3 ---", cycle + 1);

        /* Fade ON: 0 → MAX dalam 2 detik */
        ESP_LOGI(TAG, "Fade ON (0 → 100%%) dalam 2 detik...");

        /* Catat waktu mulai untuk buktikan CPU bebas */
        int64_t start = esp_timer_get_time();

        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL_1, LEDC_MAX_DUTY, 2000);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE);

        int64_t elapsed = esp_timer_get_time() - start;
        ESP_LOGI(TAG, "✓ Fade ON selesai (durasi aktual: %lld ms)", elapsed / 1000);

        vTaskDelay(pdMS_TO_TICKS(500));

        /* Fade OFF: MAX → 0 dalam 2 detik */
        ESP_LOGI(TAG, "Fade OFF (100%% → 0) dalam 2 detik...");

        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL_1, 0, 2000);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE);

        ESP_LOGI(TAG, "✓ Fade OFF selesai");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ==================== Demo 2: Breathing Effect ==================== */

/**
 * Efek "bernapas" menggunakan hardware fade
 * Fade up dan down secara terus-menerus — mirip LED MacBook saat sleep
 */
static void demo_breathing_effect(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 2: Hardware Breathing Effect                         ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Efek 'bernapas' — fade up/down otomatis oleh hardware LEDC");
    ESP_LOGI(TAG, "");

    for (int breath = 0; breath < 5; breath++) {
        /* Inhale — fade dari 0 ke max */
        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL_1, LEDC_MAX_DUTY, 1500);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE);

        /* Jeda sebentar di puncak */
        vTaskDelay(pdMS_TO_TICKS(100));

        /* Exhale — fade dari max ke 0 */
        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL_1, 0, 1500);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL_1, LEDC_FADE_WAIT_DONE);

        /* Jeda sebentar di dasar */
        vTaskDelay(pdMS_TO_TICKS(200));

        ESP_LOGI(TAG, "Napas %d/5 selesai", breath + 1);
    }
}

/* ==================== Demo 3: Cascading Multi-Channel Fade ==================== */

/**
 * Fade bertingkat pada 3 channel — efek "wave" atau "chasing"
 * Setiap LED mulai fade dengan delay, menciptakan efek bergelombang
 *
 * Ini menunjukkan kekuatan LEDC_FADE_NO_WAIT:
 * - Start fade di channel 1, JANGAN tunggu
 * - Delay sedikit, start fade di channel 2
 * - Delay sedikit, start fade di channel 3
 * - Semua 3 fade berjalan PARALEL di hardware!
 */
static void demo_cascading_fade(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 3: Cascading Multi-Channel Hardware Fade             ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "3 LED fade bertingkat — semua berjalan di HARDWARE secara paralel!");
    ESP_LOGI(TAG, "Menggunakan LEDC_FADE_NO_WAIT agar CPU langsung lanjut.");
    ESP_LOGI(TAG, "LED1=GPIO%d, LED2=GPIO%d, LED3=GPIO%d",
             LED_PIN_1, LED_PIN_2, LED_PIN_3);
    ESP_LOGI(TAG, "");

    ledc_channel_t channels[] = { LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3 };
    const char *names[] = { "LED1", "LED2", "LED3" };

    for (int wave = 0; wave < 4; wave++) {
        ESP_LOGI(TAG, "--- Wave %d/4: Fade ON bertingkat ---", wave + 1);

        /* Fade ON bertingkat — setiap LED mulai 300ms setelah sebelumnya */
        for (int ch = 0; ch < 3; ch++) {
            ledc_set_fade_with_time(LEDC_MODE, channels[ch], LEDC_MAX_DUTY, 1000);
            ledc_fade_start(LEDC_MODE, channels[ch], LEDC_FADE_NO_WAIT);
            ESP_LOGI(TAG, "  %s: Fade ON dimulai (NO_WAIT — CPU bebas!)", names[ch]);
            vTaskDelay(pdMS_TO_TICKS(300));  // Delay antar channel
        }

        /* Tunggu semua selesai */
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "--- Wave %d/4: Fade OFF bertingkat ---", wave + 1);

        /* Fade OFF bertingkat */
        for (int ch = 0; ch < 3; ch++) {
            ledc_set_fade_with_time(LEDC_MODE, channels[ch], 0, 1000);
            ledc_fade_start(LEDC_MODE, channels[ch], LEDC_FADE_NO_WAIT);
            ESP_LOGI(TAG, "  %s: Fade OFF dimulai", names[ch]);
            vTaskDelay(pdMS_TO_TICKS(300));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ==================== Demo 4: Software vs Hardware Fade ==================== */

/**
 * Perbandingan CPU usage: Software fade vs Hardware fade
 *
 * Software fade (cara STM32):
 * - CPU harus update duty setiap step
 * - Memakan CPU time secara terus-menerus
 *
 * Hardware fade (keunggulan ESP32):
 * - CPU hanya set target, hardware yang mengerjakan
 * - CPU 100% bebas selama fade berjalan
 */
static void demo_software_vs_hardware(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 4: Software Fade vs Hardware Fade (Perbandingan)     ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");

    /* --- Software Fade (cara STM32 / manual) --- */
    ESP_LOGI(TAG, "=== SOFTWARE FADE (seperti di STM32) ===");
    ESP_LOGI(TAG, "CPU harus update duty setiap step...");

    int64_t sw_start = esp_timer_get_time();
    int sw_iterations = 0;

    /* Fade up secara manual — 256 step dalam 2 detik */
    for (int duty = 0; duty <= LEDC_MAX_DUTY; duty += (LEDC_MAX_DUTY / 256)) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2);
        sw_iterations++;
        vTaskDelay(pdMS_TO_TICKS(8));  // ~2 detik total
    }

    int64_t sw_elapsed = esp_timer_get_time() - sw_start;
    ESP_LOGI(TAG, "Software fade selesai:");
    ESP_LOGI(TAG, "  Waktu: %lld ms", sw_elapsed / 1000);
    ESP_LOGI(TAG, "  Iterasi CPU: %d kali (CPU sibuk!)", sw_iterations);
    ESP_LOGI(TAG, "  CPU harus bangun %d kali untuk update duty", sw_iterations);

    /* Reset ke 0 */
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* --- Hardware Fade (keunggulan ESP32) --- */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== HARDWARE FADE (fitur ESP32) ===");
    ESP_LOGI(TAG, "CPU hanya set target, hardware yang mengerjakan...");

    int64_t hw_start = esp_timer_get_time();

    /* Satu panggilan saja! Hardware mengurus fade secara otomatis */
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL_2, LEDC_MAX_DUTY, 2000);
    ledc_fade_start(LEDC_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT);

    /* CPU BEBAS di sini! Bisa mengerjakan hal lain */
    int free_counter = 0;
    while (ledc_get_duty(LEDC_MODE, LEDC_CHANNEL_2) < LEDC_MAX_DUTY - 100) {
        free_counter++;
        /* Simulasi pekerjaan lain yang bisa dilakukan CPU */
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    int64_t hw_elapsed = esp_timer_get_time() - hw_start;
    ESP_LOGI(TAG, "Hardware fade selesai:");
    ESP_LOGI(TAG, "  Waktu: %lld ms", hw_elapsed / 1000);
    ESP_LOGI(TAG, "  Pemanggilan API: 2 kali saja (set + start)");
    ESP_LOGI(TAG, "  CPU bebas melakukan %d iterasi pekerjaan lain!", free_counter);

    /* Reset */
    ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL_2, 0, 500);
    ledc_fade_start(LEDC_MODE, LEDC_CHANNEL_2, LEDC_FADE_WAIT_DONE);

    /* Rangkuman perbandingan */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  PERBANDINGAN                                              ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║  Software Fade (STM32 style):                              ║");
    ESP_LOGI(TAG, "║    - CPU update duty %3d kali                    ║", sw_iterations);
    ESP_LOGI(TAG, "║    - CPU SIBUK selama fade                                 ║");
    ESP_LOGI(TAG, "║    - Resolusi tergantung kecepatan loop                    ║");
    ESP_LOGI(TAG, "║                                                            ║");
    ESP_LOGI(TAG, "║  Hardware Fade (ESP32 LEDC):                               ║");
    ESP_LOGI(TAG, "║    - CPU hanya 2 panggilan API                             ║");
    ESP_LOGI(TAG, "║    - CPU BEBAS selama fade                                 ║");
    ESP_LOGI(TAG, "║    - Resolusi ditangani hardware (sangat halus)             ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
}

/* ==================== Demo 5: GPIO Matrix Flexibility ==================== */

/**
 * Demonstrasi fleksibilitas GPIO Matrix
 * LEDC output bisa dipindah ke pin mana saja secara runtime!
 */
static void demo_gpio_matrix_flexibility(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  DEMO 5: GPIO Matrix Flexibility                           ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP32 GPIO Matrix memungkinkan LEDC output di pin MANA SAJA!");
    ESP_LOGI(TAG, "STM32 PWM output HARUS pada pin alternate function tertentu.");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Konfigurasi saat ini:");
    ESP_LOGI(TAG, "  LEDC Channel 0 → GPIO%d (bisa pin mana saja!)", LED_PIN_1);
    ESP_LOGI(TAG, "  LEDC Channel 1 → GPIO%d (bisa pin mana saja!)", LED_PIN_2);
    ESP_LOGI(TAG, "  LEDC Channel 2 → GPIO%d (bisa pin mana saja!)", LED_PIN_3);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Di STM32, contoh batasan pin PWM:");
    ESP_LOGI(TAG, "  TIM1_CH1 → HANYA PA8 atau PE9 (fixed!)");
    ESP_LOGI(TAG, "  TIM2_CH1 → HANYA PA0, PA5, atau PA15 (terbatas!)");
    ESP_LOGI(TAG, "  Tidak bisa sembarang pin seperti ESP32");
    ESP_LOGI(TAG, "");

    /* Demonstrasi: Nyalakan 3 LED berurutan untuk buktikan semua pin bekerja */
    ESP_LOGI(TAG, "Menyalakan 3 LED berurutan untuk buktikan GPIO matrix...");

    int pins[] = { LED_PIN_1, LED_PIN_2, LED_PIN_3 };
    ledc_channel_t chs[] = { LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3 };

    for (int i = 0; i < 3; i++) {
        ESP_LOGI(TAG, "  → GPIO%d: LEDC fade ON...", pins[i]);
        ledc_set_fade_with_time(LEDC_MODE, chs[i], LEDC_MAX_DUTY, 500);
        ledc_fade_start(LEDC_MODE, chs[i], LEDC_FADE_WAIT_DONE);
        vTaskDelay(pdMS_TO_TICKS(500));

        ledc_set_fade_with_time(LEDC_MODE, chs[i], 0, 500);
        ledc_fade_start(LEDC_MODE, chs[i], LEDC_FADE_WAIT_DONE);
    }

    ESP_LOGI(TAG, "✓ Semua 3 pin berbeda berhasil output LEDC PWM!");
    ESP_LOGI(TAG, "  Ini mustahil di STM32 tanpa batasan pin AF.");
}

/* ==================== Main Application ==================== */

void app_main(void)
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  ESP32_13: PWM LEDC Hardware Fade Demo                     ║");
    ESP_LOGI(TAG, "║  Fitur Khusus ESP32 — Tidak Tersedia di STM32              ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "");

    /* Inisialisasi LEDC dengan 3 channel */
    ledc_init();

    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Demo 1: Linear fade dasar */
    demo_linear_fade();
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Demo 2: Efek breathing */
    demo_breathing_effect();
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Demo 3: Cascading multi-channel fade */
    demo_cascading_fade();
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Demo 4: Perbandingan software vs hardware fade */
    demo_software_vs_hardware();
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Demo 5: GPIO Matrix flexibility */
    demo_gpio_matrix_flexibility();

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "Semua demo selesai!");
    ESP_LOGI(TAG, "Kesimpulan: ESP32 LEDC lebih unggul dari STM32 PWM karena:");
    ESP_LOGI(TAG, "1. Hardware fade — otomatis tanpa CPU");
    ESP_LOGI(TAG, "2. GPIO Matrix — output di pin mana saja");
    ESP_LOGI(TAG, "3. Fade ISR service — manajemen fade yang mudah");
    ESP_LOGI(TAG, "4. Multi-channel parallel fade tanpa overhead CPU");
    ESP_LOGI(TAG, "════════════════════════════════════════════════════════════════");

    /* Cleanup */
    ledc_fade_func_uninstall();
}
