# Jobsheet Modul 11: FreeRTOS Software Timer dan Task Notification

## 📋 Informasi Praktikum

| Komponen | Detail |
|----------|--------|
| **Mata Kuliah** | Praktikum Sistem Embedded |
| **Modul** | 11 - Software Timer dan Task Notification |
| **Durasi** | 3 × 50 menit |
| **Platform** | STM32F103C8T6 & ESP32 |

---

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan praktikum ini, mahasiswa mampu:
1. Membuat dan mengontrol FreeRTOS software timer
2. Mengimplementasikan timer callback non-blocking
3. Menggunakan task notification untuk event signaling
4. Mengkombinasikan timer dan notification dalam aplikasi nyata

---

## 🔧 Peralatan yang Dibutuhkan

### Hardware
| Komponen | Jumlah | Fungsi |
|----------|--------|--------|
| STM32F103C8T6 (Blue Pill) | 1 | MCU utama |
| ESP32 DevKit V1 | 1 | MCU pendukung |
| ST-Link V2 | 1 | Programmer STM32 |
| LED (berbagai warna) | 4 | Output visual |
| Push Button | 2 | Input trigger |
| Resistor 330Ω | 4 | Current limiting LED |
| Resistor 10kΩ | 2 | Pull-up button |
| Breadboard | 1 | Prototyping |
| Kabel jumper | Secukupnya | Koneksi |

### Software
- VS Code + PlatformIO
- STM32Cube Framework
- Serial Terminal (115200 baud)

---

## 📐 Rangkaian

### Wiring STM32F103C8T6

```
     STM32F103C8T6
    ┌─────────────────┐
    │                 │
    │ PA4 ──────────[LED1]──R330──GND (Timer 1)
    │ PA5 ──────────[LED2]──R330──GND (Timer 2)
    │ PA6 ──────────[LED3]──R330──GND (Timer 3)
    │ PA7 ──────────[LED4]──R330──GND (Notification)
    │                 │
    │ PB0 ──────[BTN1]──GND (w/internal pull-up)
    │ PB1 ──────[BTN2]──GND (w/internal pull-up)
    │                 │
    │ PA9 (TX) ──────── Serial Monitor
    │ PA10(RX) ──────── Serial Monitor
    │                 │
    └─────────────────┘
```

### Wiring ESP32

```
     ESP32 DevKit V1
    ┌─────────────────┐
    │                 │
    │ GPIO4 ─────────[LED1]──R330──GND
    │ GPIO16 ────────[LED2]──R330──GND
    │ GPIO17 ────────[LED3]──R330──GND
    │ GPIO5 ─────────[LED4]──R330──GND
    │                 │
    │ GPIO15 ────[BTN1]──GND (internal pull-up)
    │ GPIO2 ─────[BTN2]──GND (internal pull-up)
    │                 │
    │ USB ───────────── Serial Monitor
    │                 │
    └─────────────────┘
```

---

## 🔬 Percobaan 1: Basic Software Timer (STM32)

### Tujuan
Memahami pembuatan dan penggunaan software timer periodik dan one-shot.

### Konfigurasi PlatformIO
```ini
; platformio.ini
[env:bluepill_f103c8]
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
upload_protocol = stlink
build_flags = 
    -D HSE_VALUE=8000000U
    -D HAL_UART_MODULE_ENABLED
lib_deps =
monitor_speed = 115200
```

### Program 1: Timer LED Blink

