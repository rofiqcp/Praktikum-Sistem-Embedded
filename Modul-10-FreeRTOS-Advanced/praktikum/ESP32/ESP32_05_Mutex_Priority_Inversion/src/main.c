// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Mutex Priority Inheritance
// Demonstrasi penggunaan Mutex untuk menangani Priority Inversion

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk mutex operations
#include "freertos/semphr.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Mutex_Priority_Inversion";

// Mendeklarasikan handle untuk mutex
SemaphoreHandle_t xMutex;

// Mendeklarasikan prototipe fungsi untuk task-task
void vTaskLow(void *pvParameters);
void vTaskMedium(void *pvParameters);
void vTaskHigh(void *pvParameters);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Mutex Priority Inversion telah dimulai
    printf("=== ESP32 FreeRTOS Mutex Priority Inheritance ===\n");
    // Mencetak penjelasan tentang demonstrasi priority inversion
    printf("Demonstrasi Priority Inheritance dengan Mutex\n");

    // Membuat mutex dengan xSemaphoreCreateMutex
    xMutex = xSemaphoreCreateMutex();

    // Mengecek apakah mutex berhasil dibuat
    if (xMutex == NULL)
    {
        // Mencetak pesan error jika mutex gagal dibuat
        printf("Gagal membuat Mutex!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa mutex berhasil dibuat
    printf("Mutex berhasil dibuat\n");
    // Mencetak informasi tentang priority task
    printf("Priority: Low=%d, Medium=%d, High=%d\n", TASK_PRIORITY_LOW, TASK_PRIORITY_MEDIUM, TASK_PRIORITY_HIGH);

    // Membuat task Low dengan priority rendah
    xTaskCreate(vTaskLow, "TaskLow", TASK_STACK_SIZE, NULL, TASK_PRIORITY_LOW, NULL);
    // Membuat task Medium dengan priority menengah
    xTaskCreate(vTaskMedium, "TaskMedium", TASK_STACK_SIZE, NULL, TASK_PRIORITY_MEDIUM, NULL);
    // Membuat task High dengan priority tinggi
    xTaskCreate(vTaskHigh, "TaskHigh", TASK_STACK_SIZE, NULL, TASK_PRIORITY_HIGH, NULL);

    // Mencetak pesan bahwa semua task telah dibuat
    printf("Semua task (Low, Medium, High) telah dibuat\n");
    // Mencetak penjelasan alur demonstrasi
    printf("Task Low akan mengambil mutex, lalu Task High akan menunggu\n");
}

// Implementasi Task Low - priority rendah, mengambil mutex lama
void vTaskLow(void *pvParameters)
{
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa Task Low memulai eksekusi
        printf("\n[LOW] Task Low memulai eksekusi\n");
        // Mencetak pesan bahwa Task Low mencoba mengambil mutex
        printf("[LOW] Task Low mengambil mutex...\n");
        // Mengambil mutex dengan xSemaphoreTake
        xSemaphoreTake(xMutex, portMAX_DELAY);
        // Mencetak pesan bahwa mutex berhasil diambil
        printf("[LOW] Mutex diambil! Mulai bekerja (akan lama)...\n");
        // Menyalakan LED sebagai indikator
        gpio_set_level(LED_GPIO_PIN, 1);
        // Bekerja dengan resource (simulasi eksekusi lama)
        vTaskDelay(pdMS_TO_TICKS(LOW_TASK_EXEC_TIME_MS));
        // Mencetak pesan bahwa Task Low selesai bekerja
        printf("[LOW] Selesai bekerja, mengembalikan mutex...\n");
        // Mengembalikan mutex dengan xSemaphoreGive
        xSemaphoreGive(xMutex);
        // Mematikan LED
        gpio_set_level(LED_GPIO_PIN, 0);
        // Mencetak pesan bahwa mutex dikembalikan
        printf("[LOW] Mutex dikembalikan\n");
        // Menunggu sebelum siklus berikutnya
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Implementasi Task Medium - priority menengah, tidak menggunakan mutex
void vTaskMedium(void *pvParameters)
{
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa Task Medium berjalan
        printf("[MEDIUM] Task Medium berjalan (tidak butuh mutex)\n");
        // Simulasi pekerjaan task medium
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Implementasi Task High - priority tinggi, butuh mutex
void vTaskHigh(void *pvParameters)
{
    // Menunggu sebentar sebelum task high mulai (biar low ambil mutex dulu)
    vTaskDelay(pdMS_TO_TICKS(HIGH_TASK_DELAY_MS));
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa Task High memulai eksekusi
        printf("\n[HIGH] Task High memulai eksekusi\n");
        // Mencetak pesan bahwa Task High butuh mutex
        printf("[HIGH] Task High butuh mutex...\n");
        // Mencetak pesan bahwa Task High akan menunggu (priority inheritance aktif)
        printf("[HIGH] Menunggu mutex (Priority Inheritance aktif)...\n");
        // Mengambil mutex dengan xSemaphoreTake (akan menunggu karena Low pegang)
        xSemaphoreTake(xMutex, portMAX_DELAY);
        // Mencetak pesan bahwa mutex berhasil diambil oleh High
        printf("[HIGH] Mutex diambil! Bekerja cepat...\n");
        // Bekerja dengan resource (cepat)
        vTaskDelay(pdMS_TO_TICKS(500));
        // Mencetak pesan bahwa Task High selesai
        printf("[HIGH] Selesai, mengembalikan mutex...\n");
        // Mengembalikan mutex
        xSemaphoreGive(xMutex);
        // Mencetak pesan bahwa Task High selesai siklus
        printf("[HIGH] Siklus selesai\n");
        // Menunggu sebelum mencoba lagi
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
