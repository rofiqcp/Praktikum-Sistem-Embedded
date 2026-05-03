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
| STM32 Blue Pill/Black Pill | 1 | I2C1 PB7/PB6; HAL + DMA |
| BME280 | 1 | prioritas; alamat 0x76/0x77 |
| BMP280 | 1 opsional | fallback jika BME280 tidak tersedia; tanpa humidity |
| SSD1306 OLED 128×64 | 1 | 0x3C/0x3D |
| DS3231 RTC | 1 | 0x68; backup CR2032 |
| EEPROM AT24C32 atau 24LC256 | 1 | AT24C32=4 KB; 24LC256=32 KB |
| MPU6050 | 1 | 0x68/0x69; set AD0=HIGH jika DS3231 dipakai |
| BH1750 | 1 | 0x23/0x5C |
| LCD 16×2 + PCF8574 | 1 | 0x27/0x3F; cek pull-up 5 V |
| Resistor 4.7 kΩ | 2 | pull-up SDA/SCL jika perlu |
| Resistor 2.2 kΩ | 2 | alternatif 400 kHz |
| Breadboard, jumper, USB | sesuai | common ground wajib |
| Logic analyzer | opsional | validasi waveform |

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
| 1 | ESP32_01 | ESP32 | I2C Scanner dan Address Map |
| 2 | ESP32_02 | ESP32 | BME280/BMP280 Fallback Weather Read |
| 3 | ESP32_03 | ESP32 | SSD1306 OLED Dashboard |
| 4 | ESP32_04 | ESP32 | DS3231 + ESP32 Internal RTC + SNTP |
| 5 | ESP32_05 | ESP32 | EEPROM AT24C32/24LC256 Logger |
| 6 | ESP32_06 | ESP32 | MPU6050 Raw + Library Read |
| 7 | ESP32_07 | ESP32 | BH1750 Lux Meter |
| 8 | ESP32_08 | ESP32 | LCD PCF8574 Display |
| 9 | ESP32_09 | ESP32 | GPIO Matrix, Dual Bus, Speed Test |
| 10 | ESP32_10 | ESP32 | Error Handling dan Bus Recovery |
| 11 | STM32_01 | STM32 | HAL I2C Scanner |
| 12 | STM32_02 | STM32 | BME280/BMP280 Raw Register Driver |
| 13 | STM32_03 | STM32 | SSD1306 OLED via HAL |
| 14 | STM32_04 | STM32 | DS3231 + STM32 Internal RTC |
| 15 | STM32_05 | STM32 | EEPROM Logger + Page Boundary |
| 16 | STM32_06 | STM32 | MPU6050 Burst Read |
| 17 | STM32_07 | STM32 | BH1750 One-Shot/Continuous Mode |
| 18 | STM32_08 | STM32 | LCD PCF8574 via HAL |
| 19 | STM32_09 | STM32 | I2C DMA Transfer Benchmark |
| 20 | STM32_10 | STM32 | HAL Error Code, Timeout, Recovery |
| 21 | MULTI_01 | Multi | ESP32 Master + STM32 Sensor Node via UART |
| 22 | MULTI_02 | Multi | Bus Ownership Request/Grant |
| 23 | MULTI_03 | Multi | ESP32 SNTP Sync ke STM32/DS3231 |
| 24 | MULTI_04 | Multi | Multi-Display Weather Dashboard |
| 25 | MULTI_05 | Multi | Final Dual-MCU Weather Station Integration |

---

## 5. Eksperimen ESP32

### ESP32_01 — I2C Scanner dan Address Map

**Tujuan:** mendeteksi semua alamat I2C dan mengaitkannya dengan device.

**Hardware:** ESP32, minimal 3 device I2C, pull-up 4.7 kΩ.

**Wiring:** SDA GPIO21, SCL GPIO22, VCC 3.3 V, GND bersama.

