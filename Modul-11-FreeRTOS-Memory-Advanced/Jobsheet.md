# Jobsheet Modul 11: FreeRTOS Memory Management & Advanced Features

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami manajemen heap** — Memonitor free heap, minimum ever free, largest free block, dan fragmentasi.
2. **Mengoptimalkan alokasi memori** — Menggunakan static allocation, memory pool, dan fixed-size block untuk menghindari fragmentasi.
3. **Mendeteksi masalah memori** — Stack overflow detection, memory leak detection, dan heap integrity check.
4. **Menggunakan stream & message buffer** — Transfer data variabel-length antar task tanpa queue.
5. **Merancang system dashboard** — Menggabungkan semua teknik monitoring ke dalam dashboard komprehensif.

---

## 2. Peralatan

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | Multi-region heap (DRAM, IRAM, PSRAM) |
| 2 | STM32 Blue Pill | 1 | Heap_4 allocator, 20KB SRAM |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | LED 5mm | 2 | Indikator status |
| 5 | Resistor 330Ω | 2 | Current limiting |
| 6 | Breadboard + kabel jumper | 1 set | |

---

## 3. Teori Singkat

FreeRTOS menyediakan **heap allocator** (heap_1 sampai heap_5) untuk manajemen memori dinamis. `pvPortMalloc()` dan `vPortFree()` adalah fungsi utama. **Fragmentasi** terjadi ketika ada cukup total free memory tapi block terbesar terlalu kecil.

**Static allocation** (`xTaskCreateStatic()`) menghindari heap sepenuhnya — semua memori dari array global. **Memory pool** menggunakan queue berisi pointer ke fixed-size blocks — O(1) alloc/free tanpa fragmentasi.

**Stream Buffer** mentransfer byte stream (seperti UART data), sedangkan **Message Buffer** mentransfer pesan diskrit dengan length header. Keduanya lebih ringan dari queue untuk data variabel-length.

**Critical section** (`taskENTER_CRITICAL()`) men-disable interrupt untuk proteksi ultra-cepat pada shared data. ESP32 menggunakan spinlock karena dual-core.

---

## 4. Langkah Percobaan

> **Catatan:** Serial Monitor 115200 baud. Semua percobaan fokus pada serial output — monitoring data heap, stack, dan performa.

---

### Percobaan 01: Heap Monitor

**Tujuan:** Memonitor penggunaan heap secara real-time — free, used, minimum ever free, largest block.

#### Langkah Kerja

1. Buka project `ESP32_01` atau `STM32_01`.
2. Program menampilkan heap stats setiap 2 detik.
3. Secara periodik mengalokasi dan membebaskan 1KB test block — amati perubahan.
4. Pada ESP32, amati multi-region: DRAM, IRAM, SPIRAM (jika ada).
5. Pada STM32, amati slot tracking — berapa blok sedang aktif.
6. Catat hubungan antara free heap dan minimum ever free.

#### Tabel Pengamatan

| Waktu | Free Heap | Min Ever Free | Largest Block | Active Alloc |
|-------|----------|--------------|--------------|-------------|
| 0s | | | | 0 |
| 2s (alloc 1KB) | | | | 1 |
| 4s (free 1KB) | | | | 0 |
| 6s (alloc 4KB) | | | | 1 |

#### Pertanyaan Analisa

1. Apa perbedaan "free heap" dan "minimum ever free heap"? Mengapa keduanya penting?
2. Mengapa "largest free block" bisa jauh lebih kecil dari "total free"?
3. Pada ESP32, apa perbedaan DRAM vs IRAM vs SPIRAM? Region mana untuk apa?
4. Berapa margin free heap yang dianggap aman untuk sistem production?

---

### Percobaan 02: Memory Allocation Pattern

**Tujuan:** Menganalisis pattern alokasi memori dan dampaknya terhadap fragmentasi.

#### Langkah Kerja

