# Modul 11: FreeRTOS Memory Management & Advanced Features — Dokumen Komprehensif

Dokumen ini merupakan integrasi lengkap materi, jobsheet, project, dan tugas video untuk Modul 11: FreeRTOS Memory Management. Disusun khusus untuk dipahami oleh sistem seperti NotebookLM dengan bahasa Indonesia yang mudah dipahami dan banyak analogi praktis.

---

## BAGIAN I: TEORI, MATERI, PERINTAH & PRAKTIKUM (SLIDE 1–35)

---

### Slide 1: Pengenalan Modul 11 — Manajemen Memori Kritis

Manajemen memori adalah aspek paling kritis dalam sistem embedded yang reliable. Berbeda dengan komputer desktop yang memiliki RAM gigabytes, mikrokontroler hanya memiliki kilobytes. STM32F103 (Blue Pill) punya 20 KB SRAM, ESP32 punya 320 KB DRAM. Kesalahan manajemen menyebabkan crash fatal: heap exhaustion (sistem hang), stack overflow (merusak data task lain), memory leak (heap perlahan habis), fragmentasi (alokasi besar gagal meski total free cukup), dangling pointer (akses memori yang sudah dibebaskan). Modul ini mengajarkan teknik advanced FreeRTOS untuk membuat sistem embedded yang robust, predictable, dan bisa mendiagnosa dirinya sendiri melalui dashboard monitoring real-time.

---

### Slide 2: Mengapa Tidak Menggunakan malloc() Standar?

Fungsi malloc() dan free() dari C library standar dirancang untuk aplikasi desktop, bukan embedded RTOS. Masalahnya: (1) Tidak thread-safe—butuh mutex overhead tinggi jika digunakan di RTOS. (2) Waktu eksekusi tidak deterministic—bisa lama jika heap sangat fragmented. (3) Satu implementasi tetap—tidak bisa dioptimalkan untuk kebutuhan spesifik aplikasi. (4) Sulit di-debug—tidak ada hook function atau visibility ke heap. FreeRTOS menyediakan pvPortMalloc() dan vPortFree() yang thread-safe, bisa deterministic, configurable (5 skema heap berbeda), dan punya hook function untuk debugging. API FreeRTOS juga menyediakan xPortGetFreeHeapSize() dan xPortGetMinimumEverFreeHeapSize() untuk monitoring real-time heap health.

---

### Slide 3: Dynamic Memory Allocation — API Utama FreeRTOS

FreeRTOS menyediakan API abstraksi memori portabel: (1) pvPortMalloc(size_t xWantedSize)—alokasi memori dari heap FreeRTOS. Jika kehabisan, return NULL (harus dicek!). (2) vPortFree(void *pv)—bebaskan memori yang dialokasi. Setelah free, pointer harus dinull-kan untuk hindari dangling pointer. (3) xPortGetFreeHeapSize()—cek sisa heap tersedia saat ini dalam bytes. Gunakan ini sebelum alokasi besar. (4) xPortGetMinimumEverFreeHeapSize()—high water mark, sisa heap terendah yang pernah terjadi sejak startup. Berguna deteksi worst-case memory usage. (5) vApplicationMallocFailedHook()—callback jika pvPortMalloc() gagal. Implementasi hook untuk log error, reset, atau emergency procedure.

---

### Slide 4: Kaitan Memory dengan FreeRTOS Task & Queue

Setiap task membutuhkan memori: (1) Stack—untuk local variable, parameter, return address. (2) TCB (Task Control Block)—internal FreeRTOS, berisi state, priority, list pointers. Semua ini dialokasi dari heap saat xTaskCreate(). Queue juga butuh: (1) Queue structure—untuk pointer ke buffer dan status. (2) Queue buffer—tempat item disimpan. Jika heap habis, xTaskCreate() dan xQueueCreate() gagal return NULL. Default FreeRTOS menggunakan dynamic allocation, tapi bisa switch ke static allocation (xTaskCreateStatic(), xQueueCreateStatic()) agar tidak tergantung heap—penting untuk safety-critical system.

---

### Slide 5: Heap Statistics dan Monitoring Real-Time

Monitoring heap adalah langkah pertama deteksi problem: (1) Free Heap—sisa heap yang bisa dialokasi sekarang. Jika < 25% total, status CRITICAL. (2) Minimum Ever Free—worst-case terendah. Jika saat startup 81920 bytes tapi min-ever jadi 20KB, berarti sistem pernah stress. (3) Largest Free Block—ukuran blok terbesar yang bisa dialokasi sekarang. Jika ada 50KB free tapi largest block hanya 2KB, itu fragmentasi. (4) Fragmentation %—formula: (1 - largest_block/total_free) × 100%. Jika > 30%, warning mulai free memory. (5) Multi-region (ESP32)—monitor DRAM, IRAM, SPIRAM terpisah. STM32: monitor heap vs stack collision.

---

### Slide 6: Lima Skema Heap FreeRTOS — Heap_1

Heap_1 adalah allocator paling simpel: alokasi saja, tidak bisa free. Bayangkan menumpuk kotak—kamu bisa nambah kotak baru di atas, tapi tidak bisa ambil kotak dari tengah tumpukan. Keuntungan: paling aman (zero fragmentasi, predictable), paling cepat, ideal untuk safety-critical sistem (DO-178C, IEC 61508) di mana durability terjamin. Kelemahan: memori tidak bisa untuk-reuse—waste jika task/queue dihapus. Penggunaan: sistem embedded minimal dengan struktur task fixed (dibuat di startup, tidak pernah dihapus). Contoh: drone autopilot, sistem injeksi mobil, medical device. Konfigurasi: pilih heap_1 di MemMang folder, set configTOTAL_HEAP_SIZE di FreeRTOSConfig.h. Jangan gunakan vPortFree()—akan NOP (no operation).

---

### Slide 7: Lima Skema Heap FreeRTOS — Heap_2 & Heap_3

Heap_2 menggunakan best-fit algorithm: cari blok free terkecil yang cukup untuk menghemat memori. Namun tidak melakukan coalescing (penggabungan blok kosong bersebelahan). Contoh masalah: alokasi 5× blok 100B, free blok 2 dan 4 (alternating), sisa 3 lubang 100B. Total 300B free tapi tidak bisa alokasi 200B berturut. Heap_2 sudah deprecated oleh FreeRTOS modern. Heap_3 adalah wrapper standar malloc()/free() C library—thread-safe dengan suspend scheduler. Berguna jika kode legacy sudah pakai malloc(). Kelemahan: overhead scheduler suspend, fragmentasi dari libc malloc, xPortGetFreeHeapSize() tidak tersedia. Heap_3 jarang digunakan karena heap_4 jauh lebih baik.

