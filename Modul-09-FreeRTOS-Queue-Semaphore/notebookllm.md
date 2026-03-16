# Modul 09: FreeRTOS Queue dan Semaphore

Dokumen singkat 45 slide untuk NotebookLM | Maksimal 4000 char per 15 slide.

---

## SLIDE 1: Inter-Task Communication

Sistem multitasking perlu saling komunikasi aman. **Queue**: FIFO buffer data. **Semaphore**: event signaling (binary) atau resource counting. **Mutex**: proteksi shared resource dengan ownership. Tanpa mekanisme → race condition, data corrupt.

---

## SLIDE 2: Race Condition Contoh

Counter 0. Task A baca(0), B baca(0), A tulis(1), B tulis(1). Hasil 1 (harusnya 2). Penyebab: read-modify-write tidak atomic. Solusi: mutex lock atau queue buffer.

---

## SLIDE 3: Queue Dasar

FIFO: masuk duluan keluar duluan. Producer `xQueueSend()`, Consumer `xQueueReceive()`. Data di-copy (by-value). Blocking saat kosong/penuh. Setup: `xQueueCreate(capacity, item_size)`.

---

## SLIDE 4: Queue API

Buat: `xQueueCreate(10, sizeof(int))`. Kirim: `xQueueSend(queue, &data, 100ms)`. ISR: `xQueueSendFromISR(&woken)`. Terima: `xQueueReceive(queue, &buf, MAX)`. Info: `uxQueueMessagesWaiting()`, `uxQueueSpacesAvailable()`.

---

## SLIDE 5: P01 Queue Dasar (ESP32/STM32)

Producer kirim counter++ setiap 1s. Consumer terima, toggle LED. Queue size 10. Monitor: fill level, latency. Consumer lambat → queue penuh → producer block (flow control).

---

## SLIDE 6: P02 Queue Struct

Kirim struct `{id, temperature, timestamp}` bukan scalar. Tiga sensor → satu queue → processor parse by id. Multiple producer, one consumer pattern. Copy by-value aman.

---

## SLIDE 7: P03 Multiple Queue

Dua queue: cmd & status. Commander → cmd → Executor → status → Display. Request-response decoupled pattern. Extensible > 2 queue tanpa deadlock mutual.

---

## SLIDE 8: P04 Queue dari ISR

Button ISR `xQueueSendFromISR()` → task `xQueueReceive()` → toggle LED. ISR no-blocking. Parameter `pxHigherPriorityTaskWoken` & `portYIELD_FROM_ISR()` untuk fast context switch.

---

## SLIDE 9: P05 Queue Set

Tunggu multiple queue sekaligus. Create: `xQueueCreateSet(30)`. AddToSet(queue). SelectFromSet() → return queue siap. Satu task handle banyak queue, no polling.

---

## SLIDE 10: Binary Semaphore

State 0/1 only. Give=1, Take=wait 1→0. Event signaling. No ownership. ISR give → task take. Tidak miss event jika trigger cepat (beda queue buffer).

---

## SLIDE 11: P06 Binary Semaphore ISR

Create awal kosong. ISR `xSemaphoreGiveFromISR()` → task `xSemaphoreTake()` → LED. Catat: multiple trigger miss karena state cuma 0/1 (beda queue).

---

## SLIDE 12: Counting Semaphore

Resource pool max N. Create(3,3) = 3 slot parkir. Task take(–), give(+). Task 4 block tunggu. Gunakan: resource limiting, concurrency control.

---

## SLIDE 13: P07 Counting Semaphore

5 worker, 3 resource. Worker take → work 2s → release. Max 3 concurrent. Monitor dengan `uxSemaphoreGetCount()`. Statistik: wait time, utilization.

---

## SLIDE 14: Mutex Ownership

Hanya take task bisa give. Ownership. Priority inheritance (cegah inversion). Protect shared var, UART, data struct. Beda semaphore: no ownership, no inheritance.

---

## SLIDE 15: P08 Mutex Race Demo

P1: no mutex. 2 task += 10k → hasil 19.xxx (miss). P2: with mutex → 20k (correct). Overhead ~10-20 μs. Trade: correctness > speed.

---

