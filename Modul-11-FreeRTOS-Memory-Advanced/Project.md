# Project Modul 11: FreeRTOS Memory Management & Advanced Features

## Praktikum Sistem Embedded

---

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 11 — FreeRTOS Memory & Advanced |
| **Platform** | ESP32 DevKit V1 *atau* STM32 Blue Pill |
| **Durasi** | 2 minggu sejak modul diberikan |
| **Tipe** | Project Mandiri |
| **Bobot Nilai** | 25% dari total nilai modul |

---

## Deskripsi Umum

Project ini mengintegrasikan **seluruh 12 percobaan** memory management dan advanced features FreeRTOS ke dalam satu sistem embedded. Mahasiswa diminta merancang sistem yang robust, hemat memori, dan mampu mendiagnosa dirinya sendiri.

---

## Soal Cerita: "Sistem Avionik Mini Drone Inspeksi — SkyWatch"

### Latar Belakang

PT AeroNusantara adalah startup yang mengembangkan drone inspeksi infrastruktur — tower BTS, tiang listrik, dan jembatan. Drone mereka, **SkyWatch-1**, menggunakan ESP32/STM32 sebagai flight controller sekunder yang menangani telemetri, logging, dan health monitoring.

Bulan lalu, drone jatuh saat inspeksi tower BTS di Kalimantan. Investigasi menemukan:
- **Memory leak** pada task sensor — heap perlahan habis selama 45 menit penerbangan
- **Stack overflow** pada task logging saat menyimpan data besar
- **Race condition** pada shared telemetry struct — data korup dikirim ke ground station
- Tidak ada **health monitoring** sehingga masalah baru terdeteksi setelah crash

Kerugiannya Rp 120 juta (drone + biaya evakuasi). Tim engineering ditugaskan membangun **SkyWatch-2** dengan sistem self-diagnostic komprehensif sehingga drone bisa mendarat darurat **sebelum** terjadi kegagalan fatal.

### Spesifikasi SkyWatch-2

- **Durasi terbang:** Maksimal 30 menit
- **Memori:** Sangat terbatas (ESP32: 320KB DRAM, STM32: 20KB SRAM)
- **Requirement:** Zero crash tolerance — harus self-recovery atau safe landing
- **Sensor:** IMU, barometer, GPS, baterai, ultrasonic (semua disimulasikan)
- **Logging:** Data flight ke buffer, siap download setelah landing

---

## Tugas dan Spesifikasi

Implementasikan **SkyWatch-2 Health Monitor** dengan fitur-fitur berikut:

---

### Fitur 1: Heap Health Dashboard (dari Percobaan 01 — Heap Monitor)

Monitor heap setiap 2 detik:
```
[HEAP] Free: 45320/81920 | Min-Ever: 38400 | Largest: 32768 | Frag: 12%
```

Klasifikasi status:
- **GREEN** (free > 50%): Normal
- **YELLOW** (free 25-50%): Warning, kurangi non-essential task
- **RED** (free < 25%): Critical, mulai emergency procedures

**Penilaian:** Heap stats akurat, klasifikasi otomatis sesuai threshold.

---

### Fitur 2: Allocation Pattern Analyzer (dari Percobaan 02 — Memory Allocation)

Setiap alokasi dicatat:
- Perbandingan waktu `pvPortMalloc()` vs pool allocation
- Deteksi pattern alokasi buruk (banyak alloc/free berbeda ukuran → fragmentasi)
- Print warning jika fragmentasi > 30%

Demonstrasikan: alokasi sequential → free alternating → fragmentasi naik → warning.

**Penilaian:** Allocation pattern terukur, fragmentasi terdeteksi dengan warning.

---

### Fitur 3: Stack Guardian (dari Percobaan 03 — Stack Overflow)

Setiap task dipantau stack usage-nya:
```
[STACK] TaskSensor: 234/512 (45%) OK
[STACK] TaskLogger: 480/512 (93%) WARNING! 
[STACK] TaskComms:  256/512 (50%) OK
```

Jika usage > 85% → **WARNING**. `vApplicationStackOverflowHook()` aktif sebagai safety net. Jika overflow terdeteksi → log nama task → safe landing mode.

