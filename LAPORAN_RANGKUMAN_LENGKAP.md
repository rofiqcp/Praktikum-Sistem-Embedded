# 📋 LAPORAN RANGKUMAN LENGKAP — Praktikum Sistem Embedded

> **Tanggal**: 8 Februari 2026  
> **Total Modul**: 14 Modul  
> **Platform**: ESP32 (ESP-IDF) & STM32 (HAL/STM32Cube)  
> **Board**: ESP32 DevKit / Lolin S2 Mini / ESP32-S3 & Blue Pill F103 / Black Pill F401 / F411

---

## 📚 RINGKASAN PER MODUL

### Modul 01: GPIO dan Digital I/O
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Konfigurasi pin GPIO (input/output), mode push-pull/open-drain, pull-up/pull-down, debouncing, akses register langsung (BSRR/GPIO_OUT_REG) |
| **STM32** | 8 mode GPIO via CRL/CRH, 3 speed setting, HAL GPIO API, BSRR atomik |
| **ESP32** | gpio_config(), drive strength, GPIO matrix |
| **Praktikum** | 12 program: LED blink, running LED, binary counter, button debounce, long/short press, toggle latch, drive strength, DIP switch, port register, matrix keypad, emergency stop, test pattern |

### Modul 02: Interrupt dan Timer
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | NVIC/EXTI (STM32), Interrupt Matrix (ESP32), Timer/Counter, Watchdog (IWDG/WWDG), Timer Cascade |
| **STM32** | 16 EXTI lines, 4-bit priority, TIM1-TIM4 (16-bit), HAL callback pattern |
| **ESP32** | Dual-core interrupt, IRAM_ATTR, gpio_isr_handler, FreeRTOS queue dari ISR |
| **Praktikum** | 12 program: EXTI, debounce, timer periodic, one-shot, PWM basic, watchdog, cascade, output compare, input capture, encoder, multiple timers, NVIC priority |

### Modul 03: Serial UART
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Frame UART (start/data/parity/stop), baud rate, konfigurasi 8N1, UART interrupt, ring buffer, protokol komunikasi |
| **STM32** | 3 USART (PA9/PA10, PA2/PA3, PB10/PB11), BRR register, HAL UART API |
| **ESP32** | 3 UART (UART0=USB, UART1, UART2), driver/uart.h API |
| **Praktikum** | 12 program: echo, interrupt RX, ring buffer, printf redirect, command parser, JSON protocol, line editor, STX/ETX framing, CRC checksum, timeout parser, bridge multi, error statistics |

### Modul 04: ADC (Analog-to-Digital Converter)
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Sampling/Quantization/Encoding, Nyquist theorem, resolusi 12-bit, kalibrasi, mode operasi (single/continuous/scan/injected) |
| **STM32** | 2 ADC unit (F103), resolusi 6-12 bit selectable (F4), analog watchdog, DMA support |
| **ESP32** | ADC1 (GPIO32-39) + ADC2 (konflik WiFi!), attenuation 0-11dB, kalibrasi eFuse, API legacy vs oneshot |
| **Praktikum** | 13 program: single read, voltage display, moving average, multi-channel, kalibrasi, continuous DMA, threshold alert, battery monitor, temp internal, sampling rate, light sensor, statistical analysis, analog watchdog/attenuation |

### Modul 05: DAC dan PWM
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | DAC (R-2R ladder, delta-sigma), PWM (duty cycle, frekuensi), filter RC untuk analog output |
| **STM32** | 2 channel DAC 12-bit (PA4/PA5), DMA trigger, TIM PWM (advanced/general purpose) |
| **ESP32** | 2 channel DAC 8-bit (GPIO25/26), cosine wave generator, LEDC PWM (16 channel, 20-bit resolution) |
| **Praktikum** | 13 program: DAC voltage output, sine/triangle wave, audio tone, PWM LED breathing/brightness, servo, freq sweep, motor speed, RGB LED, buzzer melody, DAC vs PWM compare, hardware waveform/LEDC fade |

### Modul 06: I2C dan Sensor Integration
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Protokol I2C (SDA/SCL), 7-bit addressing, ACK/NACK, speed modes (100k-3.4M), pull-up resistor |
| **STM32** | 2 I2C (I2C1: PB6/PB7, I2C2: PB10/PB11), HAL I2C API, DMA support |
| **ESP32** | 2 I2C controller, flexible GPIO mapping, I2C driver API |
| **Sensor** | BME280, DS3231 RTC, SSD1306 OLED, MPU6050, AT24C32 EEPROM, BH1750, PCF8574 LCD |
| **Praktikum** | 13 program: scanner, OLED, BMP280, MPU6050, EEPROM, RTC, BH1750 light, LCD, multi sensor, raw R/W, clock speed test, error recovery, DMA/GPIO matrix |

