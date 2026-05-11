# Jobsheet Modul 07: I2C Bus dan Integrasi Sensor

## 1. Tujuan Praktikum

Setelah menyelesaikan jobsheet Modul 07, mahasiswa mampu:

1. Menggunakan I2C pada ESP32 dan STM32 untuk sensor, display, RTC, dan EEPROM.
2. Memahami START/STOP/repeated START, addressing, ACK/NACK, pull-up, open-drain, speed mode, clock stretching, dan recovery.
3. Mengimplementasikan tepat 25 eksperimen final: **10 ESP32, 10 STM32, 5 Multi STM32-ESP32**.
4. Mengintegrasikan hasil praktikum menjadi project weather station dual-MCU.

---

## 2. Peralatan

| Komponen | Jumlah | Catatan |
|---|---:|---|
| ESP32 DevKit | 1 | default SDA=GPIO21, SCL=GPIO22; dapat remap via GPIO matrix |
| STM32 Blue Pill/Black Pill | 1 | I2C1 PB7/PB6; HAL + DMA + FreeRTOS |
| BME280 | 1 | prioritas; alamat 0x76/0x77 |
| BMP280 | 1 opsional | fallback jika BME280 tidak tersedia; tanpa humidity |
| SSD1306 OLED 128×64 | 1 | 0x3C/0x3D |
| DS3231 RTC | 1 | 0x68; backup CR2032 |
| EEPROM AT24C32 atau 24LC256 | 1 | AT24C32=4 KB; 24LC256=32 KB |
| MPU6050 | 1 | 0x68/0x69; set AD0=HIGH jika DS3231 dipakai |
| BH1750 | 1 | 0x23/0x5C |
| LCD 16×2 + PCF8574 | 1 opsional | 0x27/0x3F; cek pull-up 5 V |
| Resistor 4.7 kΩ | 2 | pull-up SDA/SCL jika perlu |
| Resistor 2.2 kΩ | 2 | alternatif 400 kHz |
| Breadboard, jumper, USB | sesuai | common ground wajib |
| Logic analyzer | opsional | validasi waveform |
| PC dengan Python | 1 | untuk GUI_I2C.py (tkinter I2C monitor) |

---

## 3. Aturan Umum Wiring

### ESP32

| Sinyal | Default | Alternatif |
|---|---|---|
| SDA | GPIO21 | hampir semua GPIO valid via GPIO matrix |
| SCL | GPIO22 | hampir semua GPIO valid via GPIO matrix |
| VCC | 3.3 V | jangan pull-up I2C ke 5 V langsung |

### STM32

| Sinyal | I2C1 | I2C1 remap | I2C2 |
|---|---|---|---|
| SCL | PB6 | PB8 | PB10 |
| SDA | PB7 | PB9 | PB11 |

Aturan:

- Semua GND disatukan.
- Pull-up SDA/SCL ke 3.3 V.
- Jalankan scanner sebelum percobaan sensor.
- Jika LCD backpack menarik SDA/SCL ke 5 V, gunakan level shifter atau ubah pull-up ke 3.3 V.
- DS3231 dan MPU6050 sama-sama default 0x68; pakai MPU6050 AD0=HIGH → 0x69.

---

## 4. Daftar Final 25 Eksperimen

