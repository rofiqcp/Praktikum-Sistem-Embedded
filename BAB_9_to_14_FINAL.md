# BAB 9-14: LANJUTAN PEMBELAJARAN STM32 vs ESP32

---

## BAB 9: DMA & MEMORY MANAGEMENT

> **Tujuan Pembelajaran:**
> - Direct Memory Access untuk zero-CPU transfer
> - DMA modes (normal, circular, memory-to-memory)
> - Configure DMA untuk UART, SPI, ADC
> - Memory architecture & optimization
> - Cache management (STM32F7/H7)

### 📖 PENJELASAN MATERI

**DMA (Direct Memory Access):**
Hardware yang transfer data tanpa CPU intervention:
```
Without DMA:              With DMA:
CPU reads peripheral  →   DMA reads peripheral
CPU writes to memory      DMA writes to memory
CPU busy (blocking)       CPU free (concurrent)
```

**DMA Modes:**
- **Normal**: Single transfer, stop when complete
- **Circular**: Loop buffer, continuous transfer
- **Memory-to-Memory**: RAM to RAM copy

---

### 🔗 COMMON GROUND: Konsep DMA yang Mirip

#### 1. **Buffer Transfer Concept**
**Deskripsi:** Transfer data array

**Contoh Program STM32:**
- **"UART DMA TX"** - HAL_UART_Transmit_DMA() untuk kirim buffer
- **"ADC DMA Array"** - Collect 100 samples tanpa CPU
- **"SPI DMA Display"** - Update TFT dengan DMA

**Contoh Program ESP32:**
- **"I2S DMA Audio"** - Stream audio buffer via I2S DMA
- **"SPI Transaction Queue"** - Pseudo-DMA untuk SPI
- **"UART TX Buffer"** - Large buffer transmission

#### 2. **Circular Buffer Pattern**
**Deskripsi:** Continuous data streaming

**Contoh Program STM32:**
- **"ADC Circular DMA"** - Continuous sampling, loop buffer
- **"UART RX Circular"** - Parse data while DMA fills
- **"Double Buffer Audio"** - Ping-pong buffer audio

**Contoh Program ESP32:**
- **"Ring Buffer UART"** - Software circular buffer
- **"I2S Circular Buffer"** - Audio in/out streaming
- **"ADC Continuous via I2S"** - Loop sampling

#### 3. **Zero-Copy Transfer**
**Deskripsi:** Direct peripheral-to-memory

**Contoh Program STM32:**
- **"Peripheral to RAM DMA"** - ADC → RAM tanpa CPU
- **"RAM to Peripheral"** - Buffer → UART via DMA
- **"Memory to Memory"** - Fast memcpy via DMA

**Contoh Program ESP32:**
- **"I2S to Buffer"** - Audio stream ke RAM
- **"SPI Large Transfer"** - Transaction queue minimizes CPU
- **"WiFi TX Zero-Copy"** - lwIP buffer direct to WiFi

---

### ⭐ STM32 DMA: Keunggulan Unik

#### 1. **Full DMA Support Across Peripherals**
**Contoh Program:**
- **"UART RX/TX DMA"** - Both directions dengan DMA
- **"SPI Full-Duplex DMA"** - Simultaneous TX+RX DMA
- **"I2C DMA Transfer"** - Master TX/RX via DMA
- **"ADC Multi-Channel DMA"** - 8 channels ke array
- **"DAC DMA Waveform"** - Generate arbitrary waveform
- **"Timer DMA Burst"** - Update timer registers via DMA

**Keunggulan:** ✅ True hardware DMA untuk semua peripheral

#### 2. **Memory-to-Memory DMA**
**Contoh Program:**
- **"Fast Memcpy"** - DMA2 M2M transfer (2x faster)
- **"Image Buffer Copy"** - Copy framebuffer DMA
- **"Data Restructuring"** - Rearrange data tanpa CPU

#### 3. **DMA Request Mapping (DMAMUX)**
**Contoh Program (STM32L4/G4/H7):**
- **"Flexible DMA Routing"** - Route any peripheral to any DMA
- **"DMA Channel Optimization"** - No fixed peripheral-channel
- **"Advanced DMA Config"** - Multiple peripherals share DMA

#### 4. **Nested DMA Interrupts**
**Contoh Program:**
- **"Half Transfer + Complete Callback"** - Process while DMA continues
- **"Error Handling"** - DMA error interrupt recovery
- **"Priority DMA"** - High-priority DMA preempts low-priority

#### 5. **Cache-Coherent DMA (F7/H7)**
**Contoh Program:**
- **"DMA + Cache Mgmt"** - SCB_CleanDCache_by_Addr()
- **"DMA Buffer in D2 RAM"** - Avoid cache issues
- **"Ethernet DMA"** - High-performance network DMA

---

### ⚡ ESP32 Memory: Keunggulan Unik

#### 1. **Multiple RAM Regions**
**Contoh Program:**
- **"IRAM vs DRAM"** - Instruction RAM untuk ISR, Data RAM untuk data
- **"RTC Slow/Fast Memory"** - Persistent across deep sleep
- **"PSRAM (External)"** - 4-8 MB external PSRAM support

#### 2. **I2S DMA (Audio/ADC)**
**Contoh Program:**
- **"I2S DMA Streaming"** - Up to 150 kHz ADC via I2S
- **"PDM Microphone"** - Digital mic input via I2S DMA
- **"I2S DAC Output"** - Audio playback dengan DMA

#### 3. **Zero-Copy WiFi**
**Contoh Program:**
- **"lwIP Zero-Copy"** - Pointer passing, no memcpy
- **"TCP Fast TX"** - Direct buffer to WiFi stack
- **"UDP Burst"** - High-speed transmission

