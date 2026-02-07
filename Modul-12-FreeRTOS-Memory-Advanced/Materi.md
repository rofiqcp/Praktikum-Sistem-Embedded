# Materi Modul 12: FreeRTOS Memory Management & Advanced Features

## 📚 Daftar Isi
1. [Pendahuluan](#1-pendahuluan)
2. [Memory Management](#2-memory-management)
3. [Stack Management & Overflow](#3-stack-management--overflow)
4. [Static Memory Allocation](#4-static-memory-allocation)
5. [Event Group](#5-event-group)
6. [Stream Buffer & Message Buffer](#6-stream-buffer--message-buffer)

---

## 1. Pendahuluan

Seiring kompleksitas sistem embedded meningkat, manajemen memori menjadi faktor krusial. Kebocoran memori (*memory leak*), stack overflow, dan fragmentasi heap adalah penyebab umum kegagalan sistem yang sulit dideteksi (*hard fault*). Modul ini membahas bagaimana FreeRTOS mengelola RAM dan fitur-fitur lanjutan untuk sinkronisasi kompleks serta throughput data tinggi.

---

## 2. Memory Management

FreeRTOS membutuhkan RAM untuk membuat Task, Queue, Semaphore, dll. Secara default, FreeRTOS menggunakan **Dynamic Memory Allocation** (malloc) untuk kemudahan penggunaan. Namun, tidak seperti OS desktop, FreeRTOS menyediakan fleksibilitas total dalam memilih algoritma alokasi memori melalui file `heap_x.c`.

### A. Dynamic Allocation `pvPortMalloc` & `vPortFree`
Alih-alih `malloc()` dan `free()` standar C, FreeRTOS mendefinisikan layer portabel:
- `pvPortMalloc()`: Alokasi memori.
- `vPortFree()`: Membebaskan memori.
- `xPortGetFreeHeapSize()`: Mengecek sisa heap yang tersedia.
- `xPortGetMinimumEverFreeHeapSize()`: Mengecek sisa heap terendah yang pernah terjadi (penting untuk sizing heap).

### B. Lima Skema Heap (Heap Schemes)
FreeRTOS menyediakan 5 implementasi standar di folder `MemMang`:

#### 1. Heap_1.c
- **Sifat**: Paling sederhana, determistik.
- **Fitur**: Hanya bisa alokasi (`pvPortMalloc`), **TIDAK BISA** membebaskan (`vPortFree` kosong).
- **Penggunaan**: Sistem statis dimana semua task/queue dibuat di `main()` dan tidak pernah dihapus. Cocok untuk *Safety Critical System* karena tidak ada fragmentasi.

#### 2. Heap_2.c
- **Sifat**: Best Fit algorithm.
- **Fitur**: Bisa `malloc` dan `free`.
- **Kekurangan**: Tidak menggabungkan blok yang berdekatan (*No coalescing*). Rentan terhadap **Fragmentasi Berat** jika alokasi/dealokasi acak.
- **Status**: *Deprecated*, digantikan Heap_4.

#### 3. Heap_3.c
- **Sifat**: Wrapper library C standar.
- **Fitur**: Menggunakan `malloc()` dan `free()` bawaan compiler/linker.
- **Kelebihan**: Thread-safe (menghentikan scheduler saat alokasi).
- **Kekurangan**: Ukuran heap tidak deterministik (tergantung linker script).

#### 4. Heap_4.c (Recommended)
- **Sifat**: First Fit algorithm dengan **Coalescing**.
- **Fitur**: Menggabungkan blok kosong yang bersebelahan menjadi blok besar.
- **Kelebihan**: Sangat minim fragmentasi. Bisa menempatkan heap di alamat memori tertentu (misal CCMRAM di STM32 atau PSRAM di ESP32).
- **Penggunaan**: Hampir semua general purpose application.

#### 5. Heap_5.c
- **Sifat**: Sama seperti Heap_4, tapi mendukung **Multiple Memory Regions**.
- **Penggunaan**: Jika RAM terpisah-pisah (misal: Internal RAM + External SDRAM) dan ingin digabungkan menjadi satu heap besar.

---

## 3. Stack Management & Overflow

Setiap Task memiliki Stack sendiri. Ukuran stack ditentukan parameter `usStackDepth` pada `xTaskCreate()`.
- **STM32 (ARM Cortex-M3)**: Satuan stack adalah **Words** (4 bytes). Jadi `xTaskCreate(..., 100, ...)` berarti alokasi 400 bytes.
- **ESP32 (Xtensa)**: Satuan stack adalah **Bytes**. Jadi 100 berarti 100 bytes (Sangat kecil!).

### Deteksi Stack Overflow
Stack overflow terjadi jika variabel lokal atau recursive call melebihi batas stack. Ini merusak data Task lain (korupsi memori).

Untuk mengaktifkan deteksi, set `configCHECK_FOR_STACK_OVERFLOW` di `FreeRTOSConfig.h`:

**Method 1**:
- Cek Stack Pointer saat context switch. Cepat tapi kurang akurat.

**Method 2 (Recommended)**:
- Mengisi seluruh stack dengan pattern `0xA5` saat inisialisasi.
- Cek 20 byte terakhir stack saat context switch. Jika pattern berubah, berarti overflow.

Jika terdeteksi, FreeRTOS memanggil hook function:
```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
```

### High Water Mark
Untuk tuning ukuran stack, gunakan:
```c
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);
```
Nilai ini menunjukkan sisa stack **paling sedikit** yang pernah terjadi sejak task jalan. Semakin dekat ke 0, semakin berisiko.

---

## 4. Static Memory Allocation

Sejak FreeRTOS v9.0, tersedia *Static Allocation* dimana Task, Queue, Semaphore dibuat menggunakan memori yang dialokasikan user (Global/Static Variable), bukan Heap.

Langkah mengaktifkan:
1. Set `configSUPPORT_STATIC_ALLOCATION = 1`.
2. Implementasikan fungsi hook `vApplicationGetIdleTaskMemory()` (dan TimerMemory jika timer aktif) untuk menyediakan memori bagi Idle Task.

Keuntungan:
- **No Fragmentation**: Memori sudah fix.
- **Deterministic**: Tidak mungkin gagal karena "Out of Memory" saat runtime (gagalnya saat compile time).
- **Debugging**: Memori mudah dilihat di Memory View debugger karena punya nama variabel global.

Contoh API:
`xTaskCreateStatic()`, `xQueueCreateStatic()`.

---

## 5. Event Group

Event Group adalah metode sinkronisasi "Many-to-Many" yang ringan. Berbeda dengan Semaphore yang hanya 1 bit, Event Group biasanya 24-bit (tergantung arsitektur) flags.

### Karakteristik
- **Broadcasting**: Satu event (set bit) bisa meng-unblock banyak task sekaligus.
- **Combination**: Task bisa menunggu kombinasi bit (misal: Wait for Bit 0 AND Bit 2).
- **Efisiensi**: Lebih hemat RAM daripada membuat banyak Binary Semaphore.

### Use Case 1: System Init
Menunggu Network Up, File System Ready, dan Sensor Ready sebelum aplikasi jalan.
```c
xEventGroupWaitBits(eg, (NET_BIT | FS_BIT | SENS_BIT), pdTRUE, pdTRUE, portMAX_DELAY);
```

### Use Case 2: Rendezvous (Sync)
Tiga task (A, B, C) harus menyelesaikan tugas masing-masing, lalu sinkronisasi di satu titik sebelum lanjut ke tahap 2 bersama-sama.
GUI: `xEventGroupSync(...)`.

---

## 6. Stream Buffer & Message Buffer

Fitur ini diperkenalkan di FreeRTOS v10.0.0.

### Stream Buffer
- Struktur data untuk metransfer untaian byte (byte stream) dari satu writer ke satu reader.
- Dioptimalkan untuk scenario **ISR to Task**.
- **Tanpa Metadata**: Murni byte raw. Sangat efisien memori.
- Contoh: Menerima data UART/SPI di ISR, dikirim ke Task processing.

### Message Buffer
- Dibangun di atas Stream Buffer.
- Mengirim paket data diskrit (Message) dengan panjang bervariasi.
- Setiap pesan menyertakan header panjang pesan (4 bytes).
- Contoh: Mengirim string log, paket IP, atau struct command berbeda ukuran.

**Perbandingan dengan Queue**:
- Queue: Fixed item size, copy data (berat).
- Stream/Message Buffer: Lebih ringan untuk data besar/variable length.

---
