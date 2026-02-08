# Project Modul 10: Sistem Monitoring Parkir Cerdas

## 📋 Informasi Project
- **Modul**: 10 - FreeRTOS Queue dan Semaphore
- **Tingkat Kesulitan**: Advanced
- **Estimasi Waktu**: 2-3 minggu
- **Platform**: STM32 + ESP32 (Integrated)
- **Tim**: 2-3 orang

---

## 🎯 Deskripsi Project

Membangun sistem monitoring parkir cerdas yang menggunakan STM32 sebagai node sensor lokal dan ESP32 sebagai gateway/display dengan komunikasi via UART. Sistem ini mendemonstrasikan penggunaan **Queue untuk data transfer**, **Counting Semaphore untuk slot management**, dan **Mutex untuk resource protection**.

### Fitur Utama
1. Deteksi kendaraan masuk/keluar (IR sensor simulation dengan button)
2. Counting slot parkir tersedia (Counting Semaphore)
3. Data logging dengan timestamp (Queue)
4. Display status real-time (Mutex-protected LCD/OLED)
5. Indikator LED (penuh/tersedia)
6. Komunikasi STM32-ESP32 via UART

---

## 🏗️ Arsitektur Sistem

```
┌────────────────────────────────────────────────────────────────┐
│                      PARKING SYSTEM                             │
├─────────────────────────────┬──────────────────────────────────┤
│        STM32 NODE           │         ESP32 GATEWAY             │
│    (Sensor & Actuator)      │     (Display & Cloud)             │
├─────────────────────────────┼──────────────────────────────────┤
│                             │                                   │
│  ┌─────────────────────┐    │    ┌─────────────────────┐       │
│  │ Entry Sensor Task   │────┼───→│ UART Receive Task   │       │
│  │ (Button/IR)         │    │    │                     │       │
│  └─────────────────────┘    │    └──────────┬──────────┘       │
│           │                 │               │                   │
│           ▼                 │               ▼                   │
│  ┌─────────────────────┐    │    ┌─────────────────────┐       │
│  │  Event Queue        │    │    │   Data Queue        │       │
│  │  (Entry/Exit events)│    │    │   (Parsed events)   │       │
│  └──────────┬──────────┘    │    └──────────┬──────────┘       │
│             │               │               │                   │
│             ▼               │               ▼                   │
│  ┌─────────────────────┐    │    ┌─────────────────────┐       │
│  │ Parking Manager     │    │    │ Display Task        │       │
│  │ Task                │    │    │ (OLED + LED)        │       │
│  │ [Counting Semaphore]│    │    │ [Mutex Protected]   │       │
│  └──────────┬──────────┘    │    └─────────────────────┘       │
│             │               │               │                   │
│             ▼               │               ▼                   │
│  ┌─────────────────────┐    │    ┌─────────────────────┐       │
│  │ Exit Sensor Task    │    │    │ Logger Task         │       │
│  │ (Button/IR)         │    │    │ (Serial/SD)         │       │
│  └─────────────────────┘    │    │ [Mutex Protected]   │       │
│             │               │    └─────────────────────┘       │
│             ▼               │               │                   │
│  ┌─────────────────────┐    │               ▼                   │
│  │ LED Controller      │    │    ┌─────────────────────┐       │
│  │ Task                │    │    │ Cloud Upload Task   │       │
│  └─────────────────────┘    │    │ (WiFi - Optional)   │       │
│                             │    └─────────────────────┘       │
└─────────────────────────────┴──────────────────────────────────┘
```

---

## 🔧 Hardware Requirements

### STM32 (Blue Pill):
| Komponen | Qty | Pin | Fungsi |
|----------|-----|-----|--------|
| STM32F103C8T6 | 1 | - | Controller |
| Button Entry | 1 | PA0 | Sensor masuk |
| Button Exit | 1 | PA1 | Sensor keluar |
| LED Green | 1 | PA4 | Slot tersedia |
| LED Yellow | 1 | PA5 | Slot terbatas |
| LED Red | 1 | PA6 | Parkir penuh |
| Buzzer | 1 | PA7 | Alert |
| UART to ESP32 | - | PA9/PA10 | Komunikasi |

### ESP32:
| Komponen | Qty | Pin | Fungsi |
|----------|-----|-----|--------|
| ESP32 DevKit | 1 | - | Gateway |
| OLED 0.96" I2C | 1 | GPIO21/22 | Display |
| LED Status | 2 | GPIO4/5 | Indicator |
| UART from STM32 | - | GPIO16/17 | Komunikasi |

