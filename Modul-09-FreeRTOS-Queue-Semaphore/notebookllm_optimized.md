# Modul 09: FreeRTOS Queue dan Semaphore — Komprehensif 45 Slide

Integrasi lengkap Modul 09 Jobsheet+Materi+Project+TugasVideo untuk NotebookLM.

---

## SLIDE 1: Pendahuluan

Sistem multitasking: task paralel saling komunikasi. Analogi: departemen kantor kirim dokumen. Queue=kotak surat FIFO. Semaphore=lampu lalu lintas event. Mutex=kunci kamar (satu akses). Tanpa mekanisme→race condition, data rusak. Modul 09 ajarkan transfer aman & sinkronisasi task-ISR.

---

## SLIDE 2: Race Condition Contoh

Dua orang tulis papan tanpa koordinasi. Counter awal 0. A+1, B+1. Harusnya 2, realitas 1. Penyebab: read-modify-write atomik gagal. A baca(0), B baca(0), A tulis(1), B tulis(1). Hilang 1! Solusi: mutex/queue jamin akses beratur.

---

## SLIDE 3: Queue Pengenalan

FIFO: masuk duluan keluar duluan. Analogi kasir: pelanggan depan dilayani duluan. Data copy (by-value) aman dari shared memory. Producer xQueueSend(). Consumer xQueueReceive(). Blocking: tunggu kosong/penuh. Ukuran konfigurasi.

---

## SLIDE 4: Queue API Buat Kirim

Buat: xQueue=xQueueCreate(10, sizeof(int)) kapasitas 10 per item 4byte. Kirim: xQueueSend(xQueue, &data, 100ms). ISR: xQueueSendFromISR()+portYIELD_FROM_ISR() no-block. Alternatif: SendToFront(priority) vs SendToBack(default).

---

## SLIDE 5: Queue API Terima Info

Terima: xQueueReceive(xQueue, &buffer, MAX_DELAY) tunggu indefinite. Intip: xQueuePeek() tanpa hapus. Info: uxQueueMessagesWaiting()=count, uxQueueSpacesAvailable()=free. Reset: xQueueReset(). ISR: xQueueReceiveFromISR().

---

## SLIDE 6: P01 Queue Dasar ESP32

Producer kirim counter++ setiap 1s xQueueSend(). Consumer terima xQueueReceive() toggle LED. Queue size 10. Output: kirim,terima,fill level. Consumer lambat→queue penuh→producer block. Flow control natural. Catat timing.

---

## SLIDE 7: P01 Queue Dasar STM32

Setup FreeRTOS CubeMX/PlatformIO. LED PC13/PB0. Producer kirim++count 1s. Consumer terima toggle print. Queue sebelum vTaskStartScheduler(). Monitor serial 115200 send/terima count dan timing.

---

## SLIDE 8: P02 Queue Struct

Kirim struct bukan integer. Define: struct{uint8_t id; float temp; uint32_t ts;}. Queue copy struct setiap send. Aman, no reference. Tiga sensor→satu queue→processor parse by id. Multiple producer one consumer.

---

## SLIDE 9: P02 Struct Implementasi ESP32

Data.id=TEMP; data.temp=readTemp(); xQueueSend(queue,&data,MAX). Recv: xQueueReceive(queue,&data,MAX); if(id==TEMP){...}. Copy by-value, jangan pointer lokal. Latency>integer tapi aman.

---

## SLIDE 10: P02 Struct Implementasi STM32

EXTI callback HAL_GPIO_EXTI_Callback() ISR. Timer interrupt 500ms baca ADC→xQueueSendFromISR()+portYIELD_FROM_ISR(). Processor task sama kedua platform.

---

## SLIDE 11: P03 Multiple Queue

Dua queue: command & status. Commander→cmd_queue→Executor→status_queue→Display. Request-response pattern. Decoupled, mudah debug. Extensible >2 queue.

---

## SLIDE 12: P03 Multiple Implementasi

Setup cmd(5), status(5). Commander send cmd. Executor recv cmd→exec→send status. Monitor recv→display. Hati hati: jangan mutual block deadlock.

---

## SLIDE 13: P04 Queue ISR Fast Response

Button ISR→xQueueSendFromISR()→task handler→toggle LED. ISR no-blocking. Parameter pxHigherPriorityTaskWoken context switch. portYIELD_FROM_ISR() jika ada task tinggi terbangun. Latency ideal μs.

