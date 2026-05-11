# Modul 10: FreeRTOS Advanced

## Capaian Pembelajaran

Setelah menyelesaikan Modul 10, mahasiswa mampu:

1. Menjelaskan konsep lanjutan FreeRTOS: Event Groups, Software Timers, Task Notifications, Semaphore varieties, Mutex dan Priority Inheritance, Memory Management, Static Allocation, Critical Section, Task Suspension, Message Buffers, Stream Buffers, dan Queue Sets.
2. Mengimplementasikan fitur lanjutan FreeRTOS pada ESP32 dengan ESP-IDF/Arduino.
3. Mengimplementasikan fitur lanjutan FreeRTOS pada STM32 dengan STM32Cube HAL.
4. Memilih mekanisme komunikasi/sinkronisasi RTOS yang tepat untuk aplikasi industri.
5. Mengimplementasikan sistem multi-MCU menggunakan objek RTOS lanjutan untuk koordinasi terdistribusi.
6. Mengintegrasikan seluruh konsep menjadi project Advanced RTOS Industrial System.

---

## 1. Event Groups

Event Groups adalah objek FreeRTOS yang menggunakan bit (flags) untuk sinkronisasi antar task. Setiap Event Group memiliki 24 bit (pada port 32-bit) yang dapat diset, cleared, atau ditunggu oleh task.

Fungsi utama:
- `xEventGroupCreate()`: membuat Event Group.
- `xEventGroupSetBits()`: set bit (bisa dari task atau ISR dengan `xEventGroupSetBitsFromISR()`).
- `xEventGroupWaitBits()`: tunggu bit tertentu (bisa tunggu semua bit atau salah satu bit).
- `xEventGroupClearBits()`: clear bit manual.

Keunggulan:
- Dapat sinkronisasi multiple event dalam satu objek.
- Mendukung tunggu kombinasi bit (AND/OR logic).
- Lebih ringan dari multiple semaphore untuk sinkronisasi banyak task.

Contoh kasus: TaskA set bit 0 saat button ditekan, TaskB tunggu bit 0 lalu eksekusi, TaskC tunggu bit 0 dan bit 1 sebelum eksekusi.

---

## 2. Software Timers

Software Timer adalah timer yang dikelola FreeRTOS kernel, berjalan di task daemon timer (prioritas dikonfigurasi di `configTIMER_TASK_PRIORITY`).

Fungsi utama:
- `xTimerCreate()`: membuat timer (one-shot atau periodik).
- `xTimerStart()`: mulai timer.
- `xTimerStop()`: hentikan timer.
- `xTimerChangePeriod()`: ubah periode timer.

Keunggulan:
- Non-blocking, tidak memakan CPU saat menunggu.
- Satu task daemon mengelola semua software timer.
- Dapat digunakan untuk eksekusi berkala atau penundaan satu kali.

Perbedaan dengan task periodik: Software Timer tidak memiliki state sendiri, sedangkan task periodik memiliki stack dan kontrol penuh atas eksekusi.

---

## 3. Task Notifications

Task Notifications adalah mekanisme komunikasi antar task paling ringan di FreeRTOS, menggunakan struktur pada TCB task tujuan (tanpa objek eksplisit baru).

Fungsi utama:
- `xTaskNotifyGive()`: kirim notifikasi (increment count, mirip semaphore).
- `ulTaskNotifyTake()`: terima notifikasi (decrement count).
- `xTaskNotify()`: kirim notifikasi dengan nilai 32-bit.
- `xTaskNotifyWait()`: tunggu notifikasi dengan nilai 32-bit.

Keunggulan:
- 45% lebih cepat dan 45% lebih hemat memory dibandingkan semaphore/queue.
- Tidak perlu membuat objek FreeRTOS tambahan.
- Mendukung pengiriman nilai 32-bit antar task.

Keterbatasan: hanya bisa mengirim notifikasi ke task tertentu (unicast, bukan broadcast seperti event groups).

