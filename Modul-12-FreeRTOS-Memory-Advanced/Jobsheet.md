# Jobsheet Modul 12: FreeRTOS Memory Management & Advanced Features

## 🎯 Tujuan Praktikum

1. Mahasiswa mampu memonitor penggunaan memori **Heap** dan **Stack** pada sistem FreeRTOS
2. Mahasiswa mampu mendeteksi dan menangani **Stack Overflow** dan kegagalan alokasi memori
3. Mahasiswa mampu menggunakan **Static Memory Allocation** untuk sistem deterministik
4. Mahasiswa mampu mengimplementasikan **Memory Pool Pattern** untuk menghindari fragmentasi
5. Mahasiswa mampu menggunakan **Stream Buffer** dan **Message Buffer** untuk transfer data efisien
6. Mahasiswa mampu menggunakan **Critical Section** untuk melindungi shared resource
7. Mahasiswa memahami arsitektur memori **ESP32** (multi-heap, PSRAM) dan **STM32** (SRAM, linker)
8. Mahasiswa mampu mendeteksi **Memory Leak** dan **Heap Fragmentation**

## ⚠️ Peringatan

- Modul ini melakukan manipulasi memori intensif. **Hard Fault** mungkin terjadi — siap reset hardware
- Pastikan Serial Monitor terhubung pada baudrate **115200**
- **Framework**: ESP-IDF untuk ESP32, STM32Cube HAL untuk STM32 (**BUKAN Arduino**)
- Setiap program dilengkapi script Python untuk debug dan analisis data

## 🔧 Peralatan

| Komponen | Jumlah | Keterangan |
|----------|--------|------------|
| ESP32 DevKit | 1 | atau WROVER untuk PSRAM |
| STM32 Blue Pill (F103C8) | 1 | 20 KB SRAM |
| LED | 2 | Untuk indikator visual |
| Push Button | 1 | Opsional |
| Kabel USB | 2 | Untuk programming & serial |
| Breadboard + Jumper | 1 set | Koneksi komponen |

---

## 📋 Daftar Percobaan

### Platform ESP32 (ESP-IDF)

| No | Program | Hardware | Konsep Utama |
|----|---------|----------|-------------|
| 01 | ESP32_01_Heap_Monitor | Serial | `heap_caps_get_free_size()`, multi-heap info |
| 02 | ESP32_02_Memory_Allocation | Serial | `pvPortMalloc()`/`vPortFree()`, pattern alokasi |
| 03 | ESP32_03_Stack_Overflow_Detect | Serial | Stack overflow hook, deliberate crash |
| 04 | ESP32_04_Static_Allocation | 1x LED | `xTaskCreateStatic()`, no dynamic alloc |
| 05 | ESP32_05_Memory_Pool | Serial | Fixed-size block pool, queue free-list |
| 06 | ESP32_06_Stream_Buffer | Serial + LED | `xStreamBufferCreate()`, byte stream |
| 07 | ESP32_07_Message_Buffer | Serial | `xMessageBufferCreate()`, discrete messages |
| 08 | ESP32_08_Critical_Section | Serial + 2x LED | `taskENTER_CRITICAL()`, data protection |
| 09 | ESP32_09_Heap_Fragmentation | Serial | Fragmentasi demo, pattern analysis |
| 10 | ESP32_10_PSRAM_External_RAM | Serial | `heap_caps_malloc(MALLOC_CAP_SPIRAM)` |
| 11 | ESP32_11_Memory_Leak_Detection | Serial | Heap tracing, leak identification |
| 12 | ESP32_12_System_Dashboard | Serial + OLED | Capstone: task + heap + stack dashboard |

### Platform STM32 (STM32Cube HAL)

