# Jobsheet Modul 09: FreeRTOS Queue dan Semaphore

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami inter-task communication** — Menggunakan queue untuk mengirim data secara aman antar task.
2. **Menguasai sinkronisasi** — Menggunakan binary semaphore, counting semaphore, dan mutex.
3. **Mendesain producer-consumer pattern** — Mengimplementasikan bounded buffer dengan queue.
4. **Mencegah race condition** — Menggunakan mutex untuk melindungi shared resource.
5. **Memahami priority inversion** — Mendeteksi dan mengatasi priority inversion dengan priority inheritance.

---

## 2. Peralatan

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | Dual-core, FreeRTOS built-in |
| 2 | STM32 Blue Pill | 1 | ARM Cortex-M3 + FreeRTOS |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | LED 5mm | 2 | Indikator (merah, hijau) |
| 5 | Resistor 330Ω | 2 | Current limiting |
| 6 | Push Button | 1 | User input / interrupt source |
| 7 | Resistor 10kΩ | 1 | Pull-up button |
| 8 | Breadboard + kabel jumper | 1 set | |

---

## 3. Teori Singkat

**Queue** adalah mekanisme FIFO (First-In, First-Out) untuk mengirim data antar task secara thread-safe. Data di-copy ke buffer internal sehingga tidak ada shared memory. Queue mendukung blocking read/write dengan timeout.

**Semaphore** adalah mekanisme sinkronisasi:
- **Binary Semaphore:** Signal event dari ISR ke task (give/take, nilai 0 atau 1).
- **Counting Semaphore:** Mengontrol akses ke resource pool terbatas (N resource tersedia).
- **Mutex:** Melindungi shared resource dengan kepemilikan (hanya pemilik bisa release). Mendukung **priority inheritance** untuk mencegah priority inversion.

**Recursive Mutex** memungkinkan task yang sama mengambil mutex berkali-kali tanpa deadlock.

---

## 4. Langkah Percobaan

> **Catatan:** Serial Monitor 115200 baud. LED pada ESP32: GPIO2 (built-in), GPIO4. LED pada STM32: PC13 (built-in), PB0. Button pada ESP32: GPIO0 (BOOT). Button pada STM32: PA0.

---

### Percobaan 01: Queue Basic

**Tujuan:** Memahami mekanisme dasar queue — producer mengirim data, consumer menerima.

#### Langkah Kerja

1. Buka project `ESP32_01` atau `STM32_01`.
2. Pelajari `xQueueCreate()` — membuat queue dengan kapasitas dan ukuran item tertentu.
3. Producer task mengirim integer inkremental ke queue setiap 1 detik.
4. Consumer task menerima dari queue — toggle LED setiap data masuk.
5. Build dan upload. Amati LED berkedip sesuai data yang diterima.
6. Amati serial output: send count, receive count, free spaces, messages waiting.

#### Tabel Pengamatan

| Waktu (s) | Sent | Received | Queue Free | Queue Messages | LED State |
|-----------|------|----------|-----------|---------------|-----------|
| 5 | | | | | |
| 10 | | | | | |
| 20 | | | | | |
| 30 | | | | | |

#### Pertanyaan Analisa

1. Apa yang terjadi jika consumer lebih lambat dari producer? Apa perilaku queue saat penuh?
2. Apa perbedaan `xQueueSend()` dengan `xQueueSendToFront()` dan `xQueueSendToBack()`?
3. Mengapa menggunakan queue lebih aman daripada shared variable (seperti di Modul 08 P10)?
4. Berapa overhead memory queue? (queue header + item_size × length)

---

### Percobaan 02: Queue Struct

**Tujuan:** Mengirim data terstruktur (struct) melalui queue — multiple sensor ke satu processor.

#### Langkah Kerja

1. Buka project `ESP32_02` atau `STM32_02`.
2. Pelajari struct `sensor_data_t` yang berisi tipe sensor, nilai, dan timestamp.
3. Tiga task sensor (Temperature, Humidity, Pressure/Light) masing-masing mengirim struct ke satu queue.
4. Satu processor task menerima dan mengidentifikasi sumber data berdasarkan field struct.
5. Amati bagaimana satu queue menangani data dari multiple producer.

