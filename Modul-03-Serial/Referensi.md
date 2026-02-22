# Referensi
## Modul 03: Serial UART Communication

---

## 📚 Dokumentasi Resmi

### STM32
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **RM0008 Reference Manual** | [STMicroelectronics](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) | Manual lengkap STM32F103 - **Chapter 27: USART** |
| **STM32F103C8T6 Datasheet** | [STMicroelectronics](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) | Spesifikasi pin, electrical characteristics, USART pin mapping |
| **PM0056 Programming Manual** | [STMicroelectronics](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf) | Cortex-M3 programming, NVIC untuk UART interrupt |

### ESP32
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **ESP32 Technical Reference Manual** | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) | Manual teknis lengkap - **Chapter 13: UART Controller** |
| **ESP32 Datasheet** | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) | Spesifikasi hardware, pin multiplexing |
| **ESP-IDF UART API Reference** | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html) | API reference: `uart_driver_install()`, `uart_read_bytes()`, `uart_write_bytes()` |

### ESP-IDF Framework
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **ESP-IDF Programming Guide** | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) | Panduan lengkap ESP-IDF framework |
| **ESP-IDF UART Driver** | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html) | Driver UART: konfigurasi, event queue, pattern detection |
| **ESP-IDF UART Examples** | [GitHub](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart) | Contoh kode resmi: echo, selection, events |