| No | Program | Hardware | Konsep Utama |
|----|---------|----------|-------------|
| 01 | STM32_01_Heap_Monitor | Serial | `xPortGetFreeHeapSize()`, heap watermark |
| 02 | STM32_02_Memory_Allocation | Serial | `pvPortMalloc()`/`vPortFree()` with heap_4 |
| 03 | STM32_03_Stack_Overflow_Detect | Serial | Stack overflow hook, stack canary |
| 04 | STM32_04_Static_Allocation | 1x LED | `xTaskCreateStatic()`, no heap usage |
| 05 | STM32_05_Memory_Pool | Serial | Pool pattern, integrity checking |
| 06 | STM32_06_Stream_Buffer | Serial + LED | `xStreamBufferCreate()`, trigger level |
| 07 | STM32_07_Message_Buffer | Serial | `xMessageBufferCreate()`, multi-type messages |
| 08 | STM32_08_Critical_Section | Serial + 2x LED | Corruption demo: unprotected vs protected |
| 09 | STM32_09_Heap_Fragmentation | Serial | Multi-phase fragmentation, coalescing |
| 10 | STM32_10_PSRAM_External_RAM | Serial | Memory region explorer, FSMC concept |
| 11 | STM32_11_Memory_Leak_Detection | Serial | Allocation tracker, leak reports |
| 12 | STM32_12_System_Dashboard | Serial | `vTaskList()`, `vTaskGetRunTimeStats()` |

---

## 🛠️ Percobaan 1: Heap Monitor

### Tujuan
Memantau penggunaan memori heap secara real-time menggunakan FreeRTOS dan platform API.

### Prosedur
1. Buka project `ESP32_01_Heap_Monitor` atau `STM32_01_Heap_Monitor`
2. Compile dan upload: `pio run -t upload`
3. Buka Serial Monitor (115200 baud)
4. Amati output: free heap, minimum-ever free heap, largest free block
5. Perhatikan perubahan heap saat task mengalokasi dan membebaskan memori

### Output yang Diharapkan (ESP32)
```
[HEAP_MON] Free heap: 265432 bytes
[HEAP_MON] Min-ever free: 261204 bytes
[heap_caps] DEFAULT free: 265432 bytes
[heap_caps] INTERNAL free: 265432 bytes
[heap_caps] Largest free block: 113792 bytes
```

### Output yang Diharapkan (STM32)
```
[Heap] Free: 14208 bytes
[Heap] Min-ever free: 13856 bytes
[Heap] Allocated 1024 bytes test block
[Heap] Free after alloc: 13168 bytes
```

### Analisis
1. Mengapa `xPortGetMinimumEverFreeHeapSize()` selalu ≤ `xPortGetFreeHeapSize()`?
2. Pada ESP32, apa perbedaan `MALLOC_CAP_DEFAULT` vs `MALLOC_CAP_INTERNAL`?
3. Berapa besar overhead per alokasi di heap_4?

### Script Python
Jalankan `debug_heap_monitor.py` untuk visualisasi real-time:
```bash
python3 debug_heap_monitor.py --port /dev/ttyUSB0
```

---

## 🛠️ Percobaan 2: Memory Allocation

### Tujuan
Memahami pattern alokasi/dealokasi dan dampaknya pada heap.

### Prosedur
1. Buka project `*_02_Memory_Allocation`
2. Compile dan upload
3. Amati output: alokasi sequential, random, dan efek pada free heap
4. Bandingkan alamat yang di-return oleh pvPortMalloc

### Analisis
1. Apakah alamat yang di-return berurutan? Mengapa?
2. Setelah free semua blok, apakah free heap kembali ke nilai awal?
3. Apa yang terjadi jika pvPortMalloc dipanggil dengan size > free heap?

---

## 🛠️ Percobaan 3: Stack Overflow Detection

### Tujuan
Mengaktifkan dan menguji mekanisme deteksi stack overflow FreeRTOS.

### Prosedur
1. Buka project `*_03_Stack_Overflow_Detect`
2. Perhatikan `configCHECK_FOR_STACK_OVERFLOW = 2` di FreeRTOSConfig.h
3. Compile dan upload
4. Amati: task dengan stack kecil akan trigger overflow hook
5. Amati stack high water mark sebelum crash

