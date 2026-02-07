# 📑 PANDUAN LENGKAP 14 MODUL STM32 vs ESP32
## Untuk Pembelajaran Sistem Embedded dengan PlatformIO

**Platform:**
- STM32: Framework STM32Cube (HAL)
- ESP32: Framework Espressif (ESP-IDF)
- IDE: PlatformIO (VSCode)

**Total:** 6457 baris | 14 Bab Lengkap | Ratusan Contoh Program

---

## 📚 STRUKTUR MODUL

### FASE 1: FONDASI (3 Pertemuan)

#### **BAB 1: Pengenalan Arsitektur & Setup Environment**
**Durasi:** 2-3 jam | **Kesulitan:** ⭐☆☆☆☆

**Topik:**
- ✅ Perbandingan arsitektur ARM Cortex-M vs Xtensa
- ✅ Setup PlatformIO untuk kedua platform
- ✅ First project: Blink LED
- ✅ Memory map & peripheral organization

**Contoh Program (15+ programs):**
- **Common:** Blink LED, Serial Print, Button Input
- **STM32:** HAL Library basics, STM32CubeMX integration, Clock tree
- **ESP32:** Arduino vs ESP-IDF, FreeRTOS basics, Dual-core demo

**Output Pembelajaran:**
- Mahasiswa dapat setup environment kedua platform
- Memahami perbedaan fundamental architecture
- Sukses compile & upload program pertama

---

#### **BAB 2: Digital I/O & GPIO Programming**
**Durasi:** 2-3 jam | **Kesulitan:** ⭐⭐☆☆☆

**Topik:**
- ✅ GPIO modes (input, output, pull-up/down, open-drain)
- ✅ High-speed toggling techniques
- ✅ Atomic operations (BSRR, bit-banding)
- ✅ GPIO matrix flexibility (ESP32)
- ✅ Capacitive touch sensors (ESP32)

**Contoh Program (20+ programs):**
- **Common:** LED control, button reading, debouncing, multiple GPIO
- **STM32:** BSRR atomic toggle, bit-banding, 5V tolerant pins, GPIO lock
- **ESP32:** GPIO matrix, 10 touch sensors, RTC GPIO untuk deep sleep

**Mini Project:** Traffic Light Controller (3 LED + timing control)

---

#### **BAB 3: External Interrupts & Event Handling**
**Durasi:** 2-3 jam | **Kesulitan:** ⭐⭐⭐☆☆

**Topik:**
- ✅ Polling vs Interrupt comparison
- ✅ EXTI configuration (STM32) vs GPIO ISR (ESP32)
- ✅ Interrupt priority & nesting
- ✅ Debouncing dalam interrupt
- ✅ Deferred interrupt processing

**Contoh Program (25+ programs):**
- **Common:** Button interrupt, debounced input, dual-edge detection, multiple ISR
- **STM32:** NVIC priority grouping, nested interrupts, bit-banding flags
- **ESP32:** Any GPIO interrupt, FreeRTOS notification, per-core interrupt, touch interrupt

**Mini Project:** Reaction Time Tester Game

---

### FASE 2: KOMUNIKASI (3 Pertemuan)

#### **BAB 4: UART/Serial Communication**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐☆☆☆

**Topik:**
- ✅ UART frame format & baud rate
- ✅ Printf redirection untuk debugging
- ✅ Interrupt-driven & DMA reception
- ✅ Binary protocol design
- ✅ Command parser implementation

**Contoh Program (30+ programs):**
- **Common:** Serial print, echo, command parser, binary protocol, multi-UART
- **STM32:** DMA circular buffer, flow control (RTS/CTS), LIN mode, SmartCard
- **ESP32:** Pattern detection, RS485 auto-direction, flexible pins, sleep wakeup

**Mini Project:** Wireless Sensor Network (Master-Slave via UART)

---

#### **BAB 5: ADC - Analog Input & Sensor Reading**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐☆☆

**Topik:**
- ✅ ADC resolution & sampling time
- ✅ Sensor interfacing (potensiometer, LDR, temperature)
- ✅ Multi-channel scanning
- ✅ Noise reduction techniques
- ✅ Calibration methods