1. Buka project `ESP32_02` atau `STM32_02`.
2. **Pattern 1:** Alokasi sequential → free sequential (in-order). Amati heap recovery.
3. **Pattern 2:** Alokasi sequential → free alternating (skip ganjil/genap). Amati fragmentasi.
4. Pada ESP32, benchmark: `pvPortMalloc` vs `heap_caps_malloc` — mana lebih cepat?
5. Setelah Pattern 2, coba alokasi blok besar — gagal meskipun total free cukup!

#### Tabel Pengamatan

| Pattern | Total Free After | Largest Block | Can Alloc 8KB? | Fragmentation (%) |
|---------|-----------------|--------------|---------------|-------------------|
| Before alloc | | | | |
| After alloc 50× 256B | | | | |
| Free in-order | | | | |
| Free alternating | | | | |

#### Pertanyaan Analisa

1. Apa itu fragmentasi heap? Bagaimana cara menghitung persentasenya?
2. Mengapa free alternating menyebabkan fragmentasi lebih buruk dari free in-order?
3. Apa perbedaan heap_1, heap_2, heap_4, dan heap_5 di FreeRTOS?
4. Bagaimana heap_4 melakukan block coalescence (merge adjacent free blocks)?

---

### Percobaan 03: Stack Overflow Detection

**Tujuan:** Mendeteksi stack overflow menggunakan configCHECK_FOR_STACK_OVERFLOW dan hook.

#### Langkah Kerja

1. Buka project `ESP32_03` atau `STM32_03`.
2. Task "risky" dibuat dengan stack kecil (512 bytes ESP32 / 96 words STM32).
3. Awalnya baik-baik saja — monitor task menampilkan stack HWM secara periodik.
4. Setelah 15 detik, "risky" task mulai menggunakan large local array → overflow!
5. `vApplicationStackOverflowHook()` terpanggil — cetak nama task yang overflow.
6. Amati `vTaskList()` menampilkan semua task dan stack-nya.

#### Tabel Pengamatan

| Task | Stack Size | HWM (normal) | HWM (sebelum crash) | Overflow? |
|------|-----------|-------------|---------------------|-----------|
| Risky | | | | |
| Monitor | | | | |
| Normal | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan method 1 dan method 2 pada `configCHECK_FOR_STACK_OVERFLOW`?
2. Mengapa stack overflow bisa merusak data task lain? (adjacent memory corruption)
3. Apa yang terjadi setelah overflow terdeteksi? Bisakah sistem recovery?
4. Bagaimana menentukan stack size yang tepat untuk sebuah task?

---

### Percobaan 04: Static Allocation

**Tujuan:** Membuat task dan queue tanpa heap — 100% static allocation dari array global.

#### Langkah Kerja

1. Buka project `ESP32_04` atau `STM32_04`.
2. Task dibuat dengan `xTaskCreateStatic()` — stack dari `StackType_t` array, TCB dari `StaticTask_t`.
3. Queue dibuat dengan `xQueueCreateStatic()` — buffer dari array global.
4. Amati bahwa **heap usage tidak berubah** setelah create.
5. Pada STM32, implementasikan `vApplicationGetIdleTaskMemory()` dan `vApplicationGetTimerTaskMemory()`.

#### Tabel Pengamatan

| Item | Dynamic (pvPortMalloc) | Static (array) | Heap Impact |
|------|----------------------|---------------|-------------|
| Task A | | xTaskCreateStatic | 0 bytes |
| Task B | | xTaskCreateStatic | 0 bytes |
| Queue (10 items) | | xQueueCreateStatic | 0 bytes |
| Total heap used | | | |

#### Pertanyaan Analisa

1. Apa keuntungan static allocation dibanding dynamic? Kapan sebaiknya digunakan?
2. Apakah static allocation menghilangkan risiko fragmentasi sepenuhnya?
3. Apa kelemahan static allocation? (flexibility, RAM usage jika task jarang aktif)
4. Mengapa perlu menyediakan callback memory untuk Idle dan Timer task?

---

### Percobaan 05: Memory Pool

**Tujuan:** Mengimplementasikan fixed-size memory pool menggunakan queue — O(1) alloc/free tanpa fragmentasi.

#### Langkah Kerja

