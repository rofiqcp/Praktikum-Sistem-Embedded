# Jobsheet Modul 10: FreeRTOS Queue dan Semaphore

## 📋 Informasi Praktikum
- **Modul**: 10 - FreeRTOS Queue dan Semaphore
- **Durasi**: 3 x 50 menit
- **Platform**: STM32F103C8T6 & ESP32

---

## 🎯 Tujuan Praktikum
1. Memahami konsep Queue untuk komunikasi antar task
2. Mengimplementasikan Binary dan Counting Semaphore
3. Menggunakan Mutex untuk proteksi shared resource
4. Menerapkan pattern Producer-Consumer
5. Menghindari race condition dan deadlock

---

## 🔧 Alat dan Bahan

### Hardware STM32:
| No | Komponen | Qty | Keterangan |
|----|----------|-----|------------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | Mikrokontroler utama |
| 2 | ST-Link V2 | 1 | Programmer |
| 3 | LED (warna berbeda) | 4 | Output indicator |
| 4 | Push Button | 2 | Input trigger |
| 5 | Resistor 330Ω | 4 | LED current limiter |
| 6 | Resistor 10kΩ | 2 | Pull-up/down button |
| 7 | Potensiometer 10kΩ | 1 | Analog input |
| 8 | Breadboard | 1 | Prototyping |
| 9 | Kabel jumper | Set | Koneksi |

### Hardware ESP32:
| No | Komponen | Qty | Keterangan |
|----|----------|-----|------------|
| 1 | ESP32 DevKit V1 | 1 | Mikrokontroler dual-core |
| 2 | Kabel USB | 1 | Programming & power |
| 3 | LED (warna berbeda) | 4 | Output indicator |
| 4 | Push Button | 2 | Input trigger |
| 5 | Resistor 330Ω | 4 | LED current limiter |
| 6 | Resistor 10kΩ | 2 | Pull-up/down button |
| 7 | Potensiometer 10kΩ | 1 | Analog input |

---

## 📐 Diagram Koneksi

### STM32 Wiring:
```
STM32F103C8T6 (Blue Pill)
        ┌───────────────────────┐
        │                       │
   PA0 ─┤ Button 1 (Pull-up)    │
   PA1 ─┤ Button 2 (Pull-up)    │
   PA4 ─┤ LED Red               │
   PA5 ─┤ LED Yellow            │
   PA6 ─┤ LED Green             │
   PA7 ─┤ LED Blue              │
   PA2 ─┤ Potentiometer (ADC)   │
  PA9 ──┤ UART TX               │
  PA10 ─┤ UART RX               │
  GND ──┤ Ground                │
  3.3V ─┤ Power                 │
        └───────────────────────┘

LED Wiring (semua LED):
    PA[x] ──[330Ω]──[LED]── GND

Button Wiring:
    PA[x] ──[Button]── GND (internal pull-up enabled)
           └─[10kΩ]── 3.3V (external pull-up optional)

Potentiometer:
    3.3V ──[POT]── GND
           └── PA2 (ADC)
```

### ESP32 Wiring:
```
ESP32 DevKit V1
        ┌───────────────────────┐
        │                       │
  GPIO4 ─┤ LED Red               │
  GPIO5 ─┤ LED Yellow            │
 GPIO18 ─┤ LED Green             │
 GPIO19 ─┤ LED Blue              │
 GPIO15 ─┤ Button 1 (Pull-up)    │
 GPIO16 ─┤ Button 2 (Pull-up)    │
 GPIO34 ─┤ Potentiometer (ADC)   │
   GND ──┤ Ground                │
  3.3V ─┤ Power                 │
        └───────────────────────┘
```

---

## 📁 Struktur Project

```
Modul-10-FreeRTOS-Queue-Semaphore/
├── praktikum/
│   ├── STM32/
│   │   ├── 01_basic_queue/
│   │   ├── 02_queue_struct/
│   │   ├── 03_binary_semaphore/
│   │   ├── 04_counting_semaphore/
│   │   ├── 05_mutex/
│   │   ├── 06_producer_consumer/
│   │   ├── 07_queue_isr/
│   │   ├── 08_mutex_uart/
│   │   ├── 09_event_group/
│   │   └── 10_integrated_system/
│   └── ESP32/
│       ├── 01_basic_queue/
│       ├── 02_queue_dual_core/
│       ├── 03_binary_semaphore/
│       ├── 04_counting_semaphore/
│       ├── 05_mutex/
│       ├── 06_producer_consumer/
│       ├── 07_queue_isr/
│       ├── 08_mutex_serial/
│       ├── 09_event_group/
│       └── 10_integrated_system/
```

