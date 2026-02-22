# Referensi Modul 10: FreeRTOS Queue dan Semaphore

## 📚 Dokumentasi Resmi

### FreeRTOS Documentation
| Resource | Link | Deskripsi |
|----------|------|-----------|
| Queue API | https://www.freertos.org/Embedded-RTOS-Queues.html | Dokumentasi lengkap Queue |
| Semaphore API | https://www.freertos.org/Embedded-RTOS-Binary-Semaphores.html | Binary Semaphore guide |
| Mutex API | https://www.freertos.org/Real-time-embedded-RTOS-mutexes.html | Mutex documentation |
| Counting Semaphore | https://www.freertos.org/Real-time-embedded-RTOS-Counting-Semaphores.html | Resource pool management |

### STM32 Documentation
| Resource | Link |
|----------|------|
| STM32F1 HAL Reference | https://www.st.com/resource/en/user_manual/um1850.pdf |
| STM32F103 Reference Manual | https://www.st.com/resource/en/reference_manual/rm0008.pdf |
| FreeRTOS on STM32 | https://www.freertos.org/FreeRTOS-for-STM32F4xx-Cortex-M4F-IAR.html |

### ESP32 Documentation
| Resource | Link |
|----------|------|
| ESP-IDF FreeRTOS | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html |
| ESP32 Technical Reference | https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf |

---

## 📺 Video Tutorial

### Queue Concepts
1. **FreeRTOS Queue Tutorial** - Digi-Key Electronics
   - https://www.youtube.com/watch?v=4bRMc1FLJYc
   - Durasi: ~20 menit
   
2. **Inter-Task Communication with Queues** - Controllers Tech
   - https://www.youtube.com/watch?v=yIQPJbKWTbY
   - Durasi: ~15 menit

### Semaphore & Mutex
3. **Binary Semaphore Tutorial** - Shawn Hymel
   - https://www.youtube.com/watch?v=5JcMtbA9QEE
   - Durasi: ~25 menit
   
4. **Mutex vs Semaphore Explained** - Controllers Tech
   - https://www.youtube.com/watch?v=XNH_xQpJLIo
   - Durasi: ~18 menit

5. **Deadlock Prevention Strategies** - Programming Electronics Academy
   - Durasi: ~12 menit

### Platform Specific
6. **STM32 FreeRTOS Queue Example** - Phil's Lab
   - Durasi: ~30 menit
   
7. **ESP32 Dual Core with Queue** - Random Nerd Tutorials
   - https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
   - Durasi: ~20 menit

---

## 📖 Artikel dan Tutorial Online

### Inter-Task Communication
1. **Mastering the FreeRTOS Kernel - Queue Chapter**
   - https://www.freertos.org/Documentation/RTOS_book.html
   
2. **Understanding FreeRTOS Queues**
   - https://www.digikey.com/en/maker/projects/introduction-to-freertos-queues

3. **Producer-Consumer Pattern in FreeRTOS**
   - Best practices untuk data pipeline

### Synchronization Primitives
4. **Semaphore Types Comparison**
   - Binary vs Counting vs Mutex guide
   
5. **Priority Inversion and Priority Inheritance**
   - https://www.freertos.org/Real-time-embedded-RTOS-mutexes.html
   
6. **Deadlock Prevention Techniques**
   - Lock ordering, timeout strategies

### Platform Guides
7. **STM32 with FreeRTOS Complete Guide**
   - https://deepbluembedded.com/stm32-freertos-tutorials/
   
8. **ESP32 FreeRTOS Examples**
   - https://github.com/espressif/esp-idf/tree/master/examples/system/freertos

---

## 🔧 API Quick Reference

### Queue Functions
```c
// Create queue
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);

// Send to queue
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);

// Receive from queue
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
BaseType_t xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);

// ISR versions
BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void *pvBuffer, BaseType_t *pxHigherPriorityTaskWoken);

// Status functions
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);
UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue);

// Delete
void vQueueDelete(QueueHandle_t xQueue);
```

### Semaphore Functions
```c
// Binary Semaphore
SemaphoreHandle_t xSemaphoreCreateBinary(void);

// Counting Semaphore
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount);

// Mutex
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);

// Take (acquire)
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xMutex, TickType_t xTicksToWait);

// Give (release)
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xMutex);

// ISR versions
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken);

// Status
UBaseType_t uxSemaphoreGetCount(SemaphoreHandle_t xSemaphore);
TaskHandle_t xSemaphoreGetMutexHolder(SemaphoreHandle_t xMutex);
```