---

## SLIDE 14: P04 ISR ESP32

ISR handler xQueueSendFromISR() HigherPri flag. Setup gpio_install_isr_service(); gpio_isr_handler_add(). Task recv xQueueReceive()+toggle. No-blocking calls ISR!

---

## SLIDE 15: P04 ISR STM32

EXTI setup PA0 rising. Callback HAL_GPIO_EXTI_Callback()→xQueueSendFromISR(). Task recv→toggle. Debounce vTaskDelay() handler not ISR.

---

## SLIDE 16: P05 Queue Set Multiplexing

SelectFromSet wait multiple queue. Create(30). AddToSet(queue). SelectFromSet()→queue siap. If(q==q1)vs q2. Satu task, no polling.

---

## SLIDE 17: P05 Queue Set Implementasi

Create alarm, data queue. CreateSet(15). Both AddToSet(). SelectFromSet()→if alarm→priority first. If data→process. Alarm prioritize.

---

## SLIDE 18: Binary Semaphore Konsep

State 0 atau 1. Create awal kosong. Give=1, Take tunggu 1→0. Lampu traffic. ISR give→task take. Event signal. No ownership, no inheritance (beda mutex).

---

## SLIDE 19: P06 Binary ISR to Task

Button ISR xSemaphoreGiveFromISR()→task xSemaphoreTake()→LED toggle. Catat: multiple ISR trigger→event miss, state cuma 0/1. Beda queue buffer semua.

---

## SLIDE 20: Binary Event Miss Analysis

10× press→5× tog: 5 miss. State 0/1 only. ISR give state=1→ignore. Solusi: queue atau counting sem count semua. Counting++, sig--, no loss.

---

## SLIDE 21: Counting Semaphore Resource Pool

Parkir max 3 slot. Create(3,3). Task take(--), give(++). Task 4→blocked tunggu. Resource availability. Control concurrency max N.

---

## SLIDE 22: P07 Counting Parkir

5 worker, 3 resource. Worker take(wait full)→work 2s→release. Setup(3,3). Max 3 active. Statistik wait. uxSemaphoreGetCount() monitor.

---

## SLIDE 23: Mutex Resource Protection

Ownership: hanya take task bisa give. Kunci kamar. Priority inheritance, prevent accidental release. Shared var, UART, data struct.

---

## SLIDE 24: P08 Mutex Race Demo

P08 P1: no mutex. 2 task increment 10k each. Expect 20k, got 19.xxx lost. P08 P2: with mutex wrap counter. Result 20k always. Correctness>speed. Unprotected var=bug.

---

## SLIDE 25: Mutex Priority Inheritance Inversion Fix

Inversion: High wait Low mutex, Med preempt Low. Bad! Solution: inherit. High take→Low naik High pri. Low finish→High dapat. configUSE_MUTEXES=1.

---

## SLIDE 26: P09 Priority Inversion Demo

High(3), Med(2), Low(1). Low hold mutex 3s. High block. Med run. No inherit: High block 2+s. Inherit: Low→pri3, Med can't. Mutex safer.

---

## SLIDE 27: Recursive Mutex Nested

Allow task take same mutex multiple. Nested: outer→take, call inner→take OK. Create recursive, TakeRecursive/GiveRecursive. Give=take count. Inherit support.

---

## SLIDE 28: P10 Recursive Nested Config

output_config() take→update_floor() take. No recursive: deadlock. Recursive: level 2 OK. Display nesting. Track max. Task A update→Task B wait. A done→B access.

---

## SLIDE 29: Producer-Consumer Pattern Bounded Buffer

3 producer diff speed, 1 consumer, queue 8. Send, block full. Recv, process, delete. Monitor fill. Stats: sent, process, blocked. Throughput item/s. Trade queue size vs memory.

---

## SLIDE 30: Producer-Consumer Implementasi

Producer xQueueSend()+stats. uxQueueMessagesWaiting(). Consumer xQueueReceive()+delay. Handle full/empty. Create(8,sizeof(int)).

---

## SLIDE 31: Reader-Writer Lock Concurrent Readers

Database: many reader concurrent, writer exclusive. Pattern: 2mutex+counter. First read lock writer, last unlock. Writer exclusive. Library: read concurrent, librarian exclusive.