#### 4. **Memory Protection Unit (MPU)**
**Contoh Program:**
- **"Region Protection"** - Protect critical RAM regions
- **"Stack Overflow Detect"** - MPU triggers on overflow
- **"Security Boundary"** - Isolate sensitive data

---

### 📊 DMA COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **DMA Channels** | 7-16 (series) | Limited (I2S, SPI pseudo) |
| **Peripheral DMA** | ✅ UART/SPI/I2C/ADC/DAC/TIM | ⚠️ I2S only (true DMA) |
| **Circular Mode** | ✅ Hardware | ⚠️ Software management |
| **Memory-to-Memory** | ✅ Supported | ❌ No |
| **DMA Request Mux** | ✅ DMAMUX (L4/G4/H7) | ❌ No |
| **Cache Management** | ✅ F7/H7 | ✅ Automatic (mostly) |
| **PSRAM** | ❌ No | ✅ 4-8 MB external |
| **RTC Memory** | ⚠️ Backup RAM | ✅ 8KB RTC slow/fast |

---

### 💼 MINI PROJECT: High-Speed Data Acquisition

**STM32 Implementation:**
- **"ADC DMA @ 1 MHz"** - Continuous sampling 4 channels
- **"SD Card DMA Write"** - Stream data to SD card via DMA
- **"Real-Time FFT"** - Process in background, DMA fills buffer

**ESP32 Implementation:**
- **"I2S ADC @ 150 kHz"** - Use I2S for high-speed ADC
- **"WiFi Streaming"** - Real-time data to TCP client
- **"PSRAM Buffer"** - Store large dataset di external RAM

---

## BAB 10: CLOCK SYSTEM & TIMING CONFIGURATION

> **Tujuan Pembelajaran:**
> - Clock tree architecture
> - HSI, HSE, LSI, LSE clock sources
> - PLL configuration untuk max performance
> - Clock gating untuk power saving
> - RTC & timer clocking

### 📖 PENJELASAN MATERI

**Clock Tree:**
```
STM32:
HSE (8 MHz) ─→ PLL ─→ SYSCLK (up to 180 MHz F4, 480 MHz H7)
               ↓
            AHB ─→ APB1 (low-speed peripherals)
                ─→ APB2 (high-speed peripherals)

ESP32:
XTAL (40 MHz) ─→ PLL ─→ CPU CLK (240 MHz max)
                      ─→ APB CLK (80 MHz)
```

**Clock Sources:**
- **HSI/HSE**: High-speed internal/external (STM32)
- **LSI/LSE**: Low-speed internal/external (32 kHz for RTC)
- **XTAL**: Crystal oscillator (ESP32)
- **PLL**: Phase-Locked Loop untuk multiply frequency

---

### 🔗 COMMON GROUND: Clock Concepts

#### 1. **System Clock Configuration**
**Contoh Program STM32:**
- **"Max Speed Config"** - 180 MHz (F4), 480 MHz (H7)
- **"PLL Calculator"** - Calculate PLL_M, PLL_N, PLL_P
- **"Clock Switch Runtime"** - Change frequency on-the-fly

**Contoh Program ESP32:**
- **"CPU Frequency Set"** - 240/160/80 MHz via esp_pm
- **"Dynamic Freq Scaling"** - 240 MHz busy, 80 MHz idle
- **"XTAL Frequency"** - 40 MHz atau 26 MHz support

#### 2. **Peripheral Clock Enable**
**Contoh Program STM32:**
- **"RCC_Enable"** - __HAL_RCC_GPIOx_CLK_ENABLE()
- **"Clock Gating"** - Disable unused peripheral clocks
- **"Power Optimization"** - Enable only needed clocks

**Contoh Program ESP32:**
- **"Peripheral Module Enable"** - periph_module_enable()
- **"Power Domain"** - pd_enable/disable domains
- **"Clock Source Select"** - APB/REF/RTC clock choice

#### 3. **RTC Clock Configuration**
**Contoh Program STM32:**
- **"LSE for RTC"** - 32.768 kHz external crystal
- **"LSI Backup"** - Internal RC if LSE fails
- **"RTC Calibration"** - Compensate crystal drift

**Contoh Program ESP32:**
- **"RTC from Internal"** - 150 kHz RC oscillator
- **"32K XTAL"** - External 32 kHz for accuracy
- **"RTC Slow/Fast CLK"** - Different sources

---

### ⭐ STM32 Clock: Keunggulan Unik

#### 1. **Multiple PLL Outputs**
**Contoh Program:**
- **"PLL_P for SYSCLK"** - Main system clock
- **"PLL_Q for USB"** - 48 MHz precise USB clock
- **"PLL_R for ADC"** - Independent ADC clock
- **"PLLSAI for LCD"** - Dedicated display clock (F4/F7)

#### 2. **CSS (Clock Security System)**
**Contoh Program:**
- **"HSE Failure Detection"** - Auto switch to HSI on fail
- **"NMI Interrupt"** - CSS triggers NMI
- **"Fault Recovery"** - Reconfigure clocks safely

#### 3. **MCO (Master Clock Output)**
**Contoh Program:**
- **"Clock Output Pin"** - Output SYSCLK/PLL ke PA8
- **"Debug Clock"** - Monitor clock dengan oscilloscope
- **"Clock External Device"** - Provide clock ke peripheral

#### 4. **Fine-Grained Clock Control**
**Contoh Program:**
- **"APB1/APB2 Prescaler"** - Independent bus speeds
- **"AHB Prescaler"** - /1, /2, /4, /8, /16...
- **"Peripheral Clock Source"** - Multiple options per peripheral

---

### ⚡ ESP32 Clock: Keunggulan Unik

#### 1. **Dynamic Frequency Scaling**
**Contoh Program:**
- **"Power Management"** - esp_pm_configure()
- **"Auto Freq Adjust"** - 240 MHz → 80 MHz saat idle
- **"WiFi-Aware Scaling"** - Min 80 MHz untuk WiFi

