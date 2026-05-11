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

## 4. Daftar Final 25 Eksperimen

| No | Kode | Kelompok | Judul |
|---:|---|---|---|
| 1 | STM32_01 | STM32 | Event Groups Basic |
| 2 | STM32_02 | STM32 | Software Timers |
| 3 | STM32_03 | STM32 | Task Notifications |
| 4 | STM32_04 | STM32 | Semaphore Varieties (Binary, Counting) |
| 5 | STM32_05 | STM32 | Mutex dan Priority Inheritance |
| 6 | STM32_06 | STM32 | Memory Management (heap_1-heap_5) |
| 7 | STM32_07 | STM32 | Static Allocation |
| 8 | STM32_08 | STM32 | Critical Section dan Task Suspension |
| 9 | STM32_09 | STM32 | Message Buffers dan Stream Buffers |
| 10 | STM32_10 | STM32 | Queue Sets Multiplexing |
| 11 | ESP32_01 | ESP32 | Event Groups Basic |
| 12 | ESP32_02 | ESP32 | Software Timers |
| 13 | ESP32_03 | ESP32 | Task Notifications |
| 14 | ESP32_04 | ESP32 | Semaphore Varieties (Binary, Counting) |
| 15 | ESP32_05 | ESP32 | Mutex dan Priority Inheritance |
| 16 | ESP32_06 | ESP32 | Memory Management (heap_1-heap_5) |
| 17 | ESP32_07 | ESP32 | Static Allocation |
| 18 | ESP32_08 | ESP32 | Critical Section dan Task Suspension |
| 19 | ESP32_09 | ESP32 | Message Buffers dan Stream Buffers |
| 20 | ESP32_10 | ESP32 | Queue Sets Multiplexing |
| 21 | MULTI_01 | Multi | Distributed Event Group Synchronization |
| 22 | MULTI_02 | Multi | Multi-MCU Software Timer Network |
| 23 | MULTI_03 | Multi | Task Notification Pipeline + Message Buffer |
| 24 | MULTI_04 | Multi | Priority Inheritance + Critical Section Network |
| 25 | MULTI_05 | Multi | Full Advanced Industrial RTOS System |

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

### STM32_08 — Critical Section dan Task Suspension

**Tujuan:** memahami Critical Section untuk proteksi data bersama dan Task Suspension/Resume untuk kontrol eksekusi task.

**Hardware:** STM32, 2 LED, serial monitor.

**Wiring:** LED PB0 (task A), PB1 (task B).

**Langkah:**
1. Buat 2 task yang mengakses variabel global `shared_counter` secara bersamaan.
2. Demonstrasikan race condition tanpa proteksi (tampilkan nilai yang salah).
3. Gunakan `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` untuk proteksi atomik.
4. Bandingkan hasil dengan mutex (critical section lebih cepat tapi blokir interrupt).
5. Implementasikan `vTaskSuspend()` / `vTaskResume()` untuk kontrol eksekusi task dari task lain.

**Output diharapkan:** race condition terlihat tanpa proteksi; critical section memastikan data konsisten; task A dapat suspend/resume task B via button.

---

### STM32_09 — Message Buffers dan Stream Buffers

**Tujuan:** menggunakan Message Buffer untuk transfer data variabel dan Stream Buffer untuk byte stream kontinu.

**Hardware:** STM32, 1 LED, 1 push button, serial monitor.

**Wiring:** LED PB0; Button PA0.

**Langkah:**
1. Buat Message Buffer dengan `xMessageBufferCreate(256)`. Task A kirim pesan panjang variabel (1–128 byte) saat button ditekan. Task B terima dengan `xMessageBufferReceive()` dan print ke serial.
2. Buat Stream Buffer dengan `xStreamBufferCreate(512, 10)`. Task A tulis byte stream simulasi sensor (100 byte/100ms). Task B baca stream dengan trigger level dan rekonstruksi data.
3. Bandingkan: Message Buffer menyimpan header panjang otomatis; Stream Buffer hanya byte mentah.
4. Uji overrun: isi stream buffer melebihi kapasitas, amati perilaku.

**Output diharapkan:** Message Buffer menerima pesan utuh dengan panjang terdeteksi; Stream Buffer menerima byte stream kontinu; perbedaan perilaku keduanya terdokumentasi.

---

### STM32_10 — Queue Sets Multiplexing

**Tujuan:** menggunakan Queue Set untuk memantau multiple queue dan semaphore dalam satu task tanpa polling.

**Hardware:** STM32, 2 LED, 1 push button, serial monitor.

