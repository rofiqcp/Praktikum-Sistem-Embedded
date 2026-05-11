// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Event Group Basic
// Demonstrasi penggunaan Event Group dengan multiple bits dan AND/OR waiting

// Menginclude header file ESP-IDF untuk sistem
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk Event Group operations
#include "freertos/event_groups.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging ESP-IDF
static const char *TAG = "EventGroup_Basic";

// Mendeklarasikan handle untuk Event Group yang akan digunakan
EventGroupHandle_t xEventGroup;

// Mendefinisikan prototipe fungsi untuk task-task yang akan dibuat
void vTask1(void *pvParameters);
void vTask2(void *pvParameters);
void vTask3(void *pvParameters);
void vEventMonitorTask(void *pvParameters);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Membuat Event Group baru dengan memanggil xEventGroupCreate
    xEventGroup = xEventGroupCreate();
    // Mengecek apakah Event Group berhasil dibuat
    if (xEventGroup == NULL)
    {
        // Mencetak pesan error jika Event Group gagal dibuat (kehabisan memori)
        printf("Gagal membuat Event Group! Memori tidak cukup.\n");
        // Mengembalikan dari fungsi app_main karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa program Event Group Basic telah dimulai
    printf("=== ESP32 FreeRTOS Event Group Basic ===\n");
    // Mencetak pesan bahwa Event Group telah dibuat
    printf("Event Group berhasil dibuat\n");

    // Membuat task pertama dengan nama "Task1"
    xTaskCreate(vTask1, "Task1", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat task kedua dengan nama "Task2"
    xTaskCreate(vTask2, "Task2", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat task ketiga dengan nama "Task3"
    xTaskCreate(vTask3, "Task3", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat task monitor event dengan nama "EventMonitor"
    xTaskCreate(vEventMonitorTask, "EventMonitor", TASK_STACK_SIZE, NULL, TASK_PRIORITY + 1, NULL);

    // Mencetak pesan bahwa semua task telah dibuat
    printf("Semua task telah dibuat dan berjalan\n");
}

// Implementasi Task1 - memberi sinyal bit 0 secara periodik
void vTask1(void *pvParameters)
{
    // Variabel untuk menyimpan delay waktu dalam ticks
    TickType_t xDelay = pdMS_TO_TICKS(1000);
    // Loop tak terbatas untuk task
    while (1)
    {
        // Menyalakan LED sebagai indikator Task1 berjalan
        gpio_set_level(LED_GPIO_PIN, 1);
        // Mencetak pesan bahwa Task1 memberi sinyal BIT0
        printf("Task1: Memberi sinyal EVENT_BIT_0\n");
        // Mengatur bit EVENT_BIT_0 pada Event Group
        xEventGroupSetBits(xEventGroup, EVENT_BIT_0);
        // Menunggu selama 1 detik sebelum mengulang
        vTaskDelay(xDelay);
        // Mematikan LED
        gpio_set_level(LED_GPIO_PIN, 0);
        // Mencetak pesan bahwa Task1 menunggu
        printf("Task1: Menunggu 2 detik...\n");
        // Menunggu selama 2 detik sebelum memberi sinyal lagi
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Implementasi Task2 - memberi sinyal bit 1 secara periodik
void vTask2(void *pvParameters)
{
    // Variabel untuk menyimpan delay waktu dalam ticks
    TickType_t xDelay = pdMS_TO_TICKS(1500);
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa Task2 memberi sinyal BIT1
        printf("Task2: Memberi sinyal EVENT_BIT_1\n");
        // Mengatur bit EVENT_BIT_1 pada Event Group
        xEventGroupSetBits(xEventGroup, EVENT_BIT_1);
        // Menunggu selama 1.5 detik sebelum mengulang
        vTaskDelay(xDelay);
        // Mencetak pesan bahwa Task2 menunggu
        printf("Task2: Menunggu 2.5 detik...\n");
        // Menunggu selama 2.5 detik sebelum memberi sinyal lagi
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

// Implementasi Task3 - memberi sinyal bit 2 secara periodik
void vTask3(void *pvParameters)
{
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa Task3 memberi sinyal BIT2
        printf("Task3: Memberi sinyal EVENT_BIT_2\n");
        // Mengatur bit EVENT_BIT_2 pada Event Group
        xEventGroupSetBits(xEventGroup, EVENT_BIT_2);
        // Menunggu selama 3 detik sebelum mengulang
        vTaskDelay(pdMS_TO_TICKS(3000));
        // Mencetak pesan bahwa Task3 menunggu
        printf("Task3: Menunggu 2 detik...\n");
        // Menunggu selama 2 detik sebelum memberi sinyal lagi
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// Implementasi Event Monitor Task - menunggu event dengan AND dan OR condition
void vEventMonitorTask(void *pvParameters)
{
    // Variabel untuk menyimpan bit yang telah diterima dari Event Group
    EventBits_t uxBits;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa monitor menunggu event AND (semua bit)
        printf("\nMonitor: Menunggu event AND (BIT0 DAN BIT1)...\n");
        // Menunggu hingga BIT0 DAN BIT1 ter-set (AND condition)
        uxBits = xEventGroupWaitBits(
            xEventGroup,                    // Handle Event Group
            EVENT_BIT_0 | EVENT_BIT_1,      // Bit yang ditunggu
            pdTRUE,                         // Clear bit setelah dibaca
            pdTRUE,                         // Tunggu SEMUA bit (AND condition)
            portMAX_DELAY                   // Tunggu tanpa timeout
        );
        // Mencetak pesan bahwa event AND telah terpenuhi
        printf("Monitor: Event AND terpenuhi! Bit: 0x%lx\n", uxBits);

        // Mencetak pesan bahwa monitor menunggu event OR (salah satu bit)
        printf("Monitor: Menunggu event OR (BIT1 ATAU BIT2)...\n");
        // Menunggu hingga BIT1 ATAU BIT2 ter-set (OR condition)
        uxBits = xEventGroupWaitBits(
            xEventGroup,                    // Handle Event Group
            EVENT_BIT_1 | EVENT_BIT_2,      // Bit yang ditunggu
            pdTRUE,                         // Clear bit setelah dibaca
            pdFALSE,                        // Tunggu SALAH SATU bit (OR condition)
            portMAX_DELAY                   // Tunggu tanpa timeout
        );
        // Mencetak pesan bahwa event OR telah terpenuhi
        printf("Monitor: Event OR terpenuhi! Bit: 0x%lx\n", uxBits);

        // Mencetak pesan bahwa satu siklus monitor selesai
        printf("Monitor: Siklus selesai, mengulang...\n\n");
    }
}
