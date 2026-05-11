# Project Modul 10: Advanced RTOS Industrial System

## Informasi Project

| Item | Keterangan |
|---|---|
| Modul | 10 — FreeRTOS Advanced |
| Struktur praktikum | 25 eksperimen: 10 STM32 + 10 ESP32 + 5 Multi STM32-ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Tema | Industrial monitoring and control dengan fitur RTOS lanjutan |
| Durasi | 2 minggu |
| Output | Sistem hardware, source code, laporan, video demo |

---

## 1. Deskripsi Umum

Project Modul 10 menggabungkan seluruh materi FreeRTOS Advanced menjadi **Advanced RTOS Industrial System** untuk monitoring dan kontrol industri. ESP32 dan STM32 bekerja bersama menggunakan objek RTOS lanjutan untuk menangani event, timer, notifikasi, sinkronisasi, dan manajemen memory secara efisien.

Sistem wajib selaras dengan struktur final praktikum:
- **10 eksperimen STM32**: Event Groups, Software Timers, Task Notifications, Semaphore varieties, Mutex/Priority Inheritance, Memory Management, Static Allocation, Critical Section+Task Suspension, Message+Stream Buffers, Queue Sets.
- **10 eksperimen ESP32**: Topik yang sama dengan STM32, diimplementasikan pada platform ESP32.
- **5 eksperimen Multi STM32-ESP32**: Distributed Event Sync, Timer Network, Task Notification Pipeline, Priority Inheritance Network, Full Industrial System.

---

## 2. Skenario

Sebuah sistem monitoring pabrik harus menangani:
- Deteksi event button (panic button, mode switch) dengan Event Groups.
- Eksekusi periodik pengecekan sensor dengan Software Timers.
- Notifikasi task saat data siap dengan Task Notifications.
- Kontrol akses shared resource (kontroler aktuator) dengan Mutex/Semaphore.
- Logging data stream dengan Stream/Message Buffers.
- Pantau multiple input dengan Queue Sets.
- Manajemen memory efisien untuk sistem deterministik.

Data ditampilkan melalui LED indikator, serial monitor, dan dikirim antar MCU menggunakan objek RTOS yang tepat.

---

## 3. Arsitektur Sistem

```text
                PC Monitor
                     │
               ┌─────▼─────┐          UART
               │   ESP32   │◄────────────────────────┐
               │ Gateway   │                         │
               └─────┬─────┘                         │
                     │ RTOS Objects                   │
        ┌────────────┼─────────────┐                  │
        │            │             │                  │
    Event Groups  Software Timers  Task Notif        │
                     │                               │
               ┌───────────────────┐                  │
               │       STM32       │──────────────────┘
               │ RTOS Node         │
               └─────┬─────────────┘
                     │ RTOS Objects
         ┌────────────┼─────────────┬─────────────┐
         │            │             │             │
   Mutex/Semaphore  Message Buffer  Stream Buffer  Queue Set
```

---

## 4. Peran Platform

### STM32
- Node industri deterministik.
- STM32F103C8 / STM32F401CC / STM32F411CE.
- Implementasi 10 eksperimen STM32:
  - STM32_01: Event Groups untuk button event.
  - STM32_02: Software Timers untuk pengecekan periodik.
  - STM32_03: Task Notifications untuk data ready.
  - STM32_04: Semaphore varieties untuk kontrol akses.
  - STM32_05: Mutex dan Priority Inheritance untuk shared resource.
  - STM32_06: Memory Management pilih heap tepat.
  - STM32_07: Static Allocation untuk task/queue kritis.
  - STM32_08: Critical Section dan Task Suspension untuk proteksi atomis.
  - STM32_09: Message Buffers dan Stream Buffers untuk data variabel.
  - STM32_10: Queue Sets untuk multiple input multiplexing.

