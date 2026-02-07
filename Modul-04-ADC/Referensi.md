# Referensi Pembelajaran
## Modul 04: Analog to Digital Converter (ADC)

Berikut adalah daftar referensi untuk mendukung pemahaman teori dan implementasi praktikum ADC.

## 📚 Dokumentasi Resmi (Datasheets & App Notes)

1.  **STM32F103x8 Datasheet & Reference Manual**
    *   *Reference:* Chapter 11 (ADC) pada RM0008.
    *   *Topik:* SAR Architecture, Scan Mode, Continuous Conversion, Calibration.
    *   [Link ST.com - RM0008](https://www.st.com/resource/en/reference_manual/cd00171190.pdf)

2.  **AN2834 Application Note: How to get the best ADC accuracy in STM32F10xxx**
    *   *Deskripsi:* Dokumen wajib baca untuk memahami sumber error pada ADC (noise, impedansi sumber) dan cara mengatasinya.
    *   [Link AN2834](https://www.st.com/resource/en/application_note/cd00211314-how-to-get-the-best-adc-accuracy-in-stm32fx-series-microcontrollers-stmicroelectronics.pdf)

3.  **ESP32 Technical Reference Manual (ADC Chapter)**
    *   *Deskripsi:* Penjelasan detail tentang ADC1 vs ADC2, Attenuation, dan Vref calibration.
    *   [Link Espressif TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

## 💻 Tutorial & Artikel Teknis

1.  **Deep Blue Embedded - STM32 ADC Tutorial**
    *   Tutorial lengkap konfigurasi ADC Single Channel, Multi-channel, dan DMA menggunakan HAL/Arduino.
    *   [DeepBlueEmbedded - STM32 ADC](https://deepbluembedded.com/stm32-adc-tutorial-complete-guide-with-examples/)

2.  **Random Nerd Tutorials - ESP32 ADC Analog Read**
    *   Panduan praktis membaca tegangan analog pada ESP32, isu non-linearitas, dan penggunaan pin yang aman.
    *   [RNT - ESP32 ADC](https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/)

3.  **Moving Average Filter Implementation**
    *   Penjelasan konsep dan implementasi filter rata-rata bergerak di C++ untuk menghaluskan pembacaan sensor.
    *   [GeeksForGeeks - Moving Average](https://www.geeksforgeeks.org/program-find-simple-moving-average/)

## 🔋 Referensi Project BMS

1.  **Battery Management System Basics**
    *   Penjelasan parameter baterai: SoC (State of Charge), SoH (State of Health), OCV (Open Circuit Voltage).
    *   [DigiKey - BMS Basics](https://www.digikey.com/en/articles/a-basic-introduction-to-battery-management-systems)

2.  **Measuring Voltage and Current with Arduino/STM32**
    *   Tutorial dasar penggunaan Voltage Divider dan Current Shunt/ACS712.
    *   [DroneBot Workshop - Voltage & Current](https://dronebotworkshop.com/dc-voltage-current/)

## 📹 Video Pembelajaran

1.  **EEVblog #59 - ADC Aliasing & Nyquist**
    *   Penjelasan mendalam tapi santai tentang teori sampling dan aliasing.
    *   [YouTube - EEVblog](https://www.youtube.com/watch?v=vVjV-dhkmbA)

2.  **How ADC Works (SAR Architecture)**
    *   Animasi visual cara kerja Successive Approximation Register (SAR) ADC.
    *   [YouTube Reference](https://www.youtube.com/watch?v=...)
