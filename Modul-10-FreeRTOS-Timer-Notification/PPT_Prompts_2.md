# PPT Prompts Modul 11 - Part 2: Task Notification (Slides 16-32)

## Slide 16: Task Notification Concept
```
Buatkan slide konsep task notification:

"Task Notification - Konsep Dasar"

Definisi:
"Mekanisme komunikasi point-to-point yang sangat efisien,
built-in di setiap task FreeRTOS"

Diagram:
┌─────────────┐              ┌─────────────┐
│   Task A    │              │   Task B    │
│             │              │             │
│  xTaskNotify├─────────────→│ Notification│
│     Give    │              │    Value    │
│             │              │  [32 bits]  │
└─────────────┘              └──────┬──────┘
                                    │
                             xTaskNotifyWait
                                    │
                                    ▼
                              Task B wakes!

Key Points:
- Setiap task punya notification value (32-bit)
- Tidak perlu create/alokasi terpisah
- Lebih cepat dari semaphore
```

## Slide 17: Why Task Notification?
```
Buatkan slide keunggulan task notification:

"Kenapa Task Notification?"

Performance Comparison:
┌─────────────────────────────────────────────┐
│                                             │
│  Binary Semaphore: ████████████ 100%        │
│  Task Notification: █████████ 55% (45% faster)│
│                                             │
└─────────────────────────────────────────────┘

Memory Usage:
┌─────────────────────────────────────────────┐
│  Binary Semaphore: +76 bytes per semaphore  │
│  Task Notification: 0 bytes (built-in!)     │
└─────────────────────────────────────────────┘

Best for:
✅ ISR to task signaling
✅ Event notification  
✅ Lightweight counting
✅ Task synchronization
```

## Slide 18: Notification vs Semaphore Comparison
```
Buatkan slide perbandingan tabel:

"Task Notification vs Binary Semaphore"

| Feature           | Semaphore | Notification |
|-------------------|-----------|--------------|
| RAM Usage         | +76 bytes | 0 (built-in) |
| Speed             | Normal    | 45% faster   |
| Many-to-one       | ✅        | ✅           |
| One-to-many       | ✅        | ❌           |
| Broadcast         | ✅        | ❌           |
| Data transfer     | ❌        | ✅ (32-bit)  |
| Create needed     | ✅        | ❌           |

Decision Flow:
┌──────────────────────────────────────┐
│ Multiple receivers needed?           │
│   YES → Use Semaphore               │
│   NO → Does sender need result?     │
│         NO → Use Task Notification  │
│         YES → Use Queue             │
└──────────────────────────────────────┘
```

## Slide 19: Notification Actions
```
Buatkan slide notification actions:

"Notification Actions (eNotifyAction)"

┌─────────────────────────────────────────────┐
│ typedef enum {                              │
│     eNoAction = 0,     // Just wake task    │
│     eSetBits,          // value |= new      │
│     eIncrement,        // value++           │
│     eSetValueWithOverwrite,    // value = new│
│     eSetValueWithoutOverwrite  // if clear   │
│ } eNotifyAction;                            │
└─────────────────────────────────────────────┘

Visual untuk setiap action:
eNoAction:     [Old] → [Old] + wake
eSetBits:      [0x01] | [0x02] → [0x03]
eIncrement:    [5] + 1 → [6]
eSetValueWithOverwrite: [xxx] → [new]
eSetValueWithoutOverwrite: [clear] → [new]
```

## Slide 20: Sending Notification - Simple
```
Buatkan slide simple give:

"Mengirim Notification - Simple Give"

API:
┌─────────────────────────────────────────────┐
│ BaseType_t xTaskNotifyGive(                 │
│     TaskHandle_t xTaskToNotify              │
│ );                                          │
└─────────────────────────────────────────────┘

Behavior:
- Increment notification value
- Wake target task
- Seperti semaphore give

Example:
┌─────────────────────────────────────────────┐
│ // Producer task                            │
│ xTaskNotifyGive(xConsumerTask);             │
│                                             │
│ // Notification value: 0 → 1 → 2 → 3...     │
└─────────────────────────────────────────────┘

Use case: Counting events, simple signaling
```

## Slide 21: Sending Notification - Full Control
```
Buatkan slide full notification:

"Mengirim Notification - Full Control"

API:
┌─────────────────────────────────────────────┐
│ BaseType_t xTaskNotify(                     │
│     TaskHandle_t xTaskToNotify,             │
│     uint32_t ulValue,                       │
│     eNotifyAction eAction                   │
│ );                                          │
└─────────────────────────────────────────────┘

Examples:

Set Event Bits:
xTaskNotify(xTask, (1 << EVENT_A), eSetBits);

Send Value:
xTaskNotify(xTask, sensorData, eSetValueWithOverwrite);

Just Wake:
xTaskNotify(xTask, 0, eNoAction);

Increment (like give):
xTaskNotify(xTask, 0, eIncrement);
```