### Event Group Functions
```c
// Create
EventGroupHandle_t xEventGroupCreate(void);

// Set bits
EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet);
BaseType_t xEventGroupSetBitsFromISR(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet, BaseType_t *pxHigherPriorityTaskWoken);

// Wait for bits
EventBits_t xEventGroupWaitBits(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToWaitFor,
    const BaseType_t xClearOnExit,
    const BaseType_t xWaitForAllBits,
    TickType_t xTicksToWait);

// Clear bits
EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear);

// Get bits
EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup);
```

---

## 📊 Comparison Tables

### Queue vs Semaphore vs Mutex

| Feature | Queue | Binary Sema | Counting Sema | Mutex |
|---------|-------|-------------|---------------|-------|
| Data transfer | ✅ | ❌ | ❌ | ❌ |
| Synchronization | ✅ | ✅ | ✅ | ✅ |
| ISR safe | ✅ | ✅ | ✅ | ❌ |
| Priority inheritance | ❌ | ❌ | ❌ | ✅ |
| Ownership | ❌ | ❌ | ❌ | ✅ |
| Max count | N items | 1 | N | 1 |

### When to Use What

| Scenario | Use |
|----------|-----|
| Transfer sensor data to display task | Queue |
| Button ISR triggers task processing | Binary Semaphore |
| Manage pool of 3 database connections | Counting Semaphore (3) |
| Protect shared UART from multiple tasks | Mutex |
| Wait for multiple events to complete | Event Group |

---

## 💻 GitHub Repositories

### Learning Examples
1. **FreeRTOS Official Examples**
   - https://github.com/FreeRTOS/FreeRTOS
   
2. **STM32 FreeRTOS Examples**
   - https://github.com/STMicroelectronics/STM32CubeF1/tree/master/Projects

3. **ESP32 FreeRTOS Examples**
   - https://github.com/espressif/esp-idf/tree/master/examples/system

### Project Templates
4. **PlatformIO STM32 FreeRTOS Template**
   - Board configurations and build scripts
   
5. **ESP32 Multi-Task Template**
   - Dual-core task distribution examples

---

## 📝 Common Patterns

### Producer-Consumer
```c
// Producer
void producer_task(void *pvParam) {
    Data_t data;
    for(;;) {
        data = collect_data();
        xQueueSend(xQueue, &data, portMAX_DELAY);
    }
}

// Consumer
void consumer_task(void *pvParam) {
    Data_t data;
    for(;;) {
        xQueueReceive(xQueue, &data, portMAX_DELAY);
        process(data);
    }
}
```

### ISR-to-Task Deferred Processing
```c
SemaphoreHandle_t xButtonSemaphore;

void ISR_Handler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void handler_task(void *pvParam) {
    for(;;) {
        xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);
        // Handle button press
    }
}
```

### Safe Resource Access
```c
SemaphoreHandle_t xResourceMutex;

void safe_access_task(void *pvParam) {
    for(;;) {
        if(xSemaphoreTake(xResourceMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Access shared resource
            access_resource();
            xSemaphoreGive(xResourceMutex);
        } else {
            // Handle timeout
        }
    }
}
```

---

## 🔍 Debugging Tools

### FreeRTOS Trace Tools
1. **SEGGER SystemView** - Real-time trace visualization
2. **Percepio Tracealyzer** - Advanced RTOS tracing

### Built-in Debugging
```c
// In FreeRTOSConfig.h
#define configUSE_TRACE_FACILITY            1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1
#define configCHECK_FOR_STACK_OVERFLOW      2
#define configUSE_MALLOC_FAILED_HOOK        1

// Runtime stats
void vTaskGetRunTimeStats(char *pcWriteBuffer);

// Queue monitoring
uxQueueMessagesWaiting(xQueue);
uxQueueSpacesAvailable(xQueue);
```

---

## 📚 Books

1. **Mastering the FreeRTOS Real Time Kernel**
   - Richard Barry (FreeRTOS author)
   - Free PDF: https://www.freertos.org/Documentation/RTOS_book.html

2. **Beginning STM32: Developing with FreeRTOS**
   - Warren Gay
   - Apress publication

3. **Embedded Systems Programming with C and GNU Development Tools**
   - Michael Barr & Anthony Massa

---

## ⚠️ Common Pitfalls

| Mistake | Solution |
|---------|----------|
| Forgetting xSemaphoreGive() | Always pair Take with Give |
| Using Mutex from ISR | Use Binary Semaphore for ISR→Task |
| Ignoring xQueueSend() return | Always check for pdPASS |
| portMAX_DELAY everywhere | Use timeouts for fault tolerance |
| Different lock ordering | Document and enforce lock order |
| Too small stack size | Use uxTaskGetStackHighWaterMark() |

---

*Referensi Modul 10 - FreeRTOS Queue dan Semaphore*