#### 2. **8 MHz Clock for Low Power**
**Contoh Program:**
- **"8 MHz CPU Mode"** - Ultra-low power operation
- **"Light Sleep Freq"** - Reduce to 8 MHz before sleep
- **"Battery Optimization"** - Extend runtime dramatically

#### 3. **RTC FAST/SLOW Memory Clock**
**Contoh Program:**
- **"RTC Memory Persistent"** - Keep data across deep sleep
- **"Fast Access"** - RTC_FAST for quick wake code
- **"Slow for Variables"** - RTC_SLOW untuk stored data

---

### 📊 CLOCK COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Max CPU Frequency** | 180-480 MHz | 240 MHz |
| **PLL Outputs** | 3-4 (P/Q/R/SAI) | 1 main PLL |
| **Dynamic Freq Scale** | ⚠️ Manual | ✅ Automatic (esp_pm) |
| **Clock Security** | ✅ CSS (HSE monitor) | ⚠️ Limited |
| **Master Clock Out** | ✅ MCO1/MCO2 | ✅ CLK_OUT |
| **RTC Clock Options** | ✅ LSI/LSE/HSE/32 | ✅ Internal/32K XTAL |
| **Low Power Modes** | ✅ Stop/Standby | ✅ Light/Deep Sleep |

---

### 💼 MINI PROJECT: Power-Optimized Logger

**STM32 Implementation:**
- **"Dynamic Clock Adjust"** - 180 MHz sampling, 16 MHz idle
- **"RTC Wakeup"** - Sleep, wake every 10 seconds
- **"Clock Gating"** - Disable unused peripherals

**ESP32 Implementation:**
- **"WiFi Power Save"** - 240 MHz TX, 80 MHz RX, 10 MHz idle
- **"Deep Sleep Schedule"** - Wake every hour, log, sleep
- **"Battery Life Calculation"** - Optimize current consumption

---

## BAB 11: FreeRTOS - MULTITASKING BASICS

> **Tujuan Pembelajaran:**
> - Task creation & scheduling
> - Task priority & preemption
> - Task states (running, ready, blocked, suspended)
> - Delay & timing functions
> - Idle task & tick hook

### 📖 PENJELASAN MATERI

**FreeRTOS Scheduling:**
```
Priority-based preemptive scheduler:

High Priority ─→ Task A ─→ Running
                 Task B ─→ Ready (preempted)
Low Priority  ─→ Task C ─→ Blocked (waiting)
```

**Task States:**
```
        xTaskCreate()
            ↓
    ┌───→ READY ←──────┐
    │      ↓           │
    │   RUNNING        │
    │      ↓           │
    │   BLOCKED  ──────┘  (delay/wait)
    │      ↓
    └── SUSPENDED  (vTaskSuspend)
         ↓
      DELETED (vTaskDelete)
```

---

### 🔗 COMMON GROUND: FreeRTOS Features

#### 1. **Task Creation**
**Contoh Program STM32:**
- **"xTaskCreate LED Blink"** - 2 tasks blink different LEDs
- **"Task Stack Size"** - Allocate proper stack
- **"Task Priority"** - osPriorityNormal/High/Low

**Contoh Program ESP32:**
- **"xTaskCreatePinnedToCore"** - Pin task ke Core 0/1
- **"Multiple Tasks"** - 5 concurrent tasks
- **"Task Handle"** - Store handle untuk control

#### 2. **Task Delay**
**Contoh Program STM32:**
- **"vTaskDelay()"** - Blocking delay (ticks)
- **"osDelay()"** - CMSIS-RTOS wrapper
- **"vTaskDelayUntil()"** - Precise periodic execution

**Contoh Program ESP32:**
- **"vTaskDelay()"** - pdMS_TO_TICKS(1000)
- **"Periodic Task"** - Run exactly every 100 ms
- **"Non-Blocking Delay"** - Use millis() alternative

#### 3. **Task Priority**
**Contoh Program STM32:**
- **"Priority Levels"** - 0 (idle) to configMAX_PRIORITIES-1
- **"Preemption Demo"** - High priority interrupts low
- **"Priority Inversion"** - Mutex inheritance fix

**Contoh Program ESP32:**
- **"Core Affinity + Priority"** - Core 0 high priority WiFi
- **"Task Watchdog"** - Detect stalled high-priority tasks
- **"Balanced Priorities"** - Avoid starvation

#### 4. **Task Suspend/Resume**
**Contoh Program STM32:**
- **"vTaskSuspend()"** - Pause task execution
- **"vTaskResume()"** - Continue suspended task
- **"Dynamic Task Control"** - Enable/disable features

**Contoh Program ESP32:**
- **"Conditional Task"** - Suspend when not needed
- **"Power Saving"** - Suspend background tasks
- **"Event-Driven Suspend"** - Resume on interrupt

#### 5. **Idle Hook & Tick Hook**
**Contoh Program STM32:**
- **"vApplicationIdleHook()"** - Run when no tasks ready
- **"vApplicationTickHook()"** - Called every tick
- **"CPU Load Monitoring"** - Measure idle time percentage

**Contoh Program ESP32:**
- **"Idle Task Hook"** - Monitor CPU usage
- **"Watchdog Feeding"** - Feed watchdog in idle
- **"Power Management"** - Enter light sleep in idle

---

### ⭐ STM32 FreeRTOS: Keunggulan Unik

#### 1. **CMSIS-RTOS Wrapper**
**Contoh Program:**
- **"osThreadNew()"** - CMSIS-RTOS2 API (portable)
- **"osDelay()"** - Standard delay function
- **"osMutex/osSemaphore"** - Consistent API

#### 2. **Static Memory Allocation**
**Contoh Program:**
- **"StaticTask_t"** - Pre-allocate task control block
- **"Static Stack"** - No heap fragmentation
- **"Deterministic Memory"** - Safety-critical systems

