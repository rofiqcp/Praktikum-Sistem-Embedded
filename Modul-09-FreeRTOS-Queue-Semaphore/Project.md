# Project Modul 09: FreeRTOS Queue dan Semaphore

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 09 — FreeRTOS Queue & Semaphore |
| **Platform** | ESP32 DevKit V1 *atau* STM32 Blue Pill |
| **Durasi** | 2 minggu sejak modul diberikan |
| **Tipe** | Project Mandiri |
| **Bobot Nilai** | 25% dari total nilai modul |

---

## Deskripsi Umum

Project ini mengintegrasikan **seluruh 12 percobaan** queue, semaphore, dan mutex ke dalam satu sistem embedded. Mahasiswa diminta merancang solusi inter-task communication yang aman, efisien, dan bebas race condition.

---

## Soal Cerita: "Sistem Manajemen Parkir Cerdas — SmartPark"

### Latar Belakang

PT TransJaya mengoperasikan gedung parkir 3 lantai di pusat kota Bandung dengan kapasitas total **150 slot**: Lantai 1 (50 slot mobil), Lantai 2 (50 slot mobil), dan Lantai 3 (50 slot motor). Setiap hari rata-rata 800 kendaraan masuk-keluar.

Selama ini, sistem parkir menggunakan operator manual dengan walkie-talkie. Masalah terus terjadi:
- Kendaraan diarahkan ke lantai yang sudah penuh → harus putar balik
- Dua kendaraan diarahkan ke slot yang sama → konflik
- Pencatatan manual sering error → laporan pendapatan tidak akurat
- Saat peak hour (17:00–19:00), antrian hingga ke jalan utama

Direktur PT TransJaya meminta tim engineering membuat **Sistem SmartPark** — manajemen parkir otomatis berbasis mikrokontroler yang menangani seluruh flow kendaraan masuk-keluar secara real-time menggunakan FreeRTOS.

### Data Operasional

- **Peak hour:** 17:00–19:00 (5 kendaraan/menit masuk, 3 kendaraan/menit keluar)
- **Normal hour:** 2 kendaraan/menit masuk, 2 kendaraan/menit keluar
- **Tarif:** Mobil Rp 5.000/jam, Motor Rp 2.000/jam
- **Gate:** 2 gate masuk + 2 gate keluar (bisa paralel)
- **Sensor:** Ultrasonic per slot (detect occupied/empty)

---

## Tugas dan Spesifikasi

Implementasikan **Sistem SmartPark** dengan fitur-fitur berikut:

---

### Fitur 1: Event Queue — Gate Entry (dari Percobaan 01 — Queue Basic)

Setiap gate masuk mengirim event ke **entry queue** saat kendaraan terdeteksi:
- Gate 1 dan Gate 2 beroperasi paralel sebagai producer
- Parking Controller sebagai consumer menerima event dan memproses alokasi
- Queue berkapasitas 10 event (buffer saat burst)

Serial output menunjukkan send/receive count, queue fill level.

**Penilaian:** Queue berfungsi, data terkirim-terima tanpa loss, fill level termonitor.

---

### Fitur 2: Structured Vehicle Data (dari Percobaan 02 — Queue Struct)

Setiap event mengandung struct `vehicle_data_t`:
```c
typedef struct {
    uint8_t gate_id;        // 1 atau 2
    uint8_t vehicle_type;   // MOBIL=1, MOTOR=2
    uint32_t plate_hash;    // hash plat nomor (simulasi)
    uint32_t entry_time;    // tick count masuk
} vehicle_data_t;
```

Tiga task sensor (Gate1, Gate2, Exit) masing-masing mengirim struct ke controller queue.

**Penilaian:** Struct terkirim dengan benar, controller mengidentifikasi sumber.

---

### Fitur 3: Command-Status Pipeline (dari Percobaan 03 — Queue Multiple)

Dua queue terpisah:
- **Command Queue:** Controller → Actuator (OPEN_GATE, CLOSE_GATE, REDIRECT, FULL_WARNING)
- **Status Queue:** Actuator → Display (GATE_OPENED, VEHICLE_ENTERED, SLOT_ASSIGNED, ERROR)