---

# BAGIAN A: PRAKTIKUM STM32

## Program 1: Basic Queue (STM32)

### Tujuan
Memahami penggunaan queue untuk transfer data integer antar task.

### Kode Program
```c
/* main.c - Basic Queue STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;
QueueHandle_t xQueue;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

// Sender Task - Mengirim angka ke queue
void vSenderTask(void *pvParameters)
{
    int32_t valueToSend = 0;
    BaseType_t status;
    
    for(;;)
    {
        valueToSend++;
        
        // Kirim ke queue dengan timeout 100ms
        status = xQueueSend(xQueue, &valueToSend, pdMS_TO_TICKS(100));
        
        if(status == pdPASS)
        {
            printf("[Sender] Sent: %ld\r\n", valueToSend);
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);  // Toggle LED
        }
        else
        {
            printf("[Sender] Queue Full!\r\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Receiver Task - Menerima angka dari queue
void vReceiverTask(void *pvParameters)
{
    int32_t receivedValue;
    BaseType_t status;
    
    for(;;)
    {
        // Terima dari queue (block sampai ada data)
        status = xQueueReceive(xQueue, &receivedValue, portMAX_DELAY);
        
        if(status == pdPASS)
        {
            printf("[Receiver] Received: %ld\r\n", receivedValue);
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // Toggle LED
        }
    }
}

// Monitor Task - Monitor status queue
void vMonitorTask(void *pvParameters)
{
    for(;;)
    {
        printf("\n--- Queue Status ---\r\n");
        printf("Messages waiting: %lu\r\n", uxQueueMessagesWaiting(xQueue));
        printf("Spaces available: %lu\r\n", uxQueueSpacesAvailable(xQueue));
        printf("-------------------\r\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n=== Basic Queue Demo ===\r\n");
    
    // Buat queue untuk 5 item int32_t
    xQueue = xQueueCreate(5, sizeof(int32_t));
    
    if(xQueue != NULL)
    {
        xTaskCreate(vSenderTask, "Sender", 256, NULL, 2, NULL);
        xTaskCreate(vReceiverTask, "Receiver", 256, NULL, 2, NULL);
        xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);
        
        vTaskStartScheduler();
    }
    else
    {
        printf("Failed to create queue!\r\n");
    }
    
    for(;;);
}
```

### Tugas
1. Ubah ukuran queue menjadi 3, amati apa yang terjadi
2. Buat sender lebih cepat dari receiver, amati queue full
3. Tambahkan sender kedua, amati bagaimana data diterima

---

## Program 2: Queue dengan Struct (STM32)

### Tujuan
Mengirim data kompleks (struct) melalui queue.

