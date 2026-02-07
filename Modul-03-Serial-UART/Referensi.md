# Referensi Pembelajaran
## Modul 03: Komunikasi Serial UART (STM32 & ESP32)

Berikut adalah daftar referensi yang direkomendasikan untuk mendukung pembelajaran dan pengerjaan project modul ini.

## 📚 Dokumentasi Resmi (Datasheets & Reference Manuals)

1.  **STM32F103x8 Datasheet**
    *   Deskripsi: Spesifikasi teknis pinout dan fitur electrical karakteristik.
    *   Link: [ST.com - STM32F103C8 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)

2.  **STM32F103 Reference Manual (RM0008)**
    *   Deskripsi: Panduan lengkap regiser USART (Chapter 27: Universal synchronous asynchronous receiver transmitter).
    *   Link: [ST.com - RM0008](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)

3.  **ESP32 Technical Reference Manual**
    *   Deskripsi: Detail implementasi UART Controller pada ESP32 (Chapter 13).
    *   Link: [Espressif - ESP32 TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

4.  **Arduino Core for STM32**
    *   Deskripsi: Wiki resmi penggunaan STM32 dengan framework Arduino.
    *   Link: [GitHub - STM32duino Wiki](https://github.com/stm32duino/wiki/wiki)

## 💻 Tutorial & Guide Praktis

1.  **PlatformIO with STM32 & ESP32**
    *   Panduan setup environment development yang digunakan di praktikum.
    *   [PlatformIO Docs - STM32](https://docs.platformio.org/en/latest/platforms/ststm32.html)
    *   [PlatformIO Docs - Espressif 32](https://docs.platformio.org/en/latest/platforms/espressif32.html)

2.  **Serial Communication Basics (SparkFun)**
    *   Penjelasan konsep baud rate, framing, dan wiring dasar.
    *   [SparkFun - Serial Communication](https://learn.sparkfun.com/tutorials/serial-communication/all)

3.  **ESP32 UART Communication (Random Nerd Tutorials)**
    *   Tutorial spesifik menggunakan HardwareSerial pada ESP32.
    *   [RNT - ESP32 UART TX/RX](https://randomnerdtutorials.com/esp32-uart-communication-serial-tcp-ip/)

4.  **Implementing Circular Buffer (Embedded.com)**
    *   Teori dan implementasi Ring Buffer untuk komunikasi serial yang efisien.
    *   [Embedded.com - Ring Buffer Basics](https://www.embedded.com/ring-buffer-basics/)

## 🛠️ Tools & Libraries

1.  **ArduinoJson**
    *   Library wajib untuk parsing data JSON di ESP32.
    *   [ArduinoJson.org](https://arduinojson.org/)

2.  **RealTerm / HTerm**
    *   Software terminal serial professional untuk debugging (Hex view, capturing).
    *   [RealTerm Download](https://sourceforge.net/projects/realterm/)

3.  **Saleae Logic 2**
    *   Software untuk membuka file capture logic analyzer (jika mahasiswa memiliki data sample).
    *   [Saleae Downloads](https://www.saleae.com/downloads/)

## 📹 Video Referensi

1.  **UART Communication Protocol Explained**
    *   Penjelasan visual bagaimana bit dikirim via kabel.
    *   [YouTube - The Engineering Mindset](https://www.youtube.com/watch?v=sO7LM3r_2ns)

2.  **STM32 Blue Pill UART Tutorial**
    *   Tutorial spesifik coding UART di STM32F103.
    *   [YouTube - Controllers Tech](https://www.youtube.com/watch?v=2eBK-0xXV2I)

3.  **ESP32 + STM32 Serial Communication**
    *   Contoh interkoneksi kedua MCU.
    *   [YouTube Reference](https://www.youtube.com/watch?v=...)

## 📄 Paper & Artikel Akademik

1.  *"Design of Wireless Sensor Network Gateway Based on ESP32"* - Referensi arsitektur project gateway.
2.  *"Robust Serial Protocol Implementation for Embedded Sytems"* - Referensi untuk error handling dan checksum.