#### 3. **MPU Support (Cortex-M4/M7)**
**Contoh Program:**
- **"Memory Protection"** - Isolate task stacks
- **"Privileged vs Unprivileged"** - Secure tasks
- **"Fault Detection"** - MPU violations trigger fault

---

### ⚡ ESP32 FreeRTOS: Keunggulan Unik

#### 1. **Dual-Core Scheduling**
**Contoh Program:**
- **"xTaskCreatePinnedToCore()"** - Core 0 atau Core 1
- **"Core 0 for WiFi"** - Dedicate core untuk WiFi
- **"Core 1 for App"** - User code di core 1
- **"Load Balancing"** - Distribute tasks evenly

#### 2. **Built-in ESP32 Modifications**
**Contoh Program:**
- **"Tick Rate 100 Hz"** - (vs 1000 Hz default FreeRTOS)
- **"Task Watchdog Timer"** - Auto-detect hung tasks
- **"Interrupt Watchdog"** - Detect ISR hangs

#### 3. **Integration dengan WiFi/BT**
**Contoh Program:**
- **"WiFi Task"** - Internal task untuk WiFi stack
- **"BT Controller Task"** - Bluetooth operation
- **"Priority Coordination"** - App tasks below WiFi priority

---

### 📊 FreeRTOS COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **FreeRTOS Version** | v10.x (CMSIS-RTOS) | v10.x (ESP-IDF modified) |
| **SMP (Multi-Core)** | ❌ No (single core) | ✅ Dual-core support |
| **Task Pinning** | ❌ N/A | ✅ Core 0/1 selection |
| **CMSIS-RTOS** | ✅ CMSIS-RTOS2 | ❌ Native FreeRTOS API |
| **Static Allocation** | ✅ Supported | ✅ Supported |
| **MPU Support** | ✅ M4/M7 | ⚠️ Limited |
| **Task Watchdog** | ⚠️ Manual | ✅ Built-in (TWDT) |
| **Tick Rate** | 1000 Hz default | 100 Hz (configurable) |

---

### 💼 MINI PROJECT: Multi-Task Sensor System

**STM32 Implementation:**
- Task 1: "Sensor Read" (high priority) - ADC sampling
- Task 2: "Display Update" (medium) - OLED refresh
- Task 3: "SD Card Log" (low) - Write to SD every 10s
- Task 4: "Button Handler" (highest) - User input

**ESP32 Implementation:**
- Core 0 Task 1: "WiFi Manager" - Handle WiFi/MQTT
- Core 0 Task 2: "Webserver" - HTTP requests
- Core 1 Task 1: "Sensor Read" - Read I2C sensors
- Core 1 Task 2: "Display Update" - Update TFT
- Core 1 Task 3: "LED Indicator" - Status LED

---

## BAB 12: FreeRTOS - IPC & SYNCHRONIZATION

> **Tujuan Pembelajaran:**
> - Queue untuk inter-task communication
> - Semaphore (binary, counting, mutex)
> - Event groups untuk task synchronization
> - Stream buffers & message buffers
> - Critical sections

### 📖 PENJELASAN MATERI

**IPC Mechanisms:**
```
Queue:      Task A ─→ [Data] ─→ Task B
Semaphore:  Task A gives → Task B takes
Mutex:      Lock resource, unlock after use
Event:      Task A sets bit → Task B waits for bit
```

**Use Cases:**
- **Queue**: Producer-consumer pattern
- **Binary Semaphore**: Signaling (ISR → Task)
- **Counting Semaphore**: Resource pool (e.g., 5 buffers available)
- **Mutex**: Protect shared resource (e.g., UART, I2C)
- **Event Group**: Multiple conditions (WiFi connected + Time synced)

---

### 🔗 COMMON GROUND: IPC Features

#### 1. **Queue (Message Passing)**
**Contoh Program STM32:**
- **"xQueueCreate()"** - Create queue for sensor data
- **"xQueueSend() from ISR"** - Send from interrupt
- **"xQueueReceive()"** - Block until data available
- **"Sensor Data Pipeline"** - ADC → Queue → Processing Task

**Contoh Program ESP32:**
- **"UART Event Queue"** - uart_driver creates queue
- **"WiFi Event Queue"** - System event queue
- **"Custom Message Queue"** - Inter-task messages

#### 2. **Binary Semaphore (Signaling)**
**Contoh Program STM32:**
- **"xSemaphoreCreateBinary()"** - Create semaphore
- **"ISR gives, Task takes"** - Interrupt notifies task
- **"Button Press Signal"** - Debounced button via semaphore

**Contoh Program ESP32:**
- **"GPIO ISR Semaphore"** - gpio_isr → xSemaphoreGiveFromISR()
- **"Task Sync"** - Wait for external event
- **"Timer Semaphore"** - Periodic wakeup via timer callback

#### 3. **Mutex (Resource Protection)**
**Contoh Program STM32:**
- **"xSemaphoreCreateMutex()"** - Protect UART
- **"Priority Inheritance"** - Avoid priority inversion
- **"Shared I2C Bus"** - Multiple tasks, 1 I2C

**Contoh Program ESP32:**
- **"SPI Mutex"** - Multiple tasks use same SPI
- **"Global Variable Protection"** - Lock before access
- **"File System Mutex"** - SD card concurrent access

#### 4. **Counting Semaphore (Resource Pool)**
**Contoh Program STM32:**
- **"Buffer Pool"** - 5 buffers, semaphore count=5
- **"Connection Limit"** - Max 3 clients (semaphore=3)
- **"Parking Lot"** - 10 slots available

**Contoh Program ESP32:**
- **"WiFi TX Buffers"** - Limit concurrent transmissions
- **"Heap Memory Blocks"** - Track available blocks
- **"Task Throttling"** - Limit active workers

