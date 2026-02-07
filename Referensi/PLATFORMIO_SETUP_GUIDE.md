# 🚀 PLATFORMIO SETUP GUIDE
## STM32 & ESP32 Development Environment

**Target:** Praktikum Sistem Embedded
**Tools:** PlatformIO, VSCode, ST-LINK, USB Drivers

---

## 📦 INSTALLATION CHECKLIST

### 1️⃣ Install Visual Studio Code
- Download: https://code.visualstudio.com/
- Platform: Windows/Linux/macOS
- ✅ Install Python extension (recommended)

### 2️⃣ Install PlatformIO Extension
1. Open VSCode
2. Go to Extensions (Ctrl+Shift+X)
3. Search "PlatformIO IDE"
4. Click Install
5. Reload VSCode
6. Wait for PlatformIO to finish installing (check status bar)

### 3️⃣ Install USB Drivers

**For STM32 (ST-LINK V2):**
- **Windows:** Download ST-LINK driver from: https://www.st.com/en/development-tools/stsw-link009.html
- **Linux:** Add udev rules (see below)
- **macOS:** Usually works out of box

**For ESP32:**
- **Windows:** CH340/CP2102 driver (usually auto-installed)
- **Linux:** No driver needed (built-in)
- **macOS:** May need CP210x driver

### 4️⃣ Linux udev Rules (Important!)

Create file `/etc/udev/rules.d/99-stlink.rules`:
```bash
sudo nano /etc/udev/rules.d/99-stlink.rules
```

Add content:
```
# ST-LINK V2
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="0666"

# ST-LINK V2-1
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="0666"

# ESP32
SUBSYSTEM=="usb", ATTR{idVendor}=="1a86", ATTR{idProduct}=="7523", MODE="0666"
SUBSYSTEM=="usb", ATTR{idVendor}=="10c4", ATTR{idProduct}=="ea60", MODE="0666"
```

Reload rules:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## 🔧 PROJECT SETUP

### STM32 Project (Blue Pill - STM32F103C8T6)

#### 1. Create New Project
```bash
# Via PlatformIO CLI
pio project init --board bluepill_f103c8

# Or use VSCode:
# PlatformIO > Home > New Project
# Name: STM32_Project
# Board: Generic STM32F103C8
# Framework: STM32Cube
```

#### 2. platformio.ini Configuration
```ini
[env:bluepill_f103c8]
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
upload_protocol = stlink
debug_tool = stlink

; Serial Monitor
monitor_speed = 115200

; Build flags for printf redirect
build_flags = 
    -D PIO_FRAMEWORK_ARDUINO_ENABLE_CDC
    -D USBCON

; Optional: Enable USB Serial
; lib_deps = 
;     stm32duino/STM32duino FreeRTOS@^10.0.0
```

#### 3. Folder Structure
```
STM32_Project/
├── include/           # Header files (.h)
├── lib/               # Custom libraries
├── src/
│   └── main.c         # Main program
├── test/              # Unit tests
└── platformio.ini     # Configuration
```

#### 4. Basic main.c Template
```c
#include "stm32f1xx_hal.h"

// System Clock Configuration (72 MHz)
void SystemClock_Config(void);

int main(void) {
    // Initialize HAL
    HAL_Init();
    
    // Configure system clock
    SystemClock_Config();
    
    // Enable GPIO Clock
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // Configure PC13 (LED on Blue Pill)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(1000);
    }
}

// System Clock Configuration
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // 72 MHz configuration
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

// Required for HAL
void SysTick_Handler(void) {
    HAL_IncTick();
}
```

#### 5. Build & Upload
```bash
# Build
pio run

# Upload (with ST-LINK connected)
pio run --target upload

# Serial Monitor
pio device monitor --baud 115200

# Clean build
pio run --target clean
```

---

### ESP32 Project (ESP32 DevKit v1)

#### 1. Create New Project
```bash
# Via PlatformIO CLI
pio project init --board esp32dev

# Or use VSCode:
# PlatformIO > Home > New Project
# Name: ESP32_Project
# Board: Espressif ESP32 Dev Module
# Framework: Espressif IDF
```

#### 2. platformio.ini Configuration
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = espidf

; Serial Monitor
monitor_speed = 115200
monitor_filters = esp32_exception_decoder

; Flash settings
upload_speed = 921600
board_build.flash_mode = dio
board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L

; Partition table (optional - for OTA)
; board_build.partitions = default.csv

; Dependencies
lib_deps = 
    ; Add libraries here if needed
```

#### 3. Folder Structure
```
ESP32_Project/
├── include/           # Header files (.h)
├── lib/               # Custom libraries
├── src/
│   ├── main.c         # Main program
│   └── CMakeLists.txt # Required for ESP-IDF
├── test/              # Unit tests
└── platformio.ini     # Configuration
```

#### 4. Basic main.c Template
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_2
static const char *TAG = "main";

void app_main(void) {
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Starting Blink Example");
    
    while (1) {
        gpio_set_level(LED_PIN, 1);
        ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        gpio_set_level(LED_PIN, 0);
        ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

#### 5. CMakeLists.txt (Required in src/)
```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")
```

#### 6. Build & Upload
```bash
# Build
pio run