**Wiring:** LED PB0, PB1; Button PA0.

**Langkah:**
1. Buat 2 queue (`xDataQueue`, `xStatusQueue`) dan 1 binary semaphore (`xButtonSemaphore`).
2. Buat Queue Set dengan `xQueueCreateSet(COMBINED_LENGTH)` dan tambahkan ketiga objek dengan `xQueueAddToSet()`.
3. Task generator: isi `xDataQueue` tiap 1 detik, `xStatusQueue` tiap 2 detik.
4. ISR button berikan `xButtonSemaphore`.
5. Task monitor: `xQueueSelectFromSet()` identifikasi objek aktif dan proses sesuai sumber.

**Output diharapkan:** task monitor menangani semua event tanpa polling; identitas sumber event (Data/Status/Button) ditampilkan; LED berubah sesuai event.

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

### ESP32_08 — Critical Section dan Task Suspension

**Tujuan:** memahami Critical Section pada ESP32 dan Task Suspension/Resume untuk kontrol eksekusi.

**Hardware:** ESP32, 2 LED, serial monitor.

**Wiring:** LED GPIO2 (task A), GPIO4 (task B).

**Langkah:**
1. Buat 2 task yang mengakses variabel global `shared_counter` secara bersamaan.
2. Demonstrasikan race condition tanpa proteksi.
3. Gunakan `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` untuk proteksi atomik.
4. Bandingkan dengan mutex (critical section tidak boleh panggil RTOS API di dalamnya).
5. Implementasikan `vTaskSuspend()` / `vTaskResume()` untuk kontrol task dari luar.

**Output diharapkan:** race condition terlihat tanpa proteksi; critical section memastikan nilai counter konsisten; task A dapat suspend/resume task B.

---

### ESP32_09 — Message Buffers dan Stream Buffers

**Tujuan:** menggunakan Message Buffer untuk pesan variabel dan Stream Buffer untuk byte stream pada ESP32.

**Hardware:** ESP32, 1 LED, 1 push button, serial monitor.

**Wiring:** LED GPIO2; Button GPIO34.

**Langkah:**
1. Buat Message Buffer `xMessageBufferCreate(256)`. Task A kirim pesan panjang variabel saat button ditekan. Task B terima dan print ke serial.
2. Buat Stream Buffer `xStreamBufferCreate(512, 10)`. Task A tulis byte stream simulasi sensor tiap 100 ms. Task B baca dengan trigger level.
3. Bandingkan perilaku keduanya: Message Buffer menyimpan panjang, Stream Buffer hanya byte mentah.
4. Uji kecepatan: 100 byte/detik dan 1000 byte/detik untuk Stream Buffer.

**Output diharapkan:** Message Buffer terima pesan utuh; Stream Buffer terima byte stream; perbedaan terdokumentasi di serial log.

---

### ESP32_10 — Queue Sets Multiplexing

**Tujuan:** menggunakan Queue Set pada ESP32 untuk memantau multiple queue dan semaphore dalam satu task.

**Hardware:** ESP32, 2 LED, 1 push button, serial monitor.

**Wiring:** LED GPIO2, GPIO4; Button GPIO34.

**Langkah:**
1. Buat 2 queue (`xDataQueue`, `xStatusQueue`) dan 1 binary semaphore (`xButtonSemaphore`).
2. Buat Queue Set dengan `xQueueCreateSet()` tambahkan ketiga objek.
3. Task generator isi `xDataQueue` tiap 1 detik, `xStatusQueue` tiap 2 detik. ISR button berikan semaphore.
4. Task monitor `xQueueSelectFromSet()` identifikasi objek aktif.
5. Toggle LED GPIO2 saat data event, LED GPIO4 saat status event.

**Output diharapkan:** task monitor tangani semua event tanpa polling; LED berubah sesuai tipe event.

---

## 7. Eksperimen Multi STM32-ESP32

### MULTI_01 — Distributed Event Group Synchronization

**Tujuan:** mengimplementasikan sinkronisasi event terdistribusi antara STM32 dan ESP32 menggunakan Event Groups dan protokol UART, menunjukkan koordinasi multi-MCU berbasis event.

**Hardware:** STM32, ESP32, 2 LED masing-masing, 1 push button masing-masing, kabel UART.

**Wiring:** STM32: LED PB0, Button PA0. ESP32: LED GPIO2, Button GPIO34. UART: STM32 TX→ESP32 RX, STM32 RX←ESP32 TX, GND bersama.

