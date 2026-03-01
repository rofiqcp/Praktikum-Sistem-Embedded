# Modul 11: FreeRTOS — Memory Management dan Advanced Features

## 📚 Daftar Isi

1. [Pendahuluan](#1-pendahuluan)
2. [Dynamic Memory Allocation](#2-dynamic-memory-allocation)
3. [Lima Skema Heap](#3-lima-skema-heap)
4. [Stack Management & Overflow Detection](#4-stack-management--overflow-detection)
5. [Static Memory Allocation](#5-static-memory-allocation)
6. [Memory Pool Pattern](#6-memory-pool-pattern)
7. [Stream Buffer & Message Buffer](#7-stream-buffer--message-buffer)
8. [Critical Section](#8-critical-section)
9. [ESP32 Memory Architecture](#9-esp32-memory-architecture)
10. [STM32 Memory Architecture](#10-stm32-memory-architecture)
11. [Heap Fragmentation](#11-heap-fragmentation)
12. [Memory Leak Detection](#12-memory-leak-detection)
13. [System Monitoring Dashboard](#13-system-monitoring-dashboard)
14. [Daftar Program Praktikum](#14-daftar-program-praktikum)

---

## 1. Pendahuluan

Manajemen memori adalah aspek paling kritis dalam pengembangan sistem embedded. Berbeda dengan aplikasi desktop yang memiliki RAM gigabytes dan virtual memory, mikrokontroler hanya memiliki kilobytes RAM. Pada STM32F103 (Blue Pill), total SRAM hanya **20 KB**. Pada ESP32, internal DRAM sekitar **320 KB** (dengan opsional PSRAM 4-8 MB).

Kesalahan manajemen memori menyebabkan:

- **Stack Overflow**: Task menulis melampaui batas stack-nya, merusak data task lain → *Hard Fault*
- **Heap Exhaustion**: `pvPortMalloc()` gagal karena heap penuh → sistem hang
- **Memory Leak**: Memori dialokasi tapi tidak pernah dibebaskan → heap habis pelan-pelan
- **Fragmentasi**: Blok-blok kecil kosong tersebar di heap → alokasi besar gagal meski total free cukup
- **Dangling Pointer**: Mengakses memori yang sudah di-free → data corrupt, crash random

Modul ini membahas secara komprehensif bagaimana FreeRTOS mengelola memori dan teknik-teknik advanced untuk membuat sistem embedded yang robust dan reliable.

### Mengapa Tidak Menggunakan malloc() Standar?

Fungsi `malloc()` dan `free()` dari library C standar memiliki beberapa kelemahan untuk RTOS:

| Aspek | malloc() standar | pvPortMalloc() FreeRTOS |
|-------|-----------------|------------------------|
| Thread Safety | ❌ Tidak thread-safe | ✅ Thread-safe (suspend scheduler) |
| Deterministic | ❌ Waktu eksekusi bervariasi | ✅ Bisa deterministic (heap_1) |
| Configurable | ❌ Satu implementasi | ✅ 5 skema berbeda |
| Debug | ❌ Sulit di-debug | ✅ Hook function, watermark |
| Fragmentation | ❌ Tidak terkontrol | ✅ Bisa dipilih strategi |

---

## 2. Dynamic Memory Allocation

FreeRTOS menyediakan layer abstraksi memori yang portable:

### API Utama

```c
/* Alokasi memori dari FreeRTOS heap */
void *pvPortMalloc(size_t xWantedSize);

/* Bebaskan memori yang dialokasi pvPortMalloc */
void vPortFree(void *pv);

/* Cek sisa heap yang tersedia saat ini */
size_t xPortGetFreeHeapSize(void);

/* Cek sisa heap TERENDAH yang pernah terjadi (High Water Mark) */
size_t xPortGetMinimumEverFreeHeapSize(void);
```

### Contoh Penggunaan (ESP-IDF)

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

void memory_demo_task(void *pvParam)
{
    /* Cek heap sebelum alokasi */
    size_t free_before = xPortGetFreeHeapSize();
    ESP_LOGI("MEM", "Free heap sebelum: %u bytes", (unsigned)free_before);

    /* Alokasi buffer 1024 bytes */
    uint8_t *buffer = (uint8_t *)pvPortMalloc(1024);
    if (buffer == NULL) {
        ESP_LOGE("MEM", "pvPortMalloc GAGAL!");
        vTaskDelete(NULL);
        return;
    }

    /* Cek heap setelah alokasi */
    size_t free_after = xPortGetFreeHeapSize();
    ESP_LOGI("MEM", "Free heap sesudah: %u bytes (berkurang %u)",
             (unsigned)free_after, (unsigned)(free_before - free_after));

    /* Gunakan buffer */
    memset(buffer, 0xAA, 1024);

    /* Bebaskan memori */
    vPortFree(buffer);
    buffer = NULL;  /* Hindari dangling pointer */

    ESP_LOGI("MEM", "Free heap setelah free: %u bytes", 
             (unsigned)xPortGetFreeHeapSize());

    vTaskDelete(NULL);
}
```

### Contoh Penggunaan (STM32 HAL)

```c
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

void vMemoryDemoTask(void *pvParam)
{
    size_t free_before = xPortGetFreeHeapSize();
    printf("Free heap sebelum: %u bytes\r\n", (unsigned)free_before);

    /* Alokasi buffer */
    uint8_t *buffer = (uint8_t *)pvPortMalloc(512);
    if (buffer == NULL) {
        printf("ERROR: pvPortMalloc gagal!\r\n");
        vTaskDelete(NULL);
        return;
    }

    printf("Free heap sesudah: %u bytes\r\n", 
           (unsigned)xPortGetFreeHeapSize());

    /* Bebaskan */
    vPortFree(buffer);
    printf("Free heap setelah free: %u bytes\r\n",
           (unsigned)xPortGetFreeHeapSize());

    vTaskDelete(NULL);
}
```

### Hook Function: Malloc Failed

Jika `pvPortMalloc()` gagal (return NULL), FreeRTOS bisa memanggil hook function:

```c
/* Aktifkan di FreeRTOSConfig.h */
#define configUSE_MALLOC_FAILED_HOOK  1

/* Implementasi hook */
void vApplicationMallocFailedHook(void)
{
    printf("FATAL: Malloc failed! Free heap: %u\r\n",
           (unsigned)xPortGetFreeHeapSize());
    /* Biasanya: log error, reset, atau halt */
    while (1);
}
```

---

## 3. Lima Skema Heap

FreeRTOS menyediakan 5 implementasi heap di folder `portable/MemMang/`:

### Tabel Perbandingan

| Fitur | heap_1 | heap_2 | heap_3 | heap_4 | heap_5 |
|-------|--------|--------|--------|--------|--------|
| Alokasi | ✅ | ✅ | ✅ | ✅ | ✅ |
| Dealokasi (free) | ❌ | ✅ | ✅ | ✅ | ✅ |
| Coalescing | N/A | ❌ | Tergantung libc | ✅ | ✅ |
| Multi-region | ❌ | ❌ | ❌ | ❌ | ✅ |
| Deterministic | ✅ | Hampir | ❌ | Hampir | Hampir |
| Fragmentasi | Tidak ada | Tinggi | Tergantung | Rendah | Rendah |
| Thread-safe | ✅ | ✅ | ✅ | ✅ | ✅ |
| Penggunaan | Safety-critical | Legacy | Wrapper libc | **Recommended** | Multi-RAM |

### Heap_1: Static Only

```
┌──────────────────────────────────┐
│         FreeRTOS Heap            │
│  ┌────┬────┬────┬──────────────┐ │
│  │Task│Task│Que │   UNUSED     │ │
│  │ A  │ B  │ue  │  (wasted)    │ │
│  └────┴────┴────┴──────────────┘ │
│  ← pvPortMalloc        vPortFree = NOP
└──────────────────────────────────┘
```

- Hanya bisa alokasi, tidak bisa free
- Cocok untuk sistem yang membuat semua objek di awal dan tidak pernah menghapus
- Paling aman: tidak ada fragmentasi, tidak ada use-after-free
- Ideal untuk *safety-critical* (DO-178C, IEC 61508)

### Heap_2: Best-Fit (Deprecated)

- Algoritma Best-Fit: mencari blok terkecil yang cukup
- **Tidak melakukan coalescing** (penggabungan blok kosong bersebelahan)
- Rentan fragmentasi berat jika ukuran alokasi bervariasi
- Digantikan oleh heap_4

### Heap_3: Wrapper malloc/free

- Membungkus `malloc()` dan `free()` standar C
- Thread-safe: suspend scheduler saat alokasi
- Ukuran heap ditentukan linker script, bukan `configTOTAL_HEAP_SIZE`
- `xPortGetFreeHeapSize()` tidak tersedia

### Heap_4: First-Fit + Coalescing (RECOMMENDED)

```
Sebelum free:
┌────┬────┬────┬────┬────┬─────────┐
│ A  │ B  │ C  │ D  │ E  │  Free   │
└────┴────┴────┴────┴────┴─────────┘

Setelah free B dan D (tanpa coalescing - heap_2):
┌────┬xxxx┬────┬xxxx┬────┬─────────┐
│ A  │free│ C  │free│ E  │  Free   │  ← 2 lubang kecil
└────┴xxxx┴────┴xxxx┴────┴─────────┘

Setelah free B, C, D (dengan coalescing - heap_4):
┌────┬──────────────┬────┬─────────┐
│ A  │  FREE (BIG)  │ E  │  Free   │  ← 1 blok besar
└────┴──────────────┴────┴─────────┘
```

- Algoritma First-Fit dengan coalescing otomatis
- Menggabungkan blok kosong bersebelahan menjadi satu blok besar
- Fragmentasi minimal
- **Default di STM32CubeF1/F4 FreeRTOS**

### Heap_5: Multi-Region

- Sama seperti heap_4, tapi mendukung multiple memory region
- Berguna ketika RAM terpisah-pisah (misal: Internal SRAM + External SDRAM)
- Memerlukan inisialisasi dengan `vPortDefineHeapRegions()`:

```c
/* Contoh STM32 dengan Internal SRAM + External SRAM */
HeapRegion_t xHeapRegions[] = {
    { (uint8_t *)0x20000000, 0x5000 },  /* 20KB Internal SRAM */
    { (uint8_t *)0x60000000, 0x80000 }, /* 512KB External SRAM */
    { NULL, 0 }                          /* Terminator */
};

int main(void)
{
    vPortDefineHeapRegions(xHeapRegions);
    /* ... create tasks ... */
    vTaskStartScheduler();
}
```

### Konfigurasi di FreeRTOSConfig.h

```c
/* Ukuran total heap (untuk heap_1, heap_2, heap_4, heap_5) */
#define configTOTAL_HEAP_SIZE    ((size_t)(15 * 1024))  /* 15 KB */

/* STM32F103: 20 KB SRAM, sisakan ~5 KB untuk stack & global var */
/* ESP32: bisa sampai ~160 KB (DRAM) */
```

---

## 4. Stack Management & Overflow Detection

### Struktur Stack Task

Setiap task memiliki stack sendiri yang dialokasi dari heap (dynamic) atau dari static buffer:

```
┌─────────────────────────┐  ← Stack Top (alamat tinggi)
│   Saved Context (R0-R15)│  ← Disimpan saat context switch
├─────────────────────────┤
│   Local Variables        │
│   Function Parameters    │
│   Return Addresses       │
│         ...              │
├─────────────────────────┤  ← Stack Pointer (SP) saat ini
│                         │
│    UNUSED SPACE         │  ← High Water Mark
│                         │
├─────────────────────────┤
│   Stack Guard Pattern   │  ← 0xA5A5A5A5 (Method 2)
└─────────────────────────┘  ← Stack Bottom (alamat rendah)
```

### Satuan Stack

| Platform | Satuan usStackDepth | Contoh |
|----------|-------------------|--------|
| STM32 (ARM Cortex-M) | Words (4 bytes) | 256 = 1024 bytes |
| ESP32 (Xtensa) | Bytes | 4096 = 4096 bytes |

### Deteksi Stack Overflow

Konfigurasi di `FreeRTOSConfig.h`:

```c
/* Method 1: Cek SP saat context switch */
#define configCHECK_FOR_STACK_OVERFLOW  1

/* Method 2: Cek pattern 0xA5 di bottom stack (RECOMMENDED) */
#define configCHECK_FOR_STACK_OVERFLOW  2
```

**Method 1**: Cek apakah Stack Pointer melampaui batas saat context switch. Cepat tapi bisa miss overflow yang terjadi di antara context switch.

**Method 2**: FreeRTOS mengisi 20 bytes terakhir stack dengan pattern `0xA5`. Saat context switch, cek apakah pattern masih utuh. Lebih akurat tapi sedikit lebih lambat.

### Hook Function

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("STACK OVERFLOW di task: %s\r\n", pcTaskName);
    /* Log error, simpan ke flash, atau reset */
    while (1);  /* Halt - jangan lanjutkan */
}
```

### High Water Mark

```c
/* Cek sisa stack terendah yang pernah terjadi */
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);  /* NULL = task saat ini */
printf("Stack remaining: %u words\r\n", (unsigned)uxHighWaterMark);

/* Rule of thumb: pastikan minimal 50-100 words tersisa */
/* Jika < 20 words: BERBAHAYA, perbesar stack! */
```

### Tips Sizing Stack

| Jenis Task | Stack Minimum (STM32) | Stack Minimum (ESP32) |
|-----------|----------------------|----------------------|
| Task sederhana (LED toggle) | 128 words (512 B) | 2048 bytes |
| Task dengan printf | 256 words (1 KB) | 4096 bytes |
| Task dengan floating point | 256+ words | 4096+ bytes |
| Task dengan buffer besar | Sesuai buffer + 128 | Sesuai buffer + 2048 |

---

## 5. Static Memory Allocation

### Mengapa Static Allocation?

| Aspek | Dynamic (pvPortMalloc) | Static (CreateStatic) |
|-------|----------------------|----------------------|
| Memori dari | Heap | Global/static variable |
| Bisa gagal runtime | ✅ Ya | ❌ Tidak (compile-time) |
| Fragmentasi | Mungkin | Tidak mungkin |
| Deterministic | Tidak selalu | Selalu |
| Debugging | Sulit (heap address) | Mudah (nama variabel) |
| Safety-critical | Kurang cocok | Sangat cocok |

### Konfigurasi

```c
/* FreeRTOSConfig.h */
#define configSUPPORT_STATIC_ALLOCATION   1
#define configSUPPORT_DYNAMIC_ALLOCATION  1  /* Bisa keduanya aktif */
```

### Contoh: Static Task

```c
/* Buffer stack dan TCB harus static/global */
static StackType_t xTaskStack[256];
static StaticTask_t xTaskTCB;

void vMyTask(void *pvParam)
{
    for (;;) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    /* Task dibuat tanpa pvPortMalloc - tidak bisa gagal */
    TaskHandle_t xHandle = xTaskCreateStatic(
        vMyTask,        /* Function */
        "LED",          /* Name */
        256,            /* Stack size (words) */
        NULL,           /* Parameter */
        2,              /* Priority */
        xTaskStack,     /* Stack buffer */
        &xTaskTCB       /* TCB buffer */
    );
    /* xHandle dijamin valid - tidak perlu cek NULL */
    
    vTaskStartScheduler();
}
```

### Contoh: Static Queue

```c
#define QUEUE_LENGTH  5
#define ITEM_SIZE     sizeof(uint32_t)

static uint8_t ucQueueStorage[QUEUE_LENGTH * ITEM_SIZE];
static StaticQueue_t xQueueBuffer;

QueueHandle_t xQueue = xQueueCreateStatic(
    QUEUE_LENGTH,
    ITEM_SIZE,
    ucQueueStorage,
    &xQueueBuffer
);
```

### Wajib: Idle Task & Timer Task Memory

Jika `configSUPPORT_STATIC_ALLOCATION = 1`, WAJIB menyediakan:

```c
static StaticTask_t xIdleTaskTCB;
static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* Jika configUSE_TIMERS = 1 */
static StaticTask_t xTimerTaskTCB;
static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
```

---

## 6. Memory Pool Pattern

### Masalah Fragmentasi

Alokasi/dealokasi blok berbagai ukuran menyebabkan fragmentasi:

```
Heap setelah banyak alloc/free:
┌──┬xx┬────┬x┬──────┬xxx┬──┬xxxxxxxxx┐
│A │  │ B  │ │  C   │   │D │         │
└──┴xx┴────┴x┴──────┴xxx┴──┴xxxxxxxxx┘
     ↑        ↑           ↑
   16B free  8B free    24B free = total 48B free
   Tapi tidak bisa alokasi 32B berturutan!
```

### Solusi: Fixed-Size Memory Pool

Memory pool mengalokasi blok-blok ukuran tetap. Tidak ada fragmentasi karena semua blok sama besar.

```c
/* Implementasi sederhana menggunakan Queue sebagai free-list */
#define POOL_BLOCK_SIZE   64
#define POOL_BLOCK_COUNT  8

static uint8_t ucPoolMemory[POOL_BLOCK_COUNT][POOL_BLOCK_SIZE];
static QueueHandle_t xFreeList;

void vPoolInit(void)
{
    xFreeList = xQueueCreate(POOL_BLOCK_COUNT, sizeof(void *));
    
    /* Masukkan semua blok ke free-list */
    for (int i = 0; i < POOL_BLOCK_COUNT; i++) {
        void *pBlock = &ucPoolMemory[i][0];
        xQueueSend(xFreeList, &pBlock, 0);
    }
}

void *pvPoolAlloc(TickType_t xTimeout)
{
    void *pBlock = NULL;
    if (xQueueReceive(xFreeList, &pBlock, xTimeout) == pdPASS) {
        return pBlock;
    }
    return NULL;  /* Pool exhausted */
}

void vPoolFree(void *pBlock)
{
    xQueueSend(xFreeList, &pBlock, 0);
}
```

### Keuntungan Memory Pool

- ✅ Zero fragmentasi
- ✅ O(1) alokasi/dealokasi (deterministic)
- ✅ Thread-safe (menggunakan queue)
- ✅ Mudah monitor utilization (`uxQueueMessagesWaiting`)
- ❌ Ukuran blok tetap (waste jika data kecil)

---

## 7. Stream Buffer & Message Buffer

Diperkenalkan di FreeRTOS v10.0.0. Lebih ringan dari Queue untuk transfer data byte/message.

### Stream Buffer

Transfer *byte stream* kontinu (seperti UART RX):

```c
#include "stream_buffer.h"

/* Buat stream buffer 256 bytes, trigger level 16 bytes */
StreamBufferHandle_t xStream = xStreamBufferCreate(256, 16);

/* Sender (bisa dari ISR) */
size_t xBytesSent = xStreamBufferSend(xStream, data, len, pdMS_TO_TICKS(100));

/* Receiver (di task) - akan unblock ketika ≥ trigger_level bytes tersedia */
size_t xBytesReceived = xStreamBufferReceive(xStream, buffer, sizeof(buffer), 
                                              pdMS_TO_TICKS(1000));
```

**Trigger Level**: Receiver akan unblock hanya jika jumlah bytes ≥ trigger level. Berguna untuk efisiensi: proses data dalam batch, bukan byte per byte.

### Message Buffer

Transfer *pesan diskrit* dengan panjang bervariasi:

```c
#include "message_buffer.h"

/* Buat message buffer 256 bytes */
MessageBufferHandle_t xMsg = xMessageBufferCreate(256);

/* Kirim pesan (otomatis ditambah header 4 bytes untuk panjang) */
typedef struct {
    uint8_t type;
    uint16_t value;
} Message_t;

Message_t msg = { .type = 1, .value = 42 };
xMessageBufferSend(xMsg, &msg, sizeof(msg), pdMS_TO_TICKS(100));

/* Terima pesan */
Message_t rx_msg;
size_t xLen = xMessageBufferReceive(xMsg, &rx_msg, sizeof(rx_msg), 
                                     pdMS_TO_TICKS(1000));
```

### Perbandingan

| Fitur | Queue | Stream Buffer | Message Buffer |
|-------|-------|---------------|----------------|
| Data unit | Fixed-size item | Byte stream | Variable-length message |
| Overhead per item | sizeof(item) copy | Minimal | 4 bytes header |
| Multiple writer | ✅ | ❌ (1 writer) | ❌ (1 writer) |
| Multiple reader | ✅ | ❌ (1 reader) | ❌ (1 reader) |
| ISR safe | ✅ | ✅ | ✅ |
| Best for | Structured data | UART/SPI stream | Log messages |

**PENTING**: Stream Buffer dan Message Buffer hanya support **single writer, single reader**. Untuk multi-writer/reader, gunakan Queue.

---

## 8. Critical Section

### Apa itu Critical Section?

Region kode yang TIDAK BOLEH di-interrupt atau di-preempt. Digunakan untuk melindungi operasi yang harus atomik (contoh: update multi-word variable).

### Tiga Level Proteksi

```c
/* Level 1: Suspend Scheduler (task-level only, interrupt tetap jalan) */
vTaskSuspendAll();
{
    /* Kode yang dilindungi dari context switch */
    /* INTERRUPT MASIH BISA TERJADI! */
}
xTaskResumeAll();

/* Level 2: Critical Section (disable interrupt di bawah configMAX_SYSCALL) */
taskENTER_CRITICAL();
{
    /* Kode yang dilindungi dari context switch DAN interrupt */
    /* Harus sangat singkat! */
}
taskEXIT_CRITICAL();

/* Level 3: Disable ALL interrupts (paling agresif) */
portDISABLE_INTERRUPTS();
{
    /* SEMUA interrupt dimatikan */
    /* Hanya untuk operasi sangat kritis dan sangat singkat */
}
portENABLE_INTERRUPTS();
```

### Contoh: Protect Shared Struct

```c
typedef struct {
    uint32_t field_a;
    uint32_t field_b;
    uint32_t checksum;
} SharedData_t;

static SharedData_t xShared;

void vWriterTask(void *pvParam)
{
    uint32_t counter = 0;
    for (;;) {
        counter++;
        
        taskENTER_CRITICAL();
        {
            xShared.field_a = counter;
            xShared.field_b = counter * 2;
            xShared.checksum = xShared.field_a + xShared.field_b;
        }
        taskEXIT_CRITICAL();
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vReaderTask(void *pvParam)
{
    for (;;) {
        uint32_t a, b, cs;
        
        taskENTER_CRITICAL();
        {
            a  = xShared.field_a;
            b  = xShared.field_b;
            cs = xShared.checksum;
        }
        taskEXIT_CRITICAL();
        
        if (cs != a + b) {
            printf("DATA CORRUPT! a=%lu b=%lu cs=%lu\r\n", a, b, cs);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

### Critical Section dari ISR

```c
void EXTI0_IRQHandler(void)
{
    UBaseType_t uxSavedInterruptStatus;
    
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    {
        /* Operasi atomik di ISR */
    }
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}
```

### ⚠️ Aturan Critical Section

1. **Durasi MINIMAL** — jangan panggil printf, delay, atau fungsi blocking
2. **Jangan nest berlebihan** — waspadai deadlock
3. **Prefer Mutex** untuk proteksi yang lebih lama
4. Critical section menaikkan latency interrupt → sistem kurang responsive

---

## 9. ESP32 Memory Architecture

ESP32 memiliki arsitektur memori yang unik dengan **multi-heap**:

### Memory Map ESP32

```
┌──────────────────────────────────────┐
│ External SPIRAM (PSRAM) 4-8 MB       │ MALLOC_CAP_SPIRAM
│ 0x3F800000 - 0x3FBFFFFF              │
├──────────────────────────────────────┤
│ DRAM (Data RAM) ~320 KB              │ MALLOC_CAP_DEFAULT
│ 0x3FFB0000 - 0x3FFFFFFF              │ MALLOC_CAP_INTERNAL
│ Digunakan untuk: heap, stack, global │ MALLOC_CAP_8BIT
├──────────────────────────────────────┤
│ IRAM (Instruction RAM) ~128 KB       │ MALLOC_CAP_EXEC
│ 0x40070000 - 0x4009FFFF              │ MALLOC_CAP_32BIT
│ Digunakan untuk: ISR, hot functions  │
├──────────────────────────────────────┤
│ RTC Memory ~8 KB                     │ MALLOC_CAP_RTCRAM
│ Survives deep sleep                  │
└──────────────────────────────────────┘
```

### heap_caps API (ESP-IDF Specific)

```c
#include "esp_heap_caps.h"

/* Alokasi dari region spesifik */
void *p = heap_caps_malloc(1024, MALLOC_CAP_DEFAULT);      /* Internal DRAM */
void *q = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);       /* PSRAM */
void *r = heap_caps_malloc(256, MALLOC_CAP_DMA);           /* DMA-capable */
void *s = heap_caps_malloc(512, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

/* Bebaskan */
heap_caps_free(p);

/* Informasi heap */
size_t free_default  = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
size_t free_spiram   = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

/* Multi-heap info detail */
multi_heap_info_t info;
heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
printf("Total free: %u\n", info.total_free_bytes);
printf("Allocated: %u\n", info.total_allocated_bytes);
printf("Largest block: %u\n", info.largest_free_block);
printf("Free blocks: %u\n", info.free_blocks);
```

### PSRAM (SPIRAM) pada ESP32-WROVER

ESP32-WROVER memiliki 4MB atau 8MB PSRAM eksternal:

```c
/* Cek apakah PSRAM tersedia */
size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
if (spiram_free > 0) {
    /* PSRAM tersedia */
    uint8_t *big_buffer = heap_caps_malloc(1024 * 1024, MALLOC_CAP_SPIRAM); /* 1 MB! */
}

/* Di sdkconfig/menuconfig: */
/* Component config → ESP32-specific → Support for external, SPI-connected RAM → Enable */
```

### Capability Flags

| Flag | Deskripsi |
|------|-----------|
| `MALLOC_CAP_DEFAULT` | RAM default (biasanya DRAM) |
| `MALLOC_CAP_INTERNAL` | Internal RAM only (bukan PSRAM) |
| `MALLOC_CAP_SPIRAM` | External PSRAM only |
| `MALLOC_CAP_DMA` | DMA-accessible RAM |
| `MALLOC_CAP_8BIT` | 8-bit aligned (byte-accessible) |
| `MALLOC_CAP_32BIT` | 32-bit aligned only (IRAM) |
| `MALLOC_CAP_EXEC` | Executable memory (untuk kode) |
| `MALLOC_CAP_RTCRAM` | RTC memory (survives deep sleep) |

---

## 10. STM32 Memory Architecture

### Memory Map STM32F103 (Blue Pill)

```
┌──────────────────────────────────┐
│ Flash Memory (Program)  64 KB    │ 0x08000000 - 0x0800FFFF
│ .text, .rodata                   │
├──────────────────────────────────┤
│ SRAM  20 KB                      │ 0x20000000 - 0x20004FFF
│ ┌──────────────────────────────┐ │
│ │ .data (initialized globals)  │ │
│ ├──────────────────────────────┤ │
│ │ .bss (zero-initialized)      │ │
│ ├──────────────────────────────┤ │
│ │ FreeRTOS Heap (ucHeap[])     │ │ configTOTAL_HEAP_SIZE
│ │ (15 KB default)              │ │
│ ├──────────────────────────────┤ │
│ │ Main Stack (MSP)             │ │
│ └──────────────────────────────┘ │
└──────────────────────────────────┘
```

### STM32F4xx Memory (Advanced)

```
┌──────────────────────────────────┐
│ Flash  256KB - 1MB               │
├──────────────────────────────────┤
│ SRAM1  112 KB                    │ 0x20000000
├──────────────────────────────────┤
│ SRAM2  16 KB                     │ 0x2001C000
├──────────────────────────────────┤
│ CCM RAM  64 KB                   │ 0x10000000
│ (Core Coupled Memory)            │
│ Tidak bisa DMA!                  │
├──────────────────────────────────┤
│ Backup SRAM  4 KB                │ Battery-backed
└──────────────────────────────────┘
```

### Konfigurasi Heap Size

Di `FreeRTOSConfig.h`:

```c
/* STM32F103: 20 KB SRAM total */
/* Sisakan ~5 KB untuk .data, .bss, MSP */
#define configTOTAL_HEAP_SIZE  ((size_t)(15 * 1024))

/* STM32F401: 96 KB SRAM */
#define configTOTAL_HEAP_SIZE  ((size_t)(60 * 1024))

/* STM32F411: 128 KB SRAM */
#define configTOTAL_HEAP_SIZE  ((size_t)(80 * 1024))
```

### External SRAM via FSMC

STM32F4xx dengan FSMC (Flexible Static Memory Controller) bisa menggunakan SRAM eksternal:

```c
/* Konfigurasi FSMC untuk SRAM pada STM32F4 */
/* Alamat: 0x60000000 (Bank 1 NOR/PSRAM) */
/* Menggunakan heap_5 untuk multi-region heap */

HeapRegion_t xHeapRegions[] = {
    { (uint8_t *)ucHeap, configTOTAL_HEAP_SIZE },  /* Internal */
    { (uint8_t *)0x60000000, 0x80000 },              /* External 512KB */
    { NULL, 0 }
};
```

---

## 11. Heap Fragmentation

### Apa itu Fragmentasi?

Fragmentasi terjadi ketika total free memory cukup besar tapi tidak ada blok kontinu yang cukup besar untuk alokasi yang diminta.

### Demo Fragmentasi

```c
/* Alokasi pattern yang menyebabkan fragmentasi */
void *blocks[10];

/* Fase 1: Alokasi alternating */
for (int i = 0; i < 10; i++) {
    blocks[i] = pvPortMalloc((i % 2 == 0) ? 64 : 256);
}
/* Heap: [64][256][64][256][64][256][64][256][64][256] */

/* Fase 2: Free yang genap (64-byte blocks) */
for (int i = 0; i < 10; i += 2) {
    vPortFree(blocks[i]);
    blocks[i] = NULL;
}
/* Heap: [__][256][__][256][__][256][__][256][__][256] */
/*       64B gap  64B gap  64B gap  64B gap  64B gap   */

/* Total free = 5 × 64 = 320 bytes */
/* Tapi pvPortMalloc(128) GAGAL! Blok terbesar = 64 bytes */
```

### Fragmentation Index

```
Index = 1 - (largest_free_block / total_free)

Index = 0    → Tidak ada fragmentasi (semua free = 1 blok)
Index = 0.5  → Moderate
Index > 0.8  → Severe fragmentation!
```

### Strategi Mitigasi

1. **Gunakan heap_4** (coalescing otomatis)
2. **Alokasi di awal** — buat semua objek di `main()`, jangan hapus
3. **Memory Pool** — untuk alokasi yang ukurannya seragam
4. **Static Allocation** — `xTaskCreateStatic()` dll.
5. **Alokasi ukuran seragam** — hindari campuran ukuran berbeda-beda
6. **Monitor heap** — track `xPortGetMinimumEverFreeHeapSize()` dan largest free block

---

## 12. Memory Leak Detection

### Apa itu Memory Leak?

Memory leak terjadi ketika memori dialokasi (`pvPortMalloc`) tapi tidak pernah dibebaskan (`vPortFree`). Heap perlahan-lahan habis.

### Teknik Deteksi

#### A. Monitor Heap Trend

```c
void vLeakDetectorTask(void *pvParam)
{
    size_t prev_free = xPortGetFreeHeapSize();
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));  /* Setiap 10 detik */
        
        size_t curr_free = xPortGetFreeHeapSize();
        
        if (curr_free < prev_free) {
            printf("WARNING: Heap berkurang %u bytes (possible leak!)\r\n",
                   (unsigned)(prev_free - curr_free));
        }
        
        prev_free = curr_free;
    }
}
```

#### B. Allocation Tracker (Wrapping)

```c
/* Tracker entry */
typedef struct {
    void *address;
    size_t size;
    const char *task_name;
    uint32_t timestamp;
} AllocEntry_t;

#define MAX_TRACKED_ALLOCS  32
static AllocEntry_t xAllocLog[MAX_TRACKED_ALLOCS];
static uint32_t ulAllocCount = 0;

/* Custom alloc dengan tracking */
void *pvTrackedMalloc(size_t xSize)
{
    void *pv = pvPortMalloc(xSize);
    if (pv && ulAllocCount < MAX_TRACKED_ALLOCS) {
        taskENTER_CRITICAL();
        xAllocLog[ulAllocCount].address = pv;
        xAllocLog[ulAllocCount].size = xSize;
        xAllocLog[ulAllocCount].task_name = pcTaskGetName(NULL);
        xAllocLog[ulAllocCount].timestamp = xTaskGetTickCount();
        ulAllocCount++;
        taskEXIT_CRITICAL();
    }
    return pv;
}
```

#### C. ESP32 Heap Tracing (ESP-IDF)

```c
#include "esp_heap_trace.h"

#define TRACE_RECORD_COUNT  100
static heap_trace_record_t trace_records[TRACE_RECORD_COUNT];

void start_tracing(void)
{
    heap_trace_init_standalone(trace_records, TRACE_RECORD_COUNT);
    heap_trace_start(HEAP_TRACE_LEAKS);
}

void stop_and_report(void)
{
    heap_trace_stop();
    heap_trace_dump();  /* Print semua unmatched alloc = LEAKS */
}
```

---

## 13. System Monitoring Dashboard

### Capstone: Menggabungkan Semua Konsep

Dashboard sistem menampilkan informasi lengkap tentang keadaan RTOS:

### vTaskList()

```c
#define configUSE_TRACE_FACILITY  1
#define configUSE_STATS_FORMATTING_FUNCTIONS  1

char pcBuffer[512];
vTaskList(pcBuffer);
printf("Task Name\tState\tPri\tStack\tNum\r\n");
printf("%s\r\n", pcBuffer);
```

Output:
```
Task Name    State  Pri  Stack  Num
Dashboard    R      3    180    4
Worker1      B      2    230    1
Worker2      B      2    225    2
IDLE         R      0    118    5
Tmr Svc      B      2    240    3
```

State: R=Ready, B=Blocked, S=Suspended, D=Deleted, X=Running

### vTaskGetRunTimeStats()

```c
#define configGENERATE_RUN_TIME_STATS  1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  /* nothing */
#define portGET_RUN_TIME_COUNTER_VALUE()  HAL_GetTick()

char pcStatsBuffer[512];
vTaskGetRunTimeStats(pcStatsBuffer);
printf("Task\t\tAbs Time\t%% Time\r\n");
printf("%s\r\n", pcStatsBuffer);
```

### Heap Statistics

```c
HeapStats_t xHeapStats;
vPortGetHeapStats(&xHeapStats);

printf("=== Heap Statistics ===\r\n");
printf("Free: %u bytes\r\n", (unsigned)xHeapStats.xAvailableHeapSpaceInBytes);
printf("Largest block: %u bytes\r\n", (unsigned)xHeapStats.xSizeOfLargestFreeBlockInBytes);
printf("Smallest block: %u bytes\r\n", (unsigned)xHeapStats.xSizeOfSmallestFreeBlockInBytes);
printf("Free blocks: %u\r\n", (unsigned)xHeapStats.xNumberOfFreeBlocks);
printf("Min ever free: %u bytes\r\n", (unsigned)xHeapStats.xMinimumEverFreeBytesRemaining);
printf("Alloc calls: %u\r\n", (unsigned)xHeapStats.xNumberOfSuccessfulAllocations);
printf("Free calls: %u\r\n", (unsigned)xHeapStats.xNumberOfSuccessfulFrees);
```

---

## 14. Daftar Program Praktikum

### ESP32 (Framework: ESP-IDF)

| No | Nama Program | Hardware | Konsep |
|----|-------------|----------|--------|
| 01 | Heap_Monitor | Serial | xPortGetFreeHeapSize, heap_caps, multi-heap info |
| 02 | Memory_Allocation | Serial | pvPortMalloc/vPortFree, allocation patterns |
| 03 | Stack_Overflow_Detect | Serial | configCHECK_FOR_STACK_OVERFLOW, hook |
| 04 | Static_Allocation | 1x LED | xTaskCreateStatic, xQueueCreateStatic |
| 05 | Memory_Pool | Serial | Fixed-size pool, queue as free-list |
| 06 | Stream_Buffer | Serial + 1x LED | xStreamBufferCreate, trigger level |
| 07 | Message_Buffer | Serial | xMessageBufferCreate, discrete messages |
| 08 | Critical_Section | Serial + 2x LED | taskENTER_CRITICAL, data protection |
| 09 | Heap_Fragmentation | Serial | Fragmentation demo, pattern analysis |
| 10 | PSRAM_External_RAM | Serial | heap_caps_malloc(MALLOC_CAP_SPIRAM) |
| 11 | Memory_Leak_Detection | Serial | Heap tracing, leak identification |
| 12 | System_Dashboard | Serial + OLED | Capstone: task + heap + stack dashboard |

### STM32 (Framework: STM32Cube HAL)

| No | Nama Program | Hardware | Konsep |
|----|-------------|----------|--------|
| 01 | Heap_Monitor | Serial | xPortGetFreeHeapSize, min-ever free |
| 02 | Memory_Allocation | Serial | pvPortMalloc/vPortFree with heap_4 |
| 03 | Stack_Overflow_Detect | Serial | Stack overflow hook, watermark |
| 04 | Static_Allocation | 1x LED | xTaskCreateStatic, no dynamic alloc |
| 05 | Memory_Pool | Serial | Pool pattern with queue free-list |
| 06 | Stream_Buffer | Serial + 1x LED | xStreamBufferCreate, byte stream |
| 07 | Message_Buffer | Serial | xMessageBufferCreate, multi-type messages |
| 08 | Critical_Section | Serial + 2x LED | taskENTER_CRITICAL, corruption demo |
| 09 | Heap_Fragmentation | Serial | Multi-phase fragmentation analysis |
| 10 | PSRAM_External_RAM | Serial | Memory region comparison, FSMC concept |
| 11 | Memory_Leak_Detection | Serial | Allocation tracker, leak reports |
| 12 | System_Dashboard | Serial | Capstone: vTaskList + vTaskGetRunTimeStats |

### Python Debug Scripts

Setiap program dilengkapi script Python (`debug_*.py`) untuk:
- Membaca output serial via pyserial
- Parsing data terstruktur
- Plotting grafik dengan matplotlib
- Logging ke CSV
- Real-time monitoring

---

## Referensi

1. FreeRTOS Documentation — Memory Management: https://www.freertos.org/a00111.html
2. Kolban's Book on ESP32, Pages 315-325: Memory Management
3. Mastering STM32, 2nd Edition: FreeRTOS Memory Chapter
4. ESP-IDF Programming Guide — Heap Memory Allocation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/mem_alloc.html
5. FreeRTOS Stream & Message Buffers: https://www.freertos.org/RTOS-stream-message-buffers.html