### Kode Program
```c
/* main.c - Queue Struct STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
QueueHandle_t xSensorQueue;

// Struktur data sensor
typedef struct {
    uint8_t  sensorId;
    float    value;
    uint32_t timestamp;
    char     unit[8];
} SensorData_t;

void vSensorTask(void *pvParameters)
{
    SensorData_t data;
    uint8_t id = (uint8_t)(uint32_t)pvParameters;
    
    for(;;)
    {
        data.sensorId = id;
        data.timestamp = xTaskGetTickCount();
        
        switch(id)
        {
            case 1:  // Temperature
                HAL_ADC_Start(&hadc1);
                HAL_ADC_PollForConversion(&hadc1, 100);
                data.value = (float)HAL_ADC_GetValue(&hadc1) * 3.3 / 4095.0 * 100;
                strcpy(data.unit, "°C");
                break;
            case 2:  // Voltage
                HAL_ADC_Start(&hadc1);
                HAL_ADC_PollForConversion(&hadc1, 100);
                data.value = (float)HAL_ADC_GetValue(&hadc1) * 3.3 / 4095.0;
                strcpy(data.unit, "V");
                break;
        }
        
        if(xQueueSend(xSensorQueue, &data, pdMS_TO_TICKS(100)) == pdPASS)
        {
            printf("[Sensor%d] Sent: %.2f %s\r\n", id, data.value, data.unit);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500 + id * 200));
    }
}

void vDisplayTask(void *pvParameters)
{
    SensorData_t data;
    
    for(;;)
    {
        if(xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdPASS)
        {
            printf("\n┌─────────────────────────┐\r\n");
            printf("│ Sensor ID: %d            │\r\n", data.sensorId);
            printf("│ Value: %.2f %-4s       │\r\n", data.value, data.unit);
            printf("│ Time: %lu ms           │\r\n", data.timestamp);
            printf("└─────────────────────────┘\r\n\n");
        }
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    
    printf("=== Queue Struct Demo ===\r\n");
    
    xSensorQueue = xQueueCreate(10, sizeof(SensorData_t));
    
    if(xSensorQueue != NULL)
    {
        xTaskCreate(vSensorTask, "Temp", 256, (void*)1, 2, NULL);
        xTaskCreate(vSensorTask, "Volt", 256, (void*)2, 2, NULL);
        xTaskCreate(vDisplayTask, "Display", 512, NULL, 2, NULL);
        
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

---

## Program 3: Binary Semaphore (STM32)

### Tujuan
Menggunakan binary semaphore untuk sinkronisasi ISR dengan task.

### Kode Program
```c
/* main.c - Binary Semaphore STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;
SemaphoreHandle_t xButtonSemaphore;
volatile uint32_t buttonPressCount = 0;

// Button ISR Callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == GPIO_PIN_0)
    {
        buttonPressCount++;
        xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Button Handler Task
void vButtonHandlerTask(void *pvParameters)
{
    for(;;)
    {
        // Block sampai semaphore di-give dari ISR
        if(xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            printf("[Handler] Button #%lu pressed!\r\n", buttonPressCount);
            
            // Blink LED
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(200));
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
            
            // Proses lebih lanjut (simulated)
            printf("[Handler] Processing...\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            printf("[Handler] Done!\r\n");
        }
    }
}

// Background Task
void vBackgroundTask(void *pvParameters)
{
    for(;;)
    {
        printf("[Background] Running...\r\n");
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("=== Binary Semaphore Demo ===\r\n");
    printf("Press button to trigger handler\r\n\n");
    
    // Create binary semaphore
    xButtonSemaphore = xSemaphoreCreateBinary();
    
    if(xButtonSemaphore != NULL)
    {
        xTaskCreate(vButtonHandlerTask, "BtnHandler", 256, NULL, 3, NULL);
        xTaskCreate(vBackgroundTask, "Background", 128, NULL, 1, NULL);
        
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

---

## Program 4: Counting Semaphore (STM32)

### Tujuan
Menggunakan counting semaphore untuk resource pool management.

### Kode Program
```c
/* main.c - Counting Semaphore STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

#define MAX_RESOURCES 3

UART_HandleTypeDef huart1;
SemaphoreHandle_t xResourceSemaphore;

void vWorkerTask(void *pvParameters)
{
    uint8_t workerId = (uint8_t)(uint32_t)pvParameters;
    
    for(;;)
    {
        printf("[Worker%d] Requesting resource... ", workerId);
        printf("(Available: %lu)\r\n", uxSemaphoreGetCount(xResourceSemaphore));
        
        // Try to get resource (timeout 5 seconds)
        if(xSemaphoreTake(xResourceSemaphore, pdMS_TO_TICKS(5000)) == pdTRUE)
        {
            printf("[Worker%d] GOT resource! ", workerId);
            printf("(Available: %lu)\r\n", uxSemaphoreGetCount(xResourceSemaphore));
            
            // Indicate using resource (turn on LED)
            HAL_GPIO_WritePin(GPIOA, (GPIO_PIN_4 << workerId), GPIO_PIN_SET);
            
            // Use resource (simulated work)
            uint32_t workTime = 1000 + (workerId * 500);
            printf("[Worker%d] Working for %lu ms...\r\n", workerId, workTime);
            vTaskDelay(pdMS_TO_TICKS(workTime));
            
            // Turn off LED
            HAL_GPIO_WritePin(GPIOA, (GPIO_PIN_4 << workerId), GPIO_PIN_RESET);
            
            // Release resource
            xSemaphoreGive(xResourceSemaphore);
            printf("[Worker%d] RELEASED resource ", workerId);
            printf("(Available: %lu)\r\n", uxSemaphoreGetCount(xResourceSemaphore));
        }
        else
        {
            printf("[Worker%d] TIMEOUT waiting for resource!\r\n", workerId);
        }
        
        // Wait before requesting again
        vTaskDelay(pdMS_TO_TICKS(500 + workerId * 100));
    }
}

void vMonitorTask(void *pvParameters)
{
    for(;;)
    {
        printf("\n===== RESOURCE POOL STATUS =====\r\n");
        printf("Total: %d, Available: %lu, In Use: %lu\r\n",
               MAX_RESOURCES,
               uxSemaphoreGetCount(xResourceSemaphore),
               MAX_RESOURCES - uxSemaphoreGetCount(xResourceSemaphore));
        printf("================================\r\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("=== Counting Semaphore Demo ===\r\n");
    printf("Resource Pool: %d resources\r\n\n", MAX_RESOURCES);
    
    // Create counting semaphore: max=3, initial=3
    xResourceSemaphore = xSemaphoreCreateCounting(MAX_RESOURCES, MAX_RESOURCES);
    
    if(xResourceSemaphore != NULL)
    {
        // Create 5 workers competing for 3 resources
        for(int i = 0; i < 5; i++)
        {
            char taskName[12];
            sprintf(taskName, "Worker%d", i);
            xTaskCreate(vWorkerTask, taskName, 256, (void*)i, 2, NULL);
        }
        
        xTaskCreate(vMonitorTask, "Monitor", 256, NULL, 1, NULL);
        
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

---

## Program 5: Mutex - Protecting UART (STM32)

### Tujuan
Menggunakan mutex untuk proteksi akses UART dari multiple tasks.

### Kode Program
```c
/* main.c - Mutex UART Protection STM32 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;
SemaphoreHandle_t xUARTMutex;

// Function to send multi-line message safely
void safePrintMultiLine(const char *taskName, const char *lines[], int numLines)
{
    if(xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        printf("\r\n╔═══════════════════════════╗\r\n");
        printf("║ Task: %-19s ║\r\n", taskName);
        printf("╠═══════════════════════════╣\r\n");
        
        for(int i = 0; i < numLines; i++)
        {
            printf("║ %-25s ║\r\n", lines[i]);
        }
        
        printf("╚═══════════════════════════╝\r\n\n");
        
        xSemaphoreGive(xUARTMutex);
    }
    else
    {
        // Mutex timeout - print simple message
        printf("[%s] Mutex timeout!\r\n", taskName);
    }
}

void vTask1(void *pvParameters)
{
    const char *lines[] = {
        "Line 1 from Task 1",
        "Line 2 from Task 1",
        "Line 3 from Task 1"
    };
    
    for(;;)
    {
        safePrintMultiLine("Task1", lines, 3);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

void vTask2(void *pvParameters)
{
    const char *lines[] = {
        "Data from Task 2",
        "More data from Task 2"
    };
    
    for(;;)
    {
        safePrintMultiLine("Task2", lines, 2);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTask3(void *pvParameters)
{
    const char *lines[] = {
        "Task 3 reporting",
        "Status: OK",
        "Counter: 123",
        "Temperature: 25.5C"
    };
    
    for(;;)
    {
        safePrintMultiLine("Task3", lines, 4);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
        vTaskDelay(pdMS_TO_TICKS(900));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("=== Mutex UART Protection Demo ===\r\n\n");
    
    xUARTMutex = xSemaphoreCreateMutex();
    
    if(xUARTMutex != NULL)
    {
        xTaskCreate(vTask1, "Task1", 512, NULL, 2, NULL);
        xTaskCreate(vTask2, "Task2", 512, NULL, 2, NULL);
        xTaskCreate(vTask3, "Task3", 512, NULL, 2, NULL);
        
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

---

## Program 6-10: (Lanjutan STM32)

Karena keterbatasan ruang, program 6-10 STM32 mencakup:

6. **Producer-Consumer Pattern** - Multiple producers, single consumer
7. **Queue dari ISR** - Timer interrupt feeding queue
8. **Multiple Mutex** - Protecting multiple resources
9. **Event Groups** - Waiting for multiple events
10. **Integrated System** - Combining queue, semaphore, mutex

---

# BAGIAN B: PRAKTIKUM ESP32

## Program 1: Basic Queue ESP32

### Kode Program
```c
/* main.c - Basic Queue ESP32 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "QUEUE_BASIC";

#define LED_SENDER    GPIO_NUM_4
#define LED_RECEIVER  GPIO_NUM_5

QueueHandle_t xQueue;

void SenderTask(void *pvParameters) {
    int32_t value = 0;
    
    for(;;) {
        value++;
        
        if(xQueueSend(xQueue, &value, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGI(TAG, "[Sender@Core%d] Sent: %ld", xPortGetCoreID(), value);
            gpio_set_level(LED_SENDER, !gpio_get_level(LED_SENDER));
        } else {
            ESP_LOGW(TAG, "[Sender] Queue Full!");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void ReceiverTask(void *pvParameters) {
    int32_t value;
    
    for(;;) {
        if(xQueueReceive(xQueue, &value, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "[Receiver@Core%d] Received: %ld", xPortGetCoreID(), value);
            gpio_set_level(LED_RECEIVER, !gpio_get_level(LED_RECEIVER));
        }
    }
}

void MonitorTask(void *pvParameters) {
    for(;;) {
        ESP_LOGI(TAG, "\n=== Queue Status ===");
        ESP_LOGI(TAG, "Messages: %d", uxQueueMessagesWaiting(xQueue));
        ESP_LOGI(TAG, "Spaces: %d", uxQueueSpacesAvailable(xQueue));
        ESP_LOGI(TAG, "Free Heap: %lu", esp_get_free_heap_size());
        ESP_LOGI(TAG, "====================\n");
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void app_main(void) {
    gpio_reset_pin(LED_SENDER);
    gpio_set_direction(LED_SENDER, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_RECEIVER);
    gpio_set_direction(LED_RECEIVER, GPIO_MODE_OUTPUT);
    
    ESP_LOGI(TAG, "=== ESP32 Basic Queue Demo ===");
    
    xQueue = xQueueCreate(5, sizeof(int32_t));
    
    if(xQueue != NULL) {
        // Sender on Core 0
        xTaskCreatePinnedToCore(SenderTask, "Sender", 4096, NULL, 2, NULL, 0);
        // Receiver on Core 1
        xTaskCreatePinnedToCore(ReceiverTask, "Receiver", 4096, NULL, 2, NULL, 1);
        // Monitor on Core 0
        xTaskCreatePinnedToCore(MonitorTask, "Monitor", 4096, NULL, 1, NULL, 0);
    }
}
```

---

## Program 2: Queue Dual-Core ESP32

### Tujuan
Memanfaatkan dual-core ESP32 dengan queue communication.

### Kode Program
```c
/* main.c - Queue Dual-Core ESP32 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "QUEUE_DUALCORE";

typedef struct {
    uint8_t  sourceCore;
    float    data;
    uint32_t timestamp;
} CoreMessage_t;

QueueHandle_t xCoreQueue;
SemaphoreHandle_t xSerialMutex;

void safePrint(const char* format, ...) {
    if(xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        xSemaphoreGive(xSerialMutex);
    }
}

// Runs on Core 0
void Core0Task(void *pvParameters) {
    CoreMessage_t msg;
    float value = 0;
    
    for(;;) {
        msg.sourceCore = 0;
        msg.data = value++;
        msg.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
        
        xQueueSend(xCoreQueue, &msg, pdMS_TO_TICKS(100));
        safePrint("[Core0] Sent: %.1f\n", msg.data);
        
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// Runs on Core 1
void Core1Task(void *pvParameters) {
    CoreMessage_t msg;
    float value = 100;
    
    for(;;) {
        msg.sourceCore = 1;
        msg.data = value++;
        msg.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
        
        xQueueSend(xCoreQueue, &msg, pdMS_TO_TICKS(100));
        safePrint("[Core1] Sent: %.1f\n", msg.data);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Consumer - Processes messages from both cores
void ConsumerTask(void *pvParameters) {
    CoreMessage_t msg;
    
    for(;;) {
        if(xQueueReceive(xCoreQueue, &msg, portMAX_DELAY) == pdPASS) {
            safePrint("\n╔══════════════════════════╗\n");
            safePrint("║ Message from Core %d      ║\n", msg.sourceCore);
            safePrint("║ Data: %-18.1f║\n", msg.data);
            safePrint("║ Time: %-18lu║\n", msg.timestamp);
            safePrint("╚══════════════════════════╝\n\n");
        }
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "=== ESP32 Dual-Core Queue Demo ===");
    
    xCoreQueue = xQueueCreate(20, sizeof(CoreMessage_t));
    xSerialMutex = xSemaphoreCreateMutex();
    
    if(xCoreQueue != NULL && xSerialMutex != NULL) {
        xTaskCreatePinnedToCore(Core0Task, "Core0", 4096, NULL, 2, NULL, 0);
        xTaskCreatePinnedToCore(Core1Task, "Core1", 4096, NULL, 2, NULL, 1);
        xTaskCreatePinnedToCore(ConsumerTask, "Consumer", 8192, NULL, 3, NULL, 1);
    }
}
```

---

## Program 3: Binary Semaphore dengan Button Interrupt (ESP32)

```c
/* main.c - Binary Semaphore ESP32 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BINARY_SEM";

#define BUTTON_PIN  GPIO_NUM_15
#define LED_PIN     GPIO_NUM_4

SemaphoreHandle_t xButtonSemaphore;
volatile uint32_t pressCount = 0;

// ISR Handler
static void IRAM_ATTR buttonISR(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    pressCount++;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void ButtonHandlerTask(void *pvParameters) {
    for(;;) {
        if(xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "[Handler] Button pressed #%ld!", pressCount);
            
            // Blink LED
            for(int i = 0; i < 3; i++) {
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_set_level(LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

void BackgroundTask(void *pvParameters) {
    for(;;) {
        ESP_LOGI(TAG, "[Background] Running...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    // Configure button pin
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);
    
    // Configure LED pin
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    xButtonSemaphore = xSemaphoreCreateBinary();
    
    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, buttonISR, NULL);
    
    xTaskCreate(ButtonHandlerTask, "BtnHandler", 4096, NULL, 3, NULL);
    xTaskCreate(BackgroundTask, "Background", 2048, NULL, 1, NULL);
    
    ESP_LOGI(TAG, "=== Binary Semaphore Demo ===");
    ESP_LOGI(TAG, "Press button to trigger handler");
}
```

---

## Program 4-10: (Lanjutan ESP32)

Program 4-10 ESP32 mencakup:

4. **Counting Semaphore** - Resource pool
5. **Mutex Serial Protection** - Safe serial output
6. **Producer-Consumer** - Multi-sensor pattern
7. **Queue dari Timer ISR** - Timer-driven queue
8. **Event Groups** - Multiple event synchronization
9. **Queue Sets** - Waiting on multiple queues
10. **Integrated IoT System** - Complete system

---

## 📊 Tabel Perbandingan Queue vs Semaphore vs Mutex

| Fitur | Queue | Binary Semaphore | Counting Semaphore | Mutex |
|-------|-------|------------------|-------------------|-------|
| **Transfer Data** | ✅ Ya | ❌ Tidak | ❌ Tidak | ❌ Tidak |
| **Sinkronisasi** | ✅ Ya | ✅ Ya | ✅ Ya | ✅ Ya |
| **ISR Safe** | ✅ Ya | ✅ Ya | ✅ Ya | ❌ Tidak |
| **Priority Inheritance** | ❌ Tidak | ❌ Tidak | ❌ Tidak | ✅ Ya |
| **Ownership** | ❌ Tidak | ❌ Tidak | ❌ Tidak | ✅ Ya |
| **Max Value** | N items | 1 | N | 1 |
| **Typical Use** | Data transfer | ISR→Task | Resource pool | Protect resource |

---

## 🔍 Troubleshooting

| Masalah | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| Queue selalu full | Consumer terlalu lambat | Percepat consumer atau perbesar queue |
| Task stuck (deadlock) | Lock ordering berbeda | Gunakan urutan mutex yang konsisten |
| Data corrupted | Race condition | Gunakan mutex untuk protect data |
| Semaphore leak | Lupa give setelah take | Pastikan setiap take ada give |
| Priority inversion | Binary semaphore untuk protect | Gunakan mutex |
| Stack overflow | Stack size kurang | Tingkatkan stack size |

---

## 📝 Tugas Praktikum

### Tugas 1: Queue Multi-Sensor (30%)
Buat sistem dengan 3 sensor (suhu, cahaya, tekanan) yang mengirim data ke queue. Single consumer memproses dan menampilkan data.

### Tugas 2: Resource Pool (30%)
Implementasikan counting semaphore untuk mengatur akses ke 2 "printer" (LED) dari 4 task print job.

### Tugas 3: Safe Logger (40%)
Buat logger system dimana multiple task bisa menulis ke serial dengan aman menggunakan mutex. Setiap pesan harus atomic (tidak terpotong).

---

## ✅ Checklist Praktikum

- [ ] Program 1: Basic Queue
- [ ] Program 2: Queue Struct
- [ ] Program 3: Binary Semaphore
- [ ] Program 4: Counting Semaphore
- [ ] Program 5: Mutex
- [ ] Program 6-10: Advanced Programs
- [ ] Tugas 1 selesai
- [ ] Tugas 2 selesai
- [ ] Tugas 3 selesai
- [ ] Video demo dikirim

---

*Jobsheet Modul 10 - FreeRTOS Queue dan Semaphore*