**Langkah:**
1. STM32: buat Event Group lokal; ISR button set bit 0; timer software set bit 1 tiap 2 detik. Saat event group aktif, kirim byte event code ke ESP32 via UART.
2. ESP32: buat Event Group lokal; UART RX ISR terima event code dan set bit sesuai; task monitor tunggu event bit dengan `xEventGroupWaitBits()`.
3. ESP32 konfirmasi ke STM32 (ACK byte) via UART; STM32 terima ACK dan clear event bit.
4. LED pada kedua MCU mencerminkan state event group saat ini.
5. Demonstrasikan: tekan button STM32 → ESP32 LED nyala (round-trip event sync).

**Output diharapkan:** event dari STM32 muncul di ESP32 dalam <10 ms; ACK protocol berjalan; LED kedua MCU sinkron dengan event yang aktif.

---

### MULTI_02 — Multi-MCU Software Timer Network

**Tujuan:** membangun jaringan Software Timer terdistribusi di mana ESP32 bertindak sebagai master timer coordinator yang mengendalikan software timer di STM32 via UART.

**Hardware:** STM32, ESP32, 3 LED masing-masing, UART.

**Wiring:** STM32: LED PB0 (timer1), PB1 (timer2), PB2 (timer3). ESP32: LED GPIO2, GPIO4, GPIO5. UART bridge.

**Langkah:**
1. ESP32: buat 3 software timer periodik (1s, 2s, 5s). Saat timer fire, kirim perintah timer ke STM32 via UART (format: `T1:START`, `T2:STOP`, `T1:PERIOD:3000`).
2. STM32: task UART RX parse perintah timer; eksekusi `xTimerStart()`, `xTimerStop()`, `xTimerChangePeriod()` sesuai perintah; LED toggle saat timer fire.
3. STM32 kirim laporan timer event ke ESP32 (format: `T1:FIRED`, `T2:FIRED`).
4. ESP32 log timestamp timer event menggunakan software timer send tick count.
5. Demonstrasikan: ESP32 stop/change period timer STM32 via UART, timer STM32 merespons.

**Output diharapkan:** timer STM32 dapat dikontrol dari ESP32; laporan event real-time tampil di serial; LED mencerminkan status timer aktif.

---

### MULTI_03 — Task Notification Pipeline dengan Message Buffer

**Tujuan:** membangun pipeline data high-speed menggunakan hanya Task Notifications dan Message Buffer, tanpa semaphore/queue konvensional, untuk transfer data ADC antar MCU.

**Hardware:** STM32, ESP32, potensiometer (STM32 ADC), 2 LED, UART.

**Wiring:** STM32: potensiometer PA0 (ADC1_IN0), LED PB0. ESP32: LED GPIO2. UART bridge.

**Langkah:**
1. STM32: task ADC baca potensiometer tiap 50 ms. Setelah baca, gunakan `xTaskNotifyGive()` untuk wake task UART TX.
2. STM32: task UART TX (menunggu `ulTaskNotifyTake()`) kirim paket ADC ke ESP32 via UART.
3. ESP32: UART RX ISR gunakan `vTaskNotifyGiveFromISR()` untuk wake task processor.
4. ESP32: task processor (`ulTaskNotifyTake()`) terima data, masukkan ke Message Buffer.
5. ESP32: task analyzer baca Message Buffer, hitung rata-rata 10 sampel, tampilkan hasil + toggle LED.

**Output diharapkan:** pipeline ADC data berjalan tanpa semaphore/queue; latensi pipeline <5 ms; rata-rata 10 sampel tampil di serial setiap detik.

---

### MULTI_04 — Priority Inheritance dan Critical Section Network

**Tujuan:** mendemonstrasikan Priority Inheritance dalam skenario jaringan multi-MCU: STM32 menunjukkan priority inheritance dengan mutex UART, ESP32 menggunakan critical section untuk update statistik atomik, dan keduanya saling memantau via UART.

**Hardware:** STM32, ESP32, 2 LED masing-masing, UART.

**Wiring:** STM32: LED PB0 (low-prio), PB1 (high-prio). ESP32: LED GPIO2, GPIO4. UART bridge.

**Langkah:**
1. STM32: buat mutex `xUARTMutex` untuk akses UART. Task Low (prio 1) ambil mutex, tahan 500 ms, kirim data. Task High (prio 3) coba ambil mutex → tertunda, prioritas Low naik ke 3 (Priority Inheritance). Task Medium (prio 2) tidak bisa jalan selama Low memegang mutex.
2. STM32 kirim log priority ke ESP32: `"Low holding, High waiting, Medium blocked"`, timestamp, durasi hold.
3. ESP32: terima log priority dari STM32. Gunakan `taskENTER_CRITICAL()` untuk atomically update statistik: count inheritance events, total blocked time.
4. ESP32 tampilkan statistik periodik: berapa kali priority inheritance terjadi, rata-rata blocking time.
5. LED STM32 menunjukkan task yang sedang aktif; LED ESP32 menunjukkan jumlah inheritance events.

