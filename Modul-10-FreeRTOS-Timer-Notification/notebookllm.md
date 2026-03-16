# Modul 10: FreeRTOS Software Timer dan Task Notification

Ringkasan 15-slide komprehensif praktikum FreeRTOS untuk NotebookLM.

---

## SLIDE 1: Software Timer Dasar

Software Timer menjalankan callback setelah periode waktu tanpa task terpisah. Daemon task khusus mengelolanya. Dua jenis: **one-shot** (fire sekali) untuk timeout/debounce, **auto-reload** (berulang) untuk polling/LED. Callback di daemon context bukan ISR. States: Dormant → Running → Expired → kembali. Command queue scheduler. SysTick hardware base clock (default 10ms tick).

---

## SLIDE 2: Timer API dan Daemon Architecture

Daemon otomatis diciptakan saat timer pertama. Config: `configUSE_TIMERS=1`, priority, stack, queue di FreeRTOSConfig.h. Callback TIDAK boleh blocking (delay/wait). Keuntungan: aman ISR, timing predictable. Precision tergantung tick (100Hz). Overhead: daemon stack + queue. Q: Jika timer expire di queue, langsung fire? A: Fire sesuai daemon process urutan.

---

## SLIDE 3: API Timer Create, Control, Query

Create: `xTimerCreate("name", pdMS_TO_TICKS(ms), auto, ID, callback)`. Control: `xTimerStart()`, `xTimerStop()`, `xTimerReset()` (debounce), `xTimerChangePeriod()` (dynamic). ISR: `FromISR()` + `portYIELD_FROM_ISR()`. Query: `xTimerIsTimerActive()`, `pvTimerGetTimerID()`, `xTimerGetPeriod()`. Return: `pdPASS` atau `pdFAIL` (queue full). Callback: `void vCB(TimerHandle_t xTimer)`. Contoh: `xTimerCreate("LED", pdMS_TO_TICKS(500), pdTRUE, NULL, vLed);`.

---

## SLIDE 4: Task Notification Give, Take, Actions

32-bit built-in value setiap task tanpa alokasi. **45% lebih cepat** semaphore. Mengirim: `xTaskNotifyGive()` increment, `xTaskNotify()` full control. Terima: `ulTaskNotifyTake()` wait&read, `xTaskNotifyWait()` advanced. ISR: `vTaskNotifyGiveFromISR()` + `portYIELD_FROM_ISR()`. Actions: eSetBits (OR), eIncrement (++), eSetValueWithOverwrite (replace). One-to-one only. ISR-to-task latency-kritis optimal.

---

## SLIDE 5: Event Group AND/OR, Barrier Sync

32-bit (24 user bits) multi-task sync. Create: `xEventGroupCreate()`. `xEventGroupSetBits(group, bits)` set, `xEventGroupWaitBits(group, mask, clear, waitAll, timeout)` wait. waitAll=pdTRUE (AND all), pdFALSE (OR any). Barrier: `xEventGroupSync(group, own, all, timeout)` rendezvous semua arrive. Use: sensor fusion (AND), alert (OR), task sync.

---

## SLIDE 6: Praktikum STM32 P01-P04 Timer Fundamental

**P01**: Auto 1s one-shot 3s, LED PC13 toggle. Fire: ~30x vs 1x per 30s. **P02**: Button cycle 100→500→1000→2000ms via `xTimerChangePeriod()`. LED smooth adapt. **P03**: Tiga timer satu callback berbeda LED (PC13/PB0/PB1) sesuai ID `pvTimerGetTimerID()`. Ratio 4:2:1 verify. **P04**: Button reset 50ms timer debounce. Raw ~30x bounce, debounced 1. Window 20-50ms optimal mechanical button.

---

## SLIDE 7: Praktikum STM32 P05-P08 Notification Advanced

**P05**: Heartbeat 1s reset 5s timer, timeout → LED warning. Liveness detection cloud. **P06**: Notification vs semaphore latency ISR-to-LED <100μs. Notification 45% faster expected. **P07**: Serial "ON"/"OFF" encode `xTaskNotify(..., eSetValueWithOverwrite)`, rapid 5x last command only. **P08**: Producer faster consumer, backlog accumulate notification value. Log backlog, 32-bit limit.

---

## SLIDE 8: Praktikum STM32 P09-P12 Benchmark Sync

**P09**: 1000x alternate 500 notification vs 500 semaphore, GPIO timestamp latency measure. Expected notification faster. **P10**: Tiga sensor set bit (0/1/2), collector AND wait ready fusion. **P11**: Barrier rendezvous 3 task berbeda durasi, max arrival. **P12**: 1000x timer/notification/semaphore min/max/avg/stddev. Conclusion: notification overhead rendah (direct), semaphore sedang, timer sedang (daemon queue).

