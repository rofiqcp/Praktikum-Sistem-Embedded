# Jobsheet Modul 09: FreeRTOS Middle - GPIO, Interrupt, Encoder, Serial, DAC, ADC, I2C, SPI

## 1. Tujuan Praktikum

Setelah menyelesaikan jobsheet Modul 09, mahasiswa mampu:

1. Mengimplementasikan FreeRTOS tasks untuk GPIO, interrupt, encoder, serial, DAC, ADC, I2C, dan SPI pada ESP32 dan STM32.
2. Memahami sinkronisasi task, penggunaan semaphore, queue, dan event group dalam RTOS.
3. Mengimplementasikan tepat 20 eksperimen final: **10 STM32 + 10 ESP32**.
4. Mengintegrasikan 5 multi-task dengan komunikasi antar-MCU menggunakan RTOS.
5. Membangun sistem sensor hub berbasis RTOS.

---

## 2. Peralatan

| Komponen | Jumlah | Catatan |
|---|---:|---|
| ESP32 DevKit | 1 | FreeRTOS native, GPIO matrix |
| STM32 Blue Pill/Black Pill | 1 | FreeRTOS via CMSIS-RTOS atau STM32Cube |
| Rotary Encoder | 1 | 2-pin interrupt untuk STM32 dan ESP32 |
| Potensiometer | 1 | untuk ADC RTOS |
| LED | 3 | indikator task STM32/ESP32 |
| Resistor 220Ω | 3 | current limiting LED |
| Sensor suhu LM35 | 1 opsional | input ADC |
| Modul DAC (MCP4725) | 1 opsional | I2C DAC |
| Sensor I2C (BME280) | 1 | komunikasi I2C RTOS |
| Modul SPI (SD Card/ADXL345) | 1 | komunikasi SPI RTOS |
| Breadboard, jumper, USB | sesuai | common ground wajib |
| Logic analyzer/oscilloscope | opsional | validasi timing RTOS |

---

## 3. Daftar Final 25 Eksperimen

| No | Kode | Kelompok | Judul |
|---:|---|---|---|
| 1 | STM32_01 | STM32 | GPIO Task Blink RTOS |
| 2 | STM32_02 | STM32 | External Interrupt RTOS |
| 3 | STM32_03 | STM32 | Encoder 2-Pin Interrupt RTOS |
| 4 | STM32_04 | STM32 | Serial Communication RTOS |
| 5 | STM32_05 | STM32 | DAC dengan RTOS |
| 6 | STM32_06 | STM32 | ADC dengan RTOS |
| 7 | STM32_07 | STM32 | I2C dengan RTOS |
| 8 | STM32_08 | STM32 | SPI dengan RTOS |
| 9 | STM32_09 | STM32 | Multi Task GPIO-ADC-UART |
| 10 | STM32_10 | STM32 | FreeRTOS Semaphore Mutex |
| 11 | ESP32_01 | ESP32 | GPIO Task Blink RTOS |
| 12 | ESP32_02 | ESP32 | External Interrupt RTOS |
| 13 | ESP32_03 | ESP32 | Encoder 2-Pin Interrupt RTOS |
| 14 | ESP32_04 | ESP32 | Serial Communication RTOS |
| 15 | ESP32_05 | ESP32 | DAC dengan RTOS |
| 16 | ESP32_06 | ESP32 | ADC dengan RTOS |
| 17 | ESP32_07 | ESP32 | I2C dengan RTOS |
| 18 | ESP32_08 | ESP32 | SPI dengan RTOS |
| 19 | ESP32_09 | ESP32 | Multi Task GPIO-ADC-UART |
| 20 | ESP32_10 | ESP32 | FreeRTOS Semaphore Mutex |
| 21 | MULTI_01 | Multi | RTOS GPIO Task Sync |
| 22 | MULTI_02 | Multi | RTOS ADC-DAC Communication |
| 23 | MULTI_03 | Multi | RTOS I2C Sensor Sharing |
| 24 | MULTI_04 | Multi | RTOS SPI Data Exchange |
| 25 | MULTI_05 | Multi | RTOS Sensor Hub System |