```c
/* Percobaan 1: Basic Software Timer STM32
 * File: src/main.c
 * 
 * Demonstrasi software timer untuk blink LED
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

/* Timer Handles */
TimerHandle_t xLedTimer1 = NULL;   // Fast blink
TimerHandle_t xLedTimer2 = NULL;   // Slow blink
TimerHandle_t xOneShotTimer = NULL; // One-shot

/* Timer Callbacks */
void vLed1TimerCallback(TimerHandle_t xTimer)
{
    static uint32_t toggleCount = 0;
    toggleCount++;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
    printf("[T1@%lu] LED1 toggle #%lu\r\n", 
           (uint32_t)xTaskGetTickCount(), toggleCount);
}

void vLed2TimerCallback(TimerHandle_t xTimer)
{
    static uint32_t toggleCount = 0;
    toggleCount++;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    printf("[T2@%lu] LED2 toggle #%lu\r\n", 
           (uint32_t)xTaskGetTickCount(), toggleCount);
}

void vOneShotCallback(TimerHandle_t xTimer)
{
    printf("\n*** ONE-SHOT EXPIRED! ***\r\n");
    
    // Flash LED3 rapidly (non-blocking approach - set flag)
    for(int i = 0; i < 5; i++)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
        // Note: Dalam callback sebaiknya tidak delay
        // Ini hanya demo singkat
        for(volatile int j = 0; j < 100000; j++);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
        for(volatile int j = 0; j < 100000; j++);
    }
    
    printf("One-shot action completed\r\n\n");
}

/* Control Task */
void vControlTask(void *pvParameters)
{
    uint8_t rxChar;
    
    printf("\r\n=== Software Timer Demo ===\r\n");
    printf("Commands:\r\n");
    printf("  1 - Start/Stop Timer1 (200ms)\r\n");
    printf("  2 - Start/Stop Timer2 (1000ms)\r\n");
    printf("  3 - Trigger One-Shot (3s)\r\n");
    printf("  f - Timer1 Fast (100ms)\r\n");
    printf("  s - Timer1 Slow (500ms)\r\n");
    printf("  i - Info (timer status)\r\n\r\n");
    
    for(;;)
    {
        if(HAL_UART_Receive(&huart1, &rxChar, 1, 50) == HAL_OK)
        {
            switch(rxChar)
            {
                case '1':
                    if(xTimerIsTimerActive(xLedTimer1))
                    {
                        xTimerStop(xLedTimer1, pdMS_TO_TICKS(100));
                        printf("Timer1 STOPPED\r\n");
                    }
                    else
                    {
                        xTimerStart(xLedTimer1, pdMS_TO_TICKS(100));
                        printf("Timer1 STARTED\r\n");
                    }
                    break;
                    
                case '2':
                    if(xTimerIsTimerActive(xLedTimer2))
                    {
                        xTimerStop(xLedTimer2, pdMS_TO_TICKS(100));
                        printf("Timer2 STOPPED\r\n");
                    }
                    else
                    {
                        xTimerStart(xLedTimer2, pdMS_TO_TICKS(100));
                        printf("Timer2 STARTED\r\n");
                    }
                    break;
                    
                case '3':
                    xTimerStart(xOneShotTimer, pdMS_TO_TICKS(100));
                    printf("One-Shot timer started (3s countdown)\r\n");
                    break;
                    
                case 'f':
                    xTimerChangePeriod(xLedTimer1, pdMS_TO_TICKS(100), 
                                       pdMS_TO_TICKS(100));
                    printf("Timer1 period: 100ms (fast)\r\n");
                    break;
                    
                case 's':
                    xTimerChangePeriod(xLedTimer1, pdMS_TO_TICKS(500), 
                                       pdMS_TO_TICKS(100));
                    printf("Timer1 period: 500ms (slow)\r\n");
                    break;
                    
                case 'i':
                    printf("\n--- Timer Status ---\r\n");
                    printf("Timer1: %s (period: %lu ticks)\r\n",
                           xTimerIsTimerActive(xLedTimer1) ? "ACTIVE" : "DORMANT",
                           xTimerGetPeriod(xLedTimer1));
                    printf("Timer2: %s (period: %lu ticks)\r\n",
                           xTimerIsTimerActive(xLedTimer2) ? "ACTIVE" : "DORMANT",
                           xTimerGetPeriod(xLedTimer2));
                    printf("OneShot: %s\r\n",
                           xTimerIsTimerActive(xOneShotTimer) ? "ACTIVE" : "DORMANT");
                    printf("Free Heap: %lu bytes\r\n", xPortGetFreeHeapSize());
                    printf("--------------------\r\n\n");
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* System Configuration */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // LED outputs (PA4, PA5, PA6, PA7)
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Initialize LEDs OFF
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, 
                      GPIO_PIN_RESET);
}

void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // PA9 = TX, PA10 = RX
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);
}

int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n\n");
    printf("================================\r\n");
    printf(" FreeRTOS Software Timer Demo\r\n");
    printf("================================\r\n\n");
    
    /* Create Timer 1 - Fast periodic (200ms) */
    xLedTimer1 = xTimerCreate(
        "LED1_Timer",
        pdMS_TO_TICKS(200),
        pdTRUE,               // Auto-reload
        (void*)1,             // Timer ID
        vLed1TimerCallback
    );
    
    /* Create Timer 2 - Slow periodic (1000ms) */
    xLedTimer2 = xTimerCreate(
        "LED2_Timer",
        pdMS_TO_TICKS(1000),
        pdTRUE,
        (void*)2,
        vLed2TimerCallback
    );
    
    /* Create One-Shot Timer (3 seconds) */
    xOneShotTimer = xTimerCreate(
        "OneShot",
        pdMS_TO_TICKS(3000),
        pdFALSE,              // One-shot
        (void*)3,
        vOneShotCallback
    );
    
    if(xLedTimer1 != NULL && xLedTimer2 != NULL && xOneShotTimer != NULL)
    {
        printf("All timers created successfully!\r\n\n");
        
        /* Create control task */
        xTaskCreate(vControlTask, "Control", 256, NULL, 2, NULL);
        
        /* Start scheduler */
        vTaskStartScheduler();
    }
    else
    {
        printf("ERROR: Failed to create timers!\r\n");
    }
    
    for(;;);
}
```