| No | Kode | Kelompok | Judul |
|---:|---|---|---|
| 1 | ESP32_01 | ESP32 | I2C Bus Scanner dan Address Map |
| 2 | ESP32_02 | ESP32 | SSD1306 OLED Display Graphics |
| 3 | ESP32_03 | ESP32 | BME280 Environmental Sensor |
| 4 | ESP32_04 | ESP32 | MPU6050 IMU Raw dan Angle |
| 5 | ESP32_05 | ESP32 | AT24C32 EEPROM Data Log |
| 6 | ESP32_06 | ESP32 | DS3231 RTC Alarm dan Temperature |
| 7 | ESP32_07 | ESP32 | BH1750 Light Adaptive Display |
| 8 | ESP32_08 | ESP32 | I2C RTOS Multi Task Sensor |
| 9 | ESP32_09 | ESP32 | I2C RTOS Data Logger |
| 10 | ESP32_10 | ESP32 | I2C RTOS Interrupt Driven |
| 11 | STM32_01 | STM32 | HAL I2C Scanner |
| 12 | STM32_02 | STM32 | SSD1306 OLED HAL Driver |
| 13 | STM32_03 | STM32 | BME280 Register Driver |
| 14 | STM32_04 | STM32 | MPU6050 IMU Interrupt Ready |
| 15 | STM32_05 | STM32 | AT24C32 EEPROM Page Buffer |
| 16 | STM32_06 | STM32 | DS3231 RTC External BCD |
| 17 | STM32_07 | STM32 | Internal RTC Backup Register |
| 18 | STM32_08 | STM32 | I2C RTOS Multi Task |
| 19 | STM32_09 | STM32 | I2C RTOS DMA Transfer |
| 20 | STM32_10 | STM32 | I2C RTOS Error Recovery |
| 21 | MULTI_01 | Multi | I2C Master Slave Basic |
| 22 | MULTI_02 | Multi | I2C Role Swap Command |
| 23 | MULTI_03 | Multi | I2C Shared Sensor |
| 24 | MULTI_04 | Multi | I2C RTOS Gateway |
| 25 | MULTI_05 | Multi | I2C RTOS Weather Station |

---

## 5. Eksperimen ESP32

### ESP32_01 — I2C Bus Scanner dan Address Map

**Tujuan:** memindai bus I2C alamat 0x01–0x7F dan mengidentifikasi perangkat yang dikenal.

**Hardware:** ESP32 DevKit/S2/S3, minimal 3 device I2C, pull-up 4.7 kΩ.

**Wiring:** SDA GPIO21 (ESP32) atau GPIO8 (S2/S3), SCL GPIO22 (ESP32) atau GPIO9 (S2/S3), VCC 3.3 V, GND bersama.

**Langkah:**
1. Hubungkan OLED, BME280, BH1750.
2. Jalankan scanner 0x01–0x7F.
3. Identifikasi perangkat: 0x3C (SSD1306), 0x76/0x77 (BME280), 0x68/0x69 (MPU6050/DS3231), 0x50 (AT24C32), 0x23/0x5C (BH1750).
4. Lepas satu device, scan ulang.
5. Catat alamat yang tidak merespons.

**Output diharapkan:** serial menampilkan alamat terdeteksi beserta nama perangkat.

---

### ESP32_02 — SSD1306 OLED Display Graphics

**Tujuan:** menampilkan grafis, teks, dan ikon di OLED SSD1306.

**Hardware:** ESP32, SSD1306 128×64.

**Wiring:** VCC 3.3 V, SDA GPIO21, SCL GPIO22, address 0x3C/0x3D.

**Langkah:**
1. Init OLED address 0x3C/0x3D.
2. Tampilkan judul dan grafis dasar.
3. Tampilkan counter, uptime, dan data sensor.
4. Update display periodik.
5. Ukur waktu refresh.

**Output diharapkan:** OLED menampilkan grafis tanpa flicker berat.

---

### ESP32_03 — BME280 Environmental Sensor

**Tujuan:** membaca suhu, tekanan, kelembapan dari BME280 dengan kalibrasi register.

**Hardware:** ESP32, BME280 (alamat 0x76/0x77).

**Wiring:** SDA GPIO21, SCL GPIO22, SDO LOW=0x76 atau HIGH=0x77, CSB HIGH untuk mode I2C.

**Langkah:**
1. Scan alamat 0x76 dan 0x77.
2. Baca chip ID 0xD0 (0x60 untuk BME280).
3. Baca register kalibrasi 0x88 (26 byte) dan 0xE1 (7 byte).
4. Konfigurasi ctrl_hum 0xF2, ctrl_meas 0xF4, config 0xF5.
5. Burst read 8 byte dari 0xF7 dan hitung nilai fisik.

**Output diharapkan:** serial menampilkan T (°C), P (Pa), H (%).

---

### ESP32_04 — MPU6050 IMU Raw dan Angle

**Tujuan:** membaca akselerometer dan giroskop MPU6050, menghitung sudut kemiringan.

**Hardware:** ESP32, MPU6050.

**Wiring:** AD0=LOW=0x68, AD0=HIGH=0x69; SDA GPIO21, SCL GPIO22.