---

## SLIDE 9: Praktikum ESP32 P01-P04 Timer Precision

**P01**: Identik STM32, `esp_timer_get_time()` microsecond precision vs 10ms STM32. **P02**: Serial 'f'/'m'/'s'/'x' cycle period real-time `xTimerChangePeriod()`. **P03**: Tiga timer satu callback shared ID GPIO2/4/5, ratio 4:2:1. **P04**: GPIO0 reset 50ms debounce. ESP32 GPIO 250ns (vs STM32 1-2μs faster).

---

## SLIDE 10: Praktikum ESP32 P05-P08 Notification Backlog

**P05**: Notify 500ms reset 2s timer, freeze → timeout alert GPIO5 blink. **P06**: Interrupt → toggle task measure `esp_timer_get_time()` 100x latency. 1MHz presisi variance minimal. Dual-core pin same core. **P07**: Parse JSON `{"cmd":"led","pin":2,"state":1}` encode send `xTaskNotify(..., eSetValueWithOverwrite)`. Last command only. **P08**: Two producer `xTaskNotifyGive()` to consumer `ulTaskNotifyTake(pdFALSE)` decrement delay. Backlog accumulate.

---

## SLIDE 11: Praktikum ESP32 P09-P12 Project Overview

**P09**: 1000x comprehensive timer/notification/semaphore test CSV. **P10-P11**: P10 sensor ADC set bits, AND collector. P11 barrier cross-core IPC. **P12**: 1000x loop, CSV/SPIFFS export, Python histogram. Conclusion notification fastest, timer predictable, semaphore balanced. **PROJECT**: Mesin Cuci Otomatis FreeRTOS. Hardware: STM32/ESP32, LED (PC13/GPIO2), button start/pause/reset (PA0/PA1). States: IDLE→WASH 5s→RINSE 3s→SPIN 4s→FINISH 12s. ISR notify → timer state → LED pattern + serial.

---

## SLIDE 12: Project Implementation Testing Scenario

**STM32 Implementation**: FreeRTOSConfig.h enable, controller HIGH(5) + display MEDIUM(3), ISR → `xTaskNotifyGiveFromISR()`. Controller: `xTaskNotifyWait()` state, `xTimerChangePeriod()` durasi. Display: `vTaskDelay(1000)` state, LED toggle, serial. **ESP32**: `vTaskCreatePinnedToCore()` core1/core0. **Testing**: (1) 12s cycle START→IDLE sync, (2) Pause mid-state sisa duration preserved resume, (3) RESET instant IDLE <100ms. Repeat 5x nonstop, zero crash, LED+serial perfect sync.

---

## SLIDE 13: Video Tugas Struktur Konten

**Duration 20-35 min** (target 25m). **Opening (2m)**: Nama/NIM/kelas platform. "Modul 10 FreeRTOS Timer Notification foundation event-driven — timer scheduling, notification ISR latency minimal, event group multi-task sync elegant." **Teori (3-4m)**: Timer (one-shot/auto-reload + callback) alarm analogi, Notification (32-bit + 45% faster) bell, Event Group (AND/OR + barrier). API list + code 2-3 lines. **P01-P12 Demo (12-15m)**: 1-1.5m exp: code highlight → run → observe 5-10s → quick analysis "mengapa". Efficient pacing.

---

## SLIDE 14: Video Demonstrasi Project Showcase

**P01-P06**: Callback accuracy (30Hz vs expect), period smooth, fire ratio 4:2:1, latency/debounce/timeout analyze. **P07-P12**: Overwrite behavior, backlog, benchmark 1000x histogram, AND/barrier, comprehensive mechanism. **Project (5-7m)**: Hardware setup, state diagram. Scenario: START→WASH blink→RINSE fast→SPIN→IDLE 12s. Pause mid-WASH sisa preserved resume. RESET instant. Serial sync visible. Integration teori-praktik analisa.

---

## SLIDE 15: Video Penutup Submission Checklist

**Penutup (2-3m)**: Recap selection: Timer periodic, Notification ISR-critical, Event Group barrier. "Event-driven reduce CPU/power drastically IoT battery critical." Project integrase state machine + timer + notification + concurrency. Challenge (timer not fire → check config, queue full → increase, latency → priority). Reflection: "Hands-on hardware essential master. 60% production pakai FreeRTOS. Career foundation intensive practice." **Submit**: MP4 720p, webcam PIP 15%, narasi jelas, 1 minggu deadline, "Nama_NIM_Modul10_Video.mp4", YouTube/GDrive. Penalti: <15m −10%, >40m −5%, late −10%/hari. **Checklist**: Duration, webcam, narasi, opening, teori+P01-P12+project+closing, MP4 720p audio sync, link test, submit deadline. ✓ Good luck!