### ✅ Tugas Percobaan 1
1. Amati output dan catat perbedaan timing Timer1 vs Timer2
2. Uji perubahan periode timer saat sedang berjalan
3. Amati behavior one-shot timer
4. Screenshot serial output setelah menekan 'i' untuk info

---

## 🔬 Percobaan 2: Timer dengan ID untuk Multi-Timer (STM32)

### Tujuan
Menggunakan Timer ID untuk mengelola beberapa timer dengan satu callback.

### Program 2: Multi-Timer dengan Shared Callback

```c
/* Percobaan 2: Multi-Timer dengan Shared Callback
 * File: src/main.c
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

#define NUM_TIMERS  4

/* LED pins */
const uint16_t ledPins[NUM_TIMERS] = {
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7
};

/* Timer periods in ms */
const uint32_t timerPeriods[NUM_TIMERS] = {
    100, 200, 500, 1000
};

TimerHandle_t xTimers[NUM_TIMERS];
uint32_t toggleCounts[NUM_TIMERS] = {0};

/* Shared Callback - Use Timer ID to identify */
void vSharedTimerCallback(TimerHandle_t xTimer)
{
    uint32_t timerId = (uint32_t)pvTimerGetTimerID(xTimer);
    
    if(timerId < NUM_TIMERS)
    {
        toggleCounts[timerId]++;
        HAL_GPIO_TogglePin(GPIOA, ledPins[timerId]);
        
        // Print setiap 10 toggle untuk mengurangi spam
        if(toggleCounts[timerId] % 10 == 0)
        {
            printf("[Timer%lu] Toggle count: %lu\r\n", 
                   timerId, toggleCounts[timerId]);
        }
    }
}

/* Status Task */
void vStatusTask(void *pvParameters)
{
    for(;;)
    {
        printf("\n=== Timer Status Report ===\r\n");
        
        for(int i = 0; i < NUM_TIMERS; i++)
        {
            printf("Timer%d: Period=%4lums, Active=%s, Count=%lu\r\n",
                   i,
                   timerPeriods[i],
                   xTimerIsTimerActive(xTimers[i]) ? "YES" : "NO ",
                   toggleCounts[i]);
        }
        
        printf("===========================\r\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* Control Task */
void vControlTask(void *pvParameters)
{
    uint8_t rxChar;
    
    printf("Commands: 0-3 toggle timer, a=all start, z=all stop\r\n\n");
    
    for(;;)
    {
        if(HAL_UART_Receive(&huart1, &rxChar, 1, 50) == HAL_OK)
        {
            if(rxChar >= '0' && rxChar <= '3')
            {
                int idx = rxChar - '0';
                
                if(xTimerIsTimerActive(xTimers[idx]))
                {
                    xTimerStop(xTimers[idx], pdMS_TO_TICKS(100));
                    printf("Timer%d STOPPED\r\n", idx);
                }
                else
                {
                    xTimerStart(xTimers[idx], pdMS_TO_TICKS(100));
                    printf("Timer%d STARTED\r\n", idx);
                }
            }
            else if(rxChar == 'a')
            {
                for(int i = 0; i < NUM_TIMERS; i++)
                {
                    xTimerStart(xTimers[i], pdMS_TO_TICKS(100));
                }
                printf("All timers STARTED\r\n");
            }
            else if(rxChar == 'z')
            {
                for(int i = 0; i < NUM_TIMERS; i++)
                {
                    xTimerStop(xTimers[i], pdMS_TO_TICKS(100));
                    HAL_GPIO_WritePin(GPIOA, ledPins[i], GPIO_PIN_RESET);
                }
                printf("All timers STOPPED\r\n");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Assume SystemClock_Config, MX_GPIO_Init, MX_USART1_UART_Init same as before */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n=== Multi-Timer with Shared Callback ===\r\n\n");
    
    /* Create all timers with same callback but different IDs */
    for(int i = 0; i < NUM_TIMERS; i++)
    {
        char timerName[16];
        sprintf(timerName, "Timer%d", i);
        
        xTimers[i] = xTimerCreate(
            timerName,
            pdMS_TO_TICKS(timerPeriods[i]),
            pdTRUE,
            (void*)(uint32_t)i,  // Timer ID = index
            vSharedTimerCallback // Shared callback
        );
        
        if(xTimers[i] != NULL)
        {
            printf("Created %s (period: %lums)\r\n", timerName, timerPeriods[i]);
        }
    }
    
    printf("\n");
    
    xTaskCreate(vControlTask, "Control", 256, NULL, 2, NULL);
    xTaskCreate(vStatusTask, "Status", 256, NULL, 1, NULL);
    
    vTaskStartScheduler();
    
    for(;;);
}
```

### ✅ Tugas Percobaan 2
1. Amati bagaimana satu callback handle 4 timer berbeda
2. Bandingkan frekuensi LED dan periode yang diset
3. Hitung overhead timer daemon dengan mengamati timing

---