**Langkah:**
1. Hubungkan OLED, BME280/BMP280, BH1750.
2. Jalankan scanner 0x01–0x7E.
3. Cetak address map dan nama device dikenal.
4. Lepas satu device, scan ulang.
5. Catat ACK/NACK.

**Output diharapkan:** serial menampilkan alamat seperti 0x23, 0x3C, 0x76/0x77.

---

### ESP32_02 — BME280/BMP280 Fallback Weather Read

**Tujuan:** membaca suhu/tekanan/kelembapan dengan fallback BMP280.

**Hardware:** ESP32, BME280 atau BMP280.

**Wiring:** SDA GPIO21, SCL GPIO22, SDO LOW=0x76 atau HIGH=0x77, CSB HIGH untuk mode I2C.

**Langkah:**
1. Scan 0x76 dan 0x77.
2. Baca chip ID 0xD0.
3. Jika 0x60, tampilkan suhu, tekanan, humidity.
4. Jika 0x58, tampilkan suhu, tekanan, humidity=`N/A`.
5. Uji perubahan suhu dengan napas hangat.

**Output diharapkan:** serial `BME280` atau `BMP280 fallback`, data stabil tiap 1 s.

---

### ESP32_03 — SSD1306 OLED Dashboard

**Tujuan:** menampilkan teks, ikon, dan data sensor di OLED.

**Hardware:** ESP32, SSD1306 128×64.

**Wiring:** VCC 3.3 V, SDA GPIO21, SCL GPIO22.

**Langkah:**
1. Init address 0x3C/0x3D.
2. Tampilkan judul Modul 07.
3. Tampilkan counter, uptime, dan data dummy.
4. Update display periodik.
5. Ukur waktu full refresh.

**Output diharapkan:** OLED menampilkan dashboard tanpa flicker berat.

---

### ESP32_04 — DS3231 + ESP32 Internal RTC + SNTP

**Tujuan:** membandingkan DS3231, RTC internal ESP32, dan sinkronisasi SNTP.

**Hardware:** ESP32, DS3231, WiFi opsional.

**Wiring:** DS3231 0x68 pada bus I2C; baterai CR2032 terpasang.

**Langkah:**
1. Baca waktu DS3231.
2. Set waktu ESP32 dari DS3231.
3. Jika WiFi tersedia, lakukan SNTP.
4. Opsional tulis waktu SNTP ke DS3231.
5. Reset board dan bandingkan waktu.

**Output diharapkan:** serial menampilkan waktu DS3231, system time ESP32, status SNTP.

---

### ESP32_05 — EEPROM AT24C32/24LC256 Logger

**Tujuan:** menulis dan membaca log circular tanpa salah kapasitas.

**Hardware:** ESP32, AT24C32 atau 24LC256.

**Wiring:** EEPROM 0x50; A0/A1/A2 sesuai alamat; pull-up 3.3 V.

**Langkah:**
1. Deteksi jenis EEPROM dari konfigurasi praktikum.
2. Tulis metadata: magic, version, write index.
3. Tulis record 16 byte.
4. Terapkan page write sesuai page boundary.
5. Baca ulang dan verifikasi CRC/status.

**Output diharapkan:** AT24C32 dibatasi 240 record; 24LC256 dibatasi 2000 record.

---

### ESP32_06 — MPU6050 Raw + Library Read

**Tujuan:** membaca accel/gyro dan memahami register raw.

**Hardware:** ESP32, MPU6050.

**Wiring:** AD0=HIGH jika DS3231 juga dipakai; alamat 0x69.

**Langkah:**
1. Wake-up register 0x6B.
2. Konfigurasi range accel dan gyro.
3. Burst read 14 byte.
4. Hitung accel g dan gyro °/s.
5. Bandingkan dengan library.

**Output diharapkan:** saat datar, sumbu Z mendekati 1 g.

---

### ESP32_07 — BH1750 Lux Meter

**Tujuan:** mengukur intensitas cahaya.

**Hardware:** ESP32, BH1750.

**Wiring:** ADDR LOW=0x23 atau HIGH=0x5C.