#### Tabel Pengamatan

| Sensor | Nilai Terkirim | Timestamp | Diterima Processor? | Latency (tick) |
|--------|---------------|-----------|-------------------|----------------|
| Temperature | | | | |
| Humidity | | | | |
| Pressure/Light | | | | |

#### Pertanyaan Analisa

1. Mengapa mengirim struct by-copy (bukan by-reference) lebih aman di FreeRTOS?
2. Bagaimana processor membedakan data dari sensor berbeda dalam satu queue?
3. Apa yang terjadi jika struct terlalu besar? Berapa ukuran maksimal yang wajar?
4. Kapan sebaiknya menggunakan satu queue shared vs queue terpisah per sensor?

---

### Percobaan 03: Queue Multiple

**Tujuan:** Merancang pipeline dua arah menggunakan multiple queue — command dan status.

#### Langkah Kerja

1. Buka project `ESP32_03` atau `STM32_03`.
2. Program menggunakan dua queue: **command queue** dan **status queue**.
3. Commander task mengirim perintah (LED_ON, LED_OFF, TOGGLE, BLINK, STATUS) ke Executor.
4. Executor menerima, mengeksekusi, dan mengirim status report balik ke Monitor melalui status queue.
5. Monitor menampilkan status setiap perintah — apakah berhasil atau gagal.

#### Tabel Pengamatan

| Command | Sent To | Executor Action | Status Response | LED |
|---------|---------|----------------|-----------------|-----|
| LED_ON | cmd_queue | | | |
| LED_OFF | cmd_queue | | | |
| TOGGLE | cmd_queue | | | |
| BLINK | cmd_queue | | | |
| STATUS | cmd_queue | | | |

#### Pertanyaan Analisa

1. Apa keuntungan menggunakan pipeline dua queue dibanding satu queue bidirectional?
2. Bagaimana menangani situasi jika status queue penuh saat Executor ingin mengirim?
3. Dapatkah pattern ini di-extend ke 3+ queue? Berikan contoh skenario.
4. Apa perbedaan arsitektur command-status ini dengan polling-based approach?

---

### Percobaan 04: Queue ISR

**Tujuan:** Mengirim data ke queue dari ISR (Interrupt Service Routine) menggunakan API ISR-safe.

#### Langkah Kerja

1. Buka project `ESP32_04` atau `STM32_04`.
2. Konfigurasi button interrupt (ESP32: GPIO0 falling edge, STM32: PA0 EXTI).
3. Saat button ditekan, ISR mengirim event ke queue menggunakan `xQueueSendFromISR()`.
4. LED handler task menerima event dan toggle LED.
5. Amati parameter `pxHigherPriorityTaskWoken` dan `portYIELD_FROM_ISR()`.
6. Pada STM32, amati simulated button press jika hardware button tidak tersedia.

#### Tabel Pengamatan

| Press # | ISR Triggered | Event Sent | Task Received | LED Toggle | Latency (μs) |
|---------|-------------|-----------|-------------|-----------|--------------|
| 1 | | | | | |
| 2 | | | | | |
| 3 | | | | | |

#### Pertanyaan Analisa

1. Mengapa harus menggunakan `xQueueSendFromISR()` dan bukan `xQueueSend()` di dalam ISR?
2. Apa fungsi `pxHigherPriorityTaskWoken` dan `portYIELD_FROM_ISR()`?
3. Apa yang terjadi jika queue penuh saat ISR ingin mengirim? Bagaimana menanganinya?
4. Bandingkan pendekatan ini dengan direct task notification (preview Modul 10).

---

### Percobaan 05: Queue Set

**Tujuan:** Menggunakan QueueSet untuk menunggu pada multiple queue sekaligus (multiplexing).

#### Langkah Kerja

1. Buka project `ESP32_05` atau `STM32_05`.
2. Dua sender task masing-masing mengirim ke queue sendiri (temperature queue, humidity/alarm queue).
3. Satu receiver task menggunakan `xQueueCreateSet()` dan `xQueueSelectFromSet()` untuk menunggu kedua queue.
4. Amati receiver merespons queue mana saja yang memiliki data duluan.
5. Pada STM32, alarm event menyebabkan LED flash — prioritas lebih tinggi.