## SLIDE 16: Priority Inversion

High wait Low (hold mutex), Med preempt Low. High tertunda! Solusi: priority inheritance. Low naik ke High saat memegang. `configUSE_MUTEXES=1` enable.

---

## SLIDE 17: P09 Priority Inversion Demo

High(3), Med(2), Low(1). Low hold 3s. High block. Med run. No inherit: High-Med both active → High blocked long. With inherit: Low boosted → finish fast.

---

## SLIDE 18: Recursive Mutex

Task dapat take same mutex multiple kali tanpa deadlock. Nested function: outer→take, call inner→take OK. TakeRecursive/GiveRecursive. Give count = take count.

---

## SLIDE 19: P10 Recursive Mutex

Config: output_config()→take→update_floor()→take. No recursive=deadlock. Recursive=OK (level 2). Track nesting depth. Task A → B blocked until A fully release.

---

## SLIDE 20: Producer-Consumer Pattern

Bounded buffer. 3 producer (diff speed) → queue(8) → 1 consumer. Block jika penuh. Monitor fill level. Stats: sent, processed, blocked. Throughput items/sec.

---

## SLIDE 21: P11 Producer-Consumer

Setiap producer kirim item random delay berbeda. Consumer proses batch. Queue buffer 8. Amati: fill level cycle, producer block frequency, throughput stat per producer.

---

## SLIDE 22: Reader-Writer Lock

Database: many reader concurrent, writer exclusive. Pattern: 2 mutex + reader counter. First reader lock writer, last unlock. Java-style read-write synchronization.

---

## SLIDE 23: P12 Reader-Writer Lock

Implementasi: reader task increment count dalam mutex 1, lock writer mutex 2 jika first. Writer mutex 2 exclusive. Statistik: concurrent reader count, writer wait, read/write ratio.

---

## SLIDE 24: FreeRTOS STM32 Setup

CubeMX enable FreeRTOS. Config: `#define configUSE_COUNTING_SEMAPHORES 1`, `configUSE_MUTEXES 1`, `configUSE_RECURSIVE_MUTEXES 1`, `configUSE_QUEUE_SETS 1`. Build: `pio run`.

---

## SLIDE 25: FreeRTOS ESP32 Setup

IDF built-in FreeRTOS. Dual-core `xTaskCreatePinnedToCore()` core 0/1. Queue thread-safe cross core. Monitor 115200 baud UART1.

---

## SLIDE 26: Best Practices

1. Queue penuh → increase size or speed. 2. Deadlock → timeout all takes. 3. ISR → xQueueSendFromISR() only. 4. Inversion → mutex not sem. 5. Memory → monitor stack. 6. Naming x*Handle, v*Func. 7. Comment purpose. 8. Test load.

---

## SLIDE 27: SmartPark Project Deskripsi

3-floor parking 150 slot. 2 entry + 2 exit gate + sensor. Architecture: ISR→queue→Controller→sem alloc→cmd→Actuator→status→Display. Mutex revenue. RW-lock config tariff.

---

## SLIDE 28: SmartPark Fitur 1-6

F1: Queue entry event. F2: Struct vehicle data. F3: CMD-status pipeline. F4: ISR gate sensor. F5: QueueSet alarm priority. F6: Binary sem barrier 1 vehicle/time.

---

## SLIDE 29: SmartPark Fitur 7-12

F7: Counting sem per lantai occupancy. F8: Mutex revenue counter (demo race+fix). F9: Priority inv alarm vs stat. F10: Recursive mutex nested config. F11: Producer-consumer 2 gate. F12: RW-lock tariff database.

---

## SLIDE 30: SmartPark Task Architecture

Entry/Exit ISR → entry/exit queue → Controller(pri3) → cmd queue → Actuator(pri2) → status queue → Display(pri1). Counting sem L1/L2/L3. Background stat task. Alarm QueueSet high priority.

---

## SLIDE 31: SmartPark Implementation

Code modular: gate.c, controller.c, actuator.c, display.c. Vehicle flow: ISR→queue→alloc slot→open gate→occupy count down. Exit: release slot→count up→accept next. Revenue track mutex.

---

## SLIDE 32: SmartPark Testing Metrics

