# Modul 07: I2C Bus dan Integrasi Sensor — NotebookLM 45 Slide

## Bagian 1 — Teori I2C dan Platform (Slide 1–15)

### Slide 1 — Modul 07 dan Target Akhir

Modul 07 membahas I2C bus dan integrasi sensor pada ESP32, STM32, dan sistem gabungan. Target akhir: 25 eksperimen final, terdiri dari 10 ESP32, 10 STM32, dan 5 Multi STM32-ESP32. Semua percobaan diarahkan ke project weather station dual-MCU.

---

### Slide 2 — Dasar Bus I2C

I2C memakai dua jalur: SDA untuk data dan SCL untuk clock. Semua device berbagi bus yang sama, sehingga pin hemat. Komunikasi bersifat half-duplex. Master mengatur transaksi, slave merespons berdasarkan address. Ground bersama wajib agar level logika valid.

---

### Slide 3 — Open-Drain dan Pull-Up

SDA dan SCL bersifat open-drain. Device hanya menarik jalur ke LOW; resistor pull-up menarik jalur ke HIGH. Karena itu LOW dominan. Nilai umum pull-up adalah 4.7 kΩ untuk 100 kHz dan 2.2–4.7 kΩ untuk 400 kHz, tergantung kapasitansi bus.

---

### Slide 4 — Addressing I2C

Device I2C umum memakai address 7-bit. Byte address dikirim bersama bit R/W: 0 untuk write, 1 untuk read. STM32 HAL sering meminta address digeser kiri (`addr << 1`), sedangkan ESP32 API umumnya memakai address 7-bit langsung. 10-bit addressing ada sebagai konsep lanjutan, tetapi jarang pada sensor praktikum.

---

### Slide 5 — ACK dan NACK

Setiap byte I2C diikuti clock ke-9 untuk ACK/NACK. ACK berarti receiver menarik SDA LOW; NACK berarti SDA tetap HIGH. NACK bisa terjadi karena alamat salah, device mati, device sibuk, atau wiring bermasalah. Driver wajib mengecek status ACK/NACK.

---

### Slide 6 — START, STOP, Repeated START

START terjadi saat SDA turun ketika SCL HIGH. STOP terjadi saat SDA naik ketika SCL HIGH. Repeated START adalah START baru tanpa STOP, umum untuk membaca register: tulis alamat register, repeated START, lalu baca data. Ini menjaga transaksi tetap atomik.

---

### Slide 7 — Speed Mode

Mode umum: Standard 100 kHz, Fast 400 kHz, Fast Mode Plus 1 MHz, High Speed 3.4 MHz. Praktikum dimulai dari 100 kHz untuk stabilitas, lalu diuji 400 kHz. Kecepatan maksimum dipengaruhi pull-up, panjang kabel, kapasitansi, noise, dan kemampuan device.

---

### Slide 8 — Clock Stretching

Clock stretching terjadi saat slave menahan SCL LOW agar master menunggu. Ini dipakai ketika slave belum siap, misalnya sensor sedang konversi atau EEPROM sedang write cycle. Master harus mendukung timeout agar sistem tidak hang jika stretching terlalu lama.

---

### Slide 9 — Device Modul 07

Device utama Modul 07: BME280/BMP280, SSD1306, DS3231, EEPROM AT24C32/24LC256, MPU6050, BH1750, dan LCD PCF8574. Address penting: SSD1306 0x3C, BH1750 0x23, EEPROM 0x50, DS3231 0x68, MPU6050 0x68/0x69, BME/BMP 0x76/0x77.

---

### Slide 10 — BME280 dan BMP280 Fallback

BME280 membaca suhu, tekanan, dan kelembapan. BMP280 hanya membaca suhu dan tekanan. Driver harus membaca chip ID: 0x60 untuk BME280 dan 0x58 untuk BMP280. Jika BMP280 dipakai, humidity ditampilkan `N/A`, bukan angka palsu.

---

### Slide 11 — RTC dan Waktu

