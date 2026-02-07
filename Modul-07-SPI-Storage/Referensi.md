# Referensi Modul 07: SPI Bus dan Storage

## 📚 Dokumentasi Resmi

### STM32 Documentation
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **RM0008** | STM32F1 Reference Manual - SPI Chapter | [ST.com](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| **HAL SPI Driver** | STM32 HAL SPI Documentation | [ST GitHub](https://github.com/STMicroelectronics/stm32f1xx_hal_driver) |
| **AN4013** | STM32 SPI DMA Tips | [ST.com](https://www.st.com/resource/en/application_note/an4013-stm32-crossseries-timer-overview-stmicroelectronics.pdf) |

### ESP32 Documentation
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **ESP32 TRM** | Technical Reference Manual - SPI Controller | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) |
| **Arduino SPI** | ESP32 SPI API Reference | [Espressif IO](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html) |
| **SD Library** | Arduino ESP32 SD Implementation | [GitHub](https://github.com/espressif/arduino-esp32/tree/master/libraries/SD) |

### Storage Standards
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **SD Spec** | SD Physical Layer Simplified Specification | [SD Association](https://www.sdcard.org/downloads/pls/) |
| **FatFS** | FatFS Generic FAT File System Module | [Elm-Chan](http://elm-chan.org/fsw/ff/00index_e.html) |

---

## 📖 Datasheet Komponen

### W25Qxx Serial Flash
| Item | Link |
|------|------|
| W25Q64FW | [Winbond Datasheet](https://www.winbond.com/resource-files/w25q64fw%20revd%20032513.pdf) |
| W25Q128JV | [Winbond Datasheet](https://www.winbond.com/resource-files/w25q128jv%20revf%2008032017.pdf) |

### SD Card
| Item | Link |
|------|------|
| SD Card Pinout | [Pinouts.ru](https://pinouts.ru/Memory/sd_card_pinout.shtml) |
| SPI Mode | [Wikipedia](https://en.wikipedia.org/wiki/Secure_Digital#SPI_bus_interface_mode) |

---

## 🎓 Tutorial

### SPI Fundamentals
1. **Sparkfun SPI Tutorial** - [learn.sparkfun.com/tutorials/serial-peripheral-interface-spi](https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi)
2. **Introduction to SPI Interface** - [Analog Devices](https://www.analog.com/en/analog-dialogue/articles/introduction-to-spi-interface.html)

### STM32 SPI & FatFS
1. **STM32 SPI with HAL** - [ControllersTech](https://controllerstech.com/spi-in-stm32/)
2. **STM32 SD Card Interfacing (FatFS)** - [ControllersTech](https://controllerstech.com/sd-card-using-spi-in-stm32/)
3. **Circular Buffer Implementation** - [Embedded.com](https://www.embedded.com/ring-buffer-basics/)

### ESP32 SPI & Storage
1. **ESP32 SPI Communication** - [RandomNerdTutorials](https://randomnerdtutorials.com/esp32-spi-communication-arduino/)
2. **ESP32 MicroSD Card Data Logging** - [RandomNerdTutorials](https://randomnerdtutorials.com/esp32-microsd-card-arduino/)
3. **ESP32 Flash Memory (SPIFFS/LittleFS)** - [RandomNerdTutorials](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/)

---

## 📝 Paper/Artikel

### Academic Resources
1. "Optimization of SPI Protocol for High-Speed Embedded Systems" - IEEE Xplore
2. "Reliability of Flash Memory in Embedded Applications" - ResearchGate
3. "Comparative Analysis of FatFS and LittleFS" - Embedded Systems Journal

### Books
| Judul | Penulis | Topik |
|-------|---------|-------|
| **Mastering STM32** | Carmine Noviello | SPI & DMA Chapters |
| **Making Embedded Systems** | Elecia White | Design Patterns (Logging) |
| **The Definitive Guide to ARM Cortex-M3** | Joseph Yiu | Memory Map & Peripherals |

---

## 🎥 Video

### Teoritis
1. **SPI Protocol Explained (EEVblog)** - [YouTube](https://www.youtube.com/watch?v=ba0SqOTPkWE)
2. **How SD Cards Work (Branch Education)** - [YouTube](https://www.youtube.com/watch?v=3hRfbF8X18c)

### Praktikal
1. **STM32 SPI Transmit/Receive (Controllers Tech)** - [YouTube](https://www.youtube.com/watch?v=6P3B9o9W-xo)
2. **ESP32 SD Card Logging Tutorial** - [YouTube](https://www.youtube.com/watch?v=vVjV4YdeVv0)

---

## 🛠️ Tools

| Tool | Deskripsi | Link |
|------|-----------|------|
| **PulseView** | Logic Analyzer Software (Sigrok) | [sigrok.org](https://sigrok.org/wiki/PulseView) |
| **HxD Hex Editor** | View SD Card binary content | [mh-nexus.de](https://mh-nexus.de/en/hxd/) |
| **STM32CubeMX** | Configurator Tool | [ST.com](https://www.st.com/en/development-tools/stm32cubemx.html) |
| **DiskImager** | Tool untuk backup image SD Card | [sourceforge.net](https://sourceforge.net/projects/win32diskimager/) |

---

## 📋 SPI Modes Reference

| Mode | CPOL | CPHA | Clock Idle | Sample Edge | Common Usage |
|------|------|------|------------|-------------|--------------|
| 0    | 0    | 0    | Low        | Rising      | Most Devices |
| 1    | 0    | 1    | Low        | Falling     | Rare         |
| 2    | 1    | 0    | High       | Falling     | Rare         |
| 3    | 1    | 1    | High       | Rising      | Some Sensors |

---

*Terakhir diperbarui: 2024*