Unit test: queue 100 vehicle, full occupancy, race mutex, priority inv. Serial log: [TIME][TASK][LEVEL]msg. Tools: vTaskList(), uxTaskGetStackHighWaterMark(). Target: latency <100ms, no locks, responsive deadlock-free.

---

## SLIDE 33: Video Teknis

Duration 20-35 min MP4 720p30fps. Clear audio narasi. Webcam PIP 15%. Hardware demo button/LED. Smooth edit. Upload YouTube unlisted or GDrive. Deadline 1 week. Penalty 10%/day.

---

## SLIDE 34: Video Pembukaan (5 menit)

Intro 1m: nama, NIM, kelas, platform. Overview modul 09 inter-task comm. Board visual. Teori 3-4m: Queue FIFO, Semaphore binary/counting, Mutex ownership, inheritance. Analogi. Diagram. Code snippet API.

---

## SLIDE 35: Video Demo Eksperimen (10-18 menit)

Setiap P01-P12: (a) code 30s, (b) build pio run, (c) jalankan serial+LED, (d) analyze, (e) catat stats. Bonus: hardware button press, sensor, LED toggle real. ESP32: dual-core stat. STM32: EXTI+ADC.

---

## SLIDE 36: Video Demo SmartPark (4-6 menit)

Architecture diagram task×queue. Vehicle entry ISR→queue→alloc→gate(LED)→occupancy. Command-status pipeline display. Revenue mutex demo. Priority alarm interrupt vs stat task. Latency, queue fill serial output. Live behavior.

---

## SLIDE 37: Video Penilaian

Grading: 15% explain (queue/sem/mutex concept akurasi), 40% demo (12 expt berjalan), 25% project (12 feature + integration), 10% quality (audio/visual), 10% analysis (reasoning). Penalty: -10% no cam, -15% no narration, -5% expt no-run, -10%/day late.

---

## SLIDE 38: Queue vs Shared Variable

Queue: data copy, blocking, FIFO buffer, aman concurrent. Shared var: race condition, lost update, sync manual. Kapan queue: data exchange. Kapan shared var: simple flag (gunakan semaphore lebih aman).

---

## SLIDE 39: Semaphore vs Mutex

Semaphore: event signaling, no ownership (siapa saja give), no inheritance. Mutex: resource protection, ownership, priority inheritance. Kapan sem: ISR-task signal. Kapan mutex: protect shared data, resource critical section.

---

## SLIDE 40: Binary vs Counting Semaphore

Binary: 0/1 state, miss event jika trigger cepat. Counting: counter N, no miss. Kapan binary: flag, handshake. Kapan counting: resource pool, bounded limit, permit count event.

---

## SLIDE 41: Queue Set Advantage

Tunggu multiple queue 1 task. Multiplexing. No polling loop. SelectFromSet() return siap. Kapan gunakan: multiple sensor input, command+alarm queue priority. Kapan tidak: queue count <2.

---

## SLIDE 42: Deadlock Pencegahan

Always timeout takes, no portMAX_DELAY blindly. Consistent mutex order. no circular wait. Keep critical section short. Monitor: task stat, queue fill, contention count. Test high load burst.

---

## SLIDE 43: Stack Memory Monitoring

`uxTaskGetStackHighWaterMark()` check remaining. ISR stack separate. Queue × item_size = memory. Semaphore minimal. Mutex minimal. Avoid queue large unused. Profile real-time load.

---

## SLIDE 44: ISR Best Practice

Use `xQueueSendFromISR()`, `xSemaphoreGiveFromISR()`. Never blocking call. Variable `pxHigherPriorityTaskWoken` & `portYIELD_FROM_ISR()`. ISR fast return. Actual work di task, tidak ISR lengthy.

---

## SLIDE 45: Kesimpulan Modul 09

Queue untuk data transfer. Semaphore untuk event/resource. Mutex untuk data protection + priority safety. Design pattern: producer-consumer, reader-writer. Real-time system perlu sinkronisasi aman. Project SmartPark integrate semua konsep modul 09.

---

*Modul 09 FreeRTOS Queue Semaphore | 45 slide × ~267 char = 12,000 total | NotebookLM optimal*