1. Buka project `ESP32_05` atau `STM32_05`.
2. Pool berisi N blok berukuran tetap (mis 20×64B). Queue berisi pointer ke blok-blok free.
3. Alloc = `xQueueReceive()` (ambil pointer). Free = `xQueueSend()` (kembalikan pointer).
4. Producer/consumer task menggunakan pool untuk passing pesan.
5. Benchmark: pool alloc/free vs `pvPortMalloc()`/`vPortFree()` — mana lebih cepat?
6. Amati pool utilization dan fail count jika pool penuh.

#### Tabel Pengamatan

| Metric | Memory Pool | pvPortMalloc | Rasio |
|--------|-----------|-------------|-------|
| Alloc time (μs) | | | |
| Free time (μs) | | | |
| Fragmentation risk | 0% | Varies | |
| Max utilization | | N/A | |

#### Pertanyaan Analisa

1. Mengapa memory pool tidak menyebabkan fragmentasi?
2. Apa trade-off fixed-size block? (internal fragmentation jika data < block size)
3. Bagaimana menentukan block size dan pool count? Apa jika salah estimasi?
4. Bandingkan memory pool dengan static allocation — kapan gunakan yang mana?

---

### Percobaan 06: Stream Buffer

**Tujuan:** Transfer byte stream antar task menggunakan stream buffer — lebih ringan dari queue untuk data serial.

#### Langkah Kerja

1. Buka project `ESP32_06` atau `STM32_06`.
2. Stream buffer 256 bytes dengan trigger level 16 bytes.
3. Producer menulis 1-8 bytes per iterasi (variabel length).
4. Consumer di-trigger saat buffer mencapai trigger level — membaca semua bytes yang tersedia.
5. Amati buffer fill level, throughput, dan blocking behavior.
6. Pada STM32, amati burst sender mengirim 5 chunk secara rapid.

#### Tabel Pengamatan

| Metric | Value |
|--------|-------|
| Buffer size | 256 bytes |
| Trigger level | 16 bytes |
| Avg bytes per write | |
| Avg bytes per read | |
| Max fill level | |
| Total throughput | bytes/s |

#### Pertanyaan Analisa

1. Apa perbedaan stream buffer dan queue? Kapan gunakan yang mana?
2. Apa fungsi trigger level? Apa efek jika terlalu tinggi / rendah?
3. Apakah stream buffer bisa digunakan oleh multiple writer? Mengapa tidak?
4. Skenario apa yang cocok untuk stream buffer? (UART data, audio stream, dll)

---

### Percobaan 07: Message Buffer

**Tujuan:** Transfer pesan diskrit berbagai ukuran menggunakan message buffer — setiap pesan diterima utuh.

#### Langkah Kerja

1. Buka project `ESP32_07` atau `STM32_07`.
2. Message buffer 512 bytes. Tiga producer mengirim struct tipe berbeda (sensor, event, command).
3. Setiap pesan punya type header — consumer membaca dan mendecode berdasarkan tipe.
4. Pesan diterima **utuh** (atomis) — tidak bisa terpotong seperti stream buffer.
5. Amati per-type send/receive count dan space available.

#### Tabel Pengamatan

| Message Type | Size (bytes) | Sent | Received | Match? |
|-------------|-------------|------|----------|--------|
| Sensor | | | | |
| Event | | | | |
| Command | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan message buffer dan stream buffer?
2. Berapa overhead per pesan di message buffer? (4 bytes length header)
3. Apa yang terjadi jika buffer penuh saat sender ingin kirim pesan besar?
4. Kapan gunakan message buffer vs queue? Trade-off apa?

---

### Percobaan 08: Critical Section

**Tujuan:** Menggunakan critical section untuk proteksi ultra-cepat shared data — disable interrupt.

#### Langkah Kerja

1. Buka project `ESP32_08` atau `STM32_08`.
2. **Fase 1 (Tanpa proteksi):** Dua task update shared struct → data tearing/corruption.
3. **Fase 2 (Critical section):** `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` → zero corruption.
4. Benchmark: critical section vs mutex — mana lebih cepat?
5. Pada ESP32, perhatikan `portMUX_TYPE` spinlock untuk dual-core protection.

#### Tabel Pengamatan