---

### Slide 8: Lima Skema Heap FreeRTOS — Heap_4 (RECOMMENDED)

Heap_4 adalah rekomendasi FreeRTOS untuk kebanyakan aplikasi. Menggunakan first-fit algorithm + automatic coalescing (penggabungan blok kosong bersebelahan). Analogi: ruang kelas penuh meja student (alokasi), beberapa pindah keluar (free). Heap_4 otomatis merge meja kosong yang sebelahan menjadi satu blok besar untuk alokasi berikutnya. Keuntungan: fragmentasi minimal, deterministic behavior, mudah debug, default di STM32CubeF1. Konfigurasi: portable/MemMang/heap_4.c diselect di build. Set configTOTAL_HEAP_SIZE (mis 15 KB untuk STM32F103). Internal: FreeRTOS maintain linked list block—setiap block punya header (size, is_allocated flag) dan pointer next. Saat free, coalesce dengan neighbor. Memory waste: ~16 bytes header per blok (cukup minimal).

---

### Slide 9: Lima Skema Heap FreeRTOS — Heap_5 (Multi-Region)

Heap_5 adalah heap_4 dengan dukungan multiple memory region—berguna ketika RAM terpisah-pisah (internal SRAM + external SDRAM, atau internal DRAM + external SPIRAM). Contoh STM32F4: 192 KB internal SRAM + 8 MB external SDRAM via FSMC. Contoh ESP32: 320 KB internal DRAM + 4 MB PSRAM via SPI. Setup heap_5 memerlukan vPortDefineHeapRegions(HeapRegion_t xHeapRegions[]) di main sebelum create task. Array HeapRegion_t berisi address dan size setiap region, terminated dengan NULL entry. FreeRTOS otomatis manage semua region sebagai single heap virtual. Keuntungan: flexibility memory besar, optimal utilization. Kelemahan: sedikit lebih kompleks setup, harus careful dengan address correctness. Default STM32F103 tidak perlu heap_5 (single 20KB SRAM).

---

### Slide 10: Konfigurasi FreeRTOSConfig.h untuk Heap Management

Konfigurasi kritis di FreeRTOSConfig.h: (1) #define configTOTAL_HEAP_SIZE ((size_t)(15 * 1024))—total heap dalam bytes. STM32F103: reserve 15KB dari 20KB SRAM, sisain 5KB untuk stack global. ESP32: bisa sampai 160 KB. (2) #define configMAX_TASK_NAME_LEN 16—panjang nama task, useful untuk debug. (3) #define configUSE_MALLOC_FAILED_HOOK 1—enable vApplicationMallocFailedHook callback. (4) #define configCHECK_FOR_STACK_OVERFLOW 2—stack overflow detection method 2 (pattern-based, recommended). Setelah enable configSUPPORT_STATIC_ALLOCATION, WAJIB sediakan vApplicationGetIdleTaskMemory() dan vApplicationGetTimerTaskMemory() callback. FreeRTOS validation tools akan error compile jika missing. Default FreeRTOS uses dynamic allocation—set configSUPPORT_DYNAMIC_ALLOCATION 1 (default).

---

### Slide 11: Stack Management — Struktur Stack Task

Setiap task punya stack sendiri—memory untuk local variable, parameter, return address. Visualisasi stack (ARM Cortex-M): arah tumbuh dari high address ke low address. Top= high address (konteks saved saat context switch), pointer SP (stack pointer) tunjuk data terakhir pushed, bottom= low address (guard pattern jika overflow detection aktif). Satuan stack: STM32 pakai words (4 bytes), ESP32 pakai bytes. Jadi stack 256 words STM32 = 1024 bytes, stack 4096 bytes ESP32 = 4096 bytes. High Water Mark (HWM)—sisa stack terendah yang pernah dipakai. uxTaskGetStackHighWaterMark(xHandle) return HWM dalam words (STM32) atau bytes (ESP32). Rule thumb: minimal 50-100 words (STM32) atau 1024 bytes (ESP32) harus tersisa. Jika < 20 words: BERBAHAYA, perbesar stack!

---

### Slide 12: Stack Overflow Detection — Method 1 & Method 2

FreeRTOS menyediakan dua method deteksi stack overflow: Method 1: Cek apakah Stack Pointer melampaui batas saat context switch. Cepat tapi bisa miss overflow yang terjadi di antara context switch. Method 2 (RECOMMENDED): FreeRTOS mengisi 20 bytes terakhir stack dengan pattern 0xA5A5A5A5. Saat context switch, cek apakah pattern masih utuh—jika corrupt, overflow happened. Lebih akurat tapi sedikit lambat. Konfigurasi: set #define configCHECK_FOR_STACK_OVERFLOW 2 di FreeRTOSConfig.h. WAJIB implementasi hook: void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName). Hook ini dipanggil otomatis saat overflow terdeteksi—biasanya untuk log error, simpan ke flash, reset. Jangan coba recovery dari vApplicationStackOverflowHook—data sudah corrupt, sistem harus halt. Output hook: cetak nama task yang overflow untuk identify culprit.

---

### Slide 13: Static Allocation untuk Task Kritis

Static allocation menggunakan memory global (from .data/.bss section) bukan heap. Keuntungan: tidak bisa gagal (compile-time check), zero overhead pvPortMalloc, cocok untuk safety-critical sistem, mudah debug (variabel global named). Kelemahan: tidak flexible (size tetap), RAM usage tetap meski task jarang aktif, perlu tahu requirement saat compile. Contoh ESP32: 
```c
static StackType_t xTaskStack[4096];
static StaticTask_t xTaskTCB;
TaskHandle_t xH = xTaskCreateStatic(vTask, "name", 4096, NULL, 2, xTaskStack, &xTaskTCB);
```
xTaskCreateStatic return handle (tidak NULL seperti dynamic alloc yang bisa NULL). Jika configSUPPORT_STATIC_ALLOCATION=1, WAJIB implementasi vApplicationGetIdleTaskMemory() dan vApplicationGetTimerTaskMemory()—provide stack & TCB untuk Idle task & Timer task. Tanpa callback, compile error. Best practice: task kritis (safety monitor, emergency landing) gunakan static alloc.