---

## 4. Semaphore Varieties

FreeRTOS menyediakan 3 jenis semaphore:

### Binary Semaphore
- Nilai 0 atau 1.
- Digunakan untuk sinkronisasi (misal: ISR memberi semaphore, task mengambilnya).
- Tidak memiliki mekanisme Priority Inheritance.

### Counting Semaphore
- Nilai 0 hingga max count (ditentukan saat create).
- Digunakan untuk mengontrol akses ke multiple resource identik (misal: 3 slot resource, counting semaphore max 3).
- Fungsi: `xSemaphoreCreateCounting()`.

### Mutex
- Mirip Binary Semaphore, tetapi memiliki mekanisme Priority Inheritance.
- Digunakan untuk akses shared resource (mencegah priority inversion).
- Hanya owner mutex yang boleh mengembalikan (give) mutex.

Fungsi umum:
- `xSemaphoreCreateBinary()`
- `xSemaphoreCreateMutex()`
- `xSemaphoreTake()`
- `xSemaphoreGive()`

---

## 5. Mutex and Priority Inheritance

Priority Inversion terjadi ketika task low priority memegang mutex, task high priority menunggu mutex, dan task medium priority menunda task low priority sehingga task high priority ikut tertunda.

Mutex FreeRTOS menggunakan Priority Inheritance: saat task high priority menunggu mutex yang dipegang task low priority, prioritas task low priority dinaikkan ke prioritas task high priority sementara hingga mutex dikembalikan. Ini mencegah task medium priority menunda task low priority saat memegang mutex.

Contoh:
1. Task Low (prio 1) ambil mutex.
2. Task High (prio 3) coba ambil mutex → tertunda, prioritas Task Low dinaikkan ke 3.
3. Task Medium (prio 2) siap jalan → tidak bisa jalan karena prioritas Task Low sekarang 3 (lebih tinggi dari Medium).
4. Task Low selesai, kembalikan mutex → prioritas Task Low kembali ke 1, Task High jalan.

---

## 6. Memory Management

FreeRTOS menyediakan 5 jenis heap management (file `heap_1.c` hingga `heap_5.c`):

| Heap | Alokasi | Free | Fragmentasi | Kasus Penggunaan |
|---|---|---|---|---|
| heap_1 | Dinamis | Tidak bisa | Tidak ada | Sistem yang hanya alokasi sekali saat startup |
| heap_2 | Dinamis | Bisa | Tinggi | Sistem lama, tidak direkomendasi |
| heap_3 | Wrapper malloc/free | Bisa | Tergantung libc | Sistem dengan heap standar, thread-safe |
| heap_4 | Dinamis dengan coalescence | Bisa | Rendah | Umum, direkomendasi untuk sebagian besar aplikasi |
| heap_5 | Multiple memory region | Bisa | Rendah |STM32 dengan RAM terpisah (SRAM1, SRAM2, CCMRAM) |

Fungsi terkait:
- `pvPortMalloc()`: alokasi memory.
- `vPortFree()`: bebaskan memory.
- `xPortGetFreeHeapSize()`: cek sisa heap.
- `xPortGetMinimumEverFreeHeapSize()`: cek heap minimum pernah tersisa.

---

## 7. Static Allocation

Static Allocation membuat objek FreeRTOS (task, queue, semaphore, timer) dengan memory yang dialokasikan sebelumnya (array statis), bukan dari heap dinamis. Cocok untuk sistem deterministik yang tidak ingin fragmentasi heap.

Langkah membuat task statis:
1. Deklarasi `StaticTask_t xTaskBuffer;` dan `StackType_t xStack[STACK_SIZE];`.
2. Buat task dengan `xTaskCreateStatic()`:
```c
TaskHandle_t xTaskCreateStatic(
    TaskFunction_t pxTaskCode,
    const char * const pcName,
    uint32_t ulStackDepth,
    void *pvParameters,
    UBaseType_t uxPriority,
    StackType_t *puxStackBuffer,
    StaticTask_t *pxTaskBuffer
);
```

