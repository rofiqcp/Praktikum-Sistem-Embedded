# Modul 11: FreeRTOS — Software Timer dan Task Notification

## 📚 Daftar Isi
1. [Pendahuluan](#1-pendahuluan)
2. [Software Timer](#2-software-timer)
3. [Timer API](#3-timer-api)
4. [Task Notification](#4-task-notification)
5. [Notification API](#5-notification-api)
6. [Implementasi STM32](#6-implementasi-stm32)
7. [Implementasi ESP32](#7-implementasi-esp32)
8. [Use Cases dan Patterns](#8-use-cases-dan-patterns)
9. [Best Practices](#9-best-practices)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Pendahuluan

### 1.1 Apa yang Dipelajari
Modul ini membahas dua fitur FreeRTOS yang sangat berguna:
- **Software Timer**: Menjalankan fungsi secara periodik atau one-shot tanpa task
- **Task Notification**: Komunikasi ringan dan cepat untuk wake-up task

### 1.2 Kenapa Penting?

| Fitur | Keunggulan |
|-------|------------|
| Software Timer | Tidak perlu task dedicated, hemat stack memory |
| Task Notification | 45% lebih cepat dari binary semaphore |
| Kombinasi | Event-driven architecture yang efisien |

### 1.3 Prerequisite
- Memahami FreeRTOS task dan scheduler
- Familiar dengan Queue dan Semaphore (Modul 10)
- Pengalaman dengan interrupt handling

---

## 2. Software Timer

### 2.1 Konsep Dasar

Software Timer adalah mekanisme untuk menjalankan fungsi (callback) setelah periode waktu tertentu, tanpa memerlukan task terpisah.

```
┌─────────────────────────────────────────────────────────┐
│                    TIMER SERVICE TASK                    │
│    (daemon task yang mengelola semua software timer)    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Timer 1 ──→ Callback 1 (setiap 100ms)                │
│  Timer 2 ──→ Callback 2 (setiap 500ms)                │
│  Timer 3 ──→ Callback 3 (one-shot 2s)                 │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 2.2 Dua Jenis Timer

#### One-Shot Timer
Berjalan sekali lalu berhenti.
```
Start
  │
  ├───[Periode]───→ Callback() ───→ STOP
```

**Use case**: Timeout handler, debounce, delayed action

#### Auto-Reload (Periodic) Timer
Berjalan berulang dengan interval tetap.
```
Start
  │
  ├───[Periode]───→ Callback() ─┐
  │                             │
  └─────────────────────────────┘ (repeat)
```

**Use case**: LED blink, sensor polling, watchdog refresh

### 2.3 Timer Daemon Task

FreeRTOS menggunakan **timer service task (daemon)** untuk mengelola semua software timer:

- Dibuat otomatis saat scheduler start (jika ada timer)
- Priority dan stack size dikonfigurasi di FreeRTOSConfig.h
- Timer command queue untuk komunikasi dengan daemon

```c
// FreeRTOSConfig.h
#define configUSE_TIMERS                  1
#define configTIMER_TASK_PRIORITY         2
#define configTIMER_QUEUE_LENGTH          10
#define configTIMER_TASK_STACK_DEPTH      256
```

### 2.4 Timer States

```
                    ┌─────────┐
    xTimerCreate()  │ Dormant │
         ┌─────────→│(Created)│←──────────┐
         │          └────┬────┘           │
         │               │                │
         │         xTimerStart()          │xTimerStop()
         │               │                │
         │               ▼                │
         │          ┌─────────┐           │
         │          │ Running │───────────┘
         │          └────┬────┘
         │               │
         │           Expired
         │               │
         │               ▼
         │          ┌─────────┐
         └──────────│Callback │ (One-shot: return to Dormant)
                    │ Execute │ (Auto-reload: restart Running)
                    └─────────┘
```

---

## 3. Timer API

### 3.1 Membuat Timer

```c
TimerHandle_t xTimerCreate(
    const char * const pcTimerName,    // Nama timer (debugging)
    TickType_t xTimerPeriodInTicks,    // Periode dalam ticks
    UBaseType_t uxAutoReload,          // pdTRUE=periodic, pdFALSE=one-shot
    void * pvTimerID,                  // ID/parameter
    TimerCallbackFunction_t pxCallbackFunction  // Callback function
);
```

**Contoh:**
```c
// Timer periodic setiap 500ms
TimerHandle_t xLedTimer = xTimerCreate(
    "LED_Timer",
    pdMS_TO_TICKS(500),
    pdTRUE,                   // Auto-reload
    (void*)0,                 // ID
    vLedTimerCallback
);

// Timer one-shot 3 detik
TimerHandle_t xTimeoutTimer = xTimerCreate(
    "Timeout",
    pdMS_TO_TICKS(3000),
    pdFALSE,                  // One-shot
    (void*)0,
    vTimeoutCallback
);
```

### 3.2 Callback Function

```c
void vTimerCallback(TimerHandle_t xTimer)
{
    // JANGAN blocking atau delay lama!
    // Callback berjalan di context timer daemon task
    
    // Get timer ID jika diperlukan
    uint32_t timerId = (uint32_t)pvTimerGetTimerID(xTimer);
    
    // Lakukan aksi singkat
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
}
```

⚠️ **PENTING**: Callback TIDAK BOLEH:
- Memanggil blocking API (xQueueReceive dengan wait, vTaskDelay, dll)
- Melakukan operasi yang lama
- Memanggil API yang bisa menyebabkan yield

### 3.3 Mengontrol Timer

```c
// Start timer
BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);

// Stop timer  
BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);

// Reset timer (restart periode)
BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait);

// Change period
BaseType_t xTimerChangePeriod(TimerHandle_t xTimer, 
                              TickType_t xNewPeriod, 
                              TickType_t xTicksToWait);

// Delete timer
BaseType_t xTimerDelete(TimerHandle_t xTimer, TickType_t xTicksToWait);
```

**Contoh penggunaan:**
```c
// Start timer dengan timeout 100ms untuk command queue
if(xTimerStart(xLedTimer, pdMS_TO_TICKS(100)) == pdPASS) {
    printf("Timer started!\n");
}

// Stop timer
xTimerStop(xLedTimer, pdMS_TO_TICKS(100));

// Reset timer (untuk debounce: reset setiap ada input)
xTimerReset(xDebounceTimer, 0);

// Change period dynamically
xTimerChangePeriod(xLedTimer, pdMS_TO_TICKS(200), pdMS_TO_TICKS(100));
```

### 3.4 Timer API dari ISR

```c
BaseType_t xTimerStartFromISR(TimerHandle_t xTimer, 
                               BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xTimerStopFromISR(TimerHandle_t xTimer, 
                              BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xTimerResetFromISR(TimerHandle_t xTimer, 
                               BaseType_t *pxHigherPriorityTaskWoken);
```

**Contoh di ISR:**
```c
void EXTI0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Reset debounce timer
    xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

### 3.5 Query Timer State

```c
// Check if timer is active
BaseType_t xTimerIsTimerActive(TimerHandle_t xTimer);

// Get timer name
const char * pcTimerGetName(TimerHandle_t xTimer);

// Get/Set timer ID
void *pvTimerGetTimerID(TimerHandle_t xTimer);
void vTimerSetTimerID(TimerHandle_t xTimer, void *pvNewID);

// Get timer period
TickType_t xTimerGetPeriod(TimerHandle_t xTimer);

// Get expiry time
TickType_t xTimerGetExpiryTime(TimerHandle_t xTimer);
```

---

## 4. Task Notification

### 4.1 Konsep Dasar

Task Notification adalah mekanisme komunikasi **point-to-point** yang sangat efisien:
- Setiap task memiliki 32-bit notification value bawaan
- Lebih cepat dari semaphore/queue (tidak perlu alokasi terpisah)
- Bisa digunakan untuk signaling, counting, atau event bits

```
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
```

### 4.2 Keunggulan vs Alternatif

| Fitur | Binary Semaphore | Task Notification |
|-------|------------------|-------------------|
| RAM usage | +76 bytes | 0 (built-in) |
| Speed | Normal | 45% faster |
| Many-to-one | ✅ | ✅ |
| One-to-many | ✅ | ❌ |
| Broadcast | ✅ | ❌ |
| Data transfer | ❌ | ✅ (32-bit) |

**Gunakan Task Notification jika:**
- Komunikasi point-to-point
- Hanya satu task yang menerima notification
- Kecepatan dan efisiensi penting

### 4.3 Notification Actions

```c
typedef enum {
    eNoAction = 0,              // Hanya wake task, tidak ubah value
    eSetBits,                   // Value |= (notification value)
    eIncrement,                 // Value++
    eSetValueWithOverwrite,     // Value = notification value (selalu)
    eSetValueWithoutOverwrite   // Value = notification value (jika pending clear)
} eNotifyAction;
```

---

## 5. Notification API

### 5.1 Mengirim Notification

#### Simple Give (seperti semaphore give)
```c
BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);
```

#### Full Control
```c
BaseType_t xTaskNotify(
    TaskHandle_t xTaskToNotify,
    uint32_t ulValue,
    eNotifyAction eAction
);
```

#### Contoh:
```c
// Simple give - increment notification value
xTaskNotifyGive(xReceiverTask);

// Set specific bits
xTaskNotify(xReceiverTask, (1 << EVENT_BUTTON), eSetBits);

// Send value with overwrite
xTaskNotify(xReceiverTask, sensorReading, eSetValueWithOverwrite);
```

### 5.2 Menerima Notification

#### Simple Take (seperti semaphore take)
```c
uint32_t ulTaskNotifyTake(
    BaseType_t xClearCountOnExit,   // pdTRUE=clear, pdFALSE=decrement
    TickType_t xTicksToWait
);
```

#### Full Control
```c
BaseType_t xTaskNotifyWait(
    uint32_t ulBitsToClearOnEntry,  // Bits to clear when entering
    uint32_t ulBitsToClearOnExit,   // Bits to clear when exiting
    uint32_t *pulNotificationValue, // Receive the notification value
    TickType_t xTicksToWait
);
```

#### Contoh:
```c
// Simple take - binary semaphore style
void vReceiverTask(void *pvParam)
{
    for(;;)
    {
        // Wait for notification, clear count on exit
        uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("Received %lu notifications\n", count);
    }
}

// Event bits style
void vEventTask(void *pvParam)
{
    uint32_t notification;
    
    for(;;)
    {
        // Wait for any bit, clear on exit
        xTaskNotifyWait(0, 0xFFFFFFFF, &notification, portMAX_DELAY);
        
        if(notification & (1 << EVENT_BUTTON))
        {
            printf("Button event!\n");
        }
        if(notification & (1 << EVENT_TIMER))
        {
            printf("Timer event!\n");
        }
    }
}
```

### 5.3 Notification dari ISR

```c
void vTaskNotifyGiveFromISR(
    TaskHandle_t xTaskToNotify,
    BaseType_t *pxHigherPriorityTaskWoken
);

BaseType_t xTaskNotifyFromISR(
    TaskHandle_t xTaskToNotify,
    uint32_t ulValue,
    eNotifyAction eAction,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

**Contoh di ISR:**
```c
TaskHandle_t xButtonTask;

void EXTI0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Simple give
    vTaskNotifyGiveFromISR(xButtonTask, &xHigherPriorityTaskWoken);
    
    // Or with event bits
    // xTaskNotifyFromISR(xButtonTask, BUTTON_EVENT_BIT, eSetBits, 
    //                    &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}
```

### 5.4 Query Notification State

```c
// Check if task has pending notification
BaseType_t xTaskNotifyStateClear(TaskHandle_t xTask);

// Get notification value without waiting
uint32_t ulTaskNotifyValueClear(TaskHandle_t xTask, uint32_t ulBitsToClear);
```

---

## 6. Implementasi STM32

### 6.1 Konfigurasi Timer

```c
// FreeRTOSConfig.h
#define configUSE_TIMERS                  1
#define configTIMER_TASK_PRIORITY         (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH          10
#define configTIMER_TASK_STACK_DEPTH      (configMINIMAL_STACK_SIZE * 2)
```

### 6.2 Contoh Lengkap - LED Blink dengan Timer

```c
/* main.c - Software Timer LED Blink STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

// Timer handles
TimerHandle_t xLedTimer;
TimerHandle_t xStatusTimer;

// Timer callback - LED blink
void vLedTimerCallback(TimerHandle_t xTimer)
{
    static uint32_t count = 0;
    count++;
    
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
    printf("[Timer] LED toggle #%lu\r\n", count);
}

// Timer callback - Status report
void vStatusTimerCallback(TimerHandle_t xTimer)
{
    printf("\n--- System Status ---\r\n");
    printf("LED Timer active: %s\r\n", 
           xTimerIsTimerActive(xLedTimer) ? "Yes" : "No");
    printf("Free heap: %lu\r\n", xPortGetFreeHeapSize());
    printf("---------------------\r\n\n");
}

// Control task - Mengontrol timer berdasarkan input
void vControlTask(void *pvParameters)
{
    uint8_t rxChar;
    
    printf("Commands: s=start, p=stop, f=fast, l=slow\r\n");
    
    for(;;)
    {
        if(HAL_UART_Receive(&huart1, &rxChar, 1, 100) == HAL_OK)
        {
            switch(rxChar)
            {
                case 's':
                    xTimerStart(xLedTimer, pdMS_TO_TICKS(100));
                    printf("Timer started\r\n");
                    break;
                    
                case 'p':
                    xTimerStop(xLedTimer, pdMS_TO_TICKS(100));
                    printf("Timer stopped\r\n");
                    break;
                    
                case 'f':
                    xTimerChangePeriod(xLedTimer, pdMS_TO_TICKS(100), 
                                       pdMS_TO_TICKS(100));
                    printf("Fast mode (100ms)\r\n");
                    break;
                    
                case 'l':
                    xTimerChangePeriod(xLedTimer, pdMS_TO_TICKS(1000), 
                                       pdMS_TO_TICKS(100));
                    printf("Slow mode (1000ms)\r\n");
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n=== Software Timer Demo ===\r\n");
    
    // Create periodic timer for LED
    xLedTimer = xTimerCreate(
        "LED",
        pdMS_TO_TICKS(500),
        pdTRUE,           // Auto-reload
        (void*)0,
        vLedTimerCallback
    );
    
    // Create periodic timer for status
    xStatusTimer = xTimerCreate(
        "Status",
        pdMS_TO_TICKS(5000),
        pdTRUE,
        (void*)0,
        vStatusTimerCallback
    );
    
    if(xLedTimer != NULL && xStatusTimer != NULL)
    {
        // Start both timers
        xTimerStart(xLedTimer, 0);
        xTimerStart(xStatusTimer, 0);
        
        // Create control task
        xTaskCreate(vControlTask, "Control", 256, NULL, 2, NULL);
        
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

### 6.3 Contoh Task Notification - Button Handler

```c
/* main.c - Task Notification Button Handler STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

TaskHandle_t xButtonTask = NULL;
volatile uint32_t isrCount = 0;

// Button ISR
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == GPIO_PIN_0)
    {
        isrCount++;
        
        // Notify the button task
        vTaskNotifyGiveFromISR(xButtonTask, &xHigherPriorityTaskWoken);
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Button handler task
void vButtonTask(void *pvParameters)
{
    uint32_t notifications;
    uint32_t processedCount = 0;
    
    for(;;)
    {
        // Wait for notification (clear count on exit)
        notifications = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Process all pending notifications
        processedCount += notifications;
        
        printf("[Button] %lu new press(es), total: %lu\r\n", 
               notifications, processedCount);
        
        // Blink LED for each press
        for(uint32_t i = 0; i < notifications; i++)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// Monitor task
void vMonitorTask(void *pvParameters)
{
    for(;;)
    {
        printf("\n[Monitor] ISR count: %lu\r\n", isrCount);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n=== Task Notification Demo ===\r\n");
    printf("Press button to trigger notification\r\n\n");
    
    xTaskCreate(vButtonTask, "Button", 256, NULL, 3, &xButtonTask);
    xTaskCreate(vMonitorTask, "Monitor", 128, NULL, 1, NULL);
    
    vTaskStartScheduler();
    
    for(;;);
}
```

---

## 7. Implementasi ESP32

### 7.1 Software Timer ESP32

```c
/* main.c - Software Timer ESP32 (ESP-IDF) */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "SW_TIMER";

#define LED_PIN   GPIO_NUM_4

TimerHandle_t xLedTimer;
TimerHandle_t xOneShotTimer;

// Periodic timer callback
void vLedTimerCallback(TimerHandle_t xTimer) {
    static int ledState = 0;
    ledState = !ledState;
    gpio_set_level(LED_PIN, ledState);
    printf("[Timer@%lld] LED %s\n", esp_timer_get_time()/1000, ledState ? "ON" : "OFF");
}

// One-shot timer callback
void vOneShotCallback(TimerHandle_t xTimer) {
    printf("[OneShot] Timer expired!\n");
    
    // Do something once
    for(int i = 0; i < 5; i++) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Control task
void ControlTask(void *pvParameters) {
    uint8_t cmd;
    for(;;) {
        if(uart_read_bytes(UART_NUM_0, &cmd, 1, pdMS_TO_TICKS(10)) > 0) {
            switch((char)cmd) {
                case '1':
                    xTimerStart(xLedTimer, pdMS_TO_TICKS(100));
                    printf("Periodic timer started\n");
                    break;
                    
                case '2':
                    xTimerStop(xLedTimer, pdMS_TO_TICKS(100));
                    printf("Periodic timer stopped\n");
                    break;
                    
                case '3':
                    xTimerStart(xOneShotTimer, pdMS_TO_TICKS(100));
                    printf("One-shot timer started (3s)\n");
                    break;
                    
                case 'f':
                    xTimerChangePeriod(xLedTimer, pdMS_TO_TICKS(100), 
                                       pdMS_TO_TICKS(100));
                    printf("Fast: 100ms period\n");
                    break;
                    
                case 's':
                    xTimerChangePeriod(xLedTimer, pdMS_TO_TICKS(1000), 
                                       pdMS_TO_TICKS(100));
                    printf("Slow: 1000ms period\n");
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    printf("\n=== ESP32 Software Timer Demo ===\n");
    printf("Commands: 1=start periodic, 2=stop, 3=one-shot\n");
    printf("         f=fast, s=slow\n\n");
    
    // Create periodic timer
    xLedTimer = xTimerCreate(
        "LED",
        pdMS_TO_TICKS(500),
        pdTRUE,
        NULL,
        vLedTimerCallback
    );
    
    // Create one-shot timer
    xOneShotTimer = xTimerCreate(
        "OneShot",
        pdMS_TO_TICKS(3000),
        pdFALSE,
        NULL,
        vOneShotCallback
    );
    
    xTaskCreate(ControlTask, "Control", 4096, NULL, 2, NULL);
}
```

### 7.2 Task Notification ESP32

```c
/* main.c - Task Notification ESP32 (ESP-IDF) */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "TASK_NOTIF";

#define BUTTON_PIN  GPIO_NUM_15
#define LED_PIN     GPIO_NUM_4

// Event bits
#define EVENT_BUTTON  (1 << 0)
#define EVENT_TIMER   (1 << 1)
#define EVENT_SERIAL  (1 << 2)

TaskHandle_t xHandlerTask = NULL;
TimerHandle_t xPeriodicTimer;

// Button ISR
static void IRAM_ATTR buttonISR(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(xHandlerTask, EVENT_BUTTON, eSetBits, 
                       &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Timer callback - send notification
void vTimerCallback(TimerHandle_t xTimer) {
    xTaskNotify(xHandlerTask, EVENT_TIMER, eSetBits);
}

// Event handler task
void HandlerTask(void *pvParameters) {
    uint32_t notification;
    uint32_t buttonCount = 0;
    uint32_t timerCount = 0;
    
    for(;;) {
        // Wait for any notification
        if(xTaskNotifyWait(0, 0xFFFFFFFF, &notification, 
                           portMAX_DELAY) == pdTRUE) {
            
            if(notification & EVENT_BUTTON) {
                buttonCount++;
                printf("[Handler] Button event! Count: %lu\n", buttonCount);
                
                // Blink LED
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(LED_PIN, 0);
            }
            
            if(notification & EVENT_TIMER) {
                timerCount++;
                printf("[Handler] Timer event! Count: %lu\n", timerCount);
            }
            
            if(notification & EVENT_SERIAL) {
                printf("[Handler] Serial event!\n");
            }
        }
    }
}

// Serial monitor task
void SerialTask(void *pvParameters) {
    uint8_t cmd;
    for(;;) {
        if(uart_read_bytes(UART_NUM_0, &cmd, 1, pdMS_TO_TICKS(10)) > 0) {
            if((char)cmd == 's') {
                xTaskNotify(xHandlerTask, EVENT_SERIAL, eSetBits);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    // Configure GPIO
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);
    
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    printf("\n=== ESP32 Task Notification Demo ===\n\n");
    
    // Create handler task first
    xTaskCreate(HandlerTask, "Handler", 4096, NULL, 3, &xHandlerTask);
    
    // Create serial task
    xTaskCreate(SerialTask, "Serial", 2048, NULL, 1, NULL);
    
    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, buttonISR, NULL);
    
    // Create periodic timer
    xPeriodicTimer = xTimerCreate("Periodic", pdMS_TO_TICKS(5000), 
                                   pdTRUE, NULL, vTimerCallback);
    xTimerStart(xPeriodicTimer, 0);
    
    printf("Press button or send 's' via serial\n\n");
}
```

---

## 8. Use Cases dan Patterns

### 8.1 Debounce Timer Pattern

```c
TimerHandle_t xDebounceTimer;
volatile bool buttonPressed = false;

void vDebounceCallback(TimerHandle_t xTimer) {
    // Timer expired = input stable
    buttonPressed = true;
    printf("Button press confirmed!\n");
}

void EXTI_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Reset timer setiap ada edge
    xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Setup
xDebounceTimer = xTimerCreate("Debounce", pdMS_TO_TICKS(50), 
                               pdFALSE, NULL, vDebounceCallback);
```

### 8.2 Timeout Pattern

```c
TimerHandle_t xTimeoutTimer;

void vTimeoutCallback(TimerHandle_t xTimer) {
    printf("TIMEOUT! Connection lost\n");
    // Handle timeout
}

void processData(void) {
    // Reset timeout setiap terima data
    xTimerReset(xTimeoutTimer, 0);
    
    // Process...
}

// Setup: 5 second timeout
xTimeoutTimer = xTimerCreate("Timeout", pdMS_TO_TICKS(5000), 
                              pdFALSE, NULL, vTimeoutCallback);
xTimerStart(xTimeoutTimer, 0);
```

### 8.3 Notification sebagai Event Flags

```c
#define EVENT_DATA_READY  (1 << 0)
#define EVENT_ERROR       (1 << 1)
#define EVENT_COMPLETE    (1 << 2)

void vEventTask(void *pvParam) {
    uint32_t events;
    
    for(;;) {
        // Wait for any event
        xTaskNotifyWait(0, 0xFFFFFFFF, &events, portMAX_DELAY);
        
        if(events & EVENT_DATA_READY) {
            processData();
        }
        if(events & EVENT_ERROR) {
            handleError();
        }
        if(events & EVENT_COMPLETE) {
            cleanup();
        }
    }
}

// Sender
xTaskNotify(xEventTask, EVENT_DATA_READY | EVENT_COMPLETE, eSetBits);
```

### 8.4 Notification sebagai Counting Semaphore

```c
void vProducerTask(void *pvParam) {
    for(;;) {
        // Produce item
        produceItem();
        
        // Increment notification (like semaphore give)
        xTaskNotifyGive(xConsumerTask);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vConsumerTask(void *pvParam) {
    for(;;) {
        // Wait for notification, decrement on exit
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        
        // Consume item
        consumeItem();
    }
}
```

---

## 9. Best Practices

### 9.1 Software Timer

| Do | Don't |
|----|-------|
| ✅ Callback singkat dan cepat | ❌ Blocking call dalam callback |
| ✅ Gunakan timeout untuk control API | ❌ vTaskDelay dalam callback |
| ✅ Check return value | ❌ Loop infinit dalam callback |
| ✅ One-shot untuk timeout | ❌ Heavy processing dalam callback |

### 9.2 Task Notification

| Do | Don't |
|----|-------|
| ✅ Gunakan untuk point-to-point | ❌ Untuk broadcast |
| ✅ Simpan TaskHandle dengan benar | ❌ Multiple receivers |
| ✅ Clear bits dengan tepat | ❌ Assume notification preserved |
| ✅ Check if task exists | ❌ Notify NULL handle |

### 9.3 Timer Callback Guidelines

```c
// BAIK - Callback singkat
void vGoodCallback(TimerHandle_t xTimer) {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);  // Quick GPIO
    globalFlag = true;                       // Set flag
    // Biarkan task lain handle yang berat
}

// BURUK - Callback blocking
void vBadCallback(TimerHandle_t xTimer) {
    vTaskDelay(pdMS_TO_TICKS(100));  // ❌ NEVER!
    xQueueReceive(xQueue, &data, portMAX_DELAY);  // ❌ NEVER!
    
    for(int i = 0; i < 1000000; i++) {  // ❌ NEVER!
        // Heavy computation
    }
}
```

---

## 10. Troubleshooting

### 10.1 Timer Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Timer tidak start | Daemon task tidak jalan | Check configUSE_TIMERS=1 |
| Callback tidak dipanggil | Timer dormant | Pastikan xTimerStart() |
| Timing tidak akurat | configTICK_RATE_HZ rendah | Tingkatkan tick rate |
| Command queue full | Terlalu banyak timer commands | Perbesar configTIMER_QUEUE_LENGTH |
| Stack overflow daemon | Callback terlalu berat | Perbesar TIMER_TASK_STACK_DEPTH |

### 10.2 Notification Issues

| Problem | Cause | Solution |
|---------|-------|----------|
| Task tidak wake up | Handle NULL | Simpan handle dengan benar |
| Notification hilang | Bits tidak di-clear | Atur clear mask dengan benar |
| Multiple notifications | Counting tidak tepat | Gunakan pdTRUE untuk clear count |
| ISR yield tidak jalan | portYIELD_FROM_ISR lupa | Selalu panggil setelah FromISR |

### 10.3 Debug Techniques

```c
// Check timer state
if(xTimerIsTimerActive(xTimer)) {
    printf("Timer is running\n");
    printf("Period: %lu ticks\n", xTimerGetPeriod(xTimer));
}

// Check notification value
uint32_t notifyValue = ulTaskNotifyValueClear(xTask, 0);
printf("Current notification: 0x%08lX\n", notifyValue);

// Monitor timer daemon
// Enable in FreeRTOSConfig.h:
// #define configUSE_TRACE_FACILITY 1
// #define configUSE_STATS_FORMATTING_FUNCTIONS 1
```

---

## Ringkasan

### Software Timer
- Gunakan untuk periodic atau delayed actions
- Callback HARUS singkat dan non-blocking
- Daemon task mengelola semua timer
- One-shot untuk timeout, auto-reload untuk periodic

### Task Notification  
- Point-to-point communication, sangat efisien
- Bisa sebagai: binary semaphore, counting semaphore, event flags
- 45% lebih cepat dari semaphore tradisional
- Setiap task punya 32-bit notification value bawaan

### Kombinasi
```
ISR → Timer Reset (debounce)
Timer Callback → Task Notification → Handler Task
```

---

## 11. Daftar Program Praktikum

| No | Platform | Nama Program | Topik | Tingkat |
|----|----------|-------------|-------|---------|
| 01 | ESP32 | Timer_OneShot | Software timer one-shot | Dasar |
| 02 | ESP32 | Timer_AutoReload | Software timer auto-reload | Dasar |
| 03 | ESP32 | Timer_Debounce | Button debounce via timer | Menengah |
| 04 | ESP32 | Notification_Basic | Task notification as binary semaphore | Dasar |
| 05 | ESP32 | Notification_Value | Task notification with value | Menengah |
| 06 | ESP32 | Notification_EventFlags | Task notification as event flags | Lanjut |
| 07 | STM32 | Timer_OneShot | Software timer one-shot | Dasar |
| 08 | STM32 | Timer_AutoReload | Software timer auto-reload | Dasar |
| 09 | STM32 | Timer_Debounce | Button debounce via timer | Menengah |
| 10 | STM32 | Notification_Basic | Task notification as binary semaphore | Dasar |
| 11 | STM32 | Notification_Value | Task notification with value | Menengah |
| 12 | STM32 | Notification_EventFlags | Task notification as event flags | Lanjut |

---

## Referensi

1. FreeRTOS Software Timers: https://freertos.org/FreeRTOS-Software-Timer-API-Functions.html
2. FreeRTOS Task Notifications: https://freertos.org/RTOS-task-notifications.html
3. Mastering the FreeRTOS Real Time Kernel - Richard Barry
4. ESP-IDF FreeRTOS Documentation - Espressif Systems
