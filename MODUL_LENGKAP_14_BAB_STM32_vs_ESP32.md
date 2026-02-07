# RANGKUMAN LENGKAP: STM32 vs ESP32
## Panduan 14 Bab untuk Pembelajaran Sistem Embedded

**Dokumen Referensi:**
- STM32: Mastering STM32 - 2nd Edition (Carmine Noviello)
- ESP32: Kolban's Book on ESP32 & ESP32 Datasheet

**Tanggal:** 6 Februari 2026

---

## 📚 STRUKTUR 14 BAB PEMBELAJARAN

Setiap bab disusun dengan format:
- 🔗 **Common Ground**: Fitur/konsep yang ada di kedua platform
- ⭐ **STM32 Advantages**: Keunggulan unik STM32
- ⚡ **ESP32 Advantages**: Keunggulan unik ESP32
- 💡 **Practical Tips**: Tips praktis untuk pembelajaran

---

## 📋 DAFTAR BAB

**FASE 1: FONDASI (Bab 1-3)**
1. [Pengenalan Arsitektur & Setup Environment](#bab-1-pengenalan-arsitektur--setup-environment)
2. [Digital I/O & GPIO Programming](#bab-2-digital-io--gpio-programming)
3. [External Interrupts & Event Handling](#bab-3-external-interrupts--event-handling)

**FASE 2: KOMUNIKASI (Bab 4-6)**
4. [UART/Serial Communication](#bab-4-uartserial-communication)
5. [ADC: Analog Input & Sensor Reading](#bab-5-adc-analog-input--sensor-reading)
6. [Timer, PWM & Output Control](#bab-6-timer-pwm--output-control)

**FASE 3: PROTOKOL LANJUT (Bab 7-9)**
7. [I²C Protocol & Device Communication](#bab-7-ic-protocol--device-communication)
8. [SPI Protocol & High-Speed Transfer](#bab-8-spi-protocol--high-speed-transfer)
9. [DMA & Memory Management](#bab-9-dma--memory-management)

**FASE 4: SISTEM ADVANCED (Bab 10-12)**
10. [Clock System & Timing Configuration](#bab-10-clock-system--timing-configuration)
11. [FreeRTOS: Multitasking Basics](#bab-11-freertos-multitasking-basics)
12. [FreeRTOS: IPC & Synchronization](#bab-12-freertos-ipc--synchronization)

**FASE 5: APLIKASI PRAKTIS (Bab 13-14)**
13. [Power Management & Low-Power Design](#bab-13-power-management--low-power-design)
14. [Wireless Connectivity & IoT Integration](#bab-14-wireless-connectivity--iot-integration)

---

---

# FASE 1: FONDASI

---

## BAB 1: PENGENALAN ARSITEKTUR & SETUP ENVIRONMENT

> **Tujuan Pembelajaran:**
> - Memahami perbedaan arsitektur ARM Cortex-M vs Xtensa
> - Setup development environment (IDE, toolchain, programmer)
> - Membuat program pertama: Blink LED
> - Memahami memory layout dan address space

---

### 🔗 COMMON GROUND: Kesamaan STM32 & ESP32

**Keduanya Adalah 32-bit Microcontrollers:**
- Architecture: 32-bit RISC
- Operating Voltage: 3.3V (main logic)
- Development: C/C++ programming
- Ecosystem: Large community support
- Real-time capable: Deterministic execution
- Toolchain: GCC-based compilers

**Workflow Similarity:**
```
1. Write Code (C/C++)
2. Compile → Binary
3. Flash to MCU
4. Debug & Test
```

**Common Concepts:**
- Register-based programming
- Memory-mapped peripherals
- Interrupt-driven architecture
- Stack & heap management
- RTOS support (FreeRTOS)

---

### ⭐ STM32 ADVANTAGES: Keunggulan Unik

#### 1. **Standard ARM Architecture**
```
✅ Industry standard (ARM Cortex-M)
✅ Portable code across vendors (NXP, TI, Nordic, etc.)
✅ CMSIS standard library
✅ Extensive ARM documentation
✅ Wide ecosystem of compatible tools
```

**Impact:** Code portability! Program STM32 mudah di-port ke MCU ARM lain.

#### 2. **HAL (Hardware Abstraction Layer)**
```c
// STM32 HAL - Abstraksi tinggi, portability tinggi
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
HAL_Delay(1000);
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
```

**Keunggulan:**
- Satu HAL untuk semua series (F0, F1, F4, F7, L4, H7, dll)
- Migration mudah antar series
- Professional-grade abstraction

#### 3. **STM32CubeMX: Graphical Configuration**
```
✅ Visual pin configuration
✅ Clock tree wizard
✅ Automatic code generation
✅ Conflict detection
✅ Power consumption estimation
```

**Impact:** Beginner-friendly! No need to read 1000+ page reference manual.

#### 4. **Deterministic Real-Time Performance**
```
⚡ Fixed execution time
⚡ No cache effects (except M7)
⚡ No WiFi interference
⚡ Predictable interrupt latency: 12 cycles (M4)
```

**Use Case:** Hard real-time systems (motor control, safety-critical)

#### 5. **Professional Debugging**
```
✅ ST-LINK debugger (integrated on Nucleo)
✅ SWD/JTAG interface
✅ Breakpoints, watchpoints
✅ Live variable watching
✅ SWV (Serial Wire Viewer) trace
✅ ETM trace (on F7/H7)
```

#### 6. **Extensive Portfolio (17 Series)**
```
Entry-level    → STM32F0/G0  ($0.30+)
Mainstream     → STM32F1/F4  ($1-3)
High-perf      → STM32F7/H7  ($5-10)
Ultra-low-pwr  → STM32L0/L4  ($1-4)
Secure         → STM32L5/U5  (TrustZone)
Wireless       → STM32WB/WL  (BLE/LoRa)
Linux-capable  → STM32MP1    (Cortex-A7+M4)
```

**Impact:** Optimal chip selection untuk setiap aplikasi!

---

### ⚡ ESP32 ADVANTAGES: Keunggulan Unik

#### 1. **Integrated Wireless (WiFi + Bluetooth)**
```
✅ WiFi 802.11 b/g/n (2.4 GHz)
✅ Bluetooth Classic + BLE
✅ Built-in antenna (or external)
✅ No external module needed!
```

**Impact:** IoT-ready out of the box! STM32 butuh modul eksternal.

**Example - Connect to WiFi:**
```cpp
#include <WiFi.h>

void setup() {
    WiFi.begin("SSID", "password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("Connected! IP: " + WiFi.localIP().toString());
}
```

#### 2. **Dual-Core Processing**
```
✅ 2x Xtensa LX6 cores @ 240 MHz
✅ Symmetric Multi-Processing (SMP)
✅ Pin tasks to specific core
✅ Parallel execution
```

**Example:**
```c
// Task 1 runs on Core 0
xTaskCreatePinnedToCore(task1, "Task1", 2048, NULL, 1, NULL, 0);

// Task 2 runs on Core 1
xTaskCreatePinnedToCore(task2, "Task2", 2048, NULL, 1, NULL, 1);
```

#### 3. **Arduino Compatibility**
```
✅ Arduino IDE support (plug & play)
✅ Huge library ecosystem
✅ Beginner-friendly
✅ Rapid prototyping
```

**Example - Blink LED:**
```cpp
void setup() {
    pinMode(2, OUTPUT);
}

void loop() {
    digitalWrite(2, HIGH);
    delay(1000);
    digitalWrite(2, LOW);
    delay(1000);
}
```

**Impact:** 5 menit dari unbox sampai blink LED!

#### 4. **Built-in Peripherals**
```
✅ 10 Capacitive Touch Sensors
✅ Hall Effect Sensor
✅ Hardware Cryptography (AES, SHA, RSA)
✅ Digital/Analog Temp Sensor
```

**Touch Sensor Example:**
```c
#include "driver/touch_pad.h"

touch_pad_init();
touch_pad_config(TOUCH_PAD_NUM9, 0); // GPIO 32

uint16_t touch_value;
touch_pad_read(TOUCH_PAD_NUM9, &touch_value);
if (touch_value < threshold) {
    // Touched!
}
```

#### 5. **Large Flash Memory (External SPI)**
```
✅ 4 MB, 8 MB, 16 MB common
✅ Cheap external flash
✅ OTA (Over-The-Air) updates
✅ File system support (SPIFFS/LittleFS)
```

**STM32:** Max 2 MB on-chip (expensive for large flash)

#### 6. **Cost-Effective for IoT**
```
ESP32 DevKit: $3-5 (with WiFi+BT!)
STM32F4 + WiFi module: $8-12
```

#### 7. **Powerful Development Framework**
```
✅ ESP-IDF (native framework)
✅ FreeRTOS integrated
✅ WiFi/BLE stacks included
✅ AWS IoT, Azure IoT SDK
✅ Massive examples library
```

---

### 💡 PRACTICAL COMPARISON

| Aspect | STM32 | ESP32 |
|--------|-------|-------|
| **Learning Curve** | Steeper (HAL, registers) | Gentler (Arduino) |
| **First Program** | 30 min (CubeMX setup) | 5 min (Arduino IDE) |
| **Debugging** | ⭐⭐⭐⭐⭐ Professional | ⭐⭐⭐ Serial.print() mainly |
| **Code Portability** | ⭐⭐⭐⭐⭐ ARM standard | ⭐⭐ ESP-specific |
| **Real-Time** | ⭐⭐⭐⭐⭐ Deterministic | ⭐⭐⭐⭐ Good (WiFi impacts) |
| **IoT Ready** | ⭐⭐ Need modules | ⭐⭐⭐⭐⭐ Built-in WiFi/BT |
| **Community** | ⭐⭐⭐⭐ Professional | ⭐⭐⭐⭐⭐ Hobbyist+Pro |
| **Price** | $0.30 - $10+ | $2 - $5 |

---

### 🛠️ SETUP ENVIRONMENT

#### STM32 Development Setup

**1. IDE Options:**
```
Recommended: STM32CubeIDE (official, free)
  ├─ Eclipse-based
  ├─ Integrated CubeMX
  ├─ Built-in debugger
  └─ GCC toolchain included

Alternative:
  ├─ Keil MDK (commercial, ARM official)
  ├─ IAR Embedded Workbench (commercial)
  └─ VSCode + PlatformIO (open-source)
```

**2. Programmer/Debugger:**
```
✅ ST-LINK V2/V3 (official, $20-50)
✅ Nucleo boards (built-in ST-LINK)
✅ J-Link (professional, $60-400)
```

**3. Install STM32CubeIDE:**
```
1. Download from: st.com/stm32cubeide
2. Install (Windows/Linux/Mac)
3. Create new project → Select MCU
4. Configure pins in CubeMX
5. Generate code
6. Build & Flash
```

---

#### ESP32 Development Setup

**Option 1: Arduino IDE (Easiest)**
```
1. Install Arduino IDE 2.x
2. Add ESP32 board manager URL:
   File → Preferences → Additional Boards Manager URLs
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

3. Tools → Board → Boards Manager → Search "ESP32" → Install

4. Select Board: ESP32 Dev Module
5. Select Port: COMx / /dev/ttyUSBx
6. Upload!
```

**Option 2: ESP-IDF (Professional)**
```bash
# Linux/Mac
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
. ./export.sh

# Create project
idf.py create-project hello_world
cd hello_world
idf.py menuconfig  # Configure
idf.py build       # Compile
idf.py flash       # Upload
idf.py monitor     # Serial monitor
```

**Option 3: PlatformIO (Best of both)**
```
✅ VSCode extension
✅ ESP-IDF + Arduino framework
✅ Library manager
✅ Multi-platform
```

**Programmer:**
```
✅ USB-UART (built-in on DevKit)
✅ CP2102/CH340 (automatic driver)
✅ No external programmer needed!
```

---

### 📝 FIRST PROGRAM: BLINK LED

#### STM32 (HAL)

**CubeMX Configuration:**
```
1. Select STM32F103C8 (BluePill)
2. Configure PA5 as GPIO_Output
3. Generate code
```

**main.c:**
```c
#include "main.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    while (1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(1000);
    }
}
```

**Register-level (Advanced):**
```c
// Faster, no HAL overhead
GPIOA->BSRR = GPIO_PIN_5;        // Set
HAL_Delay(1000);
GPIOA->BSRR = (GPIO_PIN_5 << 16); // Reset
```

---

#### ESP32 (Arduino)

```cpp
#define LED_PIN 2  // Built-in LED

void setup() {
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(1000);
}
```

**ESP-IDF (Native):**
```c
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_PIN GPIO_NUM_2

void app_main(void) {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    
    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

---

### 🎯 KAPAN PAKAI STM32 vs ESP32?

#### Gunakan STM32 Jika:
```
✅ Real-time critical (motor control, precise timing)
✅ Safety-critical systems (automotive, medical)
✅ Ultra-low power (<1 µA deep sleep)
✅ Professional debugging required
✅ Code portability (ARM ecosystem)
✅ High-speed analog (16-bit ADC, fast DAC)
✅ No WiFi/BT needed (lebih murah)
✅ Industrial temperature range (-40°C to 125°C)
```

**Example Use Cases:**
- Motor controllers (BLDC, servo)
- Industrial automation
- Medical devices
- Automotive ECU
- Battery-powered sensors (years lifetime)

---

#### Gunakan ESP32 Jika:
```
✅ IoT connectivity needed (WiFi/BLE)
✅ Rapid prototyping
✅ Web server/client
✅ Wireless sensor networks
✅ Home automation
✅ Low budget
✅ Need dual-core processing
✅ OTA updates required
```

**Example Use Cases:**
- Smart home devices
- WiFi weather stations
- BLE beacons
- Web-controlled robots
- IoT gateways
- Wireless data loggers

---

### 📚 LEARNING PATH

**Week 1-2: STM32**
```
Day 1-2: Setup STM32CubeIDE, Blink LED
Day 3-4: HAL basics, GPIO output
Day 5-6: Input reading, button debounce
Day 7: Mini project - Traffic light
```

**Week 1-2: ESP32**
```
Day 1-2: Setup Arduino IDE, Blink LED
Day 3-4: Digital I/O, Serial communication
Day 5-6: WiFi connection basics
Day 7: Mini project - Web-controlled LED
```

---

### 🔧 TROUBLESHOOTING COMMON ISSUES

#### STM32
```
❌ "Cannot connect to target"
  → Check ST-LINK connection
  → Try "Connect Under Reset"
  → Check BOOT0 pin

❌ "Hard Fault"
  → Check stack size
  → Check array bounds
  → Enable fault handler debugging

❌ "Code not running"
  → Check boot pins (BOOT0=0, BOOT1=x)
  → Verify flash address
  → Check clock configuration
```

#### ESP32
```
❌ "Failed to connect to ESP32"
  → Hold BOOT button while uploading
  → Check COM port
  → Try different USB cable

❌ "Brownout detector reset"
  → Power supply issue (use 5V, min 500mA)
  → Add decoupling capacitors

❌ "Guru Meditation Error"
  → Stack overflow → increase stack size
  → Check task watchdog timeout
```

---

### 📖 REFERENCES BAB 1

**STM32:**
- STM32CubeIDE User Manual
- Getting Started with STM32 (ST AN)
- Cortex-M Programming Guide (ARM)

**ESP32:**
- ESP32 Getting Started Guide
- ESP-IDF Programming Guide
- ESP32 Datasheet

---

## BAB 2: DIGITAL I/O & GPIO PROGRAMMING

---

## BAB 2: DIGITAL I/O & GPIO PROGRAMMING

> **Tujuan Pembelajaran:**
> - Memahami GPIO modes dan konfigurasi
> - Input reading dengan pull-up/pull-down
> - Output control dengan berbagai speed
> - Debouncing techniques
> - Atomic operations vs race conditions

---

### 🔗 COMMON GROUND: Kesamaan STM32 & ESP32

**GPIO Fundamentals (Keduanya Sama):**
```
✅ Digital HIGH/LOW (3.3V logic)
✅ Input dan Output modes
✅ Internal pull-up/pull-down resistors
✅ Multiple GPIO ports/pins
✅ Direct register access (fast operation)
✅ Software-controlled pin states
```

**Common Operations:**
```c
// Conceptually sama
pinMode(pin, OUTPUT);        // Set as output
digitalWrite(pin, HIGH);     // Set HIGH
int val = digitalRead(pin);  // Read input
```

**Common Use Cases:**
- LED control
- Button reading
- Relay control
- Digital sensor interfacing
- General purpose I/O

---

### ⭐ STM32 GPIO: Keunggulan Unik

#### 1. **Flexible Speed Control**

STM32 GPIO memiliki **4 speed levels**:
```c
GPIO_SPEED_FREQ_LOW       // 8 MHz   (low EMI)
GPIO_SPEED_FREQ_MEDIUM    // 50 MHz  (balanced)
GPIO_SPEED_FREQ_HIGH      // 100 MHz (fast switching)
GPIO_SPEED_FREQ_VERY_HIGH // 180 MHz (F4/F7/H7)
```

**Keuntungan:**
- ⚡ Optimize EMI (electromagnetic interference)
- ⚡ Reduce power consumption untuk slow signals
- ⚡ High-speed untuk komunikasi protocols

**Example:**
```c
GPIO_InitStruct.Pin = GPIO_PIN_5;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // Fast toggling
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

---

#### 2. **Atomic GPIO Operations (BSRR Register)**

STM32 memiliki **Bit Set/Reset Register (BSRR)**:

```c
// Atomic set/reset (NO read-modify-write)
GPIOA->BSRR = GPIO_PIN_5;        // Set PA5 (1 instruction)
GPIOA->BSRR = (GPIO_PIN_5 << 16); // Reset PA5 (1 instruction)

// Set multiple pins atomically
GPIOA->BSRR = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;

// Simultan set & reset different pins
GPIOA->BSRR = GPIO_PIN_1 | (GPIO_PIN_2 << 16); // PA1=HIGH, PA2=LOW
```

**Keuntungan:**
- ✅ **Thread-safe** (tidak perlu disable interrupt)
- ✅ **Faster** (1 cycle vs 3 cycles read-modify-write)
- ✅ **Race condition proof**

**HAL Wrapper:**
```c
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   // Uses BSRR
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);                 // XOR operation
```

---

#### 3. **Bit-Banding (STM32F1/F3/F4 Only)**

**Atomic bit manipulation via memory aliasing:**

```c
// Macro untuk bit-banding
#define BITBAND_PERI_BASE   0x40000000
#define ALIAS_PERI_BASE     0x42000000
#define BITBAND_PERI(addr, bit) \
    ((ALIAS_PERI_BASE + ((addr) - BITBAND_PERI_BASE)*32 + (bit)*4))

// Example: Control PA5 via bit-banding
#define GPIOA_ODR_ADDR 0x40020014
uint32_t *PA5_BB = (uint32_t*)BITBAND_PERI(GPIOA_ODR_ADDR, 5);

*PA5_BB = 1; // Atomic set (1 instruction!)
*PA5_BB = 0; // Atomic clear
```

**Use Case:** RTOS-safe GPIO tanpa disable interrupt.

---

#### 4. **Open-Drain with Speed Control**

```c
// Open-drain untuk I2C, 1-wire, etc.
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
GPIO_InitStruct.Pull = GPIO_PULLUP; // External or internal
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

**Keuntungan:**
- ✅ Wired-AND logic
- ✅ Level shifting (5V tolerant pins)
- ✅ I2C/SMBus communication

---

#### 5. **5V Tolerant Pins (FT Type)**

Sebagian besar STM32 memiliki **5V tolerant pins**:

```
✅ Can read 5V signals without level shifter!
✅ Marked as "FT" in datasheet
✅ Common on: PA9, PA10, PB6, PB7, etc.
```

**Use Case:**
- Interface dengan 5V Arduino
- Read 5V sensors
- Legacy system integration

⚠️ **Catatan:** Output tetap 3.3V! Hanya input yang 5V tolerant.

---

#### 6. **Alternate Function Flexibility**

STM32 GPIO bisa di-remap ke **16 alternate functions**:

```c
// PA9/PA10 bisa jadi:
GPIO_AF1_TIM1       // Timer 1
GPIO_AF4_I2C1       // I2C 1
GPIO_AF7_USART1     // USART 1
GPIO_AF10_OTG_FS    // USB OTG
// ... dll

// Configure
GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

**Keuntungan:**
- ✅ Flexible pin routing
- ✅ Resolve pin conflicts
- ✅ Optimize PCB layout

---

#### 7. **GPIO Lock Mechanism**

```c
// Lock configuration (prevent accidental change)
HAL_GPIO_LockPin(GPIOA, GPIO_PIN_5);
// Now PA5 config cannot be changed until reset!
```

**Use Case:** Safety-critical systems, protect critical pins.

---

### ⚡ ESP32 GPIO: Keunggulan Unik

#### 1. **GPIO Matrix: Extreme Flexibility**

ESP32 memiliki **GPIO Matrix** - ANY peripheral ke ANY pin!

```c
// UART TX bisa ke pin mana saja (bukan fixed!)
uart_set_pin(UART_NUM_1, 
             17,  // TX → GPIO 17
             16,  // RX → GPIO 16
             UART_PIN_NO_CHANGE,
             UART_PIN_NO_CHANGE);

// Or use different pins:
uart_set_pin(UART_NUM_1, 
             25,  // TX → GPIO 25
             26,  // RX → GPIO 26
             UART_PIN_NO_CHANGE,
             UART_PIN_NO_CHANGE);
```

**Impact:** **ULTIMATE PCB layout freedom!** Tidak ada pin conflict seperti STM32.

---

#### 2. **Capacitive Touch Sensors (10 Pins)**

ESP32 memiliki **10 hardware touch sensors** built-in:

```
TOUCH0 → GPIO4    TOUCH5 → GPIO12
TOUCH1 → GPIO0    TOUCH6 → GPIO14
TOUCH2 → GPIO2    TOUCH7 → GPIO27
TOUCH3 → GPIO15   TOUCH8 → GPIO33
TOUCH4 → GPIO13   TOUCH9 → GPIO32
```

**Example:**
```c
#include "driver/touch_pad.h"

touch_pad_init();
touch_pad_config(TOUCH_PAD_NUM9, 0); // GPIO32

uint16_t touch_value;
touch_pad_read(TOUCH_PAD_NUM9, &touch_value);

if (touch_value < 500) {
    printf("Touched!\n");
}
```

**Arduino:**
```cpp
int touchValue = touchRead(T9); // GPIO32
if (touchValue < 40) { // threshold
    // Touched!
}
```

**Use Case:**
- Touch buttons (no mechanical parts!)
- Touch sliders
- Proximity sensing
- Water level detection

---

#### 3. **RTC GPIO: Deep Sleep Persistence**

**RTC GPIO** tetap berfungsi saat deep sleep:

```c
// Configure RTC GPIO
rtc_gpio_init(GPIO_NUM_25);
rtc_gpio_set_direction(GPIO_NUM_25, RTC_GPIO_MODE_OUTPUT_ONLY);

// Enter deep sleep
esp_deep_sleep_start();

// GPIO25 tetap bisa dikendalikan via ULP coprocessor!
```

**RTC GPIO Pins:** GPIO 0, 2, 4, 12-15, 25-27, 32-39

**Use Case:**
- Keep sensor powered during sleep
- Wake-up signal generation
- Ultra-low power applications

---

#### 4. **Strapping Pins: Auto-Config**

ESP32 gunakan **strapping pins** untuk boot mode:

```
GPIO 0  → Boot mode (LOW = download mode)
GPIO 2  → Boot mode (must be floating/HIGH)
GPIO 5  → Timing (for SDIO)
GPIO 12 → Voltage selection (flash)
GPIO 15 → Boot messages (LOW = silent)
```

**Impact:** Pin ini harus carefully used!

⚠️ **Best Practice:**
```
✅ Avoid using strapping pins for critical I/O
✅ Use them AFTER boot
✅ Add pull-up/pull-down as needed
```

---

#### 5. **ADC/DAC Capable Pins (Selected)**

GPIO tertentu memiliki fungsi analog:

**ADC1 (usable with WiFi):**
```
GPIO32-39 → ADC1_CH0 to ADC1_CH7
```

**ADC2 (NOT usable with WiFi):**
```
GPIO0, 2, 4, 12-15, 25-27 → ADC2_CHx
```

**DAC (8-bit):**
```
GPIO25 → DAC1
GPIO26 → DAC2
```

---

#### 6. **Input-Only Pins (GPIO 34-39)**

```
⚠️ GPIO 34-39: INPUT ONLY!
   ❌ No output capability
   ❌ No internal pull-up/pull-down
   ✅ ADC capable
```

**Use Case:**
- Pure analog inputs
- Sensor reading only
- Free up output-capable pins

---

### 💡 PRACTICAL EXAMPLES

#### Example 1: Button Input dengan Debouncing

**STM32 (Hardware + Software):**
```c
// Hardware: Pull-up enabled
GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
GPIO_InitStruct.Pull = GPIO_PULLUP; // Internal pull-up
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

// Software debounce
uint8_t button_state = 0;
uint8_t last_state = 1;
uint32_t last_change = 0;
#define DEBOUNCE_TIME 50 // ms

void check_button() {
    uint8_t reading = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    
    if (reading != last_state) {
        last_change = HAL_GetTick();
    }
    
    if ((HAL_GetTick() - last_change) > DEBOUNCE_TIME) {
        if (reading != button_state) {
            button_state = reading;
            if (button_state == 0) { // Pressed
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            }
        }
    }
    last_state = reading;
}
```

---

**ESP32 (Software Debounce):**
```cpp
#define BUTTON_PIN 4
#define LED_PIN 2

int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    int reading = digitalRead(BUTTON_PIN);
    
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != buttonState) {
            buttonState = reading;
            if (buttonState == LOW) { // Pressed (active-low)
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            }
        }
    }
    lastButtonState = reading;
}
```

---

#### Example 2: High-Speed GPIO Toggling

**STM32 (Register Level):**
```c
// Initialize
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

// Toggle at maximum speed (NO HAL overhead)
while(1) {
    GPIOA->BSRR = GPIO_PIN_5;        // Set (5 ns @ 200 MHz)
    GPIOA->BSRR = (GPIO_PIN_5 << 16); // Reset (5 ns)
}
// Result: ~5 MHz square wave!
```

**ESP32 (Direct Register):**
```c
#define LED_GPIO 2

gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

// Toggle via register
while(1) {
    GPIO.out_w1ts = (1 << LED_GPIO); // Set
    GPIO.out_w1tc = (1 << LED_GPIO); // Clear
}
// Result: ~1-2 MHz (slower due to cache)
```

**Performance:**
```
STM32F4@180MHz: ~10 MHz toggle possible
ESP32@240MHz:   ~2-3 MHz toggle (cache/bus latency)
```

---

#### Example 3: Multiple LED Control

**STM32 (Bitmask):**
```c
// Control PA0-PA7 simultaneously
uint8_t pattern = 0b10101010;

// Atomic write
GPIOA->ODR = (GPIOA->ODR & 0xFF00) | pattern;

// Or using HAL
for (int i = 0; i < 8; i++) {
    HAL_GPIO_WritePin(GPIOA, (1 << i), (pattern >> i) & 0x01);
}
```

**ESP32:**
```c
#define LED_MASK ((1<<12)|(1<<13)|(1<<14)|(1<<15))

// Set all LEDs
GPIO.out_w1ts = LED_MASK;

// Clear all LEDs
GPIO.out_w1tc = LED_MASK;

// Write pattern
uint32_t pattern = (1<<12) | (1<<14); // Only LED1 & LED3 ON
GPIO.out = (GPIO.out & ~LED_MASK) | pattern;
```

---

### 🎯 GPIO COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Max Toggle Speed** | ⚡ 10 MHz (register) | ⚡ 2-3 MHz |
| **Atomic Operations** | ✅ BSRR (hardware) | ⚠️ Register-level |
| **Bit-Banding** | ✅ M3/M4 | ❌ No |
| **Speed Levels** | ✅ 4 levels | ❌ Fixed |
| **5V Tolerant** | ✅ Most pins (FT) | ❌ 3.3V only |
| **Open-Drain** | ✅ Native | ✅ Via config |
| **Pin Remapping** | ⚠️ 16 AF (limited) | ✅ ANY pin, ANY function |
| **Touch Sensors** | ❌ No | ✅ 10 pins |
| **Pull-up/down** | ✅ ~40kΩ | ✅ ~45kΩ |
| **Input-Only Pins** | ❌ No | ✅ GPIO 34-39 |
| **RTC GPIO** | ❌ No | ✅ For deep sleep |
| **Max Current/Pin** | 25 mA | 40 mA (total per group) |

---

### 💼 USE CASE RECOMMENDATIONS

**Pilih STM32 GPIO Jika:**
```
✅ Need high-speed toggling (>5 MHz)
✅ 5V sensor interfacing
✅ Hard real-time switching
✅ Motor control PWM
✅ Safety-critical (GPIO lock)
✅ Deterministic timing
```

**Pilih ESP32 GPIO Jika:**
```
✅ Need touch buttons (no mechanical parts)
✅ Flexible PCB routing (GPIO matrix)
✅ Deep sleep with GPIO control (RTC GPIO)
✅ Rapid prototyping
✅ Not speed-critical
```

---

### 📚 MINI PROJECT: Traffic Light System

**STM32 Implementation:**
```c
// PA0=Red, PA1=Yellow, PA2=Green
void traffic_light_cycle() {
    // Red
    GPIOA->BSRR = GPIO_PIN_0 | (GPIO_PIN_1 << 16) | (GPIO_PIN_2 << 16);
    HAL_Delay(5000);
    
    // Red + Yellow
    GPIOA->BSRR = GPIO_PIN_1;
    HAL_Delay(2000);
    
    // Green
    GPIOA->BSRR = GPIO_PIN_2 | (GPIO_PIN_0 << 16) | (GPIO_PIN_1 << 16);
    HAL_Delay(5000);
    
    // Yellow
    GPIOA->BSRR = GPIO_PIN_1 | (GPIO_PIN_2 << 16);
    HAL_Delay(2000);
}
```

**ESP32 Implementation:**
```cpp
#define RED_LED 12
#define YELLOW_LED 13
#define GREEN_LED 14

void trafficLightCycle() {
    // Red
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    delay(5000);
    
    // Red + Yellow
    digitalWrite(YELLOW_LED, HIGH);
    delay(2000);
    
    // Green
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    delay(5000);
    
    // Yellow
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    delay(2000);
}
```

---

## BAB 3: EXTERNAL INTERRUPTS & EVENT HANDLING

> **Tujuan Pembelajaran:**
> - Memahami konsep interrupt vs polling
> - Konfigurasi external interrupt (EXTI)
> - Interrupt priority dan nesting
> - Debouncing dalam interrupt
> - Critical section management

### 📖 PENJELASAN MATERI

**Apa itu Interrupt?**
Interrupt adalah mekanisme hardware yang memungkinkan MCU merespons event eksternal secara **asynchronous** tanpa perlu polling terus-menerus.

**Polling vs Interrupt:**
```
POLLING (Busy-Waiting):
├─ CPU terus-menerus cek status pin
├─ Waste CPU cycles
├─ Delay response time
└─ Simple, predictable

INTERRUPT (Event-Driven):
├─ CPU bebas jalankan task lain
├─ Instant response saat event
├─ Efficient power usage
└─ Complex, need careful handling
```

**Interrupt Lifecycle:**
```
1. Event terjadi (rising/falling edge)
2. Hardware set interrupt flag
3. CPU finish current instruction
4. Save context (PC, registers)
5. Jump to ISR (Interrupt Service Routine)
6. Execute ISR code
7. Restore context
8. Return to main program
```

**Best Practices ISR:**
```
✅ Keep ISR SHORT (< 100 µs)
✅ No delay() atau blocking calls
✅ No printf() dalam ISR (kecuali debug)
✅ Use volatile untuk shared variables
✅ Set flag, process di main loop
❌ No complex calculation
❌ No memory allocation
```

---

### 🔗 COMMON GROUND: Fitur Interrupt yang Sama

#### 1. **Basic External Interrupt**
**Deskripsi:** Respond to button press dengan interrupt

**STM32 (HAL):**
```c
// Program: Button Interrupt LED Toggle
// Deskripsi: Tombol di PA0, LED di PA5, toggle saat tombol ditekan

// Configuration (in CubeMX: PA0 = GPIO_EXTI0)
GPIO_InitTypeDef GPIO_InitStruct = {0};

GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // Falling edge trigger
GPIO_InitStruct.Pull = GPIO_PULLUP;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);

// ISR Handler
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
```

**ESP32 (ESP-IDF):**
```c
// Program: Button Interrupt LED Toggle
// Deskripsi: Tombol di GPIO4, LED di GPIO2, toggle saat tombol ditekan

#include "driver/gpio.h"

#define BUTTON_PIN GPIO_NUM_4
#define LED_PIN GPIO_NUM_2

void IRAM_ATTR gpio_isr_handler(void* arg) {
    static uint8_t led_state = 0;
    led_state = !led_state;
    gpio_set_level(LED_PIN, led_state);
}

void setup_interrupt() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // Falling edge
    };
    gpio_config(&io_conf);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, NULL);
}
```

---

#### 2. **Interrupt with Debouncing**
**Deskripsi:** Software debounce untuk menghindari multiple trigger

**STM32 (HAL):**
```c
// Program: Debounced Button Interrupt
// Deskripsi: Debounce 50ms untuk stabilitas

volatile uint8_t button_flag = 0;
uint32_t last_interrupt_time = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t interrupt_time = HAL_GetTick();
    
    // Debounce: ignore if < 50ms since last interrupt
    if (interrupt_time - last_interrupt_time > 50) {
        button_flag = 1;
    }
    last_interrupt_time = interrupt_time;
}

// Di main loop:
while(1) {
    if (button_flag) {
        button_flag = 0;
        // Process button press
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
```

**ESP32 (ESP-IDF):**
```c
// Program: Debounced Button Interrupt
// Deskripsi: Debounce 50ms untuk stabilitas

volatile uint8_t button_flag = 0;
uint32_t last_interrupt_time = 0;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t interrupt_time = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    if (interrupt_time - last_interrupt_time > 50) {
        button_flag = 1;
    }
    last_interrupt_time = interrupt_time;
}

// Di main task:
void app_main() {
    while(1) {
        if (button_flag) {
            button_flag = 0;
            gpio_set_level(LED_PIN, !gpio_get_level(LED_PIN));
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
```

---

#### 3. **Rising and Falling Edge Detection**
**Deskripsi:** Detect both edge untuk full wave monitoring

**STM32 (HAL):**
```c
// Program: Dual Edge Interrupt Counter
// Deskripsi: Count pulses pada both rising dan falling edge

volatile uint32_t pulse_count = 0;

GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;  // Both edges
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        pulse_count++;
    }
}
```

**ESP32 (ESP-IDF):**
```c
// Program: Dual Edge Interrupt Counter  
// Deskripsi: Count pulses pada both rising dan falling edge

volatile uint32_t pulse_count = 0;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    pulse_count++;
}

io_conf.intr_type = GPIO_INTR_ANYEDGE;  // Both edges
gpio_config(&io_conf);
```

---

#### 4. **Multiple Interrupt Sources**
**Deskripsi:** Handle multiple buttons dengan ISR yang sama

**STM32 (HAL):**
```c
// Program: Multiple Button Handler
// Deskripsi: 3 tombol (PA0, PA1, PA2) control 3 LED berbeda

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch(GPIO_Pin) {
        case GPIO_PIN_0:
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);  // LED1
            break;
        case GPIO_PIN_1:
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);  // LED2
            break;
        case GPIO_PIN_2:
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);  // LED3
            break;
    }
}
```

**ESP32 (ESP-IDF):**
```c
// Program: Multiple Button Handler
// Deskripsi: 3 tombol (GPIO4, GPIO5, GPIO18) control 3 LED berbeda

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    
    switch(gpio_num) {
        case 4:
            gpio_set_level(GPIO_NUM_12, !gpio_get_level(GPIO_NUM_12));
            break;
        case 5:
            gpio_set_level(GPIO_NUM_13, !gpio_get_level(GPIO_NUM_13));
            break;
        case 18:
            gpio_set_level(GPIO_NUM_14, !gpio_get_level(GPIO_NUM_14));
            break;
    }
}

// Register multiple handlers
gpio_isr_handler_add(GPIO_NUM_4, gpio_isr_handler, (void*)4);
gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, (void*)5);
gpio_isr_handler_add(GPIO_NUM_18, gpio_isr_handler, (void*)18);
```

---

#### 5. **Deferred Interrupt Processing**
**Deskripsi:** ISR set flag, main loop process (best practice)

**STM32 (HAL):**
```c
// Program: Deferred Processing Pattern
// Deskripsi: ISR cepat, heavy processing di main loop

volatile uint8_t process_flag = 0;
volatile uint32_t sensor_data = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // Quick: just read and set flag
    sensor_data = HAL_ADC_GetValue(&hadc1);
    process_flag = 1;
}

int main() {
    while(1) {
        if (process_flag) {
            process_flag = 0;
            
            // Heavy processing here (safe, not in ISR)
            float voltage = sensor_data * 3.3f / 4095.0f;
            printf("Voltage: %.2f V\n", voltage);
            
            // Complex calculations OK here
            float temperature = calculate_temperature(voltage);
            update_display(temperature);
        }
    }
}
```

**ESP32 (ESP-IDF with FreeRTOS):**
```c
// Program: Deferred Processing with Task Notification
// Deskripsi: ISR notify task, task process data

TaskHandle_t processing_task_handle;
volatile uint32_t sensor_data = 0;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Quick: read data
    sensor_data = adc1_get_raw(ADC1_CHANNEL_0);
    
    // Notify task
    vTaskNotifyGiveFromISR(processing_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void processing_task(void* arg) {
    while(1) {
        // Wait for notification from ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Heavy processing here
        float voltage = sensor_data * 3.3f / 4095.0f;
        ESP_LOGI(TAG, "Voltage: %.2f V", voltage);
        
        // Complex calculations OK
        float temperature = calculate_temperature(voltage);
        update_display(temperature);
    }
}
```

---

### ⭐ STM32 ADVANTAGES: Keunggulan Interrupt STM32

#### 1. **NVIC: Priority Grouping & Nesting**
**Deskripsi:** Interrupt bisa interrupt interrupt lain (nested)

```c
// Program: Nested Interrupt Demo
// Deskripsi: High priority interrupt bisa preempt low priority

// Set priority group (0-4 preemption bits)
HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

// Critical button: Priority 0 (highest)
HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);  // Preempt=0, Sub=0
HAL_NVIC_EnableIRQ(EXTI0_IRQn);

// Normal button: Priority 2 (lower)
HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);  // Preempt=2, Sub=0  
HAL_NVIC_EnableIRQ(EXTI1_IRQn);

// Jika EXTI1 sedang execute, EXTI0 bisa interrupt!
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // Critical handler (can preempt PIN1)
        emergency_stop();
    }
    else if (GPIO_Pin == GPIO_PIN_1) {
        // Normal handler (can be preempted by PIN0)
        HAL_Delay(100);  // Slow processing
        process_normal_event();
    }
}
```

**Keunggulan:**
- ✅ True nested interrupt (16 priority levels)
- ✅ Critical events dijamin dihandle
- ✅ Deterministic latency (12 cycles)

---

#### 2. **EXTI Line Configuration Flexibility**
**Deskripsi:** Advanced trigger modes

```c
// Program: EXTI Software Trigger
// Deskripsi: Trigger interrupt via software (testing/simulation)

// Enable software trigger
HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);

// Software trigger interrupt
__HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_0);

// Use case: Simulate button press untuk testing
void simulate_button_press() {
    __HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_0);
    // ISR akan dipanggil tanpa hardware event!
}
```

---

#### 3. **Hardware Debouncing (External)**
**Deskripsi:** Gunakan timer interrupt untuk debounce

```c
// Program: Hardware Timer Debounce
// Deskripsi: Timer-based debounce (100% reliable)

volatile uint8_t button_state = 0;
volatile uint8_t debounce_counter = 0;

// Timer ISR (1 kHz = 1ms tick)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    uint8_t current = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    
    if (current == button_state) {
        debounce_counter = 0;
    } else {
        debounce_counter++;
        if (debounce_counter >= 50) {  // 50ms stable
            button_state = current;
            if (button_state == 0) {  // Pressed
                // Debounced event!
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            }
            debounce_counter = 0;
        }
    }
}
```

---

#### 4. **Bit-Banding untuk Thread-Safe Flag**
**Deskripsi:** Atomic flag tanpa disable interrupt

```c
// Program: Atomic Flag dengan Bit-Banding
// Deskripsi: Set flag tanpa race condition

// Define flag di SRAM bit-band region
volatile uint32_t flags = 0;
#define FLAG_BIT0 (*((volatile uint32_t*)BITBAND_SRAM(&flags, 0)))

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // Atomic set (1 instruction, no lock needed!)
    FLAG_BIT0 = 1;
}

int main() {
    while(1) {
        if (FLAG_BIT0) {  // Atomic read
            FLAG_BIT0 = 0;  // Atomic clear
            process_event();
        }
    }
}
```

---

#### 5. **SWD Debug Interrupt**
**Deskripsi:** Debug interrupt dengan breakpoint di ISR

```c
// Program: ISR Debugging dengan SWD
// Deskripsi: Live debugging interrupt behavior

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    static uint32_t count = 0;
    count++;  // ← Set breakpoint here!
    
    // Bisa lihat:
    // - interrupt count
    // - timing between interrupts
    // - stack trace
    // - peripheral registers
    
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}

// Di STM32CubeIDE:
// - Set breakpoint di ISR
// - Watch 'count' variable
// - Live Expressions untuk register
// - SWV trace untuk timing analysis
```

---

#### 6. **Wake from Sleep via EXTI**
**Deskripsi:** Interrupt bangunkan MCU dari low-power mode

```c
// Program: Sleep Mode dengan EXTI Wake-up
// Deskripsi: MCU sleep, button wake up

void enter_sleep_mode() {
    // Configure EXTI to wake from sleep
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);  // PA0
    
    // Enter sleep mode
    HAL_SuspendTick();
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_ResumeTick();
    
    // Akan bangun saat EXTI0 trigger!
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // MCU just woke up!
        handle_wakeup_event();
    }
}
```

---

### ⚡ ESP32 ADVANTAGES: Keunggulan Interrupt ESP32

#### 1. **Any GPIO Can Interrupt (No EXTI Limitation)**
**Deskripsi:** Semua GPIO bisa interrupt, tidak ada batasan line

```c
// Program: Unlimited GPIO Interrupt
// Deskripsi: 10+ buttons pada GPIO berbeda, semua bisa interrupt!

// STM32: Hanya 1 pin per EXTI line (max 16 simultaneously)
// ESP32: SEMUA GPIO bisa interrupt bersamaan!

#define BUTTON_COUNT 10
const uint8_t button_pins[BUTTON_COUNT] = {
    4, 5, 12, 13, 14, 15, 18, 19, 21, 22  // 10 pins!
};

void IRAM_ATTR multi_gpio_isr(void* arg) {
    uint32_t gpio_num = (uint32_t)arg;
    printf("Button %d pressed\n", gpio_num);
}

void setup_all_interrupts() {
    for(int i = 0; i < BUTTON_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << button_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .intr_type = GPIO_INTR_NEGEDGE
        };
        gpio_config(&io_conf);
        gpio_isr_handler_add(button_pins[i], multi_gpio_isr, (void*)button_pins[i]);
    }
}

// Tidak ada batasan EXTI line!
```

**Keunggulan:**
- ✅ No EXTI line conflict
- ✅ Unlimited simultaneous interrupt sources
- ✅ Perfect untuk keypad matrix besar

---

#### 2. **FreeRTOS Task Notification (Built-in)**
**Deskripsi:** Efficient ISR-to-Task communication

```c
// Program: ISR to Task Direct Notification
// Deskripsi: Zero-copy, low-latency event passing

TaskHandle_t sensor_task_handle;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Send value directly to task (no queue overhead!)
    uint32_t sensor_value = read_sensor();
    xTaskNotifyFromISR(sensor_task_handle, 
                       sensor_value, 
                       eSetValueWithOverwrite,
                       &xHigherPriorityTaskWoken);
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void sensor_task(void* arg) {
    uint32_t notification_value;
    
    while(1) {
        // Wait for notification (blocks, zero CPU)
        xTaskNotifyWait(0, 0, &notification_value, portMAX_DELAY);
        
        // Process immediately after ISR
        process_sensor_data(notification_value);
    }
}

// Lebih cepat dari queue, lebih efficient!
```

---

#### 3. **Per-Core Interrupt Affinity**
**Deskripsi:** Assign interrupt ke specific core

```c
// Program: Dual-Core Interrupt Distribution
// Deskripsi: Critical interrupt di core 0, normal di core 1

// Critical sensor interrupt → Core 0 (protokol core)
void setup_critical_interrupt() {
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_CORE0);
    gpio_isr_handler_add(CRITICAL_PIN, critical_isr, NULL);
}

// Normal button interrupt → Core 1 (application core)  
void setup_normal_interrupt() {
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_CORE1);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, NULL);
}

// Load balancing untuk performance!
```

---

#### 4. **GPIO Wakeup dari Deep Sleep**
**Deskripsi:** Multiple GPIO wakeup sources

```c
// Program: Multi-Source Deep Sleep Wakeup
// Deskripsi: Bangun dari deep sleep via multiple GPIO

void configure_wakeup() {
    // Configure multiple wakeup pins
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);  // LOW level
    
    // Or multiple pins (ext1)
    const uint64_t wakeup_pin_mask = 
        (1ULL << GPIO_NUM_25) | 
        (1ULL << GPIO_NUM_26) | 
        (1ULL << GPIO_NUM_27);
    
    esp_sleep_enable_ext1_wakeup(wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // Enter deep sleep
    esp_deep_sleep_start();
    
    // Bangun saat any pin trigger!
}

void app_main() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            printf("Wakeup from EXT0\n");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            printf("Wakeup from EXT1: GPIO %llx\n", 
                   esp_sleep_get_ext1_wakeup_status());
            break;
    }
}
```

---

#### 5. **Touch Sensor Interrupt**
**Deskripsi:** Capacitive touch trigger interrupt

```c
// Program: Touch Interrupt Wakeup
// Deskripsi: Touch sensor bangunkan dari sleep

void setup_touch_interrupt() {
    // Configure touch pad
    touch_pad_init();
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    
    touch_pad_config(TOUCH_PAD_NUM9, 0);  // GPIO32
    
    // Set interrupt threshold
    uint16_t touch_value;
    touch_pad_read(TOUCH_PAD_NUM9, &touch_value);
    touch_pad_set_thresh(TOUCH_PAD_NUM9, touch_value * 0.8);
    
    // Enable interrupt
    touch_pad_isr_register(touch_isr, NULL);
    touch_pad_intr_enable();
}

void IRAM_ATTR touch_isr(void* arg) {
    uint32_t pad_intr = touch_pad_get_status();
    touch_pad_clear_status();
    
    if (pad_intr & (1 << TOUCH_PAD_NUM9)) {
        // Touch detected!
        gpio_set_level(LED_PIN, 1);
    }
}

// Enable deep sleep wakeup
esp_sleep_enable_touchpad_wakeup();
esp_deep_sleep_start();
```

---

#### 6. **High-Level Interrupt (Level 3)**
**Deskripsi:** Ultra-high priority interrupt

```c
// Program: Time-Critical Interrupt
// Deskripsi: Level 3 interrupt untuk timing critical

void IRAM_ATTR high_priority_isr(void* arg) {
    // This ISR has very high priority
    // Can interrupt normal ISRs
    
    // MUST be in IRAM!
    // CANNOT call most ESP-IDF functions
    // Only direct register access
    
    GPIO.out_w1ts = (1 << RESPONSE_PIN);  // Set pin HIGH
    // < 1 µs response time!
}

// Install with high priority
esp_intr_alloc(ETS_GPIO_INTR_SOURCE,
               ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM,
               high_priority_isr,
               NULL,
               &isr_handle);
```

---

### 📊 INTERRUPT COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Priority Levels** | 16 (4-bit) | 7 (Level 0-6) |
| **Nested Interrupt** | ✅ Full support | ✅ Supported |
| **Max Interrupt Pins** | 16 (EXTI0-15) | ✅ ALL GPIO (~40) |
| **Pin Sharing Issue** | ⚠️ 1 pin per EXTI line | ✅ No limitation |
| **Latency** | ⚡ 12 cycles (M4) | ⚡ <100 ns |
| **Wake from Sleep** | ✅ EXTI wakeup | ✅ Multiple sources |
| **Debugging** | ⭐⭐⭐⭐⭐ SWD/SWV | ⭐⭐⭐ Printf/JTAG |
| **Bit-Banding** | ✅ M3/M4 (atomic) | ❌ No |
| **FreeRTOS Notify** | ⚠️ Manual | ✅ Built-in |
| **Touch Interrupt** | ❌ No | ✅ Hardware touch |
| **Core Affinity** | ❌ Single core | ✅ Pin to core |

---

### 💼 MINI PROJECT: Reaction Time Tester

**Deskripsi:** Game untuk test reaksi: LED random nyala, user tekan button, hitung waktu reaksi

**STM32 Implementation:**
```c
// Program: Reaction Time Game
// PA0 = Button, PA5 = LED, USART untuk display

volatile uint32_t led_on_time = 0;
volatile uint32_t button_press_time = 0;
volatile uint8_t game_active = 0;

void start_game() {
    // Random delay 1-5 detik
    uint32_t delay = (rand() % 4000) + 1000;
    HAL_Delay(delay);
    
    // Nyalakan LED
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    led_on_time = HAL_GetTick();
    game_active = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0 && game_active) {
        button_press_time = HAL_GetTick();
        game_active = 0;
        
        uint32_t reaction_time = button_press_time - led_on_time;
        printf("Reaction time: %lu ms\n", reaction_time);
        
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        
        // Evaluate
        if (reaction_time < 200) {
            printf("EXCELLENT!\n");
        } else if (reaction_time < 300) {
            printf("GOOD\n");
        } else {
            printf("SLOW\n");
        }
    }
}
```

**ESP32 Implementation:**
```c
// Program: Reaction Time Game dengan WiFi Leaderboard

volatile uint32_t led_on_time = 0;
volatile uint32_t button_press_time = 0;
volatile uint8_t game_active = 0;

void IRAM_ATTR button_isr(void* arg) {
    if (game_active) {
        button_press_time = esp_timer_get_time() / 1000;  // µs → ms
        game_active = 0;
    }
}

void game_task(void* arg) {
    while(1) {
        // Random delay
        vTaskDelay((rand() % 4000 + 1000) / portTICK_PERIOD_MS);
        
        // LED ON
        gpio_set_level(LED_PIN, 1);
        led_on_time = esp_timer_get_time() / 1000;
        game_active = 1;
        
        // Wait for button
        while(game_active) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        
        // Calculate
        uint32_t reaction_time = button_press_time - led_on_time;
        printf("Reaction: %lu ms\n", reaction_time);
        
        gpio_set_level(LED_PIN, 0);
        
        // BONUS: Post to WiFi leaderboard!
        post_to_server(reaction_time);
        
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
```

---

STM32 menggunakan arsitektur ARM Cortex-M dengan berbagai varian:

| Core | Arsitektur | Clock Max | Features | STM32 Series |
|------|-----------|-----------|----------|--------------|
| **Cortex-M0** | ARMv6-M | 48 MHz | Ultra low power, basic | STM32F0, L0 |
| **Cortex-M0+** | ARMv6-M | 48 MHz | M0 + optimized | STM32L0+ |
| **Cortex-M3** | ARMv7-M | 72-120 MHz | DSP basic, bit-banding | STM32F1, F2, L1 |
| **Cortex-M4** | ARMv7-M | 80-180 MHz | DSP + FPU, bit-banding | STM32F3, F4, L4 |
| **Cortex-M7** | ARMv7E-M | 400-550 MHz | Dual-issue, cache, FPU | STM32F7, H7 |
| **Cortex-M33** | ARMv8-M | 120 MHz | TrustZone security | STM32L5, U5 |

**Karakteristik Utama:**
- **RISC Architecture**: Load/Store machine, operasi hanya pada register CPU
- **Instruction Set**: Thumb-2 (campuran 16-bit dan 32-bit instructions)
- **Memory Map**: Fixed 4GB address space (standardized across vendors)
- **Bit-Banding**: Atomic bit manipulation (M3/M4 only)
- **Deterministic**: Predictable execution time untuk real-time

#### **1.2 Register Set**

**General Purpose Registers (R0-R12):**
- R0-R12: 13 general-purpose 32-bit registers
- R13 (SP): Stack Pointer - banked (MSP/PSP untuk RTOS)
- R14 (LR): Link Register - menyimpan return address
- R15 (PC): Program Counter

**Special Registers:**
- **PSR** (Program Status Register): Flags (N, Z, C, V)
- **PRIMASK**: Global interrupt mask
- **FAULTMASK**: Fault exception mask
- **BASEPRI**: Priority-based interrupt masking
- **CONTROL**: Thread mode privilege & stack selection

#### **1.3 Memory Layout (Fixed)**

```
0x0000 0000 - 0x1FFF FFFF  →  CODE (512 MB)
    ├─ 0x0000 0000: Aliased Flash/System Memory (bootloader)
    ├─ 0x0800 0000: Main Flash Memory (program code)
    └─ 0x1FFF 0000: System Memory (ST Bootloader ROM)

0x2000 0000 - 0x3FFF FFFF  →  SRAM (512 MB region)
    └─ 0x2000 0000: Internal SRAM start

0x4000 0000 - 0x5FFF FFFF  →  PERIPHERALS (512 MB)
    ├─ APB1 Bus: Low-speed peripherals
    ├─ APB2 Bus: High-speed peripherals
    └─ AHB Bus: DMA, GPIO, high-performance peripherals

0x6000 0000 - 0x9FFF FFFF  →  EXTERNAL RAM/FLASH (1 GB)
    └─ FSMC/FMC interface region

0xE000 0000 - 0xFFFF FFFF  →  SYSTEM (512 MB)
    ├─ 0xE000 E000: NVIC, SysTick, SCB
    └─ Private peripheral bus (core peripherals)
```

#### **1.4 Bit-Banding Feature**

STM32 (Cortex-M3/M4) mendukung **bit-banding** untuk akses atomik:

**Formula:**
```
bit_band_addr = alias_base + (byte_offset × 32) + (bit_number × 4)
```

**Contoh Implementasi:**
```c
// Macro untuk SRAM bit-banding
#define BITBAND_SRAM_BASE   0x20000000
#define ALIAS_SRAM_BASE     0x22000000
#define BITBAND_SRAM(a,b) \
    ((ALIAS_SRAM_BASE + ((uint32_t)&(a)-BITBAND_SRAM_BASE)*32 + (b*4)))

// Macro untuk Peripheral bit-banding
#define BITBAND_PERI_BASE   0x40000000
#define ALIAS_PERI_BASE     0x42000000
#define BITBAND_PERI(a,b) \
    ((ALIAS_PERI_BASE + ((uint32_t)a-BITBAND_PERI_BASE)*32 + (b*4)))

// Contoh: Set bit 5 dari GPIOA_ODR
#define GPIOA_ODR 0x40020014
uint32_t *pin5 = (uint32_t*)BITBAND_PERI(GPIOA_ODR, 5);
*pin5 = 1; // Atomic bit set - hanya 1 instruksi!
```

**Keuntungan:**
- ✅ Atomic operation (thread-safe tanpa disable interrupt)
- ✅ Lebih cepat daripada read-modify-write
- ✅ Menghindari race condition

---

### ESP32 (Xtensa LX6)

#### **1.5 Dual-Core Xtensa**

**Arsitektur:**
- **Processor**: Tensilica Xtensa LX6 (32-bit)
- **Cores**: Dual-core (PRO_CPU & APP_CPU)
- **Clock**: 80-240 MHz (adjustable)
- **Instruction Set**: Xtensa ISA (non-ARM)

**Spesifikasi Core:**

| Feature | Detail |
|---------|--------|
| **Architecture** | Harvard architecture (separate instruction/data bus) |
| **Pipeline** | 7-stage pipeline |
| **Registers** | 64 general-purpose 32-bit registers (windowed) |
| **FPU** | Single-precision floating point (optional) |
| **DSP Extensions** | MAC (Multiply-Accumulate), fixed-point |
| **Cache** | 32 KB instruction cache per core |

#### **1.6 Register Windowing**

ESP32 menggunakan **windowed register set**:
- Total: 64 registers fisik
- Visible: 16 registers per window (a0-a15)
- Automatic spill/fill saat function call
- Mengurangi overhead context switching

**Contoh:**
```
Window 0: a0-a15 → Physical R0-R15
Window 1: a0-a15 → Physical R4-R19 (overlap 12 registers)
```

#### **1.7 Memory Map ESP32**

```
0x0000 0000 - 0x3F3F FFFF  →  Internal ROM (448 KB)
    └─ Bootloader code & lookup tables

0x3FF0 0000 - 0x3FF7 FFFF  →  Internal SRAM 0 (192 KB)
    └─ DMA accessible

0x3FF8 0000 - 0x3FFF FFFF  →  Internal SRAM 1 (128 KB)
    └─ Cache memory (instruction/data)

0x3F40 0000 - 0x3F7F FFFF  →  Peripheral registers (4 MB)

0x4000 0000 - 0x400C 1FFF  →  Internal SRAM 2 (200 KB)
    └─ Instruction bus

0x5000 0000 - 0x5001 FFFF  →  RTC FAST Memory (8 KB)
    └─ Accessible during deep sleep

0x5000 0000 - 0x5000 1FFF  →  RTC SLOW Memory (8 KB)
    └─ Ultra-low power retention

External Flash: Mapped via cache (0x400C2000 - 0x40BF FFFF)
    └─ Up to 16 MB via SPI
```

**Perbedaan Kunci dengan STM32:**
- ✅ **Harvard Architecture**: Separate instruction & data buses
- ✅ **Cache Memory**: Hardware I-Cache untuk flash execution
- ✅ **RTC Memory**: Persistent memory during deep sleep
- ❌ **No Fixed Standard**: Memory map tidak standardized seperti ARM

---

### PERBANDINGAN ARSITEKTUR

| Aspek | STM32 (Cortex-M) | ESP32 (Xtensa LX6) |
|-------|------------------|---------------------|
| **ISA** | ARM Thumb-2 (standard) | Xtensa (proprietary) |
| **Cores** | Single core (kecuali H7 dual) | Dual-core native |
| **Clock** | 48-550 MHz | 80-240 MHz |
| **Pipeline** | 3-stage (M0/M3), 6-stage (M7) | 7-stage superscalar |
| **FPU** | Optional (M4F, M7) | Optional (per core) |
| **Register** | 13 GPR + special | 64 GPR (windowed) |
| **Bit-Banding** | ✅ M3/M4 only | ❌ No |
| **Memory Model** | Von Neumann | Harvard |
| **Cache** | M7 only (L1 I/D) | Built-in I-Cache |
| **Determinism** | ✅ Excellent (RTOS) | ⚠️ Good (cache effects) |
| **Toolchain** | Standard ARM GCC | Xtensa GCC (ESP-IDF) |

---

## 2. SPESIFIKASI HARDWARE

### STM32 Portfolio (17 Series)

| Series | Core | MHz | Flash | RAM | Focus | Price |
|--------|------|-----|-------|-----|-------|-------|
| **F0** | M0 | 48 | 16-256 KB | 4-32 KB | Entry-level | $ |
| **F1** | M3 | 72 | 16-1024 KB | 4-96 KB | Mainstream | $ |
| **F2** | M3 | 120 | 128-1024 KB | 64-128 KB | Performance | $$ |
| **F3** | M4F | 72 | 16-512 KB | 16-80 KB | Mixed-signal | $$ |
| **F4** | M4F | 180 | 128-2048 KB | 64-384 KB | High-perf DSP | $$$ |
| **F7** | M7F | 216 | 512-2048 KB | 320-512 KB | Ultra-perf | $$$$ |
| **L0** | M0+ | 32 | 8-192 KB | 2-20 KB | Ultra-low-power | $ |
| **L1** | M3 | 32 | 32-512 KB | 10-80 KB | Low-power | $$ |
| **L4** | M4F | 80 | 128-1024 KB | 64-320 KB | Power efficient | $$$ |
| **L5** | M33 | 110 | 256-512 KB | 128-256 KB | Secure low-power | $$$$ |
| **H7** | M7F | 550 | 128-2048 KB | 1 MB | Extreme perf | $$$$$ |
| **G0** | M0+ | 64 | 16-512 KB | 8-144 KB | Value line | $ |
| **G4** | M4F | 170 | 128-512 KB | 32-128 KB | Analog+DSP | $$$ |
| **U5** | M33 | 160 | 512-4096 KB | 768 KB | Ultra-low-power | $$$$ |
| **WB** | M4F | 64 | 256-1024 KB | 256 KB | Wireless BLE | $$$ |
| **WL** | M4 | 48 | 64-256 KB | 20-64 KB | LoRa/Sub-GHz | $$$ |
| **MP1** | M4+A7 | 650 | - | - | Linux MPU | $$$$$ |

### ESP32 Family

| Variant | Core | MHz | Flash | RAM | Features | Price |
|---------|------|-----|-------|-----|----------|-------|
| **ESP32** | Dual LX6 | 240 | Up to 16 MB | 520 KB | WiFi + BT Classic + BLE | $$ |
| **ESP32-S2** | Single LX7 | 240 | Up to 4 MB | 320 KB | WiFi only, USB OTG | $ |
| **ESP32-S3** | Dual LX7 | 240 | Up to 8 MB | 512 KB | WiFi + BLE, AI accel | $$$ |
| **ESP32-C3** | RISC-V | 160 | Up to 4 MB | 400 KB | WiFi + BLE, low cost | $ |
| **ESP32-C6** | RISC-V | 160 | Up to 4 MB | 512 KB | WiFi 6 + BLE + Zigbee | $$ |
| **ESP32-H2** | RISC-V | 96 | Up to 4 MB | 320 KB | BLE + Zigbee + Thread | $ |

**Catatan:** Flash pada ESP32 biasanya external SPI flash, bukan on-chip.

---

## 3. MEMORI DAN ORGANISASI

### 3.1 STM32 Memory Architecture

#### **Flash Memory**

**Karakteristik:**
- **Type**: Embedded Flash (on-die)
- **Organization**: Banks, Sectors, Pages (tergantung series)
- **Endurance**: 10,000 - 100,000 cycles (tergantung series)
- **Retention**: 20-30 years @ 25°C

**Write Protection:**
- Write Protection (WRP) per sector/page
- Read Protection (RDP) 3 levels:
  - Level 0: No protection
  - Level 1: Debug disabled, flash read protection
  - Level 2: Permanent protection (irreversible!)

**Flash Operations (HAL):**
```c
// Unlock flash
HAL_FLASH_Unlock();

// Erase sector
FLASH_EraseInitTypeDef EraseInit;
EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
EraseInit.Sector = FLASH_SECTOR_1;
EraseInit.NbSectors = 1;
EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
HAL_FLASHEx_Erase(&EraseInit, &SectorError);

// Program word
HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, Data);

// Lock flash
HAL_FLASH_Lock();
```

#### **SRAM Organization**

**STM32F4 Example:**
```
┌─────────────────────────────────────┐
│   SRAM1 (112 KB)                    │  0x2000 0000 - 0x2001 BFFF
│   - General purpose                  │
│   - DMA accessible                   │
├─────────────────────────────────────┤
│   SRAM2 (16 KB)                     │  0x2001 C000 - 0x2001 FFFF
│   - Separate power domain            │
│   - Can be retained in Stop mode     │
├─────────────────────────────────────┤
│   CCM RAM (64 KB)                   │  0x1000 0000 - 0x1000 FFFF
│   - Core-Coupled Memory              │
│   - Zero wait state                  │
│   - NOT DMA accessible               │
│   - Faster than SRAM1/2              │
└─────────────────────────────────────┘
```

**CCM (Core-Coupled Memory) - STM32F3/F4:**
- Terhubung langsung ke bus D (data bus)
- Zero wait state access
- Ideal untuk: Stack, critical variables, time-sensitive data
- **Limitation**: DMA tidak bisa akses (harus gunakan SRAM1)

**Backup SRAM - STM32F4/F7/H7:**
- Battery-backed SRAM (dengan VBAT)
- Tetap hidup saat VDD mati
- Ukuran: 4 KB (F4/L4)
- Use case: RTC data, crash logs, license keys

---

### 3.2 ESP32 Memory Architecture

#### **Internal Memory**

**SRAM Distribution:**

| Region | Size | Address | Characteristics | Usage |
|--------|------|---------|-----------------|-------|
| **SRAM0** | 192 KB | 0x3FFE0000 | DMA, cache | Heap, data |
| **SRAM1** | 128 KB | 0x3FF80000 | Cache only | Instruction cache |
| **SRAM2** | 200 KB | 0x40000000 | Instruction bus | Code execution |
| **RTC FAST** | 8 KB | 0x50000000 | Deep sleep retention | ULP, wake stubs |
| **RTC SLOW** | 8 KB | 0x50001000 | Ultra-low power | ULP code |

**Total:** ~520 KB internal SRAM

#### **External Flash**

**Connection:**
- Via SPI (Quad-SPI untuk bandwidth tinggi)
- Typical: 4 MB, 8 MB, 16 MB
- Memory-mapped via cache (MMU)

**Flash Memory Map:**
```
0x3F400000 - 0x3F7FFFFF  →  Data bus (cache via MMU)
0x400C2000 - 0x40BFFFFF  →  Instruction bus (cache)
```

**Partitioning:**
```
┌──────────────────────────────────────┐
│  Bootloader (0x1000)                  │  32 KB
├──────────────────────────────────────┤
│  Partition Table (0x8000)             │  4 KB
├──────────────────────────────────────┤
│  NVS (Non-Volatile Storage)           │  24 KB
│  - WiFi credentials, config           │
├──────────────────────────────────────┤
│  OTA Data (0x10000)                   │  8 KB
├──────────────────────────────────────┤
│  Factory App (0x20000)                │  1-3 MB
├──────────────────────────────────────┤
│  OTA_0 (for firmware update)          │  1-3 MB
├──────────────────────────────────────┤
│  OTA_1 (backup firmware)              │  1-3 MB
├──────────────────────────────────────┤
│  SPIFFS / LittleFS (file system)      │  Remaining
└──────────────────────────────────────┘
```

**NVS (Non-Volatile Storage) API:**
```c
#include "nvs_flash.h"
#include "nvs.h"

// Initialize
nvs_flash_init();

// Open namespace
nvs_handle_t my_handle;
nvs_open("storage", NVS_READWRITE, &my_handle);

// Write
int32_t value = 42;
nvs_set_i32(my_handle, "my_key", value);
nvs_commit(my_handle);

// Read
int32_t read_value = 0;
nvs_get_i32(my_handle, "my_key", &read_value);

// Close
nvs_close(my_handle);
```

---

### PERBANDINGAN MEMORY

| Aspek | STM32 | ESP32 |
|-------|-------|-------|
| **Flash Type** | On-chip embedded | External SPI |
| **Flash Speed** | Direct CPU access | Via cache (slower) |
| **Flash Endurance** | 10K-100K cycles | ~100K cycles (typical) |
| **SRAM Speed** | Zero/few wait states | Variable (bus dependent) |
| **Special RAM** | CCM (fast), Backup SRAM | RTC FAST/SLOW |
| **Cache** | M7 only | Built-in I-Cache |
| **DMA Limitations** | CCM not accessible | Some regions restricted |
| **Memory Protection** | MPU (optional) | MMU + PMS (permission) |
| **Persistent Storage** | Internal flash | NVS partition in flash |

---

## 4. GPIO DAN DIGITAL I/O

### 4.1 STM32 GPIO

#### **GPIO Modes**

STM32 GPIO sangat fleksibel dengan 4 mode dasar:

```c
typedef enum {
    GPIO_MODE_INPUT              = 0x00,  // Input floating
    GPIO_MODE_OUTPUT_PP          = 0x01,  // Output push-pull
    GPIO_MODE_OUTPUT_OD          = 0x11,  // Output open-drain
    GPIO_MODE_AF_PP              = 0x02,  // Alternate function push-pull
    GPIO_MODE_AF_OD              = 0x12,  // Alternate function open-drain
    GPIO_MODE_ANALOG             = 0x03,  // Analog mode (ADC/DAC)
    GPIO_MODE_IT_RISING          = 0x10,  // External interrupt rising
    GPIO_MODE_IT_FALLING         = 0x20,  // External interrupt falling
    GPIO_MODE_IT_RISING_FALLING  = 0x30,  // Interrupt both edges
    GPIO_MODE_EVT_RISING         = 0x10,  // Event rising
    GPIO_MODE_EVT_FALLING        = 0x20,  // Event falling
    GPIO_MODE_EVT_RISING_FALLING = 0x30   // Event both edges
} GPIO_ModeTypeDef;
```

**Speed Options:**
```c
GPIO_SPEED_FREQ_LOW       // Up to 8 MHz
GPIO_SPEED_FREQ_MEDIUM    // Up to 50 MHz
GPIO_SPEED_FREQ_HIGH      // Up to 100 MHz
GPIO_SPEED_FREQ_VERY_HIGH // Up to 180 MHz (F4/F7/H7)
```

**Pull-up/Pull-down:**
```c
GPIO_NOPULL   // Floating input
GPIO_PULLUP   // Internal pull-up (~40kΩ)
GPIO_PULLDOWN // Internal pull-down (~40kΩ)
```

#### **Contoh Konfigurasi (HAL)**

```c
// Inisialisasi GPIO (LED - Output Push-Pull)
GPIO_InitTypeDef GPIO_InitStruct = {0};

__HAL_RCC_GPIOA_CLK_ENABLE();  // Enable clock untuk GPIOA

GPIO_InitStruct.Pin = GPIO_PIN_5;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

// Write HIGH/LOW
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

// Toggle
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

// Read input
GPIO_PinState state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
```

#### **Atomic GPIO Operations (Register Level)**

```c
// Direct register access (faster than HAL)
GPIOA->BSRR = GPIO_PIN_5;        // Set bit (atomic)
GPIOA->BSRR = (GPIO_PIN_5 << 16); // Reset bit (atomic)

// Using bit-banding (M3/M4 only)
#define GPIOA_ODR_ADDR 0x40020014
#define PIN5_BB_ADDR   BITBAND_PERI(GPIOA_ODR_ADDR, 5)
*(uint32_t*)PIN5_BB_ADDR = 1; // Set pin 5 (single instruction)
```

#### **Alternate Functions**

STM32 GPIO pins dapat di-remap ke fungsi peripheral:

**Contoh: PA9/PA10 sebagai USART1 TX/RX**
```c
GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_PULLUP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF7_USART1;  // AF7 = USART1
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
```

**AF Mapping** berbeda per pin dan per series - lihat datasheet!

---

### 4.2 ESP32 GPIO

#### **GPIO Pin Capabilities**

ESP32 memiliki **39 GPIO pins** (ESP32-WROOM-32), tetapi tidak semua tersedia:

| GPIO Range | Total | Notes |
|------------|-------|-------|
| GPIO 0-19 | 20 pins | General purpose |
| GPIO 21-23 | 3 pins | General purpose |
| GPIO 25-27 | 3 pins | DAC capable |
| GPIO 32-39 | 8 pins | **INPUT ONLY** (no pull-up) |
| GPIO 34-39 | 6 pins | ADC1 channel |

**Restricted Pins:**
```
GPIO 6-11  →  Connected to integrated SPI flash (DO NOT USE)
GPIO 1/3   →  UART0 TX/RX (Serial monitor - use with caution)
GPIO 0     →  Boot mode selection (pull-down to enter bootloader)
GPIO 2     →  Boot mode (must be floating/high during boot)
GPIO 5     →  Boot mode (SDIO timing)
GPIO 12    →  Boot mode (voltage selection)
GPIO 15    →  Boot mode (silent boot if low)
```

#### **GPIO Modes**

```c
#include "driver/gpio.h"

// Input mode
gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);  // or PULLDOWN_ONLY, PULLUP_PULLDOWN

// Output mode
gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);

// Read/Write
int level = gpio_get_level(GPIO_NUM_4);
gpio_set_level(GPIO_NUM_5, 1); // HIGH
gpio_set_level(GPIO_NUM_5, 0); // LOW
```

#### **Advanced Configuration**

```c
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_NUM_5),  // Pin mask
    .mode = GPIO_MODE_OUTPUT,               // Output mode
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE          // No interrupt
};
gpio_config(&io_conf);
```

#### **Touch Sensor (Capacitive)**

ESP32 memiliki **10 touch sensor pins** built-in:

```
Touch 0 → GPIO 4
Touch 1 → GPIO 0
Touch 2 → GPIO 2
Touch 3 → GPIO 15
Touch 4 → GPIO 13
Touch 5 → GPIO 12
Touch 6 → GPIO 14
Touch 7 → GPIO 27
Touch 8 → GPIO 33
Touch 9 → GPIO 32
```

**Example:**
```c
#include "driver/touch_pad.h"

// Initialize touch pad
touch_pad_init();
touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
touch_pad_config(TOUCH_PAD_NUM9, 0);  // GPIO 32

// Read touch value
uint16_t touch_value;
touch_pad_read(TOUCH_PAD_NUM9, &touch_value);

// Set threshold untuk trigger interrupt
touch_pad_set_thresh(TOUCH_PAD_NUM9, threshold_value);
```

---

### 4.3 GPIO Performance Comparison

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Max GPIO Speed** | 180 MHz (F7/H7) | ~10 MHz (software) |
| **Atomic Operations** | ✅ BSRR register | ⚠️ Limited |
| **Pull-up/down** | ✅ ~40kΩ internal | ✅ ~45kΩ internal |
| **Open-Drain** | ✅ Native support | ✅ Via config |
| **5V Tolerance** | ✅ Most pins (FT) | ❌ 3.3V only! |
| **Alternate Functions** | ✅ Up to 16 AF per pin | ✅ GPIO matrix (flexible) |
| **Input-Only Pins** | ❌ No | ✅ GPIO 34-39 |
| **Touch Sensor** | ❌ No | ✅ 10 pins built-in |
| **Analog Capable** | ✅ All pins | ✅ Selected pins |
| **Max Output Current** | 25 mA (per pin) | 40 mA (total per group) |

**⚠️ Penting - ESP32:**
- GPIO 34-39: **INPUT ONLY**, no internal pull-up/down
- GPIO 6-11: **TIDAK BOLEH DIGUNAKAN** (connected to flash)
- Total current: Max 40 mA untuk semua GPIO combined dalam satu group

---

## 5. KOMUNIKASI SERIAL

### 5.1 UART/USART

#### **STM32 UART**

**Features:**
- **UART**: Asynchronous only (TX/RX)
- **USART**: Synchronous + Asynchronous (TX/RX/CK)
- **Instances**: 2-8 (tergantung series)
- **Baud Rate**: Up to MCU_Clock/16 (dapat lebih dengan oversampling x8)
- **Data Bits**: 7, 8, 9 (with parity)
- **Stop Bits**: 0.5, 1, 1.5, 2
- **Parity**: None, Even, Odd
- **Flow Control**: RTS/CTS hardware

**HAL Configuration:**
```c
UART_HandleTypeDef huart1;

huart1.Instance = USART1;
huart1.Init.BaudRate = 115200;
huart1.Init.WordLength = UART_WORDLENGTH_8B;
huart1.Init.StopBits = UART_STOPBITS_1;
huart1.Init.Parity = UART_PARITY_NONE;
huart1.Init.Mode = UART_MODE_TX_RX;
huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
huart1.Init.OverSampling = UART_OVERSAMPLING_16;
HAL_UART_Init(&huart1);
```

**Transmission Modes:**

1. **Polling (Blocking):**
```c
uint8_t tx_data[] = "Hello STM32\r\n";
HAL_UART_Transmit(&huart1, tx_data, sizeof(tx_data), 1000); // Timeout 1000ms

uint8_t rx_data[10];
HAL_UART_Receive(&huart1, rx_data, 10, 5000); // Receive 10 bytes
```

2. **Interrupt (Non-blocking):**
```c
// Start receiving in interrupt mode
HAL_UART_Receive_IT(&huart1, rx_buffer, 10);

// Callback when reception complete
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // Process received data
        HAL_UART_Receive_IT(&huart1, rx_buffer, 10); // Re-enable
    }
}
```

3. **DMA (Best Performance):**
```c
// Configure DMA in CubeMX first
HAL_UART_Transmit_DMA(&huart1, tx_data, sizeof(tx_data));
HAL_UART_Receive_DMA(&huart1, rx_buffer, BUFFER_SIZE);

// Callback
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    // Transmission complete
}
```

---

#### **ESP32 UART**

**Features:**
- **Instances**: UART0, UART1, UART2
- **UART0**: Default untuk Serial Monitor (GPIO1/GPIO3)
- **Baud Rate**: Up to 5 Mbps
- **FIFO**: 128-byte TX/RX hardware FIFO
- **DMA**: Tidak ada hardware DMA (menggunakan interrupt)

**Basic Usage (Arduino-style):**
```cpp
void setup() {
    Serial.begin(115200);  // UART0
    Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // UART1 custom pins
    Serial2.begin(115200); // UART2
}

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        Serial.write(c); // Echo
    }
}
```

**ESP-IDF (Native) API:**
```c
#include "driver/uart.h"

// Configuration
uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

uart_param_config(UART_NUM_1, &uart_config);
uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

// Install driver (with RX buffer)
uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);

// Transmit
char* tx_data = "Hello ESP32";
uart_write_bytes(UART_NUM_1, tx_data, strlen(tx_data));

// Receive
uint8_t data[128];
int len = uart_read_bytes(UART_NUM_1, data, sizeof(data), 100 / portTICK_PERIOD_MS);
```

**Event-driven with Queue:**
```c
QueueHandle_t uart_queue;
uart_driver_install(UART_NUM_1, 1024*2, 1024*2, 10, &uart_queue, 0);

// Task to handle UART events
void uart_event_task(void *pvParameters) {
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(1024);
    
    while(1) {
        if(xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch(event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_NUM_1, dtmp, event.size, portMAX_DELAY);
                    // Process data
                    break;
                case UART_FIFO_OVF:
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart_queue);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_NUM_1);
                    xQueueReset(uart_queue);
                    break;
                default:
                    break;
            }
        }
    }
}
```

---

### 5.2 SPI (Serial Peripheral Interface)

#### **STM32 SPI**

**Features:**
- **Speed**: Up to MCU_Clock/2 (90 MHz pada F4@180MHz)
- **Modes**: Master / Slave
- **Data Size**: 4-16 bits per frame
- **Clock Polarity/Phase**: CPOL/CPHA configurable
- **NSS**: Hardware/Software managed
- **DMA**: Full support

**HAL Configuration:**
```c
SPI_HandleTypeDef hspi1;

hspi1.Instance = SPI1;
hspi1.Init.Mode = SPI_MODE_MASTER;
hspi1.Init.Direction = SPI_DIRECTION_2LINES;
hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
hspi1.Init.NSS = SPI_NSS_SOFT;
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Clock/16
hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
HAL_SPI_Init(&hspi1);
```

**Data Transfer:**
```c
uint8_t tx_data[10] = {0x01, 0x02, 0x03, ...};
uint8_t rx_data[10];

// Polling
HAL_SPI_Transmit(&hspi1, tx_data, 10, 1000);
HAL_SPI_Receive(&hspi1, rx_data, 10, 1000);
HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 10, 1000);

// Interrupt
HAL_SPI_Transmit_IT(&hspi1, tx_data, 10);

// DMA (fastest)
HAL_SPI_Transmit_DMA(&hspi1, tx_data, 10);
```

**Manual CS Control:**
```c
#define SPI_CS_PIN GPIO_PIN_4
#define SPI_CS_PORT GPIOA

// Select device
HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_RESET);
HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 10, 1000);
HAL_GPIO_WritePin(SPI_CS_PORT, SPI_CS_PIN, GPIO_PIN_SET); // Deselect
```

---

#### **ESP32 SPI**

**Features:**
- **Instances**: SPI (SPI1), HSPI (SPI2), VSPI (SPI3)
- **SPI1**: Reserved for flash (TIDAK BOLEH DIGUNAKAN)
- **Speed**: Up to 80 MHz
- **Modes**: Master / Slave
- **DMA**: Via linked-list descriptors
- **Quad SPI**: Supported (untuk flash)

**Arduino SPI:**
```cpp
#include <SPI.h>

#define CS_PIN 5

void setup() {
    SPI.begin();  // Default: VSPI (SCK=18, MISO=19, MOSI=23)
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
}

void loop() {
    digitalWrite(CS_PIN, LOW);
    uint8_t received = SPI.transfer(0x42); // Send & receive
    digitalWrite(CS_PIN, HIGH);
}
```

**Custom Pins:**
```cpp
// Use HSPI with custom pins
SPIClass hspi(HSPI);
hspi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
```

**ESP-IDF API:**
```c
spi_bus_config_t buscfg = {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4096
};

spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 10*1000*1000,  // 10 MHz
    .mode = 0,                        // CPOL=0, CPHA=0
    .spics_io_num = PIN_NUM_CS,
    .queue_size = 7
};

// Initialize bus
spi_bus_initialize(HSPI_HOST, &buscfg, 1); // DMA channel 1

// Add device
spi_device_handle_t spi;
spi_bus_add_device(HSPI_HOST, &devcfg, &spi);

// Transfer
spi_transaction_t t = {
    .length = 8,        // bits
    .tx_buffer = &data,
    .rx_buffer = &recv
};
spi_device_transmit(spi, &t);
```

---

### 5.3 I2C (Inter-Integrated Circuit)

#### **STM32 I2C**

**Features:**
- **Speed**: Standard (100 kHz), Fast (400 kHz), Fast+ (1 MHz)
- **Addressing**: 7-bit / 10-bit
- **DMA**: Supported
- **Hardware CRC**: Available (I2C_v2 pada STM32F0/F3/F7/L4/L5)

**HAL Configuration:**
```c
I2C_HandleTypeDef hi2c1;

hi2c1.Instance = I2C1;
hi2c1.Init.ClockSpeed = 100000;        // 100 kHz
hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
hi2c1.Init.OwnAddress1 = 0;
hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
HAL_I2C_Init(&hi2c1);
```

**Master Transmit/Receive:**
```c
#define DEVICE_ADDR 0x68  // 7-bit address (shifted left in HAL)

// Write to device
uint8_t data[] = {0x00, 0x01, 0x02};
HAL_I2C_Master_Transmit(&hi2c1, DEVICE_ADDR << 1, data, 3, 1000);

// Read from device
uint8_t buffer[10];
HAL_I2C_Master_Receive(&hi2c1, DEVICE_ADDR << 1, buffer, 10, 1000);

// Write register then read (common pattern)
uint8_t reg_addr = 0x0F;
HAL_I2C_Mem_Read(&hi2c1, DEVICE_ADDR << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, buffer, 1, 1000);
```

**I2C Scanner:**
```c
void I2C_Scan(void) {
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
            printf("Device found at 0x%02X\n", addr);
        }
    }
}
```

---

#### **ESP32 I2C**

**Features:**
- **Instances**: I2C0, I2C1 (2 independent controllers)
- **Speed**: Standard (100 kHz), Fast (400 kHz), Fast+ (1 MHz)
- **Master/Slave**: Both supported
- **Clock Stretching**: Supported

**Arduino Wire:**
```cpp
#include <Wire.h>

void setup() {
    Wire.begin();  // Master mode, default pins (SDA=21, SCL=22)
    // Custom pins:
    // Wire.begin(SDA_PIN, SCL_PIN);
}

void loop() {
    Wire.beginTransmission(0x68);
    Wire.write(0x00);  // Register address
    Wire.write(0x42);  // Data
    Wire.endTransmission();
    
    // Read
    Wire.requestFrom(0x68, 2);
    if (Wire.available() >= 2) {
        uint8_t data1 = Wire.read();
        uint8_t data2 = Wire.read();
    }
}
```

**ESP-IDF API:**
```c
#include "driver/i2c.h"

// Master configuration
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000  // 100 kHz
};

i2c_param_config(I2C_NUM_0, &conf);
i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

// Write to device
i2c_cmd_handle_t cmd = i2c_cmd_link_create();
i2c_master_start(cmd);
i2c_master_write_byte(cmd, (DEVICE_ADDR << 1) | I2C_MASTER_WRITE, true);
i2c_master_write_byte(cmd, reg_addr, true);
i2c_master_write_byte(cmd, data, true);
i2c_master_stop(cmd);
i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
i2c_cmd_link_delete(cmd);

// Read from device
cmd = i2c_cmd_link_create();
i2c_master_start(cmd);
i2c_master_write_byte(cmd, (DEVICE_ADDR << 1) | I2C_MASTER_WRITE, true);
i2c_master_write_byte(cmd, reg_addr, true);
i2c_master_start(cmd);  // Repeated start
i2c_master_write_byte(cmd, (DEVICE_ADDR << 1) | I2C_MASTER_READ, true);
i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
i2c_master_stop(cmd);
i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
i2c_cmd_link_delete(cmd);
```

---

### PERBANDINGAN KOMUNIKASI SERIAL

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **UART Instances** | 2-8 | 3 (UART0/1/2) |
| **UART Max Speed** | Clock/16 (MB/s) | 5 Mbps |
| **UART FIFO** | 1 byte (older), 32B (newer) | 128 bytes |
| **SPI Instances** | 1-6 | 3 (SPI1 reserved) |
| **SPI Max Speed** | 90 MHz (F4/F7) | 80 MHz |
| **SPI Slave** | ✅ Supported | ✅ Supported |
| **I2C Instances** | 1-4 | 2 |
| **I2C Max Speed** | 1 MHz (Fast+) | 1 MHz |
| **I2C Clock Stretch** | ✅ Yes | ✅ Yes |
| **DMA Support** | ✅ All peripherals | ⚠️ SPI only (limited) |
| **CAN Bus** | ✅ Native (bxCAN/FDCAN) | ⚠️ Via TWAI controller |
| **USB** | ✅ Native (OTG/Device) | ⚠️ ESP32-S2/S3 only |

---

## 6. TIMER DAN PWM

### 6.1 STM32 Timers

STM32 memiliki **hierarki timer** yang sangat powerful:

#### **Timer Types**

| Type | Features | Count Range | Channels | Use Cases |
|------|----------|-------------|----------|-----------|
| **Basic** | Up counting only | 16-bit | 0 | DAC trigger, timebase |
| **General Purpose** | Up/Down/Up-Down | 16-bit/32-bit | 4 | PWM, Input Capture, Encoder |
| **Advanced** | All GP features + | 16-bit | 4-6 | Motor control, complementary PWM |

**STM32F4 Timer Overview:**
```
TIM1, TIM8     →  Advanced-control (complementary PWM, dead-time)
TIM2-TIM5      →  General-purpose 32-bit
TIM6, TIM7     →  Basic timers
TIM9-TIM14     →  General-purpose 16-bit
```

#### **Timer Configuration**

**Key Concepts:**
- **Prescaler (PSC)**: Divides timer clock
- **Auto-Reload Register (ARR)**: Max count value
- **Counter (CNT)**: Current count
- **Capture/Compare Register (CCR)**: For PWM duty cycle

**Formula:**
```
Timer_Frequency = Timer_Clock / (Prescaler + 1)
Update_Frequency = Timer_Frequency / (ARR + 1)
PWM_Frequency = Timer_Frequency / (ARR + 1)
Duty_Cycle = (CCR / ARR) × 100%
```

**Example: 1 kHz Timer Interrupt**
```c
// Assuming Timer clock = 84 MHz (APB1 on F4)
// Desired frequency = 1 kHz = 1000 Hz

TIM_HandleTypeDef htim2;

htim2.Instance = TIM2;
htim2.Init.Prescaler = 8400 - 1;      // 84MHz / 8400 = 10kHz
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = 10 - 1;            // 10kHz / 10 = 1kHz
htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
HAL_TIM_Base_Init(&htim2);

// Start timer with interrupt
HAL_TIM_Base_Start_IT(&htim2);

// Callback
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        // Execute every 1 ms
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
```

#### **PWM Generation**

**Example: 20 kHz PWM with 50% duty cycle**
```c
TIM_HandleTypeDef htim3;
TIM_OC_InitTypeDef sConfigOC;

// Timer clock = 84 MHz
// PWM freq = 20 kHz → Period = 84MHz / 20kHz = 4200

htim3.Instance = TIM3;
htim3.Init.Prescaler = 0;
htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
htim3.Init.Period = 4200 - 1;
htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
HAL_TIM_PWM_Init(&htim3);

// Configure channel 1
sConfigOC.OCMode = TIM_OCMODE_PWM1;
sConfigOC.Pulse = 2100;  // 50% duty (ARR/2)
sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);

// Start PWM
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

// Change duty cycle dynamically
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 3150); // 75%
```

#### **Input Capture (Frequency Measurement)**

```c
TIM_IC_InitTypeDef sConfigIC;

htim3.Instance = TIM3;
htim3.Init.Prescaler = 84 - 1;  // 84MHz/84 = 1MHz (1µs resolution)
htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
htim3.Init.Period = 65535;      // Max for 16-bit timer
HAL_TIM_IC_Init(&htim3);

sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
sConfigIC.ICFilter = 0;
HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1);

HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);

// Callback
uint32_t capture_diff = 0;
uint32_t last_capture = 0;
float frequency = 0;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        uint32_t current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        capture_diff = current_capture - last_capture;
        last_capture = current_capture;
        
        // Calculate frequency (Timer = 1 MHz = 1µs tick)
        frequency = 1000000.0f / capture_diff;  // Hz
    }
}
```

#### **Encoder Mode (Quadrature Decoder)**

```c
TIM_Encoder_InitTypeDef sConfig;

htim3.Instance = TIM3;
htim3.Init.Prescaler = 0;
htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
htim3.Init.Period = 65535;
htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

sConfig.EncoderMode = TIM_ENCODERMODE_TI12;  // Count on both edges
sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
sConfig.IC1Filter = 0;
sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
sConfig.IC2Filter = 0;

HAL_TIM_Encoder_Init(&htim3, &sConfig);
HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

// Read encoder position
int32_t encoder_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
```

---

### 6.2 ESP32 Timers

#### **Hardware Timers**

ESP32 memiliki **4 hardware timers** (2 per group):

**Timer Groups:**
```
Timer Group 0:  TIMER0, TIMER1
Timer Group 1:  TIMER0, TIMER1
```

**Features:**
- 64-bit counters
- Up/Down counting
- Auto-reload
- Prescaler: 2-65536
- Max clock: 80 MHz (APB_CLK)

**Example: 1 kHz Timer Interrupt**
```c
#include "driver/timer.h"

#define TIMER_DIVIDER   80     // Hardware timer clock divider (80MHz/80 = 1MHz)
#define TIMER_SCALE     1000   // Convert to ms (1MHz/1000 = 1kHz)

timer_config_t config = {
    .divider = TIMER_DIVIDER,
    .counter_dir = TIMER_COUNT_UP,
    .counter_en = TIMER_PAUSE,
    .alarm_en = TIMER_ALARM_EN,
    .auto_reload = true
};

timer_init(TIMER_GROUP_0, TIMER_0, &config);
timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER_SCALE);
timer_enable_intr(TIMER_GROUP_0, TIMER_0);
timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
timer_start(TIMER_GROUP_0, TIMER_0);

// ISR
void IRAM_ATTR timer_isr(void *para) {
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
    
    // Your code here (runs every 1 ms)
    gpio_set_level(GPIO_NUM_2, !gpio_get_level(GPIO_NUM_2));
}
```

#### **LEDC (LED Controller) - PWM**

ESP32 memiliki **16 PWM channels** via LEDC peripheral:

**Channels:**
- High-speed: 8 channels (80 MHz clock)
- Low-speed: 8 channels (1 MHz APB clock, supports deep sleep)

**Features:**
- Resolution: 1-16 bits
- Frequency: Up to 40 MHz (tergantung resolution)
- Fade functions (hardware smooth transitions)

**Example: PWM LED Fading**
```c
#include "driver/ledc.h"

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_GPIO           (5)
#define LEDC_DUTY_RES       LEDC_TIMER_13_BIT  // 13-bit resolution
#define LEDC_DUTY           (4095)             // 50% duty (4096/2)
#define LEDC_FREQUENCY      (5000)             // 5 kHz

// Timer configuration
ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_MODE,
    .timer_num        = LEDC_TIMER,
    .duty_resolution  = LEDC_DUTY_RES,
    .freq_hz          = LEDC_FREQUENCY,
    .clk_cfg          = LEDC_AUTO_CLK
};
ledc_timer_config(&ledc_timer);

// Channel configuration
ledc_channel_config_t ledc_channel = {
    .speed_mode     = LEDC_MODE,
    .channel        = LEDC_CHANNEL,
    .timer_sel      = LEDC_TIMER,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = LEDC_GPIO,
    .duty           = 0,
    .hpoint         = 0
};
ledc_channel_config(&ledc_channel);

// Set duty cycle
ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 4095);
ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

// Hardware fade
ledc_fade_func_install(0);
ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 8191, 1000); // Fade to max in 1s
ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
```

**Servo Control:**
```cpp
#include <ESP32Servo.h>

Servo myservo;

void setup() {
    myservo.attach(18);  // Pin 18
}

void loop() {
    myservo.write(0);    // 0 degrees
    delay(1000);
    myservo.write(90);   // 90 degrees
    delay(1000);
    myservo.write(180);  // 180 degrees
    delay(1000);
}
```

---

### PERBANDINGAN TIMER/PWM

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Hardware Timers** | 2-17 (series dependent) | 4 (64-bit) |
| **Timer Resolution** | 16/32-bit | 64-bit |
| **Max Timer Clock** | System clock | 80 MHz (APB) |
| **PWM Channels** | 4-6 per timer | 16 (LEDC) |
| **PWM Resolution** | Timer dependent | 1-16 bits (configurable) |
| **Max PWM Frequency** | ~MHz range | 40 MHz |
| **Complementary PWM** | ✅ Advanced timers | ❌ No |
| **Dead-time Insertion** | ✅ Advanced timers | ❌ No |
| **Input Capture** | ✅ 4 channels/timer | ⚠️ Limited (pulse counter) |
| **Encoder Mode** | ✅ Hardware quadrature | ⚠️ Via PCNT peripheral |
| **Hardware Fade** | ❌ No | ✅ LEDC fade engine |
| **Motor Control** | ✅ Excellent (TIM1/8) | ⚠️ Good (via MCPWM) |

**Catatan:**
- **STM32**: Lebih cocok untuk motor control presisi (FOC, BLDC) dengan dead-time & complementary PWM
- **ESP32**: Lebih mudah untuk LED dimming, servo, basic PWM dengan hardware fade

---

## 7. ADC DAN DAC

### 7.1 STM32 ADC

#### **ADC Specifications**

| Series | Resolution | Channels | Speed | Type |
|--------|-----------|----------|-------|------|
| **F0/L0** | 12-bit | 16 | 1 MSPS | SAR |
| **F1** | 12-bit | 16 | 1 MSPS | SAR |
| **F3/L4** | 12-bit | 19 | 5 MSPS | SAR |
| **F4** | 12-bit | 16 | 2.4 MSPS | SAR |
| **F7** | 12-bit | 24 | 2.4 MSPS | SAR |
| **H7** | 16-bit | 20 | 3.6 MSPS | SAR |
| **G4** | 12-bit | 19 | 5 MSPS | SAR (differential) |

**Features:**
- **SAR** (Successive Approximation Register) architecture
- Multiple ADC instances (ADC1, ADC2, ADC3)
- Internal channels: Temperature sensor, VREFINT, VBAT
- Scan mode: Multi-channel sequential
- Continuous/Single conversion
- DMA support
- Trigger sources: Software, Timer, EXTI

#### **Single Channel Read (Polling)**

```c
ADC_HandleTypeDef hadc1;

hadc1.Instance = ADC1;
hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
hadc1.Init.Resolution = ADC_RESOLUTION_12B;
hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
hadc1.Init.ScanConvMode = DISABLE;
hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
hadc1.Init.ContinuousConvMode = DISABLE;
hadc1.Init.DiscontinuousConvMode = DISABLE;
hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
hadc1.Init.DMAContinuousRequests = DISABLE;
HAL_ADC_Init(&hadc1);

// Configure channel
ADC_ChannelConfTypeDef sConfig = {0};
sConfig.Channel = ADC_CHANNEL_0;  // PA0
sConfig.Rank = 1;
sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// Read value
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 100);
uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
HAL_ADC_Stop(&hadc1);

// Convert to voltage (assuming 3.3V reference)
float voltage = (adc_value * 3.3f) / 4095.0f;
```

#### **Multi-Channel Scan with DMA**

```c
#define ADC_CHANNELS 3
uint16_t adc_values[ADC_CHANNELS];

// Configure ADC for scan mode
hadc1.Init.ScanConvMode = ENABLE;
hadc1.Init.ContinuousConvMode = ENABLE;
hadc1.Init.DMAContinuousRequests = ENABLE;
HAL_ADC_Init(&hadc1);

// Configure channels
ADC_ChannelConfTypeDef sConfig = {0};

sConfig.Channel = ADC_CHANNEL_0;
sConfig.Rank = 1;
sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

sConfig.Channel = ADC_CHANNEL_1;
sConfig.Rank = 2;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

sConfig.Channel = ADC_CHANNEL_4;
sConfig.Rank = 3;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// Start DMA
HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_values, ADC_CHANNELS);

// Values automatically updated in adc_values[] array
// adc_values[0] = Channel 0
// adc_values[1] = Channel 1
// adc_values[2] = Channel 4
```

#### **Timer-Triggered ADC with DMA**

```c
// Configure timer to trigger ADC at 1 kHz
// (TIM2 configuration omitted - see Timer section)

hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
HAL_ADC_Init(&hadc1);

// Start ADC DMA
HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, BUFFER_SIZE);

// Start timer
HAL_TIM_Base_Start(&htim2);

// ADC conversions now triggered automatically by timer
```

#### **Internal Temperature Sensor**

```c
// Enable temperature sensor channel
sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
sConfig.Rank = 1;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 100);
uint32_t temp_raw = HAL_ADC_GetValue(&hadc1);

// Convert to temperature (formula from datasheet)
#define V25 0.76f       // Voltage at 25°C (typically 0.76V)
#define AVG_SLOPE 0.0025f  // Slope (2.5 mV/°C)

float voltage = (temp_raw * 3.3f) / 4095.0f;
float temperature = ((voltage - V25) / AVG_SLOPE) + 25.0f;
```

---

### 7.2 STM32 DAC

#### **DAC Specifications**

**Available on:** F0, F1 (high-density), F3, F4, F7, L0, L1, L4, G4, H7

| Feature | Spec |
|---------|------|
| **Resolution** | 12-bit |
| **Channels** | 1 or 2 |
| **Output Range** | 0 - VREF+ (typically 3.3V) |
| **Conversion Time** | 1 µs settling |
| **Trigger** | Software, Timer, External |
| **DMA** | Supported |
| **Waveform Gen** | Triangle, Noise via DMA |

#### **Basic DAC Output**

```c
DAC_HandleTypeDef hdac;

hdac.Instance = DAC;
HAL_DAC_Init(&hdac);

DAC_ChannelConfTypeDef sConfig = {0};
sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

// Start DAC
HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

// Set output value (0-4095 for 12-bit)
HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048); // ~1.65V

// Output voltage = (value / 4095) * VREF
```

#### **Sine Wave Generation (Timer + DMA)**

```c
#define SINE_SAMPLES 32
uint16_t sine_wave[SINE_SAMPLES];

// Generate sine lookup table
for (int i = 0; i < SINE_SAMPLES; i++) {
    sine_wave[i] = (uint16_t)(2047.5f * (1.0f + sinf(2.0f * M_PI * i / SINE_SAMPLES)));
}

// Configure DAC with timer trigger
sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);

// Start DAC with DMA
HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)sine_wave, SINE_SAMPLES, DAC_ALIGN_12B_R);

// Start timer (frequency determines sine wave frequency)
HAL_TIM_Base_Start(&htim2);
```

---

### 7.3 ESP32 ADC

#### **ADC Specifications**

| Feature | ESP32 | ESP32-S2/S3 | ESP32-C3 |
|---------|-------|-------------|----------|
| **Resolution** | 12-bit | 13-bit | 12-bit |
| **Channels** | 18 (ADC1: 8, ADC2: 10) | 20 | 6 |
| **Voltage Range** | 0-3.3V (with atten) | 0-3.3V | 0-3.3V |
| **Linearity** | ⚠️ Non-linear | ✅ Improved | ✅ Good |

**Important:**
- **ADC2** tidak bisa digunakan saat WiFi aktif!
- ADC ESP32 memiliki non-linearity issue → butuh kalibrasi

**Attenuation Settings:**
```c
ADC_ATTEN_DB_0   →  0-800 mV
ADC_ATTEN_DB_2_5 →  0-1100 mV
ADC_ATTEN_DB_6   →  0-1350 mV
ADC_ATTEN_DB_11  →  0-2600 mV (approx 3.3V after calibration)
```

#### **Basic ADC Read (Arduino)**

```cpp
#define ANALOG_PIN 34  // ADC1_CH6 (GPIO34)

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);  // 12-bit resolution (0-4095)
    analogSetAttenuation(ADC_11db);  // Full range 0-3.3V
}

void loop() {
    int adc_value = analogRead(ANALOG_PIN);
    
    // Convert to voltage (rough - needs calibration)
    float voltage = (adc_value / 4095.0) * 3.3;
    
    Serial.printf("ADC: %d, Voltage: %.2f V\n", adc_value, voltage);
    delay(500);
}
```

#### **ESP-IDF with Calibration**

```c
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define ADC_CHANNEL ADC1_CHANNEL_6  // GPIO34

esp_adc_cal_characteristics_t *adc_chars;

void setup_adc() {
    // Configure ADC1
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11);
    
    // Characterize ADC (calibration)
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);
}

void read_adc() {
    uint32_t adc_reading = 0;
    
    // Multisampling untuk akurasi
    for (int i = 0; i < 64; i++) {
        adc_reading += adc1_get_raw(ADC_CHANNEL);
    }
    adc_reading /= 64;
    
    // Convert to voltage dengan kalibrasi
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    
    printf("Raw: %d, Voltage: %d mV\n", adc_reading, voltage);
}
```

#### **Continuous Reading (DMA mode - ESP-IDF)**

```c
#include "driver/adc.h"
#include "driver/i2s.h"

#define SAMPLE_RATE 10000  // 10 kHz
#define DMA_BUF_COUNT 2
#define DMA_BUF_LEN 1024

void setup_adc_dma() {
    // Configure I2S for ADC DMA mode
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
    i2s_adc_enable(I2S_NUM_0);
}

void read_adc_dma() {
    size_t bytes_read;
    uint16_t adc_data[DMA_BUF_LEN];
    
    i2s_read(I2S_NUM_0, adc_data, sizeof(adc_data), &bytes_read, portMAX_DELAY);
    
    // Process samples in adc_data[]
}
```

---

### 7.4 ESP32 DAC

#### **DAC Specifications**

**Available on:** ESP32, ESP32-S2 only (NOT on ESP32-C3/C6/S3)

| Feature | Spec |
|---------|------|
| **Resolution** | 8-bit |
| **Channels** | 2 (GPIO25, GPIO26) |
| **Output Range** | 0-3.3V |
| **Voltage Steps** | 3.3V / 256 = ~12.9 mV |

#### **Basic DAC Output**

```c
#include "driver/dac.h"

// Enable DAC
dac_output_enable(DAC_CHANNEL_1);  // GPIO25

// Set output (0-255)
dac_output_voltage(DAC_CHANNEL_1, 128);  // ~1.65V

// Output voltage = (value / 255) * 3.3V
```

#### **Cosine Wave (Built-in Generator)**

```c
// ESP32 has built-in cosine wave generator!
dac_cw_config_t cw_config = {
    .en_ch = DAC_CHANNEL_1,
    .scale = DAC_CW_SCALE_2,     // Amplitude scale (1, 2, 4, 8)
    .phase = DAC_CW_PHASE_0,
    .freq = 1000,                 // Frequency in Hz
    .offset = 0
};

dac_cw_generator_config(&cw_config);
dac_cw_generator_enable();
```

---

### PERBANDINGAN ADC/DAC

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **ADC Resolution** | 12-bit (16-bit H7) | 12-bit (13-bit S2/S3) |
| **ADC Channels** | 16-24 | 18 (ADC2 unusable with WiFi) |
| **ADC Speed** | Up to 5 MSPS | ~100 kSPS (continuous) |
| **ADC Linearity** | ✅ Excellent | ⚠️ Non-linear (needs calibration) |
| **ADC DMA** | ✅ Native support | ✅ Via I2S peripheral |
| **Internal Sensors** | ✅ Temp, VREF, VBAT | ✅ Hall sensor only |
| **DAC Resolution** | 12-bit | 8-bit |
| **DAC Channels** | 1-2 | 2 (not on all variants) |
| **DAC Waveform Gen** | ✅ Triangle, Noise | ✅ Cosine (hardware) |
| **Differential ADC** | ✅ G4 series | ❌ No |
| **Trigger Sources** | ✅ Timer, EXTI, Software | ⚠️ Software only |

**Kesimpulan:**
- **STM32**: Lebih akurat, lebih cepat, lebih banyak channel → ideal untuk precision analog
- **ESP32**: Cukup untuk sensor reading dasar, tapi butuh kalibrasi

---

## 8. INTERRUPT DAN NVIC

### 8.1 STM32 NVIC (Nested Vectored Interrupt Controller)

#### **NVIC Architecture**

NVIC adalah bagian dari ARM Cortex-M core yang mengelola interrupt:

**Features:**
- Nested interrupt support (interrupt bisa di-interrupt)
- Priority levels: 4-256 tergantung implementasi
- Priority grouping (preemption vs sub-priority)
- Deterministic latency (12 cycles worst-case pada M4)
- Automatic context saving/restoring

**Priority Levels per Core:**

| Core | Priority Bits | Levels | Example MCUs |
|------|---------------|--------|--------------|
| M0/M0+ | 2 bits | 4 | STM32F0, L0, G0 |
| M3 | 4 bits | 16 | STM32F1, L1 |
| M4/M7 | 4 bits | 16 | STM32F4, F7, L4, H7 |

**Priority Grouping:**

STM32 membagi priority menjadi **Preemption Priority** dan **Sub Priority**:

```
NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
```

| Group | Preemption Bits | Sub-Priority Bits | Description |
|-------|-----------------|-------------------|-------------|
| 0 | 0 | 4 | 16 sub-priorities, no preemption |
| 1 | 1 | 3 | 2 preempt + 8 sub |
| 2 | 2 | 2 | 4 preempt + 4 sub |
| 3 | 3 | 1 | 8 preempt + 2 sub |
| 4 | 4 | 0 | 16 preempt levels, no sub |

**Rule:**
- Lower number = **Higher priority**
- Preemption priority can interrupt lower preemption
- Sub-priority hanya menentukan urutan jika simultaneous

#### **External Interrupts (EXTI)**

STM32 memiliki **16 EXTI lines** (EXTI0-EXTI15):

**Mapping:**
```
EXTI0  → PA0, PB0, PC0, PD0, ... (salah satu)
EXTI1  → PA1, PB1, PC1, PD1, ...
...
EXTI15 → PA15, PB15, PC15, PD15, ...
```

**⚠️ Limitation:** Hanya bisa gunakan 1 pin per line number!
- ✅ PA0 + PB1 → OK (different lines)
- ❌ PA0 + PB0 → NOT possible (same EXTI0)

**HAL Configuration:**
```c
// Configure PA0 as interrupt (rising edge)
GPIO_InitTypeDef GPIO_InitStruct = {0};

GPIO_InitStruct.Pin = GPIO_PIN_0;
GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;  // or IT_FALLING, IT_RISING_FALLING
GPIO_InitStruct.Pull = GPIO_PULLDOWN;
HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

// Set priority
HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);  // Preempt=2, Sub=0

// Enable interrupt
HAL_NVIC_EnableIRQ(EXTI0_IRQn);
```

**Interrupt Handler:**
```c
// In stm32xxx_it.c
void EXTI0_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

// Callback in main.c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // Handle interrupt
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
```

#### **EXTI Line Mapping (F4 example)**

| IRQ Handler | EXTI Lines | Note |
|-------------|------------|------|
| EXTI0_IRQHandler | EXTI0 | Dedicated |
| EXTI1_IRQHandler | EXTI1 | Dedicated |
| EXTI2_IRQHandler | EXTI2 | Dedicated |
| EXTI3_IRQHandler | EXTI3 | Dedicated |
| EXTI4_IRQHandler | EXTI4 | Dedicated |
| EXTI9_5_IRQHandler | EXTI5-9 | **Shared!** |
| EXTI15_10_IRQHandler | EXTI10-15 | **Shared!** |

**Handling Shared EXTI:**
```c
void EXTI9_5_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);
        // Handle PIN5
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_6);
        // Handle PIN6
    }
}
```

#### **Critical Sections**

```c
// Disable all interrupts
__disable_irq();
// Critical code
__enable_irq();

// Or use PRIMASK (HAL)
uint32_t primask = __get_PRIMASK();
__disable_irq();
// Critical section
__set_PRIMASK(primask);

// Disable specific interrupt
HAL_NVIC_DisableIRQ(EXTI0_IRQn);
// Code
HAL_NVIC_EnableIRQ(EXTI0_IRQn);
```

---

### 8.2 ESP32 Interrupt System

#### **Interrupt Architecture**

ESP32 menggunakan **Xtensa interrupt system**:

**Features:**
- 32 interrupt sources per core
- 7 priority levels (0-6, higher = more priority)
- Non-maskable interrupt (NMI) - level 7
- Dual-core interrupt routing

**Interrupt Allocation:**

ESP-IDF menggunakan **interrupt allocator** untuk manage interrupts:

```c
#include "esp_intr_alloc.h"

// Flags
ESP_INTR_FLAG_LEVEL1    // Level 1 (lowest)
ESP_INTR_FLAG_LEVEL2    // Level 2
ESP_INTR_FLAG_LEVEL3    // Level 3 (highest normal)
ESP_INTR_FLAG_IRAM      // ISR in IRAM (faster, mandatory for some)
ESP_INTR_FLAG_SHARED    // Allow sharing with other handlers
```

#### **GPIO Interrupts**

**Example:**
```c
#include "driver/gpio.h"

#define GPIO_INPUT_PIN 4

void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    // Handle interrupt (keep short!)
    // Cannot use printf() here (unless in IRAM)
}

void setup_gpio_interrupt() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_INPUT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE  // Rising edge
    };
    gpio_config(&io_conf);
    
    // Install ISR service
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    
    // Add handler for specific GPIO
    gpio_isr_handler_add(GPIO_INPUT_PIN, gpio_isr_handler, (void*) GPIO_INPUT_PIN);
}
```

**Interrupt Types:**
```c
GPIO_INTR_DISABLE       // Disable
GPIO_INTR_POSEDGE       // Rising edge
GPIO_INTR_NEGEDGE       // Falling edge
GPIO_INTR_ANYEDGE       // Both edges
GPIO_INTR_LOW_LEVEL     // Low level
GPIO_INTR_HIGH_LEVEL    // High level
```

#### **Deferred Interrupt Handling (Task Notification)**

**Best Practice:** ISR singkat, processing di task:

```c
TaskHandle_t task_handle = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg) {
    // Notify task from ISR
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void gpio_task(void* arg) {
    while(1) {
        // Wait for notification from ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Process interrupt (can use printf, delays, etc.)
        printf("GPIO interrupt handled in task\n");
    }
}

void app_main() {
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, &task_handle);
    setup_gpio_interrupt();
}
```

#### **Critical Sections (FreeRTOS)**

```c
// Disable interrupts on single core
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void critical_function() {
    portENTER_CRITICAL(&mux);
    // Critical code (max 20 CPU cycles recommended)
    portEXIT_CRITICAL(&mux);
}

// From ISR
void IRAM_ATTR some_isr() {
    portENTER_CRITICAL_ISR(&mux);
    // Critical code
    portEXIT_CRITICAL_ISR(&mux);
}

// Disable task switching (not interrupts)
vTaskSuspendAll();
// Code
xTaskResumeAll();
```

---

### PERBANDINGAN INTERRUPT

| Feature | STM32 (NVIC) | ESP32 |
|---------|--------------|-------|
| **Priority Levels** | 4-16 | 7 (0-6) |
| **Nested Interrupts** | ✅ Full support | ✅ Supported |
| **Preemption** | ✅ Configurable | ✅ By priority |
| **Latency** | ⚡ 12 cycles (M4) | ⚡ <100 ns typical |
| **External Interrupts** | 16 lines (shared pins) | All GPIO (independent) |
| **Interrupt Routing** | Fixed per peripheral | ✅ Flexible (any GPIO) |
| **ISR Location** | Flash/RAM | ⚠️ Must be IRAM for critical |
| **Dual-Core** | ❌ No (except H7) | ✅ Per-core allocation |
| **FreeRTOS Integration** | ✅ CMSIS-RTOS | ✅ Native FreeRTOS |

**Key Differences:**
- **STM32**: EXTI line limitation (1 pin per number), tapi deterministic dan presisi
- **ESP32**: Setiap GPIO bisa interrupt independent, tapi ISR harus di IRAM untuk performance

---

*Karena panjangnya dokumen, saya akan melanjutkan bagian-bagian berikutnya. Apakah Anda ingin saya lanjutkan dengan topik:*
- **DMA**
- **Clock System**
- **Power Management**
- **Wireless Connectivity**
- **RTOS**
- **Storage & File System**
- **Security**
- **Development Tools**
- **Perbandingan Final**

*Atau apakah ada bagian spesifik yang ingin Anda fokuskan?*
# KONTEN TAMBAHAN: BAB 4-14

Ini adalah konten lengkap untuk Bab 4 sampai Bab 14 yang akan digabungkan dengan file utama.

---

## BAB 4: UART/SERIAL COMMUNICATION

> **Tujuan Pembelajaran:**
> - Komunikasi serial asynchronous
> - Polling vs Interrupt vs DMA mode
> - Printf redirection untuk debugging
> - Binary data protocol
> - Error detection dan recovery

### 📖 PENJELASAN MATERI

**UART (Universal Asynchronous Receiver/Transmitter):**
Protokol serial komunikasi yang paling simple dan widely used untuk:
- Debugging (printf ke komputer)
- Komunikasi antar MCU
- GPS, Bluetooth modules
- Sensor serial (CO2, PM2.5, etc)

**Frame Format:**
```
START BIT (0) → DATA BITS (5-9) → PARITY (optional) → STOP BIT (1)
     ↓              ↓                   ↓                  ↓
   1 bit       LSB first         Error check          1-2 bits
```

**Baud Rate Common:**
- 9600:   Standard (reliable)
- 115200: High-speed debug
- 460800: Fast data transfer
- 921600: Very fast (check cable quality!)

**TX/RX Connection:**
```
Device A          Device B
  TX ─────────────→ RX
  RX ←───────────── TX
  GND ────────────── GND
```
⚠️ **CROSSOVER** wiring! TX to RX, bukan TX to TX!

---

### 🔗 COMMON GROUND: Fitur UART yang Sama

#### 1. **Basic Serial Print**
**Deskripsi:** Printf untuk debugging

**Contoh Program STM32:**
- **"UART Printf Retarget"** - Redirect printf ke UART1 (115200 baud)
- **"Float Printf"** - Enable float support di printf (tambahkan -u _printf_float di linker)
- **"Multi-UART Print"** - Print ke multiple UART sekaligus

**Contoh Program ESP32:**
- **"Serial Monitor Print"** - Printf built-in ESP32 (UART0)
- **"Dual UART Debug"** - UART0 untuk console, UART2 untuk data
- **"Colored Log Output"** - ESP_LOGI dengan warna (ESP-IDF)

#### 2. **Receiving Data (Polling)**
**Deskripsi:** Baca data blocking

**Contoh Program STM32:**
- **"UART Echo"** - Echo karakter kembali
- **"Command Parser"** - Parse command line ("LED ON", "LED OFF")
- **"Timeout Handling"** - Handle UART timeout gracefully

**Contoh Program ESP32:**
- **"UART Driver Read"** - uart_read_bytes() dengan timeout
- **"Ring Buffer Reception"** - Continuous receive dengan buffer
- **"AT Command Handler"** - Parse AT commands seperti modem

#### 3. **Interrupt-Driven RX**
**Deskripsi:** Non-blocking receive dengan callback

**Contoh Program STM32:**
- **"HAL_UART_Receive_IT"** - RX interrupt mode
- **"Circular Buffer ISR"** - ISR isi buffer, main loop parse
- **"Line-Based Reception"** - Detect '\\n' untuk complete command

**Contoh Program ESP32:**
- **"UART Event Queue"** - event-driven dengan FreeRTOS queue
- **"Pattern Detection"** - Hardware detect '\\n' atau '\\r'
- **"Error Event Handling"** - Handle FIFO overflow, parity error

#### 4. **Binary Protocol**
**Deskripsi:** Kirim struct/data binary (bukan text)

**Contoh Program STM32:**
- **"Struct Transfer"** - Kirim typedef struct via UART
- **"CRC16 Checksum"** - Binary protocol dengan error detection
- **"Packet Framing"** - Header (0xAA), Length, Data, CRC

**Contoh Program ESP32:**
- **"JSON over UART"** - Serialize JSON, kirim via UART
- **"Protobuf Serial"** - Google Protocol Buffers untuk efficiency
- **"Binary Sensor Data"** - Pack multiple sensors dalam 1 frame

#### 5. **Multi-UART Communication**
**Deskripsi:** Gunakan multiple UART ports

**Contoh Program STM32:**
- **"UART Router"** - Forward data UART1 ↔ UART2
- **"Multi-Slave Polling"** - Master poll 3 slaves via different UARTs
- **"Debug + Data Separation"** - UART1 debug, UART2 production data

**Contoh Program ESP32:**
- **"Triple UART Setup"** - UART0 (debug), UART1 (internal), UART2 (external)
- **"GPS + Debug UART"** - GPS di UART2, debug di UART0
- **"Modbus RTU Master"** - Modbus via UART dengan RS485 transceiver

---

### ⭐ STM32 UART: Keunggulan Unik

#### 1. **DMA Mode (Zero CPU Overhead)**
**Deskripsi:** Transfer tanpa CPU load

**Contoh Program:**
- **"DMA TX Large Buffer"** - Kirim 10KB data tanpa CPU blocking
- **"DMA RX Circular"** - Continuous RX, process saat idle
- **"Double Buffer DMA"** - Ping-pong buffer untuk real-time data

#### 2. **Hardware Flow Control (RTS/CTS)**
**Deskripsi:** Auto flow control untuk prevent data loss

**Contoh Program:**
- **"RTS/CTS Configuration"** - Enable hardware flow control
- **"High-Speed Reliable Transfer"** - 921600 baud dengan flow control
- **"Buffer Management"** - Monitor CTS untuk detect receiver busy

#### 3. **USART Synchronous Mode**
**Deskripsi:** Clock signal untuk precise timing

**Contoh Program:**
- **"USART + CLK Pin"** - Synchronous communication
- **"SPI-like USART"** - Use USART sebagai pseudo-SPI
- **"LCD 8-bit Sync"** - Drive parallel LCD via USART

#### 4. **LIN Mode (Automotive Bus)**
**Deskripsi:** Local Interconnect Network protocol

**Contoh Program:**
- **"LIN Master Send"** - LIN frame transmission
- **"LIN Slave Response"** - Listen dan respond ke master
- **"Automotive Sensor Bus"** - Multiple sensors on LIN

#### 5. **SmartCard Mode (ISO 7816)**
**Deskripsi:** Smart card reader interface

**Contoh Program:**
- **"SmartCard ATR Read"** - Read Answer To Reset
- **"EMV Card Reader"** - Payment card communication
- **"SIM Card Interface"** - GSM SIM card reader

#### 6. **Multi-Processor Mode**
**Deskripsi:** Addressed multi-drop network

**Contoh Program:**
- **"Multi-Drop Master"** - 1 master, 8 slaves dengan address
- **"Slave Address Detection"** - Mute mode, wake on own address
- **"RS485 Multi-Point"** - RS485 bus dengan addressing

---

### ⚡ ESP32 UART: Keunggulan Unik

#### 1. **Flexible Pin Mapping**
**Deskripsi:** UART ke pin manapun!

**Contoh Program:**
- **"Custom UART Pins"** - TX=GPIO25, RX=GPIO26 (bukan default)
- **"Conflict Resolution"** - Move UART jika pin bentrok
- **"Triple UART Custom Pins"** - Semua 3 UART di pins pilihan

#### 2. **Large Hardware FIFO (128 bytes)**
**Deskripsi:** Buffer hardware besar untuk reduce interrupt

**Contoh Program:**
- **"FIFO Threshold Interrupt"** - Interrupt saat FIFO 80% full
- **"Batch Processing"** - Read 100 bytes sekaligus
- **"Low Interrupt Frequency"** - Efficient CPU usage

#### 3. **Pattern Detection (Hardware)**
**Deskripsi:** Detect sequence di data stream

**Contoh Program:**
- **"Newline Detection"** - Hardware detect '\\n' untuk line parsing
- **"AT Command Parser"** - Detect "\\r\\n" untuk AT response
- **"Frame Delimiter"** - Detect 0xAA 0x55 header

#### 4. **RS485 Half-Duplex Mode**
**Deskripsi:** Built-in RS485 support

**Contoh Program:**
- **"RS485 Auto Direction"** - RTS pin auto-control transceiver
- **"Modbus RTU"** - Modbus over RS485
- **"Industrial Sensor Bus"** - RS485 multi-drop network

#### 5. **Inverse Signal Support**
**Deskripsi:** Invert TX/RX logic level

**Contoh Program:**
- **"Inverted UART"** - Interface inverted RS232
- **"IR Remote TX"** - Infrared transmission via inverted UART
- **"Custom Protocol"** - Inverted signal untuk compatibility

#### 6. **Wake from Light Sleep**
**Deskripsi:** UART activity wake MCU

**Contoh Program:**
- **"Sleep on Idle"** - Sleep saat tidak ada data, wake on RX
- **"Remote Wakeup"** - Kirim karakter untuk bangunkan ESP32
- **"Low-Power UART Logger"** - Sleep between log entries

---

### 📊 UART COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Instances** | 2-8 (series dependent) | 3 (UART0/1/2) |
| **Max Baud Rate** | Up to fPCLK/16 | 5 Mbps |
| **FIFO Size** | 1-32 bytes (series) | 128 bytes RX+TX |
| **DMA Support** | ✅ Full hardware DMA | ❌ No DMA |
| **HW Flow Control** | ✅ RTS/CTS | ✅ RTS/CTS |
| **Pin Flexibility** | ⚠️ Fixed AF | ✅ Any GPIO |
| **Pattern Detection** | ❌ No | ✅ Hardware |
| **RS485 Mode** | ⚠️ Manual RTS | ✅ Auto built-in |
| **Synchronous Mode** | ✅ USART CLK | ❌ No |
| **LIN/SmartCard** | ✅ Supported | ❌ No |
| **Signal Inversion** | ❌ Need hardware | ✅ Software |
| **Sleep Wakeup** | ✅ Via EXTI | ✅ Direct UART |

---

### 💼 MINI PROJECT: Wireless Sensor Network

**Deskripsi:** Master-slave sensor network

**STM32 Master:**
- Program "Multi-Slave Poller" - Poll 4 slaves setiap 5 detik
- Program "Data Aggregator" - Collect + average sensor data
- Program "SD Card Logger" - Log data ke SD card

**ESP32 Slave:**
- Program "Sensor Node" - Respond to master requests
- Program "Deep Sleep Between Polls" - Sleep untuk save power
- Program "Multi-Sensor Read" - Temperature, humidity, light

**Protocol:**
```
Master → Slave: [0xAA][SlaveID][CMD]
Slave → Master: [0xAA][SlaveID][Temp_H][Temp_L][Humid_H][Humid_L][CRC]
```

---

## BAB 5: ADC - ANALOG INPUT & SENSOR READING

> **Tujuan Pembelajaran:**
> - Membaca sensor analog (potensiometer, temperature, light)
> - ADC resolution & sampling time
> - Calibration & noise filtering
> - Multi-channel scanning
> - DMA continuous mode

### 📖 PENJELASAN MATERI

**ADC (Analog-to-Digital Converter):**
Mengubah voltage analog (0-3.3V) menjadi nilai digital:

```
12-bit ADC: 0-4095 steps
  3.3V → 4095
  1.65V → 2048
  0V   → 0
  
Resolution: 3.3V / 4095 = 0.8 mV per step
```

**Sampling Time:**
- Longer = More accurate (high impedance sources)
- Shorter = Faster conversion (but less accurate)

**Typical Sensors:**
- Potensiometer: 0-3.3V linear
- LDR (Light): Voltage divider dengan resistor
- LM35 Temperature: 10mV/°C
- TMP36: 10mV/°C (500mV offset di 0°C)

---

### 🔗 COMMON GROUND: Fitur ADC yang Sama

#### 1. **Single Channel Read (Potentiometer)**
**Deskripsi:** Baca potensiometer

**Contoh Program STM32:**
- **"HAL_ADC_Start + Poll"** - Baca ADC blocking
- **"Voltage Conversion"** - Convert ADC value ke voltage
- **"Percentage Mapping"** - Map 0-4095 ke 0-100%

**Contoh Program ESP32:**
- **"analogRead() Simple"** - Arduino style ADC read
- **"adc1_get_raw()"** - ESP-IDF native read
- **"Resolution Configuration"** - Set 9/10/11/12-bit

#### 2. **LDR (Light Sensor)**
**Deskripsi:** Detect light intensity

**Contoh Program STM32:**
- **"Light-Activated Switch"** - LED ON saat gelap
- **"Ambient Light Meter"** - Display lux value
- **"Auto-Brightness Control"** - Adjust LED brightness

**Contoh Program ESP32:**
- **"Smart Light"** - WiFi-enabled light automation
- **"Day/Night Detector"** - Schedule berdasarkan light
- **"Threshold Notification"** - MQTT alert saat gelap

#### 3. **Temperature Sensor (LM35)**
**Deskripsi:** Analog temperature measurement

**Contoh Program STM32:**
- **"LM35 Reader"** - °C calculation
- **"Temperature Logger"** - Log every 1 minute
- **"Over-Temperature Alarm"** - Buzzer saat > 40°C

**Contoh Program ESP32:**
- **"IoT Temperature Monitor"** - Upload ke cloud
- **"Temperature + WiFi Dashboard"** - Web interface
- **"Email Alert"** - Kirim email saat overheat

#### 4. **Multi-Channel Scanning**
**Deskripsi:** Read multiple sensors

**Contoh Program STM32:**
- **"ADC Scan Mode"** - 4 channels sequential
- **"Multi-Sensor Dashboard"** - LCD display all sensors
- **"Sensor Array"** - 8-channel data acquisition

**Contoh Program ESP32:**
- **"Multi-ADC Sequential"** - Read ADC1_CH4-7
- **"Sensor Fusion"** - Combine multiple analog inputs
- **"IoT Multi-Sensor Node"** - All data to cloud

#### 5. **Averaging Filter**
**Deskripsi:** Noise reduction

**Contoh Program STM32:**
- **"64-Sample Average"** - Stable readings
- **"Moving Average Filter"** - 10-sample buffer
- **"Median Filter"** - Reject outliers

**Contoh Program ESP32:**
- **"Multisampling + Calibration"** - esp_adc_cal
- **"Kalman Filter"** - Advanced filtering
- **"Exponential Moving Average"** - Smooth curve

---

### ⭐ STM32 ADC: Keunggulan Unik

#### 1. **DMA Continuous Mode**
**Deskripsi:** Zero-CPU high-speed sampling

**Contoh Program:**
- **"DMA Circular Buffer"** - 1000 samples continuous
- **"Audio Sampling"** - 44.1 kHz audio input
- **"Signal Analyzer"** - FFT analysis dari DMA data

#### 2. **Timer-Triggered ADC**
**Deskripsi:** Precise sampling timing

**Contoh Program:**
- **"1 kHz Sampling"** - TIM2 trigger ADC setiap 1 ms
- **"Data Acquisition System"** - Multi-channel + timer
- **"Oscilloscope Mode"** - Capture waveform

#### 3. **Internal Temperature Sensor**
**Deskripsi:** MCU die temperature

**Contoh Program:**
- **"Chip Temperature Monitor"** - Read internal sensor
- **"Thermal Management"** - Throttle saat overheat
- **"Temperature Compensation"** - Calibrate dengan temp

#### 4. **VREFINT Calibration**
**Deskripsi:** Measure actual VDD

**Contoh Program:**
- **"VDD Measurement"** - Calculate true VDD voltage
- **"Self-Calibration"** - Accurate ADC tanpa external ref
- **"Battery Monitor"** - Measure battery via VDD

#### 5. **Analog Watchdog**
**Deskripsi:** Hardware threshold monitoring

**Contoh Program:**
- **"Over-Voltage Detect"** - Interrupt saat > threshold
- **"Window Comparator"** - Alert jika outside range
- **"Safety Monitor"** - Critical parameter monitoring

#### 6. **16-bit ADC (STM32H7)**
**Deskripsi:** Ultra-high resolution

**Contoh Program:**
- **"Precision Measurement"** - 0.05 mV per step
- **"Strain Gauge"** - High-accuracy load cell
- **"High-End Instrumentation"** - Lab equipment

---

### ⚡ ESP32 ADC: Keunggulan Unik

#### 1. **Built-in Calibration (eFuse)**
**Deskripsi:** Factory-calibrated accuracy

**Contoh Program:**
- **"ESP ADC Calibration"** - esp_adc_cal_characterize()
- **"Two-Point Calibration"** - Use eFuse calibration data
- **"Linearization"** - Compensate non-linearity

#### 2. **Attenuation Control**
**Deskripsi:** Adjustable voltage range

**Contoh Program:**
- **"0-800mV Range"** - ADC_ATTEN_DB_0 (most linear)
- **"0-1.1V Range"** - ADC_ATTEN_DB_2_5
- **"0-3.3V Range"** - ADC_ATTEN_DB_11

#### 3. **I2S DMA Mode (Fast Sampling)**
**Deskripsi:** Up to 150 kHz continuous

**Contoh Program:**
- **"Audio Input Processing"** - Microphone sampling
- **"Signal Capture"** - Fast waveform digitization
- **"FFT Analysis"** - Frequency domain analysis

#### 4. **Hall Sensor (Built-in)**
**Deskripsi:** Magnetic field detection

**Contoh Program:**
- **"Magnetic Switch"** - hall_sensor_read()
- **"RPM Counter"** - Count magnet pulses
- **"Door Sensor"** - Detect open/close

#### 5. **ADC2 WiFi Sharing**
**Deskripsi:** ADC2 unusable dengan WiFi

**Contoh Program:**
- **"ADC1 Only with WiFi"** - Safe GPIO selection
- **"WiFi Stop Workaround"** - Temporarily disable WiFi
- **"Pin Planning"** - Route sensors ke ADC1 pins

---

### 📊 ADC COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Resolution** | 12-bit (16-bit H7) | 12-bit (13-bit S2/S3) |
| **Channels** | 16-24 | ADC1: 8, ADC2: 10 |
| **Max Speed** | 5 MSPS | 150 kSPS (I2S) |
| **DMA Support** | ✅ Hardware | ⚠️ Via I2S only |
| **Timer Trigger** | ✅ Hardware | ❌ Software |
| **Calibration** | ⚠️ Manual | ✅ Factory eFuse |
| **Analog Watchdog** | ✅ Hardware | ❌ Software |
| **Internal Temp** | ✅ Built-in | ❌ No |
| **VREF Measure** | ✅ VREFINT | ❌ No |
| **Voltage Range** | Fixed 0-VDDA | ✅ 4 attenuation levels |
| **Hall Sensor** | ❌ External | ✅ Built-in |
| **WiFi Conflict** | ✅ No issue | ⚠️ ADC2 blocked |

---

### 💼 MINI PROJECT: Weather Station

**Deskripsi:** Multi-sensor weather monitoring

**STM32 Implementation:**
- Program "4-Channel Logger" - Temp, Humidity, Light, Pressure
- Program "DMA Continuous Sampling" - 1 Hz data logging
- Program "SD Card Storage" - Save to FAT32 filesystem

**ESP32 Implementation:**
- Program "IoT Weather Station" - WiFi + MQTT
- Program "ThingSpeak Upload" - Cloud data logging
- Program "Web Dashboard" - Real-time chart display

**Sensors:**
- Temperature: LM35 / TMP36
- Humidity: Resistive sensor (voltage divider)
- Light: LDR / Phototransistor
- Pressure: MPX5700 (analog pressure sensor)

---

## BAB 6: TIMER, PWM & OUTPUT CONTROL

> **Tujuan Pembelajaran:**
> - Timer basics (counter, prescaler, period)
> - Generate PWM untuk motor/LED
> - Input capture untuk frequency measurement
> - Output compare untuk precise timing
> - Encoder interface

### 📖 PENJELASAN MATERI

**Timer = Counter + Clock:**
```
Timer Frequency = Clock / (Prescaler + 1)
Period = Timer Frequency / (Period + 1)

Example:
Clock = 84 MHz
Prescaler = 83 → Timer clock = 84MHz / 84 = 1 MHz
Period = 999 → PWM freq = 1MHz / 1000 = 1 kHz
```

**PWM (Pulse Width Modulation):**
```
Duty Cycle = (CCR / ARR) * 100%

     ┌──┐    ┌──┐    ┌──┐
ON   │  │    │  │    │  │
OFF──┘  └────┘  └────┘  └───
     ←→ ←─────→
     50%   25%
```

**Applications:**
- LED brightness control
- DC motor speed control
- Servo motor angle control
- Audio tone generation
- Heater power control

---

### 🔗 COMMON GROUND: Fitur Timer yang Sama

#### 1. **Basic PWM Output (LED Dimming)**
**Deskripsi:** Control LED brightness

**Contoh Program STM32:**
- **"TIM2 PWM PA0"** - 1 kHz PWM di pin PA0
- **"Breathing LED"** - Fade in/fade out effect
- **"Multi-Channel PWM"** - RGB LED control

**Contoh Program ESP32:**
- **"LEDC PWM"** - LED Control peripheral untuk PWM
- **"Smooth Fade"** - Hardware fade dengan ledc_set_fade()
- **"16-Channel PWM"** - Control 16 LED independent

#### 2. **DC Motor Speed Control**
**Deskripsi:** Variable speed control

**Contoh Program STM32:**
- **"H-Bridge Motor Driver"** - L298N control
- **"Speed Ramp"** - Gradual acceleration
- **"Direction Control"** - Forward/reverse/brake

**Contoh Program ESP32:**
- **"MCPWM Motor Control"** - Motor Control PWM peripheral
- **"Dual Motor Control"** - Left/right motors
- **"PID Speed Control"** - Closed-loop dengan encoder

#### 3. **Servo Motor Control**
**Deskripsi:** Angle positioning (RC servo)

**Contoh Program STM32:**
- **"SG90 Servo"** - 0-180° control
- **"Multi-Servo Controller"** - 4 servos simultaneous
- **"Servo Sweep"** - Scan 0-180° continuously

**Contoh Program ESP32:**
- **"ESP32Servo Library"** - Arduino-style servo
- **"MCPWM Servo"** - Native ESP-IDF servo
- **"WiFi-Controlled Servo"** - Remote control via web

#### 4. **Input Capture (Frequency Measurement)**
**Deskripsi:** Measure signal frequency

**Contoh Program STM32:**
- **"Frequency Counter"** - Input Capture mode
- **"RPM Measurement"** - Motor speed sensing
- **"Pulse Width Measurement"** - Duration timing

**Contoh Program ESP32:**
- **"RMT Peripheral"** - Remote Control RX
- **"PCNT Pulse Counter"** - Count pulses
- **"Frequency from GPIO"** - Software timing

#### 5. **Timer Interrupt (Periodic Tasks)**
**Deskripsi:** Execute function setiap interval

**Contoh Program STM32:**
- **"1 ms Tick Timer"** - TIM6 untuk system tick
- **"LED Blink Timer"** - 1 Hz timer interrupt
- **"Multiple Timers"** - TIM2 (1s), TIM3 (500ms)

**Contoh Program ESP32:**
- **"ESP Timer"** - High-resolution periodic timer
- **"FreeRTOS Timer"** - Software timer
- **"Hardware Timer"** - hw_timer_t untuk us precision

---

### ⭐ STM32 TIMER: Keunggulan Unik

#### 1. **32-bit Advanced Timers (TIM2/TIM5)**
**Deskripsi:** Long period tanpa overflow

**Contoh Program:**
- **"Long Duration Timer"** - 4.2 billion counts
- **"High-Resolution PWM"** - 32-bit duty cycle resolution
- **"Microsecond Delay"** - Precise timing functions

#### 2. **Complementary PWM (TIM1/TIM8)**
**Deskripsi:** PWM + inverted PWM (motor control)

**Contoh Program:**
- **"BLDC Motor Control"** - 6-step commutation
- **"Dead-Time Insertion"** - Prevent shoot-through
- **"Break Input"** - Emergency stop input

#### 3. **Quadrature Encoder Interface**
**Deskripsi:** Hardware encoder counting

**Contoh Program:**
- **"Rotary Encoder"** - TIM3 encoder mode
- **"Motor Position Control"** - Closed-loop position
- **"Velocity Calculation"** - Speed dari encoder pulses

#### 4. **One-Pulse Mode**
**Deskripsi:** Single precise pulse output

**Contoh Program:**
- **"Stepper Motor Step"** - Generate step pulses
- **"Camera Trigger"** - Precise timing trigger
- **"Ultrasonic Sensor"** - Generate 10us pulse

#### 5. **Timer Synchronization (Master/Slave)**
**Deskripsi:** Multiple timers synchronized

**Contoh Program:**
- **"Cascaded Timers"** - TIM2 triggers TIM3
- **"ADC + Timer Sync"** - Timer trigger ADC sampling
- **"PWM Synchronized Channels"** - Phase-locked PWM

#### 6. **DMA Burst Mode**
**Deskripsi:** Update timer registers via DMA

**Contoh Program:**
- **"Waveform Generation"** - Complex PWM patterns
- **"DMA-Based Frequency Change"** - Smooth transitions
- **"Multi-Step Sequence"** - Automated timing sequence

---

### ⚡ ESP32 TIMER: Keunggulan Unik

#### 1. **LEDC PWM (16 Channels)**
**Deskripsi:** LED Control dengan 16 independent channels

**Contoh Program:**
- **"16 LED Independent Control"** - Semua fade berbeda
- **"Hardware Fade"** - ledc_set_fade_with_time()
- **"RGB Strip Control"** - Multiple RGB LEDs

#### 2. **MCPWM (Motor Control PWM)**
**Deskripsi:** Dedicated motor control peripheral

**Contoh Program:**
- **"BLDC 3-Phase Control"** - Brushless DC motor
- **"H-Bridge Driver"** - Built-in dead-time
- **"Fault Detection"** - Hardware fault pin

#### 3. **RMT (Remote Control)**
**Deskripsi:** Flexible TX/RX untuk IR/NeoPixel

**Contoh Program:**
- **"WS2812B LED Strip"** - NeoPixel control
- **"IR Remote TX"** - Send NEC/RC5 codes
- **"IR Remote RX"** - Decode remote signals

#### 4. **PCNT (Pulse Counter)**
**Deskripsi:** Hardware pulse counting

**Contoh Program:**
- **"Frequency Counter"** - Count pulses up to 40 MHz
- **"Quadrature Decoder"** - Rotary encoder
- **"Flow Meter"** - Water/gas flow measurement

#### 5. **ESP Timer (High-Resolution)**
**Deskripsi:** 64-bit microsecond timer

**Contoh Program:**
- **"Microsecond Periodic Callback"** - esp_timer_create()
- **"One-Shot Timer"** - Single execution
- **"Timer Statistics"** - Track timer overhead

#### 6. **Pin Flexibility**
**Deskripsi:** PWM/Timer ke pin manapun

**Contoh Program:**
- **"Custom PWM Pins"** - Output ke GPIO pilihan
- **"Avoid Pin Conflicts"** - Route PWM freely
- **"Dynamic Pin Allocation"** - Runtime pin change

---

### 📊 TIMER/PWM COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Timer Instances** | 2-17 (series) | 4 timers + LEDC + MCPWM |
| **PWM Channels** | 4 per timer | LEDC: 16, MCPWM: 6 |
| **Resolution** | 16/32-bit | 16-bit (up to 20-bit LEDC) |
| **Max Frequency** | Clock dependent | 40 MHz (LEDC), 160 MHz (MCPWM) |
| **Complementary PWM** | ✅ TIM1/TIM8 | ✅ MCPWM |
| **Dead-Time Insert** | ✅ Hardware | ✅ MCPWM |
| **Encoder Interface** | ✅ Hardware | ✅ PCNT |
| **DMA Support** | ✅ Full DMA | ❌ No timer DMA |
| **Hardware Fade** | ❌ No | ✅ LEDC fade |
| **IR/NeoPixel** | ⚠️ Via software | ✅ RMT hardware |
| **Pin Flexibility** | ⚠️ AF fixed | ✅ Any GPIO |
| **Timer Sync** | ✅ Master/Slave | ❌ Limited |

---

### 💼 MINI PROJECT: Smart Fan Controller

**Deskripsi:** Temperature-controlled variable speed fan

**STM32 Implementation:**
- Program "PWM Fan Control" - 25kHz PWM untuk fan
- Program "Temperature-Based Speed" - ADC temperature → PWM duty
- Program "PID Controller" - Maintain setpoint temperature

**ESP32 Implementation:**
- Program "IoT Smart Fan" - WiFi control via mobile app
- Program "Web Dashboard" - Real-time temp + speed graph
- Program "Schedule Timer" - Auto ON/OFF based on time

**Features:**
- Temperature sensor (LM35)
- DC fan (12V, PWM control via MOSFET)
- OLED display (temp + speed percentage)
- Buttons untuk manual override
- WiFi remote control (ESP32 only)

---

*Lanjutan Bab 7-14 akan saya tambahkan di reply berikutnya karena panjang konten. Silakan konfirmasi jika format dan struktur ini sudah sesuai!*

## BAB 7: I²C PROTOCOL & DEVICE COMMUNICATION

> **Tujuan Pembelajaran:**
> - I²C protocol basics (master/slave, addressing)
> - Scan I²C bus untuk detect devices
> - Read/write sensors (OLED, RTC, EEPROM)
> - Multi-master arbitration
> - Clock stretching & error handling

### 📖 PENJELASAN MATERI

**I²C (Inter-Integrated Circuit):**
Bus komunikasi serial 2-wire:
- **SDA**: Serial Data (bidirectional)
- **SCL**: Serial Clock (dari master)
- **Pull-up resistors**: Required (4.7kΩ typical)

**Kecepatan:**
- Standard Mode: 100 kHz
- Fast Mode: 400 kHz  
- Fast Mode Plus: 1 MHz
- High Speed: 3.4 MHz (rare)

**Addressing:**
```
7-bit address: 0x00 - 0x7F (128 devices)
10-bit address: 0x000 - 0x3FF (extended)

Example devices:
  OLED SSD1306:   0x3C
  RTC DS3231:     0x68
  EEPROM 24C32:   0x50
  BMP280:         0x76 atau 0x77
```

**Transaction Format:**
```
START → [Address + R/W] → ACK → [Data byte] → ACK → ... → STOP
  ↓            ↓           ↓          ↓          ↓
Master    7-bit addr    Slave    Transfer    Master
         + 1 read bit   respond   data       release
```

---

### 🔗 COMMON GROUND: Fitur I²C yang Sama

#### 1. **I²C Scanner (Device Detection)**
**Deskripsi:** Scan semua address untuk find devices

**Contoh Program STM32:**
- **"HAL I2C Scanner"** - Scan 0x00-0x7F, print found addresses
- **"Device Identifier"** - Read device ID register
- **"Bus Diagnostic Tool"** - Check SDA/SCL levels

**Contoh Program ESP32:**
- **"Wire.begin() Scanner"** - Arduino I2C scanner
- **"I2C Driver Scanner"** - ESP-IDF i2c_master_probe()
- **"OLED Auto-Detect"** - Detect 0x3C or 0x3D

#### 2. **OLED Display (SSD1306)**
**Deskripsi:** 128x64 monochrome display

**Contoh Program STM32:**
- **"SSD1306 HAL Library"** - Display text + graphics
- **"Real-Time Sensor Display"** - Show ADC values
- **"Menu System"** - Button navigation UI

**Contoh Program ESP32:**
- **"Adafruit SSD1306"** - Arduino library
- **"u8g2 Graphics"** - Advanced graphics library
- **"WiFi Status Display"** - Show IP, signal strength

#### 3. **RTC (Real-Time Clock DS3231)**
**Deskripsi:** Accurate timekeeping

**Contoh Program STM32:**
- **"Set/Get Time"** - Write/read time registers
- **"Alarm Function"** - Configure alarm interrupts
- **"Temperature Compensated"** - Read internal temp

**Contoh Program ESP32:**
- **"RTC + NTP Sync"** - Sync dengan internet time
- **"Data Logger Timestamp"** - Add time to log entries
- **"Scheduler"** - Execute tasks at specific time

#### 4. **EEPROM (24C32/24C256)**
**Deskripsi:** Non-volatile data storage

**Contoh Program STM32:**
- **"EEPROM Write/Read"** - Store configuration
- **"Wear Leveling"** - Distribute writes evenly
- **"Factory Reset"** - Restore default settings

**Contoh Program ESP32:**
- **"Preferences vs EEPROM"** - NVS better alternative
- **"Config Storage"** - WiFi credentials storage
- **"User Settings"** - Persistent user data

#### 5. **Multi-Sensor Reading**
**Deskripsi:** Multiple I²C sensors on same bus

**Contoh Program STM32:**
- **"BME280 + OLED"** - Temp/humidity/pressure display
- **"Sensor Fusion"** - MPU6050 + BMP280
- **"Multi-Device Poll"** - Read 5 sensors sequentially

**Contoh Program ESP32:**
- **"I2C Sensor Array"** - 4 temperature sensors (different addresses)
- **"Weather Station"** - BME280 + BH1750 (light) + CCS811 (air quality)
- **"IoT Sensor Node"** - All data to MQTT

---

### ⭐ STM32 I²C: Keunggulan Unik

#### 1. **Hardware DMA Support**
**Deskripsi:** I²C transfer tanpa CPU blocking

**Contoh Program:**
- **"DMA I2C Transfer"** - HAL_I2C_Master_Transmit_DMA()
- **"Large Data Transfer"** - EEPROM 1KB write dengan DMA
- **"Continuous Sensor Read"** - DMA circular buffer

#### 2. **Dual Address Mode (Slave)**
**Deskripsi:** Respond to 2 addresses

**Contoh Program:**
- **"I2C Slave Dual Address"** - 0x50 dan 0x51
- **"Multi-Function Slave"** - Different functions per address
- **"I2C Bridge"** - Forward commands to UART

#### 3. **SMBus / PMBus Support**
**Deskripsi:** SMBus protocol (I²C variant)

**Contoh Program:**
- **"SMBus Battery Monitor"** - Read smart battery
- **"PEC (Packet Error Check)"** - CRC untuk reliability
- **"Alert Response"** - SMBus alert handling

#### 4. **Fast Mode Plus (1 MHz)**
**Deskripsi:** High-speed I²C

**Contoh Program:**
- **"1 MHz I2C Configuration"** - Fast communication
- **"High-Speed Data Acquisition"** - Rapid sensor reading
- **"Performance Benchmark"** - Compare speeds

#### 5. **Hardware Timeout**
**Deskripsi:** Automatic bus hang recovery

**Contoh Program:**
- **"Bus Timeout Config"** - Prevent infinite wait
- **"Error Recovery"** - Auto-reset on hang
- **"Reliable Communication"** - Handle slave failures

---

### ⚡ ESP32 I²C: Keunggulan Unik

#### 1. **Flexible Pin Mapping**
**Deskripsi:** I²C ke pin manapun

**Contoh Program:**
- **"Custom I2C Pins"** - SDA=GPIO21, SCL=GPIO22 (or anywhere!)
- **"Dual I2C Bus"** - I2C0 dan I2C1 di pins berbeda
- **"Avoid Pin Conflicts"** - Route around used pins

#### 2. **Two Independent I²C**
**Deskripsi:** I2C0 dan I2C1 simultan

**Contoh Program:**
- **"Dual Bus Operation"** - 2 separate I2C networks
- **"Master + Slave Mode"** - I2C0 master, I2C1 slave
- **"Isolated Sensors"** - Prevent address conflicts

#### 3. **Large TX/RX Buffer**
**Deskripsi:** Hardware buffer untuk burst transfer

**Contoh Program:**
- **"256 Byte Transfer"** - Write large data at once
- **"EEPROM Fast Write"** - Page write optimization
- **"Buffered Read"** - Reduce interrupt frequency

#### 4. **Built-in Slave Mode**
**Deskripsi:** Act as I²C slave device

**Contoh Program:**
- **"I2C Slave Peripheral"** - ESP32 sebagai sensor
- **"Remote Control Slave"** - Accept commands dari master
- **"I2C to UART Bridge"** - Protocol converter

#### 5. **Clock Stretching Support**
**Deskripsi:** Slave can hold SCL low

**Contoh Program:**
- **"Slow Slave Handling"** - Support old devices
- **"Clock Stretch Config"** - i2c_set_timeout()
- **"Robust Communication"** - Handle slow responses

---

### 📊 I²C COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Instances** | 1-4 (series) | 2 (I2C0, I2C1) |
| **Max Speed** | 1 MHz (Fm+) | 1 MHz (configurable) |
| **DMA Support** | ✅ Hardware DMA | ❌ No DMA |
| **Pin Flexibility** | ⚠️ AF mapping | ✅ Any GPIO |
| **Master Mode** | ✅ Full support | ✅ Full support |
| **Slave Mode** | ✅ Supported | ✅ Supported |
| **Dual Address** | ✅ Slave mode | ❌ No |
| **SMBus/PMBus** | ✅ Supported | ❌ No |
| **Buffer Size** | Small (6 bytes) | 256 bytes (configurable) |
| **Clock Stretching** | ✅ Supported | ✅ Supported |
| **10-bit Address** | ✅ Supported | ✅ Supported |
| **Multi-Master** | ✅ Arbitration | ✅ Arbitration |

---

### 💼 MINI PROJECT: Environmental Monitoring Station

**Deskripsi:** Multi-sensor I²C dashboard

**STM32 Implementation:**
- Program "Sensor Hub" - BME280 (temp/humidity) + BH1750 (light) + OLED display
- Program "Data Logger" - Save to EEPROM every 5 minutes
- Program "RTC Timestamp" - DS3231 real-time clock

**ESP32 Implementation:**
- Program "IoT Weather Station" - Same sensors + WiFi
- Program "Web Dashboard" - Chart.js visualization
- Program "MQTT Publisher" - Send to cloud every minute

**I²C Devices:**
- BME280 (0x76): Temperature, humidity, pressure
- BH1750 (0x23): Light intensity
- SSD1306 (0x3C): OLED display 128x64
- DS3231 (0x68): RTC dengan alarm
- 24C256 (0x50): 32KB EEPROM (optional)

---

## BAB 8: SPI PROTOCOL & HIGH-SPEED TRANSFER

> **Tujuan Pembelajaran:**
> - SPI full-duplex communication
> - Chip Select management (multiple slaves)
> - Read/write SD card (FAT filesystem)
> - NRF24L01 wireless module
> - High-speed data transfer (DMA)

### 📖 PENJELASAN MATERI

**SPI (Serial Peripheral Interface):**
Bus 4-wire full-duplex:
```
MOSI: Master Out, Slave In (TX dari master)
MISO: Master In, Slave Out (RX ke master)
SCK:  Serial Clock (dari master)
CS:   Chip Select (aktif LOW)
```

**Clock Polarity & Phase:**
```
Mode 0: CPOL=0, CPHA=0 (idle LOW, sample on rising)
Mode 1: CPOL=0, CPHA=1 (idle LOW, sample on falling)
Mode 2: CPOL=1, CPHA=0 (idle HIGH, sample on falling)
Mode 3: CPOL=1, CPHA=1 (idle HIGH, sample on rising)
```

**Kecepatan:**
- Typical: 1-10 MHz
- Fast: 10-50 MHz
- Ultra-fast: 50-100 MHz (short wires!)

**Multiple Slaves:**
```
Master                Slave 1
  MOSI ─────┬────────→ MOSI
  MISO ←────┼────────── MISO
  SCK  ─────┼────────→ SCK
  CS1  ─────┴────────→ CS

            └────────→ Slave 2
                       MOSI
              ←─────── MISO
            ─────────→ SCK
  CS2  ─────────────→ CS
```

---

### 🔗 COMMON GROUND: Fitur SPI yang Sama

#### 1. **Basic SPI Transfer**
**Deskripsi:** Send/receive data byte

**Contoh Program STM32:**
- **"HAL_SPI_Transmit"** - Send data blocking
- **"HAL_SPI_TransmitReceive"** - Full-duplex exchange
- **"SPI Speed Test"** - Benchmark transfer rate

**Contoh Program ESP32:**
- **"spi_device_transmit"** - ESP-IDF SPI transfer
- **"SPIClass::transfer()"** - Arduino SPI
- **"Loopback Test"** - Connect MOSI to MISO

#### 2. **SD Card (FAT Filesystem)**
**Deskripsi:** Read/write files di SD card

**Contoh Program STM32:**
- **"FatFs Mount SD"** - f_mount(), f_open(), f_read()
- **"Data Logger CSV"** - Append sensor data to file
- **"File Browser"** - List directory contents

**Contoh Program ESP32:**
- **"SD_MMC vs SD SPI"** - SD card via SPI or SDIO
- **"SPIFFS vs SD"** - Compare flash vs SD storage
- **"Audio WAV Player"** - Play WAV dari SD card

#### 3. **NRF24L01 (Wireless Module)**
**Deskripsi:** 2.4 GHz RF communication

**Contoh Program STM32:**
- **"NRF24 Library"** - nRF24L01+ transmit/receive
- **"Wireless Sensor Node"** - Send data via RF
- **"Multi-Node Network"** - 1 master, 5 slaves

**Contoh Program ESP32:**
- **"NRF24 + WiFi Gateway"** - RF to WiFi bridge
- **"Low-Power Remote"** - NRF24 untuk control
- **"Mesh Network"** - ESP-NOW alternative

#### 4. **TFT Display (ILI9341)**
**Deskripsi:** Color touchscreen LCD

**Contoh Program STM32:**
- **"TFT Graphics"** - Draw shapes, text, images
- **"Touch Input"** - Read touch coordinates
- **"GUI Dashboard"** - Sensor data visualization

**Contoh Program ESP32:**
- **"TFT_eSPI Library"** - High-performance graphics
- **"LVGL GUI"** - Professional UI framework
- **"Video Streaming"** - Display camera feed

#### 5. **External Flash (W25Q32)**
**Deskripsi:** SPI NOR flash memory

**Contoh Program STM32:**
- **"Flash Read/Write"** - Store large data
- **"Bootloader Update"** - Firmware storage
- **"Data Buffer"** - Circular buffer di flash

**Contoh Program ESP32:**
- **"External Flash Partition"** - esp_partition API
- **"OTA from External Flash"** - Firmware update
- **"Large Data Storage"** - Audio/image files

---

### ⭐ STM32 SPI: Keunggulan Unik

#### 1. **DMA Circular Mode**
**Deskripsi:** Continuous high-speed transfer

**Contoh Program:**
- **"DMA SPI Streaming"** - Transfer large buffers
- **"Display DMA Update"** - Fast screen refresh
- **"Audio DAC Output"** - Stream audio via SPI

#### 2. **Hardware NSS Management**
**Deskripsi:** Auto CS control

**Contoh Program:**
- **"Hardware CS Pin"** - Auto-toggle NSS
- **"Multi-Slave Auto"** - Hardware multiplexing
- **"Simplified Code"** - No manual GPIO toggle

#### 3. **SPI Master/Slave Mode**
**Deskripsi:** Flexible master atau slave

**Contoh Program:**
- **"SPI Slave Peripheral"** - Act as SPI slave
- **"SPI to SPI Bridge"** - Master ↔ Slave forward
- **"Data Acquisition Slave"** - External ADC control

#### 4. **TI Mode & Motorola Mode**
**Deskripsi:** Protocol variants

**Contoh Program:**
- **"TI Synchronous Serial"** - TI protocol mode
- **"Motorola SPI"** - Standard SPI mode
- **"Protocol Compatibility"** - Support old devices

#### 5. **CRC Hardware Calculation**
**Deskripsi:** Built-in error detection

**Contoh Program:**
- **"SPI CRC Check"** - Enable hardware CRC
- **"Reliable Transfer"** - Detect transmission errors
- **"Critical Data"** - Verify integrity

---

### ⚡ ESP32 SPI: Keunggulan Unik

#### 1. **Flexible GPIO Mapping**
**Deskripsi:** SPI pins anywhere

**Contoh Program:**
- **"Custom SPI Pins"** - MOSI=GPIO23, MISO=GPIO19, etc
- **"VSPI + HSPI"** - 2 independent SPI buses
- **"Pin Conflict Resolution"** - Route around used pins

#### 2. **HSPI & VSPI (Dual SPI)**
**Deskripsi:** 2 independent SPI peripherals

**Contoh Program:**
- **"VSPI for Display"** - TFT on VSPI
- **"HSPI for SD Card"** - Separate bus untuk SD
- **"Dual-Bus Performance"** - Parallel transfers

#### 3. **DMA-Like Transfer**
**Deskripsi:** Large buffer transfer

**Contoh Program:**
- **"Transaction Queue"** - Queue multiple SPI transactions
- **"Large Display Update"** - 76800 bytes (320x240 TFT)
- **"Background Transfer"** - Non-blocking DMA-style

#### 4. **Integrated SD/SDIO Controller**
**Deskripsi:** Dedicated SD card interface

**Contoh Program:**
- **"SDIO 4-bit Mode"** - High-speed SD (50 MHz)
- **"SD_MMC vs SPI"** - Compare performance
- **"Fast File Access"** - 10+ MB/s throughput

#### 5. **Slave HD Mode (Half-Duplex)**
**Deskripsi:** SPI slave command/data mode

**Contoh Program:**
- **"SPI Slave Device"** - ESP32 as slave peripheral
- **"Command Response"** - Handle master commands
- **"Large Data Transfer"** - Slave DMA buffer

---

### 📊 SPI COMPARISON TABLE

| Feature | STM32 | ESP32 |
|---------|-------|-------|
| **Instances** | 1-6 (series) | 4 (SPI0/1/2/3, usable: 2/3) |
| **Max Speed** | 50 MHz (F4/F7: 90 MHz) | 80 MHz (40 MHz practical) |
| **DMA Support** | ✅ Full DMA | ⚠️ Transaction queue |
| **Pin Flexibility** | ⚠️ AF mapping | ✅ Any GPIO |
| **Master Mode** | ✅ Supported | ✅ Supported |
| **Slave Mode** | ✅ Supported | ✅ HD/FD mode |
| **Hardware CS** | ✅ NSS auto | ⚠️ Manual GPIO |
| **Dual SPI** | ⚠️ Series dependent | ✅ VSPI + HSPI |
| **CRC Hardware** | ✅ Built-in | ❌ No |
| **TI/Motorola Mode** | ✅ Supported | ✅ Motorola only |
| **SDIO Interface** | ⚠️ Separate | ✅ Integrated |
| **Transaction Queue** | ❌ No | ✅ 7-transaction FIFO |

---

### 💼 MINI PROJECT: Data Logger with Display

**Deskripsi:** Multi-sensor logger + TFT display

**STM32 Implementation:**
- Program "SPI TFT Display" - ILI9341 real-time graph
- Program "SD Card Logger" - FatFs write sensor CSV
- Program "Touch UI" - Button navigation menu

**ESP32 Implementation:**
- Program "IoT Logger" - TFT display + WiFi upload
- Program "SPIFFS + SD Dual Storage" - Local + removable
- Program "Web Config" - Configure via browser

**Hardware:**
- TFT 2.4" ILI9341 (SPI)
- MicroSD card module (SPI atau SDIO di ESP32)
- Multiple sensors (I²C)
- Touch screen (XPT2046 SPI)

---

*Bab 9-14 akan dilanjutkan...*
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
