# Jobsheet Modul 08: FreeRTOS Task Management

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami konsep RTOS** — Menjelaskan perbedaan bare-metal vs RTOS, konsep task/thread, scheduler, dan context switching.
2. **Membuat dan mengelola FreeRTOS task** — Menggunakan `xTaskCreate()`, `vTaskDelete()`, `vTaskSuspend()/Resume()` untuk manajemen lifecycle task.
3. **Mengatur prioritas dan penjadwalan** — Memahami preemptive scheduling, priority inversion, cooperative scheduling, dan time slicing.
4. **Memonitor kesehatan sistem** — Menggunakan stack high water mark, idle hook, watchdog timer, dan scheduler info untuk debugging.
5. **Mendesain sistem multi-task** — Merancang pembagian task yang efisien dengan mempertimbangkan prioritas, stack size, dan resource sharing.

---

## 2. Peralatan

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | Dual-core, FreeRTOS built-in |
| 2 | STM32 Blue Pill | 1 | ARM Cortex-M3 + FreeRTOS |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | LED 5mm | 3 | Indikator task (merah, hijau, kuning) |
| 5 | Resistor 330Ω | 3 | Current limiting |
| 6 | Push Button | 2 | User input |
| 7 | Resistor 10kΩ | 2 | Pull-up button |
| 8 | Breadboard + kabel jumper | 1 set | |

---

## 3. Teori Singkat

FreeRTOS adalah Real-Time Operating System yang memungkinkan multitasking pada mikrokontroler. Setiap **task** adalah unit eksekusi independen dengan stack sendiri. **Scheduler** menentukan task mana yang berjalan berdasarkan **prioritas** (angka lebih tinggi = prioritas lebih tinggi). Pada mode **preemptive**, task prioritas tinggi dapat menginterupsi task prioritas rendah kapan saja. Setiap task memiliki state: **Running**, **Ready**, **Blocked**, **Suspended**.

ESP32 memiliki keunikan dual-core — task dapat di-pin ke Core 0 atau Core 1 menggunakan `xTaskCreatePinnedToCore()`. STM32F103 single-core sehingga fokus pada preemptive scheduling dan priority management.

---

## 4. Langkah Percobaan

> **Catatan:** Serial Monitor 115200 baud. LED pada ESP32: GPIO2 (built-in), GPIO4, GPIO5. LED pada STM32: PC13 (built-in), PB0, PB1. Button pada ESP32: GPIO0 (BOOT). Button pada STM32: PA0.

---

### Percobaan 01: Task Create Basic

**Tujuan:** Membuat beberapa FreeRTOS task dasar dan memahami parameter `xTaskCreate()`.

#### Langkah Kerja

1. Buka project `ESP32_01` atau `STM32_01`.
2. Pelajari parameter `xTaskCreate()`: nama, fungsi, stack size, parameter, prioritas, handle.
3. Program membuat 2 task LED blink (frekuensi berbeda) + 1 monitor task.
4. Build dan upload. Amati 2 LED berkedip di rate berbeda.
5. Amati serial output: nama task, prioritas, stack high water mark, state.
6. Pada ESP32, perhatikan di core mana setiap task berjalan.

#### Tabel Pengamatan

| Task | Prioritas | Stack (words) | HWM (words) | Core (ESP32) | LED Rate |
|------|-----------|-------------|-------------|-------------|----------|
| Task1 | | | | | |
| Task2 | | | | | |
| Monitor | | | | | |

#### Pertanyaan Analisa

1. Apa hubungan antara stack size dan stack high water mark? Berapa margin aman yang direkomendasikan?
2. Apa yang terjadi jika dua task memiliki prioritas yang sama? Bagaimana scheduler membagi waktu?
3. Berapa total RAM yang digunakan oleh semua task? (stack × jumlah task + overhead)
4. Apa perbedaan `xTaskCreate()` dan `xTaskCreatePinnedToCore()` di ESP32?

---

### Percobaan 02: Task Priority

**Tujuan:** Memahami pengaruh prioritas terhadap penjadwalan task dan distribusi CPU time.

#### Langkah Kerja