Objek lain (queue, semaphore, timer) juga memiliki versi `CreateStatic()` dengan parameter buffer statis.

---

## 8. Critical Section dan Task Suspension

### Critical Section

Critical Section adalah mekanisme paling ringan untuk proteksi data bersama — mematikan scheduler (dan/atau interrupt) sementara agar operasi berjalan secara atomis.

```c
// STM32 / ESP32
taskENTER_CRITICAL();       // nonaktifkan scheduler/interrupt
shared_counter++;           // operasi atomis
taskEXIT_CRITICAL();        // aktifkan kembali

// ISR-safe version (simpan/pulihkan interrupt mask):
taskENTER_CRITICAL_FROM_ISR();
// ... operasi atomis ...
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
```

Kapan Critical Section vs Mutex:
- Critical Section: operasi sangat singkat (increment, flag set, read-modify-write). Blokir semua interrupt, tidak boleh memanggil RTOS API di dalamnya.
- Mutex: operasi lebih lama, perlu memanggil RTOS API, atau butuh Priority Inheritance. Tidak blokir interrupt.

### Task Suspension dan Resume

```c
vTaskSuspend(xTaskHandle);   // suspend task tertentu (NULL = self)
vTaskResume(xTaskHandle);    // resume task dari task lain
vTaskResumeFromISR(xHandle); // resume dari ISR
```

Task yang disuspend tidak menggunakan CPU dan tidak masuk scheduling. Berguna untuk pause task saat tidak diperlukan.

---

## 9. Message Buffers

Message Buffer digunakan untuk transfer data dengan panjang variabel antar task atau antara ISR dan task. Setiap pengiriman (send) menulis pesan dengan header panjang pesan otomatis, sehingga penerima tahu panjang pesan yang diterima.

Fungsi utama:
- `xMessageBufferCreate()`: buat message buffer (parameter ukuran total buffer).
- `xMessageBufferSend()`: kirim pesan (bisa dari ISR dengan `xMessageBufferSendFromISR()`).
- `xMessageBufferReceive()`: terima pesan.

Keunggulan:
- Otomatis mendeteksi panjang pesan (tidak perlu kirim panjang terpisah).
- Dapat digunakan untuk pesan 1 byte hingga ukuran buffer penuh.
- Thread-safe untuk ISR dan task.

---

## 9. Stream Buffers

Stream Buffer digunakan untuk transfer byte stream (data kontinu) antar task atau antara ISR dan task. Berbeda dengan Message Buffer, Stream Buffer tidak menyimpan panjang pesan, hanya byte mentah.

Fungsi utama:
- `xStreamBufferCreate()`: buat stream buffer (parameter ukuran total, trigger level).
- `xStreamBufferSend()`: kirim byte stream (bisa dari ISR).
- `xStreamBufferReceive()`: terima byte stream.
- `xStreamBufferBytesAvailable()`: cek byte tersedia.
- `xStreamBufferSpacesAvailable()`: cek sisa ruang buffer.

Keunggulan:
- Cocok untuk transfer data serial, audio, atau sensor stream.
- Dapat diatur trigger level (task baru bangun saat minimal X byte tersedia).
- Lebih efisien untuk data stream dibanding Message Buffer.

---

## 10. Queue Sets

Queue Set memungkinkan satu task menunggu event dari multiple queue, semaphore, atau mutex dalam satu panggilan (tidak perlu polling masing-masing objek).

Langkah penggunaan:
1. Buat Queue Set dengan `xQueueCreateSet()` (parameter ukuran total semua objek yang ditambahkan).
2. Tambahkan objek (queue, semaphore) ke Queue Set dengan `xQueueAddToSet()`.
3. Task menunggu event dari Queue Set dengan `xQueueSelectFromSet()`.
4. Setelah event terdeteksi, baca objek yang aktif seperti biasa.

