# PPT Prompts FreeRTOS Modul 12: Memory Management & Advanced Features

## Slide 1: Judul
- **Judul**: Memory Management & Advanced Features
- **Subjudul**: Modul 12 - Praktikum Sistem Embedded
- **Gambar**: Ilustrasi RAM, Heap, Stack, dan struktur Task Control Block (TCB).

## Slide 2: Tujuan Pembelajaran
- **Poin-poin**:
  1.  Memahami manajemen memori di FreeRTOS (Static vs Dynamic).
  2.  Mampu memilih skema Heap (Heap_1 sampai Heap_5).
  3.  Mampu mendeteksi dan menangani Stack Overflow.
  4.  Menguasai fitur lanjutan: Event Groups, Stream Buffer, dan Message Buffer.

## Slide 3: Bagaimana FreeRTOS Menggunakan RAM?
- **Isi**:
  - Setiap kali Task, Queue, Semaphore, atau Timer dibuat, FreeRTOS membutuhkan RAM.
  - **Dua metode alokasi**:
    1.  **Dynamic Allocation**: Otomatis menggunakan `pvPortMalloc()`. Paling umum.
    2.  **Static Allocation**: Manual menyediakan buffer global. Lebih aman/prediktif.
- **Visual**: Diagram blok RAM terbagi menjadi Global Variables, Stack, dan Heap.

## Slide 4: Dynamic Memory Allocation (Heap)
- **Isi**:
  - FreeRTOS tidak menggunakan `malloc()` bawaan C secara langsung.
  - Memiliki *Memory Allocator* sendiri yang portabel.
  - Fungsi: `pvPortMalloc()` dan `vPortFree()`.
  - Dipanggil otomatis saat `xTaskCreate`, `xQueueCreate`, dll.

## Slide 5: Pilihan Skema Heap (Heap_1.c)
- **Judul**: Heap_1.c
- **Karakteristik**:
  - Paling sederhana.
  - **Hanya bisa alokasi, TIDAK bisa free**.
  - Cocok untuk sistem yang semua object-nya dibuat di awal dan tidak pernah dihapus.
  - Deterministik dan aman.
- **Visual**: Blok memori yang terisi berurutan dari bawah ke atas.

## Slide 6: Pilihan Skema Heap (Heap_2.c)
- **Judul**: Heap_2.c
- **Karakteristik**:
  - Bisa `malloc` dan `free`.
  - Menggunakan algoritma *Best Fit*.
  - **Masalah**: Tidak menggabungkan blok kosong yang berdekatan (Fragmentasi).
  - Tidak direkomendasikan untuk alokasi/dealokasi acak yang sering.

## Slide 7: Pilihan Skema Heap (Heap_3.c)
- **Judul**: Heap_3.c
- **Karakteristik**:
  - Hanya *wrapper* untuk `malloc()` dan `free()` standar library C compiler.
  - Thread-safe (menghentikan scheduler saat alokasi).
  - Ukuran heap diatur via Linker/Startup file, bukan `configTOTAL_HEAP_SIZE`.

## Slide 8: Pilihan Skema Heap (Heap_4.c) - The Best!
- **Judul**: Heap_4.c (Recommended)
- **Karakteristik**:
  - Bisa `malloc` dan `free`.
  - Menggunakan algoritma *First Fit*.
  - **Coalescing**: Menggabungkan blok kosong yang bersebelahan untuk mencegah fragmentasi kecil.
  - Bisa menempatkan heap di alamat memori tertentu.
  - Paling sering digunakan di STM32/ESP32.

## Slide 9: Stack Management
- **Isi**:
  - Setiap Task memiliki **Stack** sendiri.
  - Ukuran ditentukan saat `xTaskCreate`.
  - Stack menyimpan: Variabel lokal, Return Address, Register CPU context.
  - **Bahaya**: Stack Overflow (Melebihi batas stack) -> Data korup / Crash.

## Slide 10: Stack Overflow Detection (Method 1)
- **Judul**: Stack Overflow Check Mode 1
- **Konfigurasi**: `configCHECK_FOR_STACK_OVERFLOW = 1`
- **Cara Kerja**:
  - Memeriksa Stack Pointer saat *Context Switch*.
  - Jika Stack Pointer keluar dari range stack task -> Panggil Hook.
- **Kelemahan**: Tidak mendeteksi overflow yang terjadi *di antara* tick interrupts.

## Slide 11: Stack Overflow Detection (Method 2)
- **Judul**: Stack Overflow Check Mode 2
- **Konfigurasi**: `configCHECK_FOR_STACK_OVERFLOW = 2`
- **Cara Kerja**:
  - Mengisi stack dengan pola (*watermarking*) saat task dibuat (0xA5A5A5A5).
  - Memeriksa 20 bytes terakhir dari stack saat context switch.
  - Jika pola berubah -> Overflow terjadi.
- **Kelebihan**: Lebih akurat menangkap overflow sesaat.

## Slide 12: Static Memory Allocation
- **Isi**:
  - Object dibuat tanpa menyentuh Heap sama sekali.
  - Developer menyediakan struct dan buffer array secara global.
  - API: `xTaskCreateStatic`, `xQueueCreateStatic`.
- **Kelebihan**:
  - Tidak ada fragmentasi.
  - Alokasi memori pasti berhasil (jika compile success).
  - Cocok untuk *Safety Critical Systems* (Automotive, Medical).

## Slide 13: Kesimpulan Memory
- **Poin Penting**:
  1.  Gunakan `Heap_4.c` untuk kebutuhan umum.
  2.  Selalu monitor `uxTaskGetStackHighWaterMark()` saat development.
  3.  Aktifkan Stack Overflow Hook (`configCHECK_FOR_STACK_OVERFLOW`).
  4.  Pertimbangkan Static Allocation jika memori sangat terbatas atau butuh determinisme tinggi.

## Slide 14: Next Topic
- **Isi**: Advanced FreeRTOS System Resources (Event Groups & Stream Buffers).
