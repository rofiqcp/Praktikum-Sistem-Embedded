// File: src/main.c
// Deskripsi: Program ESP32 FreeRTOS Advanced - Memory Allocation Dynamic
// Demonstrasi penggunaan heap_caps_malloc dan memory tracking

// Menginclude header file ESP-IDF untuk sistem input/output
#include <stdio.h>
// Menginclude header file untuk manajemen memori ESP32
#include "esp_heap_caps.h"
// Menginclude header file FreeRTOS untuk task creation dan management
#include "freertos/FreeRTOS.h"
// Menginclude header file FreeRTOS untuk task creation dan deletion
#include "freertos/task.h"
// Menginclude header file driver GPIO ESP32
#include "driver/gpio.h"
// Menginclude header file string untuk manipulasi string
#include "string.h"
// Menginclude header file konfigurasi lokal
#include "config.h"

// Mendefinisikan tag untuk logging
static const char *TAG = "Memory_Allocation_Dynamic";

// Mendefinisikan struktur untuk tracking alokasi memori
typedef struct
{
    // Pointer ke memory yang dialokasi
    void *ptr;
    // Ukuran alokasi dalam bytes
    size_t size;
    // ID tracking
    int id;
} mem_track_t;

// Mendeklarasikan array untuk tracking alokasi memori
mem_track_t mem_tracks[HEAP_TRACK_SIZE];
// Mendeklarasikan counter untuk ID tracking
int track_count = 0;

// Fungsi untuk menampilkan informasi heap
void display_heap_info(void);
// Fungsi untuk melacak alokasi memori
void track_allocation(void *ptr, size_t size);
// Fungsi untuk membebaskan memori yang dilacak
void free_tracked(int id);