### ESP32
- Gateway dan koordinator sistem.
- ESP32 DevKit dengan FreeRTOS ESP-IDF/Arduino.
- Implementasi 10 eksperimen ESP32 (topik sama dengan STM32).
- State machine berbasis Event Group: IDLE, ACTIVE, ALARM.
- Software Timer watchdog untuk deteksi timeout STM32.
- Queue Set untuk pantau multiple data source.
- Komunikasi antar MCU menggunakan Message Buffer protokol.

### Multi STM32-ESP32
- 5 eksperimen koordinasi antar MCU dengan advanced RTOS objects:
  - MULTI_01: Distributed Event Group dengan ACK protocol UART.
  - MULTI_02: ESP32 kontrol software timer STM32 via UART.
  - MULTI_03: Task Notification Pipeline end-to-end.
  - MULTI_04: Priority Inheritance dan Critical Section terdistribusi.
  - MULTI_05: Full Industrial System dengan semua objek RTOS.

---

## 5. Objek RTOS Wajib

| Objek RTOS | Fungsi | Platform |
|---|---|---|
| Event Groups | Sinkronisasi event button/mode/distributed | STM32_01, ESP32_01, MULTI_01 |
| Software Timers | Eksekusi periodik, watchdog, remote control | STM32_02, ESP32_02, MULTI_02 |
| Task Notifications | Notifikasi data ready, pipeline | STM32_03, ESP32_03, MULTI_03 |
| Binary/Counting Semaphore | Kontrol akses resource | STM32_04, ESP32_04 |
| Mutex + Priority Inheritance | Shared resource kritis, terdistribusi | STM32_05, ESP32_05, MULTI_04 |
| Heap_1-heap_5 | Manajemen memory | STM32_06, ESP32_06 |
| Static Allocation | Task/queue deterministik | STM32_07, ESP32_07, MULTI_05 |
| Critical Section + Task Suspend | Proteksi atomis, kontrol task | STM32_08, ESP32_08, MULTI_04 |
| Message Buffers | Pesan variabel antar task/MCU | STM32_09, ESP32_09, MULTI_03 |
| Stream Buffers | Byte stream data sensor/log | STM32_09, ESP32_09, MULTI_05 |
| Queue Sets | Pantau multiple input/MCU | STM32_10, ESP32_10, MULTI_05 |

---

## 6. Alur Operasi

### Startup
1. Init semua objek RTOS (Event Groups, Timers, Semaphores, Buffers, Queues).
2. Buat semua task dengan prioritas sesuai.
3. Start Software Timers.
4. Tampilkan status sistem via serial.

### Normal Loop
1. Task monitor button set event bit pada Event Group.
2. Software Timer trigger pengecekan periodik setiap 1 detik.
3. Task sensor kirim notifikasi ke task processing saat data siap.
4. Task kontrol akses shared resource dengan Mutex.
5. Task logging tulis data ke Stream/Message Buffer.
6. Task gateway pantau Queue Set untuk multiple input.

### Error Mode
1. Jika task crash, gunakan Watchdog (opsional) reset task.
2. Jika heap habis, gunakan Memory Management terbaik (heap_4).
3. Jika buffer penuh, implementasikan overrun handling.
4. Prioritas task disesuaikan dengan Priority Inheritance saat pakai Mutex.

---

## 7. Mapping 20 Eksperimen ke Project

| Eksperimen | Kontribusi project |
|---|---|
| STM32_01, ESP32_01 | Event Groups untuk button/mode event |
| STM32_02, ESP32_02 | Software Timers untuk pengecekan periodik |
| STM32_03, ESP32_03 | Task Notifications untuk data ready |
| STM32_04, ESP32_04 | Semaphore untuk kontrol akses resource |
| STM32_05, ESP32_05 | Mutex + Priority Inheritance untuk shared resource |
| STM32_06, ESP32_06 | Memory Management pilih heap tepat |
| STM32_07, ESP32_07 | Static Allocation untuk task/queue kritis |
| STM32_08, ESP32_08 | Message Buffers untuk pesan variabel |
| STM32_09, ESP32_09 | Stream Buffers untuk data sensor stream |
| STM32_10, ESP32_10 | Queue Sets untuk multiple input monitoring |

