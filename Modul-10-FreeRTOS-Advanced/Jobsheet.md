# Jobsheet Modul 10: FreeRTOS Advanced

## 1. Tujuan Praktikum

Setelah menyelesaikan jobsheet Modul 10, mahasiswa mampu:

1. Mengimplementasikan fitur lanjutan FreeRTOS pada ESP32 dan STM32 meliputi Event Groups, Software Timers, Task Notifications, Semaphore varieties, Mutex dan Priority Inheritance, Memory Management, Static Allocation, Message Buffers, Stream Buffers, dan Queue Sets.
2. Memahami perbedaan mekanisme komunikasi dan sinkronisasi RTOS lanjutan serta kasus penggunaannya.
3. Mengimplementasikan tepat 20 eksperimen final: **10 STM32 + 10 ESP32** dengan topik FreeRTOS lanjutan.
4. Mengintegrasikan seluruh konsep menjadi project Advanced RTOS Industrial System.

---

## 2. Peralatan

| Komponen | Jumlah | Catatan |
|---|---:|---|
| ESP32 DevKit | 1 | FreeRTOS via ESP-IDF/Arduino; dukungan task notification, event groups, stream/message buffer |
| STM32 Blue Pill/Black Pill | 1 | FreeRTOS via STM32Cube HAL; dukungan semua fitur RTOS lanjutan |
| LED | 3 | indikator status task/event |
| Push button | 2 | trigger event/task |
| Potensiometer | 1 | simulasi input data |
| Breadboard, jumper, USB | sesuai | common ground wajib |
| PC dengan IDE | 1 | STM32CubeIDE/Arduino/ESP-IDF |

---

## 3. Aturan Umum Wiring

### ESP32

| Sinyal | Default | Catatan |
|---|---|---|
| SDA/SCL | GPIO21/GPIO22 | tidak digunakan pada eksperimen RTOS murni, hanya jika butuh sensor eksternal |
| GPIO Input | GPIO34-39 | untuk push button/potensiometer |
| GPIO Output | GPIO2, GPIO4, GPIO5 | untuk LED indikator |

### STM32

| Sinyal | I2C1 | Catatan |
|---|---|---|
| GPIO Output | PB0, PB1, PB2 | LED indikator |
| GPIO Input | PA0, PA1 | push button/potensiometer |

Aturan:
- Semua GND disatukan.
- Gunakan resistor pull-up 10 kΩ untuk push button jika diperlukan.
- Pastikan prioritas task dan interrupt sesuai konsep RTOS.

---

## 4. Daftar Final 20 Eksperimen

| No | Kode | Kelompok | Judul |
|---:|---|---|---|
| 1 | STM32_01 | STM32 | Event Groups Basic |
| 2 | STM32_02 | STM32 | Software Timers |
| 3 | STM32_03 | STM32 | Task Notifications |
| 4 | STM32_04 | STM32 | Semaphore Varieties (Binary, Counting) |
| 5 | STM32_05 | STM32 | Mutex and Priority Inheritance |
| 6 | STM32_06 | STM32 | Memory Management (heap_1-heap_5) |
| 7 | STM32_07 | STM32 | Static Allocation |
| 8 | STM32_08 | STM32 | Message Buffers |
| 9 | STM32_09 | STM32 | Stream Buffers |
| 10 | STM32_10 | STM32 | Queue Sets |
| 11 | ESP32_01 | ESP32 | Event Groups Basic |
| 12 | ESP32_02 | ESP32 | Software Timers |
| 13 | ESP32_03 | ESP32 | Task Notifications |
| 14 | ESP32_04 | ESP32 | Semaphore Varieties (Binary, Counting) |
| 15 | ESP32_05 | ESP32 | Mutex and Priority Inheritance |
| 16 | ESP32_06 | ESP32 | Memory Management (heap_1-heap_5) |
| 17 | ESP32_07 | ESP32 | Static Allocation |
| 18 | ESP32_08 | ESP32 | Message Buffers |
| 19 | ESP32_09 | ESP32 | Stream Buffers |
| 20 | ESP32_10 | ESP32 | Queue Sets |

