# Referensi
## Modul 02: Interrupt dan Timer

---

## 📚 Dokumentasi Resmi

### STM32
1. **STM32F103C8 Reference Manual (RM0008)**
   - URL: https://www.st.com/resource/en/reference_manual/rm0008.pdf
   - Chapter 10: Interrupts and events
   - Chapter 13-15: General-purpose timers
   - Chapter 18: Independent watchdog (IWDG)
   - Chapter 19: Window watchdog (WWDG)
   - Wajib baca untuk pemahaman mendalam register-level

2. **STM32F103C8 Datasheet**
   - URL: https://www.st.com/resource/en/datasheet/stm32f103c8.pdf
   - Pinout dan electrical characteristics
   - Timer specifications dan limitations

3. **ARM Cortex-M3 Technical Reference Manual**
   - URL: https://developer.arm.com/documentation/ddi0337/
   - NVIC (Nested Vectored Interrupt Controller)
   - Exception handling dan priority

4. **STM32 Application Notes**
   - AN4228: STM32 EXTI Application Note
     - URL: https://www.st.com/resource/en/application_note/an4228.pdf
   - AN4776: Timer cookbook for STM32 microcontrollers
     - URL: https://www.st.com/resource/en/application_note/an4776.pdf
   - AN4013: Timer overview and timer firmware
     - URL: https://www.st.com/resource/en/application_note/an4013.pdf

5. **STM32Cube HAL Driver Documentation**
   - URL: https://www.st.com/resource/en/user_manual/um1850.pdf
   - HAL_GPIO_EXTI functions
   - HAL_TIM functions
   - HAL_IWDG / HAL_WWDG functions

### ESP32
1. **ESP32 Technical Reference Manual**
   - URL: https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
   - Chapter 5: Interrupts
   - Chapter 17: Timer Group
   - Wajib baca untuk understanding hardware timer

2. **ESP-IDF Programming Guide**
   - GPIO Interrupt: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html
   - GPTimer: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gptimer.html
   - Interrupt Allocation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/intr_alloc.html
   - Task Watchdog Timer: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html
   - PCNT (Pulse Counter): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/pcnt.html

3. **ESP32 Datasheet**
   - URL: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
   - GPIO specifications
   - Timer specifications

---

## 📖 Buku Referensi

### Wajib
1. **Mastering STM32** - Carmine Noviello
   - Publisher: Leanpub
   - URL: https://leanpub.com/mastering-stm32
   - **Chapter 7: Interrupts** — NVIC, EXTI, interrupt handling dengan HAL
   - **Chapter 11: Timers** — General-purpose timers, PWM, Input Capture, Output Compare
   - Chapter 12: DMA (relevan untuk timer DMA)
   - Penjelasan praktis dengan contoh code STM32Cube HAL

2. **The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors** - Joseph Yiu
   - Publisher: Newnes (3rd Edition)
   - ISBN: 978-0124080829
   - Chapter 7: Exceptions — Exception model, vector table
   - Chapter 8: NVIC — Priority configuration, nested interrupts
   - Referensi standar untuk ARM architecture

3. **Kolban's Book on ESP32** - Neil Kolban
   - URL: https://leanpub.com/kolban-ESP32
   - **p267-268: Interrupt Service Routines (ISR)** — GPIO interrupt, IRAM_ATTR
   - **p300-302: Timers** — Hardware timer configuration, alarm
   - Comprehensive ESP32 reference book

### Pendukung
4. **Programming with STM32: Getting Started with the Nucleo Board and C/C++** - Donald Norris
   - Publisher: McGraw-Hill
   - ISBN: 978-1260031317
   - STM32Cube HAL examples

5. **Beginning STM32: Developing with FreeRTOS, libopencm3, and GCC** - Warren Gay
   - Publisher: Apress
   - ISBN: 978-1484236239

---

## 🔗 Tutorial Online