### Modul 07: SPI dan Storage
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Protokol SPI (MOSI/MISO/SCLK/CS), 4 clock mode (CPOL/CPHA), SD Card SPI mode, Flash W25Q, file system |
| **STM32** | SPI1 (APB2 max 36MHz), SPI2 (APB1 max 18MHz), HAL SPI, internal Flash R/W |
| **ESP32** | HSPI (SPI2) + VSPI (SPI3), max 80MHz, NVS, SPIFFS, LittleFS |
| **Praktikum** | 12 program: loopback, SPI OLED, Flash W25Q32, SD card, MCP3208 ADC, MCP4921 DAC, multi slave, flash R/W, NVS/key-value, speed benchmark, interrupt mode, data logger |

### Modul 08: DMA (Direct Memory Access)
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Transfer Memory↔Memory/Peripheral, Normal vs Circular mode, Double Buffering (Ping-Pong), half/full complete callback |
| **STM32** | DMA1 (7 channel) hardwired mapping, priority arbiter, TC/HT/TE interrupts |
| **ESP32** | Peripheral-bound DMA (I2S, SPI, ADC), Linked List Descriptors, GDMA (S3/C3) |
| **Praktikum** | 12 program: mem-to-mem, UART TX/RX, ADC continuous/multi-ch, SPI transfer, circular buffer, double buffer, I2C transfer, DAC waveform, benchmark, linked list |

### Modul 09: FreeRTOS Task Management
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | RTOS vs bare-metal, Task state (Ready/Running/Blocked/Suspended), priority-based preemptive scheduling, stack sizing, vTaskDelay vs vTaskDelayUntil |
| **API** | xTaskCreate, vTaskDelay, vTaskDelayUntil, vTaskSuspend/Resume, vTaskDelete, uxTaskGetStackHighWaterMark |
| **Praktikum** | 12 program: create basic, priority, delay periodic, suspend/resume, delete, stack monitor, priority inversion/core affinity, idle hook, watchdog, communication, scheduler info, cooperative |

### Modul 10: FreeRTOS Queue dan Semaphore
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Queue (FIFO), Binary/Counting Semaphore, Mutex, race condition, data corruption prevention |
| **API** | xQueueCreate/Send/Receive, xSemaphoreCreateBinary/Counting, xSemaphoreCreateMutex, priority inheritance |
| **Praktikum** | 12 program: queue basic, queue struct, queue multiple, queue ISR, queue set, binary semaphore, counting semaphore, mutex shared resource, mutex priority inversion, recursive mutex, producer-consumer, reader-writer |

### Modul 11: FreeRTOS Timer dan Notification
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Software Timer (one-shot/auto-reload), Timer Daemon Task, Task Notification (45% lebih cepat dari semaphore), Event Group |
| **API** | xTimerCreate/Start/Stop/Reset, xTaskNotifyGive/Wait, xEventGroupCreate/SetBits/WaitBits |
| **Praktikum** | 12 program: SW timer basic, period change, timer ID, debounce, timeout monitor, notification basic/value/counting, notification ISR, event group basic/sync, benchmark |

### Modul 12: FreeRTOS Memory Management dan Advanced
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | 5 skema heap (heap_1 to heap_5), stack overflow detection, static allocation, memory pool, stream/message buffer, critical section, fragmentasi |
| **ESP32** | Multi-heap (DRAM/IRAM/PSRAM), heap_caps_malloc, DMA-capable memory |
| **STM32** | 20KB SRAM (F103), linker script, CCM memory |
| **Praktikum** | 12 program: heap monitor, memory allocation, stack overflow detect, static allocation, memory pool, stream buffer, message buffer, critical section, heap fragmentation, PSRAM/external RAM, memory leak detection, system dashboard |

### Modul 13: Network dan IoT
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | WiFi (STA/AP), TCP/UDP, HTTP/REST API, MQTT, BLE, WebSocket, TCP/IP model, IoT architecture |
| **ESP32** | WiFi built-in, BLE built-in, lwip sockets, esp_http, esp_mqtt |
| **STM32** | External module: ESP-01 (AT command), W5500 Ethernet, HM-10 BLE |
| **Praktikum** | 12 program per platform: ESP32 (WiFi scan/STA/AP, TCP, UDP, HTTP server/client, MQTT, BLE adv/GATT, WebSocket, IoT dashboard) — STM32 (AT command, ESP-01 WiFi/TCP/HTTP, W5500 Ethernet init/TCP/UDP/HTTP, UART bridge, HM-10 BLE, MQTT, IoT gateway) |