### Koneksi STM32-ESP32:
```
STM32 PA9 (TX) ───────────── GPIO16 (RX2) ESP32
STM32 PA10 (RX) ──────────── GPIO17 (TX2) ESP32
STM32 GND ────────────────── GND ESP32
(Level: 3.3V compatible)
```

---

## 📦 Data Structures

### Event Types (Shared)
```c
typedef enum {
    EVENT_ENTRY = 0x01,
    EVENT_EXIT  = 0x02,
    EVENT_FULL  = 0x03,
    EVENT_ERROR = 0xFF
} EventType_t;

typedef struct {
    EventType_t type;
    uint32_t    timestamp;
    uint8_t     slotAvailable;
    uint8_t     totalSlots;
} ParkingEvent_t;
```

### UART Protocol
```
Frame format:
[START][TYPE][SLOT_AVAIL][TOTAL][TIMESTAMP_4BYTES][CHECKSUM][END]
  0xAA   1B      1B        1B         4B             1B      0x55

Example: Entry event, 5 slots left out of 10
0xAA 0x01 0x05 0x0A 0x00 0x00 0x1F 0x40 0xCS 0x55
```

---

## 📝 Implementasi STM32

### main.c (STM32)
```c
/* STM32 Parking Node - main.c */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

#define TOTAL_PARKING_SLOTS    10
#define LED_GREEN              GPIO_PIN_4
#define LED_YELLOW             GPIO_PIN_5
#define LED_RED                GPIO_PIN_6
#define BUZZER                 GPIO_PIN_7

// Handles
UART_HandleTypeDef huart1;  // Debug
UART_HandleTypeDef huart2;  // To ESP32
SemaphoreHandle_t  xParkingSlots;     // Counting semaphore
SemaphoreHandle_t  xUartMutex;        // UART protection
QueueHandle_t      xEventQueue;       // Event queue

// Prototypes
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

// Printf redirect
int _write(int file, char *ptr, int len) {
    if(xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
        xSemaphoreGive(xUartMutex);
    }
    return len;
}

// Send event to ESP32
void sendEventToESP32(ParkingEvent_t *event)
{
    uint8_t frame[10];
    frame[0] = 0xAA;  // Start
    frame[1] = event->type;
    frame[2] = event->slotAvailable;
    frame[3] = event->totalSlots;
    frame[4] = (event->timestamp >> 24) & 0xFF;
    frame[5] = (event->timestamp >> 16) & 0xFF;
    frame[6] = (event->timestamp >> 8) & 0xFF;
    frame[7] = event->timestamp & 0xFF;
    
    // Checksum
    uint8_t checksum = 0;
    for(int i = 1; i < 8; i++) checksum ^= frame[i];
    frame[8] = checksum;
    frame[9] = 0x55;  // End
    
    HAL_UART_Transmit(&huart2, frame, 10, 100);
}

// Entry Sensor Task (simulated with button interrupt)
void vEntrySensorTask(void *pvParameters)
{
    ParkingEvent_t event;
    uint8_t lastState = GPIO_PIN_SET;
    
    for(;;)
    {
        uint8_t currentState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        
        // Detect falling edge (button press)
        if(lastState == GPIO_PIN_SET && currentState == GPIO_PIN_RESET)
        {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
            
            // Try to allocate parking slot
            if(xSemaphoreTake(xParkingSlots, 0) == pdTRUE)
            {
                event.type = EVENT_ENTRY;
                event.timestamp = xTaskGetTickCount();
                event.slotAvailable = uxSemaphoreGetCount(xParkingSlots);
                event.totalSlots = TOTAL_PARKING_SLOTS;
                
                xQueueSend(xEventQueue, &event, pdMS_TO_TICKS(100));
                printf("[ENTRY] Vehicle entered. Slots: %d/%d\r\n",
                       event.slotAvailable, TOTAL_PARKING_SLOTS);
            }
            else
            {
                // Parking full!
                event.type = EVENT_FULL;
                event.timestamp = xTaskGetTickCount();
                event.slotAvailable = 0;
                event.totalSlots = TOTAL_PARKING_SLOTS;
                
                xQueueSend(xEventQueue, &event, pdMS_TO_TICKS(100));
                printf("[FULL] Parking is FULL!\r\n");
                
                // Buzzer alert
                HAL_GPIO_WritePin(GPIOA, BUZZER, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(500));
                HAL_GPIO_WritePin(GPIOA, BUZZER, GPIO_PIN_RESET);
            }
        }
        
        lastState = currentState;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Exit Sensor Task
void vExitSensorTask(void *pvParameters)
{
    ParkingEvent_t event;
    uint8_t lastState = GPIO_PIN_SET;
    
    for(;;)
    {
        uint8_t currentState = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
        
        if(lastState == GPIO_PIN_SET && currentState == GPIO_PIN_RESET)
        {
            vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
            
            // Check if there's a vehicle to exit
            UBaseType_t currentCount = uxSemaphoreGetCount(xParkingSlots);
            if(currentCount < TOTAL_PARKING_SLOTS)
            {
                xSemaphoreGive(xParkingSlots);  // Release slot
                
                event.type = EVENT_EXIT;
                event.timestamp = xTaskGetTickCount();
                event.slotAvailable = uxSemaphoreGetCount(xParkingSlots);
                event.totalSlots = TOTAL_PARKING_SLOTS;
                
                xQueueSend(xEventQueue, &event, pdMS_TO_TICKS(100));
                printf("[EXIT] Vehicle exited. Slots: %d/%d\r\n",
                       event.slotAvailable, TOTAL_PARKING_SLOTS);
            }
        }
        
        lastState = currentState;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Event Processor & Transmit Task
void vEventProcessorTask(void *pvParameters)
{
    ParkingEvent_t event;
    
    for(;;)
    {
        if(xQueueReceive(xEventQueue, &event, portMAX_DELAY) == pdPASS)
        {
            // Send to ESP32
            sendEventToESP32(&event);
            
            // Update LEDs based on slot count
            if(event.slotAvailable == 0)
            {
                // FULL - Red only
                HAL_GPIO_WritePin(GPIOA, LED_GREEN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, LED_YELLOW, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, LED_RED, GPIO_PIN_SET);
            }
            else if(event.slotAvailable <= 3)
            {
                // Limited - Yellow
                HAL_GPIO_WritePin(GPIOA, LED_GREEN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, LED_YELLOW, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOA, LED_RED, GPIO_PIN_RESET);
            }
            else
            {
                // Available - Green
                HAL_GPIO_WritePin(GPIOA, LED_GREEN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOA, LED_YELLOW, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, LED_RED, GPIO_PIN_RESET);
            }
        }
    }
}

// Status Monitor Task
void vStatusTask(void *pvParameters)
{
    for(;;)
    {
        printf("\n========== PARKING STATUS ==========\r\n");
        printf("Available Slots: %lu / %d\r\n",
               uxSemaphoreGetCount(xParkingSlots), TOTAL_PARKING_SLOTS);
        printf("Events in Queue: %lu\r\n", uxQueueMessagesWaiting(xEventQueue));
        printf("=====================================\r\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    
    printf("\r\n=== STM32 Parking Sensor Node ===\r\n");
    printf("Total Slots: %d\r\n\n", TOTAL_PARKING_SLOTS);
    
    // Create counting semaphore: all slots available initially
    xParkingSlots = xSemaphoreCreateCounting(TOTAL_PARKING_SLOTS, TOTAL_PARKING_SLOTS);
    
    // Create mutex for UART
    xUartMutex = xSemaphoreCreateMutex();
    
    // Create event queue
    xEventQueue = xQueueCreate(20, sizeof(ParkingEvent_t));
    
    if(xParkingSlots && xUartMutex && xEventQueue)
    {
        xTaskCreate(vEntrySensorTask, "Entry", 256, NULL, 3, NULL);
        xTaskCreate(vExitSensorTask, "Exit", 256, NULL, 3, NULL);
        xTaskCreate(vEventProcessorTask, "Processor", 256, NULL, 2, NULL);
        xTaskCreate(vStatusTask, "Status", 256, NULL, 1, NULL);
        
        vTaskStartScheduler();
    }
    
    for(;;);
}
```