---

## 4. Eksperimen STM32

### STM32_01 — GPIO Task Blink RTOS

**Tujuan:** membuat task RTOS untuk mengendalikan LED dengan delay non-blocking.

**Hardware:** STM32, LED di PC13/PA5.

**Langkah:**
1. Init FreeRTOS dengan CMSIS-RTOS atau STM32Cube.
2. Buat task `vLEDTask` dengan delay `osDelay(500)`.
3. Buat task `vMonitorTask` untuk cetak status via UART.
4. Jalankan scheduler.
5. Amati blink LED dan serial output.

**Output diharapkan:** LED blink 1 Hz, serial menampilkan status task.

---

### STM32_02 — External Interrupt RTOS

**Tujuan:** menangani external interrupt dengan semaphore dari ISR ke task RTOS.

**Hardware:** STM32, push button di PA0 (EXTI0).

**Langkah:**
1. Konfigurasi EXTI0 pada PA0 (falling edge).
2. Buat binary semaphore `xSemaphoreCreateBinary()`.
3. Dalam ISR EXTI0, berikan semaphore `xSemaphoreGiveFromISR()`.
4. Task menunggu semaphore `xSemaphoreTake()`.
5. Toggle LED saat semaphore diterima.

**Output diharapkan:** LED toggle setiap tombol ditekan, tanpa debounce blocking.

---

### STM32_03 — Encoder 2-Pin Interrupt RTOS

**Tujuan:** membaca rotary encoder 2-pin (A dan B) dengan interrupt RTOS.

**Hardware:** STM32, rotary encoder di PB0 (A) dan PB1 (B).

**Langkah:**
1. Konfigurasi EXTI untuk PB0 dan PB1.
2. Gunakan counting semaphore atau queue untuk kirim event rotasi.
3. Task encoder memproses arah rotasi (CW/CCW).
4. Update counter dan tampilkan via UART.
5. Handle debounce di task level, bukan ISR.

**Output diharapkan:** serial menampilkan arah dan nilai counter encoder.

---

### STM32_04 — Serial Communication RTOS

**Tujuan:** komunikasi UART dengan RTOS menggunakan queue.

**Hardware:** STM32, USB-UART atau USART2.

**Langkah:**
1. Init UART dengan interrupt RX.
2. Buat queue `xQueueCreate(10, sizeof(char))`.
3. ISR UART RX masukkan karakter ke queue.
4. Task receiver baca queue dan proses command.
5. Task transmitter kirim respon via UART.

**Output diharapkan:** echo karakter dan respon command dari STM32.

---

### STM32_05 — DAC dengan RTOS

**Tujuan:** menghasilkan sinyal DAC dengan task RTOS.

**Hardware:** STM32 dengan DAC (PA4/PA5), osiloskop.

**Langkah:**
1. Init DAC dengan timer trigger atau software trigger.
2. Buat task yang menulis nilai DAC 0-4095 secara periodik.
3. Generate sin wave atau triangle wave menggunakan lookup table.
4. Gunakan delay task untuk frekuensi output.
5. Amati bentuk gelombang di osiloskop.

**Output diharapkan:** sinyal analog periodik di pin DAC.

---

### STM32_06 — ADC dengan RTOS

**Tujuan:** membaca ADC dengan task RTOS dan menyampaikan hasil via queue.

**Hardware:** STM32, potensiometer di PA0 (ADC1_IN0).

**Langkah:**
1. Init ADC dengan polling atau interrupt.
2. Buat task ADC yang baca nilai tiap 100 ms.
3. Kirim hasil ADC ke queue `xQueueSend()`.
4. Task display terima data dan tampilkan via UART.
5. Konversi ke tegangan (0-3.3V) dan tampilkan.

**Output diharapkan:** serial menampilkan nilai ADC dan tegangan real-time.

---

### STM32_07 — I2C dengan RTOS

**Tujuan:** membaca sensor I2C dengan task RTOS dan mutex.

**Hardware:** STM32, sensor I2C (BME280/LM75).