## Slide 22: Receiving Notification - Simple Take
```
Buatkan slide simple take:

"Menerima Notification - Simple Take"

API:
┌─────────────────────────────────────────────┐
│ uint32_t ulTaskNotifyTake(                  │
│     BaseType_t xClearCountOnExit,           │
│     TickType_t xTicksToWait                 │
│ );                                          │
└─────────────────────────────────────────────┘

Parameter xClearCountOnExit:
┌─────────────────────────────────────────────┐
│ pdTRUE  → Clear to 0 on exit (binary style) │
│ pdFALSE → Decrement on exit (counting style)│
└─────────────────────────────────────────────┘

Example:
// Binary semaphore style
count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

// Counting semaphore style  
count = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

Return: Notification value before clear/decrement
```

## Slide 23: Receiving Notification - Full Control
```
Buatkan slide full wait:

"Menerima Notification - Full Control"

API:
┌─────────────────────────────────────────────┐
│ BaseType_t xTaskNotifyWait(                 │
│     uint32_t ulBitsToClearOnEntry,          │
│     uint32_t ulBitsToClearOnExit,           │
│     uint32_t *pulNotificationValue,         │
│     TickType_t xTicksToWait                 │
│ );                                          │
└─────────────────────────────────────────────┘

Parameter Visualization:
Entry: notification &= ~ulBitsToClearOnEntry
Exit:  notification &= ~ulBitsToClearOnExit

Example - Event Bits Style:
xTaskNotifyWait(
    0x00,           // Don't clear on entry
    0xFFFFFFFF,     // Clear all on exit
    &events,        // Receive value
    portMAX_DELAY
);
```

## Slide 24: Notification from ISR
```
Buatkan slide notification dari ISR:

"Notification dari ISR"

ISR-Safe APIs:
┌─────────────────────────────────────────────┐
│ vTaskNotifyGiveFromISR(xTask, &xWoken);     │
│                                             │
│ xTaskNotifyFromISR(xTask, value, action,    │
│                    &xWoken);                │
└─────────────────────────────────────────────┘

Complete ISR Example:
┌─────────────────────────────────────────────┐
│ void EXTI0_IRQHandler(void)                 │
│ {                                           │
│     BaseType_t xWoken = pdFALSE;            │
│                                             │
│     // Simple give                          │
│     vTaskNotifyGiveFromISR(xButtonTask,     │
│                            &xWoken);        │
│                                             │
│     portYIELD_FROM_ISR(xWoken);             │
│                                             │
│     HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);   │
│ }                                           │
└─────────────────────────────────────────────┘
```

## Slide 25: Pattern - Binary Semaphore Style
```
Buatkan slide binary semaphore pattern:

"Pattern: Task Notification sebagai Binary Semaphore"

Diagram:
┌─────────────────────────────────────────────┐
│                                             │
│   ISR/Task                   Handler Task   │
│      │                           │          │
│      │   vTaskNotifyGive         │          │
│      ├──────────────────────────>│          │
│      │                           │ wake     │
│      │                           ▼          │
│      │                    ulTaskNotifyTake  │
│      │                    (pdTRUE, MAX)     │
│      │                           │          │
│      │                     Process event    │
│                                             │
└─────────────────────────────────────────────┘

Code:
// Receiver
for(;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    handleEvent();
}
```

## Slide 26: Pattern - Counting Semaphore Style
```
Buatkan slide counting semaphore pattern:

"Pattern: Task Notification sebagai Counting Semaphore"

Scenario: Multiple producers, one consumer

Diagram:
┌─────────────────────────────────────────────┐
│   Producer 1 ──┐                            │
│                ├──> Notification ──> Consumer│
│   Producer 2 ──┘      [count]               │
│                                             │
│   count: 0 → 1 → 2 → 1 → 0                 │
│              ↑   ↑   ↓   ↓                 │
│            give give take take             │
└─────────────────────────────────────────────┘

Consumer Code:
// Decrement instead of clear
for(;;) {
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    processOneItem();
}
```

## Slide 27: Pattern - Event Bits
```
Buatkan slide event bits pattern:

"Pattern: Task Notification sebagai Event Flags"

Event Bit Definitions:
┌─────────────────────────────────────────────┐
│ #define EVT_BUTTON    (1 << 0)  // bit 0    │
│ #define EVT_TIMER     (1 << 1)  // bit 1    │
│ #define EVT_UART      (1 << 2)  // bit 2    │
│ #define EVT_SENSOR    (1 << 3)  // bit 3    │
└─────────────────────────────────────────────┘

32-bit value visualization:
┌─────────────────────────────────────────────┐
│ Bit: 31 30 29 ... 3  2  1  0               │
│       0  0  0 ... 1  0  1  1 = 0x0B        │
│                   │     │  │               │
│                   │     │  └─ EVT_BUTTON   │
│                   │     └──── EVT_TIMER    │
│                   └────────── EVT_SENSOR   │
└─────────────────────────────────────────────┘

Multiple events in single notification!
```