## 🔬 Percobaan 3: Task Notification Basic (STM32)

### Tujuan
Memahami penggunaan task notification untuk signaling dari ISR.

### Program 3: Button Notification Handler

```c
/* Percobaan 3: Task Notification dari ISR
 * File: src/main.c
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

TaskHandle_t xButtonTask = NULL;

volatile uint32_t isrTriggerCount = 0;
volatile uint32_t processedCount = 0;

/* Button ISR Callback */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == GPIO_PIN_0)  // Button 1
    {
        isrTriggerCount++;
        
        /* Send notification to button task */
        vTaskNotifyGiveFromISR(xButtonTask, &xHigherPriorityTaskWoken);
        
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* Button Handler Task */
void vButtonTask(void *pvParameters)
{
    uint32_t notificationValue;
    
    printf("[ButtonTask] Ready - waiting for button press\r\n\n");
    
    for(;;)
    {
        /* Wait for notification - blocks until button pressed */
        notificationValue = ulTaskNotifyTake(
            pdTRUE,           // Clear count on exit
            portMAX_DELAY     // Wait forever
        );
        
        if(notificationValue > 0)
        {
            processedCount += notificationValue;
            
            printf("[ButtonTask] Received %lu notification(s)\r\n", 
                   notificationValue);
            printf("           ISR total: %lu, Processed: %lu\r\n\n",
                   isrTriggerCount, processedCount);
            
            /* Visual feedback - blink LED */
            for(uint32_t i = 0; i < notificationValue; i++)
            {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(100));
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

/* Monitor Task - Low priority background task */
void vMonitorTask(void *pvParameters)
{
    uint32_t lastProcessed = 0;
    
    for(;;)
    {
        if(processedCount != lastProcessed)
        {
            lastProcessed = processedCount;
            printf("[Monitor] Notifications per second: ~%.1f\r\n",
                   (float)processedCount / ((float)xTaskGetTickCount() / 1000.0f));
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    
    /* LED output (PA7) */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Button input with interrupt (PB0) */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // Trigger on press
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /* Enable EXTI interrupt */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);  // Priority > configMAX_SYSCALL_INTERRUPT_PRIORITY
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/* ISR Handler */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n=== Task Notification Demo ===\r\n");
    printf("Press the button to trigger notification\r\n\n");
    
    /* Create button task FIRST (to have valid handle for ISR) */
    xTaskCreate(vButtonTask, "Button", 256, NULL, 3, &xButtonTask);
    xTaskCreate(vMonitorTask, "Monitor", 128, NULL, 1, NULL);
    
    vTaskStartScheduler();
    
    for(;;);
}
```

### ✅ Tugas Percobaan 3
1. Tekan tombol beberapa kali dengan cepat dan amati notification count
2. Bandingkan ISR count vs processed count
3. Jelaskan mengapa bisa terjadi perbedaan

---

## 🔬 Percobaan 4: Notification dengan Event Bits (STM32)

### Tujuan
Menggunakan notification sebagai event flags untuk multiple events.

### Program 4: Multi-Event Notification