**Langkah:**
1. Init I2C dengan HAL.
2. Buat mutex `xSemaphoreCreateMutex()` untuk akses I2C.
3. Task sensor baca data I2C dengan ambil mutex.
4. Task display tampilkan data via UART.
5. Pastikan mutual exclusion saat akses bus I2C.

**Output diharapkan:** data sensor I2C terbaca dan ditampilkan.

---

### STM32_08 — SPI dengan RTOS

**Tujuan:** komunikasi SPI dengan task RTOS.

**Hardware:** STM32, modul SPI (SD Card/ADXL345).

**Langkah:**
1. Init SPI dengan HAL (polling/interrupt/DMA).
2. Buat task SPI untuk transfer data.
3. Gunakan mutex untuk akses SPI bersama.
4. Baca register device atau tulis data.
5. Tampilkan hasil via UART.

**Output diharapkan:** data SPI berhasil dibaca/ditulis.

---

### STM32_09 — Multi Task GPIO-ADC-UART

**Tujuan:** mengintegrasikan multiple task RTOS: GPIO, ADC, UART.

**Hardware:** STM32, LED, potensiometer.

**Langkah:**
1. Buat task LED blink.
2. Buat task ADC reader dengan queue ke task UART.
3. Buat task UART receiver untuk command.
4. Gunakan event group untuk sinkronisasi task.
5. Implementasikan command parsing (LED ON/OFF, ADC start/stop).

**Output diharapkan:** sistem multi-task berjalan stabil dengan command UART.

---

### STM32_10 — FreeRTOS Semaphore Mutex

**Tujuan:** demonstrasi penggunaan semaphore dan mutex untuk resource sharing.

**Hardware:** STM32, 2 LED, UART.

**Langkah:**
1. Buat mutex untuk akses UART (shared resource).
2. Buat 2 task yang mengakses UART secara bergantian.
3. Gunakan binary semaphore untuk task synchronization.
4. Counting semaphore untuk buffer pool.
5. Amati urutan akses resource via serial.

**Output diharapkan:** tidak ada race condition pada akses UART.

---

## 5. Eksperimen ESP32

### ESP32_01 — GPIO Task Blink RTOS

**Tujuan:** membuat task FreeRTOS native ESP32 untuk blink LED.

**Hardware:** ESP32, LED di GPIO2.

**Langkah:**
1. Buat task dengan `xTaskCreate()`.
2. Task blink dengan `vTaskDelay(pdMS_TO_TICKS(500))`.
3. Buat task monitor dengan cetak status via UART.
4. Jalankan scheduler (otomatis di ESP32).
5. Amati blink dan serial output.

**Output diharapkan:** LED blink 1 Hz, serial menampilkan status.

---

### ESP32_02 — External Interrupt RTOS

**Tujuan:** menangani interrupt GPIO dengan semaphore FreeRTOS.

**Hardware:** ESP32, push button di GPIO0.

**Langkah:**
1. Konfigurasi GPIO interrupt (falling edge).
2. Buat binary semaphore `xSemaphoreCreateBinary()`.
3. ISR berikan semaphore `xSemaphoreGiveFromISR()`.
4. Task tunggu semaphore dan toggle LED.
5. Hindari debounce di ISR.

**Output diharapkan:** LED toggle pada interrupt tombol.

---

### ESP32_03 — Encoder 2-Pin Interrupt RTOS

**Tujuan:** membaca rotary encoder dengan dual interrupt RTOS.

**Hardware:** ESP32, encoder di GPIO4 (A) dan GPIO5 (B).

**Langkah:**
1. Konfigurasi GPIO interrupt untuk pin A dan B.
2. Gunakan queue untuk kirim event rotasi ke task.
3. Task hitung arah CW/CCW berdasarkan state.
4. Update counter dan tampilkan via UART.
5. Handle debounce di task level.

**Output diharapkan:** arah dan nilai counter encoder akurat.

---

### ESP32_04 — Serial Communication RTOS

**Tujuan:** UART dengan queue FreeRTOS ESP32.

**Hardware:** ESP32, USB-UART.

**Langkah:**
1. Init UART driver dengan `uart_driver_install()`.
2. Buat queue untuk RX data.
3. Task receiver baca UART event.
4. Task transmitter kirim respon.
5. Implementasikan simple command parser.

