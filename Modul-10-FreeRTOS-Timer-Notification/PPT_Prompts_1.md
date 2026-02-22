# PPT Prompts Modul 11 - Part 1: Software Timer dan Task Notification (Slides 1-15)

## Slide 1: Cover
```
Buatkan slide cover dengan:
- Judul: "FreeRTOS Software Timer dan Task Notification"
- Subtitle: "Modul 11 - Praktikum Sistem Embedded"
- Visual: Ilustrasi timer/clock dengan signal/notification waves
- Logo institusi
- Warna tema: Biru profesional dengan aksen orange
```

## Slide 2: Agenda
```
Buatkan slide agenda dengan layout dua kolom:

Kolom Kiri - Software Timer:
1. Konsep Software Timer
2. Timer Daemon Task
3. One-Shot vs Auto-Reload
4. Timer API Functions
5. Timer Patterns

Kolom Kanan - Task Notification:
6. Konsep Task Notification
7. Notification vs Semaphore
8. Notification API
9. Event Bits Pattern
10. Kombinasi Timer + Notification

Visual: Timeline atau flowchart menghubungkan topik
```

## Slide 3: Learning Objectives
```
Buatkan slide tujuan pembelajaran dengan icons:

🎯 Setelah modul ini, mahasiswa mampu:

1. [Timer Icon] Membuat dan mengontrol software timer
2. [Callback Icon] Mengimplementasikan timer callback non-blocking  
3. [Bell Icon] Menggunakan task notification untuk signaling
4. [Event Icon] Menerapkan notification sebagai event flags
5. [Integration Icon] Mengkombinasikan timer dan notification

Level: ⭐⭐⭐⭐ (Intermediate-Advanced)
```

## Slide 4: Kenapa Software Timer?
```
Buatkan slide dengan perbandingan visual:

"Kenapa Butuh Software Timer?"

Masalah TANPA Timer:
❌ Perlu task dedicated untuk timing
❌ Setiap timing butuh stack memory
❌ vTaskDelay blocking task

Solusi DENGAN Timer:
✅ Callback function tanpa task
✅ Satu daemon handle semua timer  
✅ Non-blocking, event-driven

Diagram: Multiple timing tasks vs Single daemon dengan multiple timers
Memory comparison bar chart
```

## Slide 5: Software Timer Concept
```
Buatkan slide konsep software timer:

"Software Timer - Konsep Dasar"

Definisi:
"Mekanisme untuk menjalankan fungsi (callback) setelah 
periode waktu tertentu, tanpa memerlukan task terpisah"

Diagram animasi:
┌─────────────────────────────────┐
│     TIMER SERVICE TASK          │
│     (Timer Daemon)              │
├─────────────────────────────────┤
│  Timer 1 → Callback 1 (100ms)   │
│  Timer 2 → Callback 2 (500ms)   │
│  Timer 3 → Callback 3 (1s)      │
└─────────────────────────────────┘

Key Points:
- Dikelola oleh daemon task (built-in)
- Callback berjalan di context daemon
- Tidak perlu stack terpisah per timer
```

## Slide 6: Timer Daemon Task
```
Buatkan slide timer daemon:

"Timer Daemon Task"

Apa itu Timer Daemon?
- Background task yang mengelola semua software timer
- Dibuat otomatis oleh FreeRTOS
- Menggunakan timer command queue

Konfigurasi (FreeRTOSConfig.h):
┌─────────────────────────────────────┐
│ #define configUSE_TIMERS        1   │
│ #define configTIMER_TASK_PRIORITY  2│
│ #define configTIMER_QUEUE_LENGTH  10│
│ #define configTIMER_TASK_STACK_DEPTH 256│
└─────────────────────────────────────┘

Diagram: Timer Command Queue → Daemon Task → Callbacks
```

