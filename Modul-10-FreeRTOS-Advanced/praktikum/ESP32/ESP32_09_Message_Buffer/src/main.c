// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Message Buffer
// Demonstrasi penggunaan Message Buffer dengan xMessageBufferCreate

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file FreeRTOS untuk message buffer operations
#include "freertos/message_buffer.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file string untuk manipulasi string
#include "string.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Message_Buffer";

// Mendeklarasikan handle untuk message buffer
MessageBufferHandle_t xMessageBuffer;

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

    // Mencetak pesan bahwa program Message Buffer telah dimulai
    printf("=== ESP32 FreeRTOS Message Buffer ===\n");
    // Mencetak penjelasan tentang demonstrasi message buffer
    printf("Demonstrasi Message Buffer (xMessageBufferCreate)\n");

    // Membuat message buffer dengan xMessageBufferCreate
    xMessageBuffer = xMessageBufferCreate(MESSAGE_BUFFER_SIZE);

    // Mengecek apakah message buffer berhasil dibuat
    if (xMessageBuffer == NULL)
    {
        // Mencetak pesan error jika message buffer gagal dibuat
        printf("Gagal membuat Message Buffer!\n");
        // Mengembalikan dari fungsi karena gagal inisialisasi
        return;
    }

    // Mencetak pesan bahwa message buffer berhasil dibuat
    printf("Message Buffer berhasil dibuat\n");
    // Mencetak ukuran buffer
    printf("Buffer Size: %d bytes\n", MESSAGE_BUFFER_SIZE);
    // Mencetak panjang pesan maksimum
    printf("Max Message Length: %d bytes\n", MAX_MESSAGE_LEN);

    // Membuat task sender dengan nama "SenderTask"
    xTaskCreate(vSenderTask, "SenderTask", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);
    // Membuat task receiver dengan nama "ReceiverTask"
    xTaskCreate(vReceiverTask, "ReceiverTask", TASK_STACK_SIZE, NULL, TASK_PRIORITY + 1, NULL);

    // Mencetak pesan bahwa semua task telah dibuat
    printf("Sender dan Receiver task telah dibuat\n");
    // Mencetak pesan bahwa program siap mengirim pesan
    printf("Mulai mengirim dan menerima pesan...\n");
}

// Implementasi Sender Task - mengirim pesan ke message buffer
void vSenderTask(void *pvParameters)
{
    // Mendeklarasikan array untuk menyimpan pesan
    char message[MAX_MESSAGE_LEN];
    // Mendeklarasikan variabel counter untuk nomor pesan
    int msgCount = 0;
    // Mendeklarasikan variabel untuk menyimpan hasil operasi
    BaseType_t xResult;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Increment counter pesan
        msgCount++;
        // Format pesan dengan sprintf
        snprintf(message, MAX_MESSAGE_LEN, "Pesan #%d dari Sender", msgCount);
        // Mencetak pesan bahwa akan mengirim
        printf("[Sender] Mengirim: %s\n", message);
        // Mengirim pesan ke message buffer dengan xMessageBufferSend
        xResult = xMessageBufferSend(
            xMessageBuffer,           // Handle message buffer
            (void *)message,         // Pointer ke data yang dikirim
            strlen(message) + 1,     // Panjang data (termasuk null terminator)
            portMAX_DELAY            // Tunggu tanpa timeout
        );
        // Mengecek apakah pengiriman berhasil
        if (xResult == pdPASS)
        {
            // Mencetak pesan bahwa pengiriman sukses
            printf("[Sender] Pesan terkirim!\n");
        }
        // Jika gagal mengirim
        else
        {
            // Mencetak pesan error
            printf("[Sender] Gagal mengirim pesan!\n");
        }
        // Menunggu sebelum mengirim pesan berikutnya
        vTaskDelay(pdMS_TO_TICKS(SEND_DELAY_MS));
    }
}

// Implementasi Receiver Task - menerima pesan dari message buffer
void vReceiverTask(void *pvParameters)
{
    // Mendeklarasikan array untuk menyimpan pesan yang diterima
    char receivedMsg[MAX_MESSAGE_LEN];
    // Mendeklarasikan variabel untuk menyimpan panjang pesan yang diterima
    size_t xReceivedBytes;
    // Loop tak terbatas untuk task
    while (1)
    {
        // Mencetak pesan bahwa receiver menunggu
        printf("[Receiver] Menunggu pesan...\n");
        // Menerima pesan dari message buffer dengan xMessageBufferReceive
        xReceivedBytes = xMessageBufferReceive(
            xMessageBuffer,           // Handle message buffer
            (void *)receivedMsg,      // Pointer ke buffer penerima
            MAX_MESSAGE_LEN,          // Panjang buffer maksimum
            portMAX_DELAY             // Tunggu tanpa timeout
        );
        // Mengecek apakah ada pesan yang diterima
        if (xReceivedBytes > 0)
        {
            // Menyalakan LED sebagai indikator pesan diterima
            gpio_set_level(LED_GPIO_PIN, 1);
            // Mencetak pesan bahwa pesan diterima
            printf("[Receiver] Pesan DITERIMA: %s\n", receivedMsg);
            // Mencetak panjang bytes yang diterima
            printf("[Receiver] Bytes diterima: %d\n", xReceivedBytes);
            // Menunggu sebentar untuk melihat LED
            vTaskDelay(pdMS_TO_TICKS(200));
            // Mematikan LED
            gpio_set_level(LED_GPIO_PIN, 0);
        }
        // Jika tidak ada pesan (seharusnya tidak terjadi karena portMAX_DELAY)
        else
        {
            // Mencetak pesan error
            printf("[Receiver] Gagal menerima pesan!\n");
        }
    }
}