```c
/* Percobaan 4: Event Bits dengan Task Notification
 * File: src/main.c
 */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

/* Event bit definitions */
#define EVENT_BUTTON1     (1 << 0)
#define EVENT_BUTTON2     (1 << 1)
#define EVENT_TIMER       (1 << 2)
#define EVENT_SERIAL      (1 << 3)

TaskHandle_t xEventHandler = NULL;
TimerHandle_t xEventTimer = NULL;

/* Event counters */
volatile uint32_t button1Count = 0;
volatile uint32_t button2Count = 0;
volatile uint32_t timerCount = 0;
volatile uint32_t serialCount = 0;

/* Button ISR */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    if(GPIO_Pin == GPIO_PIN_0)  // Button 1
    {
        button1Count++;
        xTaskNotifyFromISR(xEventHandler, EVENT_BUTTON1, eSetBits,
                           &xHigherPriorityTaskWoken);
    }
    else if(GPIO_Pin == GPIO_PIN_1)  // Button 2
    {
        button2Count++;
        xTaskNotifyFromISR(xEventHandler, EVENT_BUTTON2, eSetBits,
                           &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* Timer Callback */
void vTimerCallback(TimerHandle_t xTimer)
{
    timerCount++;
    xTaskNotify(xEventHandler, EVENT_TIMER, eSetBits);
}

/* Event Handler Task */
void vEventHandlerTask(void *pvParameters)
{
    uint32_t notification;
    
    printf("[EventHandler] Ready - waiting for events...\r\n\n");
    
    for(;;)
    {
        /* Wait for any event bit */
        if(xTaskNotifyWait(
            0x00,           // Don't clear on entry
            0xFFFFFFFF,     // Clear all on exit
            &notification,
            portMAX_DELAY) == pdTRUE)
        {
            printf("[Event@%lu] Received: 0x%02lX\r\n", 
                   (uint32_t)xTaskGetTickCount(), notification);
            
            /* Process each event */
            if(notification & EVENT_BUTTON1)
            {
                printf("  -> Button1 event! (total: %lu)\r\n", button1Count);
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
            }
            
            if(notification & EVENT_BUTTON2)
            {
                printf("  -> Button2 event! (total: %lu)\r\n", button2Count);
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            }
            
            if(notification & EVENT_TIMER)
            {
                printf("  -> Timer event! (total: %lu)\r\n", timerCount);
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
            }
            
            if(notification & EVENT_SERIAL)
            {
                printf("  -> Serial event! (total: %lu)\r\n", serialCount);
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
            }
            
            printf("\n");
        }
    }
}

/* Serial Task - Monitors serial input */
void vSerialTask(void *pvParameters)
{
    uint8_t rxChar;
    
    for(;;)
    {
        if(HAL_UART_Receive(&huart1, &rxChar, 1, 50) == HAL_OK)
        {
            if(rxChar == 's')
            {
                serialCount++;
                xTaskNotify(xEventHandler, EVENT_SERIAL, eSetBits);
            }
            else if(rxChar == 't')
            {
                // Toggle timer
                if(xTimerIsTimerActive(xEventTimer))
                {
                    xTimerStop(xEventTimer, pdMS_TO_TICKS(100));
                    printf("[Serial] Timer STOPPED\r\n");
                }
                else
                {
                    xTimerStart(xEventTimer, pdMS_TO_TICKS(100));
                    printf("[Serial] Timer STARTED\r\n");
                }
            }
            else if(rxChar == 'i')
            {
                printf("\n=== Event Statistics ===\r\n");
                printf("Button1: %lu\r\n", button1Count);
                printf("Button2: %lu\r\n", button2Count);
                printf("Timer:   %lu\r\n", timerCount);
                printf("Serial:  %lu\r\n", serialCount);
                printf("========================\r\n\n");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    
    /* LED outputs */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Button inputs with interrupts */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void EXTI0_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }
void EXTI1_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1); }

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    printf("\r\n=== Multi-Event Notification Demo ===\r\n");
    printf("Events: Button1, Button2, Timer (t=toggle), Serial (s)\r\n");
    printf("Press 'i' for statistics\r\n\n");
    
    /* Create event handler task FIRST */
    xTaskCreate(vEventHandlerTask, "Events", 256, NULL, 3, &xEventHandler);
    xTaskCreate(vSerialTask, "Serial", 256, NULL, 2, NULL);
    
    /* Create periodic event timer */
    xEventTimer = xTimerCreate("EventTmr", pdMS_TO_TICKS(2000), 
                                pdTRUE, NULL, vTimerCallback);
    
    vTaskStartScheduler();
    
    for(;;);
}
```

### ✅ Tugas Percobaan 4
1. Trigger multiple event secara bersamaan dan amati hasilnya
2. Jelaskan keuntungan event bits vs multiple notification
3. Bandingkan dengan semaphore/queue untuk use case yang sama

---

## 🔬 Percobaan 5: Debounce Timer Pattern (ESP32)

### Tujuan
Mengimplementasikan debounce menggunakan software timer.

### Konfigurasi PlatformIO
```ini
; platformio.ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf
monitor_speed = 115200
```

### Program 5: Button Debounce dengan Timer

```c
/* Percobaan 5: Debounce Timer Pattern ESP32
 * File: src/main.c
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "DEBOUNCE";

#define BUTTON_PIN      GPIO_NUM_15
#define LED_PIN         GPIO_NUM_4
#define DEBOUNCE_MS     50

TimerHandle_t xDebounceTimer;
volatile bool buttonPressConfirmed = false;
volatile uint32_t rawPressCount = 0;
volatile uint32_t confirmedPressCount = 0;

/* Debounce Timer Callback */
void vDebounceCallback(TimerHandle_t xTimer) {
    // Timer expired without retriggering = stable input
    
    // Read actual button state
    if(gpio_get_level(BUTTON_PIN) == 0) {
        confirmedPressCount++;
        buttonPressConfirmed = true;
        printf("[Debounce] Button CONFIRMED! (#%lu)\n", confirmedPressCount);
    }
}

/* Button ISR */
static void IRAM_ATTR buttonISR(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    rawPressCount++;
    
    // Reset debounce timer setiap ada edge
    xTimerResetFromISR(xDebounceTimer, &xHigherPriorityTaskWoken);
    
    if(xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

/* LED Task - Responds to confirmed presses */
void LedTask(void *pvParameters) {
    for(;;) {
        if(buttonPressConfirmed) {
            buttonPressConfirmed = false;
            
            // Toggle LED
            static int ledState = 0;
            ledState = !ledState;
            gpio_set_level(LED_PIN, ledState);
            
            printf("[LED] Toggled! State: %s\n", ledState ? "ON" : "OFF");
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Statistics Task */
void StatsTask(void *pvParameters) {
    uint32_t lastRaw = 0;
    uint32_t lastConfirmed = 0;
    
    for(;;) {
        if(rawPressCount != lastRaw || confirmedPressCount != lastConfirmed) {
            printf("\n=== Debounce Statistics ===\n");
            printf("Raw ISR triggers:     %lu\n", rawPressCount);
            printf("Confirmed presses:    %lu\n", confirmedPressCount);
            printf("Noise filtered:       %lu (%.1f%%)\n",
                         rawPressCount - confirmedPressCount,
                         rawPressCount > 0 ? 
                         100.0f * (rawPressCount - confirmedPressCount) / rawPressCount : 0);
            printf("===========================\n\n");
            
            lastRaw = rawPressCount;
            lastConfirmed = confirmedPressCount;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    
    printf("\n=== ESP32 Debounce Timer Demo ===\n");
    printf("Debounce time: %d ms\n\n", DEBOUNCE_MS);
    
    // Create debounce timer (one-shot)
    xDebounceTimer = xTimerCreate(
        "Debounce",
        pdMS_TO_TICKS(DEBOUNCE_MS),
        pdFALSE,  // One-shot
        NULL,
        vDebounceCallback
    );
    
    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, buttonISR, NULL);
    
    // Create tasks
    xTaskCreate(LedTask, "LED", 2048, NULL, 2, NULL);
    xTaskCreate(StatsTask, "Stats", 2048, NULL, 1, NULL);
    
    printf("Press button to test debounce...\n\n");
}
```