### Modul 14: Power Management
| Aspek | Detail |
|-------|--------|
| **Topik Utama** | Sleep modes (Light/Deep/Hibernate/Standby), wakeup sources, RTC memory, Dynamic Frequency Scaling, ULP coprocessor, battery life calculation |
| **ESP32** | Active (80-260mA), Modem Sleep (20-30mA), Light Sleep (0.8mA), Deep Sleep (10-150μA), Hibernation (2.5-5μA) |
| **STM32** | Run (~30mA), Sleep (~10-15mA), Stop (~20μA), Standby (~2μA) |
| **Praktikum** | 12 program: current measurement, light sleep, deep sleep timer/GPIO/RTC, data retention, touch wakeup, ULP, dynamic frequency, peripheral power gate, battery logger, power budget analysis |

---

## 🔍 ANALISIS MATERI YANG OVERLAPPING

### 1. **Debouncing** — Dibahas 3× (terlalu banyak pengulangan)
| Modul | Konteks |
|-------|---------|
| Modul 01 (GPIO) | Software debouncing, hardware RC filter (teori + praktik) |
| Modul 02 (Interrupt) | Debouncing berbasis interrupt + timer |
| Modul 11 (FreeRTOS Timer) | Debouncing menggunakan software timer |

> **Rekomendasi**: Penjelasan teori debouncing cukup **di Modul 01** saja. Modul 02 dan 11 cukup merujuk "lihat Modul 01" dan fokus pada implementasi dengan interrupt/timer.

### 2. **PWM** — Dibahas 2× (overlap signifikan)
| Modul | Konteks |
|-------|---------|
| Modul 02 (Interrupt/Timer) | Timer PWM basic — praktikum "STM32_05_Timer_PWM_Basic" |
| Modul 05 (DAC/PWM) | PWM lengkap: LED dimming, servo, motor, RGB, buzzer |

> **Rekomendasi**: Modul 02 praktikum PWM basic sebaiknya **dihapus atau dijadikan preview**, dan semua PWM terkonsolidasi di Modul 05.

### 3. **Watchdog Timer** — Dibahas 3×
| Modul | Konteks |
|-------|---------|
| Modul 02 (Interrupt/Timer) | IWDG/WWDG dan esp_task_wdt |
| Modul 09 (FreeRTOS Task) | Task Watchdog di FreeRTOS context |
| Buku Mastering STM32 | Chapter terpisah (IWDG/WWDG) |

> **Rekomendasi**: Watchdog hardware di Modul 02, RTOS watchdog di Modul 09. Tidak perlu diubah, konteksnya berbeda.

### 4. **DMA + ADC** — Praktikum duplikat
| Modul | Praktikum |
|-------|-----------|
| Modul 04 (ADC) | STM32_06_ADC_Continuous_DMA, ESP32_06_ADC_Continuous_DMA |
| Modul 08 (DMA) | STM32_04_DMA_ADC_Continuous, STM32_05_DMA_ADC_Multi_Channel |

> **Rekomendasi**: Modul 04 cukup ADC basic + kalibrasi. DMA-based ADC dipindah sepenuhnya ke Modul 08.

### 5. **I2C DMA / SPI DMA** — Overlap dengan Modul 08
| Modul | Praktikum |
|-------|-----------|
| Modul 06 (I2C) | STM32_13_I2C_DMA_Transfer |
| Modul 07 (SPI) | Teori SPI+DMA dibahas |
| Modul 08 (DMA) | DMA SPI Transfer, DMA I2C Transfer |

> **Rekomendasi**: DMA transfer untuk I2C/SPI cukup di Modul 08 saja.

### 6. **FreeRTOS Queue dari ISR** — Dibahas 2×
| Modul | Konteks |
|-------|---------|
| Modul 02 (Interrupt) | ESP32 ISR handler mengirim ke queue (contoh kode lengkap) |
| Modul 10 (Queue/Semaphore) | Queue dari ISR (xQueueSendFromISR) |

> **Rekomendasi**: Modul 02 ESP32 boleh tetap sebagai preview/teaser, tapi penjelasan mendalam queue ada di Modul 10.

