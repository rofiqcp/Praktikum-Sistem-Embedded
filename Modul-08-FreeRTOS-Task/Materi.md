# Modul 09: FreeRTOS — Task Management


## Daftar Isi

## Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami konsep dasar Real-Time Operating System (RTOS) dan FreeRTOS
2. Menguasai manajemen task: pembuatan, prioritas, state, dan lifecycle
3. Mengimplementasikan multi-tasking pada STM32 dan ESP32
4. Menerapkan scheduling algorithm dan preemption
5. Melakukan debugging task-related issues
6. Menerapkan best practices dalam RTOS task design

---


## 1. Pendahuluan Real-Time Operating System

### 1.1 Apa itu RTOS?

Real-Time Operating System (RTOS) adalah sistem operasi yang dirancang untuk menjalankan aplikasi dengan waktu respons yang terjamin dan dapat diprediksi (deterministic). Berbeda dengan general-purpose OS (seperti Windows/Linux), RTOS mengutamakan:

- **Deterministic Timing**: Waktu eksekusi dapat diprediksi
- **Preemptive Scheduling**: Task prioritas tinggi dapat menginterupsi task prioritas rendah
- **Low Latency**: Waktu respons minimal terhadap event
- **Resource Efficiency**: Footprint memori kecil untuk embedded systems

```
┌─────────────────────────────────────────────────────────────┐
│                    RTOS vs Bare-Metal                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Bare-Metal (Super Loop)         RTOS Architecture          │
│  ┌──────────────────┐           ┌──────────────────────┐    │
│  │   main()         │           │     RTOS Kernel      │    │
│  │   {              │           │  ┌─────┬─────┬─────┐ │    │
│  │     while(1) {   │           │  │Task1│Task2│Task3│ │    │
│  │       task1();   │           │  └──┬──┴──┬──┴──┬──┘ │    │
│  │       task2();   │           │     │     │     │    │    │
│  │       task3();   │           │  ┌──▼──┬──▼──┬──▼──┐ │    │
│  │     }            │           │  │Sched│ Sync│ Mem │ │    │
│  │   }              │           │  └─────┴─────┴─────┘ │    │
│  └──────────────────┘           └──────────────────────┘    │
│                                                              │
│  Sequential Execution            Concurrent Execution        │
│  No Preemption                   Preemptive Scheduling       │
│  Manual Timing                   RTOS Manages Timing         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 FreeRTOS Overview

FreeRTOS adalah open-source RTOS yang paling populer untuk embedded systems:

| Fitur | Deskripsi |
|-------|-----------|
| **Footprint** | < 10KB ROM, < 1KB RAM minimum |
| **Portable** | 35+ arsitektur didukung |
| **Preemptive** | Full preemptive scheduler |
| **Features** | Tasks, Queues, Semaphores, Mutexes, Timers |
| **License** | MIT (gratis untuk komersial) |

```c
// FreeRTOS Architecture Layers
┌─────────────────────────────────────┐
│         Application Tasks           │  ← User Code
├─────────────────────────────────────┤
│         FreeRTOS API                │  ← API Functions
├─────────────────────────────────────┤
│         FreeRTOS Kernel             │  ← Scheduler, Memory
├─────────────────────────────────────┤
│         Port Layer                  │  ← Hardware Specific
├─────────────────────────────────────┤
│         Hardware (MCU)              │  ← STM32/ESP32
└─────────────────────────────────────┘
```

---

## 2. Task Fundamentals

### 2.1 Apa itu Task?

Task adalah unit eksekusi independen dalam FreeRTOS. Setiap task memiliki:

- **Stack sendiri**: Menyimpan variabel lokal dan konteks
- **Task Control Block (TCB)**: Metadata task (state, priority, dll)
- **Entry function**: Fungsi yang dijalankan task

```c
// Anatomi Task
┌─────────────────────────────────────────┐
│              Task Structure              │
├─────────────────────────────────────────┤
│                                          │
│  ┌─────────────────────────────────┐    │
│  │    Task Control Block (TCB)     │    │
│  │  • Task State                   │    │
│  │  • Priority                     │    │
│  │  • Stack Pointer               │    │
│  │  • Task Name                   │    │
│  │  • Notification Value          │    │
│  └─────────────────────────────────┘    │
│                  │                       │
│                  ▼                       │
│  ┌─────────────────────────────────┐    │
│  │         Task Stack              │    │
│  │  • Local Variables              │    │
│  │  • Function Parameters          │    │
│  │  • Return Addresses             │    │
│  │  • Saved CPU Registers          │    │
│  └─────────────────────────────────┘    │
│                                          │
└─────────────────────────────────────────┘
```

### 2.2 Task States

```
┌────────────────────────────────────────────────────────────────┐
│                     FreeRTOS Task States                        │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│                    vTaskCreate()                                │
│                         │                                       │
│                         ▼                                       │
│              ┌──────────────────┐                              │
│              │      READY       │◄──────────────────┐          │
│              │  (Siap dijalankan)│                   │          │
│              └────────┬─────────┘                   │          │
│                       │                              │          │
│          Scheduler picks task                        │          │
│                       │                              │          │
│                       ▼                              │          │
│              ┌──────────────────┐                   │          │
│              │     RUNNING      │                   │          │
│              │ (Sedang berjalan)│                   │          │
│              └────────┬─────────┘                   │          │
│                       │                              │          │
│         ┌─────────────┼─────────────┐              │          │
│         │             │             │               │          │
│    vTaskDelay()  vTaskSuspend()  Preempted/Yield   │          │
│         │             │             │               │          │
│         ▼             ▼             └───────────────┘          │
│  ┌────────────┐ ┌────────────┐                                │
│  │  BLOCKED   │ │ SUSPENDED  │                                │
│  │(Menunggu)  │ │ (Ditunda)  │                                │
│  └─────┬──────┘ └─────┬──────┘                                │
│        │              │                                        │
│   Event/Timeout  vTaskResume()                                 │
│        │              │                                        │
│        └──────────────┴────────────────► READY                 │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

