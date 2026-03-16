# Modul 08: FreeRTOS Task Management â€” NotebookLM

---

## TEORI (Slide 1-35)

### Slide 1: RTOS Basics
RTOS menjamin waktu respons terprediksi. FreeRTOS gratis, 10KB RAM, preemptive scheduler. Task penting dapat interrupt task kurang penting kapan saja. RTOS cocok embedded require responsif terhadap multiple event urgent.

---

### Slide 2: Task Definition
Unit program independen dengan stack sendiri, TCB metadata, entry function infinite loop. Setiap task jalan seolah paralel. PENTING: task harus pakai vTaskDelay() beri kesempatan task lain, jangan return dari fungsi utama.

---

### Slide 3: Task States
State: READY (menunggu CPU), RUNNING (sedang jalan), BLOCKED (tunggu event), SUSPENDED (manual pause). Transisi: vTaskCreate()â†’READYâ†’Scheduler pilih tertinggiâ†’RUNNINGâ†’preemptedâ†’delayâ†’BLOCKED/SUSPENDEDâ†’eventâ†’READY.

---

### Slide 4: Priority & Preemption
Priority 0-max; semakin tinggi semakin cepat. Task A (pri 3) jalan, Task B (pri 4) ready â†’ scheduler interrupt A jalankan B. Sama priority â†’ time-slicing bergilir. Rate Monotonic: periode pendek prioritas tinggi, periode panjang rendah.

---

### Slide 5: Stack Memory Safety
Stack simpan lokal var, parameter, return address, registers. Operasi kompleks overflow â†’ crash. Hindari: hitung kebutuhan (lokal+nested+30% margin), deteksi hook, monitor uxTaskGetStackHighWaterMark(). HWM 234/512 = aman.

---

### Slide 6: Tick & Delay Mechanics
FreeRTOS basis tick (default 1000Hz = 1ms). vTaskDelay() = relative delay, natural jitter, cocok casual. vTaskDelayUntil() = absolute delay, zero drift, presisi periodik. pdMS_TO_TICKS() konversi millisecâ†’tick.

---

### Slide 7: Heap Management
Heap dinamis alokasi task/queue/semaphore. Default heap_4.c first-fit. STM32 10-20KB, ESP32 >500KB. Monitor free heap, alokasi startup, static allocation kritis. Leak jika delete tanpa free malloc internal.

---

### Slide 8: Context Switch Flow
Atomically: (1) Save register running task, (2) Find READY tertinggi, (3) Load pointer task new, (4) Restore register, (5) Jump. Semuanya microsecond. Seamless multitasking tanpa task tahu preemption.

---

### Slide 9: Bare-Metal vs RTOS
**Bare-metal:** Sequential loop deterministic tapi unresponsive urgent event. **RTOS:** Task independent scheduler allocate CPU; responsive event urgent immediate tapi overhead sedikit. Cocok sistem requirement beragam: sensor 100ms, display 1000ms, alarm instant.

---

### Slide 10: Priority Inversion Problem
Task prioritas tinggi di-delay rendah! Skenario: Low hold mutex â†’ High wait blocked â†’ Medium ready run â†’ Medium delay High! Solusi: Priority Inheritance â€” Low priority naik temporary High saat Low hold. STM32 configUSE_MUTEXES=1.

---

### Slide 11: ESP32 Core Affinity
Dual core 240MHz each. xTaskCreatePinnedToCore(..., 0/1/floating) pin atau floating task. Strategy: I/O Core 0, compute Core 1, flexible floating. Reduce context switch overhead. STM32 single-core no affinity.

---

### Slide 12: Watchdog Protection
Hardware reset MCU reset jika tidak feed timeout 4 detik. Task hang â†’ tidak feed â†’ timeout â†’ reset â†’ recover auto. STM32 IWDG, ESP32 Task WDT. Balance: pendek false trigger, panjang slow recovery.

---

### Slide 13: Idle Task Power
Task priority 0 jalan saat nothing lain. vApplicationIdleHook() callback custom. __WFI() ARM sleep CPU idle optimum power. JANGAN blocking idle hook deadlock. Idle task tidak suspend/delete user.

---

### Slide 14: Race Condition Demo
Task berbagi variable tanpa proteksi â†’ race condition. Consumer read saat Producer write â†’ torn read. CPU tidak atomic compound instruction, load/store beberapa instruction gap. Solution Modul 09: Queue atau Mutex.