### ✅ Tugas Percobaan 5
1. Tekan tombol dengan cepat dan amati filter ratio
2. Ubah DEBOUNCE_MS menjadi 10ms dan 100ms, bandingkan hasilnya
3. Jelaskan trade-off waktu debounce pendek vs panjang

---

## 🔬 Percobaan 6: Watchdog Timer Pattern (ESP32)

### Tujuan
Mengimplementasikan timeout watchdog menggunakan software timer.

### Program 6: Communication Watchdog

```c
/* Percobaan 6: Watchdog Timer Pattern ESP32
 * File: src/main.c
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "WATCHDOG";

#define LED_ALIVE       GPIO_NUM_4
#define LED_TIMEOUT     GPIO_NUM_16
#define WATCHDOG_MS     3000

TimerHandle_t xWatchdogTimer;
volatile bool communicationActive = false;
volatile uint32_t messageCount = 0;
volatile uint32_t timeoutCount = 0;

/* Watchdog Timeout Callback */
void vWatchdogCallback(TimerHandle_t xTimer) {
    timeoutCount++;
    communicationActive = false;
    
    printf("\n!!! WATCHDOG TIMEOUT !!!\n");
    printf("No data for %d ms\n", WATCHDOG_MS);
    printf("Total timeouts: %lu\n\n", timeoutCount);
    
    // Visual indication
    gpio_set_level(LED_TIMEOUT, 1);
    gpio_set_level(LED_ALIVE, 0);
}

/* Simulated Communication Task */
void CommTask(void *pvParameters) {
    printf("[Comm] Waiting for data...\n");
    printf("Send any character to simulate incoming data\n\n");
    
    uint8_t c;
    for(;;) {
        if(uart_read_bytes(UART_NUM_0, &c, 1, pdMS_TO_TICKS(10)) > 0) {
            messageCount++;
            
            // Reset watchdog
            xTimerReset(xWatchdogTimer, pdMS_TO_TICKS(100));
            
            // Mark communication as active
            if(!communicationActive) {
                communicationActive = true;
                gpio_set_level(LED_TIMEOUT, 0);
                printf("[Comm] Communication RESTORED!\n");
            }
            
            printf("[Comm] Received: '%c' (msg #%lu)\n", (char)c, messageCount);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Heartbeat Task - Visual feedback when alive */
void HeartbeatTask(void *pvParameters) {
    static int aliveState = 0;
    for(;;) {
        if(communicationActive) {
            aliveState = !aliveState;
            gpio_set_level(LED_ALIVE, aliveState);
        }
        
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

/* Status Task */
void StatusTask(void *pvParameters) {
    for(;;) {
        printf("\n--- Status ---\n");
        printf("Messages received: %lu\n", messageCount);
        printf("Timeouts: %lu\n", timeoutCount);
        printf("Communication: %s\n", 
                     communicationActive ? "ACTIVE" : "INACTIVE");
        printf("Timer active: %s\n",
                     xTimerIsTimerActive(xWatchdogTimer) ? "YES" : "NO");
        printf("--------------\n\n");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    // Configure GPIO
    gpio_reset_pin(LED_ALIVE);
    gpio_set_direction(LED_ALIVE, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_TIMEOUT);
    gpio_set_direction(LED_TIMEOUT, GPIO_MODE_OUTPUT);
    
    gpio_set_level(LED_ALIVE, 0);
    gpio_set_level(LED_TIMEOUT, 1);  // Start in timeout state
    
    printf("\n=== ESP32 Watchdog Timer Demo ===\n");
    printf("Watchdog timeout: %d ms\n", WATCHDOG_MS);
    printf("Send characters to keep communication alive\n\n");
    
    // Create watchdog timer (one-shot)
    xWatchdogTimer = xTimerCreate(
        "Watchdog",
        pdMS_TO_TICKS(WATCHDOG_MS),
        pdFALSE,  // One-shot
        NULL,
        vWatchdogCallback
    );
    
    // Start watchdog (will timeout if no data)
    xTimerStart(xWatchdogTimer, 0);
    
    // Create tasks
    xTaskCreate(CommTask, "Comm", 4096, NULL, 2, NULL);
    xTaskCreate(HeartbeatTask, "Heartbeat", 2048, NULL, 1, NULL);
    xTaskCreate(StatusTask, "Status", 2048, NULL, 1, NULL);
}
```