| State | Deskripsi | Transisi Ke |
|-------|-----------|-------------|
| **Ready** | Siap dijalankan, menunggu CPU | Running |
| **Running** | Sedang dieksekusi | Ready, Blocked, Suspended |
| **Blocked** | Menunggu event/timeout | Ready |
| **Suspended** | Ditunda manual | Ready (via vTaskResume) |

### 2.3 Task Priority

FreeRTOS menggunakan priority-based preemptive scheduling:

```c
// Priority Levels (0 = lowest, configMAX_PRIORITIES-1 = highest)
┌─────────────────────────────────────────┐
│           Priority Hierarchy             │
├─────────────────────────────────────────┤
│                                          │
│  Priority Level        Usage             │
│  ─────────────────────────────────────  │
│  configMAX_PRIORITIES-1  Critical/ISR   │
│         ▲                                │
│         │                  Real-time     │
│         │                                │
│         │                  Normal        │
│         │                                │
│         ▼                  Background    │
│         0                  Idle Task     │
│                                          │
└─────────────────────────────────────────┘

// Contoh prioritas di sistem embedded
#define PRIORITY_IDLE        0
#define PRIORITY_LOW         1
#define PRIORITY_NORMAL      2
#define PRIORITY_HIGH        3
#define PRIORITY_CRITICAL    4
```

---

## 3. Task API Reference

### 3.1 Membuat Task

```c
// API: xTaskCreate()
BaseType_t xTaskCreate(
    TaskFunction_t pxTaskCode,      // Pointer ke fungsi task
    const char * const pcName,      // Nama task (debug)
    const uint16_t usStackDepth,    // Stack size (words)
    void * const pvParameters,      // Parameter ke task
    UBaseType_t uxPriority,         // Prioritas task
    TaskHandle_t * const pxCreatedTask  // Handle (optional)
);

// Return: pdPASS = sukses, errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY = gagal
```

**Contoh Implementasi:**

```c
// Task function template
void vTaskFunction(void *pvParameters)
{
    // Inisialisasi (dijalankan sekali)
    
    for(;;)  // atau while(1)
    {
        // Task logic
        
        // PENTING: Harus ada blocking call atau yield
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task tidak boleh return
    // Jika perlu terminate: vTaskDelete(NULL);
}

// Membuat task
TaskHandle_t xTaskHandle;
xTaskCreate(
    vTaskFunction,      // Fungsi task
    "MyTask",           // Nama
    256,                // Stack: 256 words = 1024 bytes (32-bit)
    NULL,               // Parameter
    2,                  // Prioritas
    &xTaskHandle        // Handle
);
```