---

### Slide 15: ESP32 vs STM32
**STM32:** Single core 72MHz, 20KB RAM, tick 1000Hz, stack words, IWDG. **ESP32:** Dual core 240MHz each, 520KB, tick 100Hz, stack BYTES (beda!), multi-region. Migration: adjust stack size tick factor. Setiap kelebihan.

---

### Slide 16: xTaskCreate API
`xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask)`. Return pdPASS atau error. Best: hitung stack, set priority, **infinite loop penting**, finite stack risk.

---

### Slide 17: Task Entry Function
`void vMyTask(void *p) { init; for(;;) { logic; vTaskDelay(...); } }`. Jangan return. Init once first run. Infinite loop. Blocking call beri chance task lain. Parameter bisa unused atau customize.

---

### Slide 18: vTaskDelay Casual
`vTaskDelay(pdMS_TO_TICKS(100))` = minimal 100ms. Relative delay dari sekarang. Jitter natural. Cocok non-critical display/log. NOT untuk periodik presisi PWM.

---

### Slide 19: vTaskDelayUntil Presisi
`vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(100))` absolute waktu. Zero drift periodic. Pattern: `TickType_t xLastWake = xTaskGetTickCount(); for(;;) { vTaskDelayUntil(&xLastWake, period); }`. Important control sensor/PID.

---

### Slide 20: Suspend Resume Control
`vTaskSuspend(xTask)` suspend, `vTaskResume()` resume. NULL = self. Dari ISR: `xTaskResumeFromISR()`. NOT sama delay â€” sleep sampai resume explicit. Use: maintenance mode, power saving.

---

### Slide 21: Task Delete Cleanup
`vTaskDelete(xTask)` hapus task free stack. NULL = self-delete. Pre-delete cleanup: close file, free malloc. Monitor heap leak: sebelum/sesudah create-delete kembali value awal. Long-run system hemat memory.

---

### Slide 22: Dynamic Priority Set
`vTaskPrioritySet(xTask, uxNewPriority)` ubah priority runtime. Contoh: emergency boost heater priority 4 rapid response. Selesai turun balik. Pattern: base fixed + boost anomaly temporary.

---

### Slide 23: Stack Size Rumus
Size = (local vars) + (call depth Ã— save) + (context) + (30% margin). STM32 words, ESP32 bytes. Thumb: simple 128, printf 256. Monitor HWM() â€” high danger.

---

### Slide 24: Scheduler Info APIs
`vTaskList()` task tabel: name/state/priority/HWM. `vTaskGetRunTimeStats()` CPU%. Combine dashboard comprehensive. Monitor 10sec CPU idle%, stack trend HWM.

---

### Slide 25: Stack Overflow Hook
Enable configCHECK_FOR_STACK_OVERFLOW=2. Pattern overwrite overflow â†’ call vApplicationStackOverflowHook(). Implementasi: log, restart, fault bit. Worst: silent corruption. Margin generous.

---

### Slide 26: Latihan 01 Create
2 task LED blink rate berbeda + monitor. STM32 CubeMX FREERTOS. LED1 pri 2 (500ms), LED2 pri 2 (1000ms), Monitor pri 1. Observation: LED toggle, task list, HWM. Deliverable: serial capture.

---

### Slide 27: Latihan 02 Priority
3 task High/Med/Low loop counter. Monitor CPU distribution. Awal High dominate. Input 'S' swap â†’ CPU rearrange. Analysis: preemptive allocate priority real-time.

---

### Slide 28: Latihan 03 Timing
Task A vTaskDelay(100ms) vs Task B vTaskDelayUntil(100ms). Collect 100 sample timing. Observation: Delay drift jitter, DelayUntil presisi Â±1ms zero drift. Insight: periodik pakai DelayUntil.

---

### Slide 29: Latihan 04 Suspend
LED toggle 500ms. Button interrupt suspend. Button lagi resume. vTaskSuspend()/vTaskResume(). State transition serial. Observation: LED stop blink suspend, continue resume. Maintenance mode demo.

---

### Slide 30: Latihan 05 Delete
Create task loop 20Ã—. Monitor heap before/after. Expected restore Â±50 byte. Leak symptom gradual down. Success cycle kembali normal. Insight: vTaskDelete() free stack, user free malloc().