**Contoh Program (25+ programs):**
- **Common:** Potentiometer, LDR, LM35/TMP36, multi-channel, averaging filter
- **STM32:** DMA continuous, timer-triggered ADC, internal temp, VREFINT, analog watchdog, 16-bit (H7)
- **ESP32:** eFuse calibration, attenuation control, I2S ADC (150kHz), Hall sensor

**Mini Project:** Weather Station (4-channel data logger)

---

#### **BAB 6: Timer, PWM & Output Control**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐☆☆

**Topik:**
- ✅ Timer basics (prescaler, period, counter)
- ✅ PWM generation untuk motor/LED
- ✅ Input capture untuk frequency measurement
- ✅ Servo control
- ✅ Encoder interface

**Contoh Program (28+ programs):**
- **Common:** PWM LED dimming, DC motor speed, servo control, input capture, timer interrupt
- **STM32:** 32-bit timers, complementary PWM, dead-time, quadrature encoder, DMA burst
- **ESP32:** LEDC 16-channel, MCPWM motor control, RMT (NeoPixel/IR), PCNT counter

**Mini Project:** Smart Fan Controller (temperature-based PWM)

---

### FASE 3: PROTOKOL LANJUT (3 Pertemuan)

#### **BAB 7: I²C Protocol & Device Communication**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐☆☆

**Topik:**
- ✅ I²C bus basics (SDA, SCL, addressing)
- ✅ Device scanning & identification
- ✅ Popular I²C devices (OLED, RTC, EEPROM, sensors)
- ✅ Multi-master arbitration
- ✅ Clock stretching & error handling

**Contoh Program (30+ programs):**
- **Common:** I2C scanner, OLED display, DS3231 RTC, EEPROM, multi-sensor
- **STM32:** DMA I2C, dual-address slave, SMBus/PMBus, 1MHz Fast Mode Plus
- **ESP32:** Flexible pins, dual I2C bus, 256-byte buffer, slave mode

**Mini Project:** Environmental Monitoring Station (BME280 + OLED + RTC)

---

#### **BAB 8: SPI Protocol & High-Speed Transfer**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐⭐☆

**Topik:**
- ✅ SPI full-duplex communication
- ✅ Clock polarity & phase (CPOL/CPHA)
- ✅ Multiple slave management (CS)
- ✅ SD card FAT filesystem
- ✅ High-speed peripherals (TFT, Flash)

**Contoh Program (25+ programs):**
- **Common:** Basic transfer, SD card, NRF24L01 wireless, TFT display, external flash
- **STM32:** DMA circular, hardware NSS, SPI slave mode, CRC hardware
- **ESP32:** VSPI + HSPI, flexible pins, transaction queue, SDIO 4-bit, large transfers

**Mini Project:** Data Logger with TFT Display (sensors + SD + touchscreen)

---

#### **BAB 9: DMA & Memory Management**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐⭐☆

**Topik:**
- ✅ Direct Memory Access concepts
- ✅ DMA modes (normal, circular, M2M)
- ✅ Zero-CPU transfer optimization
- ✅ Memory architecture (SRAM, Flash, Cache)
- ✅ PSRAM & external memory (ESP32)

**Contoh Program (20+ programs):**
- **Common:** Buffer transfer, circular buffer, zero-copy
- **STM32:** UART/SPI/I2C/ADC DMA, M2M memcpy, DMAMUX, cache-coherent DMA
- **ESP32:** I2S DMA audio, PSRAM, RTC memory, WiFi zero-copy

**Mini Project:** High-Speed Data Acquisition System

---

### FASE 4: SISTEM ADVANCED (3 Pertemuan)

#### **BAB 10: Clock System & Timing Configuration**
**Durasi:** 2-3 jam | **Kesulitan:** ⭐⭐⭐☆☆

**Topik:**
- ✅ Clock tree architecture
- ✅ PLL configuration untuk max performance
- ✅ Clock gating untuk power saving
- ✅ RTC clock sources
- ✅ Dynamic frequency scaling

**Contoh Program (18+ programs):**
- **Common:** System clock config, peripheral clock enable, RTC setup
- **STM32:** Multiple PLL outputs, CSS (Clock Security System), MCO output
- **ESP32:** Dynamic frequency scaling, 8MHz low-power, WiFi-aware clocking

**Mini Project:** Power-Optimized Logger

---

