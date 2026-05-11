# Jobsheet Modul 08: SPI Bus dan Komunikasi STM32-ESP32

## 1. Tujuan Praktikum

Setelah menyelesaikan jobsheet Modul 08, mahasiswa mampu:

1. Menggunakan SPI pada ESP32 dan STM32 untuk berbagai device (Flash, SD Card, OLED, ADC, DAC, Sensor).
2. Memahami clock polarity (CPOL), clock phase (CPHA), full-duplex, half-duplex, simplex, chip select (CS/SS), daisy-chain, dan multi-slave configuration.
3. Mengimplementasikan tepat 25 eksperimen final: **7 STM32 SPI, 3 STM32 SPI RTOS, 7 ESP32 SPI, 3 ESP32 SPI RTOS, 3 Multi STM32-ESP32 SPI non-RTOS, 2 Multi STM32-ESP32 SPI RTOS**.
4. Mengintegrasikan hasil praktikum menjadi project SPI Data Acquisition System dual-MCU.

---

## 2. Peralatan

| Komponen | Jumlah | Catatan |
|---|---:|---|
| ESP32 DevKit | 1 | SPI default: MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18; CS dapat dipilih GPIO apa saja |
| STM32 Blue Pill/Black Pill | 1 | SPI1: SCK=PA5, MISO=PA6, MOSI=PA7; SPI2: SCK=PB13, MISO=PB14, MOSI=PB15 |
| W25Q64 SPI Flash | 1 | 8 MB Flash, CS aktif LOW |
| Micro SD Card Module | 1 | SPI mode, CS aktif LOW |
| SSD1306 OLED 128×64 SPI | 1 | SPI interface, CS, DC, RES pins |
| MCP3008 ADC | 1 | 10-bit 8-channel ADC, SPI |
| MCP4921 DAC | 1 | 12-bit single DAC, SPI |
| ADXL345 Accelerometer | 1 | SPI/I2C, gunakan mode SPI |
| RFID RC522 | 1 opsional | SPI RFID reader |
| Resistor 10 kΩ | 4 | pull-up untuk CS lines |
| Breadboard, jumper, USB | sesuai | common ground wajib |
| Logic analyzer | opsional | validasi waveform SPI |
| PC dengan Python | 1 | untuk GUI_SPI.py (tkinter SPI monitor) |

---

## 3. Aturan Umum Wiring

### ESP32

| Sinyal | Default | Alternatif |
|---|---|---|
| MOSI | GPIO23 | hampir semua GPIO valid via GPIO matrix |
| MISO | GPIO19 | hampir semua GPIO valid via GPIO matrix |
| SCLK | GPIO18 | hampir semua GPIO valid via GPIO matrix |
| CS | GPIO5 (default) | GPIO apa saja via GPIO matrix |

### STM32

| Sinyal | SPI1 | SPI2 | SPI3 (jika ada) |
|---|---|---|---|
| SCK | PA5 | PB13 | PC10 |
| MISO | PA6 | PB14 | PC11 |
| MOSI | PA7 | PB15 | PC12 |
| CS | PA4/user | PB12/user | PA15/user |

Aturan:

- Semua GND disatukan.
- CS (Chip Select) aktif LOW - device diaktifkan saat CS=LOW.
- Setiap slave membutuhkan CS tersendiri dari master.
- SPI Flash W25Q64, SD Card, OLED, ADC, DAC, ADXL345 dapat digabung dalam satu bus SPI dengan CS berbeda.
- Clock polarity (CPOL) dan clock phase (CPHA) harus sesuai datasheet device.
- Mode 0: CPOL=0, CPHA=0 (rising edge sample, falling edge setup).
- Mode 3: CPOL=1, CPHA=1 (rising edge setup, falling edge sample).
- Jalankan scanner/tes sederhana sebelum eksperimen device lengkap.

---

## 4. Daftar Final 25 Eksperimen

