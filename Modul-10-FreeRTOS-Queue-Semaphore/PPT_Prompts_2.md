# PPT Prompts Modul 10 - FreeRTOS Queue dan Semaphore (Bagian 2)

## Slide 16: Apa itu Mutex?
```
Judul: "Mutex: Mutual Exclusion"
Definisi box:
"Mutex adalah mekanisme locking untuk memastikan 
hanya SATU task yang mengakses resource pada satu waktu"

Perbedaan dengan Binary Semaphore:
┌──────────────────┬──────────────────┐
│ Binary Semaphore │      Mutex       │
├──────────────────┼──────────────────┤
│ Signaling        │ Resource lock    │
│ No ownership     │ Has ownership    │
│ ISR compatible   │ Task only        │
│ No inheritance   │ Priority inherit │
└──────────────────┴──────────────────┘

Visual: Diagram task dengan kunci (lock)
Aturan: "Yang Take harus yang Give (ownership)"
```

---

## Slide 17: Priority Inheritance
```
Judul: "Priority Inheritance - Menghindari Priority Inversion"
Skenario TANPA mutex:
[High Priority] → Blocked waiting for resource
[Low Priority]  → Holds resource (can't run)
[Med Priority]  → RUNS! (preempts Low)
→ High blocked oleh Medium! = PRIORITY INVERSION

Skenario DENGAN mutex:
[High Priority] → Blocked waiting for mutex
[Low Priority]  → Holds mutex, priority BOOSTED to High
→ Low runs at High priority, releases quickly
→ High gets mutex!

Diagram timeline menunjukkan perbedaan
Conclusion: "Mutex = Built-in priority inheritance!"
```

---

## Slide 18: Membuat dan Menggunakan Mutex
```
Judul: "xSemaphoreCreateMutex()"
Create:
SemaphoreHandle_t xMutex;
xMutex = xSemaphoreCreateMutex();

Usage pattern:
if(xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // === CRITICAL SECTION ===
    // Access shared resource safely
    access_shared_resource();
    // ========================
    
    xSemaphoreGive(xMutex);  // MUST release!
} else {
    // Timeout - couldn't get mutex
    handle_error();
}

Visual: Lock→Critical Section→Unlock
Warning box: "Jangan lupa release! Bisa deadlock!"
```

---

## Slide 19: Recursive Mutex
```
Judul: "Recursive Mutex - Self-Take Multiple Times"
Skenario:
"Fungsi yang membutuhkan mutex memanggil fungsi lain
yang juga membutuhkan mutex yang sama"

Standard mutex = DEADLOCK!
function_a() {
    take(mutex);
    function_b();  // ← Tries to take same mutex = STUCK!
    give(mutex);
}

Recursive mutex = OK!
function_a() {
    take(recursive_mutex);  // Count = 1
    function_b();           // Count = 2
    give(recursive_mutex);  // Count = 1
}

Create:
xMutex = xSemaphoreCreateRecursiveMutex();

Usage: xSemaphoreTakeRecursive() / GiveRecursive()
```

---

## Slide 20: Deadlock - Bahaya!
```
Judul: "Deadlock: Kondisi Berbahaya"
Definisi:
"Deadlock terjadi saat dua atau lebih task saling menunggu
resource yang dikunci oleh task lain"

Diagram classic deadlock:
Task A: Take(MutexX) → Try Take(MutexY) → BLOCKED
         ↑                    ↓
Task B: Take(MutexY) → Try Take(MutexX) → BLOCKED

Kondisi deadlock (4 sekaligus):
1. Mutual Exclusion - Resource eksklusif
2. Hold and Wait - Tahan sambil menunggu
3. No Preemption - Tidak bisa direbut
4. Circular Wait - Menunggu melingkar

Visual: Dua orang memegang satu pedang masing-masing,
        saling menunjuk untuk menukar
```

---

## Slide 21: Mencegah Deadlock
```
Judul: "Strategi Menghindari Deadlock"
5 strategi:

1. Lock Ordering (Recommended!)
   Selalu take mutex dalam urutan yang sama
   ✅ A→B→C (konsisten)
   ❌ A→B dan B→A (berbahaya!)

2. Timeout
   Gunakan timeout, bukan portMAX_DELAY
   if(xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
       // Timeout! Release semua lock, coba lagi
   }

3. Single Lock
   Gunakan satu mutex untuk melindungi semua resource terkait

4. Try-Lock Pattern
   Coba take, jika gagal lepas semua dan retry

5. Hierarchy
   Beri level pada mutex, hanya boleh take level lebih tinggi

Code example untuk tiap strategi
```

---