| Method | Iterations | Corruption Count | Time (μs) |
|--------|----------|-----------------|-----------|
| No protection | 10000 | | |
| Critical section | 10000 | 0 | |
| Mutex | 10000 | 0 | |

#### Pertanyaan Analisa

1. Apa yang dilakukan `taskENTER_CRITICAL()` secara internal? (disable interrupt)
2. Mengapa critical section lebih cepat dari mutex? Apa trade-off-nya?
3. Mengapa critical section tidak boleh terlalu lama? Apa dampak ke real-time response?
4. Pada ESP32 dual-core, mengapa butuh spinlock selain disable interrupt?

---

### Percobaan 09: Heap Fragmentation Analysis

**Tujuan:** Menganalisis fragmentasi heap secara mendalam dan memahami efek coalescence.

#### Langkah Kerja

1. Buka project `ESP32_09` atau `STM32_09`.
2. Alokasi banyak blok kecil-menengah (10-50 blok).
3. Bebaskan blok genap — sisakan blok ganjil → banyak "hole".
4. Coba alokasi blok besar → GAGAL meskipun total free cukup.
5. Bebaskan semua blok → amati coalescence (merge adjacent free blocks).
6. Pada STM32, gunakan `vPortGetHeapStats()` untuk detail: jumlah free blocks, smallest block.

#### Tabel Pengamatan

| State | Free Heap | Largest Block | Free Blocks | Frag (%) |
|-------|----------|--------------|------------|---------|
| Initial | | | | |
| After alloc all | | | | |
| After free even | | | | |
| Large alloc attempt | FAIL | | | |
| After free all | | | | |

#### Pertanyaan Analisa

1. Formula fragmentasi: `1 - (largest_free / total_free)`. Jelaskan mengapa.
2. Apa itu coalescence? Bagaimana heap_4 menggabungkan free blocks berdekatan?
3. Strategi apa untuk mengurangi fragmentasi? (pool, static alloc, block sizing)
4. Berapa persentase fragmentasi yang masih acceptable untuk sistem embedded?

---

### Percobaan 10: PSRAM / External Memory (Platform-Specific)

**Tujuan ESP32:** Menggunakan PSRAM (external SPI RAM) untuk memperluas memori.
**Tujuan STM32:** Memahami memory map dan address regions (Flash, SRAM, stack, heap).

#### Langkah Kerja (ESP32)

1. Buka project `ESP32_10`.
2. Enumerate memory regions: DRAM (internal), SPIRAM (external), DMA-capable.
3. Alokasi buffer di SPIRAM: `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`.
4. Benchmark memset/read: DRAM vs SPIRAM at 1KB, 4KB, 16KB, 64KB.
5. Amati SPIRAM lebih lambat tapi jauh lebih besar.

#### Langkah Kerja (STM32)

1. Buka project `STM32_10`.
2. Print address analysis: lokasi .data, .bss, stack, heap, Flash.
3. Simulasikan external RAM buffer (2KB static array sebagai "emulasi").
4. Benchmark akses internal SRAM vs simulated external.
5. Pelajari konsep FSMC untuk STM32F4+ yang mendukung external RAM.

#### Tabel Pengamatan (ESP32)

| Region | Size | Alloc Speed | Read Speed | Write Speed |
|--------|------|------------|-----------|------------|
| DRAM | | | | |
| SPIRAM | | | | |
| Ratio | | | | |

#### Tabel Pengamatan (STM32)

| Region | Start Address | Size | Purpose |
|--------|-------------|------|---------|
| Flash | 0x0800_0000 | | Code |
| SRAM | 0x2000_0000 | | Data |
| Stack | | | |
| Heap | | | |

#### Pertanyaan Analisa

1. (ESP32) Berapa speed ratio DRAM vs SPIRAM? Mengapa SPIRAM lebih lambat?
2. (STM32) Bagaimana linker script menentukan letak stack dan heap?
3. Kapan sebaiknya data disimpan di external RAM vs internal RAM?
4. Apa risiko menyimpan real-time data di SPIRAM/external RAM?

---

### Percobaan 11: Memory Leak Detection