1. Buka project `ESP32_02` atau `STM32_02`.
2. Program membuat 3 task (Low/Med/High priority) yang melakukan pekerjaan CPU-intensive.
3. Amati distribusi CPU — task prioritas tinggi menyelesaikan work lebih cepat.
4. Perhatikan mekanisme **priority swap** di runtime — prioritas bertukar mid-execution.
5. Amati bagaimana distribusi CPU berubah setelah swap.

#### Tabel Pengamatan

| Phase | Task High (%) | Task Med (%) | Task Low (%) | Catatan |
|-------|-------------|-------------|-------------|---------|
| Normal | | | | |
| After Swap | | | | |

#### Pertanyaan Analisa

1. Mengapa task prioritas tinggi mendapat lebih banyak CPU time pada preemptive scheduler?
2. Apa yang terjadi jika semua task memiliki prioritas sama? Bagaimana time-slicing bekerja?
3. Kapan sebaiknya menggunakan `vTaskPrioritySet()` untuk mengubah prioritas secara dinamis?
4. Apa risiko jika task prioritas tinggi tidak pernah melakukan `vTaskDelay()` atau blocking call?

---

### Percobaan 03: Task Delay Periodic

**Tujuan:** Membandingkan `vTaskDelay()` (relatif) vs `vTaskDelayUntil()` (absolut) untuk timing periodik presisi.

#### Langkah Kerja

1. Buka project `ESP32_03` atau `STM32_03`.
2. Program menjalankan dua task periodik — satu pakai `vTaskDelay()`, satu pakai `vTaskDelayUntil()`.
3. Amati statistik timing: mean, min, max, stddev, drift.
4. Perhatikan `vTaskDelay()` menunjukkan drift terakumulasi seiring waktu.
5. Perhatikan `vTaskDelayUntil()` mempertahankan periode presisi.

#### Tabel Pengamatan

| Metode | Target (ms) | Mean (ms) | Max Drift (ms) | StdDev (μs) |
|--------|------------|-----------|----------------|-------------|
| vTaskDelay | | | | |
| vTaskDelayUntil | | | | |

#### Pertanyaan Analisa

1. Mengapa `vTaskDelay()` mengalami drift sedangkan `vTaskDelayUntil()` tidak?
2. Dalam skenario apa drift dari `vTaskDelay()` bisa menjadi masalah serius?
3. Berapa tick rate FreeRTOS default? Apa pengaruhnya terhadap resolusi delay?
4. Apakah `vTaskDelayUntil()` bisa mengejar ketinggalan jika suatu iterasi terlambat?

---

### Percobaan 04: Task Suspend Resume

**Tujuan:** Mengontrol eksekusi task menggunakan suspend/resume termasuk resume dari ISR.

#### Langkah Kerja

1. Buka project `ESP32_04` atau `STM32_04`.
2. Program memiliki task LED yang bisa di-suspend/resume.
3. Tekan tombol — LED task di-suspend (LED berhenti berkedip).
4. Tekan tombol lagi — `xTaskResumeFromISR()` membangunkan task dari ISR.
5. Amati state transition di serial: Running → Suspended → Ready → Running.
6. Perhatikan `vTaskSuspendAll()` / `xTaskResumeAll()` untuk suspend semua task sebaligus.

#### Tabel Pengamatan

| Aksi | Task State | LED | Serial Output |
|------|-----------|-----|---------------|
| Awal | | | |
| Tekan tombol (suspend) | | | |
| Tekan tombol (resume) | | | |
| SuspendAll | | | |

#### Pertanyaan Analisa

1. Apa perbedaan `vTaskSuspend()` dan `vTaskDelay()`? Kapan gunakan yang mana?
2. Mengapa perlu `xTaskResumeFromISR()` alih-alih `vTaskResume()` di dalam ISR?
3. Apa efek `vTaskSuspendAll()` terhadap scheduler? Apakah interrupt tetap dilayani?
4. Apa risiko jika task di-suspend saat sedang memegang mutex/semaphore?

---

### Percobaan 05: Task Delete

**Tujuan:** Memahami proses penghapusan task dan dampaknya terhadap memory (heap) — deteksi memory leak.

#### Langkah Kerja

