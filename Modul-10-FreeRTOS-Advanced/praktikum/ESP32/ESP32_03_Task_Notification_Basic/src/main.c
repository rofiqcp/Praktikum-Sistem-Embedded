// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Task Notification Basic
// Demonstrasi penggunaan Task Notifications (Give/Take, Value Setting)

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Task_Notification_Basic";

// Mendeklarasikan handle untuk task penerima notifikasi
TaskHandle_t xReceiverTaskHandle = NULL;
// Mendeklarasikan handle untuk task pengirim notifikasi
TaskHandle_t xSenderTaskHandle = NULL;

// Mendeklarasikan prototipe fungsi untuk task-task
void vSenderTask(void *pvParameters);
void vReceiverTask(void *pvParameters);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Task Notification Basic telah dimulai
    printf("=== ESP32 FreeRTOS Task Notification Basic ===\n");
    // Mencetak penjelasan tentang demonstrasi task notification
    printf("Demonstrasi Task Notifications (Give/Take, Value)\n");

    // Membuat task receiver dengan nama "ReceiverTask"
    xTaskCreate(vReceiverTask, "ReceiverTask", TASK_STACK_SIZE, NULL, TASK_PRIORITY + 1, &xReceiverTaskHandle);
    // Membuat task sender dengan nama "SenderTask"
    xTaskCreate(vSenderTask, "SenderTask", TASK_STACK_SIZE, NULL, TASK_PRIORITY, &xSenderTaskHandle);

    // Mengecek apakah task receiver berhasil dibuat
    if (xReceiverTaskHandle == NULL)
    {
        // Mencetak pesan error jika task gagal dibuat
        printf("Gagal membuat Receiver Task!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mengecek apakah task sender berhasil dibuat
    if (xSenderTaskHandle == NULL)
    {
        // Mencetak pesan error jika task gagal dibuat
        printf("Gagal membuat Sender Task!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa semua task telah dibuat
    printf("Task Receiver dan Sender telah dibuat\n");
    // Mencetak pesan bahwa program siap menerima notifikasi
    printf("Receiver akan menunggu notifikasi dari Sender...\n");
}

// Implementasi Sender Task - mengirim notifikasi ke receiver
void vSenderTask(void *pvParameters)
{
    // Mendeklarasikan variabel counter untuk nilai notifikasi
    uint32_t ulNotificationValue = 0;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Menunggu selama 2 detik sebelum mengirim notifikasi pertama
        vTaskDelay(pdMS_TO_TICKS(2000));
        // Increment nilai notifikasi
        ulNotificationValue++;
        // Mencetak pesan bahwa sender akan mengirim notifikasi
        printf("\nSender: Mengirim notifikasi (nilai: %lu)\n", ulNotificationValue);
        // Mengirim notifikasi ke receiver task dengan xTaskNotify
        xTaskNotify(xReceiverTaskHandle, ulNotificationValue, eSetValueWithOverwrite);
        // Mencetak pesan bahwa notifikasi telah dikirim
        printf("Sender: Notifikasi terkirim!\n");

        // Menunggu selama 1 detik
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Mencetak pesan bahwa sender akan mengirim notifikasi tanpa nilai
        printf("\nSender: Mengirim notifikasi (tanpa nilai increment)\n");
        // Mengirim notifikasi ke receiver task tanpa nilai spesifik (hanya memberi sinyal)
        xTaskNotifyGive(xReceiverTaskHandle);
        // Mencetak pesan bahwa notifikasi give telah dikirim
        printf("Sender: Notifikasi Give terkirim!\n");

        // Menunggu selama 3 detik sebelum siklus berikutnya
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// Implementasi Receiver Task - menerima notifikasi dari sender
void vReceiverTask(void *pvParameters)
{
    // Mendeklarasikan variabel untuk menyimpan nilai notifikasi yang diterima
    uint32_t ulNotifiedValue;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xResult;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa receiver menunggu notifikasi
        printf("Receiver: Menunggu notifikasi...\n");
        // Menunggu notifikasi dengan xTaskNotifyWait
        xResult = xTaskNotifyWait(
            pdFALSE,              // Jangan clear bit notifikasi saat entry
            ULONG_MAX,            // Clear semua bit notifikasi saat exit
            &ulNotifiedValue,     // Pointer untuk menyimpan nilai notifikasi
            portMAX_DELAY         // Tunggu tanpa timeout
        );
        // Mengecek apakah notifikasi diterima
        if (xResult == pdPASS)
        {
            // Menyalakan LED sebagai indikator notifikasi diterima
            gpio_set_level(LED_GPIO_PIN, 1);
            // Mencetak pesan bahwa notifikasi telah diterima
            printf("Receiver: Notifikasi DITERIMA!\n");
            // Mencetak nilai notifikasi yang diterima
            printf("Receiver: Nilai notifikasi: %lu\n", ulNotifiedValue);
            // Mengecek apakah nilai notifikasi bukan 0 (berarti ada nilai yang dikirim)
            if (ulNotifiedValue != 0)
            {
                // Mencetak pesan bahwa nilai notifikasi berhasil diterima
                printf("Receiver: Nilai berhasil diterima via eSetValueWithOverwrite\n");
            }
            // Mengecek apakah nilai notifikasi adalah 0 (berarti xTaskNotifyGive yang dipanggil)
            else
            {
                // Mencetak pesan bahwa ini adalah notifikasi give (tanpa nilai)
                printf("Receiver: Ini adalah notifikasi Give (tanpa nilai)\n");
            }
            // Menunggu selama 500ms untuk melihat LED menyala
            vTaskDelay(pdMS_TO_TICKS(500));
            // Mematikan LED
            gpio_set_level(LED_GPIO_PIN, 0);
        }
        // Jika notifikasi tidak diterima (seharusnya tidak terjadi karena portMAX_DELAY)
        else
        {
            // Mencetak pesan error
            printf("Receiver: Gagal menerima notifikasi!\n");
        }
    }
}
