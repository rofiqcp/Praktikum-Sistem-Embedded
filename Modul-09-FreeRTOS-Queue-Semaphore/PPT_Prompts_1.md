# PPT Prompts Modul 10 - FreeRTOS Queue dan Semaphore (Bagian 1)

## Slide 1: Cover
```
Judul: "FreeRTOS Queue dan Semaphore"
Subtitle: "Inter-Task Communication & Synchronization"
Gambar: Diagram queue dengan multiple producer-consumer
Elemen:
- Logo institusi
- Logo FreeRTOS
- Ikon STM32 dan ESP32
- Modul 10 - Praktikum Sistem Embedded
Style: Modern, warna biru-hijau gradient
```

---

## Slide 2: Tujuan Pembelajaran
```
Judul: "Apa yang Akan Anda Pelajari?"
Konten (5 poin dengan ikon):
🔹 Memahami konsep Queue untuk transfer data antar task
🔹 Menggunakan Binary dan Counting Semaphore
🔹 Menerapkan Mutex untuk proteksi resource
🔹 Menghindari race condition dan deadlock
🔹 Implementasi pada STM32 dan ESP32

Visual: Flowchart sederhana Producer→Queue→Consumer
Catatan: "Dasar komunikasi dan sinkronisasi real-time"
```

---

## Slide 3: Review Modul Sebelumnya
```
Judul: "Review: FreeRTOS Task Management"
Konten dalam box:
✅ Task creation dan state machine
✅ Priority dan preemptive scheduling
✅ Task parameters dan notifications
✅ Stack dan heap management

Pertanyaan: "Bagaimana task berkomunikasi satu sama lain?"
Visual: Diagram 3 task terisolasi dengan tanda tanya
Transisi: "Jawabannya: Queue dan Semaphore!"
```

---

## Slide 4: Masalah Komunikasi Task
```
Judul: "Masalah: Task Perlu Berbagi Data"
Konten (3 skenario):
1. ❌ Global variable → Race condition
2. ❌ Polling shared memory → Inefficient
3. ❌ Direct function call → Blocking

Code snippet yang SALAH:
volatile int shared_data;  // BERBAHAYA!
// Task A menulis, Task B membaca = unpredictable!

Visual: Animasi 2 task menulis ke variable yang sama
Highlight: "Kita butuh mekanisme SAFE untuk sharing!"
```

---

## Slide 5: Solusi FreeRTOS
```
Judul: "Solusi: Primitif Sinkronisasi FreeRTOS"
Diagram 3 kolom:

QUEUE                 SEMAPHORE              MUTEX
├─ Transfer data     ├─ Binary: Signal      ├─ Protect resource
├─ FIFO buffer       ├─ Counting: Pool      ├─ Ownership
└─ Thread-safe       └─ ISR compatible      └─ Priority inherit

Visual: Ikon untuk masing-masing (mailbox, traffic light, lock)
Caption: "Pilih yang tepat sesuai kebutuhan!"
```

---

## Slide 6: Apa itu Queue?
```
Judul: "Queue: FIFO Message Passing"
Definisi box:
"Queue adalah buffer FIFO (First-In-First-Out) yang 
memungkinkan transfer data thread-safe antar task"

Visual diagram FIFO:
[Producer] → |D1|D2|D3|D4|D5| → [Consumer]
              ↑              ↑
             Send          Receive

Karakteristik (4 poin):
✓ Copy data (bukan pointer)
✓ Blocking dengan timeout
✓ Multiple senders/receivers
✓ ISR-safe (versi FromISR)
```

---

## Slide 7: Membuat Queue
```
Judul: "xQueueCreate() - Membuat Queue"
Syntax box:
QueueHandle_t xQueueCreate(
    UBaseType_t uxQueueLength,    // Jumlah item
    UBaseType_t uxItemSize        // Ukuran tiap item
);

Contoh:
// Queue untuk 10 item integer (4 bytes)
QueueHandle_t xQueue;
xQueue = xQueueCreate(10, sizeof(int32_t));

// Queue untuk 5 struct SensorData
xQueue = xQueueCreate(5, sizeof(SensorData_t));

Memory diagram:
[Queue Header] + [10 x 4 bytes] = dari Heap

Warning: "Selalu cek return value != NULL!"
```

---

## Slide 8: Mengirim ke Queue
```
Judul: "xQueueSend() - Mengirim Data"
Syntax box:
BaseType_t xQueueSend(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    TickType_t xTicksToWait
);

Contoh lengkap:
int32_t data = 42;
BaseType_t result;

// Kirim dengan timeout 100ms
result = xQueueSend(xQueue, &data, pdMS_TO_TICKS(100));

if(result == pdPASS) {
    // Berhasil!
} else {
    // Queue full, timeout!
}

Visual: Animasi data masuk ke queue
Variasi: xQueueSendToBack(), xQueueSendToFront()
```

---