## Slide 22: Race Condition
```
Judul: "Race Condition: Bug Tersembunyi"
Definisi:
"Race condition terjadi saat hasil bergantung pada
timing tak terduga dari multiple task"

Contoh klasik:
// BUGGY CODE!
counter++;  // ← NOT ATOMIC!

Assembly view:
  LDR R0, [counter]  // Load counter ke register
  ADD R0, R0, #1     // Increment
  STR R0, [counter]  // Store back

Task A: LDR (counter=5)
    ↓
Task B: LDR (counter=5), ADD, STR (counter=6)
    ↓
Task A: ADD, STR (counter=6) ← SEHARUSNYA 7!

Solusi: Gunakan MUTEX!
Visual: Timeline diagram menunjukkan interleaving
```

---

## Slide 23: Studi Kasus - Safe UART Print
```
Judul: "Case Study: Thread-Safe Serial Output"
Problem:
Task A: printf("Hello ")
Task B: printf("World!")
Output: "HeWorllod !" ← KACAU!

Solution dengan Mutex:
SemaphoreHandle_t xUartMutex;

void safePrint(const char* msg) {
    if(xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        printf("%s", msg);
        xSemaphoreGive(xUartMutex);
    }
}

// Atau untuk multi-line:
xSemaphoreTake(xUartMutex, portMAX_DELAY);
printf("Line 1\n");
printf("Line 2\n");  // Atomic block!
xSemaphoreGive(xUartMutex);

Output: Terurut dan rapi!
```

---

## Slide 24: Queue vs Semaphore vs Mutex - Kapan Pakai?
```
Judul: "Memilih Primitif yang Tepat"
Decision flowchart:

START
  ↓
Perlu transfer data?
  ├─ Ya → QUEUE
  ↓
Perlu signal ISR→Task?
  ├─ Ya → BINARY SEMAPHORE
  ↓
Perlu pool management?
  ├─ Ya → COUNTING SEMAPHORE
  ↓
Perlu protect resource?
  └─ Ya → MUTEX

Ringkasan tabel:
| Use Case | Primitif |
|----------|----------|
| Sensor data → Display | Queue |
| Button ISR → Handler | Binary Semaphore |
| 3 database connections | Counting Semaphore |
| Protect shared variable | Mutex |
```

---

## Slide 25: Producer-Consumer Pattern
```
Judul: "Design Pattern: Producer-Consumer"
Arsitektur:
[Producer 1] ─┐
[Producer 2] ─┼─→ [Bounded Queue] ─→ [Consumer]
[Producer N] ─┘

Keuntungan:
✓ Decoupling - Producer dan consumer independen
✓ Buffering - Menangani burst traffic
✓ Load balancing - Multiple consumers

Implementasi:
// Producers
void producer(void *pvParam) {
    Data_t data = collect_data();
    xQueueSend(xQueue, &data, portMAX_DELAY);
}

// Consumer
void consumer(void *pvParam) {
    Data_t data;
    xQueueReceive(xQueue, &data, portMAX_DELAY);
    process(data);
}

Visual: Pipeline diagram dengan buffer
```

---

## Slide 26: Event Groups (Advanced)
```
Judul: "Event Groups: Multiple Event Synchronization"
Skenario:
"Task harus menunggu SEMUA sensor selesai kalibrasi"

Event bits:
#define TEMP_READY  (1 << 0)  // Bit 0
#define HUMID_READY (1 << 1)  // Bit 1
#define PRESS_READY (1 << 2)  // Bit 2

Sensor tasks set bit masing-masing:
xEventGroupSetBits(xEventGroup, TEMP_READY);

Main task menunggu semua:
xEventGroupWaitBits(
    xEventGroup,
    TEMP_READY | HUMID_READY | PRESS_READY,  // Wait for all
    pdTRUE,   // Clear bits on exit
    pdTRUE,   // Wait for ALL bits
    portMAX_DELAY
);
printf("All sensors ready!\n");

Visual: 3 LED indikator, semua nyala = go!
```

---

## Slide 27: STM32 Implementation Tips
```
Judul: "Tips Implementasi STM32"
1. NVIC Priority untuk ISR
   // NVIC priority harus >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
   HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);  // Contoh

2. Printf ke UART
   int _write(int file, char *ptr, int len) {
       HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
       return len;
   }

3. FreeRTOSConfig.h settings:
   #define configUSE_MUTEXES              1
   #define configUSE_RECURSIVE_MUTEXES    1
   #define configUSE_COUNTING_SEMAPHORES  1

4. Heap configuration:
   #define configTOTAL_HEAP_SIZE  (10 * 1024)  // 10KB for Blue Pill

Diagram: STM32 memory layout dengan FreeRTOS heap
```

---

## Slide 28: ESP32 Implementation Tips
```
Judul: "Tips Implementasi ESP32"
1. Dual Core Awareness
   // Sender di Core 0, Receiver di Core 1
   xTaskCreatePinnedToCore(sender, "Send", 4096, NULL, 2, NULL, 0);
   xTaskCreatePinnedToCore(receiver, "Recv", 4096, NULL, 2, NULL, 1);

2. Serial Mutex (penting!)
   SemaphoreHandle_t xSerialMutex = xSemaphoreCreateMutex();
   
   void safePrint(const char* msg) {
       xSemaphoreTake(xSerialMutex, portMAX_DELAY);
       printf("%s\n", msg);
       xSemaphoreGive(xSerialMutex);
   }

3. Heap monitoring:
   printf("Free heap: %lu\n", esp_get_free_heap_size());
   printf("Min free: %lu\n", esp_get_minimum_free_heap_size());

4. ISR dalam IRAM:
   void IRAM_ATTR buttonISR() { ... }

Diagram: ESP32 dual-core dengan shared queue
```