#### 5. **Event Groups (Multi-Condition)**
**Contoh Program STM32:**
- **"xEventGroupCreate()"** - 24 event bits
- **"Wait for Multiple Events"** - WiFi + Time + Data ready
- **"Synchronization Point"** - All tasks reach checkpoint

**Contoh Program ESP32:**
- **"WiFi Event Group"** - CONNECTED_BIT | FAIL_BIT
- **"Sensor Ready Flags"** - Temp ready | Humidity ready
- **"Boot Sequence"** - Wait for all init complete

---

### ⭐ STM32 FreeRTOS IPC: Keunggulan Unik

#### 1. **CMSIS-RTOS Wrappers**
**Contoh Program:**
- **"osMessageQueue"** - CMSIS message queue API
- **"osMutex"** - Standard mutex interface
- **"osSemaphore"** - Binary/counting semaphore
- **"osEventFlags"** - Event group wrapper

#### 2. **Static Allocation**
**Contoh Program:**
- **"StaticQueue_t"** - No heap allocation
- **"Static Mutex"** - Deterministic memory
- **"Safety-Critical IPC"** - Pre-allocated resources

---

### ⚡ ESP32 FreeRTOS IPC: Keunggulan Unik

#### 1. **Dual-Core Queue Passing**
**Contoh Program:**
- **"Cross-Core Queue"** - Core 0 → Core 1 messaging
- **"WiFi to App Queue"** - WiFi task → application
- **"Load Distribution"** - Balance work across cores

#### 2. **Task Notification (Fast Signaling)**
**Contoh Program:**
- **"xTaskNotify()"** - Faster than semaphore
- **"ISR to Task Notify"** - Ultra-low latency
- **"Direct Task Wakeup"** - No queue overhead

#### 3. **Stream Buffers**
**Contoh Program:**
- **"UART to TCP Stream"** - Continuous byte stream
- **"Audio Streaming"** - Mic → WiFi pipeline
- **"Zero-Copy Pipeline"** - Efficient data flow

---

### 📊 IPC COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Queue** | ✅ Standard FreeRTOS | ✅ + System queues |
| **Mutex** | ✅ + Priority Inherit | ✅ Standard |
| **Semaphore** | ✅ Binary/Counting | ✅ Binary/Counting |
| **Event Groups** | ✅ 24 bits | ✅ 24 bits |
| **Task Notification** | ✅ Supported | ✅ Widely used |
| **Stream Buffer** | ✅ v10+ | ✅ v10+ |
| **Static Allocation** | ✅ Full support | ✅ Supported |
| **Cross-Core IPC** | ❌ Single core | ✅ Core-to-core |

---

### 💼 MINI PROJECT: Producer-Consumer System

**STM32 Implementation:**
- Producer Task: "ADC Sampler" - Sample sensors → Queue
- Consumer Task 1: "Display Handler" - Queue → OLED
- Consumer Task 2: "Logger" - Queue → SD card
- Mutex: Protect I2C bus (OLED + sensors share)

**ESP32 Implementation:**
- Producer (Core 1): "Sensor Reader" - I2C → Queue
- Producer (Core 1): "Camera" - Image → Queue
- Consumer (Core 0): "WiFi Uploader" - Queue → MQTT/HTTP
- Event Group: Signal when WiFi connected + Queue ready

---

## BAB 13: POWER MANAGEMENT & LOW-POWER DESIGN

> **Tujuan Pembelajaran:**
> - Low-power modes (sleep, stop, standby)
> - Wake-up sources (RTC, EXTI, timers)
> - Clock gating untuk reduce consumption
> - Peripheral power down
> - Battery-powered optimization

### 📖 PENJELASAN MATERI

**Power Consumption Hierarchy:**
```
Active Mode:     50-200 mA (full speed)
Sleep Mode:      10-50 mA (CPU stop, peripherals on)
Deep Sleep:      1-10 mA (most peripherals off)
Standby/Hibernation: <1 mA (only RTC + wakeup pins)
```

**Low-Power Strategy:**
1. **Reduce Clock Speed** - Lower MHz = lower current
2. **Disable Unused Peripherals** - Clock gating
3. **Enter Sleep Between Tasks** - Idle hook sleep
4. **Deep Sleep on Long Idle** - Wake periodically
5. **Optimize Peripheral Usage** - DMA reduces active time

---

### 🔗 COMMON GROUND: Power Management Concepts

#### 1. **Sleep Mode**
**Deskripsi:** CPU stop, peripherals running

**Contoh Program STM32:**
- **"HAL_PWR_EnterSLEEPMode()"** - WFI (Wait For Interrupt)
- **"Any Interrupt Wakes"** - Timer, UART, GPIO
- **"Idle Hook Sleep"** - Sleep in FreeRTOS idle task

**Contoh Program ESP32:**
- **"Light Sleep"** - esp_light_sleep_start()
- **"Auto Light Sleep"** - esp_pm_configure()
- **"GPIO Wakeup"** - esp_sleep_enable_gpio_wakeup()

#### 2. **Deep Sleep/Standby**
**Deskripsi:** Most hardware off, RTC running

**Contoh Program STM32:**
- **"HAL_PWR_EnterSTANDBYMode()"** - <10 µA consumption
- **"RTC Wakeup"** - Wake every 10 seconds
- **"WKUP Pin"** - External button wakeup

**Contoh Program ESP32:**
- **"Deep Sleep"** - esp_deep_sleep_start()
- **"Timer Wakeup"** - esp_sleep_enable_timer_wakeup(10*1e6)
- **"EXT0/EXT1 Wakeup"** - GPIO wakeup dari deep sleep
- **"Touch Wakeup"** - esp_sleep_enable_touchpad_wakeup()

#### 3. **Clock Gating**
**Deskripsi:** Disable peripheral clocks