DS3231 memberi timestamp presisi dengan baterai backup. STM32 punya internal RTC berbasis backup domain. ESP32 punya RTC domain dan dapat sinkron waktu melalui SNTP jika WiFi tersedia. Project memakai SNTP sebagai koreksi, DS3231 sebagai waktu offline, dan RTC internal sebagai fallback.

---

### Slide 12 — EEPROM dan Kapasitas Log

AT24C32 berkapasitas 4 KB, sedangkan 24LC256 berkapasitas 32 KB. Jika record log 16 byte dan metadata 16 byte, AT24C32 aman dibatasi 240 record. 24LC256 aman untuk 2000 record. Klaim 1000 record pada AT24C32 adalah salah.

---

### Slide 13 — STM32 HAL, DMA, dan RTC Internal

STM32 memakai HAL I2C untuk scan, read/write register, timeout, interrupt, dan DMA. DMA berguna untuk transfer blok tanpa membebani CPU. STM32 internal RTC dapat disinkronkan dari DS3231 lalu menyimpan marker pada backup register agar waktu tidak diset ulang sembarangan.

---

### Slide 14 — ESP32 ESP-IDF, GPIO Matrix, SNTP

ESP32 memiliki dua controller I2C dan GPIO matrix, sehingga SDA/SCL dapat dipetakan ke banyak pin. ESP-IDF memberi kontrol driver, timeout, dan error code. ESP32 juga dapat memakai SNTP untuk sinkronisasi waktu jaringan, lalu memperbarui DS3231 atau mengirim waktu ke STM32.

---

### Slide 15 — Error Recovery dan Bus Ownership

Jika SDA stuck LOW, recovery dilakukan dengan deinit I2C, pulse SCL 9 kali, buat STOP manual, lalu init ulang. Untuk dua master ESP32-STM32, perlu bus ownership: request, grant, transaksi, release. Alternatif aman adalah split bus dan komunikasi antar-MCU lewat UART.

---

## Bagian 2 — Praktikum 25 Eksperimen (Slide 16–30)

### Slide 16 — Struktur Final Praktikum

Modul 07 memiliki tepat 25 eksperimen: ESP32_01 sampai ESP32_10, STM32_01 sampai STM32_10, dan MULTI_01 sampai MULTI_05. Struktur ini memastikan mahasiswa memahami I2C dari sisi ESP32, STM32, dan integrasi dua MCU.

---

### Slide 17 — ESP32_01 dan ESP32_02

ESP32_01 adalah I2C scanner dan address map. Output berupa daftar alamat device dan nama yang dikenali. ESP32_02 membaca BME280 atau BMP280 fallback. Output wajib menunjukkan chip ID dan data suhu/tekanan/humidity atau `N/A`.

---

### Slide 18 — ESP32_03 dan ESP32_04

ESP32_03 menampilkan dashboard pada OLED SSD1306. Mahasiswa mengukur efek refresh pada 100 kHz dan 400 kHz. ESP32_04 menggabungkan DS3231, RTC internal ESP32, dan SNTP. Output berupa waktu dari beberapa sumber dan status sinkronisasi.

---

### Slide 19 — ESP32_05 dan ESP32_06

ESP32_05 membuat EEPROM logger dengan record 16 byte dan batas kapasitas benar: 240 record untuk AT24C32 atau 2000 record untuk 24LC256. ESP32_06 membaca MPU6050 secara raw dan/atau library, termasuk accel, gyro, dan perubahan saat sensor digerakkan.

---

### Slide 20 — ESP32_07 dan ESP32_08

ESP32_07 memakai BH1750 untuk mengukur lux pada kondisi gelap, ruangan, dan senter. ESP32_08 memakai LCD PCF8574 untuk menampilkan data dua baris. Pada LCD, cek address 0x27/0x3F, kontras, power 5 V, dan keamanan pull-up I2C.

---

### Slide 21 — ESP32_09 dan ESP32_10

ESP32_09 menguji GPIO matrix, remap pin, dual bus, serta speed 100 kHz vs 400 kHz. ESP32_10 fokus error handling: NACK, timeout, sensor dicabut, recovery 9 pulse SCL, STOP manual, reinit, dan scan ulang.

---