### 7. **Priority Inversion** — Dibahas 2×
| Modul | Konteks |
|-------|---------|
| Modul 09 (FreeRTOS Task) | STM32_07_Task_Priority_Inversion |
| Modul 10 (Queue/Semaphore) | STM32_09_Mutex_Priority_Inversion |

> **Rekomendasi**: Tetap di kedua tempat karena konteksnya berbeda (task scheduling vs mutex inheritance).

---

## 🔌 REKOMENDASI PENEMPATAN MATERI USB

### Analisis
Berdasarkan buku **Mastering STM32 (2nd Ed), Chapter "Universal Serial Bus" (hal. 795-871)**, materi USB meliputi:
- USB 2.0 protocol fundamentals
- STM32 USB Device Library
- USB CDC (Virtual COM Port)
- USB HID (keyboard/mouse)
- USB Mass Storage
- Custom USB devices

### ✅ Rekomendasi: **Masukkan sebagai Modul 03B atau Sub-bab di Modul 03**

**Alasan:**
1. USB CDC (Virtual COM Port) adalah **evolusi natural dari Serial UART** — mahasiswa sudah paham konsep serial di Modul 03
2. USB sebagai komunikasi serial modern menggantikan USB-to-UART converter chip
3. STM32F411/F401 memiliki USB OTG bawaan (Blue Pill F103 juga punya USB)
4. ESP32-S2/S3 memiliki USB native (CDC + HID)

### 📍 Opsi Penempatan:

| Opsi | Posisi | Pro | Kontra |
|------|--------|-----|--------|
| **A (Recommended)** | **Sub-modul di Modul 03** — tambahkan setelah UART | Natural flow dari serial → USB serial | Modul 03 menjadi lebih panjang |
| **B** | **Modul baru 03B** antara UART dan ADC | Modul terpisah, fokus | Menggeser nomor modul 04-14 |
| **C** | **Setelah Modul 07 (SPI)** — semua komunikasi selesai | Setelah semua peripheral comm | Terlalu jauh dari konteks serial |

### 📋 Materi USB yang Direkomendasikan:

```
Modul 03 (diperluas): Serial Communication — UART & USB
├── Bagian 1: UART (existing)
├── Bagian 2: USB Fundamentals
│   ├── USB Protocol Overview (endpoints, descriptors)
│   ├── USB Device Classes (CDC, HID, MSC)
│   └── USB vs UART comparison
├── Bagian 3: USB CDC pada STM32
│   ├── STM32F103 USB (Full-Speed, 12 Mbps)
│   ├── STM32F401/F411 USB OTG (Full-Speed)
│   └── Virtual COM Port implementation
├── Bagian 4: USB pada ESP32
│   ├── ESP32-S2/S3 native USB (CDC + HID)
│   ├── TinyUSB framework
│   └── USB Host capability (S2/S3)
└── Praktikum USB
    ├── USB_01_CDC_Virtual_COM_Port
    ├── USB_02_USB_HID_Keyboard
    ├── USB_03_USB_Mass_Storage
    └── USB_04_USB_Custom_Device
```

---

## 📖 MATERI YANG BELUM DISAMPAIKAN (dari Referensi)

### Dari **Mastering STM32 (2nd Ed.)**:

| No | Topik | Chapter | Status |
|----|-------|---------|--------|
| 1 | **USB (Universal Serial Bus)** | Ch. USB (p.795) | ❌ **BELUM** — sangat penting |
| 2 | **Clock Tree Management** | Ch. Clock Tree (p.275) | ⚠️ Hanya disinggung, belum modul sendiri |
| 3 | **CRC (Cyclic Redundancy Check) Hardware** | Ch. CRC (p.455) | ⚠️ Ada di Modul 03 (software CRC), tapi bukan hardware CRC peripheral |
| 4 | **RTC (Real-Time Clock)** | Ch. RTC (p.471) | ⚠️ Ada di Modul 06 (DS3231 via I2C), tapi **internal RTC STM32 belum** |
| 5 | **Flash Memory Management** | Ch. Flash (p.564) | ⚠️ Ada di Modul 07 (external flash), tapi **internal flash STM32 belum detail** |
| 6 | **Booting Process & Bootloader** | Ch. Booting (p.588) | ❌ **BELUM** |
| 7 | **Advanced Debugging (SWV, Fault analysis)** | Ch. Debug (p.703) | ❌ **BELUM** |
| 8 | **FAT Filesystem (FatFs)** | Ch. FAT (p.750) | ⚠️ Disinggung di Modul 07 tapi belum detail |
| 9 | **Memory Layout & MPU** | Ch. Memory (p.520) | ⚠️ Disinggung di Modul 12 tapi belum detail |

