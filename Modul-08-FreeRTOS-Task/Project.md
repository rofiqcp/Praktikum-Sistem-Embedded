# Project Modul 08: FreeRTOS Task Management

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 08 — FreeRTOS Task Management |
| **Platform** | ESP32 DevKit V1 *atau* STM32 Blue Pill |
| **Durasi** | 2 minggu sejak modul diberikan |
| **Tipe** | Project Mandiri |
| **Bobot Nilai** | 25% dari total nilai modul |

---

## Deskripsi Umum

Project ini mengintegrasikan **seluruh 12 percobaan** task management FreeRTOS ke dalam satu sistem embedded terpadu. Mahasiswa diminta menyelesaikan permasalahan nyata menggunakan konsep task creation, prioritas, scheduling, monitoring, watchdog, dan komunikasi antar-task.

---

## Soal Cerita: "Sistem Kontrol Akuarium Cerdas — AquaGuard"

### Latar Belakang

Pak Dimas adalah pemilik toko ikan hias **"AquaLand"** di Surabaya yang memiliki 3 akuarium besar berisi ikan-ikan premium: Arwana Platinum, Discus Wild, dan Koi Showa. Total nilai ikan mencapai Rp 150 juta.

Selama ini Pak Dimas mengontrol semua peralatan akuarium secara manual — heater, aerator, lampu UV, dan pemberi pakan otomatis. Setiap malam, ia harus begadang untuk memastikan suhu stabil di 26°C. Suatu malam ia ketiduran dan heater mati tanpa disadari, menyebabkan suhu turun drastis ke 18°C. Keesokan harinya, 3 ekor Discus senilai Rp 15 juta mati karena thermal shock.

Pak Dimas mendatangi laboratorium Teknik Elektro dan meminta mahasiswa membuat **Sistem AquaGuard** — pengontrol akuarium otomatis yang berjalan 24/7 tanpa gagal, menggunakan FreeRTOS pada mikrokontroler.

### Spesifikasi Akuarium

- **Akuarium A** (Arwana): Suhu 27-29°C, pH 6.5-7.5, oksigen terlarut > 5 ppm
- **Akuarium B** (Discus): Suhu 28-30°C, pH 5.5-6.5, oksigen > 6 ppm
- **Akuarium C** (Koi): Suhu 22-26°C, pH 7.0-8.0, oksigen > 5 ppm

---

## Tugas dan Spesifikasi

Implementasikan **Sistem AquaGuard** dengan fitur-fitur berikut. Setiap fitur pemetaan dari percobaan yang telah dilakukan:

---

### Fitur 1: Task Initialization (dari Percobaan 01 — Task Create Basic)

Sistem harus membuat **minimal 6 task** terpisah saat startup:
- `TaskSensor` — membaca sensor suhu, pH, dan oksigen setiap akuarium
- `TaskHeater` — mengontrol heater berdasarkan set-point suhu
- `TaskAerator` — mengontrol aerator berdasarkan level oksigen
- `TaskFeeder` — pemberian pakan otomatis berdasarkan jadwal
- `TaskDisplay` — menampilkan status semua akuarium di serial monitor
- `TaskAlarm` — memonitor kondisi darurat dan membunyikan buzzer

Setiap task harus memiliki stack size yang sesuai dengan beban kerjanya.

**Penilaian:** Task berhasil dibuat, serial menampilkan daftar task beserta prioritas dan stack.

---

### Fitur 2: Priority Management (dari Percobaan 02 — Task Priority)

Atur prioritas task sesuai kritikalitas:
- **Prioritas 4 (Tertinggi):** `TaskAlarm` — respons darurat harus tercepat
- **Prioritas 3:** `TaskSensor` — data harus up-to-date 
- **Prioritas 2:** `TaskHeater`, `TaskAerator` — aktuasi berdasarkan data sensor
- **Prioritas 1:** `TaskFeeder`, `TaskDisplay` — fungsi sekunder

Saat terjadi kondisi **CRITICAL** (suhu di luar ±5°C dari range), prioritas `TaskHeater` harus **naik ke 4** secara runtime menggunakan `vTaskPrioritySet()`, dan kembali ke 2 setelah normal.

**Penilaian:** Prioritas berubah saat kondisi kritis, serial menampilkan pergantian prioritas.

---

### Fitur 3: Periodic Sampling (dari Percobaan 03 — Task Delay Periodic)