---

### Slide 14: Static Allocation untuk Queue & Semaphore

Static queue: butuh queue structure buffer dan queue item buffer. Contoh 5-item queue untuk 4-byte integer:
```c
#define QUEUE_LEN 5
#define ITEM_SIZE sizeof(uint32_t)
static uint8_t ucQueueStorage[QUEUE_LEN * ITEM_SIZE];
static StaticQueue_t xQueueBuf;
QueueHandle_t xQueue = xQueueCreateStatic(QUEUE_LEN, ITEM_SIZE, ucQueueStorage, &xQueueBuf);
```
Keuntungan: queue tidak tergantung heap, guaranteed available. Static semaphore & mutex juga didukung xSemaphoreCreateStaticBinary(), xSemaphoreCreateStaticCounting(), xMutexCreateStatic(). Pattern: jika systemnya safety-critical atau resource sangat terbatas, gunakan static semua. Jika development jadi ribet karena perlu hitung size di compile-time—compromise: static untuk kritis (safety monitor, emergency), dynamic untuk non-kritis (display, logging). Modul 11 project (SkyWatch) require static allocation untuk task safety-critical.

---

### Slide 15: Memory Pool Pattern — Fixed-Size Block Allocation

Memory pool memecahkan fragmentasi dengan cara: alokasi blok-blok ukuran tetap di startup (static array), simpan pointer ke setiap blok dalam queue. Alloc=xQueueReceive() (ambil pointer free block), free=xQueueSend() (kembalikan pointer ke queue). Zero fragmentasi karena semua blok sama besar. Contoh 8 blok 64 bytes: total memory 512 bytes fixed, tidak pernah change. Implementasi simple:
```c
static uint8_t ucPool[8][64];
QueueHandle_t xFreeList = xQueueCreate(8, sizeof(void *));
// init: loop i, send &ucPool[i][0] to queue
void* pvPoolAlloc(TickType_t ticks) { void *p; xQueueReceive(xFreeList, &p, ticks); return p; }
void vPoolFree(void *p) { xQueueSend(xFreeList, &p, 0); }
```
Keuntungan: O(1) alloc/free deterministic, zero fragmentation, thread-safe, mudah monitor utilization. Kelemahan: internal fragmentation jika data < block size (waste space), perlu estimasi size. Penggunaan: sistem real-time demand low latency (audio processing, packet handling), atau sistem memory tight dengan fragmentasi concern.

---

### Slide 16: Stream Buffer — Byte Stream Transfer

Stream buffer transfer byte stream kontinu (seperti UART RX data). Sintaks:
```c
StreamBufferHandle_t xStream = xStreamBufferCreate(256, 16);  // 256B buffer, 16B trigger
xStreamBufferSend(xStream, data, len, msTimeout);
xStreamBufferReceive(xStream, buf, sizeof(buf), msTimeout);
```
Trigger level=16 berarti receiver unblock hanya jika ≥16 bytes tersedia—efisien, not byte-by-byte. Keuntungan atas queue: overhead minimal, pas untuk streaming data (sensor raw, log bytes). Kelemahan: single writer single reader (queue support multi reader/writer), data bisa terpotong (tidak atomis). Konfigurasi trigger level penting: terlalu tinggi=latency besar (batch processing). Terlalu rendah=overhead ISR frequent. Skenario bagus: UART RX pipe trigger saat 32 bytes tersedia, task proses batch. Skenario buruk: stress test alloc/free dengan tiny buffer—bukan gunanya, gunakan queue/pool untuk itu.

---

### Slide 17: Message Buffer — Discrete Message Transfer

Message buffer transfer pesan diskrit berbagai ukuran. Setiap pesan diterima utuh (atomis)—tidak terpotong seperti stream buffer. Internal: 4 bytes header (message length), payload. Sintaks:
```c
MessageBufferHandle_t xMsg = xMessageBufferCreate(256);
xMessageBufferSend(xMsg, &msg, sizeof(msg), msTimeout);
size_t len = xMessageBufferReceive(xMsg, &rxMsg, sizeof(rxMsg), msTimeout);
```
Skenario: 3 command message types (64B STATUS, 16B CONTROL, 128B CONFIG). Sender kirim arbitrary mix, receiver decode per-type. Message buffer handle atomicity—tidak bisa rusak di tengah receive. Keuntungan: variable-length data, atomic, ISR-safe. Kelemahan: single writer/reader, overhead header per pesan. Vs queue: message buffer ringan jika message bervariasi ukuran; queue lebih versatile multi-writer/reader. Vs stream buffer: message buffer atomis, stream buffer continuous. Best practice: log message → message buffer, sensor stream → stream buffer, structured data → queue.

---

### Slide 18: Critical Section — Proteksi Data Atomis

Critical section adalah region kode yang TIDAK BOLEH di-interrupt/preempt supaya operasi atomic. Contoh: update 32-bit counter shared oleh 2 task. Tanpa protection, context switch di tengah update corrupt data. FreeRTOS menyediakan 3 level: (1) Suspend Scheduler: taskENTER_CRITICAL() / taskEXIT_CRITICAL()—disable context switch (interrupt still active). Fastest. (2) Disable Interrupt: portDISABLE_INTERRUPTS()—disable semua interrupt. Very fast, very short duration sahaja. (3) Mutex: xSemaphoreTake/Give—slowest, wake-up overhead, tapi flexible (bisa recursive, priority inheritance). Rekomendasi: gunakan critical section untuk < 10 μs protection; pake mutex untuk >= 10 μs (give interrupt chance). ESP32 dual-core: critical section tanpa spinlock tidak cukup (core A protect core B gak keliatan), harus portENTER_CRITICAL_SAFE() atau portMUX_TYPE. STM32 single-core: critical section sufficient. Code:
```c
taskENTER_CRITICAL(); { shared_var++; } taskEXIT_CRITICAL();
```

---

### Slide 19: Percobaan STM32 — Heap Monitor (P01)