### 3.2 Stack Size Calculation

```c
// Stack size harus mencakup:
// 1. Variabel lokal fungsi task
// 2. Nested function calls
// 3. Context save area (CPU registers)
// 4. ISR nesting (jika configUSE_PORT_OPTIMISED_TASK_SELECTION = 0)
// 5. Safety margin (20-50%)

// Rumus kasar:
// Stack = (LocalVars + MaxCallDepth*32 + 64) * 1.3

// Contoh perhitungan:
// - Local variables: 100 bytes
// - Max call depth: 5 functions × 32 bytes = 160 bytes
// - Context: 64 bytes
// - Total: 324 bytes
// - Dengan margin 30%: 324 × 1.3 = 421 bytes ≈ 128 words (512 bytes)

// Minimum recommended:
// - Simple task: 128 words (512 bytes)
// - Task with printf: 256 words (1024 bytes)
// - Task with floating point: 256+ words
```

### 3.3 Task Control APIs

```c
// === DELAY APIs ===
// Relative delay (dari sekarang)
void vTaskDelay(const TickType_t xTicksToDelay);

// Absolute delay (periodic execution)
void vTaskDelayUntil(
    TickType_t * const pxPreviousWakeTime,
    const TickType_t xTimeIncrement
);

// Contoh periodic task dengan timing presisi
void vPeriodicTask(void *pvParam)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100);  // 100ms
    
    for(;;)
    {
        // Task logic here
        
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
        // xLastWakeTime di-update otomatis
    }
}

// === SUSPEND/RESUME ===
void vTaskSuspend(TaskHandle_t xTaskToSuspend);  // NULL = self
void vTaskResume(TaskHandle_t xTaskToResume);
BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume);

// === PRIORITY ===
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask);

// === DELETE ===
void vTaskDelete(TaskHandle_t xTaskToDelete);  // NULL = self
```

### 3.4 Task Utilities

```c
// Get task info
char * pcTaskGetName(TaskHandle_t xTaskToQuery);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
UBaseType_t uxTaskGetNumberOfTasks(void);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);

// Runtime stats (perlu configGENERATE_RUN_TIME_STATS = 1)
void vTaskGetRunTimeStats(char *pcWriteBuffer);
void vTaskList(char *pcWriteBuffer);
```

---

## 4. Scheduler dan Context Switching

### 4.1 Scheduler Operation

```
┌─────────────────────────────────────────────────────────────────┐
│                    Scheduler Algorithm                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   1. Find highest priority ready task                           │
│                                                                  │
│   2. If multiple tasks at same priority:                        │
│      → Round-robin (time-slicing)                               │
│                                                                  │
│   3. Context switch if:                                         │
│      a) Higher priority task becomes ready                      │
│      b) Running task blocks/yields                              │
│      c) Time slice expires (same priority)                      │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                      Time Slice Example                     │ │
│  │                                                             │ │
│  │  Task A (Pri 2)  ████████░░░░░░░░████████░░░░░░░░          │ │
│  │  Task B (Pri 2)  ░░░░░░░░████████░░░░░░░░████████          │ │
│  │  Task C (Pri 1)  ─────────────────────────────────          │ │
│  │                  │        │        │        │               │ │
│  │               Tick     Tick     Tick     Tick              │ │
│  │                                                             │ │
│  │  Task C tidak mendapat CPU karena prioritas lebih rendah    │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 Context Switch Mechanism

```c
// Context switch terjadi saat:
// 1. SysTick interrupt (tick timer)
// 2. PendSV (software trigger)
// 3. API call yang menyebabkan preemption

// Context yang disimpan:
// - CPU registers (R0-R12, LR, PC, xPSR untuk ARM Cortex-M)
// - Stack pointer
// - Task-specific data