| No | Kode | Kelompok | Judul |
|---:|---|---|---|
| 1 | STM32_01 | STM32 SPI | SPI Bus Scanner dan Device Detection |
| 2 | STM32_02 | STM32 SPI | W25Q64 Flash Read/Write |
| 3 | STM32_03 | STM32 SPI | Micro SD Card File System |
| 4 | STM32_04 | STM32 SPI | SSD1306 OLED SPI Graphics |
| 5 | STM32_05 | STM32 SPI | MCP3008 ADC Multi-Channel |
| 6 | STM32_06 | STM32 SPI | MCP4921 DAC Signal Generation |
| 7 | STM32_07 | STM32 SPI | ADXL345 Accelerometer SPI |
| 8 | STM32_08 | STM32 SPI RTOS | SPI RTOS Multi Device |
| 9 | STM32_09 | STM32 SPI RTOS | SPI RTOS DMA Transfer |
| 10 | STM32_10 | STM32 SPI RTOS | SPI RTOS Data Logger |
| 11 | ESP32_01 | ESP32 SPI | SPI Bus Master Configuration |
| 12 | ESP32_02 | ESP32 SPI | W25Q64 Flash ID dan Memory Map |
| 13 | ESP32_03 | ESP32 SPI | Micro SD Card SPI Mode |
| 14 | ESP32_04 | ESP32 SPI | SSD1306 OLED SPI Display |
| 15 | ESP32_05 | ESP32 SPI | MCP3008 ADC Read Potentiometer |
| 16 | ESP32_06 | ESP32 SPI | MCP4921 DAC Waveform Output |
| 17 | ESP32_07 | ESP32 SPI | ADXL345 SPI Acceleration |
| 18 | ESP32_08 | ESP32 SPI RTOS | SPI RTOS Multi Task |
| 19 | ESP32_09 | ESP32 SPI RTOS | SPI RTOS DMA Benchmark |
| 20 | ESP32_10 | ESP32 SPI RTOS | SPI RTOS Interrupt Driven |
| 21 | MULTI_01 | Multi SPI | SPI Master-Slave Basic |
| 22 | MULTI_02 | Multi SPI | SPI Multi Slave CS Management |
| 23 | MULTI_03 | Multi SPI | SPI Shared Bus Multi Device |
| 24 | MULTI_04 | Multi SPI RTOS | SPI RTOS Gateway |
| 25 | MULTI_05 | Multi SPI RTOS | SPI Data Acquisition System |

---

## 5. Eksperimen STM32 SPI

### STM32_01 — SPI Bus Scanner dan Device Detection

**Tujuan:** mendeteksi device SPI yang terhubung dengan membaca ID register atau status.

**Hardware:** STM32, W25Q64 Flash, SSD1306 OLED, MCP3008, MCP4921, ADXL345.

**Wiring:** SPI1 PA5/PA6/PA7, CS masing-masing device: Flash=PA4, OLED=PA3, ADC=PA2, DAC=PA1, ADXL=PA0.

**Langkah:**
1. Init SPI1 dengan mode 0, 8-bit, MSB first, 1 MHz clock.
2. Untuk W25Q64: set CS LOW, kirim command 0x9F (Read JEDEC ID), baca 3 byte (Manufacturer, Memory Type, Capacity).
3. Untuk SSD1306: set CS LOW, kirim command 0x81 (Set Contrast), baca status jika ada.
4. Untuk MCP3008: set CS LOW, kirim konfigurasi channel, baca hasil ADC.
5. Untuk MCP4921: set CS LOW, kirim data DAC, verifikasi output dengan ADC.
6. Untuk ADXL345: set CS LOW, baca register 0x00 (DEVID), harus 0xE5.
7. Catat semua device yang berhasil dideteksi.

**Output diharapkan:** serial menampilkan device terdeteksi beserta ID/nilai pembacaan.

---

### STM32_02 — W25Q64 Flash Read/Write

**Tujuan:** membaca dan menulis data ke SPI Flash W25Q64.

**Hardware:** STM32, W25Q64 Flash (CS=PA4).

**Wiring:** SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA4.

**Langkah:**
1. Init SPI1 mode 0, clock 10 MHz.
2. Baca JEDEC ID (command 0x9F) untuk verifikasi device.
3. Baca status register (command 0x05).
4. Write Enable (command 0x06).
5. Sector Erase (command 0x20) pada alamat 0x000000.
6. Page Program (command 0x02) dengan data 256 byte.
7. Read Data (command 0x03) pada alamat yang sama.
8. Verifikasi data tulis dan baca sama.