#### **BAB 11: FreeRTOS - Multitasking Basics**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐⭐☆

**Topik:**
- ✅ Task creation & scheduling
- ✅ Priority & preemption
- ✅ Task states (running, ready, blocked, suspended)
- ✅ Delay & timing functions
- ✅ Idle & tick hooks

**Contoh Program (25+ programs):**
- **Common:** Task creation, delay, priority, suspend/resume, idle hook
- **STM32:** CMSIS-RTOS wrapper, static allocation, MPU support
- **ESP32:** Dual-core pinning, task watchdog, WiFi integration

**Mini Project:** Multi-Task Sensor System (5 concurrent tasks)

---

#### **BAB 12: FreeRTOS - IPC & Synchronization**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐⭐⭐

**Topik:**
- ✅ Queue untuk message passing
- ✅ Semaphore (binary, counting, mutex)
- ✅ Event groups untuk synchronization
- ✅ Stream buffers & message buffers
- ✅ Critical sections & race conditions

**Contoh Program (28+ programs):**
- **Common:** Queue, binary semaphore, mutex, counting semaphore, event groups
- **STM32:** CMSIS-RTOS wrappers, static IPC allocation
- **ESP32:** Cross-core queues, task notification, stream buffers

**Mini Project:** Producer-Consumer System

---

### FASE 5: APLIKASI PRAKTIS (2 Pertemuan)

#### **BAB 13: Power Management & Low-Power Design**
**Durasi:** 3-4 jam | **Kesulitan:** ⭐⭐⭐⭐☆

**Topik:**
- ✅ Low-power modes (sleep, stop, standby)
- ✅ Wake-up sources (RTC, EXTI, timers)
- ✅ Clock gating optimization
- ✅ ULP co-processor (ESP32)
- ✅ Battery monitoring & optimization

**Contoh Program (22+ programs):**
- **Common:** Sleep mode, deep sleep, clock gating, wake sources, battery monitor
- **STM32:** Multiple modes (Sleep/Stop/Standby), VBAT backup, dynamic voltage scaling
- **ESP32:** ULP co-processor, RTC memory persistence, WiFi power save, hibernation (5µA)

**Mini Project:** Solar-Powered Weather Station

---

#### **BAB 14: Wireless Connectivity & IoT Integration**
**Durasi:** 4-5 jam | **Kesulitan:** ⭐⭐⭐⭐⭐

**Topik:**
- ✅ WiFi basics (STA, AP, STA+AP)
- ✅ MQTT protocol untuk IoT
- ✅ HTTP client/server (REST API)
- ✅ WebSocket real-time
- ✅ Bluetooth Low Energy (BLE)
- ✅ Cloud platforms (AWS IoT, ThingSpeak, Firebase)

**Contoh Program (35+ programs):**
- **Common:** UART wireless modules, MQTT pub/sub, HTTP REST, WebSocket, cloud integration
- **STM32:** ESP8266/NRF24 integration, industrial protocols, secure boot
- **ESP32:** Native WiFi/BLE, ESP-NOW, OTA updates, WiFi provisioning, mesh network

**Mini Project:** Smart Home Gateway (multi-protocol IoT hub)

---

## 🎯 RINGKASAN PERBANDINGAN

### STM32 - Best For:
```
✅ Hard real-time systems
✅ Deterministic timing
✅ Safety-certified applications (automotive, medical)
✅ Industrial protocols (CAN, Modbus, Profinet)
✅ 5V sensor interfacing
✅ Ultra-low power (<1 µA standby)
✅ High-precision ADC (16-bit pada H7)
✅ DMA-heavy applications
```

### ESP32 - Best For:
```
✅ IoT & WiFi connectivity
✅ Rapid prototyping
✅ Web-based interfaces
✅ Bluetooth/BLE applications
✅ Dual-core processing
✅ Flexible GPIO mapping
✅ Large community & libraries
✅ Cost-effective projects
```

### Ideal Kombinasi: STM32 + ESP32
```
STM32: Real-time control, sensors, actuators, CAN bus
  ↕ UART/SPI
ESP32: WiFi gateway, cloud, web interface, BLE

Contoh: Industrial IoT Gateway
- STM32 handle critical timing & protocols
- ESP32 handle connectivity & user interface
```

---