Display task menerima status dan memperbarui tampilan serial:
```
[GATE 1] OPENED → Vehicle MOBIL entered → Assigned L1-S23
[GATE 2] REDIRECT → Lantai 1 FULL, redirect to Lantai 2
```

**Penilaian:** Pipeline dua arah berfungsi, status terupdate di display.

---

### Fitur 4: Interrupt-Driven Gate Trigger (dari Percobaan 04 — Queue ISR)

Button press mensimulasikan sensor kendaraan di gate:
- ISR mengirim event ke queue menggunakan `xQueueSendFromISR()`
- Controller task memproses event dan mengalokasikan slot
- Implementasikan `portYIELD_FROM_ISR()` untuk fast response

Ukur latency dari button press sampai gate response.

**Penilaian:** ISR-to-task communication berjalan, latency terukur.

---

### Fitur 5: Multi-Source Monitoring (dari Percobaan 05 — Queue Set)

Gunakan QueueSet untuk memonitor multiple event source sekaligus:
- Sensor queue (data sensor regular)
- Alarm queue (event darurat: kebakaran, kendaraan terjepit, dll)

Saat alarm masuk → prioritas lebih tinggi, langsung proses sebelum sensor data.

**Penilaian:** QueueSet berfungsi, alarm diprioritaskan.

---

### Fitur 6: Gate Barrier Control (dari Percobaan 06 — Binary Semaphore)

Binary semaphore untuk sinkronisasi gate barrier:
- ISR sensor mendeteksi kendaraan → give semaphore
- Gate task menunggu semaphore → angkat barrier → tunggu kendaraan lewat → turunkan barrier
- Hanya satu kendaraan boleh melewati gate pada satu waktu

**Penilaian:** Barrier hanya terbuka saat kendaraan terdeteksi, satu per satu.

---

### Fitur 7: Slot Resource Pool (dari Percobaan 07 — Counting Semaphore)

Counting semaphore merepresentasikan jumlah slot tersedia per lantai:
- Lantai 1: counting semaphore initial = 50
- Lantai 2: counting semaphore initial = 50
- Lantai 3: counting semaphore initial = 50

Kendaraan masuk → `xSemaphoreTake()` (kurangi slot). Keluar → `xSemaphoreGive()` (tambah slot). Saat semaphore = 0 → lantai penuh, redirect ke lantai lain.

Tampilkan occupancy real-time: `L1: 42/50 | L2: 38/50 | L3: 50/50 FULL`

**Penilaian:** Slot terkelola akurat, redirect saat penuh, occupancy display benar.

---

### Fitur 8: Revenue Counter Protection (dari Percobaan 08 — Mutex Shared Resource)

Total pendapatan adalah shared variable yang diakses saat kendaraan keluar:
- Fase 1 (demo): Tanpa mutex → race condition, pendapatan terakumulasi tidak akurat
- Fase 2 (fix): Dengan mutex → pendapatan akurat

Tampilkan perbandingan:
```
[DEMO] Without mutex: Total Rp 4.850.000 (expected Rp 5.000.000) — LOST!
[SAFE] With mutex:    Total Rp 5.000.000 (expected Rp 5.000.000) — CORRECT!
```

**Penilaian:** Race condition terdemonstrasikan, mutex fix terbukti akurat.

---

### Fitur 9: Priority-Aware Access (dari Percobaan 09 — Priority Inversion)

Implementasikan skenario priority inversion dan solusinya:
- Task Low: background statistics gathering (menggunakan shared data)
- Task Med: display update (tidak akses shared data)
- Task High: alarm handler (perlu akses shared data segera)

Demonstrasikan: tanpa priority inheritance → High tertunda. Dengan mutex (priority inheritance) → High dilayani lebih cepat.

**Penilaian:** Priority inversion terdemonstrasikan, priority inheritance memperbaiki.

---

### Fitur 10: Nested Config Access (dari Percobaan 10 — Recursive Mutex)