---

## 5. Eksperimen STM32

### STM32_01 — Event Groups Basic

**Tujuan:** menggunakan Event Groups untuk sinkronisasi antar task dengan flag bit.

**Hardware:** STM32, 2 LED, 1 push button.

**Wiring:** LED PB0, PB1; Button PA0 (pull-up).

**Langkah:**
1. Buat 2 task: TaskA (set event bit 0 saat button ditekan), TaskB (menunggu event bit 0 lalu toggle LED PB0).
2. Init Event Group dengan `xEventGroupCreate()`.
3. Gunakan `xEventGroupSetBits()` dan `xEventGroupWaitBits()` dengan timeout portMAX_DELAY.
4. Uji dengan menekan button dan amati LED.
5. Tambahkan event bit 1 untuk task ketiga dengan LED PB1.

**Output diharapkan:** LED berubah status saat button ditekan, sesuai sinkronisasi event bit.

---

### STM32_02 — Software Timers

**Tujuan:** membuat Software Timer FreeRTOS untuk eksekusi berkala non-blocking.

**Hardware:** STM32, 1 LED.

**Wiring:** LED PB0.

**Langkah:**
1. Buat Software Timer dengan `xTimerCreate()` untuk toggle LED setiap 1 detik.
2. Start timer dengan `xTimerStart()`.
3. Buat task tambahan yang tidak memblokir CPU.
4. Ukur jeda waktu toggle LED dengan serial monitor.
5. Uji timer satu kali (one-shot) dan periodik.

**Output diharapkan:** LED toggle tepat setiap 1 detik tanpa blocking task lain.

---

### STM32_03 — Task Notifications

**Tujuan:** menggunakan Task Notifications untuk komunikasi antar task yang lebih ringan daripada queue/semaphore.

**Hardware:** STM32, 1 LED, 1 push button.

**Wiring:** LED PB0; Button PA0.

**Langkah:**
1. Buat TaskA (menunggu notifikasi dengan `ulTaskNotifyTake()`), TaskB (mengirim notifikasi dengan `xTaskNotifyGive()` saat button ditekan).
2. Toggle LED saat notifikasi diterima.
3. Bandingkan dengan penggunaan binary semaphore untuk efisiensi.
4. Uji dengan menekan button berulang.
5. Catat waktu respon task.

**Output diharapkan:** LED toggle saat button ditekan, respon lebih cepat dibanding semaphore.

---

### STM32_04 — Semaphore Varieties (Binary, Counting)

**Tujuan:** membedakan penggunaan Binary Semaphore dan Counting Semaphore.

**Hardware:** STM32, 2 LED, 1 push button.

**Wiring:** LED PB0, PB1; Button PA0.

**Langkah:**
1. Buat Binary Semaphore untuk sinkronisasi button ke task toggle LED PB0.
2. Buat Counting Semaphore dengan max count 3 untuk mengontrol LED PB1 (hanya nyala jika count > 0).
3. Button menambah count semaphore (panic button simulasi).
4. Task consumer mengurangi count setiap 2 detik.
5. Amati perbedaan perilaku kedua semaphore.

**Output diharapkan:** Binary semaphore sinkronisasi 1:1; Counting semaphore mengizinkan multiple resource.

---

### STM32_05 — Mutex and Priority Inheritance

**Tujuan:** memahami Mutex untuk shared resource dan mekanisme Priority Inheritance mencegah priority inversion.

**Hardware:** STM32, 2 LED.

**Wiring:** LED PB0 (low priority task), PB1 (high priority task).

**Langkah:**
1. Buat Mutex dengan `xSemaphoreCreateMutex()` untuk akses shared resource (serial print).
2. Buat 3 task dengan prioritas Low, Medium, High.
3. Low task mengambil mutex, lalu High task mencoba mengambil mutex yang sama.
4. Amati Priority Inheritance: Medium task tidak boleh menunda High task saat Low memegang mutex.
5. Bandingkan dengan Binary Semaphore (tanpa priority inheritance).