`TaskSensor` harus membaca ketiga akuarium secara **periodik presisi** setiap 500ms menggunakan `vTaskDelayUntil()`. Tampilkan statistik:
- Mean reading period
- Maximum jitter

`TaskDisplay` cukup menggunakan `vTaskDelay(1000)` karena presisi tidak kritis.

Buktikan bahwa pembacaan sensor tidak mengalami drift setelah 100+ iterasi.

**Penilaian:** Statistik timing menunjukkan jitter < 5ms, tidak ada drift akumulatif.

---

### Fitur 4: Suspend/Resume Control (dari Percobaan 04 — Task Suspend Resume)

Implementasikan **mode maintenance**:
- Tekan tombol → semua task akuasi (`TaskHeater`, `TaskAerator`, `TaskFeeder`) di-suspend
- Tampilan serial menunjukkan "MAINTENANCE MODE — Actuators SUSPENDED"
- Tekan tombol lagi → resume semua task (gunakan `xTaskResumeFromISR()` untuk button interrupt)
- `TaskSensor` dan `TaskAlarm` tetap berjalan selama maintenance

**Penilaian:** Task dapat di-suspend/resume via button, state transition benar, sensor tetap aktif.

---

### Fitur 5: Dynamic Task Lifecycle (dari Percobaan 05 — Task Delete)

Implementasikan **feeding schedule** yang efisien:
- `TaskFeeder` **tidak perlu aktif 24 jam** — ia hanya dibutuhkan saat jam pakan (07:00, 12:00, 19:00)
- Saat jam pakan: buat task feeder → lakukan pemberian pakan → setelah selesai task menghapus dirinya sendiri (`vTaskDelete(NULL)`)
- Monitor heap sebelum dan sesudah setiap create/delete — pastikan **tidak ada memory leak**

**Penilaian:** Task dibuat dan dihapus sesuai jadwal, heap kembali ke nilai semula setelah delete.

---

### Fitur 6: Stack Safety (dari Percobaan 06 — Stack Monitor)

Setiap task harus memonitor stack high water mark. `TaskDisplay` setiap 10 detik menampilkan:
```
=== STACK MONITOR ===
TaskSensor  : 120/512 words used (23%), OK
TaskHeater  : 89/256 words used (35%), OK  
TaskAlarm   : 234/256 words used (91%), WARNING!
```

Jika usage > 80%, print **WARNING**. Implementasikan `vApplicationStackOverflowHook()` sebagai safety net terakhir.

**Penilaian:** Tabel stack ditampilkan periodik, warning muncul jika usage tinggi, overflow hook terpasang.

---

### Fitur 7: Platform-Specific Feature (dari Percobaan 07)

**Jika menggunakan ESP32:**
Pinning task ke core:
- Core 0: `TaskSensor`, `TaskDisplay` (I/O heavy)
- Core 1: `TaskHeater`, `TaskAerator`, `TaskFeeder` (computation)
- Floating: `TaskAlarm` (agar bisa respond di core manapun)

Tampilkan informasi core assignment dan validasi dengan `xPortGetCoreID()`.

**Jika menggunakan STM32:**
Implementasikan priority inheritance mutex untuk mengakses shared resource (serial port). Demonstrasikan bahwa tanpa mutex terjadi priority inversion (seperti Percobaan 07), lalu tunjukkan solusinya.

**Penilaian:** Core affinity benar (ESP32) atau priority inversion dihindari (STM32).

---

### Fitur 8: CPU Usage Monitoring (dari Percobaan 08 — Idle Hook)

Implementasikan idle hook untuk mengukur **CPU usage** sistem:
- Tampilkan persentase CPU usage setiap 5 detik
- Saat semua akuarium normal → CPU usage rendah (< 30%)
- Saat ada kondisi kritis → CPU usage naik (terukur)
- Jika CPU usage > 90% → print warning "SYSTEM OVERLOADED"

**Penilaian:** CPU usage terukur dan berkorelasi dengan beban kerja.

---

### Fitur 9: Watchdog Protection (dari Percobaan 09 — Watchdog Timer)

Sistem harus **self-recovery** dari hang/crash:
- Daftarkan semua task kritikal ke watchdog (ESP32: task WDT, STM32: IWDG)
- Setiap task harus "feed" watchdog dalam interval yang sesuai
- Simulasikan task hang → watchdog timeout → system reset
- Setelah reset, sistem harus kembali ke kondisi operasional otomatis

**Penilaian:** Watchdog terkonfigurasi, task feed tepat waktu, system recovery benar setelah hang.

---