Konfigurasi sistem parkir memiliki nested access:
- `update_floor_config()` → takes mutex → calls `update_slot_range()` → takes mutex lagi
- Gunakan recursive mutex agar nested call tidak deadlock
- Tampilkan nesting level saat akses

**Penilaian:** Nested lock berfungsi tanpa deadlock, nesting level terukur.

---

### Fitur 11: Vehicle Flow Pipeline (dari Percobaan 11 — Producer-Consumer)

Implementasikan bounded buffer untuk vehicle flow:
- 2 Gate Entry (producer) + 2 Gate Exit (producer) menghasilkan event
- 1 Central Controller (consumer) memproses semua event
- Queue berkapasitas 8 (bounded buffer)
- Saat peak hour, monitor queue fill level dan potential overflow
- Statistik: throughput per gate, average queue fill, max wait time

**Penilaian:** Producer-consumer berjalan, statistik throughput akurat.

---

### Fitur 12: Configuration Database (dari Percobaan 12 — Reader-Writer)

Database konfigurasi parkir (tarif, kapasitas, jadwal) menggunakan reader-writer lock:
- **Reader tasks:** Display, Statistics, Reporting — bisa baca bersamaan
- **Writer tasks:** Admin Config, Auto-Tariff Adjustment — exclusive access

Statistik: read/write count, max concurrent readers, writer starvation check.

**Penilaian:** Multiple reader bersamaan, writer exclusive, statistik benar.

---

## Integrasi Sistem

Semua 12 fitur harus terintegrasi dalam satu codebase. Alur utama:

```
[Gate Sensor ISR] → entry_queue → [Controller Task]
                                        ↓
                    [Counting Semaphore: slot allocation]
                                        ↓
                    command_queue → [Gate Actuator Task]
                                        ↓
                    status_queue → [Display Task]
                                        ↓
                    [Mutex: Revenue] + [RW Lock: Config]
```

---

## Ketentuan Pengerjaan

1. **Pilih SATU platform** — ESP32 atau STM32.
2. **Semua 12 fitur** harus diimplementasikan.
3. Gunakan **PlatformIO + VS Code**.
4. Sensor dan gate boleh **disimulasikan** (random/timer/button).
5. Kode harus **terdokumentasi** — setiap fitur diberi komentar.
6. **Race condition** pada Fitur 8 harus didemonstrasikan dulu sebelum fix.

---

## Rubrik Penilaian

| Komponen | Bobot | Kriteria A (90-100) | Kriteria B (75-89) | Kriteria C (60-74) | Kriteria D (<60) |
|----------|-------|--------------------|--------------------|--------------------|--------------------|
| Queue Communication | 25% | 5 tipe queue (basic, struct, multiple, ISR, set) berfungsi | 4 tipe queue | 2-3 tipe queue | ≤ 1 tipe |
| Semaphore & Mutex | 25% | Binary, counting, mutex, priority inv., recursive mutex semua benar | 4 dari 5 mekanisme | 2-3 mekanisme | ≤ 1 |
| Design Patterns | 20% | Producer-consumer + reader-writer terintegrasi, statistik lengkap | Kedua pattern ada, statistik parsial | 1 pattern | Tidak ada |
| System Integration | 20% | Semua 12 fitur connected, demo berjalan lancar | 10 fitur connected | 7-9 fitur | < 7 fitur |
| Code Quality | 10% | Terdokumentasi, modular, error handling baik | Dokumentasi ada, sedikit error handling | Kode jalan tapi tidak terdokumentasi | Kode tidak terstruktur |

---

## Referensi

1. FreeRTOS Queue API — https://www.freertos.org/a00018.html
2. FreeRTOS Semaphore/Mutex API — https://www.freertos.org/a00113.html
3. Mastering the FreeRTOS Real Time Kernel — Richard Barry
4. AN4631 — Using FreeRTOS on STM32, STMicroelectronics
5. ESP-IDF FreeRTOS Documentation — Espressif Systems

---

*Project Modul 09 — FreeRTOS Queue & Semaphore | Praktikum Sistem Embedded | 2025/2026*