---

### Slide 31: Latihan 06 Stack Monitor
3 task Small/Medium/Large stack. Recursive depth 10/20/30. Monitor HWM%. Over 80% warning. Trigger overflow Small depth 50+. Hook implement. Observation: Small tight, Large comfortable.

---

### Slide 32: Latihan 07 Affinity
**ESP32:** Pin Core 0/1/floating. Print xPortGetCoreID() setiap task. Measure execution time diff. Pinning reduce overhead. **STM32:** Priority inversion demo Low hold â†’ Medium delay High. Priority inheritance mutex solusi.

---

### Slide 33: Latihan 08 CPU Percent
configUSE_IDLE_HOOK=1. vApplicationIdleHook() idle counter. CPU% = 100Ã—(Totalâˆ’Idle)/Total. Load 0â€“95%. Monitor 5sec. Observation linear correlation loadâ†‘ CPU%â†‘. Power saving __WFI().

---

### Slide 34: Latihan 09 Watchdog
Good task feed normal, Bad NOT feed. Monitor 30sec WDT timeout. Reset recovery auto. **STM32:** IWDG HAL. **ESP32:** Task WDT. Deliverable: recovery boot log. Insight: failsafe critical, NEVER disable.

---

### Slide 35: Latihan 10 Race
Producer write, Consumer read shared var NO mutex. Corruption detect checksum mismatch counter. Low load zero, high load 5-10% corruption possible. Analysis: compound instruction vulnerable. Modul 09: Queue/Mutex.

---

## PROJECT (Slide 36-40)

### Slide 36: AquaGuard Scenario
Pak Dimas ikan premium Rp 150jt. Heater manual mati malam suhu 18Â°C â†’ 3 Discus Rp 15jt mati. Sistem auto 24/7 diperlukan. 6 task: Sensor/Heater/Aerator/Feeder/Display/Alarm sensor 3 akuarium temp/pH/Oâ‚‚ monitor.

---

### Slide 37: Feature 1-6
**F1:** 6 task. **F2:** Alarm 4, Sensor 3, Heater 2, Feeder 1. **F3:** DelayUntil(500ms) presisi. **F4:** Suspend. **F5:** Dynamic delete. **F6:** HWM% warning/critical.

---

### Slide 38: Feature 7-12
**F7-F12:** Core affinity, CPU%, watchdog 4sec, corruption demo, dashboard vTaskList, mode compare preemptive/coop/hogging.

---

### Slide 39: Build & Test Sequence
GPIO LED/button ADC. Sensor simulate potensio. Test: 6 task, periodik, boost, maint, dashboard, hang/recovery, mode switch. 12 fitur integrated.

---

### Slide 40: Rubrik & Deliverable
A 90-100, B 75-89, C 60-74, D <60. Deliverable: GitHub source binary README video.

---

## VIDEO (Slide 41-45)

### Slide 41: Video Timeline & Tech
20-35 menit: intro 2min, materi 5min, P01-12 15min, project 6min. MP4 720p webcam PIP. RTOS task state scheduler periodik dual-core API demo.

---

### Slide 42: Percobaan Demo 01-12
P01 create, P02 CPU%, P03 delay drift, P04 suspend, P05 delete heap, P06 stack HWM, P07 affinity/inversion, P08 CPU% load, P09 watchdog, P10 race corruption, P11 dashboard, P12 fairness.

---

### Slide 43: Project AquaGuard Demo
6 task diagram. Code sensor ADC heater alarm. Potentiometer temp rotate naik red alarm, turun stop. Button maintain suspend. Dashboard list heap uptime.

---

### Slide 44: Analysis & Reflection
P03 "DelayUntil presisi periodik"; P10 "race 7% Modul 09 queue/mutex". Project "6 task concurrent watchdog <2sec real-time". Explain "why" tidak hanya "berhasil".

---

### Slide 45: Submission Rules Penalty
Webcam -10%, narasi -15%, P -5%, <15min -10%, >40min -5%, <720p -10%, late -10%/day max 3. Checklist: 20-35min webcam narasi clear P01-12 project 8 fitur. MP4 720p Drive/YouTube 1 minggu.

---

*Modul 08: FreeRTOS Task Management â€” 45 Slide NotebookLM*
*Target per 15 slide: â‰¤4000 char | Rata-rata slide: ~267 char*