**Output diharapkan:** serial menampilkan ID Flash, status, dan verifikasi data berhasil.

---

### STM32_03 — Micro SD Card File System

**Tujuan:** mengakses kartu SD dengan SPI mode dan membuat file system sederhana.

**Hardware:** STM32, Micro SD Card Module (CS=PA4).

**Wiring:** SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA4.

**Langkah:**
1. Init SPI1 mode 0, clock 400 kHz (init) lalu naik ke 10 MHz.
2. Kirim command CMD0 (GO_IDLE_STATE) untuk reset SD card.
3. Kirim CMD8 (SEND_IF_COND) untuk cek voltage range.
4. Kirim ACMD41 untuk inisialisasi SD card ke SPI mode.
5. Baca CSD register untuk info kapasitas.
6. Baca blok data (CMD17) dari sektor 0.
7. Tulis blok data (CMD24) ke sektor tertentu.
8. Implementasikan FAT16/32 sederhana atau raw block access.

**Output diharapkan:** serial menampilkan tipe SD card, kapasitas, dan berhasil baca/tulis blok.

---

### STM32_04 — SSD1306 OLED SPI Graphics

**Tujuan:** menampilkan grafis pada OLED SSD1306 dengan interface SPI.

**Hardware:** STM32, SSD1306 OLED 128×64 SPI (CS=PA3, DC=PA2, RES=PA1).

**Wiring:** SCK=PA5, MOSI=PA7, CS=PA3, DC=PA2, RES=PA1.

**Langkah:**
1. Init SPI1 mode 0, clock 8 MHz.
2. Reset OLED via pin RES.
3. Kirim sequence inisialisasi SSD1306 (register display, contrast, dll).
4. Buat framebuffer 1024 byte (128×64/8).
5. Gambar teks, garis, bentuk geometri ke framebuffer.
6. Kirim framebuffer ke OLED via SPI.
7. Update display periodik dengan animasi sederhana.

**Output diharapkan:** OLED menampilkan grafis tanpa flicker berat.

---

### STM32_05 — MCP3008 ADC Multi-Channel

**Tujuan:** membaca 8 channel ADC dari MCP3008 via SPI.

**Hardware:** STM32, MCP3008 ADC (CS=PA2).

**Wiring:** SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA2.

**Langkah:**
1. Init SPI1 mode 0, clock 1 MHz.
2. Konfigurasi channel ADC: start bit (1), single-ended/differential (bit 7-6), channel (bit 5-3).
3. Kirim konfigurasi 5 bit lalu baca 10-bit hasil ADC (2 byte).
4. Baca semua channel 0-7 secara berurutan.
5. Konversi nilai ADC ke tegangan (Vref=3.3V).
6. Tampilkan hasil di serial monitor.

**Output diharapkan:** serial menampilkan nilai ADC 8 channel dalam satuan digital dan tegangan.

---

### STM32_06 — MCP4921 DAC Signal Generation

**Tujuan:** menghasilkan sinyal analog dari MCP4921 DAC via SPI.

**Hardware:** STM32, MCP4921 DAC (CS=PA1).

**Wiring:** SCK=PA5, MISO=PA6 (tidak dipakai), MOSI=PA7, CS=PA1.

**Langkah:**
1. Init SPI1 mode 0, clock 5 MHz.
2. Konfigurasi DAC: write command (bit 15-12), output gain (bit 13), output shutdown (bit 12), 12-bit data (bit 11-0).
3. Generate sinyal: ramp (0-4095), triangle, sine (menggunakan tabel lookup).
4. Update DAC dengan frekuensi tertentu (misal 1 kHz).
5. Ukur output DAC dengan ADC (MCP3008) untuk verifikasi.

**Output diharapkan:** sinyal analog terbentuk di output DAC, terverifikasi oleh ADC.

---

### STM32_07 — ADXL345 Accelerometer SPI

**Tujuan:** membaca akselerasi 3-axis dari ADXL345 via SPI.

**Hardware:** STM32, ADXL345 (CS=PA0).

**Wiring:** SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA0.