**Tujuan:** Mendeteksi memory leak secara otomatis — tracking alokasi dan membunyikan alarm.

#### Langkah Kerja

1. Buka project `ESP32_11` atau `STM32_11`.
2. "Good task" — alokasi dan free dengan benar (no leak).
3. "Leaky task" — alokasi periodik **tanpa free** (128 bytes/detik).
4. Monitor task mengambil snapshot heap setiap 2 detik.
5. Jika free heap turun 3+ kali berturut-turut → "LEAK DETECTED!".
6. Pada STM32, tracking table menyimpan setiap alokasi (address, size, task, tick).

#### Tabel Pengamatan

| Waktu | Free Heap | Δ Change | Consecutive Drops | Leak Alert? |
|-------|----------|---------|------------------|-------------|
| 0s | | | 0 | |
| 2s | | | | |
| 4s | | | | |
| 6s | | | | |
| 10s | | | ≥3 | DETECTED! |

#### Pertanyaan Analisa

1. Bagaimana mendeteksi leak hanya dari snapshot free heap? Apa kelemahannya?
2. Apa keuntungan tracking table (STM32 approach) vs snapshot (ESP32 approach)?
3. Bagaimana leak detection di development vs production? (overhead trade-off)
4. Strategi apa untuk mencegah leak? (RAII pattern, pool, static allocation)

---

### Percobaan 12: System Dashboard (Capstone)

**Tujuan:** Menggabungkan semua teknik monitoring ke dalam satu dashboard komprehensif.

#### Langkah Kerja

1. Buka project `ESP32_12` atau `STM32_12`.
2. Dashboard dicetak setiap 5 detik, berisi:
   - **Uptime** (jam:menit:detik)
   - **Heap stats:** Free, min-ever, largest block, fragmentation
   - **Task list:** Nama, state, prioritas, stack HWM, CPU%
   - **Queue/Semaphore status:** fill level, count
   - **Heartbeat:** via software timer
3. Background task menjalankan sensor producer-consumer dan compute task.
4. Amati dashboard berubah seiring waktu — CPU load, heap usage, queue fill.

#### Tabel Pengamatan

| Komponen Dashboard | Nilai Awal | Setelah 30s | Setelah 60s |
|-------------------|-----------|------------|------------|
| Free Heap | | | |
| Fragmentation % | | | |
| Task Count | | | |
| CPU Load (%) | | | |
| Queue Fill | | | |

#### Pertanyaan Analisa

1. Informasi apa yang paling penting untuk debugging sistem embedded production?
2. Berapa overhead dashboard task terhadap sistem? Apakah mempengaruhi real-time behavior?
3. Bagaimana mengoptimalkan dashboard agar overhead minimal?
4. Apa yang harus dilakukan jika dashboard menunjukkan heap terus turun?

---

## 5. Tabel Komparatif

| Aspek | STM32 (heap_4) | ESP32 (multi-region) |
|-------|----------------|---------------------|
| Heap Allocator | heap_4 (coalescence) | Multi-heap (DRAM+IRAM+SPIRAM) |
| Heap Stats API | `vPortGetHeapStats()` | `heap_caps_get_info()` |
| External RAM | Butuh FSMC (F4+) | PSRAM via SPI |
| Critical Section | `taskENTER_CRITICAL` | `portENTER_CRITICAL` + spinlock |
| Stack Overflow | Method 1 & 2 | ESP panic handler + method 2 |
| Memory Map | Single SRAM 20KB | DRAM 320KB + IRAM + PSRAM 4MB |

---

## 6. Referensi

1. FreeRTOS Memory Management — https://www.freertos.org/a00111.html
2. FreeRTOS Stream & Message Buffers — https://www.freertos.org/RTOS-stream-message-buffers.html
3. Mastering the FreeRTOS Real Time Kernel — Richard Barry (Ch. 2, 14)
4. ESP-IDF Heap Memory Allocation — Espressif Systems
5. AN4838 — Managing Memory Protection Unit in STM32, STMicroelectronics

---

*Jobsheet Modul 11 — FreeRTOS Memory & Advanced | Praktikum Sistem Embedded | 2025/2026*