**Langkah:**
1. Wake-up register 0x6B (tulis 0x00).
2. Konfigurasi aksel dan gyro range (register 0x1B, 0x1C).
3. Burst read 14 byte dari register 0x3B.
4. Hitung sudut pitch/roll dari akselerometer.
5. Bandingkan data raw dengan library.

**Output diharapkan:** saat datar, sumbu Z ~1g; sudut pitch/roll ~0°.

---

### ESP32_05 — AT24C32 EEPROM Data Log

**Tujuan:** menulis dan membaca log data ke EEPROM AT24C32 dengan manajemen page boundary.

**Hardware:** ESP32, AT24C32 (4 KB) atau 24LC256 (32 KB).

**Wiring:** EEPROM 0x50; A0/A1/A2 sesuai alamat; pull-up 3.3 V.

**Langkah:**
1. Deteksi jenis EEPROM dari konfigurasi.
2. Tulis metadata: magic, version, write index.
3. Tulis record data (misal 16 byte).
4. Terapkan page write sesuai page boundary (32 byte untuk AT24C32).
5. Baca ulang dan verifikasi data.

**Output diharapkan:** AT24C32 dibatasi 240 record; 24LC256 dibatasi 2000 record.

---

### ESP32_06 — DS3231 RTC Alarm dan Temperature

**Tujuan:** membaca dan mengatur RTC eksternal DS3231, menggunakan fitur alarm dan baca suhu.

**Hardware:** ESP32, DS3231, baterai CR2032.

**Wiring:** DS3231 0x68 pada bus I2C; SDA GPIO21, SCL GPIO22.

**Langkah:**
1. Baca waktu dari register 0x00–0x06 (format BCD).
2. Konversi BCD ke desimal.
3. Set alarm (register 0x07–0x0A) dan enable interrupt (register 0x0E).
4. Baca suhu dari register 0x11–0x12.
5. Cek flag alarm di register 0x0F.

**Output diharapkan:** serial menampilkan waktu, suhu DS3231, dan notifikasi alarm.

---

### ESP32_07 — BH1750 Light Adaptive Display

**Tujuan:** membaca intensitas cahaya BH1750 dan menampilkan bar grafik adaptif.

**Hardware:** ESP32, BH1750.

**Wiring:** ADDR LOW=0x23 atau HIGH=0x5C; SDA GPIO21, SCL GPIO22.

**Langkah:**
1. Init continuous high resolution (command 0x10).
2. Baca lux tiap 1000 ms.
3. Klasifikasi: gelap (<50 lux), normal (50-500 lux), terang (>500 lux).
4. Tampilkan bar grafik adaptif berdasarkan intensitas.
5. Uji one-shot mode (command 0x20).

**Output diharapkan:** lux valid dan bar grafik responsif terhadap perubahan cahaya.

---

### ESP32_08 — I2C RTOS Multi Task Sensor

**Tujuan:** membaca sensor I2C dengan FreeRTOS multi-task.

**Hardware:** ESP32, BME280/BH1750.

**Wiring:** sensor di SDA GPIO21, SCL GPIO22.

**Langkah:**
1. Buat task RTOS untuk baca sensor.
2. Gunakan queue untuk kirim data antar task.
3. Task display update OLED periodik.
4. Task logger tulis ke EEPROM.
5. Monitor CPU usage tiap task.

**Output diharapkan:** multi-task berjalan stabil tanpa blocking.

---

### ESP32_09 — I2C RTOS Data Logger

**Tujuan:** logging data sensor secara kontinu ke EEPROM dengan FreeRTOS.

**Hardware:** ESP32, sensor I2C, EEPROM AT24C32.

**Wiring:** sensor dan EEPROM di bus I2C yang sama.

**Langkah:**
1. Buat task RTOS untuk baca sensor periodik.
2. Simpan data ke buffer circular.
3. Task logger tulis buffer ke EEPROM dengan page write.
4. Gunakan semaphore untuk sync akses EEPROM.
5. Cek integritas data dengan CRC.

**Output diharapkan:** data ter-log kontinu dan dapat dibaca ulang setelah reset.

---