**Output diharapkan:** echo dan respon command via serial.

---

### ESP32_05 — DAC dengan RTOS

**Tujuan:** generate sinyal DAC dengan task ESP32.

**Hardware:** ESP32 dengan DAC channel 1 (GPIO25) atau channel 2 (GPIO26).

**Langkah:**
1. Init DAC dengan `dac_output_enable()`.
2. Buat task yang tulis nilai DAC 0-255 periodik.
3. Generate sin wave dengan lookup table.
4. Gunakan `vTaskDelay()` untuk kontrol frekuensi.
5. Amati output di osiloskop.

**Output diharapkan:** sinyal analog periodik di pin DAC ESP32.

---

### ESP32_06 — ADC dengan RTOS

**Tujuan:** membaca ADC ESP32 dengan task RTOS.

**Hardware:** ESP32, potensiometer di GPIO34 (ADC1_CH6).

**Langkah:**
1. Init ADC dengan `adc1_config_width()` dan `adc1_config_channel_atten()`.
2. Buat task yang baca ADC tiap 100 ms.
3. Kirim hasil ke queue untuk task display.
4. Konversi ke tegangan dan tampilkan via UART.
5. Handle kalibrasi ADC jika tersedia.

**Output diharapkan:** nilai ADC dan tegangan real-time di serial.

---

### ESP32_07 — I2C dengan RTOS

**Tujuan:** membaca sensor I2C dengan RTOS dan mutex ESP32.

**Hardware:** ESP32, sensor I2C (BME280/BMP280).

**Langkah:**
1. Init I2C master dengan `i2c_param_config()`.
2. Buat mutex `xSemaphoreCreateMutex()` untuk bus I2C.
3. Task sensor baca data dengan proteksi mutex.
4. Task display tampilkan data.
5. Pastikan akses I2C thread-safe.

**Output diharapkan:** data sensor I2C tampil di serial.

---

### ESP32_08 — SPI dengan RTOS

**Tujuan:** komunikasi SPI dengan task RTOS ESP32.

**Hardware:** ESP32, modul SPI (SD Card/ADXL345).

**Langkah:**
1. Init SPI bus dengan `spi_bus_initialize()`.
2. Tambah device dengan `spi_bus_add_device()`.
3. Buat task SPI transfer dengan mutex protection.
4. Baca/tulis register device.
5. Tampilkan hasil via UART.

**Output diharapkan:** data SPI berhasil ditransfer.

---

### ESP32_09 — Multi Task GPIO-ADC-UART

**Tujuan:** integrasi multi-task ESP32: GPIO, ADC, UART.

**Hardware:** ESP32, LED, potensiometer.

**Langkah:**
1. Buat task LED blink.
2. Buat task ADC reader dengan queue ke UART task.
3. Buat task UART command parser.
4. Gunakan event group untuk sinkronisasi.
5. Implementasikan kontrol LED dan ADC via command.

**Output diharapkan:** sistem multi-task stabil dengan kontrol UART.

---

### ESP32_10 — FreeRTOS Semaphore Mutex

**Tujuan:** demonstrasi semaphore/mutex ESP32 untuk resource sharing.

**Hardware:** ESP32, 2 LED, UART.

**Langkah:**
1. Buat mutex untuk akses UART shared resource.
2. Buat 2 task dengan akses UART bergantian.
3. Gunakan binary semaphore untuk task sync.
4. Counting semaphore untuk buffer management.
5. Amati urutan akses via serial monitor.

**Output diharapkan:** akses resource teratur tanpa race condition.

---

## 6. Eksperimen Multi STM32-ESP32

### MULTI_01 — RTOS GPIO Task Sync

**Tujuan:** sinkronisasi task GPIO antara STM32 dan ESP32 via UART.

**Hardware:** STM32, ESP32, LED di kedua sisi.

**Langkah:**
1. STM32 task blink LED dengan delay tertentu.
2. ESP32 task blink LED dengan sinkronisasi waktu.
3. Komunikasi UART antar-MCU untuk sync timing.
4. Gunakan protocol sederhana: START, SYNC, ACK.
5. Amati keselarasan blink LED.