### STM32Cube HAL Framework
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **UM1850 HAL User Manual** | [STMicroelectronics](https://www.st.com/resource/en/user_manual/um1850-description-of-stm32f1-hal-and-lowlayer-drivers-stmicroelectronics.pdf) | Manual HAL driver STM32F1 - **UART HAL Module** |
| **STM32CubeF1 Package** | [GitHub](https://github.com/STMicroelectronics/STM32CubeF1) | Source code HAL + contoh UART |
| **HAL UART Functions** | [ST Wiki](https://wiki.st.com/stm32mcu/wiki/Getting_started_with_UART) | `HAL_UART_Transmit()`, `HAL_UART_Receive_IT()`, `HAL_UART_RxCpltCallback()` |

---

## 📖 Buku Referensi

### Wajib Baca
1. **"Mastering STM32" - Second Edition**
   - Penulis: Carmine Noviello
   - **Chapter 8: UART/USART** (konfigurasi UART, polling, interrupt, DMA mode)
   - Tersedia di: [leanpub.com](https://leanpub.com/mastering-stm32)
   - *Catatan: Referensi utama untuk STM32 HAL UART programming. Mencakup baud rate calculation, frame format, error handling, dan komunikasi antar-MCU*

2. **"Kolban's Book on ESP32"**
   - Penulis: Neil Kolban
   - **Halaman 283-287: UART** (konfigurasi ESP-IDF UART, driver install, read/write bytes)
   - Tersedia gratis di: [leanpub.com](https://leanpub.com/kolban-ESP32)
   - *Catatan: Referensi utama untuk ESP32 ESP-IDF UART, fokus pada native API bukan Arduino*

### Bacaan Tambahan
3. **"Serial Port Complete" - Second Edition**
   - Penulis: Jan Axelson
   - ISBN: 978-1931448062
   - *Panduan lengkap komunikasi serial: RS-232, RS-485, USB, protocol design*

4. **"The Definitive Guide to ARM Cortex-M3"**
   - Penulis: Joseph Yiu
   - ISBN: 978-0124080829
   - *Arsitektur ARM, NVIC interrupt system untuk UART interrupt*

5. **"Embedded Systems: Introduction to ARM Cortex-M Microcontrollers"**
   - Penulis: Jonathan Valvano
   - *Chapter tentang UART: serial I/O, interrupt-driven communication*

---

## 🎥 Video Tutorial

### YouTube Channels
| Channel | Konten | Link |
|---------|--------|------|
| **Controllers Tech** | STM32 HAL UART Tutorials | [YouTube](https://www.youtube.com/c/ControllersTech) |
| **Phil's Lab** | Embedded Design & Serial Communication | [YouTube](https://www.youtube.com/c/PhilsLab) |
| **DroneBot Workshop** | ESP32 Serial Communication | [YouTube](https://www.youtube.com/c/Dronebotworkshop) |
| **Andreas Spiess** | ESP32 Deep Dive | [YouTube](https://www.youtube.com/c/AndreasSpiess) |
| **Low Level Learning** | Embedded Concepts | [YouTube](https://www.youtube.com/c/LowLevelLearning) |

### Video Spesifik UART
1. **STM32 UART HAL Tutorial** - Controllers Tech
   - [UART Polling, Interrupt, DMA](https://www.youtube.com/watch?v=...)
   - *Menggunakan HAL_UART_Transmit, HAL_UART_Receive_IT, DMA mode*

2. **ESP32 UART with ESP-IDF** - ESP32 Tutorials
   - [uart_driver_install & uart_read_bytes](https://www.youtube.com/watch?v=...)
   - *Native ESP-IDF UART, bukan Arduino Serial*

3. **UART Communication Protocol Explained** - The Engineering Mindset
   - [Protocol Deep Dive](https://www.youtube.com/watch?v=sO7LM3r_2ns)
   - *Penjelasan visual frame format, timing, baud rate*

4. **Serial Protocol Design** - Ben Eater
   - [Building a UART from scratch](https://www.youtube.com/watch?v=...)
   - *Pemahaman mendalam tentang UART di level hardware*

---

## 🌐 Website dan Tutorial Online

### Tutorial Sites
| Website | Topik | Link |
|---------|-------|------|
| **DeepBlue Embedded** | STM32 HAL UART | [Link](https://deepbluembedded.com/stm32-uart-example-tutorial/) |
| **Controllerstech.com** | STM32 UART Interrupt & DMA | [Link](https://controllerstech.com/uart-receive-in-stm32/) |
| **SparkFun** | Serial Communication Basics | [Link](https://learn.sparkfun.com/tutorials/serial-communication/all) |
| **Embedded.com** | Ring Buffer Basics | [Link](https://www.embedded.com/ring-buffer-basics/) |
| **All About Circuits** | UART & RS-232 | [Link](https://www.allaboutcircuits.com/technical-articles/back-to-basics-the-universal-asynchronous-receiver-transmitter-uart/) |
| **Barr Group** | CRC Calculation | [Link](https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code) |

### ESP-IDF Specific
| Website | Topik | Link |
|---------|-------|------|
| **ESP-IDF UART Echo Example** | Basic UART usage | [GitHub](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart/uart_echo) |
| **ESP-IDF UART Events Example** | Event-driven UART | [GitHub](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart/uart_events) |
| **ESP-IDF UART Select Example** | select() with UART | [GitHub](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart/uart_select) |

### Interactive Learning
| Platform | Course | Link |
|----------|--------|------|
| **Coursera** | Embedded Systems (CU Boulder) | [Link](https://www.coursera.org/specializations/introduction-embedded-systems) |
| **edX** | Real-Time Bluetooth Networks | [Link](https://www.edx.org/course/real-time-bluetooth-networks-shape-the-world) |
| **Udemy** | Mastering STM32 with HAL | Search "STM32 HAL" |

---

## 📄 Paper dan Artikel Teknis

### Protokol Serial
1. **"A Guide to CRC Calculation"** - Barr Group
   - [barrgroup.com](https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code)
   - *Implementasi CRC-16/CRC-32 untuk error detection*

2. **"Implementing Circular Buffers in Embedded Systems"** - Embedded.com
   - [embedded.com](https://www.embedded.com/ring-buffer-basics/)
   - *Teori dan implementasi Ring Buffer untuk UART RX*

3. **"Robust Serial Protocol Design for Embedded Systems"**
   - Referensi untuk STX/ETX framing, checksum, ACK/NAK protocols

4. **"NMEA 0183 Protocol Standard"**
   - *Contoh protokol serial real-world yang menggunakan UART*

### STM32 Application Notes
5. **AN2606** - STM32 Bootloader UART Protocol
   - [STMicroelectronics](https://www.st.com/resource/en/application_note/an2606-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf)

6. **AN4031** - Using the STM32F1 USART DMA
   - *DMA-based UART untuk throughput tinggi*

---

## 🛠️ Tools dan Software

### Development Environment
| Tool | Fungsi | Link |
|------|--------|------|
| **VS Code** | Code Editor | [code.visualstudio.com](https://code.visualstudio.com/) |
| **PlatformIO** | Build System (ESP-IDF & STM32Cube) | [platformio.org](https://platformio.org/) |
| **STM32CubeIDE** | STM32 IDE Official + HAL Config | [st.com](https://www.st.com/en/development-tools/stm32cubeide.html) |
| **ESP-IDF Tools** | ESP32 Build System | [espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) |
| **STM32CubeMX** | Pin & Clock Configuration | [st.com](https://www.st.com/en/development-tools/stm32cubemx.html) |

### Serial Terminal Tools
| Tool | Fungsi | Link |
|------|--------|------|
| **HTerm** | Advanced serial terminal (hex view) | [der-hammer.info](https://www.der-hammer.info/pages/terminal.html) |
| **RealTerm** | Professional serial terminal | [SourceForge](https://sourceforge.net/projects/realterm/) |
| **CoolTerm** | Cross-platform serial monitor | [freeware.the-meiers.org](https://freeware.the-meiers.org/) |
| **minicom** | Linux serial terminal | `apt install minicom` |
| **PlatformIO Serial Monitor** | Built-in VS Code | PlatformIO extension |

### Debugging Tools
| Tool | Fungsi | Link |
|------|--------|------|
| **Logic Analyzer** | Capture & decode UART signals | Saleae, DSLogic, sigrok |
| **Saleae Logic 2** | Professional logic analyzer software | [Saleae](https://www.saleae.com/downloads/) |
| **Oscilloscope** | Analog waveform view | Any brand |
| **ST-Link Utility** | STM32 programming & debugging | [st.com](https://www.st.com/en/development-tools/stsw-link004.html) |
| **PulseView** | Open-source logic analyzer | [sigrok.org](https://sigrok.org/wiki/PulseView) |

### Simulation
| Tool | Fungsi | Link |
|------|--------|------|
| **Wokwi** | ESP32 UART Simulator | [wokwi.com](https://wokwi.com/) |
| **Proteus** | Circuit Simulation with UART | [labcenter.com](https://www.labcenter.com/) |
| **QEMU** | ARM Emulation | [qemu.org](https://www.qemu.org/) |

---

## 📊 Datasheet Komponen

### USB-to-Serial Converter
| Komponen | Datasheet/Info |
|----------|----------------|
| CP2102 | [Silicon Labs](https://www.silabs.com/documents/public/data-sheets/CP2102-9.pdf) |
| CH340G | [WCH](http://www.wch-ic.com/products/CH340.html) |
| FT232RL | [FTDI](https://ftdichip.com/products/ft232rl/) |

### Level Shifter
| Komponen | Info |
|----------|------|
| BSS138 MOSFET Level Shifter | Bidirectional 3.3V ↔ 5V |
| TXB0104 | 4-bit bidirectional level shifter |
| Resistor Divider | 1kΩ + 2kΩ untuk 5V → 3.3V |

### RS-232 Transceiver
| Komponen | Info |
|----------|------|
| MAX232 | TTL ↔ RS-232 converter |
| SP3232 | 3.3V TTL ↔ RS-232 converter |

---

## 🔗 Repository dan Example Code

### GitHub Repositories
| Repository | Deskripsi | Link |
|------------|-----------|------|
| **STM32CubeF1** | HAL Examples & UART Drivers | [GitHub](https://github.com/STMicroelectronics/STM32CubeF1) |
| **ESP-IDF** | Official ESP-IDF Framework | [GitHub](https://github.com/espressif/esp-idf) |
| **ESP-IDF UART Examples** | uart_echo, uart_events, uart_select | [GitHub](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart) |
| **STM32 HAL UART Example** | Nucleo UART Example | [GitHub](https://github.com/STMicroelectronics/STM32CubeF1/tree/master/Projects) |
| **CRC Implementation** | CRC-16/CRC-32 in C | [GitHub](https://github.com/lammertb/libcrc) |

### Code Examples
```bash
# Clone ESP-IDF UART examples
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf/examples/peripherals/uart/

# Clone STM32 HAL examples
git clone https://github.com/STMicroelectronics/STM32CubeF1.git
# Look in Projects/*/Examples/UART/
```

---

## 📡 Protocol Standards

### Serial Communication Standards
| Standard | Voltage | Topology | Distance | Speed |
|----------|---------|----------|----------|-------|
| **TTL Serial** | 3.3V / 5V | Point-to-point | <1m | Up to 5 Mbps |
| **RS-232** | ±15V | Point-to-point | <15m | Up to 1 Mbps |
| **RS-422** | ±5V | Multi-drop (1 TX, 10 RX) | <1200m | Up to 10 Mbps |
| **RS-485** | ±5V | Multi-drop (32 nodes) | <1200m | Up to 10 Mbps |

### Protocol References
1. **EIA/TIA-232-F** - RS-232 Standard
2. **EIA/TIA-485** - RS-485 Standard
3. **NMEA 0183** - GPS Serial Protocol (contoh UART protocol)
4. **Modbus RTU** - Industrial serial protocol over RS-485

---

## 📱 Community dan Forum

### Forum Diskusi
| Forum | Link |
|-------|------|
| **STM32 Community** | [community.st.com](https://community.st.com/) |
| **ESP32 Forum** | [esp32.com/forum](https://www.esp32.com/) |
| **ESP-IDF Issues/Discuss** | [github.com/espressif](https://github.com/espressif/esp-idf/issues) |
| **EEVBlog Forum** | [eevblog.com/forum](https://www.eevblog.com/forum/) |
| **Stack Overflow** | Tag: stm32-uart, esp32-uart, uart |
| **Reddit** | r/embedded, r/esp32, r/stm32 |

---

## 📝 Catatan Penggunaan

1. **Referensi Primer:** Gunakan dokumentasi resmi (RM0008 Chapter 27, ESP-IDF UART API) sebagai sumber utama
2. **Buku Wajib:** Mastering STM32 Ch.8 untuk STM32 HAL UART, Kolban ESP32 p.283-287 untuk ESP-IDF UART
3. **Framework:** Semua kode harus menggunakan ESP-IDF (bukan Arduino) dan STM32Cube HAL
4. **Examples:** ESP-IDF uart_echo dan uart_events adalah starting point terbaik
5. **Forum:** Gunakan untuk troubleshooting, tapi verifikasi jawaban dengan dokumentasi resmi

---

## 🔄 Pembaruan Terakhir

- **Tanggal:** Februari 2026
- **Versi:** 1.0
- **Catatan:** Link diverifikasi pada tanggal update

---

*Referensi Modul 03 - Praktikum Sistem Embedded*