// Fungsi utama program (entry point ESP-IDF)
void app_main(void)
{
    // Menginisialisasi konfigurasi GPIO untuk pin LED
    gpio_reset_pin(LED_GPIO_PIN);
    // Mengatur arah pin GPIO sebagai output
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    // Mengatur level awal LED ke LOW (mati)
    gpio_set_level(LED_GPIO_PIN, 0);

    // Mencetak pesan bahwa program Memory Allocation Dynamic telah dimulai
    printf("=== ESP32 FreeRTOS Memory Allocation Dynamic ===\n");
    // Mencetak penjelasan tentang demonstrasi heap operations
    printf("Demonstrasi heap_caps_malloc dan Memory Tracking\n");

    // Menampilkan informasi heap awal
    display_heap_info();

    // Mencetak pesan bahwa akan melakukan alokasi memori pertama
    printf("\n--- Alokasi Memori 1: %d bytes di DMA_CAPABLE ---\n", ALLOC_SIZE_1);
    // Mengalokasi memori dengan heap_caps_malloc (DMA capable)
    void *ptr1 = heap_caps_malloc(ALLOC_SIZE_1, MALLOC_CAP_DMA);
    // Mengecek apakah alokasi berhasil
    if (ptr1 != NULL)
    {
        // Mencetak pesan sukses alokasi
        printf("Sukses! Pointer: %p\n", ptr1);
        // Melacak alokasi ini
        track_allocation(ptr1, ALLOC_SIZE_1);
        // Mengisi memori dengan data
        memset(ptr1, 0xAA, ALLOC_SIZE_1);
    }
    // Jika alokasi gagal
    else
    {
        // Mencetak pesan gagal
        printf("Gagal alokasi!\n");
    }
    // Menampilkan informasi heap setelah alokasi
    display_heap_info();

    // Mencetak pesan bahwa akan melakukan alokasi memori kedua
    printf("\n--- Alokasi Memori 2: %d bytes di DEFAULT ---\n", ALLOC_SIZE_2);
    // Mengalokasi memori dengan heap_caps_malloc (default)
    void *ptr2 = heap_caps_malloc(ALLOC_SIZE_2, MALLOC_CAP_DEFAULT);
    // Mengecek apakah alokasi berhasil
    if (ptr2 != NULL)
    {
        // Mencetak pesan sukses alokasi
        printf("Sukses! Pointer: %p\n", ptr2);
        // Melacak alokasi ini
        track_allocation(ptr2, ALLOC_SIZE_2);
        // Mengisi memori dengan data
        memset(ptr2, 0xBB, ALLOC_SIZE_2);
    }
    // Jika alokasi gagal
    else
    {
        // Mencetak pesan gagal
        printf("Gagal alokasi!\n");
    }
    // Menampilkan informasi heap setelah alokasi
    display_heap_info();

    // Mencetak pesan bahwa akan melakukan alokasi memori ketiga
    printf("\n--- Alokasi Memori 3: %d bytes di INTERNAL ---\n", ALLOC_SIZE_3);
    // Mengalokasi memori dengan heap_caps_malloc (internal RAM)
    void *ptr3 = heap_caps_malloc(ALLOC_SIZE_3, MALLOC_CAP_INTERNAL);
    // Mengecek apakah alokasi berhasil
    if (ptr3 != NULL)
    {
        // Mencetak pesan sukses alokasi
        printf("Sukses! Pointer: %p\n", ptr3);
        // Melacak alokasi ini
        track_allocation(ptr3, ALLOC_SIZE_3);
        // Mengisi memori dengan data
        memset(ptr3, 0xCC, ALLOC_SIZE_3);
    }
    // Jika alokasi gagal
    else
    {
        // Mencetak pesan gagal
        printf("Gagal alokasi!\n");
    }
    // Menampilkan informasi heap setelah alokasi
    display_heap_info();

    // Mencetak pesan bahwa akan membebaskan memori
    printf("\n--- Membebaskan Memori ---\n");
    // Membebaskan memori yang dilacak dengan ID 1
    free_tracked(1);
    // Menampilkan informasi heap setelah pembebasan
    display_heap_info();

    // Membebaskan memori yang dilacak dengan ID 2
    free_tracked(2);
    // Menampilkan informasi heap setelah pembebasan
    display_heap_info();

    // Membebaskan memori yang dilacak dengan ID 3
    free_tracked(3);
    // Menampilkan informasi heap setelah pembebasan
    display_heap_info();

    // Mencetak pesan bahwa demonstrasi selesai
    printf("\n=== Demonstrasi Selesai ===\n");

    // Loop utama dengan LED berkedip
    while (1)
    {
        // Menyalakan LED
        gpio_set_level(LED_GPIO_PIN, 1);
        // Menunggu 500ms
        vTaskDelay(pdMS_TO_TICKS(500));
        // Mematikan LED
        gpio_set_level(LED_GPIO_PIN, 0);
        // Menunggu 500ms
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Implementasi fungsi untuk menampilkan informasi heap
void display_heap_info(void)
{
    // Mendeklarasikan variabel untuk menyimpan ukuran heap
    size_t free_heap, min_free_heap;
    // Mendapatkan ukuran heap bebas
    free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    // Mendapatkan ukuran heap bebas minimum yang pernah ada
    min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    // Mencetak informasi heap bebas
    printf("Heap Bebas: %lu bytes\n", (unsigned long)free_heap);
    // Mencetak informasi heap bebas minimum
    printf("Heap Bebas Minimum: %lu bytes\n", (unsigned long)min_free_heap);
}

// Implementasi fungsi untuk melacak alokasi memori
void track_allocation(void *ptr, size_t size)
{
    // Mengecek apakah masih ada slot tracking tersedia
    if (track_count < HEAP_TRACK_SIZE)
    {
        // Menyimpan pointer ke array tracking
        mem_tracks[track_count].ptr = ptr;
        // Menyimpan ukuran ke array tracking
        mem_tracks[track_count].size = size;
        // Menyimpan ID ke array tracking
        mem_tracks[track_count].id = track_count + 1;
        // Mencetak pesan bahwa alokasi dilacak
        printf("Tracking: ID=%d, Size=%lu\n", mem_tracks[track_count].id, (unsigned long)size);
        // Increment counter tracking
        track_count++;
    }
}

// Implementasi fungsi untuk membebaskan memori yang dilacak
void free_tracked(int id)
{
    // Mendeklarasikan variabel iterator
    int i;
    // Loop untuk mencari ID yang sesuai
    for (i = 0; i < track_count; i++)
    {
        // Mengecek apakah ID cocok
        if (mem_tracks[i].id == id && mem_tracks[i].ptr != NULL)
        {
            // Mencetak pesan bahwa memori akan dibebaskan
            printf("Membebaskan: ID=%d, Pointer=%p, Size=%lu\n", id, mem_tracks[i].ptr, (unsigned long)mem_tracks[i].size);
            // Membebaskan memori dengan heap_caps_free
            heap_caps_free(mem_tracks[i].ptr);
            // Mengatur pointer ke NULL
            mem_tracks[i].ptr = NULL;
            // Mengembalikan dari fungsi
            return;
        }
    }
    // Mencetak pesan jika ID tidak ditemukan
    printf("ID %d tidak ditemukan!\n", id);
}