#### Tabel Pengamatan

| Event Source | Queue | Data | Receiver Action | Response Time (ms) |
|-------------|-------|------|----------------|-------------------|
| Temperature | temp_q | | | |
| Humidity/Alarm | hum_q | | | |
| Both | | | Mana duluan? | |

#### Pertanyaan Analisa

1. Apa keuntungan QueueSet dibanding polling setiap queue secara bergiliran?
2. Berapa ukuran QueueSet yang harus dibuat? (total semua item dari semua queue)
3. Apakah QueueSet bisa dicampur dengan semaphore? Jika ya, berikan contoh.
4. Apa kelemahan QueueSet? Kapan lebih baik menggunakan satu queue shared?

---

### Percobaan 06: Binary Semaphore

**Tujuan:** Menggunakan binary semaphore untuk sinkronisasi event dari ISR ke task.

#### Langkah Kerja

1. Buka project `ESP32_06` atau `STM32_06`.
2. Binary semaphore dibuat dengan `xSemaphoreCreateBinary()` — awalnya kosong.
3. ISR button memberikan semaphore (`xSemaphoreGiveFromISR()`).
4. LED task menunggu (blocking) pada `xSemaphoreTake()` — bangun saat button ditekan.
5. Amati ISR trigger count vs task wakeup count — apakah ada event yang hilang?
6. Tekan button berkali-kali sangat cepat — amati apakah semua event tertangkap.

#### Tabel Pengamatan

| Test | Button Presses | ISR Count | Task Wakeup | Missed Events? |
|------|---------------|----------|------------|---------------|
| Slow (1/detik) | | | | |
| Fast (5/detik) | | | | |
| Burst (10 sekaligus) | | | | |

#### Pertanyaan Analisa

1. Mengapa binary semaphore bisa miss event jika ISR trigger lebih cepat dari task processing?
2. Apa perbedaan binary semaphore dan mutex? Kapan gunakan yang mana?
3. Jika ingin menghitung semua event tanpa miss, solusi apa yang lebih cocok? (Counting semaphore? Queue?)
4. Mengapa binary semaphore tidak memiliki konsep "ownership" seperti mutex?

---

### Percobaan 07: Counting Semaphore

**Tujuan:** Mengontrol akses ke resource pool terbatas — analogi parking lot.

#### Langkah Kerja

1. Buka project `ESP32_07` atau `STM32_07`.
2. Counting semaphore dibuat dengan max 3 (3 resource/parking spot tersedia).
3. Lima worker task berkompetisi untuk mendapatkan resource.
4. Worker mengambil resource (`xSemaphoreTake`), menggunakan selama beberapa detik, lalu melepas (`xSemaphoreGive`).
5. Amati bahwa maksimal 3 worker bisa menggunakan resource bersamaan.
6. Worker ke-4 dan ke-5 harus menunggu sampai ada yang selesai.

#### Tabel Pengamatan

| Waktu | Worker 1 | Worker 2 | Worker 3 | Worker 4 | Worker 5 | Available |
|-------|---------|---------|---------|---------|---------|-----------|
| T+0s | | | | | | 3/3 |
| T+1s | | | | | | |
| T+3s | | | | | | |
| T+5s | | | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan counting semaphore dan binary semaphore?
2. Apa yang terjadi jika `xSemaphoreTake()` timeout habis sebelum resource tersedia?
3. Bagaimana menentukan nilai max counting semaphore? Apa risikonya jika terlalu kecil/besar?
4. Dapatkah counting semaphore digunakan untuk menghitung event (bukan resource pool)?

---

### Percobaan 08: Mutex Shared Resource

**Tujuan:** Membuktikan bahwa mutex mencegah race condition pada shared variable.

#### Langkah Kerja

1. Buka project `ESP32_08` atau `STM32_08`.
2. **Fase 1 (Tanpa Mutex):** Dua task increment shared counter masing-masing 10.000×.
3. Catat hasil akhir — seharusnya 20.000 tapi sering kurang (race condition!).
4. **Fase 2 (Dengan Mutex):** Percobaan sama tapi dilindungi mutex.
5. Catat hasil — seharusnya tepat 20.000.
6. Bandingkan waktu eksekusi Fase 1 vs Fase 2 — mutex menambah overhead.

