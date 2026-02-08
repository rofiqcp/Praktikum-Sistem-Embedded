# Referensi Modul 14: Power Management

Dokumen ini berisi kumpulan referensi yang digunakan dalam penyusunan materi dan praktikum Modul 14 — Power Management pada sistem embedded berbasis ESP32 dan STM32.

---

## Buku & Dokumentasi Resmi

1. **ESP-IDF Programming Guide — Sleep Modes**
   - [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html)
   - Dokumentasi resmi Espressif mengenai Light Sleep dan Deep Sleep pada ESP32, termasuk konfigurasi wake-up source (timer, GPIO, touch, ULP coprocessor).

2. **STM32F103 Reference Manual (RM0008) — Bab Power Control (PWR)**
   - [https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
   - Referensi lengkap register PWR, mode Sleep/Stop/Standby, konfigurasi voltage regulator, dan mekanisme wake-up pada keluarga STM32F1xx.

3. **STM32F103 Datasheet — Tabel Konsumsi Daya**
   - [https://www.st.com/resource/en/datasheet/stm32f103c8.pdf](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
   - Data konsumsi arus pada berbagai mode operasi (Run, Sleep, Stop, Standby) dan kondisi pengujian (frekuensi, tegangan, suhu).

4. **ESP32 Technical Reference Manual — Bab RTC dan Low-Power Management**
   - [https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
   - Penjelasan arsitektur RTC domain, ULP coprocessor, RTC memory, dan mekanisme pengelolaan daya rendah pada level hardware ESP32.

5. **"Designing Ultra-Low-Power IoT Devices"**
   - Konsep-konsep perancangan perangkat IoT berdaya sangat rendah meliputi: duty cycling, power gating, clock gating, energy harvesting, dan strategi optimasi daya pada level firmware maupun hardware.

6. **"Making Embedded Systems" — Elecia White (O'Reilly Media)**
   - [https://www.oreilly.com/library/view/making-embedded-systems/9781449308889/](https://www.oreilly.com/library/view/making-embedded-systems/9781449308889/)
   - Buku panduan praktis pengembangan sistem embedded yang mencakup bab mengenai pengelolaan daya, pemilihan komponen hemat energi, dan teknik debugging konsumsi arus.

---

## Referensi Online

1. **Espressif ESP-IDF API Reference — `esp_sleep`**
   - [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html)
   - API untuk konfigurasi dan eksekusi Light Sleep & Deep Sleep, termasuk `esp_deep_sleep_start()`, `esp_light_sleep_start()`, dan berbagai fungsi wake-up source.

2. **Espressif ESP-IDF API Reference — `esp_pm` (Power Management)**
   - [https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/power_management.html)
   - API Dynamic Frequency Scaling (DFS) dan power management lock untuk mengelola frekuensi CPU dan konsumsi daya secara dinamis.

3. **STM32 HAL Driver Documentation — PWR & RCC**
   - [https://www.st.com/en/embedded-software/stm32cubef1.html](https://www.st.com/en/embedded-software/stm32cubef1.html)
   - Dokumentasi HAL driver untuk modul Power Control (PWR) dan Reset & Clock Control (RCC), termasuk fungsi `HAL_PWR_EnterSLEEPMode()`, `HAL_PWR_EnterSTOPMode()`, dan `HAL_PWR_EnterSTANDBYMode()`.

4. **Random Nerd Tutorials — ESP32 Deep Sleep**
   - [https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/](https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/)
   - Tutorial praktis penggunaan Deep Sleep pada ESP32 dengan berbagai sumber wake-up (timer, touch pin, external wake-up) menggunakan Arduino framework.

5. **STM32 Low-Power Modes Application Notes**
   - **AN2629** — STM32F101xx dan STM32F103xx low-power modes
     - [https://www.st.com/resource/en/application_note/an2629-stm32f101xx-and-stm32f103xx-lowpower-modes-stmicroelectronics.pdf](https://www.st.com/resource/en/application_note/an2629-stm32f101xx-and-stm32f103xx-lowpower-modes-stmicroelectronics.pdf)
   - **AN3193** — Getting started with STM32F1/F2/F4 low-power modes
     - [https://www.st.com/resource/en/application_note/an3193-stm32f1-stm32f2-stm32f4-lowpower-modes-stmicroelectronics.pdf](https://www.st.com/resource/en/application_note/an3193-stm32f1-stm32f2-stm32f4-lowpower-modes-stmicroelectronics.pdf)
   - Panduan aplikasi resmi dari ST untuk memahami dan mengimplementasikan mode daya rendah pada mikrokontroler STM32.

6. **PlatformIO Documentation**
   - ESP32: [https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
   - STM32: [https://docs.platformio.org/en/latest/boards/ststm32/bluepill_f103c8.html](https://docs.platformio.org/en/latest/boards/ststm32/bluepill_f103c8.html)
   - Dokumentasi konfigurasi board, framework, dan opsi build pada PlatformIO untuk kedua platform yang digunakan dalam praktikum.

---

## Tools & Software

1. **PlatformIO IDE**
   - [https://platformio.org/](https://platformio.org/)
   - Lingkungan pengembangan terintegrasi (IDE) berbasis VS Code untuk pengembangan embedded system. Mendukung ESP-IDF, Arduino, dan STM32 HAL framework dengan manajemen library dan build system otomatis.

2. **Python Matplotlib — Analisis Daya**
   - [https://matplotlib.org/](https://matplotlib.org/)
   - Library visualisasi data Python yang digunakan untuk membuat grafik konsumsi arus, profil daya, dan analisis duty cycle dari data pengukuran sensor arus.

3. **INA219 Current Sensor Library**
   - Adafruit INA219: [https://github.com/adafruit/Adafruit_INA219](https://github.com/adafruit/Adafruit_INA219)
   - Library untuk sensor arus/tegangan INA219 berbasis I2C. Digunakan untuk mengukur konsumsi arus secara real-time pada berbagai mode operasi mikrokontroler.

4. **Nordic Power Profiler Kit (PPK2) Documentation**
   - [https://www.nordicsemi.com/Products/Development-hardware/Power-Profiler-Kit-2](https://www.nordicsemi.com/Products/Development-hardware/Power-Profiler-Kit-2)
   - Alat pengukuran daya presisi tinggi dari Nordic Semiconductor untuk profiling konsumsi arus perangkat embedded pada rentang nanoampere hingga ampere. Dokumentasi mencakup penggunaan perangkat keras dan software pendamping.

5. **STM32CubeMonitor-Power**
   - [https://www.st.com/en/development-tools/stm32cubemonpwr.html](https://www.st.com/en/development-tools/stm32cubemonpwr.html)
   - Tool resmi dari STMicroelectronics untuk monitoring dan analisis konsumsi daya secara real-time pada board STM32. Mendukung pengukuran arus dinamis dan visualisasi profil daya.

---

## Datasheet & Application Notes

1. **ESP32 Datasheet — Bagian Konsumsi Daya**
   - [https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
   - Tabel konsumsi arus pada berbagai mode: Active (RF TX/RX), Modem-Sleep, Light-Sleep, Deep-Sleep, dan Hibernation. Data penting untuk estimasi masa pakai baterai.

2. **STM32F103 Low-Power Application Notes**
   - AN2629 & AN3193 (lihat bagian Referensi Online di atas)
   - Panduan lengkap implementasi mode Sleep, Stop, dan Standby beserta contoh kode, diagram alir, dan pengukuran konsumsi arus aktual.

3. **Datasheet Baterai Li-Ion/LiPo & Profil Pengisian**
   - Spesifikasi umum sel baterai Li-Ion/LiPo:
     - Tegangan nominal: 3.7V, tegangan penuh: 4.2V, tegangan cut-off: 3.0V
     - Profil pengisian CC-CV (Constant Current — Constant Voltage)
     - Kapasitas (mAh), C-rate, siklus hidup, dan kurva discharge
   - Referensi desain charging circuit menggunakan IC seperti TP4056, MCP73831
   - Pertimbangan keamanan: proteksi overcharge, overdischarge, dan overcurrent

4. **Perbandingan LDO vs Buck Converter untuk Sistem Embedded**
   - **LDO (Low-Dropout Regulator)**:
     - Kelebihan: desain sederhana, noise rendah, ukuran kecil
     - Kekurangan: efisiensi rendah pada perbedaan tegangan input-output besar (daya terbuang sebagai panas)
     - Cocok untuk: aplikasi low-noise (sensor analog), beban arus kecil, perbedaan tegangan kecil
   - **Buck Converter (Step-Down Switching Regulator)**:
     - Kelebihan: efisiensi tinggi (85–95%), cocok untuk beban arus besar
     - Kekurangan: ripple & noise lebih tinggi, desain PCB lebih kompleks, memerlukan induktor
     - Cocok untuk: konversi tegangan besar (misal 12V → 3.3V), aplikasi battery-powered yang membutuhkan efisiensi tinggi
   - Referensi: Application Note TI — *"Choosing Between an LDO and a DC-DC Converter"*
     - [https://www.ti.com/lit/an/slva057/slva057.pdf](https://www.ti.com/lit/an/slva057/slva057.pdf)

---

## Catatan Penggunaan

- Semua referensi di atas digunakan sebagai acuan dalam penyusunan materi kuliah, jobsheet praktikum, dan proyek akhir Modul 14.
- Mahasiswa diharapkan membaca minimal dokumentasi resmi ESP-IDF (Sleep Modes) dan STM32 Reference Manual (bab PWR) sebelum mengerjakan praktikum.
- Link referensi dapat berubah sewaktu-waktu. Jika link tidak aktif, silakan cari judul dokumen melalui mesin pencari.
- Untuk pemahaman mendalam tentang power management, disarankan membaca Application Notes dari ST (AN2629, AN3193) dan datasheet ESP32 secara lengkap.