1. Buka project `ESP32_05` atau `STM32_05`.
2. Program membuat task baru saat tombol ditekan, dan menghapusnya saat ditekan lagi.
3. Amati heap usage sebelum, sesudah create, dan sesudah delete.
4. Perhatikan apakah heap kembali ke nilai awal setelah task dihapus.
5. Amati warning tentang memory leak jika task yang dihapus memiliki alokasi yang tidak di-free.

#### Tabel Pengamatan

| Aksi | Free Heap | Min Ever Free | Task Count | Catatan |
|------|----------|--------------|-----------|---------|
| Awal | | | | |
| Setelah Create | | | | |
| Setelah Delete | | | | |
| Create+Delete 10× | | | | |

#### Pertanyaan Analisa

1. Apakah `vTaskDelete()` otomatis membebaskan memori yang dialokasikan task dengan `pvPortMalloc()`?
2. Apa yang terjadi jika task menghapus dirinya sendiri (`vTaskDelete(NULL)`)? Siapa yang membersihkan stack-nya?
3. Mengapa bisa terjadi memory leak setelah berulang kali create/delete task?
4. Bagaimana caranya mencegah memory leak saat menghapus task?

---

### Percobaan 06: Task Stack Monitor

**Tujuan:** Memonitor penggunaan stack task dan mendeteksi stack overflow.

#### Langkah Kerja

1. Buka project `ESP32_06` atau `STM32_06`.
2. Program membuat 3 task dengan stack size berbeda (small/medium/large).
3. Setiap task melakukan operasi rekursif dengan kedalaman berbeda.
4. Amati stack high water mark — semakin kecil = semakin penuh.
5. Monitor task memberikan warning jika penggunaan stack > 80%.
6. Task dengan stack kecil akan overflow → amati `vApplicationStackOverflowHook()`.

#### Tabel Pengamatan

| Task | Stack Size | Recursion Depth | HWM (words) | Usage (%) | Overflow? |
|------|-----------|----------------|-------------|-----------|-----------|
| Small | | | | | |
| Medium | | | | | |
| Large | | | | | |

#### Pertanyaan Analisa

1. Apa itu stack high water mark? Bagaimana FreeRTOS mengukurnya?
2. Berapa margin stack yang aman? Mengapa tidak cukup memberi stack minimal?
3. Apa yang terjadi saat stack overflow? Mengapa bisa merusak data task lain?
4. Bagaimana `vApplicationStackOverflowHook()` mendeteksi overflow? Apa kelemahannya?

---

### Percobaan 07: Core Affinity (ESP32) / Priority Inversion (STM32)