## Slide 7: Two Types of Timers
```
Buatkan slide dua jenis timer dengan visual:

"One-Shot vs Auto-Reload Timer"

┌─────────────────────────────────────┐
│          ONE-SHOT TIMER             │
│                                     │
│  Start──[Period]──>Callback──>STOP  │
│                                     │
│  Use cases:                         │
│  • Timeout handler                  │
│  • Debounce                         │
│  • Delayed action                   │
└─────────────────────────────────────┘

┌─────────────────────────────────────┐
│       AUTO-RELOAD TIMER             │
│                                     │
│  Start──[Period]──>Callback──┐      │
│    ^                         │      │
│    └─────────────────────────┘      │
│                                     │
│  Use cases:                         │
│  • LED blink                        │
│  • Sensor polling                   │
│  • Heartbeat/watchdog               │
└─────────────────────────────────────┘
```

## Slide 8: Timer States
```
Buatkan slide state diagram timer:

"Timer States"

State Diagram:
                ┌─────────┐
  xTimerCreate()│ DORMANT │
       ┌───────>│(Created)│<──────────┐
       │        └────┬────┘           │
       │             │                │
       │       xTimerStart()     xTimerStop()
       │             │                │
       │             ▼                │
       │        ┌─────────┐           │
       │        │ RUNNING │───────────┘
       │        └────┬────┘
       │             │
       │         Expired
       │             │
       │             ▼
       │        ┌─────────┐
       └────────│CALLBACK │
                │ Execute │
                └─────────┘
                    │
    One-shot: → Dormant
    Auto-reload: → Running
```

## Slide 9: xTimerCreate API
```
Buatkan slide API xTimerCreate:

"Membuat Timer: xTimerCreate()"

Syntax:
┌─────────────────────────────────────────────┐
│ TimerHandle_t xTimerCreate(                 │
│     const char *pcTimerName,    // Nama     │
│     TickType_t xPeriod,         // Periode  │
│     UBaseType_t uxAutoReload,   // Jenis    │
│     void *pvTimerID,            // ID       │
│     TimerCallbackFunction_t pxCallback      │
│ );                                          │
└─────────────────────────────────────────────┘

Parameter Visual:
• pcTimerName → "LED_Timer" (untuk debug)
• xPeriod → pdMS_TO_TICKS(500) 
• uxAutoReload → pdTRUE (periodic) / pdFALSE (one-shot)
• pvTimerID → (void*)0 (optional identifier)
• pxCallback → vMyTimerCallback

Return: Handle atau NULL jika gagal
```

## Slide 10: Timer Callback Function
```
Buatkan slide timer callback:

"Timer Callback Function"

Syntax:
┌─────────────────────────────────────────────┐
│ void vTimerCallback(TimerHandle_t xTimer)   │
│ {                                           │
│     // Aksi singkat dan cepat               │
│     HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);  │
│ }                                           │
└─────────────────────────────────────────────┘

⚠️ PENTING - Callback TIDAK BOLEH:
┌─────────────────────────────────────────────┐
│ ❌ vTaskDelay()     // Blocking!            │
│ ❌ xQueueReceive()  // dengan wait          │
│ ❌ Heavy computation                        │
│ ❌ Loop yang lama                           │
└─────────────────────────────────────────────┘

BOLEH:
┌─────────────────────────────────────────────┐
│ ✅ Toggle GPIO                              │
│ ✅ Set global flag                          │
│ ✅ Send to queue (no wait)                  │
│ ✅ Give notification                        │
└─────────────────────────────────────────────┘
```

## Slide 11: Timer Control APIs
```
Buatkan slide timer control:

"Mengontrol Timer"

Start Timer:
xTimerStart(xTimer, pdMS_TO_TICKS(100));

Stop Timer:
xTimerStop(xTimer, pdMS_TO_TICKS(100));

Reset Timer:
xTimerReset(xTimer, pdMS_TO_TICKS(100));
→ Restart periode dari awal

Change Period:
xTimerChangePeriod(xTimer, pdMS_TO_TICKS(200), 
                   pdMS_TO_TICKS(100));

Delete Timer:
xTimerDelete(xTimer, pdMS_TO_TICKS(100));

Note: Parameter kedua adalah timeout untuk command queue
```