---

## 8. Fitur Minimal Wajib

1. Event Groups sinkronisasi minimal 2 event.
2. Software Timers berjalan periodik tanpa blocking task lain.
3. Task Notifications lebih efisien dari semaphore (dicatat di laporan).
4. Mutex mencegah priority inversion (dibuktikan dengan eksperimen).
5. Memory Management pilih heap yang tepat (heap_4 direkomendasi).
6. Static Allocation untuk task/queue kritis.
7. Message Buffer menangani pesan variabel 1-128 byte.
8. Stream Buffer menangani byte stream tanpa kehilangan data.
9. Queue Sets pantau minimal 2 queue + 1 semaphore.
10. Komunikasi antar MCU menggunakan objek RTOS yang tepat.

---

## 9. Output Tampilan Minimum

### Serial Monitor
```text
STM32_INIT: EventGroups, Timers, Mutex, Buffers, Queues created
ESP32_INIT: All RTOS objects ready
EVENT: Button pressed, bit 0 set
TIMER: Periodic check triggered
NOTIF: Data ready from sensor
MUTEX: Shared resource accessed by High priority task
BUFFER: 128 bytes received from stream
QUEUE_SET: Event from Queue1, Semaphore triggered
```

### LED Indikator
- LED PB0 (STM32): Event received.
- LED PB1 (STM32): Timer triggered.
- LED GPIO2 (ESP32): Notification received.
- LED GPIO4 (ESP32): Queue Set event.

---

## 10. Rubrik Penilaian

| Komponen | Bobot |
|---|---:|
| Implementasi Event Groups + Software Timers | 15% |
| Task Notifications + Semaphore/Mutex | 15% |
| Memory Management + Static Allocation | 10% |
| Message/Stream Buffers | 15% |
| Queue Sets + Multi-input monitoring | 15% |
| Integrasi ESP32 + STM32 | 10% |
| Project stability dan error handling | 10% |
| Kode modular, laporan, video | 10% |

---

## 11. Struktur Kode Disarankan

```text
project_modul_10/
├─ esp32/
│  ├─ event_groups.c
│  ├─ software_timers.c
│  ├─ task_notifications.c
│  ├─ semaphores.c
│  ├─ message_stream_buffers.c
│  └─ queue_sets.c
├─ stm32/
│  ├─ event_groups.c
│  ├─ software_timers.c
│  ├─ task_notifications.c
│  ├─ mutex_priority.c
│  ├─ memory_management.c
│  └─ static_allocation.c
└─ shared/
    ├─ rtos_objects.h
    └─ protocol_rtos.md
```

Dokumen baru tidak wajib dibuat; struktur ini hanya panduan implementasi.

---

## 12. Checklist Demo

- [ ] Menunjukkan hardware ESP32 + STM32.
- [ ] Menunjukkan LED indikator untuk setiap objek RTOS.
- [ ] Event Groups merespons button press.
- [ ] Software Timers toggle LED periodik tanpa blocking.
- [ ] Task Notifications lebih cepat dari semaphore.
- [ ] Mutex mencegah priority inversion.
- [ ] Memory Management menunjukkan pilihan heap tepat.
- [ ] Static Allocation task/queue berjalan tanpa heap dinamis.
- [ ] Message Buffer menerima pesan variabel.
- [ ] Stream Buffer menerima byte stream utuh.
- [ ] Queue Sets menangani multiple input.
- [ ] Komunikasi antar MCU berjalan.

---

## 13. Catatan Keselamatan dan Keandalan

- Jangan ada task dengan prioritas sama yang mengakses shared resource tanpa proteksi.
- Pastikan heap tidak habis (gunakan `xPortGetFreeHeapSize()`).
- Gunakan Priority Inheritance untuk Mutex shared resource kritis.
- Buffer (Message/Stream) harus cukup besar untuk menghindari overrun.
- Queue Sets harus mencakup semua objek yang dipantau.
- Task notifications hanya untuk unicast, bukan broadcast event.