Praktikum STM32 Percobaan 01: Monitor heap real-time. Langkah: (1) Create project STM32_01 (HAL + FreeRTOS v10). (2) Task "Monitor" setiap 2 detik print heap stats: free, min-ever, largest block. (3) Task "Allocator" setiap 4 detik alokasi 1KB—observe free heap turun. (4) Task "Deallocator" setiap 8 detik free block—observe free heap naik. (5) Monitor coalescence: saat dealloc, largest block biasanya jump besar (coalescing). Output serial monitor 115200 baud:
```
[00:02s] Free: 12288 | MinEver: 10752 | Largest: 8192 | Active: 0
[00:04s] Free: 11264 | MinEver: 10752 | Largest: 8192 | Active: 1
[00:08s] Free: 12288 | MinEver: 10752 | Largest: 9216 | Active: 0
```
Note: STM32F103 hanya 20KB SRAM, minus stack/global, ~15KB heap. Jangan alokasi terlalu banyak. Analisa: min-ever yang rendah menunjukkan system pernah tight memory. Setup command: enable configUSE_MALLOC_FAILED_HOOK, implement vApplicationMallocFailedHook(), set configTOTAL_HEAP_SIZE 15*1024 di config.

---

### Slide 20: Percobaan STM32 — Allocation Pattern & Fragmentasi (P02)

Praktikum STM32 Percobaan 02: Demonstrate memory fragmentation problem & coalescence. Langkah: (1) Alokasikan 50 blok 256 bytes (sequential). (2) Pattern A: Free in-order (0,1,2,...)—observe coalesces to single large block. (3) Pattern B: Free alternating (0,2,4,6,...)—observe banyak small holes, largest block jauh lebih kecil dari total free. (4) Coba alloc 8KB—gagal di Pattern B padahal total free > 8KB! Demonstrasi fragmentasi. Monitoring pake vPortGetHeapStats() (STM32 HAL) atau xPortGetFreeHeapSize() API. Output:
```
Pattern A (in-order free):
 FreeHeap: 13000 | Largest: 12800  => Semua blok coalesc jadi 1 besar

Pattern B (alternating free):
 FreeHeap: 13000 | Largest: 2400  => Banyak hole kecil, alloc 8KB fail!
```
Formula fragmentasi: frag = (1 - largest/total) × 100%. Arti: semakin tinggi, semakin banyak "wasted" space. Threshold warning: > 30% fragmentation → caution, > 60% → critical. Lesson: dynamic alloc dengan size berbeda dangerous. Solution: use memory pool, static alloc, atau heap_4 coalescing (yang STM32 sudah pakai).

---

### Slide 21: Percobaan STM32 — Stack Overflow Detection (P03)

Praktikum STM32 Percobaan 03: Detect stack overflow menggunakan Method 2 pattern-based detection. Langkah: (1) Task "Risky" diciptakan dengan tiny stack (96 words = 384 bytes). (2) Task "Monitor" cetak HWM semua task setiap 2s. (3) Awal baik-baik. (4) Setelah 15s, Risky task create large local buffer (> stack)→ stack pointer cross guard pattern. (5) Next context switch detects corruption→ vApplicationStackOverflowHook(){print("OVERFLOW: Risky"); while(1);}. (6) System halts. Output:
```
[00:00] Risky HWM: 85/96  Monitor HWM: 60/256  OK
[00:02] Risky HWM: 80/96  Monitor HWM: 62/256  OK
...
[00:16] STACK OVERFLOW di task: Risky!  [HALT]
```
Konfigurasi: FreeRTOSConfig.h set configCHECK_FOR_STACK_OVERFLOW 2, implement vApplicationStackOverflowHook(). Test: create Risky task dengan stack kurang dari kebutuhan (hitung local var size + call overhead). Pelajaran: HWM monitoring adalah early warning sebelum crash.

---

### Slide 22: Percobaan STM32 — Static Allocation untuk Task Kritis (P04)

Praktikum STM32 Percobaan 04: Create task tanpa pvPortMalloc—100% static. Langkah: (1) Define xTaskStack[256] dan xTaskTCB global. (2) Call xTaskCreateStatic() with stack & TCB buffer. (3) Snapshot heap sebelum create—catat free heap. (4) Heap tidak berubah setelah create (proof: static allocation zero heap impact). (5) Create queue static juga: xQueueCreateStatic(). (6) Demonstrasi: dynamic create burn heap, static tidak. Output:
```
Before: Free = 12288 bytes
After dyn create task: Free = 11232  (used 1056) ← pvPortMalloc
After static create task: Free = 11232  (no change) ← static buffer
```
Code structure:
```c
static StackType_t xTaskStack[256];
static StaticTask_t xTaskTCB;
xTaskCreateStatic(vTask, "Static", 256, NULL, 2, xTaskStack, &xTaskTCB);
```
Benefit: safety-critical task dijamin jalan (not failed by malloc). Kebutuhan: implement vApplicationGetIdleTaskMemory() & vApplicationGetTimerTaskMemory() callbacks. Pelajaran: static allocation adalah investasi memory untuk reliability—trade-off space vs safety.

---

### Slide 23: Percobaan STM32 — Memory Pool Demonstration (P05)

Praktikum STM32 Percobaan 05: Implement fixed-size memory pool menggunakan queue. Langkah: (1) Create pool: 20 blok × 64 bytes static array. (2) Queue 20 item berisi pointer ke setiap blok (free list). (3) Task Producer: xQueueReceive() extract pointer from free list, isi with sensor data, send ke processing queue. (4) Task Consumer: xQueueReceive() dari processing queue, proses, xQueueSend() kembalikan pointer ke free list. (5) Benchmark: pool alloc/free time vs pvPortMalloc/vPortFree—pool harus faster & deterministic. Output
```
Algorithm      Alloc(μs)  Free(μs)
Pool (queue)      2.1       2.0  ← deterministic O(1)
pvPortMalloc()     8.5      15.2  ← variable, coalescing overhead
```
Demonstrasi: alloc/free loop 1000x, measure time. Pool margin: zero overhead, consistent timing. Pelajaran: pool pattern cocok untuk real-time demand predictable latency (audio processing, high-speed sensor). Drawback: internal fragmentation jika block size > typical data size.

---

### Slide 24: Percobaan STM32 — Leak Detection (P11)