### Output yang Diharapkan
```
[INIT] Normal task stack HWM: 180 words
[INIT] Starting dangerous task...
[DANGER] Recursion depth: 10, HWM: 45
[DANGER] Recursion depth: 20, HWM: 12
STACK OVERFLOW detected in task: DangerTask
```

### Analisis
1. Apa perbedaan Method 1 vs Method 2 stack overflow detection?
2. Mengapa Method 2 lebih akurat tapi lebih lambat?
3. Berapa minimum stack high water mark yang aman?

---

## 🛠️ Percobaan 4: Static Allocation

### Tujuan
Membuat task dan queue tanpa menggunakan heap (static allocation).

### Prosedur
1. Buka project `*_04_Static_Allocation`
2. Perhatikan `configSUPPORT_STATIC_ALLOCATION = 1` di FreeRTOSConfig.h
3. Compile dan upload
4. Amati: heap usage tetap konstan setelah inisialisasi

### Analisis
1. Apa keuntungan static allocation dibanding dynamic?
2. Mengapa harus menyediakan `vApplicationGetIdleTaskMemory()`?
3. Kapan sebaiknya menggunakan static vs dynamic allocation?

---

## 🛠️ Percobaan 5: Memory Pool

### Tujuan
Mengimplementasikan fixed-size memory pool untuk menghindari fragmentasi.

### Prosedur
1. Buka project `*_05_Memory_Pool`
2. Pool terdiri dari 8 blok × 64 bytes
3. Amati: alokasi/dealokasi tanpa fragmentasi
4. Monitor pool utilization

### Analisis
1. Mengapa memory pool lebih efisien dari heap untuk alokasi seragam?
2. Apa tradeoff ukuran blok pool?
3. Bagaimana cara menentukan jumlah blok yang optimal?

---

## 🛠️ Percobaan 6: Stream Buffer

### Tujuan
Menggunakan Stream Buffer untuk transfer byte stream antar task.

### Prosedur
1. Buka project `*_06_Stream_Buffer`
2. Compile dan upload
3. Amati: sender mengirim data variable-length, receiver membaca batch

### Analisis
1. Apa fungsi trigger level pada Stream Buffer?
2. Mengapa Stream Buffer hanya support single writer/single reader?
3. Kapan menggunakan Stream Buffer vs Queue?

---

## 🛠️ Percobaan 7: Message Buffer

### Tujuan
Menggunakan Message Buffer untuk transfer pesan diskrit berbeda ukuran.

### Prosedur
1. Buka project `*_07_Message_Buffer`
2. Compile dan upload
3. Amati: pesan berbeda tipe (command, data, status) dikirim dan diterima

### Analisis
1. Berapa overhead per message di Message Buffer?
2. Apa perbedaan Message Buffer vs Stream Buffer?
3. Mengapa Message Buffer dibangun di atas Stream Buffer?

---

## 🛠️ Percobaan 8: Critical Section

### Tujuan
Memahami dan menggunakan critical section untuk melindungi shared resource.

### Prosedur
1. Buka project `*_08_Critical_Section`
2. Compile dan upload
3. Fase 1: Tanpa proteksi → amati data corruption
4. Fase 2: Dengan `taskENTER_CRITICAL` → data konsisten

### Analisis
1. Mengapa data bisa corrupt tanpa critical section?
2. Apa dampak critical section terhadap interrupt latency?
3. Kapan menggunakan critical section vs mutex?

---

## 🛠️ Percobaan 9: Heap Fragmentation

### Tujuan
Mendemonstrasikan dan menganalisis heap fragmentation.

### Prosedur
1. Buka project `*_09_Heap_Fragmentation`
2. Compile dan upload
3. Amati: alokasi alternating sizes → free setiap blok kedua → fragmentasi
4. Hitung fragmentation index

### Analisis
1. Mengapa total free heap cukup tapi alokasi besar gagal?
2. Bagaimana heap_4 coalescing mengurangi fragmentasi?
3. Hitung fragmentation index dari output program

---

## 🛠️ Percobaan 10: PSRAM / External RAM