### ESP32_10 — I2C RTOS Interrupt Driven

**Tujuan:** membaca sensor I2C dengan interrupt-driven I2C pada FreeRTOS.

**Hardware:** ESP32, sensor I2C (misal BME280).

**Wiring:** sensor di SDA GPIO21, SCL GPIO22.

**Langkah:**
1. Konfigurasi I2C master dengan interrupt.
2. Gunakan callback selesai transfer.
3. Buat task RTOS yang menunggu semaphore dari ISR.
4. Bandingkan dengan polling mode.
5. Ukur respon time dan CPU usage.

**Output diharapkan:** transfer I2C non-blocking, CPU bebas untuk task lain.

---

## 6. Eksperimen STM32

### STM32_01 — HAL I2C Scanner

**Tujuan:** scan bus I2C memakai `HAL_I2C_IsDeviceReady`.

**Hardware:** STM32 (F103C8/F401CC/F411CE), minimal 3 device I2C.

**Wiring:** I2C1 SCL PB6, SDA PB7, pull-up 4.7 kΩ.

**Langkah:**
1. Konfigurasi GPIO AF open-drain.
2. Init I2C 100 kHz.
3. Loop address 1–126 dengan `addr << 1`.
4. Cetak via UART.
5. Bandingkan hasil dengan ESP32_01.

**Output diharapkan:** daftar address sama dengan scanner ESP32.

---

### STM32_02 — SSD1306 OLED HAL Driver

**Tujuan:** mengirim command/data SSD1306 via HAL I2C.

**Hardware:** STM32, SSD1306.

**Wiring:** OLED 0x3C pada PB7/PB6.

**Langkah:**
1. Kirim sequence init SSD1306.
2. Buat framebuffer 1024 byte.
3. Tulis teks dan grafis sederhana.
4. Flush buffer dengan I2C.
5. Ukur durasi refresh.

**Output diharapkan:** OLED menampilkan teks Modul 07.

---

### STM32_03 — BME280 Register Driver

**Tujuan:** menulis driver register dasar BME280 tanpa library tinggi.

**Hardware:** STM32, BME280.

**Wiring:** PB7/PB6, address 0x76/0x77.

**Langkah:**
1. Baca chip ID 0xD0 (0x60).
2. Baca calibration register 0x88 (26 byte) dan 0xE1 (7 byte).
3. Konfigurasi ctrl_meas 0xF4, config 0xF5.
4. Burst read data pressure/temp/humidity dari 0xF7.
5. Tampilkan hasil kompensasi.

**Output diharapkan:** suhu/tekanan/kelembapan valid.

---

### STM32_04 — MPU6050 IMU Interrupt Ready

**Tujuan:** membaca accel/gyro MPU6050 dengan interrupt.

**Hardware:** STM32, MPU6050.

**Wiring:** AD0=LOW=0x68, AD0=HIGH=0x69; PB6/PB7.

**Langkah:**
1. Wake-up MPU6050 (register 0x6B = 0x00).
2. Konfigurasi INT pin (register 0x38).
3. Set range accel 0x1C dan gyro 0x1B.
4. Burst read 14 byte dari 0x3B saat interrupt.
5. Hitung sudut pitch/roll.

**Output diharapkan:** data berubah saat sensor digerakkan, interrupt trigger.

---

### STM32_05 — AT24C32 EEPROM Page Buffer

**Tujuan:** logging EEPROM dengan page boundary benar.

**Hardware:** STM32, AT24C32 atau 24LC256.

**Wiring:** EEPROM 0x50.

**Langkah:**
1. Tulis byte tunggal dan verifikasi.
2. Tulis string lintas alamat.
3. Uji page write tidak melewati page boundary (32 byte).
4. Implementasikan ACK polling.
5. Simulasikan circular log.

**Output diharapkan:** data terbaca ulang benar setelah reset; AT24C32 maks. 240 record 16 byte.

---

### STM32_06 — DS3231 RTC External BCD

**Tujuan:** baca waktu dari RTC eksternal DS3231 dengan konversi BCD ke desimal.

**Hardware:** STM32, DS3231 (alamat 0x68).

**Wiring:** I2C1 PB6/7, DS3231 0x68.

