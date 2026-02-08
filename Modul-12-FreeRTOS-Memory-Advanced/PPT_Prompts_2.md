# PPT Prompts Modul 12: FreeRTOS Memory Management & Advanced (Set 2 — Perspektif Praktis)

## Instruksi untuk AI Slide Generator

Buat presentasi 16 slide dengan fokus **hands-on dan debugging** untuk Modul 12 Memory Management.

---

### Slide 1: Cover
"Modul 12: Debugging Memory Problems in FreeRTOS"
Visual: Magnifying glass melihat memory map MCU

### Slide 2: Common Memory Bugs
"5 Masalah Memori Paling Sering"
1. Stack Overflow → Hard Fault
2. Heap Exhaustion → pvPortMalloc returns NULL
3. Memory Leak → Heap perlahan habis
4. Fragmentation → Alloc besar gagal meski total free cukup
5. Use After Free → Data corrupt random
Visual: Ikon error untuk setiap masalah

### Slide 3: Tools of the Trade
"Senjata untuk Debug Memory"
- xPortGetFreeHeapSize() — Berapa free?
- xPortGetMinimumEverFreeHeapSize() — Pernah sekritis apa?
- uxTaskGetStackHighWaterMark() — Seberapa dekat overflow?
- vTaskList() — Siapa saja task yang jalan?
- vTaskGetRunTimeStats() — Siapa yang paling boros CPU?
- Python serial parser — Visualisasi data real-time

### Slide 4: Live Demo 1 - Heap Monitor
"Percobaan 01: Monitoring Heap Real-Time"
Screenshot output serial + Python plot
Diagram: Heap timeline dengan alloc/free events

### Slide 5: Live Demo 2 - Stack Overflow
"Percobaan 03: Deliberately Crashing a Task"
Code: Task dengan rekursi tanpa batas
Output: Stack overflow hook triggered
Lesson: SELALU set configCHECK_FOR_STACK_OVERFLOW = 2

### Slide 6: Sizing Guide
"Berapa Stack yang Cukup?"
Tabel sizing:
- LED toggle: 128 words (STM32) / 2048 bytes (ESP32)
- printf: 256 words / 4096 bytes
- String processing: 512 words / 8192 bytes
Rule: Monitor HWM, aim for 30%+ margin

### Slide 7: Live Demo 3 - Fragmentation
"Percobaan 09: When Free Memory Isn't Enough"
Animasi: Alloc pattern → fragment → alloc besar FAIL
Metric: Fragmentation Index

### Slide 8: Solving Fragmentation
"Strategi Anti-Fragmentasi"
1. heap_4 (coalescing) — default, sudah bagus
2. Memory Pool — fixed size, zero fragment
3. Alloc at init only — gunakan heap_1
4. Static allocation — compile-time guarantee
5. Uniform sizes — hindari campuran

### Slide 9: Live Demo 4 - Static Allocation
"Percobaan 04: Zero Heap Usage"
Perbandingan RAM usage: dynamic vs static
Kode: xTaskCreateStatic(), vApplicationGetIdleTaskMemory()

### Slide 10: Stream & Message Buffer
"Percobaan 06-07: Lightweight Data Transfer"
Comparison table: Queue vs Stream Buffer vs Message Buffer
Use cases: UART RX (stream), Log messages (message), Sensor data (queue)

### Slide 11: Live Demo 5 - Critical Section
"Percobaan 08: Data Corruption Demo"
Side-by-side: Unprotected (corrupt count: 47) vs Protected (corrupt: 0)
Timeline diagram: Interleaved writes → checksum mismatch

### Slide 12: ESP32 vs STM32 Memory
"Platform-Specific Memory Management"
ESP32: Multi-heap, capability flags, PSRAM 4-8 MB
STM32: Single heap, configTOTAL_HEAP_SIZE, linker-controlled
Tabel perbandingan

### Slide 13: Live Demo 6 - Memory Leak
"Percobaan 11: Finding the Leak"
Grafik: Heap decreasing over time
Output: Allocation tracker log
Technique: Monitor, track, identify, fix

### Slide 14: Capstone Dashboard
"Percobaan 12: System Health Monitor"
Screenshot full dashboard output
Components: Task list + Heap stats + Stack HWM + Runtime %
Python visualization screenshot

### Slide 15: Best Practices Checklist
"Memory Management Checklist"
☑ Set configCHECK_FOR_STACK_OVERFLOW = 2
☑ Set configUSE_MALLOC_FAILED_HOOK = 1
☑ Monitor heap regularly
☑ Check stack HWM during development
☑ Use static alloc for critical tasks
☑ Prefer memory pool for uniform sizes
☑ Keep critical sections SHORT
☑ Test with worst-case scenarios

### Slide 16: Project Assignment
"Project: Sistem Monitor Kesehatan Embedded"
3 levels: Basic (70), Intermediate (85), Advanced (100)
Timeline: 4 minggu
Deliverables: Code, Python script, laporan, video

---

## Catatan
- Sertakan screenshot output serial yang realistis
- Gunakan animasi step-by-step untuk diagram memory
- Warna: Biru teknik (#0066cc) dengan merah untuk alert (#cc0000)