**Langkah:**
1. Init SPI1 mode 3 (CPOL=1, CPHA=1), clock 5 MHz.
2. Baca register DEVID (0x00), harus 0xE5.
3. Tulis register POWER_CTL (0x2D) = 0x08 (measure mode).
4. Tulis register DATA_FORMAT (0x31) = 0x08 (full resolution).
5. Baca 6 byte data (DATAX0, DATAX1, DATAY0, DATAY1, DATAZ0, DATAZ1).
6. Konversi ke nilai g (±2g, ±4g, ±8g, ±16g).
7. Hitung orientasi device.

**Output diharapkan:** serial menampilkan nilai akselerasi X, Y, Z dalam g saat sensor digerakkan.

---

## 6. Eksperimen STM32 SPI RTOS

### STM32_08 — SPI RTOS Multi Device

**Tujuan:** mengakses multi device SPI dengan FreeRTOS multi-task.

**Hardware:** STM32, W25Q64, SSD1306, MCP3008, MCP4921.

**Wiring:** SPI1 dengan CS masing-masing device.

**Langkah:**
1. Buat task RTOS untuk baca ADC periodik.
2. Buat task RTOS untuk update OLED display.
3. Buat task RTOS untuk tulis log ke Flash.
4. Gunakan mutex untuk akses SPI shared bus.
5. Gunakan queue untuk kirim data antar task.
6. Monitor CPU usage tiap task.

**Output diharapkan:** multi-task berjalan stabil tanpa conflict akses SPI.

---

### STM32_09 — SPI RTOS DMA Transfer

**Tujuan:** transfer SPI DMA dengan FreeRTOS pada STM32.

**Hardware:** STM32, W25Q64 Flash (CS=PA4).

**Wiring:** SPI1 SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA4, DMA channel/stream sesuai seri STM32.

**Langkah:**
1. Init SPI dengan DMA (hdmatx, hdmarx).
2. Buat task RTOS yang trigger DMA transfer ke Flash.
3. Pakai semaphore dari callback `HAL_SPI_TxCpltCallback`.
4. Bandingkan dengan polling mode.
5. Ukur CPU usage saat DMA berjalan.

**Output diharapkan:** DMA membebaskan CPU; transfer selesai via callback.

---

### STM32_10 — SPI RTOS Data Logger

**Tujuan:** logging data sensor ke W25Q64 Flash dengan FreeRTOS.

**Hardware:** STM32, MCP3008 ADC, W25Q64 Flash.

**Wiring:** SPI1 dengan CS ADC=PA2, CS Flash=PA4.

**Langkah:**
1. Buat task RTOS baca ADC periodik.
2. Simpan data ke buffer circular.
3. Task logger tulis buffer ke Flash dengan page program.
4. Gunakan semaphore untuk sync akses Flash.
5. Cek integritas data dengan CRC.

**Output diharapkan:** data ter-log kontinu dan dapat dibaca ulang setelah reset.

---

## 7. Eksperimen ESP32 SPI

### ESP32_01 — SPI Bus Master Configuration

**Tujuan:** mengkonfigurasi ESP32 sebagai SPI master dengan ESP-IDF.

**Hardware:** ESP32, W25Q64 Flash.

**Wiring:** MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18, CS=GPIO5.

**Langkah:**
1. Install ESP-IDF dan setup project.
2. Konfigurasi SPI bus dengan `spi_bus_initialize`.
3. Tambahkan device dengan `spi_bus_add_device` (mode 0, clock 10 MHz).
4. Siapkan transaksi SPI: `spi_transaction_t`.
5. Kirim command read JEDEC ID ke W25Q64.
6. Baca response dan tampilkan di serial.

**Output diharapkan:** serial menampilkan konfigurasi SPI berhasil dan ID Flash terbaca.

---

### ESP32_02 — W25Q64 Flash ID dan Memory Map

**Tujuan:** membaca ID dan memetakan memory W25Q64 Flash.

**Hardware:** ESP32, W25Q64 Flash (CS=GPIO5).

**Wiring:** MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18, CS=GPIO5.

**Langkah:**
1. Baca JEDEC ID (command 0x9F): Manufacturer=0xEF, Type=0x40, Capacity=0x17 (untuk W25Q64).
2. Baca Unique ID (command 0x4B) jika didukung.
3. Baca status register 1, 2, 3.
4. Tulis data test ke alamat 0x000000.
5. Baca ulang dan verifikasi.
6. Tampilkan memory map: total 8 MB = 128 blocks = 2048 sectors = 32768 pages.