**Langkah:**
1. Baca register 0x00–0x06 (detik–tahun, format BCD).
2. Konversi BCD ke biner.
3. Set alarm (register 0x07–0x0A) dan enable interrupt (0x0E).
4. Baca suhu dari register 0x11–0x12.
5. Cek flag alarm di register 0x0F.

**Output diharapkan:** UART menampilkan waktu DS3231 dan notifikasi alarm.

---

### STM32_07 — Internal RTC Backup Register

**Tujuan:** gunakan RTC internal STM32 dan backup register untuk menyimpan data.

**Hardware:** STM32, baterai CR2032.

**Wiring:** backup register akses via HAL.

**Langkah:**
1. Init RTC internal (LSE 32.768 kHz).
2. Baca/write backup register (RTC_BKP_DRx).
3. Set waktu awal RTC.
4. Konfigurasi wakeup timer (F4 only).
5. Reset board dan cek data tetap ada.

**Output diharapkan:** UART menampilkan waktu RTC internal dan data backup.

---

### STM32_08 — I2C RTOS Multi Task

**Tujuan:** membaca sensor I2C dengan FreeRTOS multi-task.

**Hardware:** STM32, sensor I2C.

**Wiring:** sensor di PB6/PB7.

**Langkah:**
1. Buat task RTOS untuk baca sensor.
2. Gunakan queue untuk kirim data antar task.
3. Task display update via UART.
4. Task logger tulis ke EEPROM.
5. Monitor CPU usage tiap task.

**Output diharapkan:** multi-task berjalan stabil tanpa blocking.

---

### STM32_09 — I2C RTOS DMA Transfer

**Tujuan:** transfer I2C DMA dengan FreeRTOS queue/semaphore.

**Hardware:** STM32, sensor I2C (alamat 0x76, misal BME280).

**Wiring:** PB6/7, DMA channel/stream sesuai seri STM32.

**Langkah:**
1. Init I2C dengan DMA (hdmatx, hdmarx).
2. Buat task RTOS yang trigger DMA transfer.
3. Pakai semaphore dari callback `HAL_I2C_MemTxCpltCallback`.
4. Bandingkan dengan polling mode.
5. Ukur CPU usage saat DMA berjalan.

**Output diharapkan:** DMA membebaskan CPU; transfer selesai via callback.

---

### STM32_10 — I2C RTOS Error Recovery

**Tujuan:** FreeRTOS task dengan watchdog/error recovery pada I2C.

**Hardware:** STM32, sensor I2C (alamat 0x76).

**Wiring:** PB6/7 standar.

**Langkah:**
1. Buat task sensor dengan retry (MAX_RETRY=3).
2. Watchdog task monitor error_count.
3. Jika error, lakukan I2C recovery (deinit, pulse SCL 9x, init).
4. Gunakan mutex untuk akses I2C bersama.
5. Tampilkan statistik error dan recovery.

**Output diharapkan:** sistem pulih otomatis dari error I2C tanpa reset.

---

## 7. Eksperimen Multi STM32-ESP32

### MULTI_01 — I2C Master Slave Basic

**Tujuan:** mengimplementasikan komunikasi I2C dimana STM32 berperan sebagai slave dan ESP32 sebagai master.

**Hardware:** ESP32, STM32, koneksi I2C bersama.

**Wiring:** STM32 PB7 (SDA) <-> ESP32 GPIO21; STM32 PB6 (SCL) <-> ESP32 GPIO22; GND bersama; pull-up 4.7k ke 3V3; STM32 slave addr 0x42.

**Langkah:**
1. Upload kode slave ke STM32 (addr 0x42).
2. Upload kode master ke ESP32.
3. STM32 ekspos virtual register (REG_STATUS, REG_COUNTER, REG_TEMP_C, REG_LED_CMD).
4. ESP32 poll data dari STM32 tiap 1 detik.
5. Observer respon via serial monitor.

**Output diharapkan:** ESP32 berhasil baca/write register STM32 slave.

---

### MULTI_02 — I2C Role Swap Command

**Tujuan:** mengimplementasikan pergantian peran master-slave via command.

**Hardware:** ESP32, STM32, koneksi I2C bersama, GPIO REQ/GRANT opsional.