**Output diharapkan:** LED kedua MCU blink secara sinkron.

---

### MULTI_02 — RTOS ADC-DAC Communication

**Tujuan:** STM32 baca ADC, kirim ke ESP32 untuk DAC output.

**Hardware:** STM32, ESP32, potensiometer, LED.

**Langkah:**
1. STM32 task ADC baca potensiometer.
2. STM32 kirim nilai ADC ke ESP32 via UART.
3. ESP32 terima data dan set DAC output.
4. Gunakan queue di kedua sisi.
5. Tampilkan nilai di serial monitor.

**Output diharapkan:** DAC ESP32 mengikuti potensiometer STM32.

---

### MULTI_03 — RTOS I2C Sensor Sharing

**Tujuan:** berbagi akses sensor I2C antara STM32 dan ESP32.

**Hardware:** STM32, ESP32, sensor I2C (BME280), UART antar-MCU.

**Langkah:**
1. STM32 baca sensor I2C dengan RTOS.
2. ESP32 request data sensor via UART.
3. STM32 kirim data sensor ke ESP32.
4. ESP32 tampilkan data di serial.
5. Gunakan semaphore/mutex untuk koordinasi.

**Output diharapkan:** data sensor dapat diakses kedua MCU.

---

### MULTI_04 — RTOS SPI Data Exchange

**Tujuan:** pertukaran data SPI antar STM32 dan ESP32 via UART bridge.

**Hardware:** STM32, ESP32, modul SPI, UART.

**Langkah:**
1. STM32 baca data SPI (misal ADXL345).
2. STM32 kirim data ke ESP32 via UART.
3. ESP32 proses data dan kirim command balik.
4. Implementasikan protokol data exchange.
5. Tampilkan data di kedua sisi.

**Output diharapkan:** data SPI tersedia di ESP32 untuk diproses.

---

### MULTI_05 — RTOS Sensor Hub System

**Tujuan:** integrasi sistem sensor hub RTOS dengan STM32 dan ESP32.

**Hardware:** STM32 (sensor node), ESP32 (gateway), I2C sensor, ADC, UART.

**Langkah:**
1. STM32 baca ADC dan sensor I2C dengan RTOS.
2. STM32 kirim data teraggregasi ke ESP32.
3. ESP32 tampilkan data hub dan kirim command.
4. Implementasikan error handling dan recovery.
5. Demo sistem sensor hub lengkap.

**Output diharapkan:** sistem sensor hub RTOS stabil dengan komunikasi MCU.

---

## 7. Format Pengamatan Minimal

Untuk tiap eksperimen, catat:

| Item | Nilai |
|---|---|
| Kode eksperimen | |
| Platform | STM32 / ESP32 / Multi |
| Task yang dibuat | |
| FreeRTOS primitive | |
| Interrupt digunakan | |
| Output serial/display | |
| Error yang muncul | |
| Solusi | |

---

## 8. Pertanyaan Analisis Wajib

1. Apa perbedaan task, semaphore, mutex, dan queue di FreeRTOS?
2. Mengapa ISR tidak boleh melakukan operasi blocking?
3. Kapan menggunakan mutex vs semaphore?
4. Bagaimana menangani encoder 2-pin dengan interrupt?
5. Mengapa DAC/ADC perlu RTOS task terpisah?
6. Bagaimana I2C/SPI sharing antar task dengan mutex?
7. Apa keuntungan RTOS dibanding superloop?
8. Bagaimana sinkronisasi antar-MCU dengan RTOS?
9. Bagaimana handling debounce encoder tanpa blocking ISR?
10. Apa peran event group dalam multi-task?

---

## 9. Deliverable Laporan

1. Cover dan identitas.
2. Ringkasan teori FreeRTOS Middle.
3. Tabel hasil **25 eksperimen**.
4. Foto wiring tiap kelompok eksperimen.
5. Screenshot serial monitor.
6. Analisis error dan troubleshooting.
7. Source code utama atau link repository.
8. Integrasi project sensor hub.
9. Kesimpulan.