/*
 * Context Switch Flow:
 * 
 *  Current Task                         New Task
 *  ────────────                         ────────────
 *      │                                     │
 *      │ ◄── Save Context                    │
 *      │     (Push registers to stack)       │
 *      │                                     │
 *      │ ◄── Update TCB                      │
 *      │     (Save SP to TCB)                │
 *      │                                     │
 *      │ ◄── Select Next Task                │
 *      │     (Scheduler decision)            │
 *      │                                     │
 *      │                      Load Context ──► │
 *      │                      (Load SP from TCB)
 *      │                                     │
 *      │                  Restore Context ──► │
 *      │                  (Pop registers)    │
 *      │                                     │
 *                                            ▼
 *                                        Continue
 */
```

### 4.3 Tick Configuration

```c
// FreeRTOSConfig.h
#define configTICK_RATE_HZ    1000   // 1ms tick (1000 Hz)
#define configCPU_CLOCK_HZ    72000000  // 72 MHz (STM32F103)

// Trade-offs:
// Higher tick rate:
// + More responsive
// + Better timing resolution
// - More overhead (context switches)
// - Higher power consumption

// Lower tick rate:
// + Less overhead
// + Better power efficiency
// - Less responsive
// - Coarser timing

// Conversion macros
pdMS_TO_TICKS(ms)    // Convert milliseconds to ticks
pdTICKS_TO_MS(ticks) // Convert ticks to milliseconds
```

---

## 5. Implementasi STM32

### 5.1 Setup Project dengan STM32CubeMX

```c
// 1. Enable FreeRTOS di STM32CubeMX
//    Middleware → FREERTOS → Interface: CMSIS_V2

// 2. Konfigurasi di FreeRTOSConfig.h
#define configUSE_PREEMPTION            1
#define configUSE_TIME_SLICING          1
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              SystemCoreClock
#define configTICK_RATE_HZ              1000
#define configMAX_PRIORITIES            7
#define configMINIMAL_STACK_SIZE        128
#define configTOTAL_HEAP_SIZE           10240
#define configMAX_TASK_NAME_LEN         16
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_QUEUE_SETS            1
#define configQUEUE_REGISTRY_SIZE       8
#define configUSE_TASK_NOTIFICATIONS    1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 3

// Heap implementation (pilih salah satu)
// heap_1.c: Simplest, no free
// heap_2.c: Best fit, no coalescing
// heap_3.c: Wrapper malloc/free
// heap_4.c: First fit, coalescing (recommended)
// heap_5.c: Multiple memory regions
```

### 5.2 STM32 Code Template

```c
/* Includes */
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Task Handles */
TaskHandle_t xLedTaskHandle = NULL;
TaskHandle_t xUartTaskHandle = NULL;

/* Task Functions */
void vLedTask(void *pvParameters)
{
    (void)pvParameters;
    
    // Konfigurasi GPIO untuk LED
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    
    for(;;)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vUartTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    for(;;)
    {
        printf("[UART] Tick: %lu\n", xTaskGetTickCount());
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART_Init();
    
    printf("=== FreeRTOS Task Demo ===\n");
    
    // Buat tasks
    xTaskCreate(vLedTask, "LED", 128, NULL, 2, &xLedTaskHandle);
    xTaskCreate(vUartTask, "UART", 256, NULL, 1, &xUartTaskHandle);
    
    // Start scheduler
    vTaskStartScheduler();
    
    // Tidak akan sampai sini
    while(1);
}
```

---

## 6. Implementasi ESP32

### 6.1 ESP32 FreeRTOS Differences

ESP32 menggunakan FreeRTOS yang dimodifikasi oleh Espressif (ESP-IDF FreeRTOS):

| Fitur | Standard FreeRTOS | ESP-IDF FreeRTOS |
|-------|-------------------|-------------------|
| **Cores** | Single core | Dual core (PRO & APP) |
| **Affinity** | N/A | Task dapat di-pin ke core |
| **Stack** | Words | Bytes |
| **Default tick** | Configurable | 1ms (100 Hz default) |

```c
// ESP32 Task Creation dengan Core Affinity
// Core 0: PRO_CPU (Protocol CPU)
// Core 1: APP_CPU (Application CPU)

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char *const pcName,
    const uint32_t usStackDepth,  // dalam BYTES!
    void *const pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *const pvCreatedTask,
    const BaseType_t xCoreID      // 0, 1, atau tskNO_AFFINITY
);