**Wiring:** I2C bersama; STM32 slave addr 0x42; ESP32 slave addr 0x32; GPIO untuk sinyal control.

**Langkah:**
1. STM32 init sebagai master, ESP32 sebagai slave.
2. STM32 poll data dari ESP32.
3. Kirim command role swap.
4. ESP32 jadi master, STM32 jadi slave.
5. Uji komunikasi dua arah.

**Output diharapkan:** pergantian peran berjalan tanpa collision.

---

### MULTI_03 — I2C Shared Sensor

**Tujuan:** berbagi akses sensor I2C antara ESP32 dan STM32.

**Hardware:** ESP32, STM32, sensor I2C (BME280/BH1750), GPIO REQ/GRANT.

**Wiring:** sensor di bus I2C bersama; GPIO REQ/GRANT antar MCU.

**Langkah:**
1. ESP32 default master, STM32 ready.
2. STM32 set REQ saat ingin akses sensor.
3. ESP32 selesaikan transaksi lalu set GRANT.
4. STM32 akses sensor, lalu release.
5. ESP32 lanjutkan akses.

**Output diharapkan:** tidak ada collision saat akses bersama.

---

### MULTI_04 — I2C RTOS Gateway

**Tujuan:** membuat gateway RTOS yang mengagregasi data dari sensor dan mengirim ke host.

**Hardware:** ESP32, STM32, sensor I2C, display.

**Wiring:** ESP32 dan STM32 di bus I2C; ESP32 sebagai gateway.

**Langkah:**
1. STM32 baca sensor tiap 1 s (FreeRTOS task).
2. ESP32 request data via I2C.
3. ESP32 tampilkan di OLED dan kirim via UART.
4. Implementasikan queue/semphore RTOS.
5. Monitor CPU usage.

**Output diharapkan:** data sensor teragregasi dan ditampilkan stabil.

---

### MULTI_05 — I2C RTOS Weather Station

**Tujuan:** mengintegrasikan seluruh konsep Modul 07 dalam project weather station RTOS.

**Hardware:** ESP32, STM32, BME280, BH1750, DS3231, OLED, LCD (opsional).

**Wiring:** sesuai desain; ESP32 dan STM32 komunikasi I2C; sensor di bus bersama.

**Langkah:**
1. Init RTOS di ESP32 dan STM32.
2. Scan semua device di bus.
3. Baca sensor periodik dengan task RTOS.
4. Tampilkan data di OLED (ESP32) dan LCD (STM32).
5. Log data ke EEPROM dengan RTOS.
6. Implementasikan error recovery RTOS.

**Output diharapkan:** weather station dual-MCU RTOS berjalan stabil.

---

## 8. Format Pengamatan Minimal

Untuk tiap eksperimen, catat:

| Item | Nilai |
|---|---|
| Kode eksperimen | |
| Platform | ESP32 / STM32 / Multi |
| Device I2C | |
| Address terdeteksi | |
| Clock speed | |
| Pull-up | |
| Output serial/display | |
| Error yang muncul | |
| Solusi | |

---

## 9. Pertanyaan Analisis Wajib

1. Mengapa I2C butuh open-drain dan pull-up?
2. Apa bedanya STOP dan repeated START?
3. Apa arti ACK dan NACK pada transaksi sensor?
4. Mengapa DS3231 dan MPU6050 dapat konflik alamat?
5. Kapan 100 kHz lebih aman daripada 400 kHz?
6. Mengapa AT24C32 tidak boleh diklaim menyimpan 1000 record 16 byte?
7. Apa keuntungan DMA I2C pada STM32?
8. Apa keuntungan GPIO matrix ESP32?
9. Bagaimana cara recovery saat SDA stuck LOW?
10. Bagaimana mencegah collision pada multi-master ESP32-STM32?

---

## 10. Deliverable Laporan

1. Cover dan identitas.
2. Ringkasan teori I2C Modul 07.
3. Tabel hasil **25 eksperimen**.
4. Foto wiring tiap kelompok eksperimen.
5. Screenshot serial/OLED/LCD.
6. Analisis error dan troubleshooting.
7. Source code utama atau link repository.
8. Integrasi project weather station.
9. Kesimpulan.