### Tujuan
- **ESP32**: Menggunakan PSRAM (SPIRAM) untuk alokasi memori besar
- **STM32**: Memahami memory region dan konsep external SRAM via FSMC

### Prosedur (ESP32)
1. Buka project `ESP32_10_PSRAM_External_RAM`
2. Jika menggunakan ESP32-WROVER (ada PSRAM): amati alokasi dari SPIRAM
3. Jika tidak ada PSRAM: program akan mendeteksi dan skip

### Prosedur (STM32)
1. Buka project `STM32_10_PSRAM_External_RAM`
2. Program mendemonstrasikan perbedaan memory region: .data, .bss, stack, heap
3. Amati alamat variabel di region berbeda

### Analisis
1. Apa perbedaan DRAM dan SPIRAM pada ESP32?
2. Mengapa PSRAM lebih lambat dari internal RAM?
3. Pada STM32, region mana yang bisa diakses DMA?

---

## 🛠️ Percobaan 11: Memory Leak Detection

### Tujuan
Mendeteksi memory leak menggunakan heap tracing dan allocation tracking.

### Prosedur
1. Buka project `*_11_Memory_Leak_Detection`
2. Compile dan upload
3. Amati: satu task secara sengaja melakukan leak (alloc tanpa free)
4. Detector task melaporkan heap yang berkurang

### Analisis
1. Bagaimana cara mendeteksi memory leak secara otomatis?
2. Apa overhead dari allocation tracking?
3. Pada ESP32, bagaimana menggunakan `esp_heap_trace` API?

---

## 🛠️ Percobaan 12: System Dashboard (CAPSTONE)

### Tujuan
Menggabungkan semua konsep FreeRTOS dari Modul 09-12 dalam satu system dashboard.

### Prosedur
1. Buka project `*_12_System_Dashboard`
2. Compile dan upload
3. Amati dashboard: task list, heap stats, stack watermarks, runtime stats
4. Jalankan Python script untuk visualisasi

### Output yang Diharapkan
```
╔══════════════════════════════════════╗
║     SYSTEM DASHBOARD - STM32        ║
╠══════════════════════════════════════╣
║ TASK LIST:                          ║
║ Name        State Pri Stack  Runtime║
║ Dashboard   Run    3   180   15%    ║
║ Worker1     Blk    2   230    5%    ║
║ Worker2     Blk    2   225    8%    ║
║ IDLE        Rdy    0   118   72%    ║
╠══════════════════════════════════════╣
║ HEAP: Free=12048 Min=11200 Frag=12%║
║ UPTIME: 00:05:32                    ║
╚══════════════════════════════════════╝
```

### Analisis
1. Task mana yang menggunakan CPU paling banyak? Mengapa?
2. Apakah ada indikasi memory leak dari trend heap?
3. Stack mana yang paling berisiko overflow?

---

## 🐍 Script Python untuk Debug & Analisis

Setiap percobaan dilengkapi script Python yang bisa dijalankan:

```bash
# Install dependencies
pip install pyserial matplotlib

# Jalankan script (contoh)
python3 debug_heap_monitor.py --port /dev/ttyUSB0 --baud 115200

# Simpan data ke CSV
python3 debug_heap_monitor.py --port /dev/ttyUSB0 --log heap_data.csv

# Mode real-time plot
python3 debug_system_dashboard.py --port /dev/ttyUSB0 --plot
```

---

## 📝 Tugas

1. **Analisis Heap**: Jalankan percobaan 01 dan 09 berturut-turut. Buat grafik heap usage vs waktu. Identifikasi titik-titik kritis.

2. **Optimasi Stack**: Untuk percobaan 03, tentukan ukuran stack minimum yang aman untuk setiap task. Jelaskan reasoning-nya.

3. **Perbandingan**: Bandingkan performa (throughput, latency) antara Queue, Stream Buffer, dan Message Buffer dari percobaan 06 dan 07. Buat tabel perbandingan.

4. **Video Demo**: Rekam video 5-10 menit mendemonstrasikan percobaan 12 (System Dashboard) dengan penjelasan setiap metrik yang ditampilkan.
