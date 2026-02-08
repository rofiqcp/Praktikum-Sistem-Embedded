# 📚 Referensi Modul 05: DAC & PWM (Digital-to-Analog Converter & Pulse Width Modulation)

## 📖 Dokumentasi Resmi

### ESP32

| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | ESP-IDF LEDC (LED Control) | [docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html) | API PWM menggunakan peripheral LEDC, hingga 16 channel |
| 2 | ESP-IDF DAC (Digital-to-Analog Converter) | [docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html) | API DAC 8-bit pada GPIO25/GPIO26 |
| 3 | ESP-IDF MCPWM (Motor Control PWM) | [docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/mcpwm.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/mcpwm.html) | PWM khusus kontrol motor (servo, BLDC) |
| 4 | ESP32 Technical Reference Manual – DAC | [espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) | Bab 29: DAC Controller (2 channel, 8-bit) |
| 5 | ESP32 Technical Reference Manual – LEDC | [espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) | Bab 14: LED PWM Controller |

### STM32

| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | STM32 HAL TIM PWM Reference | [st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers.pdf](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers.pdf) | HAL API untuk Timer PWM STM32F4 |
| 2 | STM32F411 Reference Manual (RM0383) – Timer | [st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus.pdf](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus.pdf) | Bab 10-11: General-purpose & Advanced Timers |
| 3 | AN4013: Timer Cookbook for STM32 | [st.com/resource/en/application_note/an4013-stm32-cross-series-timer-overview.pdf](https://www.st.com/resource/en/application_note/an4013-stm32-cross-series-timer-overview.pdf) | Panduan lengkap konfigurasi Timer dan PWM |
| 4 | AN4776: General-purpose Timer in STM32 | [st.com/resource/en/application_note/an4776-generalpurpose-timer-cookbook-for-stm32-microcontrollers.pdf](https://www.st.com/resource/en/application_note/an4776-generalpurpose-timer-cookbook-for-stm32-microcontrollers.pdf) | Contoh penggunaan Timer untuk PWM, input capture, dll |
| 5 | STM32F411 Datasheet | [st.com/resource/en/datasheet/stm32f411ce.pdf](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf) | Spesifikasi Timer dan pin alternate function |

## 📘 Buku Teks

### ESP32

| No | Judul Buku | Penulis | Bagian Relevan | Keterangan |
|----|-----------|---------|----------------|------------|
| 1 | Kolban's Book on ESP32 | Neil Kolban | Halaman 303-311 | DAC pada ESP32: konfigurasi output analog, cosine wave generator. PWM dengan LEDC: channel, duty, frekuensi |
| 2 | ESP32 Programming for the IoT | Sever Spanulescu | Bab 8: Analog Output & PWM | Penggunaan DAC dan LEDC untuk output analog dan kontrol motor |
| 3 | Internet of Things with ESP32 | Agus Kurniawan | Bab PWM & Motor Control | Kontrol LED, servo, dan buzzer dengan PWM ESP32 |

### STM32

| No | Judul Buku | Penulis | Bagian Relevan | Keterangan |
|----|-----------|---------|----------------|------------|
| 1 | Mastering STM32 | Carmine Noviello | **Bab 11: Timer** | Konfigurasi timer, prescaler, ARR, dan mode PWM |
| 2 | Mastering STM32 | Carmine Noviello | **Bab 13: DAC** | Digital-to-Analog Converter pada STM32 (untuk seri yang memiliki DAC) |
| 3 | Beginning STM32 | Warren Gay | Bab 10: Timer & PWM | Dasar Timer dan output PWM dengan HAL |
| 4 | Embedded Systems with ARM Cortex-M | Jonathan Valvano | Bab 12: DAC & Bab 13: Timer/PWM | Teori DAC R-2R, implementasi PWM pada ARM Cortex-M |

## 📄 Datasheet Komponen

| No | Komponen | Dokumen | Keterangan |
|----|----------|---------|------------|
| 1 | Servo SG90 | [Datasheet SG90](http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf) | Spesifikasi: 50Hz PWM, pulse 1-2ms, torsi 1.8 kg·cm |
| 2 | PAM8403 Amplifier | [Datasheet PAM8403](https://www.diodes.com/assets/Datasheets/PAM8403.pdf) | Mini amplifier stereo 3W+3W untuk output speaker |
| 3 | Passive Buzzer | Datasheet umum | Frekuensi resonansi tipikal 2-4 kHz, driven by PWM |

## 🎥 Video Tutorial

| No | Judul | Platform | URL | Keterangan |
|----|-------|----------|-----|------------|
| 1 | ESP32 DAC Tutorial | YouTube | [youtu.be/GvFMJ5JGRnE](https://youtu.be/GvFMJ5JGRnE) | Output analog dengan DAC ESP32 |
| 2 | ESP32 LEDC PWM Tutorial | YouTube | [youtu.be/ERuFOamWQ68](https://youtu.be/ERuFOamWQ68) | Kontrol LED dan servo dengan LEDC |
| 3 | STM32 PWM Tutorial - HAL | YouTube | [youtu.be/AjN58ceQaF4](https://youtu.be/AjN58ceQaF4) | Konfigurasi Timer PWM STM32 |
| 4 | PWM vs DAC Explained | YouTube | [youtu.be/B_Ysdv1xRbA](https://youtu.be/B_Ysdv1xRbA) | Perbandingan PWM dan DAC untuk output analog |
| 5 | Servo Motor with ESP32/STM32 | YouTube | [youtu.be/dsPZ9Okcsxo](https://youtu.be/dsPZ9Okcsxo) | Kontrol servo SG90 dengan PWM |

## 🌐 Artikel & Tutorial Online

| No | Judul | URL | Keterangan |
|----|-------|-----|------------|
| 1 | Random Nerd Tutorials: ESP32 PWM | [randomnerdtutorials.com/esp32-pwm-arduino-ide/](https://randomnerdtutorials.com/esp32-pwm-arduino-ide/) | Tutorial LEDC PWM ESP32 dengan Arduino framework |
| 2 | Random Nerd Tutorials: ESP32 DAC | [randomnerdtutorials.com/esp32-dac-audio-arduino/](https://randomnerdtutorials.com/esp32-dac-audio-arduino/) | Output audio sederhana dengan DAC ESP32 |
| 3 | DeepBlueMbedded: STM32 PWM | [deepbluembedded.com/stm32-pwm-example-timer-pwm-mode/](https://deepbluembedded.com/stm32-pwm-example-timer-pwm-mode/) | Tutorial lengkap PWM STM32 dengan Timer |
| 4 | Controllerstech: STM32 Servo Control | [controllerstech.com/servo-motor-with-stm32/](https://controllerstech.com/servo-motor-with-stm32/) | Kontrol servo menggunakan PWM Timer STM32 |
| 5 | DeepBlueMbedded: STM32 DAC | [deepbluembedded.com/stm32-dac-tutorial/](https://deepbluembedded.com/stm32-dac-tutorial/) | Tutorial DAC pada STM32 (seri yang mendukung DAC) |
| 6 | Last Minute Engineers: Servo SG90 | [lastminuteengineers.com/servo-motor-arduino-tutorial/](https://lastminuteengineers.com/servo-motor-arduino-tutorial/) | Prinsip kerja dan kontrol servo motor |

## 📊 Application Notes Tambahan

| No | Dokumen | Keterangan |
|----|---------|------------|
| 1 | AN3126: Audio and waveform generation using DAC in STM32 | Generasi audio dan gelombang dengan DAC STM32 |
| 2 | AN4566: Extending the DAC performance of STM32 | Teknik meningkatkan performa DAC pada STM32 |
| 3 | ESP-IDF Examples: LEDC | Contoh kode LEDC resmi dari Espressif |
| 4 | ESP-IDF Examples: DAC | Contoh kode DAC resmi (cosine wave, DMA output) |
| 5 | TI Application Note: PWM DAC | Teknik menggunakan PWM sebagai DAC dengan RC filter |

## 🔗 Tautan Cepat Repository Contoh

| Platform | Repository | Keterangan |
|----------|-----------|------------|
| ESP32 | [github.com/espressif/esp-idf/tree/master/examples/peripherals/ledc](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/ledc) | Contoh LEDC PWM resmi ESP-IDF |
| ESP32 | [github.com/espressif/esp-idf/tree/master/examples/peripherals/dac](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/dac) | Contoh DAC resmi ESP-IDF |
| STM32 | [github.com/STMicroelectronics/STM32CubeF4/tree/master/Projects](https://github.com/STMicroelectronics/STM32CubeF4/tree/master/Projects) | Contoh project STM32F4 termasuk Timer PWM |
| Arduino | [github.com/espressif/arduino-esp32/tree/master/libraries](https://github.com/espressif/arduino-esp32/tree/master/libraries) | Library Arduino-ESP32 untuk analogWrite dan LEDC |

## 📐 Tabel Referensi Cepat

### Frekuensi Nada Musik (Oktaf 4-5)

| Nada | Frekuensi (Hz) | Nada | Frekuensi (Hz) |
|------|:--------------:|------|:--------------:|
| C4 | 262 | C5 | 523 |
| D4 | 294 | D5 | 587 |
| E4 | 330 | E5 | 659 |
| F4 | 349 | F5 | 698 |
| G4 | 392 | G5 | 784 |
| A4 | 440 | A5 | 880 |
| B4 | 494 | B5 | 988 |

### Servo SG90 Pulse Width

| Sudut | Pulse Width | Duty Cycle (50Hz) |
|:-----:|:-----------:|:-----------------:|
| 0° | 500 µs | 2.5% |
| 45° | 1000 µs | 5.0% |
| 90° | 1500 µs | 7.5% |
| 135° | 2000 µs | 10.0% |
| 180° | 2500 µs | 12.5% |