**Output diharapkan:** serial menampilkan ID Flash lengkap dan memory map 8 MB.

---

### ESP32_03 — Micro SD Card SPI Mode

**Tujuan:** mengakses SD Card dengan SPI mode pada ESP32.

**Hardware:** ESP32, Micro SD Card Module (CS=GPIO5).

**Wiring:** MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18, CS=GPIO5.

**Langkah:**
1. Init SPI dengan clock 400 kHz untuk inisialisasi.
2. Reset SD card dengan CMD0.
3. Cek voltage dengan CMD8.
4. Inisialisasi dengan ACMD41 hingga idle.
5. Baca CSD register untuk kapasitas.
6. Baca dan tulis blok 512 byte.
7. Dapat menggunakan FatFS library untuk file system.

**Output diharapkan:** serial menampilkan tipe SD, kapasitas, dan berhasil akses blok.

---

### ESP32_04 — SSD1306 OLED SPI Display

**Tujuan:** menampilkan grafis pada OLED SSD1306 SPI.

**Hardware:** ESP32, SSD1306 OLED 128×64 SPI (CS=GPIO5, DC=GPIO16, RES=GPIO17).

**Wiring:** MOSI=GPIO23, SCLK=GPIO18, CS=GPIO5, DC=GPIO16, RES=GPIO17.

**Langkah:**
1. Init SPI bus mode 0, clock 8 MHz.
2. Reset OLED via pin RES.
3. Inisialisasi register SSD1306.
4. Buat framebuffer 1024 byte.
5. Gambar teks, bentuk, dan update display.
6. Tampilkan data sensor atau animasi.

**Output diharapkan:** OLED menampilkan grafis dan teks dengan jelas.

---

### ESP32_05 — MCP3008 ADC Read Potentiometer

**Tujuan:** membaca potentiometer via MCP3008 ADC.

**Hardware:** ESP32, MCP3008 (CS=GPIO5), potentiometer 10k.

**Wiring:** MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18, CS=GPIO5. Potentiometer terhubung ke channel 0 MCP3008.

**Langkah:**
1. Init SPI mode 0, clock 1 MHz.
2. Baca channel 0 MCP3008 dengan konfigurasi SPI.
3. Konversi nilai ADC (0-1023) ke tegangan (0-3.3V).
4. Baca semua 8 channel jika tersedia.
5. Tampilkan nilai di serial dan OLED.

**Output diharapkan:** nilai ADC berubah saat potentiometer diputar.

---

### ESP32_06 — MCP4921 DAC Waveform Output

**Tujuan:** menghasilkan waveform analog dari MCP4921 DAC.

**Hardware:** ESP32, MCP4921 DAC (CS=GPIO5).

**Wiring:** MOSI=GPIO23, SCLK=GPIO18, CS=GPIO5. Output DAC diukur dengan multimeter/oscilloscope.

**Langkah:**
1. Init SPI mode 0, clock 5 MHz.
2. Tulis register DAC dengan data 0-4095.
3. Generate waveform: ramp, triangle, sine.
4. Output DAC diukur atau dibaca balik dengan ADC.
5. Hitung frekuensi output berdasarkan delay antar sampel.

**Output diharapkan:** waveform analog terlihat di output DAC.

---

### ESP32_07 — ADXL345 SPI Acceleration

**Tujuan:** membaca akselerasi dari ADXL345 via SPI.

**Hardware:** ESP32, ADXL345 (CS=GPIO5).

**Wiring:** MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18, CS=GPIO5.

**Langkah:**
1. Init SPI mode 3 (CPOL=1, CPHA=1), clock 5 MHz.
2. Baca DEVID register (0x00) = 0xE5.
3. Set measure mode (POWER_CTL=0x08).
4. Baca data X, Y, Z (6 byte dari register 0x32).
5. Hitung nilai g dan orientasi.
6. Tampilkan di serial/OLED.

**Output diharapkan:** nilai akselerasi berubah saat ADXL345 digerakkan.

---

## 8. Eksperimen ESP32 SPI RTOS