## Slide 28: Event Bits Implementation
```
Buatkan slide implementasi event bits:

"Implementasi Event Bits"

Sender (multiple sources):
┌─────────────────────────────────────────────┐
│ // Button ISR                               │
│ xTaskNotifyFromISR(xHandler, EVT_BUTTON,    │
│                    eSetBits, &woken);       │
│                                             │
│ // Timer callback                           │
│ xTaskNotify(xHandler, EVT_TIMER, eSetBits); │
│                                             │
│ // UART task                                │
│ xTaskNotify(xHandler, EVT_UART, eSetBits);  │
└─────────────────────────────────────────────┘

Receiver:
┌─────────────────────────────────────────────┐
│ uint32_t events;                            │
│ xTaskNotifyWait(0, 0xFFFFFFFF, &events,     │
│                 portMAX_DELAY);             │
│                                             │
│ if(events & EVT_BUTTON) handleButton();     │
│ if(events & EVT_TIMER)  handleTimer();      │
│ if(events & EVT_UART)   handleUart();       │
└─────────────────────────────────────────────┘
```

## Slide 29: Timer + Notification Combo
```
Buatkan slide kombinasi timer dan notification:

"Kombinasi: Timer + Notification"

Architecture:
┌─────────────────────────────────────────────┐
│                                             │
│  Button ISR ──> Debounce Timer              │
│                      │                      │
│                      ▼                      │
│                Timer Callback               │
│                      │                      │
│                xTaskNotify ───> Handler     │
│                                   Task      │
│  Periodic Timer ─────────────────>│         │
│                                   │         │
│  Serial Input ───────────────────>│         │
│                                             │
└─────────────────────────────────────────────┘

Benefits:
- Debounced input via timer
- Central event handler
- Clean separation of concerns
```

## Slide 30: Complete Example - Event System
```
Buatkan slide contoh lengkap:

"Contoh Lengkap: Event-Driven System"

Code Flow:
┌─────────────────────────────────────────────┐
│ // Event handler task                       │
│ void EventHandler(void *p) {                │
│     uint32_t events;                        │
│                                             │
│     for(;;) {                               │
│         xTaskNotifyWait(0, 0xFFFF, &events, │
│                         portMAX_DELAY);     │
│                                             │
│         if(events & EVT_BTN) {              │
│             LED_Toggle();                   │
│         }                                   │
│         if(events & EVT_TIMER) {            │
│             SendHeartbeat();                │
│         }                                   │
│     }                                       │
│ }                                           │
└─────────────────────────────────────────────┘

Demo: Multiple input sources → Single handler
```

## Slide 31: Best Practices
```
Buatkan slide best practices:

"Best Practices"

Software Timer:
┌─────────────────────────────────────────────┐
│ ✅ DO                 │ ❌ DON'T            │
│─────────────────────────────────────────────│
│ Callback singkat      │ vTaskDelay          │
│ Non-blocking API      │ Blocking wait       │
│ Set flag/notify       │ Heavy computation   │
│ One-shot untuk timeout│ Loop panjang        │
└─────────────────────────────────────────────┘

Task Notification:
┌─────────────────────────────────────────────┐
│ ✅ DO                 │ ❌ DON'T            │
│─────────────────────────────────────────────│
│ Point-to-point comm   │ Broadcast           │
│ Store handle properly │ NULL handle         │
│ Clear bits correctly  │ Multiple receivers  │
│ FromISR + yield       │ Forget yield        │
└─────────────────────────────────────────────┘
```

## Slide 32: Summary & Review
```
Buatkan slide ringkasan:

"Ringkasan Modul 11"

Software Timer:
📍 Callback tanpa dedicated task
📍 One-shot (timeout) vs Auto-reload (periodic)
📍 Daemon task mengelola semua timer
📍 Callback HARUS non-blocking

Task Notification:
📍 Built-in 32-bit value per task
📍 45% lebih cepat dari semaphore
📍 Bisa sebagai: binary sem, counting sem, event flags
📍 Point-to-point only

Kombinasi:
┌─────────────────────────────────────────────┐
│ ISR → Timer (debounce) → Notification →     │
│                          Handler Task       │
└─────────────────────────────────────────────┘

Next: FreeRTOS Memory Management & Advanced Topics
```

---

## Catatan untuk Pembuat PPT

### Visual Guidelines
- Warna orange untuk notification-related content
- Warna biru untuk timer-related content
- Animasi flow untuk event bits patterns
- Comparison tables dengan highlighting

### Key Diagrams
1. Notification flow (Slide 16)
2. Performance comparison bar chart (Slide 17)
3. Event bits visualization (Slide 27)
4. Timer + Notification architecture (Slide 29)

### Interactive Elements
- Quiz: When to use timer vs notification?
- Hands-on: Implement event handler
- Code walkthrough animasi

### Demo Suggestions
1. Button debounce dengan timer reset
2. Event flags dari multiple sources
3. Watchdog timeout detection