#### Tabel Pengamatan

| Fase | Expected | Actual | Lost Count | Execution Time (ms) |
|------|----------|--------|-----------|-------------------|
| Tanpa Mutex | 20000 | | | |
| Dengan Mutex | 20000 | | | |

#### Pertanyaan Analisa

1. Mengapa tanpa mutex hasilnya kurang dari 20.000? Jelaskan urutan operasi yang menyebabkan lost increment.
2. Berapa persentase overhead waktu yang ditambahkan mutex?
3. Apa perbedaan mutex dan binary semaphore untuk proteksi resource?
4. Apa itu critical section (`taskENTER_CRITICAL()`)? Kapan gunakan critical section vs mutex?

---

### Percobaan 09: Priority Inversion dengan Mutex

**Tujuan:** Mengamati priority inversion dan mekanisme priority inheritance pada mutex FreeRTOS.

#### Langkah Kerja

1. Buka project `ESP32_09` atau `STM32_09`.
2. Tiga task: Low (prioritas 1), Medium (prioritas 2), High (prioritas 3).
3. Low task mengambil mutex dan mengerjakan sesuatu selama 3 detik (simulasi).
4. High task mencoba mengambil mutex yang sama → blocked.
5. Medium task (tidak butuh mutex) berjalan → **priority inversion!** High tertunda oleh Medium.
6. Amati log: priority inheritance membuat Low sementara naik ke prioritas 3.
7. Amati timeline: Low selesai → mutex released → High langsung jalan.

#### Tabel Pengamatan

| Waktu | Low | Medium | High | Low's Effective Priority | Mutex Owner |
|-------|-----|--------|------|------------------------|-------------|
| T+0 | Running (has mutex) | Ready | Ready | 1 | Low |
| T+1 | | | Blocked (wants mutex) | → 3 (inherited!) | Low |
| T+2 | Running (boosted) | Ready | Blocked | 3 | Low |
| T+3 | Release mutex | | | 1 (restored) | None |
| T+3+ | Ready | Ready | Running (got mutex) | | High |

#### Pertanyaan Analisa

1. Apa itu priority inversion? Berikan contoh kasus nyata yang berbahaya (mis. Mars Pathfinder).
2. Bagaimana priority inheritance menyelesaikan masalah ini?
3. Apakah priority inheritance menyelesaikan inversion sepenuhnya? Atau hanya membatasi durasinya?
4. Apa perbedaan priority inheritance dan priority ceiling protocol?

---

### Percobaan 10: Recursive Mutex

**Tujuan:** Memahami mutex rekursif — satu task boleh mengambil mutex berkali-kali tanpa deadlock.

#### Langkah Kerja

1. Buka project `ESP32_10` atau `STM32_10`.
2. TaskA memanggil fungsi nested: `outer()` → `mid()` → `inner()`, masing-masing take recursive mutex.
3. Amati nesting level naik saat take dan turun saat give — harus balanced.
4. TaskB mencoba mengambil mutex yang sama — harus menunggu sampai TaskA fully released.
5. Amati max nesting depth dan total operasi.

#### Tabel Pengamatan

| Task | Operation | Nesting Level | Status |
|------|----------|--------------|--------|
| TaskA | outer: take | 1 | OK |
| TaskA | mid: take | 2 | OK |
| TaskA | inner: take | 3 | OK |
| TaskA | inner: give | 2 | |
| TaskA | mid: give | 1 | |
| TaskA | outer: give | 0 | Released! |
| TaskB | take | 1 | OK (sekarang giliran B) |

#### Pertanyaan Analisa

1. Apa yang terjadi jika menggunakan mutex biasa (non-recursive) dalam skenario nested call?
2. Mengapa recursive mutex memerlukan give yang jumlahnya sama dengan take?
3. Kapan sebaiknya menggunakan recursive mutex vs redesign kode untuk menghindari nested lock?
4. Apakah recursive mutex mendukung priority inheritance?

---

### Percobaan 11: Producer-Consumer Pattern

**Tujuan:** Mengimplementasikan pattern producer-consumer klasik dengan bounded buffer (queue).

#### Langkah Kerja