### Slide 22 — STM32_01 dan STM32_02

STM32_01 memakai `HAL_I2C_IsDeviceReady` untuk scan bus. Address harus digeser kiri pada HAL. STM32_02 membuat driver raw register untuk BME280/BMP280: baca chip ID, calibration data, register kontrol, lalu hitung data cuaca.

---

### Slide 23 — STM32_03 dan STM32_04

STM32_03 mengendalikan SSD1306 via HAL dengan framebuffer. STM32_04 membaca DS3231, konversi BCD, sinkron ke STM32 internal RTC, dan menyimpan marker di backup register. Output dikirim lewat UART.

---

### Slide 24 — STM32_05 dan STM32_06

STM32_05 membuat EEPROM logger dan menguji page boundary serta ACK polling. STM32_06 membaca MPU6050 dengan burst read 14 byte dari register accel/gyro. Output menunjukkan sumbu berubah saat sensor diputar.

---

### Slide 25 — STM32_07 dan STM32_08

STM32_07 memakai BH1750 mode continuous dan one-shot, lalu membandingkan waktu ukur. STM32_08 mengendalikan LCD PCF8574 melalui HAL, termasuk init 4-bit, backlight, posisi cursor, dan custom character.

---

### Slide 26 — STM32_09 dan STM32_10

STM32_09 membandingkan transfer I2C polling, interrupt, dan DMA. DMA membebaskan CPU saat transfer blok. STM32_10 membaca HAL error code, menangani timeout, melakukan bus recovery, dan memastikan sistem tidak perlu reset board.

---

### Slide 27 — MULTI_01 dan MULTI_02

MULTI_01 menjadikan STM32 sebagai sensor node dan ESP32 sebagai gateway melalui UART. MULTI_02 menerapkan bus ownership request/grant jika kedua MCU dapat mengakses bus I2C yang sama. Tujuannya mencegah collision multi-master.

---

### Slide 28 — MULTI_03 dan MULTI_04

MULTI_03 memakai ESP32 untuk mengambil waktu SNTP, menulis DS3231, dan mengirim timestamp ke STM32. MULTI_04 membuat multi-display weather dashboard: OLED di ESP32 dan LCD di STM32 menampilkan data yang konsisten.

---

### Slide 29 — MULTI_05 Final Integration

MULTI_05 adalah integrasi akhir: scanner, BME280/BMP280 fallback, BH1750, MPU6050, DS3231, EEPROM, OLED, LCD, ESP32-STM32 communication, error recovery, dan bus ownership atau split bus. Output akhirnya weather station dual-MCU stabil.

---

### Slide 30 — Data Pengamatan Praktikum

Setiap eksperimen mencatat kode, platform, wiring, address, clock speed, nilai pull-up, output serial/display, error, dan solusi. Tabel 25 eksperimen wajib lengkap. Foto wiring dan screenshot serial/OLED/LCD menjadi bukti utama laporan.

---

## Bagian 3 — Project, Video, dan Evaluasi (Slide 31–45)

### Slide 31 — Project Weather Station Dual-MCU

Project akhir Modul 07 adalah weather station dual-MCU. ESP32 berperan sebagai gateway, display OLED, SNTP, dan koordinator. STM32 berperan sebagai sensor/logger node dengan HAL, raw register, DMA, dan RTC internal. Keduanya bertukar data melalui UART atau bus ownership.

---

### Slide 32 — Arsitektur Project

Sensor cuaca meliputi BME280/BMP280, BH1750, dan MPU6050. DS3231 memberi timestamp. EEPROM menyimpan log. SSD1306 dan LCD menampilkan data. Sistem boleh memakai split bus agar aman, atau satu bus bersama dengan request/grant ownership.

---

### Slide 33 — Record Log Project

Record log disarankan 16 byte: timestamp 4 byte, temperature 2 byte, pressure 2 byte, humidity 2 byte, lux 2 byte, accel magnitude 2 byte, status 1 byte, checksum 1 byte. Jika BMP280 dipakai, field humidity diisi 0xFFFF.

---

### Slide 34 — Kapasitas EEPROM Project