# Upload (ESP32 in bootloader mode)
pio run --target upload

# Serial Monitor
pio device monitor --baud 115200

# Upload + Monitor
pio run --target upload && pio device monitor

# Clean build
pio run --target clean

# Erase flash
pio run --target erase
```

---

## 🔍 TROUBLESHOOTING

### STM32 Issues

**Problem:** "Error: target not found" during upload
```
Solution:
1. Check ST-LINK connection (LED should be solid red)
2. Verify USB cable (must be data cable, not charge-only)
3. Try different USB port
4. Install ST-LINK driver (Windows)
5. Check udev rules (Linux)
6. Try: pio run --target upload --upload-port /dev/ttyUSB0
```

**Problem:** "Could not open serial device"
```
Solution (Linux):
sudo usermod -a -G dialout $USER
# Then logout and login again
```

**Problem:** Printf not working
```
Solution:
1. Add UART1 configuration in main.c
2. Redirect printf in syscalls.c:
   int _write(int file, char *ptr, int len) {
       HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
       return len;
   }
3. Use USB-TTL adapter on PA9 (TX) and PA10 (RX)
```

### ESP32 Issues

**Problem:** "Failed to connect to ESP32"
```
Solution:
1. Hold BOOT button while uploading starts
2. Or add auto-reset circuit (EN + GPIO0)
3. Try lower upload speed: upload_speed = 115200
4. Check USB cable (must be data cable)
5. Install CH340/CP2102 driver
```

**Problem:** "Brownout detector was triggered"
```
Solution:
1. Use better power supply (avoid USB hub)
2. Add capacitor (100µF) on 3.3V rail
3. Reduce WiFi usage
4. Lower CPU frequency temporarily
```

**Problem:** "Stack overflow in task"
```
Solution:
1. Increase stack size in xTaskCreate:
   xTaskCreate(task, "name", 4096, NULL, 5, NULL); // 4KB stack
2. Check for large local arrays
3. Use heap allocation (malloc) for large data
```

**Problem:** "Guru Meditation Error"
```
Solution:
1. Enable exception decoder: monitor_filters = esp32_exception_decoder
2. Check backtrace in serial monitor
3. Common causes:
   - Null pointer dereference
   - Stack overflow
   - Writing to read-only memory
   - Division by zero
```

---

## 📝 COMMON COMMANDS

### PlatformIO CLI

```bash
# List all boards
pio boards

# List connected devices
pio device list

# Update platforms
pio platform update

# Install library
pio lib install "ArduinoJson"

# Search library
pio lib search "OLED"

# Project info
pio project config

# Generate compile_commands.json (for IntelliSense)
pio run --target compiledb

# Clean all
pio run --target clean

# Test
pio test
```

### Git Version Control

```bash
# Initialize repo
git init
git add .
git commit -m "Initial commit"

# Create .gitignore
echo ".pio/
.vscode/
*.bak
*.swp" > .gitignore
```

---

## 🎯 QUICK START WORKFLOW

### For STM32:
```
1. Create project in VSCode (PlatformIO > New Project)
2. Select board: Generic STM32F103C8
3. Framework: STM32Cube
4. Copy-paste template main.c
5. Connect ST-LINK to Blue Pill:
   - SWDIO → SWDIO
   - SWCLK → SWCLK
   - GND   → GND
   - 3.3V  → 3.3V
6. pio run --target upload
7. LED on PC13 should blink!
```

### For ESP32:
```
1. Create project in VSCode (PlatformIO > New Project)
2. Select board: Espressif ESP32 Dev Module
3. Framework: Espressif IDF
4. Copy-paste template main.c + CMakeLists.txt
5. Connect ESP32 via USB
6. Hold BOOT button during upload
7. pio run --target upload
8. LED on GPIO2 should blink!
```

---

## 📚 NEXT STEPS

After successful blink:
1. ✅ Test Serial Monitor (printf debugging)
2. ✅ Add button input (GPIO read)
3. ✅ Try external interrupt
4. ✅ Read ADC value (potentiometer)
5. ✅ Generate PWM (LED dimming)
6. ✅ I²C OLED display
7. ✅ FreeRTOS multi-tasking

Refer to main documentation: `MODUL_LENGKAP_14_BAB_STM32_vs_ESP32.md`

---

## 🆘 SUPPORT

### Official Resources:
- PlatformIO Docs: https://docs.platformio.org
- STM32 HAL: https://www.st.com/resource/en/user_manual/dm00154093.pdf
- ESP-IDF: https://docs.espressif.com/projects/esp-idf/

### Community:
- PlatformIO Forum: https://community.platformio.org
- STM32 Forum: https://community.st.com
- ESP32 Forum: https://www.esp32.com
- r/embedded (Reddit)

---

**Setup Guide v1.0** | PlatformIO + STM32 + ESP32