1. Buka project `ESP32_11` atau `STM32_11`.
2. Tiga producer task mengirim item ke queue berkapasitas 8 (bounded buffer).
3. Satu consumer task mengambil dan memproses item, toggle LED.
4. Amati queue fill level — naik saat producer lebih cepat, turun saat consumer catches up.
5. Bandingkan statistik per-producer: send count, block count.
6. Amati apa yang terjadi saat queue penuh — producer blocked menunggu space.

#### Tabel Pengamatan

| Waktu | Producer 1 Sent | Producer 2 Sent | Producer 3 Sent | Consumer Processed | Queue Fill | Dropped |
|-------|----------------|----------------|----------------|-------------------|-----------|---------|
| 10s | | | | | | |
| 30s | | | | | | |
| 60s | | | | | | |

#### Pertanyaan Analisa

1. Apa yang membedakan bounded buffer dari unbounded buffer? Mengapa bounded lebih aman?
2. Apa trade-off ukuran queue? Terlalu kecil → blocking sering. Terlalu besar → memory waste.
3. Bagaimana jika consumer crash/hang? Apa dampaknya pada producer?
4. Bagaimana menerapkan flow control jika producer rate tidak bisa dikontrol?

---

### Percobaan 12: Reader-Writer Lock

**Tujuan:** Mengimplementasikan reader-writer lock — banyak reader bersamaan, writer eksklusif.

#### Langkah Kerja

1. Buka project `ESP32_12` atau `STM32_12`.
2. Pelajari implementasi: 2 mutex + reader counter.
3. **Empat reader task** — bisa membaca data bersamaan (concurrent).
4. **Dua writer task** — harus exclusive access (tidak boleh bersamaan dengan reader atau writer lain).
5. First reader yang masuk mengunci writer mutex. Last reader yang keluar melepas.
6. Amati statistik: read/write ratio, max concurrent readers, writer wait time.

#### Tabel Pengamatan

| Waktu | Reader Active | Writer Active | Max Concurrent Readers | Writer Wait Count |
|-------|-------------|-------------|----------------------|------------------|
| 10s | | | | |
| 30s | | | | |
| 60s | | | | |

#### Pertanyaan Analisa

1. Mengapa reader boleh bersamaan tapi writer harus eksklusif?
2. Apa masalah **writer starvation** pada reader-writer lock? Bagaimana mencegahnya?
3. Jelaskan mekanisme "first reader locks, last reader unlocks" — mengapa perlu counter?
4. Kapan reader-writer lock lebih baik dari mutex biasa? Kapan justru lebih buruk (overhead)?

---

## 5. Tabel Komparatif

| Mekanisme | Fungsi Utama | Ownership? | ISR Safe? | Priority Inheritance? |
|-----------|-------------|-----------|----------|---------------------|
| Queue | Transfer data antar task | Tidak | `FromISR` variant | Tidak |
| Binary Semaphore | Event signaling | Tidak | `FromISR` variant | Tidak |
| Counting Semaphore | Resource pool limiting | Tidak | `FromISR` variant | Tidak |
| Mutex | Shared resource protection | Ya | Tidak | Ya |
| Recursive Mutex | Nested locking | Ya | Tidak | Ya |

| Aspek | STM32 (FreeRTOS) | ESP32 (FreeRTOS) |
|-------|-------------------|-------------------|
| Queue Implementation | Standard FreeRTOS | Standard FreeRTOS |
| ISR Context | EXTI + `FromISR` | GPIO ISR + `FromISR` |
| Mutex | Standard | Standard + spinlock (SMP) |
| Priority Inheritance | Ya | Ya |
| Button Input | PA0 EXTI | GPIO0 (BOOT) |

---

## 6. Referensi

1. FreeRTOS Queue API — https://www.freertos.org/a00018.html
2. FreeRTOS Semaphore/Mutex API — https://www.freertos.org/a00113.html
3. Mastering the FreeRTOS Real Time Kernel — Richard Barry (Ch. 4-7)
4. AN4631 — Using FreeRTOS on STM32, STMicroelectronics
5. ESP-IDF FreeRTOS Documentation — Espressif Systems

---

*Jobsheet Modul 09 — FreeRTOS Queue & Semaphore | Praktikum Sistem Embedded | 2025/2026*
