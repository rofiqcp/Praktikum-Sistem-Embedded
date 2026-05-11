// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Software Timer Basic
// Demonstrasi penggunaan Software Timers (One-shot, Auto-reload, Timer ID)

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk software timer operations
#include "freertos/timers.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging ESP-IDF
static const char *TAG = "Software_Timer_Basic";

// Mendeklarasikan handle untuk software timer one-shot
TimerHandle_t xOneShotTimer;
// Mendeklarasikan handle untuk software timer auto-reload
TimerHandle_t xAutoReloadTimer;

// Mendeklarasikan prototipe fungsi callback untuk timer
void vOneShotTimerCallback(TimerHandle_t xTimer);
void vAutoReloadTimerCallback(TimerHandle_t xTimer);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Software Timer Basic telah dimulai
    printf("=== ESP32 FreeRTOS Software Timer Basic ===\n");
    // Mencetak penjelasan tentang jenis timer yang akan didemo
    printf("Demonstrasi One-shot dan Auto-reload Timer\n");

    // Membuat software timer one-shot dengan xTimerCreate
    xOneShotTimer = xTimerCreate(
        "OneShotTimer",                  // Nama timer untuk debugging
        pdMS_TO_TICKS(ONESHOT_TIMER_DELAY_MS), // Periode timer dalam ticks (3 detik)
        pdFALSE,                         // pdFALSE = one-shot (tidak auto-reload)
        (void *)TIMER_ID_ONE_SHOT,       // ID timer yang akan diteruskan ke callback
        vOneShotTimerCallback            // Fungsi callback yang dipanggil saat timer expire
    );

    // Membuat software timer auto-reload dengan xTimerCreate
    xAutoReloadTimer = xTimerCreate(
        "AutoReloadTimer",               // Nama timer untuk debugging
        pdMS_TO_TICKS(TIMER_PERIOD_MS),  // Periode timer dalam ticks (1 detik)
        pdTRUE,                          // pdTRUE = auto-reload (berulang)
        (void *)TIMER_ID_AUTO_RELOAD,    // ID timer yang akan diteruskan ke callback
        vAutoReloadTimerCallback         // Fungsi callback yang dipanggil saat timer expire
    );

    // Mengecek apakah one-shot timer berhasil dibuat
    if (xOneShotTimer == NULL)
    {
        // Mencetak pesan error jika timer gagal dibuat
        printf("Gagal membuat One-shot Timer!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mengecek apakah auto-reload timer berhasil dibuat
    if (xAutoReloadTimer == NULL)
    {
        // Mencetak pesan error jika timer gagal dibuat
        printf("Gagal membuat Auto-reload Timer!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa kedua timer berhasil dibuat
    printf("Timer berhasil dibuat\n");
    // Mencetak informasi tentang one-shot timer
    printf("One-shot Timer: %d detik\n", ONESHOT_TIMER_DELAY_MS / 1000);
    // Mencetak informasi tentang auto-reload timer
    printf("Auto-reload Timer: %d detik periodik\n", TIMER_PERIOD_MS / 1000);

    // Memulai one-shot timer dengan xTimerStart
    xTimerStart(xOneShotTimer, 0);
    // Mencetak pesan bahwa one-shot timer telah dimulai
    printf("One-shot timer dimulai...\n");

    // Memulai auto-reload timer dengan xTimerStart
    xTimerStart(xAutoReloadTimer, 0);
    // Mencetak pesan bahwa auto-reload timer telah dimulai
    printf("Auto-reload timer dimulai...\n");

    // Loop utama untuk menjaga program tetap berjalan
    while (1)
    {
        // Menyalakan LED sebagai indikator program berjalan
        gpio_set_level(LED_GPIO_PIN, 1);
        // Menunggu selama 500ms
        vTaskDelay(pdMS_TO_TICKS(500));
        // Mematikan LED
        gpio_set_level(LED_GPIO_PIN, 0);
        // Menunggu selama 500ms
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Implementasi callback untuk one-shot timer
void vOneShotTimerCallback(TimerHandle_t xTimer)
{
    // Mendeklarasikan variabel untuk menyimpan ID timer
    uint32_t ulTimerID;
    // Mengambil ID timer dari parameter xTimer
    ulTimerID = (uint32_t)pvTimerGetTimerID(xTimer);
    // Mencetak pesan bahwa one-shot timer telah expire
    printf(">>> One-shot Timer EXPIRED! (ID: %lu)\n", ulTimerID);
    // Mencetak pesan bahwa timer hanya berjalan sekali
    printf("    Timer one-shot selesai, tidak akan berjalan lagi\n");
}

// Implementasi callback untuk auto-reload timer
void vAutoReloadTimerCallback(TimerHandle_t xTimer)
{
    // Mendeklarasikan variabel static untuk menghitung jumlah callback
    static uint32_t ulCallCount = 0;
    // Mendeklarasikan variabel untuk menyimpan ID timer
    uint32_t ulTimerID;
    // Mengambil ID timer dari parameter xTimer
    ulTimerID = (uint32_t)pvTimerGetTimerID(xTimer);
    // Increment counter setiap kali callback dipanggil
    ulCallCount++;
    // Mencetak pesan bahwa auto-reload timer callback dipanggil
    printf(">>> Auto-reload Timer callback #%lu (ID: %lu)\n", ulCallCount, ulTimerID);
    // Mengecek apakah sudah mencapai 10 kali callback
    if (ulCallCount >= 10)
    {
        // Mencetak pesan bahwa timer akan dihentikan
        printf("    10 callback selesai, menghentikan auto-reload timer...\n");
        // Menghentikan timer dengan xTimerStop
        xTimerStop(xAutoReloadTimer, 0);
        // Mencetak pesan bahwa timer telah dihentikan
        printf("    Auto-reload timer dihentikan\n");
    }
}
