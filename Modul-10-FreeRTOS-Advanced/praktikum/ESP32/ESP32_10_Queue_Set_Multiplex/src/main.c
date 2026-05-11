// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Queue Set Multiplex
// Demonstrasi penggunaan Queue Set untuk menunggu multiple queues

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk queue operations
#include "freertos/queue.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Queue_Set_Multiplex";

// Mendeklarasikan handle untuk queue 1
QueueHandle_t xQueue1;
// Mendeklarasikan handle untuk queue 2
QueueHandle_t xQueue2;
// Mendeklarasikan handle untuk queue 3
QueueHandle_t xQueue3;
// Mendeklarasikan handle untuk queue set
QueueSetHandle_t xQueueSet;

// Mendeklarasikan prototipe fungsi untuk task-task
void vProducer1Task(void *pvParameters);
void vProducer2Task(void *pvParameters);
void vProducer3Task(void *pvParameters);
void vConsumerTask(void *pvParameters);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Queue Set Multiplex telah dimulai
    printf("=== ESP32 FreeRTOS Queue Set Multiplex ===\n");
    // Mencetak penjelasan tentang demonstrasi queue set
    printf("Demonstrasi Wait on Multiple Queues dengan Queue Set\n");

    // Membuat queue 1 dengan xQueueCreate
    xQueue1 = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    // Mengecek apakah queue 1 berhasil dibuat
    if (xQueue1 == NULL)
    {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Gagal membuat Queue 1!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }
    // Mencetak pesan bahwa queue 1 berhasil dibuat
    printf("Queue 1 berhasil dibuat\n");

    // Membuat queue 2 dengan xQueueCreate
    xQueue2 = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    // Mengecek apakah queue 2 berhasil dibuat
    if (xQueue2 == NULL)
    {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Gagal membuat Queue 2!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }
    // Mencetak pesan bahwa queue 2 berhasil dibuat
    printf("Queue 2 berhasil dibuat\n");

    // Membuat queue 3 dengan xQueueCreate
    xQueue3 = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    // Mengecek apakah queue 3 berhasil dibuat
    if (xQueue3 == NULL)
    {
        // Mencetak pesan error jika queue gagal dibuat
        printf("Gagal membuat Queue 3!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }
    // Mencetak pesan bahwa queue 3 berhasil dibuat
    printf("Queue 3 berhasil dibuat\n");

    // Membuat queue set dengan xQueueCreateSet
    xQueueSet = xQueueCreateSet(
        (QUEUE_LENGTH * NUM_QUEUES * QUEUE_ITEM_SIZE) // Kapasitas total queue set
    );

    // Mengecek apakah queue set berhasil dibuat
    if (xQueueSet == NULL)
    {
        // Mencetak pesan error jika queue set gagal dibuat
        printf("Gagal membuat Queue Set!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }
    // Mencetak pesan bahwa queue set berhasil dibuat
    printf("Queue Set berhasil dibuat\n");

    // Menambahkan queue 1 ke queue set dengan xQueueAddToSet
    xQueueAddToSet(xQueue1, xQueueSet);
    // Mencetak pesan bahwa queue 1 ditambahkan ke set
    printf("Queue 1 ditambahkan ke Queue Set\n");

    // Menambahkan queue 2 ke queue set dengan xQueueAddToSet
    xQueueAddToSet(xQueue2, xQueueSet);
    // Mencetak pesan bahwa queue 2 ditambahkan ke set
    printf("Queue 2 ditambahkan ke Queue Set\n");

    // Menambahkan queue 3 ke queue set dengan xQueueAddToSet
    xQueueAddToSet(xQueue3, xQueueSet);
    // Mencetak pesan bahwa queue 3 ditambahkan ke set
    printf("Queue 3 ditambahkan ke Queue Set\n");

    // Membuat producer task 1 dengan nama "Producer1"
    xTaskCreate(vProducer1Task, "Producer1", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat producer task 2 dengan nama "Producer2"
    xTaskCreate(vProducer2Task, "Producer2", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat producer task 3 dengan nama "Producer3"
    xTaskCreate(vProducer3Task, "Producer3", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat consumer task dengan nama "Consumer"
    xTaskCreate(vConsumerTask, "Consumer", TASK_STACK_SIZE, NULL, TASK_PRIORITY + 1, NULL);

    // Mencetak pesan bahwa semua task telah dibuat
    printf("3 Producer tasks dan 1 Consumer task telah dibuat\n");
    // Mencetak pesan bahwa consumer akan menunggu data dari multiple queues
    printf("Consumer akan menunggu data dari queue manapun...\n");
}

// Implementasi Producer 1 Task - mengirim data ke queue 1
void vProducer1Task(void *pvParameters)
{
    // Mendeklarasikan variabel counter untuk data yang dikirim
    int sendValue = 100;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xStatus;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Increment nilai yang akan dikirim
        sendValue++;
        // Mencetak pesan bahwa producer 1 akan mengirim
        printf("[Prod1] Mengirim: %d\n", sendValue);
        // Mengirim data ke queue 1 dengan xQueueSend
        xStatus = xQueueSend(xQueue1, &sendValue, 0);
        // Mengecek apakah pengiriman berhasil
        if (xStatus == pdPASS)
        {
            // Mencetak pesan sukses mengirim
            printf("[Prod1] Data terkirim ke Queue 1\n");
        }
        // Jika queue penuh
        else
        {
            // Mencetak pesan bahwa queue penuh
            printf("[Prod1] Queue 1 penuh!\n");
        }
        // Menunggu sebelum mengirim data berikutnya
        vTaskDelay(pdMS_TO_TICKS(SEND_DELAY_MS));
    }
}

// Implementasi Producer 2 Task - mengirim data ke queue 2
void vProducer2Task(void *pvParameters)
{
    // Mendeklarasikan variabel counter untuk data yang dikirim
    int sendValue = 200;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xStatus;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Increment nilai yang akan dikirim
        sendValue += 2;
        // Mencetak pesan bahwa producer 2 akan mengirim
        printf("[Prod2] Mengirim: %d\n", sendValue);
        // Mengirim data ke queue 2 dengan xQueueSend
        xStatus = xQueueSend(xQueue2, &sendValue, 0);
        // Mengecek apakah pengiriman berhasil
        if (xStatus == pdPASS)
        {
            // Mencetak pesan sukses mengirim
            printf("[Prod2] Data terkirim ke Queue 2\n");
        }
        // Jika queue penuh
        else
        {
            // Mencetak pesan bahwa queue penuh
            printf("[Prod2] Queue 2 penuh!\n");
        }
        // Menunggu sebelum mengirim data berikutnya (waktu berbeda)
        vTaskDelay(pdMS_TO_TICKS(SEND_DELAY_MS + 500));
    }
}

// Implementasi Producer 3 Task - mengirim data ke queue 3
void vProducer3Task(void *pvParameters)
{
    // Mendeklarasikan variabel counter untuk data yang dikirim
    int sendValue = 300;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xStatus;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Increment nilai yang akan dikirim
        sendValue += 3;
        // Mencetak pesan bahwa producer 3 akan mengirim
        printf("[Prod3] Mengirim: %d\n", sendValue);
        // Mengirim data ke queue 3 dengan xQueueSend
        xStatus = xQueueSend(xQueue3, &sendValue, 0);
        // Mengecek apakah pengiriman berhasil
        if (xStatus == pdPASS)
        {
            // Mencetak pesan sukses mengirim
            printf("[Prod3] Data terkirim ke Queue 3\n");
        }
        // Jika queue penuh
        else
        {
            // Mencetak pesan bahwa queue penuh
            printf("[Prod3] Queue 3 penuh!\n");
        }
        // Menunggu sebelum mengirim data berikutnya (waktu berbeda)
        vTaskDelay(pdMS_TO_TICKS(SEND_DELAY_MS + 1000));
    }
}

// Implementasi Consumer Task - menerima data dari queue manapun dalam set
void vConsumerTask(void *pvParameters)
{
    // Mendeklarasikan variabel untuk menyimpan data yang diterima
    int receiveValue;
    // Mendeklarasikan variabel untuk menyimpan handle queue yang ready
    QueueSetMemberHandle_t xActivatedMember;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa consumer menunggu data
        printf("[Consumer] Menunggu data dari queue manapun...\n");
        // Menunggu queue manapun dalam set yang memiliki data dengan xQueueSelectFromSet
        xActivatedMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
        // Mengecek apakah ada queue yang ready
        if (xActivatedMember != NULL)
        {
            // Menyalakan LED sebagai indikator data diterima
            gpio_set_level(LED_GPIO_PIN, 1);
            // Mengecek apakah yang ready adalah queue 1
            if (xActivatedMember == xQueue1)
            {
                // Menerima data dari queue 1
                xQueueReceive(xActivatedMember, &receiveValue, 0);
                // Mencetak pesan bahwa data diterima dari queue 1
                printf("[Consumer] Data dari Queue 1: %d\n", receiveValue);
            }
            // Mengecek apakah yang ready adalah queue 2
            else if (xActivatedMember == xQueue2)
            {
                // Menerima data dari queue 2
                xQueueReceive(xActivatedMember, &receiveValue, 0);
                // Mencetak pesan bahwa data diterima dari queue 2
                printf("[Consumer] Data dari Queue 2: %d\n", receiveValue);
            }
            // Mengecek apakah yang ready adalah queue 3
            else if (xActivatedMember == xQueue3)
            {
                // Menerima data dari queue 3
                xQueueReceive(xActivatedMember, &receiveValue, 0);
                // Mencetak pesan bahwa data diterima dari queue 3
                printf("[Consumer] Data dari Queue 3: %d\n", receiveValue);
            }
            // Jika anggota set tidak dikenali
            else
            {
                // Mencetak pesan error
                printf("[Consumer] Unknown queue member!\n");
            }
            // Mematikan LED
            gpio_set_level(LED_GPIO_PIN, 0);
        }
        // Jika tidak ada yang ready (seharusnya tidak terjadi karena portMAX_DELAY)
        else
        {
            // Mencetak pesan error
            printf("[Consumer] Tidak ada data!\n");
        }
    }
}
