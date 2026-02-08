# Referensi
## Modul 01: GPIO Digital I/O

---

## 📚 Dokumentasi Resmi

### STM32
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **RM0008 Reference Manual** | [STMicroelectronics](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) | Manual lengkap STM32F103 |
| **STM32F103C8T6 Datasheet** | [STMicroelectronics](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) | Spesifikasi pin dan elektrikal |
| **PM0056 Programming Manual** | [STMicroelectronics](https://www.st.com/resource/en/programming_manual/pm0056-stm32f10xxx20xxx21xxxl1xxxx-cortexm3-programming-manual-stmicroelectronics.pdf) | Cortex-M3 programming |

### ESP32
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **ESP32 Technical Reference** | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) | Manual teknis lengkap |
| **ESP32 Datasheet** | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) | Spesifikasi hardware |
| **ESP-IDF GPIO Documentation** | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html) | API reference GPIO |

### ESP-IDF Framework
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **ESP-IDF Programming Guide** | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) | Panduan lengkap ESP-IDF |
| **ESP-IDF GPIO Driver** | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html) | API reference driver GPIO |

### STM32Cube HAL Framework
| Dokumen | Link | Deskripsi |
|---------|------|-----------|
| **STM32Cube HAL User Manual** | [STMicroelectronics](https://www.st.com/resource/en/user_manual/um1850-description-of-stm32f1-hal-and-lowlayer-drivers-stmicroelectronics.pdf) | Manual HAL driver STM32F1 |
| **STM32CubeF1 Package** | [GitHub](https://github.com/STMicroelectronics/STM32CubeF1) | Source code HAL + contoh |

---

## 📖 Buku Referensi

### Wajib Baca
1. **"Mastering STM32" - Second Edition**
   - Penulis: Carmine Noviello
   - **Chapter 6: GPIO Management** (konfigurasi, mode, register access)
   - Tersedia di: [leanpub.com](https://leanpub.com/mastering-stm32)
   - *Catatan: Referensi utama untuk STM32 HAL programming*

2. **"Kolban's Book on ESP32"**
   - Penulis: Neil Kolban
   - Chapter: GPIO and RTC GPIO, **Halaman 251-257**
   - Tersedia gratis di: [leanpub.com](https://leanpub.com/kolban-ESP32)
   - *Catatan: Referensi utama untuk ESP32, fokus pada bagian GPIO*

### Bacaan Tambahan
3. **"The Definitive Guide to ARM Cortex-M3"**
   - Penulis: Joseph Yiu
   - ISBN: 978-0124080829
   - *Untuk pemahaman mendalam arsitektur ARM*

4. **"Make: AVR Programming"**
   - Penulis: Elliot Williams
   - *Konsep GPIO yang transferable ke platform lain*

---

## 🎥 Video Tutorial

### YouTube Channels
| Channel | Konten | Link |
|---------|--------|------|
| **Controllers Tech** | STM32 HAL Tutorials | [YouTube](https://www.youtube.com/c/ControllersTech) |
| **Phil's Lab** | Embedded Design | [YouTube](https://www.youtube.com/c/PhilsLab) |
| **DroneBot Workshop** | ESP32 Tutorials | [YouTube](https://www.youtube.com/c/Dronebotworkshop) |
| **Andreas Spiess** | ESP32 Deep Dive | [YouTube](https://www.youtube.com/c/AndreasSpiess) |
| **Low Level Learning** | Embedded Concepts | [YouTube](https://www.youtube.com/c/LowLevelLearning) |

### Video Spesifik GPIO
1. **STM32 GPIO Tutorial** - Controllers Tech
   - [Part 1: Output](https://www.youtube.com/watch?v=...)
   - [Part 2: Input](https://www.youtube.com/watch?v=...)

2. **ESP32 GPIO Explained** - Random Nerd Tutorials
   - [Complete Guide](https://www.youtube.com/watch?v=...)

3. **Button Debouncing Explained** - All About Circuits
   - [Hardware & Software Methods](https://www.youtube.com/watch?v=...)

---

## 🌐 Website dan Tutorial Online

### Tutorial Sites
| Website | Topik | Link |
|---------|-------|------|
| **Random Nerd Tutorials** | ESP32 GPIO | [Link](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/) |
| **DeepBlue Embedded** | STM32 GPIO | [Link](https://deepbluembedded.com/stm32-gpio-tutorial-input-output-pins/) |
| **Embedded Artistry** | Best Practices | [Link](https://embeddedartistry.com/blog/2018/06/25/controlling-gpio-lines-from-userspace/) |
| **All About Circuits** | Debouncing | [Link](https://www.allaboutcircuits.com/technical-articles/switch-bounce-how-to-deal-with-it/) |
| **Ganssle Group** | Debouncing Deep Dive | [Link](http://www.ganssle.com/debouncing.htm) |

### Interactive Learning
| Platform | Course | Link |
|----------|--------|------|
| **Coursera** | Embedded Systems | [Link](https://www.coursera.org/specializations/introduction-embedded-systems) |
| **edX** | Real-Time Systems | [Link](https://www.edx.org/course/real-time-bluetooth-networks-shape-the-world) |
| **Udemy** | STM32 Programming | Search "STM32" |

---

## 📄 Paper dan Artikel Teknis

### Debouncing
1. **"A Guide to Debouncing"** - Jack Ganssle
   - [ganssle.com/debouncing.htm](http://www.ganssle.com/debouncing.htm)
   - *Artikel klasik tentang debouncing*

2. **"Debouncing Switches in Software"** - Application Note
   - Microchip AN1152

### GPIO Design
3. **"GPIO Best Practices"** - STMicroelectronics
   - AN4899: Guidelines for using GPIO

4. **"ESP32 GPIO and RTC GPIO"** - Espressif Application Note
   - [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/)

---

## 🛠️ Tools dan Software

### Development Environment
| Tool | Fungsi | Link |
|------|--------|------|
| **VS Code** | Code Editor | [code.visualstudio.com](https://code.visualstudio.com/) |
| **PlatformIO** | Build System | [platformio.org](https://platformio.org/) |
| **STM32CubeIDE** | STM32 IDE Official | [st.com](https://www.st.com/en/development-tools/stm32cubeide.html) |
| **ESP-IDF Tools** | ESP32 Build System | [espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) |

### Debugging Tools
| Tool | Fungsi | Link |
|------|--------|------|
| **Logic Analyzer** | Signal analysis | Saleae, DSLogic |
| **Oscilloscope** | Waveform view | Any brand |
| **Serial Monitor** | Debug output | Built-in PlatformIO |
| **ST-Link Utility** | STM32 programming | [st.com](https://www.st.com/en/development-tools/stsw-link004.html) |

### Simulation
| Tool | Fungsi | Link |
|------|--------|------|
| **Wokwi** | ESP32 Simulator | [wokwi.com](https://wokwi.com/) |
| **Proteus** | Circuit Simulation | [labcenter.com](https://www.labcenter.com/) |
| **QEMU** | ARM Emulation | [qemu.org](https://www.qemu.org/) |

---

## 📊 Datasheet Komponen

### LED
| Komponen | Datasheet |
|----------|-----------|
| LED 5mm Standard | [Typical LED Datasheet](https://www.sparkfun.com/datasheets/Components/LED/COM-09590-YSL-R531R3D-D2.pdf) |
| WS2812B (RGB) | [Worldsemi](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf) |

### Button/Switch
| Komponen | Datasheet |
|----------|-----------|
| Tactile Switch 6mm | [Omron B3F](https://omronfs.omron.com/en_US/ecb/products/pdf/en-b3f.pdf) |
| DIP Switch | [Various manufacturers](https://www.mouser.com/datasheet/2/58/BPA-1147188.pdf) |

### Resistor
| Komponen | Info |
|----------|------|
| Standard Resistor | E24 series: 220Ω, 330Ω, 1kΩ, 10kΩ |
| Power Dissipation | P = I² × R |

---

## 🔗 Repository dan Example Code

### GitHub Repositories
| Repository | Deskripsi | Link |
|------------|-----------|------|
| **STM32CubeF1** | HAL Examples & Drivers | [GitHub](https://github.com/STMicroelectronics/STM32CubeF1) |
| **ESP-IDF** | Official ESP-IDF Framework | [GitHub](https://github.com/espressif/esp-idf) |
| **ESP-IDF Examples** | GPIO Examples | [GitHub](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/gpio) |
| **STM32 HAL GPIO Example** | Nucleo GPIO Example | [GitHub](https://github.com/STMicroelectronics/STM32CubeF1/tree/master/Projects) |

### Code Examples Spesifik GPIO
```
# Clone untuk referensi
git clone https://github.com/STMicroelectronics/STM32CubeF1.git
git clone --recursive https://github.com/espressif/esp-idf.git
```

---

## 📱 Community dan Forum

### Forum Diskusi
| Forum | Link |
|-------|------|
| **STM32 Community** | [community.st.com](https://community.st.com/) |
| **ESP32 Forum** | [esp32.com/forum](https://www.esp32.com/) |
| **ESP-IDF Issues/Discuss** | [github.com/espressif](https://github.com/espressif/esp-idf/issues) |
| **EEVBlog Forum** | [eevblog.com/forum](https://www.eevblog.com/forum/) |
| **Stack Overflow** | Tag: stm32, esp32, gpio |
| **Reddit** | r/embedded, r/esp32, r/stm32 |

### Discord Servers
- Embedded Systems Community
- ESP32 & ESP8266 Community
- ARM Developers

---

## 📝 Catatan Penggunaan

1. **Referensi Primer:** Gunakan dokumentasi resmi (datasheet, reference manual) sebagai sumber utama
2. **Tutorial:** Gunakan untuk memahami konsep, tapi verifikasi dengan dokumentasi resmi
3. **Forum:** Bagus untuk troubleshooting dan tips dari praktisi
4. **Code Examples:** Selalu review dan pahami sebelum menggunakan

---

## 🔄 Pembaruan Terakhir

- **Tanggal:** Februari 2026
- **Versi:** 1.0
- **Catatan:** Link diverifikasi pada tanggal update

---

*Referensi Modul 01 - Praktikum Sistem Embedded*