**Langkah:**
1. Init continuous high resolution.
2. Baca lux tiap 500 ms.
3. Uji gelap, ruangan, senter.
4. Uji one-shot mode.
5. Tampilkan klasifikasi gelap/redup/terang.

**Output diharapkan:** lux turun saat ditutup, naik saat disorot.

---

### ESP32_08 — LCD PCF8574 Display

**Tujuan:** menampilkan data ke LCD 16×2 via PCF8574.

**Hardware:** ESP32, LCD PCF8574.

**Wiring:** VCC LCD 5 V bila perlu; SDA/SCL aman 3.3 V; address 0x27/0x3F.

**Langkah:**
1. Scan address LCD.
2. Init LCD.
3. Atur kontras.
4. Tampilkan suhu dan lux dummy.
5. Uji custom character derajat/panah.

**Output diharapkan:** LCD menampilkan 2 baris data terbaca jelas.

---

### ESP32_09 — GPIO Matrix, Dual Bus, Speed Test

**Tujuan:** memakai fleksibilitas ESP32 untuk remap pin, dual bus, dan uji speed.

**Hardware:** ESP32, OLED, sensor.

**Wiring:** bus A default GPIO21/22; bus B pin alternatif yang aman.

**Langkah:**
1. Jalankan scanner pada pin default.
2. Deinit dan remap ke pin alternatif.
3. Jalankan dua bus: sensor dan display.
4. Bandingkan 100 kHz vs 400 kHz.
5. Catat error count.

**Output diharapkan:** device terdeteksi pada pin remap dan transfer 400 kHz lebih cepat jika bus stabil.

---

### ESP32_10 — Error Handling dan Bus Recovery

**Tujuan:** menangani NACK, timeout, SDA stuck, dan recovery.

**Hardware:** ESP32, satu sensor I2C, jumper simulasi error.

**Wiring:** standar; siapkan akses ke SDA/SCL.

**Langkah:**
1. Jalankan pembacaan normal.
2. Cabut SDA atau power sensor sesaat.
3. Deteksi error return.
4. Deinit I2C, pulse SCL 9 kali, buat STOP.
5. Init ulang dan scan.

**Output diharapkan:** sistem tidak hang; sensor kembali online setelah disambung.

---

## 6. Eksperimen STM32

### STM32_01 — HAL I2C Scanner

**Tujuan:** scan bus memakai `HAL_I2C_IsDeviceReady`.

**Hardware:** STM32, minimal 3 device I2C.

**Wiring:** I2C1 SCL PB6, SDA PB7, pull-up 4.7 kΩ.

**Langkah:**
1. Konfigurasi GPIO AF open-drain.
2. Init I2C 100 kHz.
3. Loop address 1–126 dengan `addr << 1`.
4. Cetak via UART.
5. Bandingkan hasil dengan ESP32_01.

**Output diharapkan:** daftar address sama dengan scanner ESP32.

---

### STM32_02 — BME280/BMP280 Raw Register Driver

**Tujuan:** menulis driver register dasar tanpa library tinggi.

**Hardware:** STM32, BME280/BMP280.

**Wiring:** PB7/PB6, address 0x76/0x77.

**Langkah:**
1. Baca chip ID 0xD0.
2. Baca calibration register.
3. Konfigurasi ctrl_meas/config.
4. Burst read data pressure/temp/humidity jika BME280.
5. Tampilkan hasil kompensasi.

**Output diharapkan:** suhu/tekanan valid; humidity `N/A` jika BMP280.

---

### STM32_03 — SSD1306 OLED via HAL

**Tujuan:** mengirim command/data SSD1306 via HAL.

**Hardware:** STM32, SSD1306.

**Wiring:** OLED 0x3C pada PB7/PB6.

**Langkah:**
1. Kirim sequence init SSD1306.
2. Buat framebuffer 1024 byte.
3. Tulis teks sederhana.
4. Flush buffer dengan I2C.
5. Ukur durasi refresh.

