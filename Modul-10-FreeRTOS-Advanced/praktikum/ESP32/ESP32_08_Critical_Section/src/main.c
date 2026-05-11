// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Critical Section
// Demonstrasi penggunaan Critical Section dengan taskENTER_CRITICAL

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file untuk operasi portmux/critical section
#include "freertos/portmacro.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Critical_Section";

// Mendeklarasikan variabel shared resource (global)
volatile uint32_t sharedCounter = 0;
// Mendeklarasikan struktur spinlock/portmux untuk critical section
portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

// Mendeklarasikan prototipe fungsi untuk task-task
void vTaskAccessResource(void *pvParameters);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Critical Section telah dimulai
    printf("=== ESP32 FreeRTOS Critical Section ===\n");
    // Mencetak penjelasan tentang demonstrasi critical section
    printf("Demonstrasi Critical Section dengan portmux\n");

    // Mencetak pesan bahwa shared counter diinisialisasi
    printf("Shared Counter: %lu\n", sharedCounter);
    // Mencetak informasi tentang task yang akan dibuat
    printf("Membuat %d task yang mengakses shared resource...\n", ACCESS_ITERATIONS);

    // Membuat task 1 dengan priority tinggi
    xTaskCreate(vTaskAccessResource, "Task1", TASK_STACK_SIZE, (void *)1, TASK_PRIORITY_ACCESS, NULL);
    // Membuat task 2 dengan priority tinggi
    xTaskCreate(vTaskAccessResource, "Task2", TASK_STACK_SIZE, (void *)2, TASK_PRIORITY_ACCESS, NULL);
    // Membuat task 3 dengan priority tinggi
    xTaskCreate(vTaskAccessResource, "Task3", TASK_STACK_SIZE, (void *)3, TASK_PRIORITY_ACCESS, NULL);

    // Mencetak pesan bahwa semua task telah dibuat
    printf("Semua task telah dibuat, menunggu akses critical section...\n");
}

// Implementasi Task - mengakses shared resource dalam critical section
void vTaskAccessResource(void *pvParameters)
{
    // Mendeklarasikan variabel untuk menyimpan ID task
    int taskID = (int)pvParameters;
    // Mendeklarasikan variabel iterator
    int i;
    // Loop untuk beberapa iterasi akses
    for (i = 0; i < ACCESS_ITERATIONS; i++)
    {
        // Mencetak pesan bahwa task akan masuk critical section
        printf("[Task%d] Masuk Critical Section...\n", taskID);
        // Masuk ke critical section dengan taskENTER_CRITICAL
        taskENTER_CRITICAL(&myMutex);
        // Mencetak pesan bahwa task dalam critical section
        printf("[Task%d] DALAM CRITICAL SECTION (iterasi %d)\n", taskID, i + 1);
        // Mengakses shared resource (increment counter)
        sharedCounter++;
        // Mencetak nilai counter saat ini
        printf("[Task%d] Shared Counter: %lu\n", taskID, sharedCounter);
        // Simulasi delay dalam critical section (tidak boleh lama!)
        // esp_rom_delay_us(CRITICAL_DELAY_US); // Dihapus - gunakan simulasi lain
        // Mengakses lagi untuk demonstrasi
        sharedCounter++;
        // Mencetak nilai counter lagi
        printf("[Task%d] Shared Counter setelah increment: %lu\n", taskID, sharedCounter);
        // Keluar dari critical section dengan taskEXIT_CRITICAL
        taskEXIT_CRITICAL(&myMutex);
        // Mencetak pesan bahwa task keluar critical section
        printf("[Task%d] Keluar Critical Section\n", taskID);
        // Menunggu sebelum akses berikutnya
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    // Mencetak pesan bahwa task selesai semua iterasi
    printf("[Task%d] Selesai semua iterasi akses\n", taskID);
    // Menghapus task sendiri setelah selesai
    vTaskDelete(NULL);
}