## 📋 DAFTAR CONTOH PROGRAM PER BAB

### Format Program:
Setiap program mencakup:
1. **Judul Program** - Nama deskriptif
2. **Deskripsi Singkat** - Apa yang dilakukan program
3. **Target Hardware** - STM32 atau ESP32 atau Both
4. **Tingkat Kesulitan** - ⭐ (mudah) sampai ⭐⭐⭐⭐⭐ (advanced)
5. **Konsep yang Dipelajari** - Key learning points

### Total Program: **350+ Contoh**
- Common Ground: ~120 programs (berlaku untuk kedua platform)
- STM32 Unique: ~110 programs
- ESP32 Unique: ~120 programs

---

## 🛠️ TOOLS & SETUP REQUIREMENTS

### Software:
- ✅ PlatformIO Core/IDE (VSCode extension)
- ✅ Git (version control)
- ✅ Python 3.7+ (untuk ESP-IDF)
- ✅ Terminal emulator (PuTTY, Tera Term, atau VSCode terminal)

### Hardware Minimum:
**STM32:**
- STM32F103 Blue Pill / STM32F401 Black Pill / STM32 Nucleo
- ST-LINK V2 programmer (atau built-in pada Nucleo)
- USB-TTL adapter (untuk serial debugging)

**ESP32:**
- ESP32 DevKit v1 / ESP32-WROOM-32
- USB cable (data + power)
- Breadboard & jumper wires

### Hardware Recommended (untuk semua project):
- OLED Display 0.96" I2C (SSD1306)
- Sensors: BME280, DS3231 RTC, LDR, LM35
- SD Card module (SPI)
- TFT Display 2.4" (ILI9341) - optional
- LEDs, buttons, resistors, capacitors
- Breadboard & prototyping tools

---

## 📖 CARA PENGGUNAAN MODUL

### Untuk Dosen/Instruktur:
1. **Pertemuan 1**: Bab 1 (Architecture & Setup) - 3 jam
2. **Pertemuan 2**: Bab 2 (GPIO) - 3 jam
3. **Pertemuan 3**: Bab 3 (Interrupts) - 3 jam
4. ... dan seterusnya (14 pertemuan total)

### Untuk Mahasiswa/Self-Learner:
1. Baca **Penjelasan Materi** untuk understand konsep
2. Study **Common Ground** examples (berlaku untuk kedua platform)
3. Explore **STM32 Advantages** untuk deep-dive STM32-specific features
4. Explore **ESP32 Advantages** untuk ESP32-specific features
5. Bandingkan di **Comparison Table**
6. Implement **Mini Project** untuk hands-on practice

### Tips Belajar:
- ✅ Mulai dari Common Ground (kesamaan konsep)
- ✅ Jangan skip praktik hands-on
- ✅ Debug dengan serial monitor (printf debugging)
- ✅ Gunakan oscilloscope/logic analyzer untuk timing critical
- ✅ Join community (STM32 forum, ESP32 Reddit)
- ✅ Baca datasheet untuk deep understanding

---

## 🚀 NEXT STEPS

### Setelah Menyelesaikan 14 Bab:
1. **Advanced STM32:**
   - USB Device/Host
   - Ethernet & lwIP stack
   - FatFs advanced
   - Bootloader development

2. **Advanced ESP32:**
   - ESP-MESH networking
   - ESP-NOW protocols
   - TensorFlow Lite on ESP32
   - Matter protocol (smart home)

3. **Real Projects:**
   - Drone flight controller (STM32)
   - IoT weather station network (ESP32)
   - Industrial data logger (STM32 + ESP32)
   - Home automation gateway

---

## 📞 SUPPORT & RESOURCES

### Official Documentation:
- STM32: [st.com/stm32](https://www.st.com/stm32)
- ESP32: [espressif.com](https://www.espressif.com)
- PlatformIO: [platformio.org](https://platformio.org)

### Community:
- STM32 Forum: [community.st.com](https://community.st.com)
- ESP32 Forum: [esp32.com](https://www.esp32.com)
- r/stm32 & r/esp32 (Reddit)

---

**Versi:** 1.0
**Tanggal:** 6 Februari 2026
**Author:** Compiled for embedded systems education
**License:** Educational use

---

**Selamat Belajar! Happy Coding! 🎓🚀**