## Slide 9: Menerima dari Queue
```
Judul: "xQueueReceive() - Menerima Data"
Syntax box:
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void *pvBuffer,
    TickType_t xTicksToWait
);

Contoh:
int32_t receivedData;

// Block selamanya sampai ada data
if(xQueueReceive(xQueue, &receivedData, portMAX_DELAY) == pdPASS) {
    printf("Received: %d\n", receivedData);
}

Timeout options table:
| Value | Behavior |
|-------|----------|
| 0 | Non-blocking, return immediately |
| pdMS_TO_TICKS(100) | Wait max 100ms |
| portMAX_DELAY | Block forever |

Visual: Animasi data keluar dari queue
```

---

## Slide 10: Queue Status Functions
```
Judul: "Fungsi Pemantau Queue"
3 box fungsi:

1. uxQueueMessagesWaiting(xQueue)
   → Jumlah item dalam queue
   
2. uxQueueSpacesAvailable(xQueue)
   → Slot kosong yang tersedia
   
3. xQueuePeek(xQueue, &data, timeout)
   → Baca tanpa menghapus

Contoh monitoring:
printf("Items: %lu\n", uxQueueMessagesWaiting(xQueue));
printf("Space: %lu\n", uxQueueSpacesAvailable(xQueue));

Visual: Diagram queue dengan indikator count
Use case: "Berguna untuk debugging dan adaptive behavior"
```

---

## Slide 11: Queue dari ISR
```
Judul: "xQueueSendFromISR() - Thread-Safe dari ISR"
Perbedaan dengan versi normal:
❌ xQueueSend() - JANGAN dari ISR
✅ xQueueSendFromISR() - AMAN dari ISR

Syntax:
BaseType_t xQueueSendFromISR(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    BaseType_t *pxHigherPriorityTaskWoken
);

Contoh di ISR:
void EXTI0_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int32_t data = sensor_read();
    
    xQueueSendFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

Warning: "Selalu gunakan portYIELD_FROM_ISR!"
```

---

## Slide 12: Contoh Praktis - Multi Sensor Queue
```
Judul: "Studi Kasus: Multi-Sensor Data Collection"
Arsitektur diagram:
[Temp Sensor] ─┐
               ├─→ [Queue 10 items] ─→ [Display Task]
[Light Sensor]─┘

Struct data:
typedef struct {
    uint8_t  sensorId;
    float    value;
    uint32_t timestamp;
} SensorReading_t;

xQueue = xQueueCreate(10, sizeof(SensorReading_t));

Visual: Diagram alur data dengan warna berbeda per sensor
Keuntungan:
✓ Decoupling producer & consumer
✓ Buffering jika consumer lambat
✓ Type-safe data transfer
```

---

## Slide 13: Apa itu Semaphore?
```
Judul: "Semaphore: Sinkronisasi & Signaling"
Definisi box:
"Semaphore adalah mekanisme sinyal non-data yang digunakan 
untuk sinkronisasi antar task atau ISR-to-task"

Analogi dunia nyata:
🚦 Traffic light - Binary semaphore
🎫 Tiket antrian - Counting semaphore

Tidak mentransfer data, hanya SINYAL!

Dua jenis:
┌────────────────────┬────────────────────┐
│ Binary Semaphore   │ Counting Semaphore │
├────────────────────┼────────────────────┤
│ Value: 0 atau 1    │ Value: 0 sampai N  │
│ Give-Take signal   │ Resource pool      │
└────────────────────┴────────────────────┘
```

---

## Slide 14: Binary Semaphore
```
Judul: "Binary Semaphore: ISR-to-Task Signaling"
Diagram:
[ISR] ─────Give────→ [Binary Sema] ←───Take─── [Task]
           |              |              |
        Event!         0 → 1          Blocked
                         ↓
                      1 → 0
                         ↓
                    Task Unblocked!

Create & Use:
SemaphoreHandle_t xSema;
xSema = xSemaphoreCreateBinary();

// Di ISR
xSemaphoreGiveFromISR(xSema, &pxHigherPriorityTaskWoken);

// Di Task
xSemaphoreTake(xSema, portMAX_DELAY);
printf("Event received!\n");

Use case: "Button press, data ready, timer event"
```

---

## Slide 15: Counting Semaphore
```
Judul: "Counting Semaphore: Resource Pool Management"
Skenario:
"Ada 3 printer, tapi 5 task ingin print"

Diagram visual:
[Task 1] ──┐
[Task 2] ──┤
[Task 3] ──┼──→ [Count: 3] ──→ [Printer 1]
[Task 4] ──┤                    [Printer 2]
[Task 5] ──┘                    [Printer 3]

Create:
// 3 max, 3 initially available
SemaphoreHandle_t xPrinterPool;
xPrinterPool = xSemaphoreCreateCounting(3, 3);

Usage:
if(xSemaphoreTake(xPrinterPool, timeout) == pdTRUE) {
    use_printer();
    xSemaphoreGive(xPrinterPool);  // Release
}

Status: uxSemaphoreGetCount(xPrinterPool) → berapa tersedia
```

---

*Lanjutan di PPT_Prompts_2.md*