**Tujuan ESP32:** Memahami pinning task ke core tertentu pada dual-core ESP32.
**Tujuan STM32:** Memahami masalah priority inversion pada single-core system.

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_07`.
2. Program membuat task yang di-pin ke Core 0, Core 1, atau floating (`tskNO_AFFINITY`).
3. Amati di core mana setiap task berjalan menggunakan `xPortGetCoreID()`.
4. Bandingkan execution time task pinned vs floating.

#### Langkah Kerja (STM32)

1. Buka project `STM32_07`.
2. Program mendemonstrasikan priority inversion:
   - Task Low (prioritas 1) mengambil resource
   - Task High (prioritas 3) ingin resource → blocked
   - Task Med (prioritas 2) berjalan → High tertunda oleh Med (inversion!)
3. Amati timeline event — durasi inversion terukur.

#### Tabel Pengamatan (ESP32)

| Task | Affinity | Actual Core | Execution Time (μs) |
|------|----------|------------|---------------------|
| Pinned Core 0 | 0 | | |
| Pinned Core 1 | 1 | | |
| Floating | ANY | | |

#### Tabel Pengamatan (STM32)

| Event | Timestamp | Task | State |
|-------|-----------|------|-------|
| Low acquires resource | | Low | Running |
| High requests resource | | High | Blocked |
| Med starts running | | Med | Running (INVERSION!) |
| Low releases resource | | Low | Ready |
| High gets resource | | High | Running |

#### Pertanyaan Analisa

1. (ESP32) Kapan sebaiknya pin task ke core tertentu? Apa keuntungan dan risikonya?
2. (STM32) Apa itu priority inversion? Mengapa task High bisa di-delay oleh task Med?
3. (STM32) Bagaimana priority inheritance protocol (mutex) menyelesaikan masalah inversion?
4. Apa perbedaan arsitektur dual-core ESP32 vs single-core STM32 dalam konteks FreeRTOS?

---

### Percobaan 08: Task Idle Hook

**Tujuan:** Menggunakan idle hook untuk mengukur CPU usage dan mengimplementasikan power saving.

#### Langkah Kerja

1. Buka project `ESP32_08` atau `STM32_08`.
2. Idle hook terdaftar — menghitung berapa kali idle task dijalankan.
3. Task dengan variable load cycle melalui 0%, 25%, 50%, 75%, 95% CPU load.
4. Amati: semakin tinggi load, semakin sedikit idle count, semakin tinggi CPU usage.
5. Pada STM32, idle hook memanggil `__WFI()` (Wait For Interrupt) untuk power saving.

#### Tabel Pengamatan

| Load Level | Idle Count | CPU Usage (%) | Catatan |
|-----------|-----------|-------------|---------|
| 0% | | | |
| 25% | | | |
| 50% | | | |
| 75% | | | |
| 95% | | | |

#### Pertanyaan Analisa

1. Bagaimana idle hook menghitung CPU usage? Apa formulanya?
2. Apa itu `__WFI()` dan bagaimana mengurangi konsumsi daya?
3. Pada ESP32 dual-core, apakah idle hook per-core? Bagaimana mengukur usage per core?
4. Apa yang terjadi jika idle hook berisi kode yang memblokir (blocking)?

---

### Percobaan 09: Task Watchdog

**Tujuan:** Menggunakan watchdog timer untuk mendeteksi dan recovery dari task yang hang.

#### Langkah Kerja

1. Buka project `ESP32_09` atau `STM32_09`.
2. ESP32: Pelajari Task Watchdog (`esp_task_wdt_*`) — setiap task terdaftar harus "feed" dalam timeout.
3. STM32: Pelajari IWDG hardware watchdog — task feeder harus refresh sebelum timeout 4 detik.
4. Build dan upload. Amati task "good" yang feed tepat waktu.
5. Setelah beberapa cycle, task "bad" berhenti feed → watchdog timeout → system reset.
6. Amati proses recovery setelah reset.

#### Tabel Pengamatan

| Event | Waktu | Task | Feed Status | System Action |
|-------|-------|------|------------|---------------|
| Startup | | | | |
| Normal feed | | Good | OK | |
| Bad task hangs | | Bad | TIMEOUT | |
| Watchdog reset | | — | | Reset! |
| Recovery | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan Task Watchdog (ESP32) dan IWDG hardware watchdog (STM32)?
2. Mengapa watchdog penting dalam sistem embedded? Berikan contoh skenario nyata.
3. Berapa timeout watchdog yang optimal? Terlalu pendek vs terlalu panjang — apa risikonya?
4. Bagaimana cara mendeteksi bahwa reset disebabkan oleh watchdog (bukan power cycle)?

---

### Percobaan 10: Task Communication (Unsafe)

**Tujuan:** Mendemonstrasikan race condition dan data corruption saat task berbagi variabel tanpa proteksi.

#### Langkah Kerja

1. Buka project `ESP32_10` atau `STM32_10`.
2. Program sengaja **TIDAK menggunakan** mutex atau queue — shared struct diakses langsung.
3. Producer task menulis data, consumer task membaca data secara bersamaan.
4. Amati statistik corruption: counter mismatch, checksum error, torn reads.
5. Semakin tinggi load, semakin banyak corruption.

> **Catatan:** Ini adalah demonstrasi masalah — solusinya di Modul 09 (Queue & Semaphore).

#### Tabel Pengamatan

| Waktu | Total Reads | Corruption Count | Corruption Rate (%) |
|-------|------------|-----------------|-------------------|
| 10 detik | | | |
| 30 detik | | | |
| 60 detik | | | |

#### Pertanyaan Analisa

1. Mengapa terjadi data corruption saat dua task mengakses shared variable secara bersamaan?
2. Apa itu race condition? Berikan contoh urutan eksekusi yang menyebabkan corruption.
3. Apa itu torn read? Mengapa struct yang lebih besar lebih rentan?
4. Bagaimana cara menyelesaikan masalah ini? (Preview: mutex, queue, semaphore di Modul 09)

---

### Percobaan 11: Task Scheduler Info

**Tujuan:** Menggunakan API FreeRTOS untuk mendapatkan informasi runtime tentang semua task dan scheduler.

#### Langkah Kerja

1. Buka project `ESP32_11` atau `STM32_11`.
2. Program mencetak tabel lengkap semua task: nama, state, prioritas, stack HWM, core.
3. Juga mencetak runtime stats: CPU usage per task (%).
4. Program membuat, suspend, dan delete task — amati perubahan tabel.
5. Identifikasi task-task sistem (IDLE, Tmr Svc, esp_timer, dll).

#### Tabel Pengamatan

| Task Name | State | Priority | Stack HWM | CPU (%) |
|-----------|-------|----------|-----------|---------|
| | | | | |
| | | | | |
| | | | | |

#### Pertanyaan Analisa

1. Apa fungsi `vTaskList()` dan `vTaskGetRunTimeStats()`? Apa perbedaannya?
2. Task apa saja yang dibuat oleh sistem (bukan oleh user)? Apa fungsi masing-masing?
3. Mengapa task IDLE selalu ada? Apa yang dilakukan jika tidak ada task lain yang ready?
4. Bagaimana runtime stats menghitung CPU percentage per task?

---

### Percobaan 12: Task Cooperative Scheduling

**Tujuan:** Membandingkan preemptive scheduling, cooperative scheduling (`taskYIELD()`), dan efek CPU hogging.

#### Langkah Kerja

1. Buka project `ESP32_12` atau `STM32_12`.
2. Program berjalan dalam 3 fase:
   - **Fase 1:** Preemptive — task menggunakan `vTaskDelay()`, distribusi merata
   - **Fase 2:** Cooperative — task menggunakan `taskYIELD()`, distribusi mirip
   - **Fase 3:** Hogging — satu task tidak yield/delay, memonopoli CPU
3. Amati fairness index setiap fase.
4. Pada fase 3, amati task lain kelaparan (starvation).

#### Tabel Pengamatan

| Fase | Task A (%) | Task B (%) | Task C (%) | Fairness Index |
|------|----------|----------|----------|---------------|
| Preemptive | | | | |
| Cooperative | | | | |
| Hogging | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan preemptive dan cooperative scheduling?
2. Apa yang terjadi jika task tidak pernah yield pada cooperative scheduling?
3. Apa itu fairness index? Bagaimana menghitungnya?
4. Dalam skenario apa cooperative scheduling lebih cocok daripada preemptive?

---

## 5. Tabel Komparatif

| Aspek | STM32 (FreeRTOS) | ESP32 (FreeRTOS) |
|-------|-------------------|-------------------|
| Core | Single-core Cortex-M3 | Dual-core Xtensa LX6 |
| Tick Rate | Configurable (default 1kHz) | 100Hz (default ESP-IDF) |
| Core Affinity | N/A | `xTaskCreatePinnedToCore()` |
| Watchdog | IWDG/WWDG hardware | Task WDT (software) |
| Idle Hook | `vApplicationIdleHook()` | `esp_register_freertos_idle_hook()` |
| Stack Overflow | `configCHECK_FOR_STACK_OVERFLOW` | Same + ESP panic handler |
| Heap | heap_4 (default) | Multi-region (DRAM, IRAM, PSRAM) |

---

## 6. Referensi

1. FreeRTOS API Reference — https://www.freertos.org/a00106.html
2. Mastering the FreeRTOS Real Time Kernel — Richard Barry
3. AN4631 — Using FreeRTOS on STM32, STMicroelectronics
4. ESP-IDF FreeRTOS Documentation — Espressif Systems
5. STM32F103 Reference Manual (RM0008) — NVIC, SysTick

---

*Jobsheet Modul 08 — FreeRTOS Task | Praktikum Sistem Embedded | 2025/2026*