### ESP32_08 — SPI RTOS Multi Task

**Tujuan:** multi-task akses SPI device dengan FreeRTOS.

**Hardware:** ESP32, W25Q64, SSD1306, MCP3008.

**Wiring:** SPI bus dengan CS masing-masing.

**Langkah:**
1. Buat task RTOS baca ADC.
2. Buat task RTOS update OLED.
3. Buat task RTOS log data ke Flash.
4. Gunakan mutex untuk akses SPI shared.
5. Gunakan queue untuk komunikasi antar task.

**Output diharapkan:** multi-task SPI berjalan bersama tanpa konflik.

---

### ESP32_09 — SPI RTOS DMA Benchmark

**Tujuan:** benchmark SPI transfer dengan DMA vs polling pada ESP32.

**Hardware:** ESP32, W25Q64 Flash.

**Wiring:** SPI dengan CS Flash.

**Langkah:**
1. Lakukan tulis/baca Flash dengan polling SPI.
2. Lakukan tulis/baca Flash dengan DMA SPI.
3. Ukur waktu transfer untuk data 256 byte, 1024 byte, 4096 byte.
4. Bandingkan CPU usage.
5. Tampilkan hasil benchmark di serial.

**Output diharapkan:** DMA lebih cepat dan mengurangi beban CPU dibanding polling.

---

### ESP32_10 — SPI RTOS Interrupt Driven

**Tujuan:** SPI transfer non-blocking dengan interrupt pada FreeRTOS.

**Hardware:** ESP32, W25Q64 Flash atau SSD1306.

**Wiring:** SPI dengan CS device.

**Langkah:**
1. Konfigurasi SPI dengan interrupt callback.
2. Buat task RTOS yang menunggu semaphore dari ISR.
3. Lakukan transfer SPI non-blocking.
4. Bandingkan dengan polling mode.
5. Ukur respon time dan CPU usage.

**Output diharapkan:** transfer SPI non-blocking, CPU bebas untuk task lain.

---

## 9. Eksperimen Multi STM32-ESP32 SPI

### MULTI_01 — SPI Master-Slave Basic

**Tujuan:** mengimplementasikan komunikasi SPI dimana STM32 berperan sebagai slave dan ESP32 sebagai master.

**Hardware:** ESP32, STM32, koneksi SPI.

**Wiring:** ESP32 MOSI=GPIO23 ↔ STM32 SPI1 MISO=PA6; ESP32 MISO=GPIO19 ↔ STM32 SPI1 MOSI=PA7; ESP32 SCLK=GPIO18 ↔ STM32 SPI1 SCK=PA5; ESP32 CS=GPIO5 → STM32 NSS/NSS_PIN. GND bersama.

**Langkah:**
1. Config STM32 sebagai SPI slave (hardware NSS atau software CS).
2. STM32 siapkan data buffer untuk dikirim ke master.
3. ESP32 sebagai master kirim command ke STM32 slave.
4. STM32 slave respon dengan data register virtual.
5. ESP32 baca data dari STM32 slave.

**Output diharapkan:** ESP32 berhasil baca data dari STM32 slave via SPI.

---

### MULTI_02 — SPI Multi Slave CS Management

**Tujuan:** mengelola multiple slave pada satu bus SPI dengan manajemen CS.

**Hardware:** ESP32 atau STM32 sebagai master, W25Q64, SSD1306, MCP3008 sebagai slave.

**Wiring:** Bus SPI bersama (MOSI, MISO, SCLK), CS masing-masing slave terpisah.

**Langkah:**
1. Master init SPI bus.
2. Tulis fungsi select slave (aktifkan CS slave tertentu, nonaktifkan CS slave lain).
3. Akses W25Q64 (CS Flash LOW, CS lain HIGH).
4. Akses SSD1306 (CS OLED LOW, CS lain HIGH).
5. Akses MCP3008 (CS ADC LOW, CS lain HIGH).
6. Demostrasikan multitasking akses slave berbeda.

**Output diharapkan:** semua slave dapat diakses tanpa konflik CS.

---

### MULTI_03 — SPI Shared Bus Multi Device

**Tujuan:** berbagi bus SPI antara ESP32 dan STM32 dengan device yang sama atau berbeda.