**Contoh Program STM32:**
- **"__HAL_RCC_UART1_CLK_DISABLE()"** - Turn off UART clock
- **"Conditional Clock Enable"** - Enable only when needed
- **"Peripheral Sleep"** - Low-power UART/I2C mode

**Contoh Program ESP32:**
- **"periph_module_disable()"** - Disable peripheral
- **"Power Domain Control"** - pd_option_t settings
- **"WiFi Modem Sleep"** - WiFi power save mode

#### 4. **Wake-Up Sources**
**Deskripsi:** What can wake MCU from sleep

**Contoh Program STM32:**
- **"RTC Alarm"** - Periodic wakeup
- **"EXTI Line"** - GPIO interrupt wakeup
- **"USART Activity"** - Data reception wakeup

**Contoh Program ESP32:**
- **"Timer"** - esp_sleep_enable_timer_wakeup()
- **"GPIO"** - EXT0 (single pin), EXT1 (multiple pins)
- **"Touch Pad"** - esp_sleep_enable_touchpad_wakeup()
- **"ULP Co-processor"** - Wake on custom condition

#### 5. **Battery Monitoring**
**Deskripsi:** Track remaining capacity

**Contoh Program STM32:**
- **"ADC Battery Voltage"** - Voltage divider → ADC
- **"Fuel Gauge IC"** - I2C gas gauge (MAX17048)
- **"Coulomb Counting"** - Track current over time

**Contoh Program ESP32:**
- **"ADC1 Battery"** - Read LiPo voltage
- **"Hall Sensor"** - Magnetic switch for power save
- **"WiFi Battery Report"** - Send level to server

---

### ⭐ STM32 Power: Keunggulan Unik

#### 1. **Multiple Low-Power Modes**
**Contoh Program:**
- **"Sleep Mode"** - 10-50 mA, instant wake
- **"Stop Mode"** - 1-10 mA, µs wake
- **"Standby Mode"** - <10 µA, ms wake
- **"Low-Power Run"** - Active at low speed (few mA)

#### 2. **VBAT & Backup Domain**
**Contoh Program:**
- **"RTC Keep Running"** - RTC + backup registers on VBAT
- **"Coin Cell Backup"** - CR2032 for timekeeping
- **"Power Loss Recovery"** - Resume from backup registers

#### 3. **Dynamic Voltage Scaling (L4)**
**Contoh Program:**
- **"Low-Power Run Mode"** - Reduce core voltage
- **"Range 1/2"** - 1.2V (fast) or 1.0V (efficient)
- **"Optimize Power/Performance"** - Runtime adjustment

#### 4. **Independent Watchdog in Standby**
**Contoh Program:**
- **"IWDG Keep Running"** - Watchdog active in standby
- **"Fail-Safe Wakeup"** - Auto-wake if hang
- **"System Reliability"** - Prevent infinite sleep

---

### ⚡ ESP32 Power: Keunggulan Unik

#### 1. **ULP Co-Processor**
**Deskripsi:** Ultra-low-power processor untuk monitoring

**Contoh Program:**
- **"ULP ADC Monitor"** - ULP read sensor, wake if threshold
- **"ULP GPIO Monitor"** - Wake on complex conditions
- **"Custom Wake Logic"** - Program ULP untuk intelligent wake
- **"8 µA + ULP Power"** - Main CPU off, ULP monitors

#### 2. **Deep Sleep with RTC Memory**
**Contoh Program:**
- **"Persist Variables"** - RTC_DATA_ATTR int count;
- **"Fast Boot"** - Skip init, resume from RTC data
- **"Deep Sleep Counter"** - Track wake cycles

#### 3. **WiFi Power Save Modes**
**Contoh Program:**
- **"Modem Sleep"** - WiFi on, but sleep between beacons
- **"Light Sleep + WiFi"** - CPU sleep, WiFi maintain connection
- **"DTIM Period"** - Adjust wake frequency (power vs latency)

#### 4. **Hibernation Mode**
**Contoh Program:**
- **"5 µA Ultra-Deep Sleep"** - esp_deep_sleep()
- **"RTC Slow Mem Only"** - Keep minimal data
- **"Cold Boot Resume"** - Almost like power off

---

### 📊 POWER MANAGEMENT COMPARISON

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Active Current** | 50-200 mA | 160-260 mA (WiFi TX) |
| **Sleep Mode** | 10-50 mA | 0.8 mA (light sleep) |
| **Deep Sleep** | <10 µA (Standby) | 10-150 µA |
| **ULP Processor** | ❌ No | ✅ 8 µA + ULP |
| **RTC Persistence** | ✅ Backup domain | ✅ RTC SLOW/FAST |
| **VBAT Support** | ✅ Coin cell | ❌ No |
| **Wake Sources** | RTC/EXTI/USART | Timer/GPIO/Touch/ULP |
| **WiFi Power Save** | ❌ N/A | ✅ Modem/Light Sleep |

---

### 💼 MINI PROJECT: Solar-Powered Weather Station

**STM32 Implementation:**
- **"15-Minute Wake Cycle"** - RTC alarm every 15 min
- **"Sensor Read in 2s"** - Quickly sample sensors
- **"SD Card Log"** - Append data, close file
- **"Standby Mode"** - <10 µA between samples
- **"Battery Voltage Check"** - ADC monitor solar charge

**ESP32 Implementation:**
- **"Deep Sleep 10 Minutes"** - Timer wakeup
- **"Fast Connect WiFi"** - Send MQTT in <5s
- **"ULP Monitor Battery"** - Wake if battery low
- **"Solar MPPT Controller"** - PWM charge control
- **"Emergency Mode"** - Skip WiFi if battery <20%

---

## BAB 14: WIRELESS CONNECTIVITY & IOT INTEGRATION