### Fitur 10: Data Integrity Awareness (dari Percobaan 10 — Task Communication Unsafe)

Demonstrasikan apa yang terjadi **TANPA proteksi** saat beberapa task mengakses data akuarium secara bersamaan:
- Task membaca suhu bersamaan → hasil inconsistent
- Tampilkan corruption counter dan rate (%)

Kemudian berikan komentar di kode: `// TODO: Fix with Queue/Mutex in Modul 09`

> Ini adalah setup untuk integrasi di Modul 09.

**Penilaian:** Race condition terdemonstrasikan, corruption terukur, kode menunjukkan awareness.

---

### Fitur 11: System Dashboard (dari Percobaan 11 — Scheduler Info)

Implementasikan dashboard komprehensif yang menampilkan:
```
========== AQUAGUARD SYSTEM DASHBOARD ==========
Task Name        State    Prio  Stack HWM  CPU%
------------------------------------------------
TaskAlarm        Ready    4     120        2.1%
TaskSensor       Running  3     234        15.3%
TaskHeater       Ready    2     89         8.7%
TaskAerator      Blocked  2     156        5.2%
TaskFeeder       Suspend  1     ---        0.0%
TaskDisplay      Ready    1     312        3.4%
IDLE             Ready    0     45         65.3%
================================================
Free Heap: 45320 / 81920 bytes
Uptime: 02:34:15
```

Gunakan `vTaskList()` dan `vTaskGetRunTimeStats()`.

**Penilaian:** Dashboard lengkap dengan semua task, state-nya benar, CPU% terukur.

---

### Fitur 12: Scheduling Mode Demo (dari Percobaan 12 — Cooperative Scheduling)

Implementasikan mode demo yang menunjukkan perbedaan scheduling:
- **Mode A (Normal):** Preemptive — semua task menggunakan `vTaskDelay()` → sistem berjalan lancar
- **Mode B (Cooperative):** Semua task menggunakan `taskYIELD()` → masih bisa berjalan tapi timing kurang presisi
- **Mode C (Stress Test):** Satu task sengaja melakukan busy loop → demonstrasikan starvation

Switch mode dengan input serial ('A', 'B', 'C').

**Penilaian:** Ketiga mode berjalan, fairness index terukur, starvation terdemonstrasikan.

---

## Ketentuan Pengerjaan

1. **Pilih SATU platform** — ESP32 atau STM32 (sesuaikan Fitur 7 dan 9).
2. **Semua 12 fitur harus diimplementasikan** dalam satu project terpadu.
3. Gunakan **PlatformIO + VS Code**.
4. Sensor boleh **disimulasikan** menggunakan random value atau potentiometer.
5. Kode harus **terdokumentasi** — setiap task diberi komentar fungsinya.
6. **Tidak boleh copy-paste** dari percobaan — kode harus diintegrasi dan disesuaikan dengan skenario AquaGuard.

---

## Rubrik Penilaian

| Komponen | Bobot | Kriteria A (90-100) | Kriteria B (75-89) | Kriteria C (60-74) | Kriteria D (<60) |
|----------|-------|--------------------|--------------------|--------------------|--------------------|
| Task Architecture | 20% | 6+ task terstruktur, stack optimal, prioritas tepat | 5 task, stack wajar | 3-4 task, stack default | < 3 task |
| Scheduling & Timing | 20% | Periodik presisi, jitter < 5ms, 3 mode scheduling | Periodik ada, jitter > 5ms, 2 mode | Delay basic saja | Tidak periodik |
| Monitoring & Safety | 20% | Stack monitor, CPU usage, watchdog, dashboard lengkap | 3 dari 4 fitur monitoring | 2 monitoring | Tanpa monitoring |
| Lifecycle Management | 20% | Create/delete tanpa leak, suspend/resume benar, dynamic priority | 2 dari 3 lifecycle fitur | 1 lifecycle fitur | Tanpa lifecycle |
| Integration & Demo | 20% | Semua 12 fitur terintegrasi, demo berjalan lancar | 10 fitur berjalan | 7-9 fitur | < 7 fitur |

---

## Referensi

1. FreeRTOS API Reference — https://www.freertos.org/a00106.html
2. Mastering the FreeRTOS Real Time Kernel — Richard Barry
3. AN4631 — Using FreeRTOS on STM32, STMicroelectronics
4. ESP-IDF FreeRTOS Documentation — Espressif Systems

---

*Project Modul 08 — FreeRTOS Task | Praktikum Sistem Embedded | 2025/2026*