Praktikum STM32 Percobaan 11: Deteksi memory leak otomatis. Langkah: (1) Task Good: alloc & free balanced—no leak. (2) Task Leaky: alokasi 128 bytes per detik tanpa free (intentional leak untuk demo). (3) Task Monitor: snapshot heap setiap 2s. (4) Jika free heap turun ≥3 kali berturut—"LEAK DETECTED!". (5) Program print leak rate (bytes/min) & TTL (time-to-exhaustion). Output:
```
[00:00] Free: 12288
[00:02] Free: 12160  ▼
[00:04] Free: 12032  ▼
[00:06] Free: 11904  ▼ LEAK DETECTED!
Leak rate: 128 bytes/2s = ~3.8 KB/min
TTL: 12032 / 3.8 = ~51 minutes to exhaustion
```
Implementasi:
```c
static size_t prev_free = 0, drop_count = 0;
size_t curr_free = xPortGetFreeHeapSize();
if (curr_free < prev_free) drop_count++; else drop_count = 0;
if (drop_count >= 3) printf("LEAK!\n");
prev_free = curr_free;
```
Pelajaran: leak tidak membuat crash immediate—tapi slowly kills system over hours/days. Early detection gives time for safe landing.

---

### Slide 25: Percobaan STM32 — System Dashboard (P12)

Praktikum STM32 Percobaan 12: Comprehensive health dashboard setiap 5s. Menggabung semua monitoring: heap, task, queue, timer. Output sample:
```
╔════ SYSTEM DASHBOARD (uptime 1:45)═══╗
║ HEAP: 9120/15360 (59%) [GREEN]       ║
║ MinEver: 8320 | Largest: 7680 (50%) ║
║ Fragmentation: 12% | Status: NORMAL  ║
╠══════════════════════════════════════╣
║ Task       Pri  Stack_HWM  State     ║
║ Monitor     2   160/256    Ready     ║
║ Producer    3   140/512    Blocked   ║
║ Consumer    3   180/512    Running   ║
║ Display     1   85/256     Ready     ║
║ IDLE        0   45/256     Ready     ║
╠══════════════════════════════════════╣
║ Queue: 5/10 full | Pool: 16/20 free  ║
╚════════════════════════════════════════╝
```
Implementation: create task "Dashboard" priority 1 (low), every 5s loop: print heap, task list via vTaskList(), queue status via uxQueueMessagesWaiting(). Library support: vTaskGetInfo() for task detail. Pelajaran: dashboard adalah window system health—gunakan untuk diagnosa problem, identify bottleneck, capacity planning. Real-world: plane cockpit dashboard—pilot lihat semua sistem kondisi instantly.

---

### Slide 26: Percobaan ESP32 — Memory Architecture (Multi-Region)

ESP32 memory architecture jauh lebih kompleks dari STM32: (1) Internal DRAM ~320KB (not all usable due to code/heap/stack). (2) Internal IRAM ~160KB (fast, for critical code). (3) PSRAM ~4MB (optional, external SPI, slower). (4) Flash ~4MB (code storage). Default FreeRTOS use DRAM. Advanced: alokasi di IRAM untuk low-latency (ISR handling), PSRAM untuk large buffers (audio, image). API: esp_heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_IRAM) untuk specific region. Monitoring: heap_caps_get_info() return free/total per capability type. Penggunaan praktik: static sensor data buffer→static alloc (zero heap), frequently-accessed→DRAM (fast), large cache→PSRAM. Pelajaran: multi-region flexibility requires plan memory layout carefully—salah tempat allocation bisa bikin slow application.

---

### Slide 27: Percobaan ESP32 — Heap Monitor Multi-Region (P01)

Praktikum ESP32 Percobaan 01: Monitor heap multi-region. Langkah: (1) Query 3 region: DRAM free/used, IRAM free/used, SPIRAM free/used (jika ada). (2) API: heap_caps_get_free_size(MALLOC_CAP_DRAM), dll. (3) Alloc + free test di each region—observe behavior. Output:
```
[DRAM]   Free: 180KB / 240KB (75%) | Frag: 8%   [GREEN]
[IRAM]   Free: 120KB / 160KB (75%) | Frag: 5%   [GREEN]
[SPIRAM] Free: 3.8MB / 4.0MB (95%) | Frag: 2%   [GREEN]
Heap Stats: Good allocation pattern, no leak detected.
```
Advanced: benchmark alloc speed each region—DRAM fastest (internal), SPIRAM slowest (SPI). Konfigurasi: ESP32 DevKit usually punya PSRAM chip onboard (flip config enable). FreeRTOSConfig.h: configUSE_MALLOC_FAILED_HOOK 1. Pelajaran: ESP32 flexibility memory region—use wisely untuk optimize speed vs capacity trade-off.

---

### Slide 28: Percobaan ESP32 — Stream Buffer over UART RX (P06)

Praktikum ESP32 Percobaan 06: Receive UART data into stream buffer. Langkah: (1) UART ISR setup vUARTISR_Handler() callback. (2) create stream buffer 256B trigger 32B. (3) UART ISR xStreamBufferSendFromISR() push bytes. (4) Task Receiver xStreamBufferReceive() block until 32B. (5) Demo: send command dari serial terminal, observe buffer behavior. Output:
```
[ISR] RX byte 'A'  → stream fill: 1/256
[ISR] RX byte 'T'  → stream fill: 2/256
...32 bytes cumulative...
[Task] Triggered! Received 32 bytes: "ATCOMMAND..."
```
Trigger level choice: balance latency vs ISR frequency. Low trigger (1B)→latency 1-2 ISR cycle (good), High trigger (64B)→latency higher (bad jika need quick response). ISR usage: xStreamBufferSendFromISR() return > 0 bytes written atau 0 if buffer full. Pelajaran: stream buffer ideal untuk UART/SPI data, reduce queue overhead.

---

### Slide 29: Percobaan ESP32 — Message Buffer untuk Command (P07)

Praktikum ESP32 Percobaan 07: Command message system. Message type: STATUS (type=1, 64B), CONTROL (type=2, 16B), CONFIG (type=3, 128B). Langkah: (1) Create message buffer 512B. (2) Sender task post different message types. (3) Receiver xMessageBufferReceive() get full message—decode per type. (4) Demo multiple msg flow: CONTROL then STATUS then CONFIG—all received intact. Output:
```
Sent: STATUS message (64B)
Sent: CONTROL message (16B)
Received: STATUS OK (64B parsed)
Received: CONTROL OK (16B parsed)
```
Key: message buffer atomic—no corruption partial data. Drawback: single writer/reader (otherwise use queue). Konfigurasi: configMESSAGE_BUFFER_LENGTH_TYPE uint32_t (allow up to 4GB message teoretis). Pelajaran: message buffer for discrete command/log; stream buffer for continuous data.

---

