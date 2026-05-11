// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Static Allocation
// Demonstrasi penggunaan Static Allocation untuk Task dan Queue

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task static allocation
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk queue operations
#include "freertos/queue.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Static_Allocation";

// Mendefinisikan array statis untuk stack task sender
static StaticTask_t xSenderTaskTCB;
static StackType_t xSenderStack[TASK_STACK_SIZE_WORDS];

// Mendefinisikan array statis untuk stack task receiver
static StaticTask_t xReceiverTaskTCB;
static StackType_t xReceiverStack[TASK_STACK_SIZE_WORDS];

// Mendefinisikan array statis untuk queue
static StaticQueue_t xStaticQueue;
static uint8_t ucQueueStorageArea[QUEUE_LENGTH * QUEUE_ITEM_SIZE];

// Mendeklarasikan handle untuk queue
QueueHandle_t xQueue;

// Mendeklarasikan handle untuk task
TaskHandle_t xSenderTaskHandle = NULL;
TaskHandle_t xReceiverTaskHandle = NULL;

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

    // Mencetak pesan bahwa program Static Allocation telah dimulai
    printf("=== ESP32 FreeRTOS Static Allocation ===\n");
    // Mencetak penjelasan tentang demonstrasi static allocation
    printf("Demonstrasi Static Task dan Queue Allocation\n");

    // Membuat queue statis dengan xQueueCreateStatic
    xQueue = xQueueCreateStatic(
        QUEUE_LENGTH,               // Panjang queue (jumlah item)
        QUEUE_ITEM_SIZE,            // Ukuran setiap item
        ucQueueStorageArea,         // Pointer ke area penyimpanan queue
        &xStaticQueue               // Pointer ke struktur statis queue
    );

    // Mengecek apakah queue berhasil dibuat
    if (xQueue == NULL)
    {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Gagal membuat Static Queue!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa queue statis berhasil dibuat
    printf("Static Queue berhasil dibuat\n");
    // Mencetak informasi panjang queue
    printf("Queue Length: %d, Item Size: %d bytes\n", QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    // Membuat task sender secara statis dengan xTaskCreateStatic
    xSenderTaskHandle = xTaskCreateStatic(
        vSenderTask,                // Fungsi task
        "SenderTask",               // Nama task
        TASK_STACK_SIZE_WORDS,      // Ukuran stack dalam words
        NULL,                       // Parameter task
        TASK_PRIORITY,              // Priority task
        xSenderStack,               // Pointer ke array stack
        &xSenderTaskTCB             // Pointer ke TCB (Task Control Block)
    );

    // Mengecek apakah task sender berhasil dibuat
    if (xSenderTaskHandle == NULL)
    {
        // Mencetak pesan error jika task gagal dibuat
        printf("Gagal membuat Static Sender Task!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa task sender statis berhasil dibuat
    printf("Static Sender Task berhasil dibuat\n");

    // Membuat task receiver secara statis dengan xTaskCreateStatic
    xReceiverTaskHandle = xTaskCreateStatic(
        vReceiverTask,              // Fungsi task
        "ReceiverTask",             // Nama task
        TASK_STACK_SIZE_WORDS,      // Ukuran stack dalam words
        NULL,                       // Parameter task
        TASK_PRIORITY + 1,          // Priority task (lebih tinggi dari sender)
        xReceiverStack,             // Pointer ke array stack
        &xReceiverTaskTCB           // Pointer ke TCB (Task Control Block)
    );

    // Mengecek apakah task receiver berhasil dibuat
    if (xReceiverTaskHandle == NULL)
    {
        // Mencetak pesan error jika task gagal dibuat
        printf("Gagal membuat Static Receiver Task!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa task receiver statis berhasil dibuat
    printf("Static Receiver Task berhasil dibuat\n");
    // Mencetak pesan bahwa semua komponen statis siap
    printf("Semua komponen statis siap, program berjalan...\n");
}

// Implementasi Sender Task - mengirim data ke queue secara statis
void vSenderTask(void *pvParameters)
{
    // Mendeklarasikan variabel counter untuk data yang dikirim
    int sendValue = 0;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xStatus;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Increment nilai yang akan dikirim
        sendValue++;
        // Mencetak pesan bahwa task akan mengirim data
        printf("[Sender] Mengirim: %d\n", sendValue);
        // Mengirim data ke queue dengan xQueueSend
        xStatus = xQueueSend(xQueue, &sendValue, 0);
        // Mengecek apakah pengiriman berhasil
        if (xStatus == pdPASS)
        {
            // Mencetak pesan sukses mengirim
            printf("[Sender] Data terkirim ke queue\n");
        }
        // Jika queue penuh
        else
        {
            // Mencetak pesan bahwa queue penuh
            printf("[Sender] Queue penuh!\n");
        }
        // Menunggu sebelum mengirim data berikutnya
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Implementasi Receiver Task - menerima data dari queue secara statis
void vReceiverTask(void *pvParameters)
{
    // Mendeklarasikan variabel untuk menyimpan data yang diterima
    int receiveValue;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xStatus;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa task menunggu data
        printf("[Receiver] Menunggu data dari queue...\n");
        // Menerima data dari queue dengan xQueueReceive
        xStatus = xQueueReceive(xQueue, &receiveValue, portMAX_DELAY);
        // Mengecek apakah penerimaan berhasil
        if (xStatus == pdPASS)
        {
            // Menyalakan LED sebagai indikator data diterima
            gpio_set_level(LED_GPIO_PIN, 1);
            // Mencetak pesan bahwa data diterima
            printf("[Receiver] Data DITERIMA: %d\n", receiveValue);
            // Menunggu sebentar untuk melihat LED
            vTaskDelay(pdMS_TO_TICKS(200));
            // Mematikan LED
            gpio_set_level(LED_GPIO_PIN, 0);
        }
        // Jika gagal menerima (seharusnya tidak terjadi karena portMAX_DELAY)
        else
        {
            // Mencetak pesan error
            printf("[Receiver] Gagal menerima data!\n");
        }
    }
}