**Output diharapkan:** priority inheritance terbukti (Medium tidak dapat preempt Low saat Low memegang mutex); statistik di ESP32 mencatat setiap kejadian; log urutan eksekusi task terdokumentasi.

---

### MULTI_05 — Full Advanced Industrial RTOS System

**Tujuan:** mengintegrasikan SEMUA fitur advanced FreeRTOS dari Modul 10 dalam satu sistem industri lengkap: STM32 sebagai node sensor deterministik, ESP32 sebagai gateway koordinator, dengan komunikasi UART menggunakan semua objek RTOS lanjutan.

**Hardware:** STM32, ESP32, potensiometer, 3 LED masing-masing, 2 push button masing-masing, sensor I2C (opsional), UART.

**Wiring:** STM32: potensiometer PA0, LED PB0-2, Button PA0-1. ESP32: LED GPIO2/4/5, Button GPIO34/35. UART bridge.

**Langkah:**
1. **STM32 Node (Static + Critical + Message Buffer)**:
   - Task kritis dibuat dengan `xTaskCreateStatic()` (buffer pre-allocated).
   - ADC task baca sensor tiap 100 ms, gunakan task notification untuk wake TX task.
   - `taskENTER_CRITICAL()` untuk update global stats atomis.
   - Kirim data ke ESP32 via UART dalam Message Buffer berformat JSON-like.
2. **ESP32 Gateway (Event Group + Software Timer + Queue Set)**:
   - State machine berbasis Event Group: `IDLE`, `ACTIVE`, `ALARM`.
   - Software Timer watchdog 5 detik: jika tidak ada data dari STM32, set ALARM state.
   - Queue Set: pantau queue data STM32, queue local ADC, semaphore button dalam satu task.
   - Stream Buffer untuk logging data sensor kontinu.
3. **Sistem Terintegrasi**:
   - ESP32 dapat kirim perintah ke STM32: `START`, `STOP`, `ALARM_RESET`, `SET_RATE`.
   - STM32 merespons perintah menggunakan event group + software timer.
   - LED indicators: hijau (ACTIVE), kuning (IDLE), merah (ALARM) di kedua sisi.
4. Demonstrasikan seluruh fitur: event group state machine, software timer watchdog, task notification pipeline, message buffer data, stream buffer log, critical section stats, static allocation, queue set multiplexing, mutex, priority inheritance.

**Output diharapkan:** sistem industri stabil dengan semua objek RTOS aktif; state machine ESP32 merespons data dan timeout; log lengkap menampilkan semua event; watchdog berfungsi.

---

## 8. Format Pengamatan Minimal

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

## 9. Pertanyaan Analisis Wajib

1. Apa perbedaan Event Groups dan Semaphore untuk sinkronisasi task?
2. Kapan menggunakan Software Timer dibandingkan task periodik biasa?
3. Mengapa Task Notifications lebih ringan dari Semaphore/Queue?
4. Apa perbedaan Binary Semaphore dan Mutex?
5. Bagaimana Priority Inheritance mencegah priority inversion?
6. Kapan memilih heap_1 dibandingkan heap_4?
7. Apa keuntungan Static Allocation untuk sistem real-time?
8. Perbedaan Critical Section dan Mutex? Kapan masing-masing digunakan?
9. Perbedaan Message Buffer dan Stream Buffer? Berikan contoh kasus nyata.
10. Kapan menggunakan Queue Set dan bagaimana mengkombinasikannya dengan Event Group?
11. Bagaimana Task Notification digunakan dalam pipeline data multi-MCU?
12. Bagaimana merancang sistem industri RTOS yang fault-tolerant?

---

## 10. Deliverable Laporan

1. Cover dan identitas.
2. Ringkasan teori FreeRTOS Advanced Modul 10.
3. Tabel hasil **25 eksperimen** (10 STM32 + 10 ESP32 + 5 Multi).
4. Foto wiring tiap kelompok eksperimen.
5. Screenshot serial/LED.
6. Analisis error dan troubleshooting.
7. Source code utama atau link repository.
8. Integrasi project Advanced RTOS Industrial System.
9. Kesimpulan dan perbandingan STM32 vs ESP32 vs Multi.