> **Tujuan Pembelajaran:**
> - WiFi basics (STA, AP, STA+AP mode)
> - MQTT protocol untuk IoT
> - HTTP client/server (REST API)
> - WebSocket untuk real-time
> - BLE (Bluetooth Low Energy)
> - Cloud platforms (ThingSpeak, Blynk, AWS IoT)

### 📖 PENJELASAN MATERI

**Wireless Options:**
```
STM32:
  ├─ WiFi: External module (ESP8266, ESP32-WROOM)
  ├─ Bluetooth: External (HC-05, HM-10)
  ├─ LoRa: SX1276/SX1278 module
  ├─ Cellular: SIM800/SIM7600 module
  └─ NRF24L01: 2.4 GHz RF (short range)

ESP32:
  ├─ WiFi: Built-in 802.11 b/g/n
  ├─ Bluetooth Classic: Built-in
  ├─ BLE 4.2: Built-in
  ├─ ESP-NOW: Proprietary 2.4 GHz
  └─ External: LoRa, Cellular via UART
```

**IoT Protocols:**
- **MQTT**: Publish/Subscribe (lightweight, ideal untuk IoT)
- **HTTP/HTTPS**: Request/Response (REST API)
- **WebSocket**: Full-duplex real-time
- **CoAP**: Constrained devices (UDP-based)

---

### 🔗 COMMON GROUND: Wireless Concepts

#### 1. **UART-Based Modules (STM32 Approach)**
**Deskripsi:** External wireless via AT commands

**Contoh Program STM32:**
- **"ESP8266 WiFi Module"** - AT commands via UART
- **"HC-05 Bluetooth"** - Serial to Bluetooth bridge
- **"SIM800 GSM"** - Send SMS, make calls
- **"NRF24L01"** - 2.4 GHz wireless (SPI)

**Contoh Program ESP32:**
- **"UART to WiFi Bridge"** - ESP32 act as WiFi modem for STM32
- **"Transparent Bridge"** - Forward UART to TCP/UDP
- **"AT Command Server"** - Compatibility dengan ESP8266 AT firmware

#### 2. **MQTT Pub/Sub**
**Deskripsi:** IoT messaging protocol

**Contoh Program STM32 + ESP8266:**
- **"MQTT via ESP8266"** - Connect to broker via AT commands
- **"Publish Sensor Data"** - Send temperature to cloud
- **"Subscribe to Commands"** - Receive control messages

**Contoh Program ESP32:**
- **"Native MQTT Client"** - esp-mqtt library
- **"TLS/SSL Support"** - Secure MQTT (port 8883)
- **"QoS Levels"** - QoS 0/1/2 delivery guarantee
- **"Last Will Testament"** - Auto-notify disconnect

#### 3. **HTTP REST API**
**Deskripsi:** Web-based communication

**Contoh Program STM32 + ESP8266:**
- **"HTTP GET"** - Fetch data dari API
- **"HTTP POST"** - Send JSON data
- **"Parse JSON Response"** - Extract values

**Contoh Program ESP32:**
- **"esp_http_client"** - GET/POST/PUT/DELETE
- **"HTTPS Support"** - SSL/TLS encryption
- **"HTTP Server"** - Embedded web server
- **"REST API Endpoint"** - /api/sensors endpoint

#### 4. **WebSocket**
**Deskripsi:** Real-time bidirectional

**Contoh Program STM32:**
- **"WebSocket via Module"** - External module handling
- **"Real-Time Chart"** - Stream data to web dashboard

**Contoh Program ESP32:**
- **"WebSocket Server"** - esp_websocket library
- **"Real-Time Updates"** - Push sensor data to browser
- **"Two-Way Control"** - Browser ↔ ESP32 control

#### 5. **Cloud Integration**
**Deskripsi:** IoT platforms

**Contoh Program STM32:**
- **"ThingSpeak Upload"** - HTTP POST every 15 seconds
- **"Blynk App"** - Mobile app control
- **"Custom Server"** - Node.js backend

**Contoh Program ESP32:**
- **"AWS IoT Core"** - MQTT with X.509 certificates
- **"Google Firebase"** - Real-time database
- **"ThingSpeak + Grafana"** - Data visualization
- **"IFTTT Integration"** - Trigger actions (email, SMS)

---

### ⭐ STM32 Wireless: Keunggulan Unik

#### 1. **Deterministic Hard Real-Time**
**Contoh Program:**
- **"CAN Bus + WiFi"** - CAN critical, WiFi secondary
- **"Industrial Ethernet"** - STM32F7 Ethernet MAC
- **"Profinet/EtherCAT"** - Industrial protocols
- **"Safety-Certified"** - IEC 61508 SIL3 certified variants

#### 2. **Multiple Communication Stacks**
**Contoh Program:**
- **"LoRaWAN Stack"** - Long-range IoT (STM32WL)
- **"Sigfox Module"** - Ultra-low-power IoT
- **"NB-IoT Modem"** - Cellular IoT
- **"Multi-Protocol Gateway"** - Bridge different networks

#### 3. **Secure Boot & Crypto**
**Contoh Program:**
- **"STM32 Secure Boot"** - Verified firmware
- **"Hardware Crypto"** - AES/SHA accelerator
- **"TrustZone (STM32L5)"** - Secure/non-secure isolation

---

### ⚡ ESP32 Wireless: Keunggulan Unik

#### 1. **Built-in WiFi 802.11n**
**Contoh Program:**
- **"Station Mode"** - Connect ke router
- **"Access Point Mode"** - ESP32 sebagai router
- **"STA+AP Mode"** - Simultaneous client + AP
- **"WiFi Provisioning"** - SmartConfig, WPS, BLE provisioning
- **"WiFi Mesh"** - ESP-MESH networking

#### 2. **Bluetooth Classic + BLE**
**Contoh Program:**
- **"BLE GATT Server"** - Custom BLE service
- **"BLE Beacon"** - iBeacon/Eddystone
- **"Bluetooth Serial (SPP)"** - Classic Bluetooth UART
- **"BLE + WiFi Coexistence"** - Share 2.4 GHz radio