**Output diharapkan:** High task langsung menunda Medium task saat Low memegang mutex (priority inheritance bekerja).

---

### STM32_06 — Memory Management (heap_1-heap_5)

**Tujuan:** memahami 5 jenis heap FreeRTOS dan memilih heap sesuai aplikasi.

**Hardware:** STM32, serial monitor.

**Wiring:** hanya koneksi USB serial.

**Langkah:**
1. Uji heap_1 (alokasi statis, tidak bisa free).
2. Uji heap_2 (bisa free, tapi fragmentasi tinggi).
3. Uji heap_3 (wrapper malloc/free standar, thread-safe).
4. Uji heap_4 (coalescence, kurangi fragmentasi).
5. Uji heap_5 (multiple memory region, untuk STM32 dengan RAM terpisah).
6. Bandingkan fragmentasi dan sisa heap dengan `xPortGetFreeHeapSize()`.

**Output diharapkan:** laporan penggunaan heap, fragmentasi, dan kasus penggunaan masing-masing heap.

---

### STM32_07 — Static Allocation

**Tujuan:** membuat task, queue, semaphore dengan alokasi statis (tanpa heap dinamis).

**Hardware:** STM32, 1 LED.

**Wiring:** LED PB0.

**Langkah:**
1. Alokasikan memory statis untuk TCB task dan stack dengan array `StaticTask_t` dan `StackType_t`.
2. Buat task dengan `xTaskCreateStatic()`.
3. Buat queue statis dengan `xQueueCreateStatic()`.
4. Kirim data dari taskA ke taskB via queue statis.
5. Toggle LED saat data diterima.

**Output diharapkan:** sistem berjalan tanpa alokasi dinamis, cocok untuk sistem deterministik.

---

### STM32_08 — Message Buffers

**Tujuan:** menggunakan Message Buffer untuk transfer data bervariasi ukuran antar task/ISR.

**Hardware:** STM32, 1 LED, 1 push button.

**Wiring:** LED PB0; Button PA0.

**Langkah:**
1. Buat Message Buffer dengan `xMessageBufferCreate()`.
2. TaskA mengirim pesan variabel (string pendek/panjang) saat button ditekan.
3. TaskB menerima pesan dengan `xMessageBufferReceive()` dan print ke serial.
4. Uji dengan pesan 1 byte hingga 128 byte.
5. Bandingkan dengan Stream Buffer.

**Output diharapkan:** pesan variabel diterima utuh, panjang pesan otomatis terdeteksi.

---

### STM32_09 — Stream Buffers

**Tujuan:** menggunakan Stream Buffer untuk transfer byte stream antar task/ISR.

**Hardware:** STM32, serial monitor.

**Wiring:** USB serial.

**Langkah:**
1. Buat Stream Buffer dengan `xStreamBufferCreate()`.
2. TaskA menulis byte stream (simulasi data sensor) ke buffer.
3. TaskB membaca stream dan rekonstruksi data.
4. Uji dengan kecepatan tulis 100 byte/detik dan 1000 byte/detik.
5. Amati overrun/underrun buffer.

**Output diharapkan:** stream byte utuh diterima, tidak ada byte hilang jika buffer cukup.

---

### STM32_10 — Queue Sets

**Tujuan:** menggunakan Queue Set untuk memantau multiple queue/semaphore dalam satu task.

**Hardware:** STM32, 2 LED, 1 push button.

**Wiring:** LED PB0, PB1; Button PA0.

**Langkah:**
1. Buat 2 queue dan 1 binary semaphore.
2. Buat Queue Set dengan `xQueueCreateSet()` dan tambahkan ketiga objek.
3. Task menunggu event dari Queue Set dengan `xQueueSelectFromSet()`.
4. Trigger semaphore (button), kirim data ke queue1 dan queue2.
5. Task menangani event sesuai objek yang aktif.

**Output diharapkan:** task menangani multiple queue/semaphore tanpa polling per objek.

---

## 6. Eksperimen ESP32

### ESP32_01 — Event Groups Basic