**Penilaian:** Stack monitoring per task, warning sebelum overflow, hook terpasang.

---

### Fitur 4: Static Allocation for Critical Tasks (dari Percobaan 04 — Static Allocation)

Task-task kritis harus menggunakan **static allocation** agar tidak bergantung pada heap:
- `TaskSafetyMonitor` — static (tidak pernah gagal karena heap habis)
- `TaskEmergencyLanding` — static (tersedia bahkan saat heap penuh)

Task non-kritis (display, logging) boleh dynamic.

Tunjukkan bahwa task kritis berhasil dibuat tanpa mengubah free heap.

**Penilaian:** Task kritis menggunakan static allocation, heap usage tidak berubah.

---

### Fitur 5: Telemetry Memory Pool (dari Percobaan 05 — Memory Pool)

Data telemetri menggunakan **fixed-size memory pool** (bukan malloc):
- Pool: 20 blok × 64 bytes
- Sensor task alokasi blok → isi data → kirim ke queue
- Processor task terima → proses → kembalikan blok ke pool

Benchmark: pool alloc/free vs pvPortMalloc — pool harus lebih cepat.

**Penilaian:** Pool berfungsi, throughput lebih cepat dari malloc, zero fragmentasi.

---

### Fitur 6: Sensor Data Stream (dari Percobaan 06 — Stream Buffer)

IMU data (accelerometer, gyroscope) dikirim sebagai byte stream:
- Stream buffer 256 bytes, trigger level 32 bytes
- Sensor task menulis variable-length packets
- Processing task membaca saat trigger level tercapai

Monitor fill level — jika terlalu tinggi, sensor terlalu cepat (adjust rate).

**Penilaian:** Stream buffer berfungsi, trigger level tepat, fill level termonitor.

---

### Fitur 7: Command Message System (dari Percobaan 07 — Message Buffer)

Ground station mengirim command sebagai message (variable size):
- Message types: `CMD_START_LOG`, `CMD_STOP`, `CMD_CALIBRATE`, `CMD_STATUS`, `CMD_LAND`
- Setiap message memiliki header type + payload berbeda ukuran
- Receiver mendecode berdasarkan type

Tunjukkan 3+ tipe message dikirim dan diterima dengan benar.

**Penilaian:** Message buffer menerima pesan utuh, decode berdasarkan type benar.

---

### Fitur 8: Telemetry Data Protection (dari Percobaan 08 — Critical Section)

Shared telemetry struct diakses oleh multiple task:
- Fase 1 (demo): Tanpa proteksi → data tearing terdeteksi
- Fase 2 (fix): Critical section → zero corruption

Benchmark critical section vs mutex — critical section harus lebih cepat.

Pada ESP32, gunakan spinlock (`portMUX_TYPE`) untuk dual-core safety.

**Penilaian:** Data tearing didemonstrasikan, critical section memperbaiki, benchmark ada.

---

### Fitur 9: Anti-Fragmentation System (dari Percobaan 09 — Heap Fragmentation)

Implementasikan monitor fragmentasi real-time:
- Formula: `frag = 1 - (largest_block / total_free) × 100%`
- Jika frag > 30% → WARNING, kurangi dynamic alloc
- Jika frag > 60% → CRITICAL, switch ke pool/static only
- Demonstrasikan coalescence setelah cleanup

**Penilaian:** Fragmentasi terukur real-time, threshold warning benar, coalescence diamati.

---

### Fitur 10: Memory Region Info (dari Percobaan 10 — PSRAM/External RAM)

**ESP32:** Tampilkan region info: DRAM free/total, SPIRAM free/total, DMA-capable. Benchmark DRAM vs SPIRAM speed.

**STM32:** Tampilkan memory map: Flash, SRAM, stack pointer, heap boundaries. Print address analysis.

Gunakan informasi ini untuk memutuskan di region mana data telemetri disimpan.

**Penilaian:** Memory region terenumerasi lengkap, benchmark/address analysis ada.

---

### Fitur 11: Leak Detector (dari Percobaan 11 — Memory Leak Detection)