#### 3. **ESP-NOW (Proprietary)**
**Contoh Program:**
- **"ESP-NOW Broadcast"** - Peer-to-peer without router
- **"<2 ms Latency"** - Ultra-low latency
- **"250 Bytes Payload"** - Fast small messages
- **"Mesh-like Network"** - Multi-hop forwarding

#### 4. **WiFi Direct & P2P**
**Contoh Program:**
- **"WiFi Direct Mode"** - Device-to-device
- **"Local Control"** - No internet required
- **"AP + Webserver"** - Configuration portal

#### 5. **lwIP TCP/IP Stack**
**Contoh Program:**
- **"TCP Server/Client"** - Raw sockets
- **"UDP Multicast"** - Broadcast to group
- **"DNS Client"** - Resolve hostnames
- **"DHCP Server"** - Assign IPs in AP mode

#### 6. **OTA (Over-The-Air) Update**
**Contoh Program:**
- **"HTTP OTA"** - Download firmware dari server
- **"Rollback Support"** - Revert if new firmware fails
- **"Secure OTA"** - HTTPS with certificate validation

---

### 📊 WIRELESS COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **WiFi** | ⚠️ External module | ✅ Built-in 802.11n |
| **Bluetooth Classic** | ⚠️ External | ✅ Built-in |
| **BLE** | ⚠️ External | ✅ Built-in BLE 4.2 |
| **Proprietary RF** | ✅ Sub-GHz (STM32WL) | ✅ ESP-NOW |
| **Ethernet** | ✅ STM32F4/F7 MAC | ❌ No (external PHY) |
| **CAN Bus** | ✅ Built-in | ⚠️ External (TWAI) |
| **LoRa** | ✅ STM32WL | ⚠️ External module |
| **Cellular** | ⚠️ External modem | ⚠️ External modem |
| **OTA Update** | ⚠️ Bootloader needed | ✅ Native support |
| **Cloud Integration** | ⚠️ Via libraries | ✅ ESP-IDF built-in |

---

### 💼 MINI PROJECT: Smart Home Gateway

**STM32 Implementation:**
- **"CAN Bus Nodes"** - Read home automation CAN network
- **"ESP8266 WiFi Bridge"** - Forward data to cloud
- **"Local LCD Display"** - Show status on TFT
- **"Modbus RTU"** - Industrial sensor protocol
- **"SD Card Logging"** - Backup data locally

**ESP32 Implementation:**
- **"WiFi MQTT Gateway"** - Connect to broker (Mosquitto)
- **"BLE Device Scanner"** - Scan BLE sensors (temp, humidity)
- **"ESP-NOW Mesh"** - Collect data dari multiple ESP32 nodes
- **"Web Dashboard"** - Real-time control via browser
- **"Alexa/Google Home"** - Voice control integration
- **"OTA Updates"** - Remote firmware update

**Features:**
- Multi-protocol: WiFi, BLE, RF (NRF24)
- Local control (WebServer)
- Cloud control (MQTT to AWS IoT)
- Voice control (Alexa skill)
- Mobile app (Blynk)
- Data logging (SD card + cloud)
- Secure communication (TLS/SSL)

---

## 🎯 KESIMPULAN AKHIR

### Kapan Pilih STM32?
```
✅ Hard real-time requirements
✅ Deterministic timing critical
✅ Safety-certified applications
✅ CAN bus / industrial protocols
✅ 5V sensor interfacing
✅ Ultra-low power (<1 µA standby)
✅ Precise analog (16-bit ADC)
✅ High-speed GPIO (10 MHz+)
```

### Kapan Pilih ESP32?
```
✅ IoT / WiFi connectivity
✅ Rapid prototyping
✅ Web-based interfaces
✅ Bluetooth requirements
✅ Dual-core processing
✅ Large community support
✅ Cost-sensitive projects
✅ Flexible GPIO mapping
```

### Kombinasi STM32 + ESP32
```
Ideal Setup:
  STM32 → Real-time control, sensors, actuators
    ↕ UART
  ESP32 → WiFi gateway, cloud communication
  
Use Case: Industrial IoT
  - STM32 handle CAN bus, Modbus, critical timing
  - ESP32 handle WiFi, MQTT, web interface
  - Best of both worlds!
```

---

## 📚 RESOURCES & NEXT STEPS

### STM32 Learning Path
1. HAL basics → GPIO, UART, ADC
2. Interrupts & Timers → PWM, Input Capture
3. Communication → I2C, SPI, CAN
4. DMA & Advanced peripherals
5. FreeRTOS multitasking
6. USB, Ethernet (F4/F7)
7. Low-power optimization

### ESP32 Learning Path
1. Arduino basics → GPIO, Serial
2. WiFi basics → STA mode, HTTP client
3. ESP-IDF → Native development
4. FreeRTOS → Multitasking
5. BLE & ESP-NOW
6. MQTT & Cloud integration
7. OTA & Production deployment

### Recommended Projects
**Beginner:**
- Temperature logger (ADC + SD card)
- OLED menu system (I2C display)
- PWM fan controller

**Intermediate:**
- Multi-sensor IoT node (WiFi + MQTT)
- BLE heart rate monitor
- Real-time data acquisition (DMA + USB)

**Advanced:**
- Multi-protocol gateway (CAN + WiFi)
- Drone flight controller (STM32)
- Smart home hub (ESP32 mesh)

---

**Selamat Belajar! 🚀**

*Dokumen ini mencakup 14 bab lengkap untuk pembelajaran paralel STM32 dan ESP32. Setiap bab menekankan common ground (kesamaan) dan unique advantages (keunggulan masing-masing platform) untuk pemahaman mendalam.*