**Tujuan:** menggunakan Event Groups pada ESP32 FreeRTOS untuk sinkronisasi task.

**Hardware:** ESP32, 2 LED, 1 push button.

**Wiring:** LED GPIO2, GPIO4; Button GPIO34 (input).

**Langkah:**
1. Buat 2 task: TaskA (set event bit 0 saat button ditekan), TaskB (wait event bit 0 toggle LED GPIO2).
2. Init Event Group dengan `xEventGroupCreate()`.
3. Gunakan `xEventGroupSetBits()` dan `xEventGroupWaitBits()`.
4. Uji dengan button dan amati LED.
5. Tambahkan event bit 1 untuk LED GPIO4.

**Output diharapkan:** LED berubah status sesuai event bit yang diset.

---

### ESP32_02 — Software Timers

**Tujuan:** membuat Software Timer pada ESP32 FreeRTOS untuk eksekusi periodik.

**Hardware:** ESP32, 1 LED.

**Wiring:** LED GPIO2.

**Langkah:**
1. Buat Software Timer dengan `xTimerCreate()` toggle LED setiap 1 detik.
2. Start timer dengan `xTimerStart()`.
3. Buat task tambahan non-blocking.
4. Ukur jeda toggle LED.
5. Uji one-shot dan periodik timer.

**Output diharapkan:** LED toggle tepat 1 detik tanpa blocking task lain.

---

### ESP32_03 — Task Notifications

**Tujuan:** menggunakan Task Notifications pada ESP32 untuk komunikasi ringan antar task.

**Hardware:** ESP32, 1 LED, 1 push button.

**Wiring:** LED GPIO2; Button GPIO34.

**Langkah:**
1. Buat TaskA (wait notification `ulTaskNotifyTake()`), TaskB (send notification `xTaskNotifyGive()` saat button ditekan).
2. Toggle LED saat notifikasi diterima.
3. Bandingkan dengan binary semaphore.
4. Uji respon time.
5. Catat penggunaan memory.

**Output diharapkan:** LED toggle saat button ditekan, lebih efisien dari semaphore.

---

### ESP32_04 — Semaphore Varieties (Binary, Counting)

**Tujuan:** membedakan Binary dan Counting Semaphore pada ESP32.

**Hardware:** ESP32, 2 LED, 1 push button.

**Wiring:** LED GPIO2, GPIO4; Button GPIO34.

**Langkah:**
1. Binary Semaphore untuk sinkronisasi button ke LED GPIO2.
2. Counting Semaphore max count 3 untuk LED GPIO4.
3. Button tambah count semaphore.
4. Task consumer kurangi count setiap 2 detik.
5. Amati perbedaan perilaku.

**Output diharapkan:** Binary semaphore 1:1 sync; Counting semaphore multiple resource.

---

### ESP32_05 — Mutex and Priority Inheritance

**Tujuan:** memahami Mutex dan Priority Inheritance pada ESP32.

**Hardware:** ESP32, 2 LED.

**Wiring:** LED GPIO2 (low priority), GPIO4 (high priority).

**Langkah:**
1. Buat Mutex dengan `xSemaphoreCreateMutex()` untuk shared serial.
2. 3 task: Low, Medium, High priority.
3. Low ambil mutex, High coba ambil mutex.
4. Amati Priority Inheritance: Medium tidak menunda High.
5. Bandingkan dengan Binary Semaphore.

**Output diharapkan:** High task langsung jalan saat Low pegang mutex (priority inheritance).

---

### ESP32_06 — Memory Management (heap_1-heap_5)

**Tujuan:** memahami heap FreeRTOS pada ESP32.

**Hardware:** ESP32, serial monitor.

**Wiring:** USB serial.

**Langkah:**
1. Uji heap_1 sampai heap_5 (ESP32 default heap_4).
2. Bandingkan fragmentasi dengan `xPortGetFreeHeapSize()`.
3. Alokasi dan free memory berulang.
4. Catat sisa heap tiap operasi.
5. Pilih heap sesuai aplikasi.