Sistem harus mendeteksi memory leak **sebelum** menyebabkan crash:
- Snapshot heap setiap 2 detik
- Jika free heap turun 3+ kali berturut-turut → "LEAK DETECTED!"
- Identifikasi leak rate (bytes/detik)
- Estimasi time-to-exhaustion: `remaining / leak_rate` detik
- Jika time-to-exhaustion < 120 detik → trigger emergency landing

Demonstrasikan: "leaky task" menyebabkan leak → detector alarm → landing warning.

**Penilaian:** Leak terdeteksi otomatis, rate dan TTL akurat, emergency trigger berjalan.

---

### Fitur 12: Flight Dashboard (dari Percobaan 12 — System Dashboard)

Dashboard komprehensif setiap 5 detik:
```
╔══════════ SKYWATCH-2 HEALTH MONITOR ══════════╗
║ Uptime: 00:15:32  |  Mode: FLYING              ║
╠════════════════════════════════════════════════╣
║ HEAP: 45320/81920 (55%) [GREEN]  Frag: 12%    ║
║ LEAK: Not detected | Min-Ever: 38400           ║
╠════════════════════════════════════════════════╣
║ Task           State  Pri  Stack   CPU%        ║
║ SafetyMon      Run    5    120     2.1%        ║
║ SensorIMU      Blk    3    234     15.3%       ║
║ TelemetryTx    Rdy    3    156     8.7%        ║
║ Logger         Blk    2    312     5.2%        ║
║ Display        Rdy    1    89      3.4%        ║
║ IDLE           Rdy    0    45      65.3%       ║
╠════════════════════════════════════════════════╣
║ Pool: 14/20 free | Stream: 45/256 fill         ║
║ MsgBuf: 380/512 avail | Queue: 3/10 items      ║
╚════════════════════════════════════════════════╝
```

**Penilaian:** Dashboard lengkap, semua metrik terisi, status klasifikasi benar.

---

## Ketentuan Pengerjaan

1. **Pilih SATU platform** — ESP32 atau STM32.
2. **Semua 12 fitur** harus diimplementasikan dalam satu codebase.
3. Gunakan **PlatformIO + VS Code**.
4. Sensor boleh **disimulasikan** (random values + timer).
5. Kode harus menggunakan **minimal**: 5 task, 2 queue, 1 mutex, 1 timer, 1 pool, 1 stream/message buffer.
6. Task kritis (SafetyMonitor, EmergencyLanding) **wajib** static allocation.

---

## Rubrik Penilaian

| Komponen | Bobot | Kriteria A (90-100) | Kriteria B (75-89) | Kriteria C (60-74) | Kriteria D (<60) |
|----------|-------|--------------------|--------------------|--------------------|--------------------|
| Memory Management | 25% | Heap monitor, pool, static alloc, fragmentation analysis | 3 dari 4 fitur | 2 fitur | ≤1 fitur |
| Buffer & Communication | 20% | Stream + message buffer + critical section berfungsi | 2 dari 3 | 1 dari 3 | Tanpa buffer |
| Safety & Detection | 25% | Stack guard, leak detector, watchdog, emergency trigger | 3 dari 4 | 2 | ≤1 |
| Dashboard & Integration | 20% | Dashboard lengkap, 12 fitur terintegrasi | 10 fitur, dashboard parsial | 7-9 fitur | <7 fitur |
| Code Quality | 10% | Modular, documented, static alloc untuk kritis | Dokumentasi ada | Kode jalan tapi tidak terstruktur | Tidak compilable |

---

## Referensi

1. FreeRTOS Memory Management — https://www.freertos.org/a00111.html
2. FreeRTOS Stream & Message Buffers — https://www.freertos.org/RTOS-stream-message-buffers.html
3. Mastering the FreeRTOS Real Time Kernel — Richard Barry
4. ESP-IDF Heap Allocation — Espressif Systems
5. AN4838 — STM32 Memory Protection — STMicroelectronics

---

*Project Modul 11 — FreeRTOS Memory & Advanced | Praktikum Sistem Embedded | 2025/2026*
