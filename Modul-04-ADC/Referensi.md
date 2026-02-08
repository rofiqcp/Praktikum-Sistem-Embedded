# 📚 Referensi Modul 04: ADC (Analog-to-Digital Converter)

## 📖 Dokumentasi Resmi

### ESP32

| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | ESP-IDF ADC Oneshot Mode | [docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_oneshot.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_oneshot.html) | API ADC mode pembacaan tunggal |
| 2 | ESP-IDF ADC Continuous Mode | [docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_continuous.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_continuous.html) | API ADC mode pembacaan kontinu dengan DMA |
| 3 | ESP-IDF ADC Calibration | [docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_calibration.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_calibration.html) | Kalibrasi ADC untuk akurasi pembacaan |
| 4 | ESP32 Technical Reference Manual – SAR ADC | [espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) | Bab 29: SAR ADC (Successive Approximation Register) |
| 5 | ESP32 Datasheet | [espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) | Spesifikasi elektrikal ADC |

### STM32

| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | STM32 HAL ADC Reference | [st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers.pdf](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers.pdf) | HAL API untuk ADC STM32F4 |
| 2 | STM32F411 Reference Manual (RM0383) | [st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus.pdf](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus.pdf) | Bab 11: ADC pada STM32F411 |
| 3 | STM32F411 Datasheet | [st.com/resource/en/datasheet/stm32f411ce.pdf](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf) | Spesifikasi elektrikal ADC |
| 4 | AN2834: How to get the best ADC accuracy in STM32 | [st.com/resource/en/application_note/an2834-how-to-get-the-best-adc-accuracy-in-stm32-microcontrollers.pdf](https://www.st.com/resource/en/application_note/an2834-how-to-get-the-best-adc-accuracy-in-stm32-microcontrollers.pdf) | Panduan optimasi akurasi ADC |
| 5 | AN3116: STM32 ADC modes and their applications | [st.com/resource/en/application_note/an3116-stm32s-adc-modes-and-their-applications.pdf](https://www.st.com/resource/en/application_note/an3116-stm32s-adc-modes-and-their-applications.pdf) | Mode-mode ADC dan penggunaannya |

## 📘 Buku Teks

### ESP32

| No | Judul Buku | Penulis | Bagian Relevan | Keterangan |
|----|-----------|---------|----------------|------------|
| 1 | Kolban's Book on ESP32 | Neil Kolban | Halaman 308-310 | ADC pada ESP32: konfigurasi, attenuation, channel mapping |
| 2 | ESP32 Programming for the IoT | Sever Spanulescu | Bab 7: Analog Input | Pembacaan sensor analog, kalibrasi ADC |
| 3 | Internet of Things with ESP32 | Agus Kurniawan | Bab ADC & Sensor | Integrasi sensor analog dengan ESP32 |

### STM32

| No | Judul Buku | Penulis | Bagian Relevan | Keterangan |
|----|-----------|---------|----------------|------------|
| 1 | Mastering STM32 | Carmine Noviello | Bab 12: ADC (Analog-to-Digital Conversion) | Konfigurasi ADC, mode polling/interrupt/DMA, multi-channel |
| 2 | Beginning STM32 | Warren Gay | Bab 11: Analog-to-Digital Conversion | Dasar ADC dengan HAL, pembacaan sensor |
| 3 | Embedded Systems with ARM Cortex-M | Jonathan Valvano | Bab 14: ADC | Teori ADC dan implementasi pada ARM Cortex-M |

## 📄 Datasheet Sensor

| No | Sensor | Dokumen | Keterangan |
|----|--------|---------|------------|
| 1 | MQ-135 | [Datasheet MQ-135](https://www.olimex.com/Products/Components/Sensors/Gas/SNS-MQ135/resources/SNS-MQ135.pdf) | Kurva sensitivitas Rs/R0 vs konsentrasi gas |
| 2 | LM35 | [Datasheet LM35](https://www.ti.com/lit/ds/symlink/lm35.pdf) | Sensor suhu analog, 10mV/°C, range -55°C sampai 150°C |
| 3 | LDR (GL5528) | [Datasheet GL5528](https://components101.com/sites/default/files/component_datasheet/LDR%20Datasheet.pdf) | Karakteristik resistansi vs intensitas cahaya |

## 🎥 Video Tutorial

| No | Judul | Platform | URL | Keterangan |
|----|-------|----------|-----|------------|
| 1 | ESP32 ADC Tutorial | YouTube | [youtu.be/RlKMJknsNpo](https://youtu.be/RlKMJknsNpo) | Dasar penggunaan ADC ESP32 |
| 2 | STM32 ADC Tutorial - HAL | YouTube | [youtu.be/oGBOKsAxS_o](https://youtu.be/oGBOKsAxS_o) | Konfigurasi ADC STM32 dengan HAL |
| 3 | STM32 ADC Multi-Channel DMA | YouTube | [youtu.be/VfbW6dQ5sYw](https://youtu.be/VfbW6dQ5sYw) | Multi-channel ADC dengan DMA |
| 4 | ADC Basics - Phil's Lab | YouTube | [youtu.be/EnfjYwe2A0w](https://youtu.be/EnfjYwe2A0w) | Teori ADC: sampling, quantization, aliasing |

## 🌐 Artikel & Tutorial Online

| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | Random Nerd Tutorials: ESP32 ADC | [randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/](https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/) | Tutorial ADC ESP32 dengan Arduino framework |
| 2 | DeepBlueMbedded: STM32 ADC | [deepbluembedded.com/stm32-adc-tutorial/](https://deepbluembedded.com/stm32-adc-tutorial/) | Tutorial lengkap ADC STM32 |
| 3 | Controllerstech: STM32 ADC Multi-Channel | [controllerstech.com/stm32-adc-multi-channel/](https://controllerstech.com/stm32-adc-multi-channel/) | Multi-channel ADC tanpa dan dengan DMA |
| 4 | ESP32.com: ADC Non-Linearity | [esp32.com/viewtopic.php?t=2881](https://esp32.com/viewtopic.php?t=2881) | Diskusi non-linearitas ADC ESP32 dan solusi |
| 5 | Analog Devices: MT-002 - What the Nyquist Criterion Means | [analog.com/media/en/training-seminars/tutorials/MT-002.pdf](https://www.analog.com/media/en/training-seminars/tutorials/MT-002.pdf) | Teori Nyquist untuk sampling ADC |

## 📊 Application Notes Tambahan

| No | Dokumen | Keterangan |
|----|---------|------------|
| 1 | AN4073: STM32F4 ADC Modes Overview | Overview mode ADC pada STM32F4 series |
| 2 | AN5601: Getting started with STM32 ADC | Panduan awal konfigurasi ADC STM32 |
| 3 | ESP-IDF Examples: ADC | Contoh kode ADC resmi dari Espressif |
| 4 | TI Application Report SLAA013: Sensor Signal Conditioning | Teknik pengkondisian sinyal sensor analog |

## 🔗 Tautan Cepat Repository Contoh

| Platform | Repository | Keterangan |
|----------|-----------|------------|
| ESP32 | [github.com/espressif/esp-idf/tree/master/examples/peripherals/adc](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/adc) | Contoh ADC resmi ESP-IDF |
| STM32 | [github.com/STMicroelectronics/STM32CubeF4/tree/master/Projects](https://github.com/STMicroelectronics/STM32CubeF4/tree/master/Projects) | Contoh project STM32F4 termasuk ADC |
| Arduino | [github.com/espressif/arduino-esp32/tree/master/libraries](https://github.com/espressif/arduino-esp32/tree/master/libraries) | Library Arduino-ESP32 untuk analogRead |