### Slide 30: Percobaan ESP32 — Critical Section Dual-Core (P08)

Praktikum ESP32 Percobaan 08: Critical section across dual-core. Langkah: (1) Shared variable: counter 32-bit. (2) Phase 1 (No protection): Core 0 & Core 1 both increment 1000x—data tearing → final counter ≠ 2000. (3) Phase 2 (Critical section): wrap increment dengan taskENTER_CRITICAL() / taskEXIT_CRITICAL()—zero corruption. (4) Benchmark: critical section vs mutex. Output:
```
Phase 1 (No protection):
  Final counter: 1847 (expected 2000!)  ✗ Tearing detected!

Phase 2 (Critical section):
  Final counter: 2000 (correct!)  ✓ Zero corruption
  
Benchmark:
  Critical section: 0.8 μs overhead
  Mutex: 8.5 μs overhead  ← 10x slower due to wake-up cost
```
Code structure:
```c
portMUX_TYPE xMutex = portMUX_INITIALIZER_UNLOCKED;
taskENTER_CRITICAL(&xMutex); { counter++; } taskEXIT_CRITICAL(&xMutex);
```
ESP32 dual-core: portENTER_CRITICAL() manage spinlock—invisible to programmer. STM32 single-core: just disable interrupt. Pelajaran: critical section appropriate untuk < 10μs protection; mutex overhead worth untuk longer critical section.

---

### Slide 31: Percobaan ESP32 — Fragmentation & Coalescence Demo (P09)

Praktikum ESP32 Percobaan 09: Demonstrate fragmentation problem & coalescence fix. Langkah: (1) Create esp32_09 project. (2) Allocate 50× blocks (random size 256-512B). (3) Free alternating blok (every 2nd)→fragmentation spike. (4) Heap stats show: total_free big, largest_block kecil. (5) Alloc 4KB→FAIL! (6) Free remaining blocks→coalescence happen, largest_block jump. Output:
```
After alloc 50 blocks: Free: 150KB | Largest: 100KB   OK
After free alternating: Free: 150KB | Largest: 8KB    ✗ Fragment
Alloc 4KB attempt:  FAIL (largest block only 8KB, waste)
After free remaining: Free: 150KB | Largest: 145KB    ✓ Coalesce!
```
Formula fragmentation percentage: (1 - largest / total_free) × 100%. Threshold warning: > 30%→caution, > 60%→critical. API: esp_heap_caps_get_info() untuk detailed stats. Pelajaran: modern heap allocator (heap_4) mitigate fragmentasi dengan coalescing—but prevention via pool/static still better.

---

### Slide 32: Percobaan ESP32 — Benchmark DRAM vs SPIRAM Speed (P10)

Praktikum ESP32 Percobaan 10: Compare speed DRAM vs SPIRAM. Langkah: (1) Allocate 64KB buffer in DRAM (internal). (2) Allocate 64KB buffer in SPIRAM (external). (3) Microbenchmark: memset(), memcpy(), read loop 1000x each. Measure time. Output:
```
1KB memset benchmark:
  DRAM:   15 μs
  SPIRAM: 85 μs
  Ratio: SPIRAM ~5.7x slower

16KB memcpy benchmark:
  DRAM:   200 μs
  SPIRAM: 980 μs
  Ratio: SPIRAM ~4.9x slower
```
Explanation: SPIRAM connected via SPI bus—lower bandwidth, higher latency than internal DRAM. SPIRAM besar (4MB) tapi slow—suitable untuk non-time-critical data (buffering, logging, large cache). DRAM kecil tapi fast—prioritize real-time critical data (sensor state, PDI). Strategy: stateful sensor data→DRAM, historical log→SPIRAM. Konfigurasi: menuconfig enable SPIRAM, heap_4.c support multi-region, heap_caps_malloc fine-grained allocation. Pelajaran: memory speed matter—optimize layout untuk maximum performance.

---

### Slide 33: Critical Section vs Mutex Trade-off

Ringkasan critical section vs mutex protection (ringkasan dari P08 insight): Critical Section: disable interrupt/scheduler untuk < 10 μs, zero overhead scheduling, fast. Mutex: lock-based, thread-safe, overhead scheduler wake-up (~8 μs per task), priority inheritance option. Guideline: update single 32-bit variable→critical. Update large struct/buffer→critical. ISR+task share single variable→critical. Multiple task long-hold resource→mutex. Single task queue multiple consumer →queue. Praktek buruk: long critical section (> 100 μs)→bad real-time, starve task. Praktek baik: minimal critical section, wake other task outside critical. Dokumentasi: setiap shared variable WAJIB document protection method—jangan guess. Analisa tool: thread analyzer (if available) detects unprotected access.

---

### Slide 34: Integrasi Praktikum STM32 & ESP32 — Common Learning

Kesamaan STM32 & ESP32 memory management: (1) FreeRTOS heap API sama (pvPortMalloc, vPortFree, xPortGetFreeHeapSize). (2) Stack management sama (context saved, HWM monitoring, overflow detection). (3) Static allocation API sama (xTaskCreateStatic, xQueueCreateStatic). (4) Memory pool pattern sama (queue-based free list). (5) Critical section concept sama (disable preemption/interrupt untuk atomic update). Perbedaan: STM32 heap_4 single region 20KB, ESP32 multi-region (DRAM+IRAM+SPIRAM). STM32 stack size unit words, ESP32 bytes (conversion needed). STM32 critical section via taskENTER_CRITICAL(), ESP32 also support portMUX for dual-core spinlock. STM32 UART buffer manual manage, ESP32 hardware support DMA ring buffer. Learning: portable FreeRTOS API—switch platform seamless jika disciplined. Non-portable: region-specific alloc (heap_caps_malloc ESP32 only), hardware capability (PSRAM, FSMC).

---

### Slide 35: Modul 11 Assessment Criteria & Completion Grade

Modul 11 evaluation: 3 komponen = praktikum (10%), project (25%), video (15%), total 50% dari course. Praktikum: 12 percobaan wajib lakukan STM32 ATAU ESP32 platform. Grading: setiap percobaan punya check-list (kode compile, output correct, analisis written). Minimal 10/12 passed=A, 8/12=B, 6/12=C, <6=D. Project: SkyWatch-2 system terpadu 12 fitur memory management. Rubrik: memory management (25%), buffer communication (20%), safety detection (25%), dashboard (20%), code quality (10%). Target A: semua 12 fitur, dashboard lengkap, analisa mendalam. Video: 20-35 min, webcam + narasi + 12 percobaan demo + project showcase. Penalty: webcam missing (-10%), no narasi (-15%), late submit (-10%/day). Completion: setelah submit Project + Video + Praktikum report, baru modul dinilai. Rekomendasi: start project minggu 1, video minggu 2, agar tidak stress akhir-akhir.