### STM32 Tutorials (HAL-based)
1. **DeepBlue Embedded - STM32 HAL Tutorials**
   - URL: https://deepbluembedded.com/stm32-tutorials/
   - STM32 External Interrupt HAL Example
   - STM32 Timers HAL Tutorial
   - STM32 Input Capture / Output Compare

2. **Controllers Tech - STM32 HAL Series**
   - URL: https://controllerstech.com/stm32-tutorial/
   - Timer, EXTI, Watchdog tutorials
   - Video + text format

3. **Embedded Systems Programming on ARM Cortex-M3/M4**
   - URL: https://www.udemy.com/course/embedded-system-programming-on-arm-cortex-m3m4/
   - Interrupt dan timer detail

### ESP32 Tutorials (ESP-IDF based)
1. **ESP-IDF Official Examples**
   - URL: https://github.com/espressif/esp-idf/tree/master/examples
   - `peripherals/gpio/` — GPIO interrupt examples
   - `peripherals/timer_group/` — GPTimer examples
   - `system/task_watchdog/` — Task WDT examples

2. **ESP-IDF Getting Started**
   - URL: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   - Setup, build system, component architecture

---

## 📄 Paper & Artikel Ilmiah

1. **"A Survey on Interrupt Handling in Real-Time Operating Systems"**
   - Journal of Systems Architecture
   - DOI: 10.1016/j.sysarc.2020.101813

2. **"Real-Time Response in Embedded Systems"**
   - ACM Computing Surveys
   - DOI: 10.1145/3340555

3. **"Comparison of STM32 and ESP32 for IoT Applications"**
   - IEEE IoT Conference 2021
   - Performance comparison kedua platform

---

## 🎥 Video Tutorial

### YouTube Channels
1. **Controllers Tech** — STM32 Timer & Interrupt series (HAL-based)
   - URL: https://www.youtube.com/c/ControllersTech
2. **Phil's Lab** — Professional embedded tutorials
   - URL: https://www.youtube.com/c/PhilsLab
3. **Andreas Spiess** — ESP32 tutorials
   - URL: https://www.youtube.com/c/AndreasSpiess

---

## 🛠️ Tools & Software

### Development Environment
1. **PlatformIO IDE** — https://platformio.org/
   - Framework: `stm32cube` (STM32) dan `espidf` (ESP32)
2. **VS Code** + PlatformIO Extension
3. **ESP-IDF Extension for VS Code** — https://github.com/espressif/vscode-esp-idf-extension

### Debugging
1. **ST-Link Utility** — Programming dan debugging STM32
2. **Wokwi ESP32 Simulator** — https://wokwi.com/ (online simulator)

---

## 📋 Quick Reference

### Timer Calculation Formulas
- **STM32**: `Timer_Freq = APB_Clock / (PSC + 1)`, `Period = (ARR + 1) / Timer_Freq`
- **ESP32**: `Resolution = APB_Clock / prescaler`, `Period = alarm_count / resolution`

### Watchdog Timeout Formulas
- **STM32 IWDG**: `Timeout = (Prescaler × Reload) / LSI_freq` (LSI ≈ 40kHz)
- **ESP32 TWDT**: `Timeout = timeout_ms` (configurable via `esp_task_wdt_config_t`)

---

## 📞 Community & Support

1. **STM32 Community Forum** — https://community.st.com/
2. **ESP32 Forum** — https://www.esp32.com/
3. **Stack Overflow** — Tags: [stm32], [esp32], [esp-idf], [stm32-hal]

---

## ⚠️ Catatan Penting

1. **Dokumentasi resmi SELALU menjadi sumber utama** — datasheet dan reference manual adalah authoritative source
2. **Gunakan framework native**: STM32Cube HAL untuk STM32, ESP-IDF untuk ESP32 (bukan Arduino)
3. **Versi library berubah** — selalu cek compatibility dengan versi yang digunakan
4. **Kolban p267-268 dan p300-302** adalah referensi penting untuk memahami ISR dan timer di ESP32
5. **Mastering STM32 Ch7 dan Ch11** adalah referensi utama untuk interrupt dan timer di STM32