### ✅ Tugas Percobaan 6
1. Amati LED saat tidak ada input vs saat ada input
2. Berhenti mengirim data dan amati timeout
3. Hitung waktu exact dari timeout (gunakan millis())

---

## 🔬 Percobaan 7: Timer + Notification Kombinasi (ESP32)

### Tujuan
Mengkombinasikan software timer dengan task notification untuk event-driven system.

### Program 7: Multi-Source Event Handler

```c
/* Percobaan 7: Timer + Notification Kombinasi ESP32
 * File: src/main.c
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "EVT_HANDLER";

#define BUTTON_PIN  GPIO_NUM_15
#define LED1_PIN    GPIO_NUM_4
#define LED2_PIN    GPIO_NUM_16
#define LED3_PIN    GPIO_NUM_17
#define LED4_PIN    GPIO_NUM_5

/* Event definitions */
#define EVT_BUTTON      (1 << 0)
#define EVT_TIMER_1S    (1 << 1)
#define EVT_TIMER_5S    (1 << 2)
#define EVT_SERIAL      (1 << 3)

TaskHandle_t xEventHandler = NULL;
TimerHandle_t xTimer1s = NULL;
TimerHandle_t xTimer5s = NULL;
TimerHandle_t xDebounceTimer = NULL;

/* Event statistics */
struct {
    uint32_t button;
    uint32_t timer1s;
    uint32_t timer5s;
    uint32_t serial;
} eventCounts = {0};

/* 1 Second Timer Callback */
void vTimer1sCallback(TimerHandle_t xTimer) {
    eventCounts.timer1s++;
    xTaskNotify(xEventHandler, EVT_TIMER_1S, eSetBits);
}

/* 5 Second Timer Callback */
void vTimer5sCallback(TimerHandle_t xTimer) {
    eventCounts.timer5s++;
    xTaskNotify(xEventHandler, EVT_TIMER_5S, eSetBits);
}

/* Debounce Timer Callback */
void vDebounceCallback(TimerHandle_t xTimer) {
    if(gpio_get_level(BUTTON_PIN) == 0) {
        eventCounts.button++;
        xTaskNotify(xEventHandler, EVT_BUTTON, eSetBits);
    }
}

/* Button ISR */
static void IRAM_ATTR buttonISR(void *arg) {
    BaseType_t woken = pdFALSE;
    xTimerResetFromISR(xDebounceTimer, &woken);
    if(woken) portYIELD_FROM_ISR();
}

/* Central Event Handler */
void EventHandlerTask(void *pvParameters) {
    uint32_t events;
    
    printf("[Handler] Started - waiting for events...\n\n");
    
    for(;;) {
        if(xTaskNotifyWait(0, 0xFFFFFFFF, &events, portMAX_DELAY) == pdTRUE) {
            
            printf("\n[Event@%lld] Flags: 0x%02lX\n", esp_timer_get_time()/1000, events);
            
            if(events & EVT_BUTTON) {
                printf("  [BUTTON] Press #%lu\n", eventCounts.button);
                static int led1 = 0; led1 = !led1;
                gpio_set_level(LED1_PIN, led1);
            }
            
            if(events & EVT_TIMER_1S) {
                printf("  [1S] Tick #%lu\n", eventCounts.timer1s);
                static int led2 = 0; led2 = !led2;
                gpio_set_level(LED2_PIN, led2);
            }
            
            if(events & EVT_TIMER_5S) {
                printf("  [5S] Tick #%lu\n", eventCounts.timer5s);
                
                // Flash LED3 rapidly
                for(int i = 0; i < 5; i++) {
                    gpio_set_level(LED3_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    gpio_set_level(LED3_PIN, 0);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
            
            if(events & EVT_SERIAL) {
                printf("  [SERIAL] Event #%lu\n", eventCounts.serial);
                static int led4 = 0; led4 = !led4;
                gpio_set_level(LED4_PIN, led4);
            }
        }
    }
}

/* Serial Monitor Task */
void SerialTask(void *pvParameters) {
    uint8_t cmd;
    for(;;) {
        if(uart_read_bytes(UART_NUM_0, &cmd, 1, pdMS_TO_TICKS(10)) > 0) {
            switch((char)cmd) {
                case 's':
                    eventCounts.serial++;
                    xTaskNotify(xEventHandler, EVT_SERIAL, eSetBits);
                    break;
                    
                case '1':
                    if(xTimerIsTimerActive(xTimer1s)) {
                        xTimerStop(xTimer1s, pdMS_TO_TICKS(100));
                        printf("[Cmd] 1s timer STOPPED\n");
                    } else {
                        xTimerStart(xTimer1s, pdMS_TO_TICKS(100));
                        printf("[Cmd] 1s timer STARTED\n");
                    }
                    break;
                    
                case '5':
                    if(xTimerIsTimerActive(xTimer5s)) {
                        xTimerStop(xTimer5s, pdMS_TO_TICKS(100));
                        printf("[Cmd] 5s timer STOPPED\n");
                    } else {
                        xTimerStart(xTimer5s, pdMS_TO_TICKS(100));
                        printf("[Cmd] 5s timer STARTED\n");
                    }
                    break;
                    
                case 'i':
                    printf("\n=== Event Statistics ===\n");
                    printf("Button events:  %lu\n", eventCounts.button);
                    printf("1s timer ticks: %lu\n", eventCounts.timer1s);
                    printf("5s timer ticks: %lu\n", eventCounts.timer5s);
                    printf("Serial events:  %lu\n", eventCounts.serial);
                    printf("\nTimer1s: %s\n", 
                                 xTimerIsTimerActive(xTimer1s) ? "ACTIVE" : "STOPPED");
                    printf("Timer5s: %s\n", 
                                 xTimerIsTimerActive(xTimer5s) ? "ACTIVE" : "STOPPED");
                    printf("========================\n\n");
                    break;
                    
                case 'h':
                    printf("\n=== Commands ===\n");
                    printf("s - Send serial event\n");
                    printf("1 - Toggle 1s timer\n");
                    printf("5 - Toggle 5s timer\n");
                    printf("i - Info/statistics\n");
                    printf("================\n\n");
                    break;
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
    
    gpio_num_t leds[] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};
    for(int i = 0; i < 4; i++) {
        gpio_reset_pin(leds[i]);
        gpio_set_direction(leds[i], GPIO_MODE_OUTPUT);
    }
    
    printf("\n=== ESP32 Timer + Notification Demo ===\n");
    printf("Press 'h' for help\n\n");
    
    // Create event handler task first
    xTaskCreate(EventHandlerTask, "Events", 4096, NULL, 3, &xEventHandler);
    xTaskCreate(SerialTask, "Serial", 2048, NULL, 2, NULL);
    
    // Create timers
    xTimer1s = xTimerCreate("1s", pdMS_TO_TICKS(1000), pdTRUE, NULL, vTimer1sCallback);
    xTimer5s = xTimerCreate("5s", pdMS_TO_TICKS(5000), pdTRUE, NULL, vTimer5sCallback);
    xDebounceTimer = xTimerCreate("Debounce", pdMS_TO_TICKS(50), pdFALSE, NULL, vDebounceCallback);
    
    // Start timers
    xTimerStart(xTimer1s, 0);
    xTimerStart(xTimer5s, 0);
    
    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, buttonISR, NULL);
}
```