---

## SLIDE 32: Reader-Writer Implementasi

Reader mutex protect counter, writer mutex protect data. First lock writer, last unlock. Writer exclusive take/give. Reader read concurrent. Code counter++/-- lock.

---

## SLIDE 33: FreeRTOS STM32 Setup

CubeMX FreeRTOS CMSIS_V2. PlatformIO lib_deps. Config enable counting, mutex, recursive, set. Build pio run. Upload t upload. Monitor 115200.

---

## SLIDE 34: FreeRTOS ESP32 Setup

IDF built-in FreeRTOS. Dual-core xTaskCreatePinnedToCore(). Queue thread-safe cross core. Build pio. Monitor 115200. UART1 GPIO1/3 USB.

---

## SLIDE 35: Best Practices Troubleshooting

1. Queue full? Increase size or speed. 2. Deadlock? Timeout takes. 3. ISR safe? xQueueSendFromISR() only. 4. Inversion? Mutex no sem. 5. Memory? Monitor stack. 6. Naming x*Handle v*Func. 7. Comment purpose. 8. Test load burst.

---

## SLIDE 36: SmartPark Sistem Deskripsi

3 lantai parkir 150 slot. 2 entry+2 exit gate+sensor occupancy. Task: ISR→queue→Controller→sem→cmd→Actuator→status→Display. Mutex revenue. RW config. 12 feature integrate.

---

## SLIDE 37: SmartPark Fitur Implementasi

1-2: Queue basic+struct. 3: CMD-STS queue. 4: ISR gate. 5: QueueSet alarm. 6: Binary barrier. 7: Counting/lantai. 8: Mutex revenue. 9: Priority inv. 10: Recursive config. 11: Producer 2gate. 12: RW tariff. Modular.

---

## SLIDE 38: SmartPark Task Queue Arsitektur

Gate→entry_queue→Controller(3)→cmd_queue→Actuator(2)→status_queue→Display(1). Sem/lantai. Mutex revenue. RW config. Performance smooth, deadlock-free, responsive.

---

## SLIDE 39: SmartPark Testing Debugging

Unit: queue 100, full occupancy, race, priority. Serial [TIME][TASK][LEVEL]msg. Tools vTaskList(), stack. Metrics <100ms latency, max fill, contention. Load 5min peak hour.

---

## SLIDE 40: SmartPark Rubrik Penilaian

100pt: Queue(20)5type. Sem&Mutex(20)binary,count,inherit,recursive. Pattern(15)consumer+RW. Integration(25)12feature. Code(10)modular. Bonus(10). Final 10min demo 5Q answer.

---

## SLIDE 41: Video Teknis Requirement

20-35min MP4 720p30fps. Clear audio narasi. Webcam PIP 15%, hardware cam. Smooth edit no lag. Upload YT unlisted/GDrive. Deadline 1 week. Penalty 10%/day late.

---

## SLIDE 42: Video Konten Pembukaan

Intro 1m: nama NIM kelas platform. Overview modul inter-task comm. Board visual. Explain 3-4m: Queue FIFO analogi. Sem binary/counting. Mutex ownership inherit. Case race, deadlock, inversion. Diagram. Code API.

---

## SLIDE 43: Video Konten Demo Eksperimen

Expt 1-12 setiap: code 30s highlight. Build pio run. Jalankan serial+LED. Analyze queue full→consumer wait atau binary miss. Record stats. Bonus hardware demo button+sensor+LED. ESP32 dual-core. STM32 EXTI+ADC queue.

---

## SLIDE 44: Video Konten Project Demo

Architecture diagram. Vehicle entry ISR→queue→controller→gate LED. Occupancy counting. Full→redirect. CMD-STS entry→queue→actuator→status→display. Revenue mutex. Priority alarm ISR vs stat. Serial realtime. Queue fill, latency stats.

---

## SLIDE 45: Video Penilaian Catatan Akhir

15% explain accuracy. 40% demo 12 expt. 25% project. 10% quality. 10% analysis. Penalty -10% no cam, -15% no narration, -5% expt no-run, -10%/day late. Explain why not just works. Deep understanding not mechanical. <20m or >35m rejected. <100MB ideal.

---

*Modul 09 — 45 Slide Komprehensif | Praktikum Sistem Embedded 2025/2026*
