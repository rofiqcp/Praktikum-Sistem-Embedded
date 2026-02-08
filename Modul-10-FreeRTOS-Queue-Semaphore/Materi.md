# Materi Modul 10: FreeRTOS Queue dan Semaphore

## 📚 Daftar Isi
1. [Pendahuluan](#1-pendahuluan)
2. [Queue (Antrian)](#2-queue-antrian)
3. [Semaphore](#3-semaphore)
4. [Mutex](#4-mutex)
5. [Implementasi pada STM32](#5-implementasi-pada-stm32)
6. [Implementasi pada ESP32](#6-implementasi-pada-esp32)
7. [Best Practices](#7-best-practices)
8. [Debugging dan Troubleshooting](#8-debugging-dan-troubleshooting)

---

## 1. Pendahuluan

### 1.1 Mengapa Perlu Inter-Task Communication?

Dalam sistem multitasking, task-task perlu:
- **Bertukar data** (sensor data, commands, status)
- **Koordinasi** (siapa yang akses resource)
- **Sinkronisasi** (kapan harus jalan, kapan menunggu)

```
┌─────────────────────────────────────────────────────────────┐
│              INTER-TASK COMMUNICATION                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌────────┐                           ┌────────┐           │
│   │ Task A │──── Data Exchange ───────►│ Task B │           │
│   └────────┘        (Queue)            └────────┘           │
│                                                              │
│   ┌────────┐                           ┌────────┐           │
│   │ Task C │◄─── Synchronization ─────►│ Task D │           │
│   └────────┘     (Semaphore)           └────────┘           │
│                                                              │
│   ┌────────┐      ┌──────────┐         ┌────────┐           │
│   │ Task E │─────►│ Shared   │◄────────│ Task F │           │
│   └────────┘      │ Resource │         └────────┘           │
│                   └────┬─────┘                               │
│                        │ Mutex                               │
│                        ▼                                     │
│                   Protected!                                 │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Masalah Tanpa Mekanisme Sinkronisasi

#### Race Condition
```c
// PROBLEM: Race condition pada shared variable
volatile int counter = 0;

void Task1(void *p) {
    for(;;) {
        counter++;  // Read-Modify-Write NOT atomic!
        vTaskDelay(1);
    }
}

void Task2(void *p) {
    for(;;) {
        counter++;  // Bisa terjadi konflik!
        vTaskDelay(1);
    }
}
// Counter bisa salah karena operasi tidak atomic
```

#### Data Corruption
```c
// PROBLEM: Data corruption pada struct
typedef struct {
    int x;
    int y;
    int z;
} Point3D_t;

Point3D_t position;  // Shared variable

void WriterTask(void *p) {
    position.x = 10;
    // Context switch terjadi di sini!
    position.y = 20;
    position.z = 30;
}

void ReaderTask(void *p) {
    // Bisa baca data tidak konsisten:
    // x=10 (baru), y=old, z=old
    printf("Pos: %d, %d, %d\n", position.x, position.y, position.z);
}
```

### 1.3 Solusi: Queue, Semaphore, dan Mutex

| Mekanisme | Kegunaan | Analogi |
|-----------|----------|---------|
| **Queue** | Transfer data antar task | Kotak surat |
| **Binary Semaphore** | Sinkronisasi event | Lampu lalu lintas |
| **Counting Semaphore** | Menghitung resource tersedia | Parkir (slot tersedia) |
| **Mutex** | Proteksi shared resource | Kunci kamar mandi |

---

## 2. Queue (Antrian)

### 2.1 Konsep Queue

Queue adalah mekanisme FIFO (First In First Out) untuk mentransfer data antar task dengan aman.

```
┌─────────────────────────────────────────────────────────────┐
│                    QUEUE OPERATION                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   Producer Task                              Consumer Task   │
│   ┌─────────┐                               ┌─────────┐     │
│   │ Sensor  │                               │ Process │     │
│   │  Task   │                               │  Task   │     │
│   └────┬────┘                               └────▲────┘     │
│        │                                         │          │
│        │ xQueueSend()                            │          │
│        │                                         │          │
│        ▼                                         │          │
│   ┌─────────────────────────────────────┐       │          │
│   │  Queue (FIFO Buffer)                 │       │          │
│   │  ┌───┬───┬───┬───┬───┐              │       │          │
│   │  │ D │ C │ B │ A │   │◄─────────────┼───────┘          │
│   │  └───┴───┴───┴───┴───┘              │  xQueueReceive() │
│   │   ↑               ↑                  │                  │
│   │  Head           Tail                 │                  │
│   │  (oldest)       (newest)             │                  │
│   └─────────────────────────────────────┘                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Queue API

#### Membuat Queue
```c
QueueHandle_t xQueueCreate(
    UBaseType_t uxQueueLength,    // Jumlah item maksimal
    UBaseType_t uxItemSize        // Ukuran setiap item (bytes)
);

// Contoh:
QueueHandle_t xSensorQueue;
xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
// Queue untuk 10 item SensorData_t
```

#### Mengirim ke Queue
```c
// Kirim dengan timeout
BaseType_t xQueueSend(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    TickType_t xTicksToWait    // Timeout
);

// Kirim ke depan queue (override FIFO)
BaseType_t xQueueSendToFront(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    TickType_t xTicksToWait
);

// Kirim dari ISR (no blocking!)
BaseType_t xQueueSendFromISR(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

#### Menerima dari Queue
```c
// Ambil dan hapus dari queue
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void *pvBuffer,
    TickType_t xTicksToWait
);

// Intip tanpa menghapus
BaseType_t xQueuePeek(
    QueueHandle_t xQueue,
    void *pvBuffer,
    TickType_t xTicksToWait
);

// Terima dari ISR
BaseType_t xQueueReceiveFromISR(
    QueueHandle_t xQueue,
    void *pvBuffer,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

#### Informasi Queue
```c
// Jumlah item dalam queue
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);

// Ruang tersisa
UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue);

// Hapus semua isi queue
BaseType_t xQueueReset(QueueHandle_t xQueue);
```

### 2.3 Contoh Penggunaan Queue

```c
// Definisi data
typedef struct {
    uint32_t timestamp;
    float temperature;
    float humidity;
    uint16_t lightLevel;
} SensorData_t;

QueueHandle_t xSensorQueue;

// Producer Task - Membaca sensor
void vSensorTask(void *pvParameters)
{
    SensorData_t data;
    
    for(;;)
    {
        // Baca sensor
        data.timestamp = xTaskGetTickCount();
        data.temperature = readTemperature();
        data.humidity = readHumidity();
        data.lightLevel = readLightSensor();
        
        // Kirim ke queue (wait max 100ms jika penuh)
        if(xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) != pdPASS)
        {
            printf("ERROR: Queue full!\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Consumer Task - Memproses data
void vProcessTask(void *pvParameters)
{
    SensorData_t receivedData;
    
    for(;;)
    {
        // Tunggu data dari queue (block indefinitely)
        if(xQueueReceive(xSensorQueue, &receivedData, portMAX_DELAY) == pdPASS)
        {
            printf("T: %.1f, H: %.1f, L: %d\n",
                   receivedData.temperature,
                   receivedData.humidity,
                   receivedData.lightLevel);
            
            // Proses data
            if(receivedData.temperature > 30.0)
            {
                // Trigger alert
            }
        }
    }
}

// Inisialisasi
int main(void)
{
    // Buat queue
    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    
    if(xSensorQueue != NULL)
    {
        xTaskCreate(vSensorTask, "Sensor", 256, NULL, 2, NULL);
        xTaskCreate(vProcessTask, "Process", 256, NULL, 2, NULL);
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

### 2.4 Queue dari ISR

```c
QueueHandle_t xButtonQueue;

// ISR untuk button press
void EXTI0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t buttonEvent = 1;
    
    if(EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;  // Clear flag
        
        // Kirim event ke queue dari ISR
        xQueueSendFromISR(xButtonQueue, &buttonEvent, &xHigherPriorityTaskWoken);
        
        // Context switch jika ada task dengan priority lebih tinggi
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Task yang menunggu button event
void vButtonHandlerTask(void *pvParameters)
{
    uint8_t event;
    
    for(;;)
    {
        if(xQueueReceive(xButtonQueue, &event, portMAX_DELAY) == pdPASS)
        {
            printf("Button pressed!\n");
            // Handle button
        }
    }
}
```

### 2.5 Multiple Producers/Consumers

```c
// Multiple sensors kirim ke satu queue
void vTempSensorTask(void *p) {
    SensorData_t data;
    data.type = SENSOR_TEMPERATURE;
    for(;;) {
        data.value = readTemperature();
        xQueueSend(xSensorQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vHumiditySensorTask(void *p) {
    SensorData_t data;
    data.type = SENSOR_HUMIDITY;
    for(;;) {
        data.value = readHumidity();
        xQueueSend(xSensorQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Single consumer
void vDataLoggerTask(void *p) {
    SensorData_t data;
    for(;;) {
        if(xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdPASS) {
            logToSD(data.type, data.value);
        }
    }
}
```

---

## 3. Semaphore

### 3.1 Konsep Semaphore

Semaphore adalah mekanisme sinkronisasi yang menggunakan counter untuk mengontrol akses.

```
┌─────────────────────────────────────────────────────────────┐
│              BINARY SEMAPHORE                                │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   State: Available (1) or Taken (0)                         │
│                                                              │
│   ┌───────────┐      Give        ┌───────────┐              │
│   │   ISR     │───────────────►  │ Semaphore │              │
│   │ (Button)  │                  │   [1]     │              │
│   └───────────┘                  └─────┬─────┘              │
│                                        │                     │
│                                        │ Take                │
│                                        ▼                     │
│                                  ┌───────────┐              │
│                                  │   Task    │              │
│                                  │ (Handler) │              │
│                                  └───────────┘              │
│                                                              │
│   Timeline:                                                  │
│   ─────────────────────────────────────────────────────►    │
│   ISR gives ──► Task takes ──► Task runs ──► Task blocks    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Binary Semaphore

Binary semaphore hanya memiliki dua state: tersedia (1) atau tidak tersedia (0).

#### API Binary Semaphore
```c
// Membuat binary semaphore
SemaphoreHandle_t xSemaphoreCreateBinary(void);

// Give (release) semaphore
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreGiveFromISR(
    SemaphoreHandle_t xSemaphore,
    BaseType_t *pxHigherPriorityTaskWoken
);

// Take (acquire) semaphore
BaseType_t xSemaphoreTake(
    SemaphoreHandle_t xSemaphore,
    TickType_t xTicksToWait
);
BaseType_t xSemaphoreTakeFromISR(
    SemaphoreHandle_t xSemaphore,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

#### Contoh: ISR to Task Synchronization
```c
SemaphoreHandle_t xButtonSemaphore;

void EXTI0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(EXTI->PR & EXTI_PR_PR0)
    {
        EXTI->PR = EXTI_PR_PR0;
        
        // Signal ke task bahwa button ditekan
        xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void vButtonTask(void *pvParameters)
{
    for(;;)
    {
        // Block sampai semaphore available
        if(xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            printf("Button pressed! Handling...\n");
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        }
    }
}

int main(void)
{
    // Buat binary semaphore (initially empty/unavailable)
    xButtonSemaphore = xSemaphoreCreateBinary();
    
    xTaskCreate(vButtonTask, "Button", 128, NULL, 3, NULL);
    vTaskStartScheduler();
}
```

### 3.3 Counting Semaphore

Counting semaphore memiliki counter yang bisa lebih dari 1, cocok untuk mengelola multiple resources.

```
┌─────────────────────────────────────────────────────────────┐
│            COUNTING SEMAPHORE                                │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   Analogi: Parkir dengan 3 slot                             │
│                                                              │
│   ┌─────┬─────┬─────┐                                       │
│   │  1  │  2  │  3  │  Slots                                │
│   └──┬──┴──┬──┴──┬──┘                                       │
│      │     │     │                                          │
│      ▼     ▼     ▼                                          │
│   Counter = 3 (semua kosong)                                │
│                                                              │
│   Task A takes → Counter = 2                                │
│   Task B takes → Counter = 1                                │
│   Task C takes → Counter = 0                                │
│   Task D waits... (blocked, no slot available)              │
│                                                              │
│   Task A gives → Counter = 1                                │
│   Task D wakes up and takes → Counter = 0                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### API Counting Semaphore
```c
// Membuat counting semaphore
SemaphoreHandle_t xSemaphoreCreateCounting(
    UBaseType_t uxMaxCount,      // Nilai maksimal
    UBaseType_t uxInitialCount   // Nilai awal
);

// Contoh: Semaphore untuk 3 resource
SemaphoreHandle_t xResourceSemaphore;
xResourceSemaphore = xSemaphoreCreateCounting(3, 3);

// Cek nilai counter saat ini
UBaseType_t uxSemaphoreGetCount(SemaphoreHandle_t xSemaphore);
```

#### Contoh: Resource Pool
```c
#define MAX_CONNECTIONS 3

SemaphoreHandle_t xConnectionSemaphore;
int connectionPool[MAX_CONNECTIONS] = {0, 1, 2};

void vClientTask(void *pvParameters)
{
    int taskId = (int)pvParameters;
    
    for(;;)
    {
        printf("Task %d: Requesting connection...\n", taskId);
        
        // Tunggu koneksi tersedia
        if(xSemaphoreTake(xConnectionSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE)
        {
            printf("Task %d: Got connection! (%d available)\n", 
                   taskId, uxSemaphoreGetCount(xConnectionSemaphore));
            
            // Gunakan koneksi
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            // Lepas koneksi
            xSemaphoreGive(xConnectionSemaphore);
            printf("Task %d: Released connection\n", taskId);
        }
        else
        {
            printf("Task %d: Timeout waiting for connection!\n", taskId);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    xConnectionSemaphore = xSemaphoreCreateCounting(MAX_CONNECTIONS, MAX_CONNECTIONS);
    
    // Buat 5 client tasks untuk 3 koneksi
    for(int i = 0; i < 5; i++)
    {
        xTaskCreate(vClientTask, "Client", 256, (void*)i, 2, NULL);
    }
    
    vTaskStartScheduler();
}
```

### 3.4 Event Groups (Alternatif Semaphore)

Event groups memungkinkan task menunggu multiple events.

```c
EventGroupHandle_t xEventGroup;

#define BIT_SENSOR_READY   (1 << 0)
#define BIT_WIFI_CONNECTED (1 << 1)
#define BIT_USER_INPUT     (1 << 2)

void vSensorTask(void *p) {
    // ... sensor ready
    xEventGroupSetBits(xEventGroup, BIT_SENSOR_READY);
}

void vWiFiTask(void *p) {
    // ... wifi connected
    xEventGroupSetBits(xEventGroup, BIT_WIFI_CONNECTED);
}

void vMainTask(void *p) {
    // Tunggu SEMUA bit di-set
    EventBits_t bits = xEventGroupWaitBits(
        xEventGroup,
        BIT_SENSOR_READY | BIT_WIFI_CONNECTED,
        pdTRUE,        // Clear bits setelah return
        pdTRUE,        // Wait for ALL bits
        portMAX_DELAY
    );
    
    printf("All systems ready!\n");
}
```

---

## 4. Mutex

### 4.1 Konsep Mutex

Mutex (Mutual Exclusion) digunakan untuk melindungi shared resource dari akses simultan. Berbeda dengan semaphore, mutex memiliki konsep ownership.

```
┌─────────────────────────────────────────────────────────────┐
│                    MUTEX OPERATION                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   Tanpa Mutex:                    Dengan Mutex:             │
│                                                              │
│   Task A ─┐      ┌─ Task A       Task A ─┐      ┌─ Task A   │
│           ├──────┤                       │ LOCK │           │
│   Task B ─┘      └─ Task B       Task B ─┼──────┼─ Task B   │
│          Race!                          WAIT   LOCK         │
│                                                              │
│   ┌──────────────────────────────────────────────────┐      │
│   │                  Shared Resource                  │      │
│   │    ┌────────────────────────────────────┐        │      │
│   │    │  Only ONE task can access at a time │        │      │
│   │    └────────────────────────────────────┘        │      │
│   └──────────────────────────────────────────────────┘      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Mutex vs Binary Semaphore

| Aspek | Mutex | Binary Semaphore |
|-------|-------|------------------|
| **Ownership** | Ya - hanya task yang take bisa give | Tidak - siapapun bisa give |
| **Priority Inheritance** | Ya | Tidak |
| **Recursion** | Ada recursive mutex | Tidak bisa |
| **Penggunaan utama** | Protect shared resource | Sinkronisasi task/ISR |
| **ISR** | Tidak boleh di ISR | Bisa di ISR |

### 4.3 Priority Inheritance

```
┌─────────────────────────────────────────────────────────────┐
│              PRIORITY INHERITANCE                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   Tanpa Priority Inheritance (Priority Inversion):          │
│                                                              │
│   High Pri   ████████████████▓▓▓▓▓▓▓▓▓▓▓▓▓████              │
│   Med Pri    ────────────────██████████████────              │
│   Low Pri    ████████████████────────────────██              │
│              ↑               ↑               ↑               │
│              Low takes      High waits      Low releases     │
│              mutex          for mutex       mutex            │
│                             (blocked by                      │
│                              Med Pri!)                       │
│                                                              │
│   Dengan Priority Inheritance:                               │
│                                                              │
│   High Pri   ████████████████▓▓▓████████████                │
│   Med Pri    ────────────────────██████████────              │
│   Low Pri    ████████████████████─────────────              │
│              ↑               ↑   ↑                          │
│              Low takes      High waits, Low inherited       │
│              mutex          Low gets High priority!         │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 4.4 Mutex API

```c
// Membuat mutex
SemaphoreHandle_t xSemaphoreCreateMutex(void);

// Membuat recursive mutex
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);

// Take mutex (sama dengan semaphore)
BaseType_t xSemaphoreTake(
    SemaphoreHandle_t xSemaphore,
    TickType_t xTicksToWait
);

// Give mutex (sama dengan semaphore)
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

// Recursive mutex
BaseType_t xSemaphoreTakeRecursive(
    SemaphoreHandle_t xMutex,
    TickType_t xTicksToWait
);
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xMutex);

// Cek siapa yang memegang mutex
TaskHandle_t xSemaphoreGetMutexHolder(SemaphoreHandle_t xMutex);
```

### 4.5 Contoh: Protecting Shared Resource

```c
SemaphoreHandle_t xUARTMutex;

void vPrintTask(void *pvParameters)
{
    char *taskName = (char *)pvParameters;
    char buffer[64];
    
    for(;;)
    {
        // Ambil mutex sebelum akses UART
        if(xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // Critical section - hanya satu task bisa print
            sprintf(buffer, "Task %s is printing...\n", taskName);
            HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 100);
            
            // Simulate long print operation
            vTaskDelay(pdMS_TO_TICKS(10));
            
            // Lepas mutex
            xSemaphoreGive(xUARTMutex);
        }
        else
        {
            // Timeout - UART busy
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    xUARTMutex = xSemaphoreCreateMutex();
    
    xTaskCreate(vPrintTask, "PrintA", 256, "A", 2, NULL);
    xTaskCreate(vPrintTask, "PrintB", 256, "B", 2, NULL);
    xTaskCreate(vPrintTask, "PrintC", 256, "C", 2, NULL);
    
    vTaskStartScheduler();
}
```

### 4.6 Recursive Mutex

Berguna ketika function yang sudah memegang mutex memanggil function lain yang juga butuh mutex.

```c
SemaphoreHandle_t xRecursiveMutex;

void functionA(void)
{
    xSemaphoreTakeRecursive(xRecursiveMutex, portMAX_DELAY);
    
    // Do something
    functionB();  // Call nested function
    
    xSemaphoreGiveRecursive(xRecursiveMutex);
}

void functionB(void)
{
    // Ini bisa berhasil karena recursive mutex
    xSemaphoreTakeRecursive(xRecursiveMutex, portMAX_DELAY);
    
    // Do something else
    
    xSemaphoreGiveRecursive(xRecursiveMutex);
}

void vTask(void *p)
{
    for(;;)
    {
        functionA();  // Akan take mutex 2x, give 2x
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 4.7 Deadlock dan Cara Menghindari

```
┌─────────────────────────────────────────────────────────────┐
│                     DEADLOCK                                 │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   Task A                    Task B                          │
│      │                         │                            │
│      │ Take Mutex 1            │                            │
│      ▼                         │                            │
│   ┌─────┐                      │ Take Mutex 2               │
│   │ M1  │                      ▼                            │
│   └─────┘                   ┌─────┐                         │
│      │                      │ M2  │                         │
│      │ Try Take Mutex 2     └─────┘                         │
│      │        │                │ Try Take Mutex 1           │
│      ▼        │                │        │                   │
│   BLOCKED ◄───┘                └───► BLOCKED                │
│      │                                  │                   │
│      └──────────── DEADLOCK! ───────────┘                   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

#### Mencegah Deadlock:
```c
// WRONG: Dapat menyebabkan deadlock
void TaskA(void *p) {
    xSemaphoreTake(xMutex1, portMAX_DELAY);
    xSemaphoreTake(xMutex2, portMAX_DELAY);  // Deadlock risk!
    // ...
    xSemaphoreGive(xMutex2);
    xSemaphoreGive(xMutex1);
}

void TaskB(void *p) {
    xSemaphoreTake(xMutex2, portMAX_DELAY);
    xSemaphoreTake(xMutex1, portMAX_DELAY);  // Deadlock risk!
    // ...
    xSemaphoreGive(xMutex1);
    xSemaphoreGive(xMutex2);
}

// CORRECT: Selalu ambil mutex dengan urutan yang sama
void TaskA(void *p) {
    xSemaphoreTake(xMutex1, portMAX_DELAY);  // Always Mutex1 first
    xSemaphoreTake(xMutex2, portMAX_DELAY);
    // ...
    xSemaphoreGive(xMutex2);
    xSemaphoreGive(xMutex1);
}

void TaskB(void *p) {
    xSemaphoreTake(xMutex1, portMAX_DELAY);  // Same order
    xSemaphoreTake(xMutex2, portMAX_DELAY);
    // ...
    xSemaphoreGive(xMutex2);
    xSemaphoreGive(xMutex1);
}

// BETTER: Gunakan timeout
void TaskA(void *p) {
    if(xSemaphoreTake(xMutex1, pdMS_TO_TICKS(100)) == pdTRUE) {
        if(xSemaphoreTake(xMutex2, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Success - do work
            xSemaphoreGive(xMutex2);
        }
        xSemaphoreGive(xMutex1);
    }
    // Handle failure case
}
```

---

## 5. Implementasi pada STM32

### 5.1 Setup FreeRTOS dengan Queue dan Semaphore

#### FreeRTOSConfig.h
```c
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_QUEUE_SETS            1
#define configQUEUE_REGISTRY_SIZE       8
```

#### platformio.ini untuk STM32
```ini
[env:bluepill]
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
build_flags = 
    -D USE_HAL_DRIVER
    -D STM32F103xB
lib_deps = 
    freertos
```

### 5.2 Contoh Lengkap STM32

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32f1xx_hal.h"

// Handles
QueueHandle_t xSensorQueue;
QueueHandle_t xCommandQueue;
SemaphoreHandle_t xUARTMutex;
SemaphoreHandle_t xButtonSemaphore;

// Data structures
typedef struct {
    uint8_t sensorId;
    float value;
    uint32_t timestamp;
} SensorData_t;

typedef struct {
    uint8_t command;
    uint8_t param;
} Command_t;

// Protected print function
void safePrint(const char *msg)
{
    if(xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
        xSemaphoreGive(xUARTMutex);
    }
}

// Sensor Task - Producer
void vSensorTask(void *pvParameters)
{
    SensorData_t data;
    uint8_t sensorId = (uint8_t)(uint32_t)pvParameters;
    
    for(;;)
    {
        data.sensorId = sensorId;
        data.timestamp = xTaskGetTickCount();
        
        // Simulate sensor reading
        data.value = (float)(HAL_ADC_GetValue(&hadc1) * 3.3 / 4095.0);
        
        // Send to queue
        if(xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS)
        {
            char msg[64];
            sprintf(msg, "[Sensor%d] Sent: %.2f\n", sensorId, data.value);
            safePrint(msg);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500 + sensorId * 100));  // Different rates
    }
}

// Process Task - Consumer
void vProcessTask(void *pvParameters)
{
    SensorData_t data;
    float average = 0;
    uint32_t count = 0;
    
    for(;;)
    {
        if(xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdPASS)
        {
            // Calculate running average
            average = (average * count + data.value) / (count + 1);
            count++;
            
            char msg[64];
            sprintf(msg, "[Process] S%d=%.2f, Avg=%.2f\n", 
                    data.sensorId, data.value, average);
            safePrint(msg);
            
            // Check threshold
            if(data.value > 2.5)
            {
                Command_t cmd = {.command = 0x01, .param = data.sensorId};
                xQueueSend(xCommandQueue, &cmd, 0);
            }
        }
    }
}

// Actuator Task
void vActuatorTask(void *pvParameters)
{
    Command_t cmd;
    
    for(;;)
    {
        if(xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY) == pdPASS)
        {
            char msg[64];
            sprintf(msg, "[Actuator] Cmd: 0x%02X, Param: %d\n", 
                    cmd.command, cmd.param);
            safePrint(msg);
            
            switch(cmd.command)
            {
                case 0x01:  // Alert
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
                    vTaskDelay(pdMS_TO_TICKS(200));
                    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                    break;
            }
        }
    }
}

// Button Handler Task
void vButtonTask(void *pvParameters)
{
    for(;;)
    {
        // Wait for button press (signaled from ISR)
        if(xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            safePrint("[Button] Pressed!\n");
            
            // Send command
            Command_t cmd = {.command = 0x02, .param = 0xFF};
            xQueueSend(xCommandQueue, &cmd, 0);
        }
    }
}

// Button ISR
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == BUTTON_Pin)
    {
        xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    
    // Create communication objects
    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    xCommandQueue = xQueueCreate(5, sizeof(Command_t));
    xUARTMutex = xSemaphoreCreateMutex();
    xButtonSemaphore = xSemaphoreCreateBinary();
    
    // Create tasks
    xTaskCreate(vSensorTask, "Sensor1", 256, (void*)1, 2, NULL);
    xTaskCreate(vSensorTask, "Sensor2", 256, (void*)2, 2, NULL);
    xTaskCreate(vProcessTask, "Process", 512, NULL, 3, NULL);
    xTaskCreate(vActuatorTask, "Actuator", 256, NULL, 2, NULL);
    xTaskCreate(vButtonTask, "Button", 128, NULL, 3, NULL);
    
    vTaskStartScheduler();
    
    for(;;);
}
```

---

## 6. Implementasi pada ESP32

### 6.1 ESP32 dengan Dual-Core Queue

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *TAG = "DUAL_CORE";

// Queue handles
QueueHandle_t xSensorQueue;
QueueHandle_t xDisplayQueue;
SemaphoreHandle_t xSerialMutex;
SemaphoreHandle_t xWiFiSemaphore;

// Data structure
typedef struct {
    uint8_t type;
    float value;
    uint32_t timestamp;
} SensorReading_t;

// Protected serial print
void safePrintf(const char* format, ...) {
    if(xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        xSemaphoreGive(xSerialMutex);
    }
}

// Sensor Task (Core 1)
void SensorTask(void *pvParameters) {
    SensorReading_t reading;
    const uint8_t sensorType = (uint8_t)(uint32_t)pvParameters;
    
    for(;;) {
        reading.type = sensorType;
        reading.timestamp = millis();
        
        switch(sensorType) {
            case 0:  // Temperature
                reading.value = temperatureRead();
                break;
            case 1:  // Light
                reading.value = analogRead(34) / 4095.0 * 100;
                break;
            case 2:  // Hall
                reading.value = hallRead();
                break;
        }
        
        // Send to both queues
        xQueueSend(xSensorQueue, &reading, 0);
        xQueueSend(xDisplayQueue, &reading, 0);
        
        safePrintf("[Sensor%d@Core%d] Value: %.2f\n", 
                   sensorType, xPortGetCoreID(), reading.value);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Cloud Task (Core 0) - Heavy WiFi operations
void CloudTask(void *pvParameters) {
    SensorReading_t reading;
    
    for(;;) {
        if(xQueueReceive(xSensorQueue, &reading, pdMS_TO_TICKS(1000)) == pdPASS) {
            // Wait for WiFi semaphore
            if(xSemaphoreTake(xWiFiSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE) {
                safePrintf("[Cloud@Core%d] Uploading S%d=%.2f\n",
                           xPortGetCoreID(), reading.type, reading.value);
                
                // Simulate cloud upload
                vTaskDelay(pdMS_TO_TICKS(100));
                
                xSemaphoreGive(xWiFiSemaphore);
            }
        }
    }
}

// Display Task (Core 1)
void DisplayTask(void *pvParameters) {
    SensorReading_t reading;
    
    for(;;) {
        if(xQueueReceive(xDisplayQueue, &reading, portMAX_DELAY) == pdPASS) {
            safePrintf("[Display@Core%d] S%d: %.2f\n",
                       xPortGetCoreID(), reading.type, reading.value);
        }
    }
}

// Monitor Task
void MonitorTask(void *pvParameters) {
    for(;;) {
        safePrintf("\n=== System Monitor ===\n");
        safePrintf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        safePrintf("Sensor Queue: %d/%d\n", 
                   uxQueueMessagesWaiting(xSensorQueue), 20);
        safePrintf("Display Queue: %d/%d\n",
                   uxQueueMessagesWaiting(xDisplayQueue), 10);
        safePrintf("========================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    // Create queues
    xSensorQueue = xQueueCreate(20, sizeof(SensorReading_t));
    xDisplayQueue = xQueueCreate(10, sizeof(SensorReading_t));
    
    // Create semaphores
    xSerialMutex = xSemaphoreCreateMutex();
    xWiFiSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xWiFiSemaphore);  // Initially available
    
    // Create sensor tasks on Core 1
    xTaskCreatePinnedToCore(SensorTask, "TempSensor", 4096, (void*)0, 2, NULL, 1);
    xTaskCreatePinnedToCore(SensorTask, "LightSensor", 4096, (void*)1, 2, NULL, 1);
    xTaskCreatePinnedToCore(SensorTask, "HallSensor", 4096, (void*)2, 2, NULL, 1);
    
    // Create cloud task on Core 0 (WiFi runs on Core 0)
    xTaskCreatePinnedToCore(CloudTask, "Cloud", 8192, NULL, 2, NULL, 0);
    
    // Create display task on Core 1
    xTaskCreatePinnedToCore(DisplayTask, "Display", 4096, NULL, 1, NULL, 1);
    
    // Create monitor task
    xTaskCreatePinnedToCore(MonitorTask, "Monitor", 4096, NULL, 1, NULL, 0);
    
    ESP_LOGI(TAG, "System started!");
}
```

### 6.2 ESP32 Queue Set

Queue set memungkinkan satu task menunggu multiple queues/semaphores.

```cpp
#include "freertos/queue.h"

QueueSetHandle_t xQueueSet;
QueueHandle_t xQueue1, xQueue2;
SemaphoreHandle_t xSemaphore;

void app_main(void) {
    // Create queue set yang bisa hold 20 items total
    xQueueSet = xQueueCreateSet(20);
    
    // Create and add queues/semaphores
    xQueue1 = xQueueCreate(10, sizeof(int));
    xQueue2 = xQueueCreate(5, sizeof(float));
    xSemaphore = xSemaphoreCreateBinary();
    
    xQueueAddToSet(xQueue1, xQueueSet);
    xQueueAddToSet(xQueue2, xQueueSet);
    xQueueAddToSet(xSemaphore, xQueueSet);
    
    xTaskCreate(MultiplexerTask, "Mux", 4096, NULL, 2, NULL);
}

void MultiplexerTask(void *p) {
    QueueSetMemberHandle_t xActivatedMember;
    
    for(;;) {
        // Block until something is available in any queue/semaphore
        xActivatedMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
        
        if(xActivatedMember == xQueue1) {
            int value;
            xQueueReceive(xQueue1, &value, 0);
            printf("Queue1: %d\n", value);
        }
        else if(xActivatedMember == xQueue2) {
            float value;
            xQueueReceive(xQueue2, &value, 0);
            printf("Queue2: %.2f\n", value);
        }
        else if(xActivatedMember == xSemaphore) {
            xSemaphoreTake(xSemaphore, 0);
            printf("Semaphore triggered!\n");
        }
    }
}
```

---

## 7. Best Practices

### 7.1 Queue Best Practices

```c
// DO: Gunakan timeout yang wajar
if(xQueueSend(xQueue, &data, pdMS_TO_TICKS(100)) != pdPASS) {
    // Handle queue full
    handleError();
}

// DON'T: Block forever tanpa error handling
xQueueSend(xQueue, &data, portMAX_DELAY);  // Bisa stuck selamanya

// DO: Cek status queue
if(uxQueueSpacesAvailable(xQueue) < 2) {
    // Queue almost full, take action
}

// DO: Pass by value untuk data kecil
typedef struct { int x; int y; } Point_t;
xQueueSend(xQueue, &point, 0);  // Copy entire struct

// DO: Pass by pointer untuk data besar
typedef struct { char data[1024]; } BigData_t;
BigData_t *ptr = malloc(sizeof(BigData_t));
xQueueSend(xQueue, &ptr, 0);  // Send pointer only
// Receiver harus free()!
```

### 7.2 Semaphore Best Practices

```c
// DO: Selalu cek return value
if(xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Critical section
    xSemaphoreGive(xMutex);
}

// DON'T: Forget to give back
xSemaphoreTake(xMutex, portMAX_DELAY);
// ... do something
// OOPS! Forgot xSemaphoreGive()

// DO: Keep critical section short
xSemaphoreTake(xMutex, portMAX_DELAY);
localCopy = sharedData;  // Quick copy
xSemaphoreGive(xMutex);
processData(localCopy);  // Process outside critical section

// DON'T: Long operations in critical section
xSemaphoreTake(xMutex, portMAX_DELAY);
sendToCloud(sharedData);  // Blocking operation - BAD!
xSemaphoreGive(xMutex);
```

### 7.3 Mutex Best Practices

```c
// DO: Consistent lock ordering
// Task A and Task B both do:
xSemaphoreTake(xMutex1, portMAX_DELAY);
xSemaphoreTake(xMutex2, portMAX_DELAY);
// ... critical section
xSemaphoreGive(xMutex2);
xSemaphoreGive(xMutex1);

// DO: Use wrapper functions
void writeToSharedResource(int value) {
    if(xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sharedResource = value;
        xSemaphoreGive(xMutex);
    }
}

// DON'T: Use mutex from ISR
void ISR_Handler(void) {
    xSemaphoreTake(xMutex, 0);  // WRONG! Mutex in ISR
}

// DO: Use critical section for quick ISR protection
taskENTER_CRITICAL();
sharedVar++;
taskEXIT_CRITICAL();
```

### 7.4 Design Patterns

#### Producer-Consumer Pattern
```c
// Single producer, single consumer
void Producer(void *p) {
    Data_t data;
    for(;;) {
        produceData(&data);
        xQueueSend(xQueue, &data, portMAX_DELAY);
    }
}

void Consumer(void *p) {
    Data_t data;
    for(;;) {
        xQueueReceive(xQueue, &data, portMAX_DELAY);
        consumeData(&data);
    }
}
```

#### Reader-Writer Pattern
```c
SemaphoreHandle_t xReaderCount;
SemaphoreHandle_t xWriteMutex;
int readerCount = 0;

void Reader(void *p) {
    // Entry
    xSemaphoreTake(xReaderCount, portMAX_DELAY);
    if(++readerCount == 1)
        xSemaphoreTake(xWriteMutex, portMAX_DELAY);
    xSemaphoreGive(xReaderCount);
    
    // Read data (multiple readers allowed)
    readData();
    
    // Exit
    xSemaphoreTake(xReaderCount, portMAX_DELAY);
    if(--readerCount == 0)
        xSemaphoreGive(xWriteMutex);
    xSemaphoreGive(xReaderCount);
}

void Writer(void *p) {
    xSemaphoreTake(xWriteMutex, portMAX_DELAY);
    writeData();  // Exclusive access
    xSemaphoreGive(xWriteMutex);
}
```

---

## 8. Debugging dan Troubleshooting

### 8.1 Common Issues

| Masalah | Gejala | Solusi |
|---------|--------|--------|
| Queue full | Data loss, timeout | Increase queue size atau kurangi production rate |
| Deadlock | System hang | Consistent lock ordering, use timeout |
| Priority inversion | Low priority task blocking high | Use mutex (bukan binary semaphore) |
| Semaphore leak | Resource starvation | Always give after take |
| Stack overflow | Random crash | Increase stack, check HWM |

### 8.2 Debug Techniques

```c
// Monitor queue status
void vMonitorTask(void *p) {
    for(;;) {
        printf("Queue status:\n");
        printf("  Messages waiting: %d\n", 
               uxQueueMessagesWaiting(xQueue));
        printf("  Spaces available: %d\n",
               uxQueueSpacesAvailable(xQueue));
        
        printf("Mutex holder: %s\n",
               pcTaskGetName(xSemaphoreGetMutexHolder(xMutex)));
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Queue trace hooks (in FreeRTOSConfig.h)
#define traceBLOCKING_ON_QUEUE_RECEIVE(pxQueue) \
    printf("Task blocked on queue receive\n")

#define traceBLOCKING_ON_QUEUE_SEND(pxQueue) \
    printf("Task blocked on queue send\n")
```

### 8.3 Visual Debugging

```
Queue Status Visualization:

[===-------]  30% full (3/10 items)
     ↑
   Head (oldest)

Semaphore Status:
Binary: [■] Taken  [□] Available
Counting: [■■■□□] 3/5 taken

Mutex Chain:
TaskA → Mutex1 → TaskB (waiting)
        └→ Mutex2 → TaskC (waiting)
```

---

## 📝 Ringkasan

| Konsep | Kapan Digunakan | API Utama |
|--------|-----------------|-----------|
| **Queue** | Transfer data antar task | `xQueueCreate`, `xQueueSend`, `xQueueReceive` |
| **Binary Semaphore** | Sinkronisasi ISR-to-Task | `xSemaphoreCreateBinary`, `xSemaphoreGive`, `xSemaphoreTake` |
| **Counting Semaphore** | Resource counting | `xSemaphoreCreateCounting` |
| **Mutex** | Protect shared resource | `xSemaphoreCreateMutex` |
| **Event Groups** | Multiple event sync | `xEventGroupCreate`, `xEventGroupSetBits`, `xEventGroupWaitBits` |

---

## 📚 Referensi
1. FreeRTOS Queue API: https://freertos.org/a00018.html
2. FreeRTOS Semaphore API: https://freertos.org/a00113.html
3. FreeRTOS Mutex: https://freertos.org/Real-time-embedded-RTOS-mutexes.html
4. Mastering the FreeRTOS Real Time Kernel - Richard Barry
