// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Counting Semaphore
// Demonstrasi penggunaan Counting Semaphore untuk Resource Pool Management

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk semaphore operations
#include "freertos/semphr.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Counting_Semaphore";

// Mendeklarasikan handle untuk counting semaphore
SemaphoreHandle_t xCountingSemaphore;

// Mendeklarasikan prototipe fungsi untuk task-task
void vResourceUserTask(void *pvParameters);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Counting Semaphore telah dimulai
    printf("=== ESP32 FreeRTOS Counting Semaphore ===\n");
    // Mencetak penjelasan tentang demonstrasi counting semaphore
    printf("Demonstrasi Resource Pool Management dengan Counting Semaphore\n");

    // Membuat counting semaphore dengan xSemaphoreCreateCounting
    xCountingSemaphore = xSemaphoreCreateCounting(
        MAX_RESOURCES,       // Nilai maksimum semaphore (jumlah resource)
        INITIAL_RESOURCES    // Nilai awal semaphore (resource tersedia)
    );

    // Mengecek apakah counting semaphore berhasil dibuat
    if (xCountingSemaphore == NULL)
    {
        // Mencetak pesan error jika semaphore gagal dibuat
        printf("Gagal membuat Counting Semaphore!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa semaphore berhasil dibuat
    printf("Counting Semaphore berhasil dibuat\n");
    // Mencetak jumlah maksimum resource
    printf("Jumlah resource maksimum: %d\n", MAX_RESOURCES);
    // Mencetak jumlah resource awal yang tersedia
    printf("Resource awal tersedia: %d\n", INITIAL_RESOURCES);

    // Membuat task user 1 dengan nama "User1"
    xTaskCreate(vResourceUserTask, "User1", TASK_STACK_SIZE, (void *)1, TASK_PRIORITY, NULL);
    // Membuat task user 2 dengan nama "User2"
    xTaskCreate(vResourceUserTask, "User2", TASK_STACK_SIZE, (void *)2, TASK_PRIORITY, NULL);
    // Membuat task user 3 dengan nama "User3"
    xTaskCreate(vResourceUserTask, "User3", TASK_STACK_SIZE, (void *)3, TASK_PRIORITY, NULL);
    // Membuat task user 4 dengan nama "User4"
    xTaskCreate(vResourceUserTask, "User4", TASK_STACK_SIZE, (void *)4, TASK_PRIORITY, NULL);
    // Membuat task user 5 dengan nama "User5"
    xTaskCreate(vResourceUserTask, "User5", TASK_STACK_SIZE, (void *)5, TASK_PRIORITY, NULL);

    // Mencetak pesan bahwa semua task user telah dibuat
    printf("5 Resource User tasks telah dibuat\n");
    // Mencetak penjelasan bahwa hanya 3 resource yang tersedia
    printf("Hanya %d resource yang tersedia (akan ada yang menunggu)\n", MAX_RESOURCES);
}

// Implementasi Resource User Task - menggunakan resource melalui counting semaphore
void vResourceUserTask(void *pvParameters)
{
    // Mendeklarasikan variabel untuk menyimpan ID task dari parameter
    int taskID = (int)pvParameters;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xResult;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa task meminta resource
        printf("User%d: Meminta resource...\n", taskID);
        // Mengambil semaphore (mengurangi count) dengan xSemaphoreTake
        xResult = xSemaphoreTake(xCountingSemaphore, portMAX_DELAY);
        // Mengecek apakah semaphore berhasil diambil
        if (xResult == pdPASS)
        {
            // Menyalakan LED sebagai indikator resource didapat
            gpio_set_level(LED_GPIO_PIN, 1);
            // Mencetak pesan bahwa resource berhasil didapat
            printf("User%d: Resource DIPEROLEH! Menggunakannya...\n", taskID);
            // Menggunakan resource selama waktu yang ditentukan
            vTaskDelay(pdMS_TO_TICKS(RESOURCE_USE_TIME_MS));
            // Mencetak pesan bahwa task selesai menggunakan resource
            printf("User%d: Selesai menggunakan resource, mengembalikan...\n", taskID);
            // Mengembalikan semaphore (menambah count) dengan xSemaphoreGive
            xSemaphoreGive(xCountingSemaphore);
            // Mematikan LED
            gpio_set_level(LED_GPIO_PIN, 0);
            // Mencetak pesan bahwa resource telah dikembalikan
            printf("User%d: Resource dikembalikan\n", taskID);
        }
        // Jika gagal mengambil semaphore (seharusnya tidak terjadi karena portMAX_DELAY)
        else
        {
            // Mencetak pesan error
            printf("User%d: Gagal mendapatkan resource!\n", taskID);
        }
        // Menunggu sebelum mencoba lagi
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