AT24C32 memiliki 4096 byte. Dengan metadata 16 byte dan record 16 byte, batas aman adalah 240 record. 24LC256 memiliki 32768 byte dan aman untuk 2000 record. Batas ini wajib disebut pada laporan dan video agar tidak terjadi kesalahan kapasitas.

---

### Slide 35 — Alur Startup Project

Saat startup, sistem melakukan scan bus, membaca chip ID BME/BMP, mengecek konflik 0x68, menginisialisasi RTC, memvalidasi EEPROM metadata, menyiapkan display, dan mencetak address map. Jika device tidak ditemukan, status ditandai offline tetapi sistem tetap berjalan.

---

### Slide 36 — Alur Normal Project

Pada mode normal, sistem membaca sensor periodik, memvalidasi range data, memperbarui OLED/LCD, membuat record log, menulis ke EEPROM circular, mengirim data antar-MCU, dan mencatat error count. Loop tidak boleh blocking terlalu lama.

---

### Slide 37 — Error Mode Project

Jika terjadi NACK, timeout, atau sensor disconnect, sistem menandai sensor offline dan tetap memakai sensor lain. Recovery dilakukan berkala. Jika SDA/SCL stuck, sistem pulse SCL 9 kali, membuat STOP, init ulang I2C, lalu scan ulang.

---

### Slide 38 — Troubleshooting Wajib

Troubleshooting utama: cek GND bersama, SDA/SCL tidak tertukar, pull-up ke 3.3 V, konflik address DS3231-MPU6050, clock speed terlalu tinggi, page boundary EEPROM, LCD pull-up 5 V, dan bus stuck. Scanner adalah langkah debug pertama.

---

### Slide 39 — Deliverable Laporan

Laporan memuat teori I2C, tabel 25 eksperimen, foto hardware, screenshot output, analisis error, source code, dan integrasi project. Bahasa Indonesia wajib. Penamaan harus konsisten memakai Modul 07.

---

### Slide 40 — Tugas Video Modul 07

Video berdurasi 20–35 menit. Isi video: pembukaan, teori I2C, demo 10 ESP32, demo 10 STM32, demo 5 Multi STM32-ESP32, project final, troubleshooting, dan kesimpulan. Rekaman hardware dan screen recording wajib.

---

### Slide 41 — Checklist Demo ESP32

Demo ESP32 harus menunjukkan scanner, BME/BMP fallback, OLED, DS3231/RTC/SNTP, EEPROM logger, MPU6050, BH1750, LCD, GPIO matrix/speed test, dan recovery. Output minimal berupa serial log, perubahan sensor, serta display yang terlihat.

---

### Slide 42 — Checklist Demo STM32

Demo STM32 harus menunjukkan HAL scanner, raw driver BME/BMP, OLED via HAL, DS3231 plus internal RTC, EEPROM page write, MPU6050 burst read, BH1750 mode, LCD PCF8574, DMA benchmark, dan HAL error recovery.

---

### Slide 43 — Checklist Demo Multi

Demo multi harus menunjukkan STM32 sensor node ke ESP32 gateway, bus ownership request/grant, sinkronisasi SNTP ke DS3231/STM32, multi-display dashboard, dan final integration. Bukti utama: data konsisten di ESP32, STM32, OLED, LCD, dan serial monitor.

---

### Slide 44 — Rubrik Penilaian

Penilaian mencakup teori I2C, demo ESP32, demo STM32, demo Multi, project weather station, hardware wiring, kualitas video, dan analisis troubleshooting. Penalti diberikan untuk penamaan modul tidak konsisten, demo tidak lengkap, klaim EEPROM salah, tidak menjelaskan fallback, atau tanpa hardware.

---

### Slide 45 — Kesimpulan Modul 07

Modul 07 membangun keterampilan I2C lengkap: teori bus, sensor, display, RTC, EEPROM, STM32 HAL/DMA, ESP32 ESP-IDF/GPIO matrix/SNTP, recovery, bus ownership, dan integrasi project. Hasil akhir adalah weather station dual-MCU dengan 25 eksperimen terdokumentasi.