Keunggulan:
- Menghindari polling multiple objek (menghemat CPU).
- Satu task dapat menangani banyak sumber event.
- Cocok untuk task gateway yang menerima input dari banyak sumber.

---

## 11. Struktur Praktikum Final Modul 10

Praktikum final berisi tepat **25 eksperimen**:

### 10 Eksperimen STM32

| No | Kode | Topik | Objek RTOS |
|---|---|---|---|
| 1 | STM32_01 | Event Groups Basic | Event Group |
| 2 | STM32_02 | Software Timers | Software Timer |
| 3 | STM32_03 | Task Notifications | Task Notification |
| 4 | STM32_04 | Semaphore Varieties | Binary, Counting Semaphore |
| 5 | STM32_05 | Mutex dan Priority Inheritance | Mutex |
| 6 | STM32_06 | Memory Management | heap_1-heap_5 |
| 7 | STM32_07 | Static Allocation | Static Task/Queue |
| 8 | STM32_08 | Critical Section dan Task Suspension | Critical Section, vTaskSuspend |
| 9 | STM32_09 | Message Buffers dan Stream Buffers | Message Buffer, Stream Buffer |
| 10 | STM32_10 | Queue Sets Multiplexing | Queue Set |

### 10 Eksperimen ESP32

| No | Kode | Topik | Objek RTOS |
|---|---|---|---|
| 1 | ESP32_01 | Event Groups Basic | Event Group |
| 2 | ESP32_02 | Software Timers | Software Timer |
| 3 | ESP32_03 | Task Notifications | Task Notification |
| 4 | ESP32_04 | Semaphore Varieties | Binary, Counting Semaphore |
| 5 | ESP32_05 | Mutex dan Priority Inheritance | Mutex |
| 6 | ESP32_06 | Memory Management | heap_1-heap_5 |
| 7 | ESP32_07 | Static Allocation | Static Task/Queue |
| 8 | ESP32_08 | Critical Section dan Task Suspension | Critical Section, vTaskSuspend |
| 9 | ESP32_09 | Message Buffers dan Stream Buffers | Message Buffer, Stream Buffer |
| 10 | ESP32_10 | Queue Sets Multiplexing | Queue Set |

### 5 Eksperimen Multi STM32-ESP32

| No | Kode | Topik | Fokus RTOS |
|---|---|---|---|
| 1 | MULTI_01 | Distributed Event Group Sync | Event Group + UART protocol |
| 2 | MULTI_02 | Multi-MCU Software Timer Network | Software Timer + remote control |
| 3 | MULTI_03 | Task Notification Pipeline + Message Buffer | Task Notification + Message Buffer |
| 4 | MULTI_04 | Priority Inheritance + Critical Section Network | Mutex + Critical Section |
| 5 | MULTI_05 | Full Advanced Industrial RTOS System | Semua objek RTOS terintegrasi |

Ringkasan fokus:

| Kelompok | Jumlah | Fokus |
|---|---:|---|
| STM32 | 10 | STM32Cube HAL, FreeRTOS lanjutan, priority inheritance, heap, critical section |
| ESP32 | 10 | ESP-IDF/Arduino, FreeRTOS lanjutan, task notification, stream/message buffer |
| Multi | 5 | Koordinasi multi-MCU dengan objek RTOS lanjutan, protokol UART |

---

## 12. Referensi

1. FreeRTOS Official Documentation — Event Groups, Software Timers, Task Notifications, Semaphores, Mutex, Memory Management, Static Allocation, Critical Sections, Message/Stream Buffers, Queue Sets.
2. Espressif ESP-IDF Programming Guide — FreeRTOS on ESP32.
3. STM32Cube Reference Manual — FreeRTOS integration with STM32 HAL.
4. Book: "Mastering the FreeRTOS Real Time Kernel" by Richard Barry.