### Dari **kolban-ESP32 Book**:

| No | Topik | Section | Status |
|----|-------|---------|--------|
| 1 | **ESP-NOW (peer-to-peer tanpa WiFi)** | — | ❌ **BELUM** |
| 2 | **ESP-Mesh Networking** | Mesh (p.484) | ❌ **BELUM** |
| 3 | **Camera Interface** | Hardware (p.250+) | ❌ **BELUM** |
| 4 | **I2S (Inter-IC Sound)** | Hardware (p.250+) | ❌ **BELUM** |
| 5 | **RMT (Remote Control)** | Hardware (p.250+) | ❌ **BELUM** — berguna untuk NeoPixel, IR |
| 6 | **OTA (Over-The-Air Update)** | — | ❌ **BELUM** |
| 7 | **MicroPython** | Programming (p.432) | ❌ **BELUM** |
| 8 | **Partition Table** | Storage (p.485) | ❌ **BELUM** |
| 9 | **NVS (detail)** | Storage (p.485) | ⚠️ Ada di Modul 07 tapi ringkas |
| 10 | **Security (Secure Boot, Flash Encryption)** | — | ❌ **BELUM** |
| 11 | **ULP detail (instruction set)** | ULP (p.579) | ⚠️ Disinggung di Modul 14 tapi belum detail |

### Topik yang Sama Sekali Belum Ada di Kedua Buku maupun Modul:

| No | Topik | Relevansi |
|----|-------|-----------|
| 1 | **CAN Bus** | Penting untuk automotive/industrial |
| 2 | **Ethernet (RMII/MII)** | Untuk STM32F4 dengan PHY |
| 3 | **Display interface (parallel, LVGL)** | GUI untuk embedded |
| 4 | **Motor control (FOC, BLDC)** | Industrial application |
| 5 | **DSP (Digital Signal Processing)** | Audio, vibration analysis |

---

## 📊 PETA MATERI LENGKAP

```
Modul 01: GPIO ─────────────────────────── ✅ Fundamental
Modul 02: Interrupt & Timer ────────────── ✅ Fundamental  
Modul 03: UART ─────────────────────────── ✅ Komunikasi   ← 🔌 USB masuk di sini
Modul 04: ADC ──────────────────────────── ✅ Analog Input
Modul 05: DAC & PWM ───────────────────── ✅ Analog Output
Modul 06: I2C ──────────────────────────── ✅ Bus Komunikasi
Modul 07: SPI & Storage ───────────────── ✅ Bus + Storage
Modul 08: DMA ──────────────────────────── ✅ Efisiensi Transfer
Modul 09: FreeRTOS Task ───────────────── ✅ RTOS Basic
Modul 10: FreeRTOS Queue/Semaphore ─────── ✅ RTOS Sync
Modul 11: FreeRTOS Timer/Notification ──── ✅ RTOS Advanced
Modul 12: FreeRTOS Memory/Advanced ─────── ✅ RTOS Expert
Modul 13: Network & IoT ──────────────── ✅ Konektivitas
Modul 14: Power Management ────────────── ✅ Optimasi Daya
```

---

## 🔧 STATUS PLATFORMIO.INI

### Format yang Ditemukan:

| Modul | ESP32 Format | STM32 Format | Extra Scripts | FreeRTOS |
|-------|-------------|-------------|---------------|----------|
| 01-03 | Old (no [env]) | Old (no [env]) | F103 only (stub) | Tidak |
| 04-06 | New ([env] shared) | New ([env] shared) | **TIDAK ADA** ⚠️ | Tidak |
| 07-08 | Old | Old | F103 only (stub) | Tidak |
| 09-12 | Old | Old | F103 only (FreeRTOS) | **YA** |
| 13-14 | Old | Old | F103 + F4xx (FreeRTOS) | **YA** |

### Masalah yang Ditemukan:
1. **Format tidak konsisten** — campuran old style dan new style
2. **Modul 04-06 STM32 tidak punya extra_script** padahal F103 butuhnya
3. **Modul 01-08 F4xx tidak punya extra_script** — jika nanti butuh FreeRTOS akan error
4. **FreeRTOS modules (09-14)** butuh extra_script untuk compile FreeRTOS sources
5. **Non-FreeRTOS modules (01-08)** F103 cukup stub, F4xx tidak perlu extra_script