---

## BAGIAN II: PROJECT (SLIDE 36–40)

---

### Slide 36: Project SkyWatch-2 — Konteks & Spesifikasi Umum

Cerita konteks: PT AeroNusantara drone inspeksi jatuh bulan lalu—causal analysis menemukan memory leak, stack overflow, race condition pada shared telemetry. Kerugian Rp 120 juta. Team engineer ditugaskan bangun SkyWatch-2 dengan komprehensif self-diagnostic. Requirement SkyWatch-2: durasi terbang 30 menit, memori sangat limited (ESP32 320KB atau STM32 20KB SRAM), zero crash tolerance (harus self-recovery atau safe landing), 6 sensor simulat (IMU, barometer, GPS, baterai, ultrasonic, temperature). Sistem harus robust detection: memory leak, stack overflow, heap fragmentation, race condition. Output: flight telemetry log, health dashboard real-time. Pilihan platform: SATU dari ESP32 atau STM32 (pilih yg familiar). Durasi develop: 2 minggu sejak modul diberikan. Deliverable: (1) Source code modular C, (2) Compiled binary, (3) Documentation (architecture, test report), (4) Demo video 20-35 min.

---

### Slide 37: Project SkyWatch-2 — 12 Fitur Wajib Implementasi (1/2)

Fitur 1-6 dari modul praktik: (1) Heap Dashboard: monitor free/used/min-ever/fragmentation every 2s, classify GREEN/YELLOW/RED per threshold. (2) Allocation Pattern: detect buruk alloc pattern, log warning jika fragmentasi > 30%. (3) Stack Guardian: monitor HWM setiap task, warning > 85%, hook untuk overflow. (4) Static Critical: task safety-critical (SafetyMonitor, EmergencyLanding) pakai static allocation—heap unchanged proof. (5) Telemetry Pool: 20×64B block pool queue, sensor alloc from pool—benchmark vs malloc. (6) Sensor Stream Buffer: IMU data stream buffer 256B trigger 32B, demonstrate variable-length packet handling. Fitur 7-12 (slide 38). Implementasi requirement: semua 12 fitur HARUS in single codebase, compile clean no warning, run stable 30 min sim, dashboard output every 5s. Best practice: modular code—separate mem.c, pool.c, dashboard.c, sensor.c untuk easy maintain & test.

---

### Slide 38: Project SkyWatch-2 — 12 Fitur Wajib Implementasi (2/2)

Fitur 7-12: (7) Command Message Buffer: ground station send command (START, STOP, CALIBRATE, STATUS, LAND)—different message type size, receiver parse atomically. (8) Telemetry Protection: shared telemetry struct protected critical section, demonstrate data tearing phase 1 → fixed phase 2. (9) Fragmentation Monitor: formula frag = 1 - (largest/total_free), real-time threshold warning, coalescence after cleanup. (10) Multi-Region Info (ESP32) or Memory Map (STM32): enumerate region free/total, benchmark region speed, decide allocation strategy. (11) Leak Detector: snapshot heap every 2s, consecutive drop ≥3→alert, calculate leak rate, TTL to exhaustion. Trigger emergency landing jika TTL < 2 menit. (12) Flight Dashboard: comprehensive output uptime, heap, task list (priority, stack HWM, state, CPU%), pool/queue/buffer status. Requirement: ≥8/12 fitur berjalan untuk lulus (C grade). ≥10/12 untuk B. Semua 12 + deep analysis untuk A.

---

### Slide 39: Project SkyWatch-2 — Arsitektur & Task Structure

Rekomendasikan minimal 5 task: (1) SafetyMonitor (priority 5, static, 256B stack): loop 100ms, monitor all health metric, trigger emergency jika condition critical. (2) SensorIMU (priority 3): read simulat IMU, alloc from pool, post to processing queue. (3) TelemetryProcessor (priority 3): receive IMU from queue, compute average/filter, store to telemetry buffer protected critical section. (4) Logger (priority 2): periodic log heap/task state to flash buffer. (5) Display/Dashboard (priority 1, low): every 5s print comprehensive dashboard. Queue: sensor data queue (20 item, IMU struct), command queue (telemetry RX dari UART). Memory pool: telemetry 20×64B. Semaphore binary: LED indicator. Timer: periodic 100ms health check. Code structure: main.c (init), task.c (task function), memory.c (pool, heap monitor), dashboard.c (print dashboard), sensor.c (simulate IMU). Compile PlatformIO, debug serial monitor 115200. Simulat sensor via rand() + sin wave pattern (repeatable, testable).

---

### Slide 40: Project SkyWatch-2 — Testing & Validation Plan

Test scenario: (1) Normal operation 5 min—heap stable, no leak, dashboard printed every 5s without stall. (2) Stress alloc/free 1000 cycle—fragmentation spike but coalescence fix within 10s. (3) Leaky task introduce 128B/2s leak—leak detector alert within 6s (3 consecutive drops). (4) Overflow task cause stack overflow—hook catch overflow, system halt. (5) Race condition test—2 task write shared telemetry without critical section → check tearing (intentional phase 1), then enable critical section → verify zero tearing (phase 2). Validation: (a) Memory test=no hardware needed, all heap/pool logic simulat in code. (b) Baseline: startup free heap, min-ever heap, fragment %. (c) Duration: run simulation 30 min (or 3 min compressed speed), capture data log. (d) Analysis: plot heap over time (should stay > 25%), fragmentation (should stay < 60%), leak rate (should be 0 bytes/min). Report: document test summary, failure count, mitigation taken. Success criteria: ≤1 crash in 30 min test, heap never go < 20%, no undetected leak.

---

## BAGIAN III: TUGAS VIDEO (SLIDE 41–45)

---

### Slide 41: Tugas Video — Struktur & Ketentuan Umum