**Output diharapkan:** OLED menampilkan teks Modul 07.

---

### STM32_04 — DS3231 + STM32 Internal RTC

**Tujuan:** sinkronisasi DS3231 dengan RTC internal STM32.

**Hardware:** STM32, DS3231, baterai.

**Wiring:** DS3231 0x68.

**Langkah:**
1. Baca register BCD DS3231.
2. Konversi BCD ke biner.
3. Set STM32 RTC internal.
4. Simpan marker di backup register.
5. Reset dan cek waktu tetap valid.

**Output diharapkan:** UART menampilkan waktu DS3231 dan RTC internal.

---

### STM32_05 — EEPROM Logger + Page Boundary

**Tujuan:** logging EEPROM dengan page boundary benar.

**Hardware:** STM32, AT24C32 atau 24LC256.

**Wiring:** EEPROM 0x50.

**Langkah:**
1. Tulis byte tunggal dan verifikasi.
2. Tulis string lintas alamat.
3. Uji page write tidak melewati page boundary.
4. Implementasikan ACK polling.
5. Simulasikan circular log.

**Output diharapkan:** data terbaca ulang benar setelah reset; AT24C32 maks. 240 record 16 byte.

---

### STM32_06 — MPU6050 Burst Read

**Tujuan:** membaca 14 byte accel/gyro secara efisien.

**Hardware:** STM32, MPU6050.

**Wiring:** AD0=HIGH jika DS3231 ada.

**Langkah:**
1. Wake-up MPU6050.
2. Set range.
3. Burst read mulai register 0x3B.
4. Konversi raw ke unit fisik.
5. Hitung magnitude accel.

**Output diharapkan:** data berubah saat sensor digerakkan.

---

### STM32_07 — BH1750 One-Shot/Continuous Mode

**Tujuan:** membandingkan mode ukur BH1750.

**Hardware:** STM32, BH1750.

**Wiring:** ADDR LOW=0x23.

**Langkah:**
1. Kirim command power on.
2. Kirim continuous high resolution.
3. Baca 2 byte lux.
4. Uji one-shot high resolution.
5. Bandingkan waktu respons.

**Output diharapkan:** nilai lux valid dan mode one-shot hemat transaksi.

---

### STM32_08 — LCD PCF8574 via HAL

**Tujuan:** mengendalikan LCD HD44780 melalui expander PCF8574.

**Hardware:** STM32, LCD PCF8574.

**Wiring:** address 0x27/0x3F; cek level 5 V.

**Langkah:**
1. Scan address LCD.
2. Kirim init 4-bit mode via PCF8574.
3. Tulis karakter baris 1 dan 2.
4. Toggle backlight.
5. Buat custom character.

**Output diharapkan:** LCD menampilkan teks dan counter.

---

### STM32_09 — I2C DMA Transfer Benchmark

**Tujuan:** membandingkan polling, interrupt, dan DMA.

**Hardware:** STM32, EEPROM atau OLED.

**Wiring:** standar PB7/PB6.

**Langkah:**
1. Transfer blok 256 byte dengan polling.
2. Ulangi dengan interrupt.
3. Ulangi dengan DMA.
4. Saat DMA berjalan, jalankan task CPU dummy.
5. Catat waktu dan CPU availability.

**Output diharapkan:** DMA membebaskan CPU; transfer tetap selesai via callback.

---

### STM32_10 — HAL Error Code, Timeout, Recovery

**Tujuan:** membaca `HAL_I2C_GetError`, timeout, dan recovery bus.

**Hardware:** STM32, sensor I2C.

**Wiring:** standar; siapkan simulasi SDA/SCL error.

**Langkah:**
1. Baca sensor normal.
2. Cabut sensor/simulasikan NACK.
3. Cetak `hi2c.ErrorCode`.
4. Deinit I2C dan pulse SCL 9 kali via GPIO.
5. Init ulang dan scan.