// Contoh:
xTaskCreatePinnedToCore(
    vSensorTask,
    "Sensor",
    4096,           // 4KB stack (bytes)
    NULL,
    2,
    &xSensorHandle,
    1               // Pin ke Core 1 (APP_CPU)
);
```

### 6.2 ESP32 Arduino Framework

```cpp
// ESP32 dengan Arduino Framework
// FreeRTOS sudah built-in

void setup() {
    Serial.begin(115200);
    
    // Create tasks
    xTaskCreatePinnedToCore(
        ledTask,        // Function
        "LED_Task",     // Name
        2048,           // Stack (bytes)
        NULL,           // Parameters
        1,              // Priority
        NULL,           // Handle
        0               // Core 0
    );
    
    xTaskCreatePinnedToCore(
        sensorTask,
        "Sensor_Task",
        4096,
        NULL,
        2,
        NULL,
        1               // Core 1
    );
}

void loop() {
    // Tidak digunakan - semua di tasks
    vTaskDelete(NULL);  // Delete loop task
}

void ledTask(void *pvParameter) {
    pinMode(2, OUTPUT);  // Built-in LED
    
    for(;;) {
        digitalWrite(2, HIGH);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        digitalWrite(2, LOW);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void sensorTask(void *pvParameter) {
    for(;;) {
        Serial.printf("[Sensor] Running on Core %d\n", xPortGetCoreID());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

### 6.3 ESP-IDF Native

```c
// ESP-IDF Framework
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TASK_DEMO";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FreeRTOS Task Demo");
    
    xTaskCreate(
        vTask1,
        "Task1",
        2048,
        NULL,
        5,
        NULL
    );
    
    // app_main dapat return di ESP-IDF
    // FreeRTOS scheduler sudah berjalan
}
```

---

## 7. Advanced Task Patterns

### 7.1 Dynamic Task Creation/Deletion

```c
// Pattern: Worker task yang dibuat on-demand
TaskHandle_t xWorkerHandle = NULL;

void vCreateWorker(void)
{
    if(xWorkerHandle == NULL) {
        xTaskCreate(vWorkerTask, "Worker", 256, NULL, 2, &xWorkerHandle);
    }
}

void vDeleteWorker(void)
{
    if(xWorkerHandle != NULL) {
        vTaskDelete(xWorkerHandle);
        xWorkerHandle = NULL;
    }
}

void vWorkerTask(void *pvParam)
{
    // Do work
    for(int i = 0; i < 10; i++) {
        printf("Working... %d\n", i);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Self-delete
    xWorkerHandle = NULL;
    vTaskDelete(NULL);
}
```

### 7.2 Task dengan Parameter

```c
// Passing parameters ke task
typedef struct {
    uint32_t id;
    uint16_t interval;
    GPIO_TypeDef *port;
    uint16_t pin;
} TaskParams_t;

void vParameterizedTask(void *pvParameters)
{
    TaskParams_t *params = (TaskParams_t *)pvParameters;
    
    for(;;) {
        HAL_GPIO_TogglePin(params->port, params->pin);
        printf("Task %lu: Toggle\n", params->id);
        vTaskDelay(pdMS_TO_TICKS(params->interval));
    }
}

// PENTING: params harus persistent (static atau heap)
static TaskParams_t taskParams = {
    .id = 1,
    .interval = 500,
    .port = GPIOA,
    .pin = GPIO_PIN_5
};

xTaskCreate(vParameterizedTask, "Param", 256, &taskParams, 2, NULL);
```

### 7.3 Rate Monotonic Scheduling

```c
// Rate Monotonic: Task dengan period lebih pendek = prioritas lebih tinggi

#define TASK_FAST_PERIOD    10   // 10ms → Priority tinggi
#define TASK_MED_PERIOD     50   // 50ms → Priority medium
#define TASK_SLOW_PERIOD    100  // 100ms → Priority rendah

void vFastTask(void *pvParam)
{
    TickType_t xLastWake = xTaskGetTickCount();
    for(;;) {
        // Critical fast operations
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(TASK_FAST_PERIOD));
    }
}

void vMediumTask(void *pvParam)
{
    TickType_t xLastWake = xTaskGetTickCount();
    for(;;) {
        // Medium frequency operations
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(TASK_MED_PERIOD));
    }
}

void vSlowTask(void *pvParam)
{
    TickType_t xLastWake = xTaskGetTickCount();
    for(;;) {
        // Background operations
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(TASK_SLOW_PERIOD));
    }
}

// Create dengan prioritas sesuai period
xTaskCreate(vFastTask, "Fast", 256, NULL, 3, NULL);   // Highest
xTaskCreate(vMediumTask, "Med", 256, NULL, 2, NULL);
xTaskCreate(vSlowTask, "Slow", 256, NULL, 1, NULL);   // Lowest
```

### 7.4 Idle Hook dan Tick Hook

```c
// FreeRTOSConfig.h
#define configUSE_IDLE_HOOK     1
#define configUSE_TICK_HOOK     1

// Idle Hook - dipanggil saat tidak ada task ready
void vApplicationIdleHook(void)
{
    // Power saving
    __WFI();  // Wait For Interrupt (ARM)
    
    // Atau background maintenance
    // JANGAN blocking di sini!
}

// Tick Hook - dipanggil setiap tick
void vApplicationTickHook(void)
{
    // Timing-critical operations
    // Harus sangat cepat!
    
    static uint32_t tickCount = 0;
    tickCount++;
    
    // Toggle pin setiap 1000 ticks
    if(tickCount % 1000 == 0) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
    }
}
```

---

## 8. Debugging dan Troubleshooting

### 8.1 Stack Overflow Detection

```c
// FreeRTOSConfig.h
#define configCHECK_FOR_STACK_OVERFLOW  2  // Method 2 (pattern fill)

// Hook function
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack Overflow in: %s\n", pcTaskName);
    
    // Hentikan sistem atau reset
    while(1);
}

// Monitor stack usage
void vPrintStackUsage(void)
{
    UBaseType_t highWater;
    
    highWater = uxTaskGetStackHighWaterMark(xLedTaskHandle);
    printf("LED Task: %u words remaining\n", highWater);
    
    highWater = uxTaskGetStackHighWaterMark(xUartTaskHandle);
    printf("UART Task: %u words remaining\n", highWater);
}
```

### 8.2 Task Statistics

```c
// Enable runtime stats
#define configGENERATE_RUN_TIME_STATS          1
#define configUSE_TRACE_FACILITY               1
#define configUSE_STATS_FORMATTING_FUNCTIONS   1

// Timer for stats (perlu hardware timer)
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  ConfigureTimerForRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE()          GetRuntimeCounterValue()

// Print task list
void vPrintTaskList(void)
{
    char buffer[512];
    
    printf("\nTask List:\n");
    printf("Name\t\tState\tPri\tStack\tNum\n");
    vTaskList(buffer);
    printf("%s", buffer);
}

// Print runtime stats
void vPrintRunTimeStats(void)
{
    char buffer[512];
    
    printf("\nRuntime Stats:\n");
    printf("Task\t\tAbs Time\t%% Time\n");
    vTaskGetRunTimeStats(buffer);
    printf("%s", buffer);
}
```

### 8.3 Common Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Task tidak berjalan | Priority terlalu rendah | Naikkan priority |
| Stack overflow | Stack terlalu kecil | Tambah stack size |
| System hang | Task tidak yield | Tambah vTaskDelay |
| Erratic behavior | Priority inversion | Gunakan mutex dengan priority inheritance |
| Memory exhausted | Terlalu banyak task | Reduce tasks atau increase heap |

---

## 9. Best Practices

### 9.1 Task Design Guidelines

```c
// DO: Clear, focused tasks
void vSensorTask(void *pvParam)
{
    for(;;) {
        // Satu tanggung jawab: baca sensor
        readSensors();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// DON'T: Monolithic super-task
void vDoEverythingTask(void *pvParam)
{
    for(;;) {
        readSensors();
        updateDisplay();
        checkButtons();
        sendData();
        // Terlalu banyak tanggung jawab!
    }
}
```

### 9.2 Memory Considerations

```c
// DO: Static allocation untuk predictability
static StackType_t xStack[256];
static StaticTask_t xTaskBuffer;

TaskHandle_t xHandle = xTaskCreateStatic(
    vTaskFunction,
    "Static",
    256,
    NULL,
    2,
    xStack,
    &xTaskBuffer
);

// DON'T: Excessive dynamic allocation
for(int i = 0; i < 100; i++) {
    xTaskCreate(...);  // Mungkin kehabisan memory!
}
```

### 9.3 Priority Guidelines

```c
// Recommended priority scheme:
// 0       : Idle (reserved)
// 1       : Background/logging
// 2       : Normal operations
// 3       : Time-sensitive
// 4       : Critical/safety
// 5+      : Reserved for system

#define PRIO_IDLE       0
#define PRIO_LOG        1
#define PRIO_NORMAL     2
#define PRIO_SENSOR     3
#define PRIO_CRITICAL   4
```

---

## 10. Perbandingan STM32 vs ESP32

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| **Cores** | Single (72MHz) | Dual (240MHz each) |
| **RAM** | 20KB | 520KB |
| **FreeRTOS** | Manual integration | Built-in |
| **Stack unit** | Words (4 bytes) | Bytes |
| **Default heap** | heap_4.c | ESP-IDF heap |
| **Tick rate** | 1000Hz typical | 100Hz default |
| **Core affinity** | N/A | Supported |
| **Watchdog** | Manual | Integrated |

---

## Diagram Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│                    FreeRTOS Task Management Summary                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  TASK LIFECYCLE                                                      │
│  ═════════════                                                       │
│                                                                      │
│    xTaskCreate() ──► READY ◄────── vTaskResume()                    │
│                         │              ▲                             │
│                         ▼              │                             │
│                      RUNNING ──────────┼────► SUSPENDED              │
│                         │              │      (vTaskSuspend)         │
│                         ▼              │                             │
│                      BLOCKED ──────────┘                             │
│                   (vTaskDelay, etc.)                                 │
│                                                                      │
│                                                                      │
│  KEY APIs                                                            │
│  ════════                                                            │
│                                                                      │
│  • xTaskCreate()           - Create task                             │
│  • vTaskDelete()           - Delete task                             │
│  • vTaskDelay()            - Relative delay                          │
│  • vTaskDelayUntil()       - Absolute delay (periodic)               │
│  • vTaskSuspend/Resume()   - Manual control                          │
│  • vTaskPrioritySet/Get()  - Priority management                     │
│                                                                      │
│                                                                      │
│  BEST PRACTICES                                                      │
│  ══════════════                                                      │
│                                                                      │
│  ✓ Always include blocking call in task loop                         │
│  ✓ Calculate stack size carefully                                    │
│  ✓ Use vTaskDelayUntil() for periodic tasks                         │
│  ✓ Enable stack overflow detection                                   │
│  ✓ Monitor stack high water mark                                     │
│  ✓ Keep ISRs short, defer to tasks                                   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 11. Daftar Program Praktikum

| No | Platform | Nama Program | Topik | Tingkat |
|----|----------|-------------|-------|---------|
| 01 | ESP32 | Task_Create | Membuat task dasar | Dasar |
| 02 | ESP32 | Task_Priority | Prioritas dan preemption | Dasar |
| 03 | ESP32 | Task_Delay | vTaskDelay dan vTaskDelayUntil | Dasar |
| 04 | ESP32 | Task_Param | Task dengan parameter struct | Menengah |
| 05 | ESP32 | Task_DualCore | Pinning task ke core (dual-core) | Menengah |
| 06 | ESP32 | Task_Dynamic | Dynamic create/delete task | Lanjut |
| 07 | STM32 | Task_Create | Membuat task dasar | Dasar |
| 08 | STM32 | Task_Priority | Prioritas dan preemption | Dasar |
| 09 | STM32 | Task_Delay | vTaskDelay dan vTaskDelayUntil | Dasar |
| 10 | STM32 | Task_Param | Task dengan parameter struct | Menengah |
| 11 | STM32 | Task_Stats | Runtime statistics + vTaskList | Menengah |
| 12 | STM32 | Task_Dynamic | Dynamic create/delete task | Lanjut |

---

3. **STM32 HAL and FreeRTOS Guide** - STMicroelectronics AN4631
4. **ESP-IDF FreeRTOS Documentation** - Espressif Systems
5. **FreeRTOS API Reference** - https://www.freertos.org/a00106.html