Video requirement: format MP4 720p, durasi 20-35 menit, narasi voice-over menjelaskan setiap step (bukan hanya visual kode), webcam terlihat minimal 15% layar (picture-in-picture), tunjukkan hardware jika ada LED/buzzer demo. Tangga struktur video: (1) Pembukaan 1-2 min (perkenalan, platform, overview modul). (2) Teori 3-5 min (heap allocator jenis, fragmentation, pool, stream buffer, critical section—simplified explanation dengan analogi). (3) Praktik 10-18 min (12 percobaan dishow singkat: kode→compile→run→output→analyze). (4) Project 4-6 min (arsitektur diagram, feature demo, dashboard output, leak detection alarm). (5) Penutup 1-2 min (lesson learned, when use which technique, reflection). Total: 20-35 menit. Quality: audio clear, no background noise, brightness/contrast normal. Penalty: webcam hilang −10%, no narasi −15%, percobaan tidak run (hanya kode) −5% per percobaan, durasi < 15 min −10%. Submit: upload YouTube Unlisted atau Google Drive, submit link, deadline 1 minggu setelah praktikum. Check plagiarism: original script + demo, tidak copy paste video orang lain.

---

### Slide 42: Tugas Video — Tema Teori (3-5 menit bagian)

Script teori bagian (simplified untuk general audience, bukan just engineer): "Bayangkan gudang warehouse (heap)—barang inventory berbagai ukuran disimpan. Allocator adalah manager: (1) Heap_1 simple—stack barang, hanya push, tidak pernah pop. Safe untuk fixed inventory. (2) Heap_4 smart—saat barang diambil, spot kosong merge dengan adjacent spot—prevent fragment. (3) Memory pool—bukannya simpan barang mana saja, pake standar box fix 64B (analogi LEGO block). Alloc=ambil box kosong, free=kembalikan box. Zero overhead, zero fragment. (4) Queue/Semaphore—untuk coordinate multi-warehouse worker (task) supaya tidak sialan ambil barang sama. Critical section—beri satu worker kunci utama, worker lain tunggu. Lepas kunci=worker berikutnya jalan. (5) Stack overflow—worker naik tumpukan barang, jatuh—danger! Monitor height (high water mark)—warn jika mendekati ceiling." Gajah: pakai visual aid (diagram atau slide deck) sambil narasi. Hindari pure kode dump atau datasheet quotation—explain why & when, not what (yang nama API).

---

### Slide 43: Tugas Video — Demo 12 Percobaan (Strategi Singkat)

12 percobaan dishow dalam ~1.5 menit each (total 18 menit per 12 percobaan). Format setiap percobaan: (1) Code snippet—highlight relevant part (malloc, free, loop, alloc size). (2) Compile—show PlatformIO build success. (3) Serial output—capture 10-20 line data (tidak perlu scrolling panjang—cut ke relevant part). (4) Analysis—1-2 sentence insight ("notice largest block turun dari 8KB to 2KB—lihat fragmentasi setelah free ganjil"). Best practice: pre-record semua 12, edit untuk concise. Jangan show full compile (20 detik build=sebaliknya, clip 2-3 detik). Query untuk setiap percobaan: (P01 heap monitor) serial output snapshot. (P02 fragmentation) alloc→free alternating→largest block change graph. (P03 stack overflow) HWM values decreasing→overflow hook message. (P04 static alloc) heap unchanged before/after proof. (P05 pool) benchmark alloc time comparison. (P06 stream buffer) fill level trending. (P07 message buffer) multiple message type received. (P08 critical) data corruption count before/after. (P09 coalescence) heap stats progression. (P10 region benchmark) DRAM vs SPIRAM speed table. (P11 leak) free heap snapshot dropping → leak alert. (P12 dashboard) full dashboard output scrolled slowly.

---

### Slide 44: Tugas Video — Project SkyWatch-2 Demo (4-6 menit bagian)

Demo project SkyWatch mencakup: (1) Architecture diagram—5 task, queue, pool, critical section protection visualisasi (draw or PowerPoint slide). Narasi penjelasan tiap task role. (2) Code walkthrough—highlight 3 key code snippet: (a) static alloc SafetyMonitor task, (b) memory pool telemetry alloc/free, (c) critical section shared telemetry update. (3) Compile+run—show build success, initiate 30 min (atau 3 min compressed) simulation. (4) Dashboard output—capture real dashboard output 2-3 cycle (5 second interval), point out each field: heap color status, task list, fragmentation. (5) Leak demo—stream leaky task alloc 128B/cycle, observer heap slope downward, point out TTL calculation "18 minutes to exhaustion", emergency landing trigger. (6) Feature check—walk through 8+ dari 12 fitur implemented (show evidence: code line number or output matching feature requirement). Avoid: full code listing (boring), compilation screen lingering (waste time), silent mode (impossible understand). Do: talk slow, pause untuk audience absorb, use pointer/highlight.

---

### Slide 45: Tugas Video — Checklist & Tips Submit Final

Pre-submit checklist: [✓] intro 1-2 min (nama, NIM, platform). [✓] Teori 3-5 min. [✓] Praktik 10-18 min (12 percobaan). [✓] Project 4-6 min (≥8 fitur shown). [✓] Penutup 1-2 min (less learned). [✓] Total durasi 20-35 menit. [✓] Webcam terlihat throughout. [✓] Narasi suara clear (no background music). [✓] Resolusi 720p+ (tidak blur). [✓] MP4 format. [✓] Link accessible (YouTube unlisted atau GDrive share). Editing tips: gunakan OBS Studio (free), record screen + microphone, edit with CapCut (free) atau Kdenlive (Linux). Subtitle optional tapi recommended—increase clarity. Color grading: normal exposure (jangan dark/bright ekstrem). Audio level: normalize −3dB peak (tidak clipping), background noise min. Practice: dry run sebelum record final—memorize script, test flow. Upload: YouTube beri video schedule unlisted (tidak lost in feed), set custom thumbnail, copy link. Submit: form link + screenshot first 5 second (buktikan own work). Deadline: 1 minggu after praktikum. Late penalty: −10%/day (max 3 hari, after that 0 point). Final advice: jangan perfectionist—functional & explained > polished & silent.

---

*Modul 11 — FreeRTOS Memory Management & Advanced Features | Dokumen Komprehensif NotebookLM | Praktikum Sistem Embedded 2025/2026*

**Total konten: 45 slide, teori-materi-perintah-praktik (1-35), project (36-40), tugas video (41-45). Setiap slide 250-280 karakter, mudah dipahami dengan analogi, dan banyak detail instruksi bahasa Indonesia.**