**Output diharapkan:** laporan penggunaan heap ESP32 dan rekomendasi heap.

---

### ESP32_07 — Static Allocation

**Tujuan:** alokasi statis task/queue pada ESP32.

**Hardware:** ESP32, 1 LED.

**Wiring:** LED GPIO2.

**Langkah:**
1. Alokasi statis TCB dan stack dengan `StaticTask_t` dan `StackType_t`.
2. Buat task dengan `xTaskCreateStatic()`.
3. Buat queue statis `xQueueCreateStatic()`.
4. Kirim data taskA ke taskB via queue statis.
5. Toggle LED saat data diterima.

**Output diharapkan:** sistem berjalan tanpa alokasi dinamis.

---

### ESP32_08 — Message Buffers

**Tujuan:** menggunakan Message Buffer pada ESP32.

**Hardware:** ESP32, 1 LED, 1 push button.

**Wiring:** LED GPIO2; Button GPIO34.

**Langkah:**
1. Buat Message Buffer `xMessageBufferCreate()`.
2. TaskA kirim pesan variabel saat button ditekan.
3. TaskB terima pesan `xMessageBufferReceive()` print serial.
4. Uji pesan 1-128 byte.
5. Bandingkan dengan Stream Buffer.

**Output diharapkan:** pesan variabel diterima utuh.

---

### ESP32_09 — Stream Buffers

**Tujuan:** menggunakan Stream Buffer pada ESP32.

**Hardware:** ESP32, serial monitor.

**Wiring:** USB serial.

**Langkah:**
1. Buat Stream Buffer `xStreamBufferCreate()`.
2. TaskA tulis byte stream simulasi sensor.
3. TaskB baca stream rekonstruksi data.
4. Uji 100 byte/detik dan 1000 byte/detik.
5. Amati overrun/underrun.

**Output diharapkan:** stream byte utuh diterima.

---

### ESP32_10 — Queue Sets

**Tujuan:** menggunakan Queue Set pada ESP32 untuk multiple queue/semaphore.

**Hardware:** ESP32, 2 LED, 1 push button.

**Wiring:** LED GPIO2, GPIO4; Button GPIO34.

**Langkah:**
1. Buat 2 queue, 1 binary semaphore.
2. Buat Queue Set `xQueueCreateSet()` tambahkan objek.
3. Task tunggu event `xQueueSelectFromSet()`.
4. Trigger semaphore, kirim data queue1/queue2.
5. Tangani event sesuai objek aktif.

**Output diharapkan:** task tangani multiple objek tanpa polling.

---

## 7. Format Pengamatan Minimal

Untuk tiap eksperimen, catat:

| Item | Nilai |
|---|---|
| Kode eksperimen | |
| Platform | STM32 / ESP32 |
| Objek RTOS | |
| Wiring | |
| Output serial/LED | |
| Error yang muncul | |
| Solusi | |

---

## 8. Pertanyaan Analisis Wajib

1. Apa perbedaan Event Groups dan Semaphore untuk sinkronisasi task?
2. Kapan menggunakan Software Timer dibandingkan task periodik biasa?
3. Mengapa Task Notifications lebih ringan dari Semaphore/Queue?
4. Apa perbedaan Binary Semaphore dan Mutex?
5. Bagaimana Priority Inheritance mencegah priority inversion?
6. Kapan memilih heap_1 dibandingkan heap_4?
7. Apa keuntungan Static Allocation untuk sistem real-time?
8. Perbedaan Message Buffer dan Stream Buffer?
9. Kapan menggunakan Queue Set?
10. Bagaimana memilih objek komunikasi RTOS yang tepat untuk aplikasi industri?

---

## 9. Deliverable Laporan

1. Cover dan identitas.
2. Ringkasan teori FreeRTOS Advanced Modul 10.
3. Tabel hasil **20 eksperimen**.
4. Foto wiring tiap kelompok eksperimen.
5. Screenshot serial/LED.
6. Analisis error dan troubleshooting.
7. Source code utama atau link repository.
8. Integrasi project Advanced RTOS Industrial System.
9. Kesimpulan.