## Slide 12: Timer API from ISR
```
Buatkan slide timer ISR API:

"Timer API dari ISR"

ISR-Safe Versions:
┌─────────────────────────────────────────────┐
│ xTimerStartFromISR(xTimer, &xWoken);        │
│ xTimerStopFromISR(xTimer, &xWoken);         │
│ xTimerResetFromISR(xTimer, &xWoken);        │
└─────────────────────────────────────────────┘

Contoh: Debounce dari ISR
┌─────────────────────────────────────────────┐
│ void EXTI0_IRQHandler(void)                 │
│ {                                           │
│     BaseType_t xWoken = pdFALSE;            │
│                                             │
│     // Reset debounce timer setiap edge     │
│     xTimerResetFromISR(xDebounce, &xWoken); │
│                                             │
│     portYIELD_FROM_ISR(xWoken);             │
│ }                                           │
└─────────────────────────────────────────────┘
```

## Slide 13: Timer Query Functions
```
Buatkan slide timer query:

"Query Timer State"

Check if Active:
┌─────────────────────────────────────────────┐
│ if(xTimerIsTimerActive(xTimer)) {           │
│     printf("Timer is running\n");           │
│ }                                           │
└─────────────────────────────────────────────┘

Get Timer Name:
const char *name = pcTimerGetName(xTimer);

Get/Set Timer ID:
void *id = pvTimerGetTimerID(xTimer);
vTimerSetTimerID(xTimer, newId);

Get Period:
TickType_t period = xTimerGetPeriod(xTimer);

Get Expiry Time:
TickType_t expiry = xTimerGetExpiryTime(xTimer);

Use case: Monitoring dan debugging
```

## Slide 14: Timer Pattern - Debounce
```
Buatkan slide debounce pattern:

"Pattern: Debounce Timer"

Problem: Button noise menghasilkan multiple interrupts

Solution: Timer debounce
┌─────────────────────────────────────────────┐
│ ISR                    Timer Callback       │
│  │                         │                │
│  │  Button press           │                │
│  ├────> Start timer ──────>│                │
│  │                         │                │
│  │  Noise                  │                │
│  ├────> Reset timer ──────>│                │
│  │                         │                │
│  │  Noise                  │                │
│  ├────> Reset timer ──────>│                │
│  │                         │                │
│  │                    [50ms expired]        │
│  │                         │                │
│  │                    Confirmed press!      │
└─────────────────────────────────────────────┘

Result: Only 1 confirmed press from multiple ISR triggers
```

## Slide 15: Timer Pattern - Timeout/Watchdog
```
Buatkan slide timeout pattern:

"Pattern: Timeout/Watchdog"

Use case: Detect communication loss

Flow:
┌─────────────────────────────────────────────┐
│                                             │
│  Data Received ──> Reset Timer              │
│       │                │                    │
│       │                │                    │
│       ▼                ▼                    │
│  Process Data      [3s countdown]           │
│                        │                    │
│                        ▼                    │
│                   TIMEOUT!                  │
│                   Handle error              │
│                                             │
└─────────────────────────────────────────────┘

Code:
// Reset setiap terima data
xTimerReset(xWatchdog, 0);

// Callback saat timeout
void vWatchdogCallback() {
    printf("Connection lost!\n");
}
```

---

## Catatan untuk Pembuat PPT

### Konsistensi Visual
- Gunakan ikon timer/clock untuk software timer
- Gunakan ikon bell/notification untuk task notification
- Warna biru untuk timer, orange untuk notification
- Animasi bertahap untuk diagram kompleks

### Diagram Priorities
1. Timer daemon architecture (Slide 5)
2. State diagram (Slide 8)
3. Debounce timing diagram (Slide 14)
4. Watchdog flow (Slide 15)

### Code Highlighting
- Syntax highlighting untuk semua code blocks
- Highlight fungsi-fungsi penting dengan warna berbeda
- Box warning untuk hal yang tidak boleh dilakukan

### Tips Presentasi
- Demo langsung timer blink LED
- Tunjukkan serial output dengan timing
- Bandingkan kode dengan dan tanpa timer