---

## 📝 Implementasi ESP32

### main.c (ESP32)
```c
/* ESP32 Parking Gateway - main.c */
/* NOTE: OLED display code is pseudo-code reference.
   Students should implement using ESP-IDF I2C + SSD1306 driver. */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"

static const char *TAG = "PARKING_GW";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define LED_STATUS_GREEN  GPIO_NUM_4
#define LED_STATUS_RED    GPIO_NUM_5
#define UART2_RX          GPIO_NUM_16
#define UART2_TX          GPIO_NUM_17
#define UART2_PORT        UART_NUM_2

typedef enum {
    EVENT_ENTRY = 0x01,
    EVENT_EXIT  = 0x02,
    EVENT_FULL  = 0x03,
    EVENT_ERROR = 0xFF
} EventType_t;

typedef struct {
    EventType_t type;
    uint32_t    timestamp;
    uint8_t     slotAvailable;
    uint8_t     totalSlots;
} ParkingEvent_t;

typedef struct {
    uint8_t  slotAvailable;
    uint8_t  totalSlots;
    uint32_t lastUpdate;
    uint32_t totalEntries;
    uint32_t totalExits;
} DisplayData_t;

// Display driver: students should implement using ESP-IDF I2C + SSD1306 library
// e.g., https://github.com/nopnop2002/esp-idf-ssd1306

QueueHandle_t      xDataQueue;
SemaphoreHandle_t  xDisplayMutex;
SemaphoreHandle_t  xSerialMutex;

DisplayData_t globalStatus = {10, 10, 0, 0, 0};

void safePrint(const char* format, ...) {
    if(xSemaphoreTake(xSerialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        xSemaphoreGive(xSerialMutex);
    }
}

// UART Receive Task
void UARTReceiveTask(void *pvParameters) {
    uint8_t buffer[10];
    uint8_t idx = 0;
    bool inFrame = false;
    
    for(;;) {
        uint8_t byte;
        while(uart_read_bytes(UART2_PORT, &byte, 1, 0) > 0) {
            
            if(byte == 0xAA && !inFrame) {
                inFrame = true;
                idx = 0;
                buffer[idx++] = byte;
            }
            else if(inFrame) {
                buffer[idx++] = byte;
                
                if(byte == 0x55 && idx == 10) {
                    // Complete frame received
                    // Verify checksum
                    uint8_t checksum = 0;
                    for(int i = 1; i < 8; i++) checksum ^= buffer[i];
                    
                    if(checksum == buffer[8]) {
                        ParkingEvent_t event;
                        event.type = (EventType_t)buffer[1];
                        event.slotAvailable = buffer[2];
                        event.totalSlots = buffer[3];
                        event.timestamp = ((uint32_t)buffer[4] << 24) |
                                         ((uint32_t)buffer[5] << 16) |
                                         ((uint32_t)buffer[6] << 8) |
                                         buffer[7];
                        
                        xQueueSend(xDataQueue, &event, pdMS_TO_TICKS(100));
                        safePrint("[UART] Valid frame received\n");
                    }
                    else {
                        safePrint("[UART] Checksum error!\n");
                    }
                    
                    inFrame = false;
                }
                else if(idx >= 10) {
                    // Frame too long, reset
                    inFrame = false;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Data Processor Task
void DataProcessorTask(void *pvParameters) {
    ParkingEvent_t event;
    
    for(;;) {
        if(xQueueReceive(xDataQueue, &event, portMAX_DELAY) == pdPASS) {
            // Update global status
            globalStatus.slotAvailable = event.slotAvailable;
            globalStatus.totalSlots = event.totalSlots;
            globalStatus.lastUpdate = (uint32_t)(esp_timer_get_time() / 1000);
            
            switch(event.type) {
                case EVENT_ENTRY:
                    globalStatus.totalEntries++;
                    safePrint("[PROC] Entry event. Slots: %d/%d\n",
                             event.slotAvailable, event.totalSlots);
                    break;
                    
                case EVENT_EXIT:
                    globalStatus.totalExits++;
                    safePrint("[PROC] Exit event. Slots: %d/%d\n",
                             event.slotAvailable, event.totalSlots);
                    break;
                    
                case EVENT_FULL:
                    safePrint("[PROC] PARKING FULL!\n");
                    // Blink red LED
                    for(int i = 0; i < 5; i++) {
                        gpio_set_level(LED_STATUS_RED, 1);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        gpio_set_level(LED_STATUS_RED, 0);
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    break;
                    
                default:
                    safePrint("[PROC] Unknown event\n");
            }
            
            // Update status LEDs
            if(event.slotAvailable == 0) {
                gpio_set_level(LED_STATUS_GREEN, 0);
                gpio_set_level(LED_STATUS_RED, 1);
            } else {
                gpio_set_level(LED_STATUS_GREEN, 1);
                gpio_set_level(LED_STATUS_RED, 0);
            }
        }
    }
}

// Display Task
// NOTE: Students should implement OLED display using ESP-IDF I2C + SSD1306 driver
// The following is pseudo-code showing the display logic
void DisplayTask(void *pvParameters) {
    char line[25];
    
    for(;;) {
        if(xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Clear display buffer
            // ssd1306_clear_screen(&dev);
            
            // Title
            // ssd1306_display_text(&dev, 0, "SMART PARKING", 13, false);
            
            // Slot count
            sprintf(line, "%d/%d", globalStatus.slotAvailable, globalStatus.totalSlots);
            // ssd1306_display_text(&dev, 2, line, strlen(line), false);
            
            // Status text
            if(globalStatus.slotAvailable == 0) {
                // ssd1306_display_text(&dev, 5, "FULL!", 5, false);
            } else if(globalStatus.slotAvailable <= 3) {
                // ssd1306_display_text(&dev, 5, "LIMITED", 7, false);
            } else {
                // ssd1306_display_text(&dev, 5, "AVAILABLE", 9, false);
            }
            
            // Stats
            sprintf(line, "In:%lu Out:%lu", globalStatus.totalEntries, globalStatus.totalExits);
            // ssd1306_display_text(&dev, 7, line, strlen(line), false);
            
            xSemaphoreGive(xDisplayMutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Logger Task
void LoggerTask(void *pvParameters) {
    for(;;) {
        safePrint("\n====== PARKING STATUS ======\n");
        safePrint("Slots: %d / %d\n", globalStatus.slotAvailable, globalStatus.totalSlots);
        safePrint("Total IN:  %lu\n", globalStatus.totalEntries);
        safePrint("Total OUT: %lu\n", globalStatus.totalExits);
        safePrint("Queue:     %d items\n", uxQueueMessagesWaiting(xDataQueue));
        safePrint("Free Heap: %lu bytes\n", esp_get_free_heap_size());
        safePrint("============================\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void) {
    // Configure UART2 for STM32 communication
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART2_PORT, 256, 0, 0, NULL, 0);
    uart_param_config(UART2_PORT, &uart_config);
    uart_set_pin(UART2_PORT, UART2_TX, UART2_RX, -1, -1);
    
    // Configure LED pins
    gpio_reset_pin(LED_STATUS_GREEN);
    gpio_set_direction(LED_STATUS_GREEN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_STATUS_RED);
    gpio_set_direction(LED_STATUS_RED, GPIO_MODE_OUTPUT);
    
    // Initialize OLED (students: implement using ESP-IDF I2C + SSD1306 driver)
    // Example: i2c_master_init(); ssd1306_init();
    
    ESP_LOGI(TAG, "=== ESP32 Parking Gateway ===");
    
    // Create primitives
    xDataQueue = xQueueCreate(30, sizeof(ParkingEvent_t));
    xDisplayMutex = xSemaphoreCreateMutex();
    xSerialMutex = xSemaphoreCreateMutex();
    
    if(xDataQueue && xDisplayMutex && xSerialMutex) {
        // Pin UART task to Core 0
        xTaskCreatePinnedToCore(UARTReceiveTask, "UART", 4096, NULL, 3, NULL, 0);
        
        // Pin processor to Core 1
        xTaskCreatePinnedToCore(DataProcessorTask, "Proc", 4096, NULL, 2, NULL, 1);
        
        // Display on Core 1
        xTaskCreatePinnedToCore(DisplayTask, "Display", 4096, NULL, 2, NULL, 1);
        
        // Logger on Core 0
        xTaskCreatePinnedToCore(LoggerTask, "Logger", 4096, NULL, 1, NULL, 0);
    }
}
```

---

## 📊 Kriteria Penilaian

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| **Counting Semaphore** | 20% | Implementasi slot management yang benar |
| **Queue Implementation** | 20% | Event passing dan UART communication |
| **Mutex Usage** | 15% | Display dan UART protection |
| **STM32 Node** | 15% | Sensor detection dan LED control |
| **ESP32 Gateway** | 15% | Display update dan data processing |
| **Integrasi** | 10% | Komunikasi UART berhasil |
| **Dokumentasi** | 5% | Kode dan laporan |

---

## 📋 Deliverables

1. **Source Code** - STM32 dan ESP32 (terstruktur)
2. **Dokumentasi** - Penjelasan arsitektur dan flowchart
3. **Video Demo** - 5-10 menit demonstrasi sistem
4. **Laporan** - PDF dengan analisis dan kesimpulan

---

## ⏰ Timeline

| Minggu | Aktivitas |
|--------|-----------|
| 1 | Setup hardware, implementasi STM32 |
| 2 | Implementasi ESP32, integrasi UART |
| 3 | Testing, debugging, dokumentasi |

---

*Project Modul 10 - FreeRTOS Queue dan Semaphore*