**Hardware:** ESP32, STM32, W25Q64, MCP3008, atau device SPI lain.

**Wiring:** ESP32 dan STM32 terhubung ke bus SPI yang sama dengan CS masing-masing.

**Langkah:**
1. ESP32 sebagai master utama, STM32 siap jika diperlukan.
2. Implementasikan protokol akses bus (polling atau request/grant).
3. ESP32 akses W25Q64 (CS=GPIO5).
4. STM32 akses MCP3008 (CS=PA2).
5. Pastikan CS tidak aktif bersamaan untuk slave yang sama.

**Output diharapkan:** tidak ada konflik saat akses shared bus.

---

## 10. Eksperimen Multi STM32-ESP32 SPI RTOS

### MULTI_04 — SPI RTOS Gateway

**Tujuan:** membuat gateway RTOS yang mengagregasi data dari multi SPI device dan mengirim ke host.

**Hardware:** ESP32, STM32, W25Q64, SSD1306, MCP3008, MCP4921.

**Wiring:** ESP32 dan STM32 pada bus SPI yang diatur dengan protokol komunikasi.

**Langkah:**
1. STM32 baca sensor (ADC, Accel) tiap 1 s dengan FreeRTOS task.
2. ESP32 request data dari STM32 via SPI.
3. ESP32 tampilkan di OLED dan kirim via UART ke PC.
4. Implementasikan queue/semaphore RTOS.
5. Monitor CPU usage.

**Output diharapkan:** data dari multi device teragregasi dan ditampilkan stabil.

---

### MULTI_05 — SPI Data Acquisition System

**Tujuan:** mengintegrasikan seluruh konsep Modul 08 dalam project Data Acquisition System RTOS.

**Hardware:** ESP32, STM32, W25Q64, SSD1306, MCP3008, MCP4921, ADXL345.

**Wiring:** sesuai desain; ESP32 dan STM32 komunikasi SPI; device SPI di bus bersama.

**Langkah:**
1. Init RTOS di ESP32 dan STM32.
2. Scan semua device di bus SPI.
3. Baca sensor periodik (ADC, Accel) dengan task RTOS.
4. Tampilkan data di OLED (ESP32) atau serial (STM32).
5. Log data ke W25Q64 Flash dengan RTOS.
6. Implementasikan error recovery RTOS.
7. ESP32 dan STM32 bertukar data via SPI master-slave.

**Output diharapkan:** Data Acquisition System dual-MCU RTOS berjalan stabil.

---

## 11. Format Pengamatan Minimal

Untuk tiap eksperimen, catat:

| Item | Nilai |
|---|---|
| Kode eksperimen | |
| Platform | STM32 / ESP32 / Multi |
| Device SPI | |
| Mode (CPOL/CPHA) | |
| Clock speed | |
| CS Pin | |
| Output serial/display | |
| Error yang muncul | |
| Solusi | |

---

## 12. Pertanyaan Analisis Wajib

1. Mengapa SPI butuh CS (Chip Select) untuk setiap slave?
2. Apa perbedaan mode 0, 1, 2, dan 3 pada SPI?
3. Mengapa SPI称为full-duplex sedangkan I2C half-duplex?
4. Apa keuntungan SPI dibanding I2C dalam hal kecepatan?
5. Mengapa SD Card butuh inisialisasi clock rendah (400 kHz) lalu dinaikkan?
6. Bagaimana cara ESP32 dan STM32 mengakses multi slave pada satu bus SPI?
7. Apa keuntungan DMA SPI pada STM32 dan ESP32?
8. Bagaimana mengatasi konflik CS saat multi-master pada SPI?
9. Mengapa W25Q64 butuh Write Enable (0x06) sebelum program/erase?
10. Bagaimana cara recovery saat komunikasi SPI timeout atau error?

---

## 13. Deliverable Laporan

1. Cover dan identitas.
2. Ringkasan teori SPI Modul 08.
3. Tabel hasil **25 eksperimen**.
4. Foto wiring tiap kelompok eksperimen.
5. Screenshot serial/OLED.
6. Analisis error dan troubleshooting.
7. Source code utama atau link repository.
8. Integrasi project Data Acquisition System.
9. Kesimpulan.