---

## Slide 29: Debugging Queue & Semaphore
```
Judul: "Teknik Debugging"
1. Monitor Queue Status:
   printf("Queue items: %lu\n", uxQueueMessagesWaiting(xQueue));
   printf("Queue space: %lu\n", uxQueueSpacesAvailable(xQueue));

2. Trace API Calls:
   #define TRACE_QUEUE 1
   #if TRACE_QUEUE
   printf("[TRACE] xQueueSend called, items now: %lu\n", ...);
   #endif

3. Deadlock Detection:
   // Selalu gunakan timeout
   if(xSemaphoreTake(mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
       printf("ERROR: Possible deadlock on mutex!\n");
       // Dump task states
   }

4. Stack Watermark:
   printf("Stack remaining: %u\n", uxTaskGetStackHighWaterMark(NULL));

5. Configurable hooks (FreeRTOSConfig.h):
   #define configCHECK_FOR_STACK_OVERFLOW 2
   #define configUSE_MALLOC_FAILED_HOOK   1

Visual: Debug output screenshot
```

---

## Slide 30: Common Mistakes
```
Judul: "Kesalahan Umum & Cara Menghindari"
❌ 1. Lupa xSemaphoreGive()
   → Deadlock! Selalu pastikan give setelah take
   
❌ 2. xSemaphoreTake() dari ISR
   → Gunakan xSemaphoreTakeFromISR() - tapi hati-hati!
   → Lebih baik: hanya Give dari ISR, Take dari task
   
❌ 3. Mutex untuk ISR signaling
   → Gunakan Binary Semaphore untuk ISR→Task
   
❌ 4. Queue penuh diabaikan
   → Selalu cek return value xQueueSend()
   
❌ 5. Stack overflow
   → Tingkatkan stack size atau kurangi local variables
   
❌ 6. portMAX_DELAY tanpa pertimbangan
   → Bisa menyebabkan task stuck selamanya

Visual: Setiap kesalahan dengan kode yang salah dan yang benar
```

---

## Slide 31: Best Practices Summary
```
Judul: "Ringkasan Best Practices"
10 Golden Rules:

1. 📦 Queue untuk DATA, Semaphore untuk SIGNAL
2. 🔒 Mutex untuk PROTECT resource
3. ⏱️ Gunakan TIMEOUT, hindari infinite wait
4. 📋 Check RETURN VALUE selalu
5. 🔄 Lock ordering KONSISTEN
6. 📊 Keep CRITICAL SECTION singkat
7. 🐛 Monitor dengan TRACE dan status functions
8. 💾 SIZING queue dan stack dengan tepat
9. 🚨 ISR hanya untuk Give/SendFromISR
10. 📝 DOKUMENTASI primitif yang digunakan

Visual: Checklist dengan centang hijau
Footer: "Follow these rules for robust RTOS applications!"
```

---

## Slide 32: Praktikum Preview
```
Judul: "Preview Praktikum"
10 Program yang akan dikerjakan:

STM32:
1. Basic Queue
2. Queue dengan Struct
3. Binary Semaphore (Button)
4. Counting Semaphore (Resource Pool)
5. Mutex UART Protection

ESP32:
6. Basic Queue
7. Dual-Core Queue
8. Binary Semaphore
9. Counting Semaphore
10. Integrated System

Visual: Screenshot atau foto hardware setup
Persiapan: LED, button, potensiometer, USB serial
```

---

## Slide 33: Q&A
```
Judul: "Pertanyaan?"
Konten:
"Tanya apa saja tentang:
- Queue API
- Semaphore types
- Mutex dan priority inheritance
- Deadlock prevention
- STM32 vs ESP32 differences"

Visual: Ikon tanda tanya besar
Contact info instruktur
Link ke resource tambahan
```

---

## Slide 34: Penutup
```
Judul: "Rangkuman Modul 10"
Key takeaways:
✅ Queue = Data transfer antar task (FIFO, copy-based)
✅ Binary Semaphore = Event signaling (ISR→Task)
✅ Counting Semaphore = Resource pool management
✅ Mutex = Resource protection dengan priority inheritance
✅ Hindari deadlock dengan lock ordering & timeout

Next: Modul 11 - FreeRTOS Timer & Task Notification

Visual: Diagram lengkap semua primitif dan use case-nya
Motivasi: "Dengan sinkronisasi yang tepat, aplikasi RTOS Anda
          akan handal dan bebas race condition!"
```

---

*Akhir PPT Modul 10*
