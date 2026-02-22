# PPT Prompts Modul 12: FreeRTOS Memory Management & Advanced Features (Set 1)

## Instruksi untuk AI Image/Slide Generator

Buat presentasi PowerPoint 18 slide untuk mata kuliah Praktikum Sistem Embedded, Modul 12: FreeRTOS Memory Management & Advanced Features.

---

### Slide 1: Cover
**Judul**: "Modul 12: FreeRTOS Memory Management & Advanced Features"
**Subtitle**: "Praktikum Sistem Embedded"
**Visual**: Diagram memory map MCU dengan blok-blok heap, stack, dan flash
**Elemen**: Logo universitas, nama dosen, tanggal

### Slide 2: Tujuan Pembelajaran
**Judul**: "Tujuan Pembelajaran"
**Konten**: 8 poin tujuan:
1. Memonitor heap & stack
2. Deteksi stack overflow
3. Static allocation
4. Memory pool pattern
5. Stream & Message Buffer
6. Critical section
7. ESP32 multi-heap & PSRAM
8. Memory leak detection

### Slide 3: Mengapa Memory Management Penting?
**Judul**: "Memory: The Most Critical Resource"
**Visual**: Perbandingan RAM: Desktop (16 GB) vs ESP32 (320 KB) vs STM32F103 (20 KB)
**Konten**: Infographic dengan ikon peringatan: stack overflow, heap exhaustion, fragmentation, memory leak
**Catatan**: Tampilkan bar chart perbandingan ukuran RAM

### Slide 4: FreeRTOS Heap API
**Judul**: "Dynamic Memory Allocation"
**Konten**: Tabel API: pvPortMalloc, vPortFree, xPortGetFreeHeapSize, xPortGetMinimumEverFreeHeapSize
**Visual**: Diagram alur: Request → pvPortMalloc → Success/Fail → Hook
**Kode**: Snippet contoh penggunaan

### Slide 5: Lima Skema Heap
**Judul**: "5 Heap Schemes: heap_1 to heap_5"
**Visual**: Tabel perbandingan 5 skema dengan ikon ✅/❌
**Konten**: Kolom: Alloc, Free, Coalescing, Multi-region, Fragmentasi, Use Case
**Highlight**: heap_4 sebagai RECOMMENDED

### Slide 6: Heap_4 Coalescing
**Judul**: "Heap_4: First-Fit + Coalescing"
**Visual**: Diagram animasi 3 fase:
  - Fase 1: Blok A-B-C-D-E terisi
  - Fase 2: Free B dan D → gap kecil (heap_2)
  - Fase 3: Coalescing → gap besar (heap_4)
**Konten**: Penjelasan mengapa coalescing mengurangi fragmentasi

### Slide 7: Stack Management
**Judul**: "Stack: Every Task Has Its Own"
**Visual**: Diagram stack task dengan region: saved context, local vars, unused space, guard pattern
**Konten**: Tabel satuan stack: STM32 (words) vs ESP32 (bytes)
**Alert**: "Stack overflow = Hard Fault = System Crash!"

### Slide 8: Stack Overflow Detection
**Judul**: "Detecting Stack Overflow"
**Visual**: Side-by-side Method 1 vs Method 2
**Konten**: Method 1 (SP check) vs Method 2 (pattern 0xA5) dengan pro/con
**Kode**: vApplicationStackOverflowHook snippet

### Slide 9: Static Allocation
**Judul**: "Static vs Dynamic Allocation"
**Visual**: Tabel perbandingan: Sumber memori, Bisa gagal?, Fragmentasi, Deterministic
**Kode**: xTaskCreateStatic() contoh dengan static buffer
**Konten**: Kapan menggunakan static (safety-critical)

### Slide 10: Memory Pool Pattern
**Judul**: "Memory Pool: Zero Fragmentation"
**Visual**: Diagram pool: 8 blok × 64 bytes dengan free-list queue
**Konten**: Keuntungan: O(1), thread-safe, no fragmentation
**Kode**: Pool init, alloc, free snippet

### Slide 11: Stream Buffer
**Judul**: "Stream Buffer: Byte Stream Transfer"
**Visual**: Diagram sender → stream buffer → receiver dengan trigger level
**Konten**: Perbandingan Stream Buffer vs Queue
**Kode**: xStreamBufferCreate, xStreamBufferSend, xStreamBufferReceive

### Slide 12: Message Buffer
**Judul**: "Message Buffer: Discrete Messages"
**Visual**: Diagram pesan berbeda ukuran dalam buffer dengan header 4-byte
**Konten**: Perbedaan dari Stream Buffer (discrete vs continuous)
**Kode**: xMessageBufferCreate, Send, Receive

### Slide 13: Critical Section
**Judul**: "Critical Section: Atomic Operations"
**Visual**: Timeline 2 task: tanpa proteksi (interleaved = corrupt) vs dengan proteksi (serialized = correct)
**Konten**: 3 level: SuspendAll, ENTER_CRITICAL, DISABLE_INTERRUPTS
**Warning**: "Keep critical sections SHORT!"

### Slide 14: ESP32 Memory Architecture
**Judul**: "ESP32: Multi-Heap Architecture"
**Visual**: Memory map ESP32: DRAM, IRAM, SPIRAM, RTC RAM dengan capability flags
**Konten**: Tabel capability flags: MALLOC_CAP_DEFAULT, SPIRAM, DMA, EXEC
**Kode**: heap_caps_malloc() contoh

### Slide 15: STM32 Memory Architecture
**Judul**: "STM32: SRAM & Linker Control"
**Visual**: Memory map STM32F103 dan STM32F4 (dengan CCM RAM)
**Konten**: configTOTAL_HEAP_SIZE sizing guide
**Tabel**: Sizing recommendations per task type

### Slide 16: Heap Fragmentation
**Judul**: "Fragmentasi: The Silent Killer"
**Visual**: Diagram fragmentasi: blok-blok kecil kosong tersebar
**Formula**: Fragmentation Index = 1 - (largest_block / total_free)
**Konten**: Strategi mitigasi: heap_4, pool, static, uniform sizes

### Slide 17: Memory Leak Detection
**Judul**: "Finding Memory Leaks"
**Visual**: Grafik heap size vs waktu: garis menurun = LEAK
**Konten**: Teknik: heap trend monitoring, allocation tracker, ESP32 heap_trace
**Kode**: Leak detector task snippet

### Slide 18: System Dashboard (Capstone)
**Judul**: "Putting It All Together"
**Visual**: Screenshot dashboard output di serial monitor
**Konten**: vTaskList(), vTaskGetRunTimeStats(), HeapStats_t
**Summary**: Gabungan semua konsep Modul 09-12

---

## Catatan Desain
- Warna tema: Hijau gelap (#1a5632) dengan aksen kuning (#ffc107)
- Font: Consolas untuk kode, Segoe UI untuk teks
- Setiap slide maksimal 6 bullet points
- Gunakan diagram dan visual sebanyak mungkin
- Sertakan QR code ke dokumentasi FreeRTOS