**Output diharapkan:** error terklasifikasi; sistem pulih tanpa reset board.

---

## 7. Eksperimen Multi STM32-ESP32

### MULTI_01 — ESP32 Master + STM32 Sensor Node via UART

**Tujuan:** membagi tugas: STM32 membaca sensor, ESP32 mengagregasi via UART.

**Hardware:** ESP32, STM32, BME280/BMP280, BH1750.

**Wiring:** sensor ke STM32 I2C; UART STM32 TX/RX ke ESP32 RX/TX; GND bersama.

**Langkah:**
1. STM32 membaca sensor tiap 1 s.
2. STM32 mengirim frame CSV/JSON ringkas ke ESP32.
3. ESP32 parsing dan tampilkan serial/OLED.
4. Tambahkan checksum sederhana.
5. Simulasikan sensor offline.

**Output diharapkan:** ESP32 menerima data sensor dari STM32 stabil.

---

### MULTI_02 — Bus Ownership Request/Grant

**Tujuan:** menerapkan kepemilikan bus saat dua MCU dapat menjadi master.

**Hardware:** ESP32, STM32, satu bus I2C bersama, GPIO REQ/GRANT.

**Wiring:** SDA/SCL bersama; GPIO STM32_REQ ke ESP32; ESP32_GRANT ke STM32.

**Langkah:**
1. ESP32 menjadi master default.
2. STM32 set REQ saat ingin akses.
3. ESP32 menyelesaikan transaksi lalu set GRANT.
4. STM32 akses bus, lalu release.
5. Uji tanpa dan dengan ownership.

**Output diharapkan:** tidak ada collision saat ownership aktif.

---

### MULTI_03 — ESP32 SNTP Sync ke STM32/DS3231

**Tujuan:** menyebarkan waktu akurat dari ESP32 ke DS3231 dan STM32.

**Hardware:** ESP32 WiFi, STM32, DS3231.

**Wiring:** DS3231 pada bus yang dimiliki ESP32 saat sync; UART/GPIO sync ke STM32.

**Langkah:**
1. ESP32 mengambil waktu SNTP.
2. ESP32 menulis DS3231.
3. ESP32 mengirim timestamp ke STM32 via UART.
4. STM32 set internal RTC.
5. Matikan WiFi dan verifikasi waktu tetap dari DS3231.

**Output diharapkan:** semua sumber waktu sinkron dalam selisih kecil.

---

### MULTI_04 — Multi-Display Weather Dashboard

**Tujuan:** menampilkan data weather station di OLED ESP32 dan LCD STM32.

**Hardware:** ESP32, STM32, OLED, LCD, sensor weather.

**Wiring:** OLED ke ESP32; LCD ke STM32; data antar MCU via UART.

**Langkah:**
1. ESP32 agregasi data sensor.
2. ESP32 tampilkan grafik ringkas di OLED.
3. ESP32 kirim ringkasan ke STM32.
4. STM32 tampilkan suhu/lux/status di LCD.
5. Uji sensor disconnect.

**Output diharapkan:** OLED dan LCD menampilkan data konsisten.

---

### MULTI_05 — Final Dual-MCU Weather Station Integration

**Tujuan:** mengintegrasikan seluruh konsep Modul 07.

**Hardware:** ESP32, STM32, BME280/BMP280, BH1750, MPU6050, DS3231, EEPROM, OLED, LCD.

**Wiring:** sesuai desain project; hindari konflik 0x68; EEPROM sesuai kapasitas.

**Langkah:**
1. Startup scan semua bus.
2. Init sensor dengan fallback BME280/BMP280.
3. Sync waktu DS3231/RTC/SNTP.
4. Baca sensor periodik.
5. Tampilkan OLED/LCD.
6. Log record 16 byte ke EEPROM circular.
7. Terapkan error recovery.
8. Demo bus ownership atau split bus.

**Output diharapkan:** weather station dual-MCU berjalan stabil, log valid, display aktif, recovery bekerja.

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