### ✅ Tugas Percobaan 7
1. Amati kombinasi events saat multiple sources trigger bersamaan
2. Jelaskan keuntungan central event handler
3. Modifikasi untuk menambah event baru (sensor, dll)

---

## 📝 Laporan Praktikum

### Format Laporan
1. **Cover** - Identitas dan judul praktikum
2. **Tujuan** - Capaian pembelajaran modul
3. **Dasar Teori** - Ringkasan materi timer dan notification
4. **Hasil Percobaan** - Screenshot dan output per percobaan
5. **Analisis** - Jawaban tugas dan pembahasan
6. **Kesimpulan** - Rangkuman pembelajaran

### Pertanyaan Analisis
1. Apa perbedaan fundamental software timer vs hardware timer?
2. Kapan sebaiknya menggunakan task notification vs semaphore?
3. Jelaskan mengapa timer callback tidak boleh blocking!
4. Apa keuntungan event-driven architecture dengan notification?
5. Bagaimana debounce timer meningkatkan reliability input?

---

## 📚 Referensi

1. FreeRTOS Timer API: https://freertos.org/FreeRTOS-timers-xTimerCreate.html
2. FreeRTOS Task Notification: https://freertos.org/RTOS-task-notifications.html
3. STM32 HAL Documentation
4. ESP-IDF FreeRTOS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html

---

*Jobsheet Modul 11 - FreeRTOS Software Timer dan Task Notification*
