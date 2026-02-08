# Jobsheet Modul 06: Komunikasi Cerdas Antar-Chip — I2C Bus & Sensor Integration

## Praktikum Sistem Embedded

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami protokol I2C** — Menjelaskan prinsip kerja komunikasi I2C (SDA, SCL, Start/Stop condition, addressing 7-bit)
2. **Melakukan scanning perangkat I2C** — Mendeteksi alamat semua perangkat yang terhubung pada bus I2C
3. **Mengintegrasikan berbagai sensor I2C** — Mengkonfigurasi dan membaca data dari sensor suhu, tekanan, akselerometer, light sensor, RTC, dan EEPROM
4. **Menampilkan data pada display I2C** — Menggunakan OLED SSD1306 dan LCD 16x2 via I2C backpack untuk menampilkan informasi
5. **Menangani error dan optimasi I2C** — Memahami pull-up resistor, clock stretching, bus recovery, dan perbandingan kecepatan 100kHz vs 400kHz

---

## 2. Peralatan yang Dibutuhkan

### Hardware

| No | Komponen | Jumlah | Alamat I2C | Keterangan |
|----|----------|--------|------------|------------|
| 1 | ESP32 DevKit V1 | 1 | - | SDA=GPIO21, SCL=GPIO22 |
| 2 | STM32 Blue Pill (F103) / Black Pill (F411) | 1 | - | SCL=PB6, SDA=PB7 |
| 3 | OLED Display SSD1306 0.96" | 1 | 0x3C | 128×64 piksel, I2C |
| 4 | Sensor BMP280 | 1 | 0x76 (atau 0x77) | Suhu + tekanan barometrik |
| 5 | Sensor MPU6050 | 1 | 0x68 (atau 0x69) | Akselerometer + giroskop 6-axis |
| 6 | EEPROM AT24C32 | 1 | 0x50 (0x50-0x57) | 4KB memory, biasanya di modul DS3231 |
| 7 | RTC DS3231 | 1 | 0x68 | Real-Time Clock presisi tinggi |
| 8 | Sensor BH1750 | 1 | 0x23 (atau 0x5C) | Sensor cahaya ambient (lux) |
| 9 | LCD 16×2 + I2C Backpack PCF8574 | 1 | 0x27 (atau 0x3F) | Display karakter |
| 10 | Resistor 4.7kΩ | 2 | - | Pull-up untuk SDA dan SCL |
| 11 | Resistor 10kΩ | 2 | - | Pull-up alternatif |
| 12 | Breadboard full-size | 1 | - | Untuk prototyping |
| 13 | Kabel jumper | ~30 | - | Male-male & male-female |
| 14 | Multimeter digital | 1 | - | Untuk verifikasi tegangan |
| 15 | Logic analyzer (opsional) | 1 | - | Untuk menganalisis sinyal I2C |
| 16 | Kabel USB Micro/Type-C | 2 | - | Programming & power |

### Software
- **PlatformIO** di VS Code
- **Serial Monitor / Plotter**
- **Python 3.x** dengan `pyserial` dan `matplotlib`
- Library: `Adafruit SSD1306`, `Adafruit BMP280`, `Adafruit MPU6050`, `BH1750`, `LiquidCrystal_I2C`, `RTClib`

---

## 3. Teori Singkat

### 3.1 Protokol I2C (Inter-Integrated Circuit)

I2C adalah protokol komunikasi serial **half-duplex** yang menggunakan 2 jalur:
- **SDA (Serial Data)** — jalur data bidirectional
- **SCL (Serial Clock)** — jalur clock, dikendalikan oleh master

Karakteristik utama:
- **Multi-master, multi-slave** — satu bus dapat memiliki beberapa master dan slave
- **Addressing** — setiap perangkat memiliki alamat unik 7-bit (0x00-0x7F)
- **Open-drain** — memerlukan **pull-up resistor** pada SDA dan SCL
- **Acknowledgment (ACK/NACK)** — slave memberikan konfirmasi setiap byte

### 3.2 Sinyal I2C

Urutan komunikasi I2C:

```
START → Address (7-bit) + R/W → ACK → Data Byte → ACK → ... → STOP
```

- **START condition**: SDA turun saat SCL HIGH
- **STOP condition**: SDA naik saat SCL HIGH
- **ACK**: Receiver menarik SDA LOW setelah menerima 8 bit
- **NACK**: SDA tetap HIGH (tidak diakui)

### 3.3 Kecepatan I2C

| Mode | Clock Speed | Keterangan |
|------|------------|------------|
| Standard Mode | 100 kHz | Default, kompatibel semua device |
| Fast Mode | 400 kHz | Untuk transfer data lebih cepat |
| Fast Mode Plus | 1 MHz | Memerlukan driver pull-up khusus |
| High Speed | 3.4 MHz | Jarang digunakan di embedded |

### 3.4 Pull-up Resistor

Pull-up resistor **wajib** dipasang pada jalur SDA dan SCL karena output I2C bersifat open-drain:

$$R_{pull-up} = \frac{V_{CC} - V_{OL}}{I_{OL}}$$

Nilai umum:
- **4.7kΩ** — standar untuk 100kHz pada bus pendek
- **2.2kΩ** — untuk 400kHz atau bus yang lebih panjang
- **10kΩ** — untuk bus sangat pendek dengan sedikit device

> **Catatan:** Banyak modul breakout (OLED, BMP280, dll) sudah memiliki pull-up on-board. Jika menghubungkan banyak modul, pull-up yang terlalu banyak paralel dapat menyebabkan masalah.

### 3.5 Pin I2C

| Board | SDA | SCL | I2C Alternatif |
|-------|-----|-----|----------------|
| ESP32 | GPIO21 | GPIO22 | Bisa di-remap ke pin lain |
| STM32F103 | PB7 | PB6 | I2C2: PB11(SDA), PB10(SCL) |
| STM32F411 | PB7 | PB6 | I2C1/I2C2/I2C3 tersedia |

### 3.6 Alamat I2C Perangkat

| Perangkat | Alamat Default | Alamat Alternatif |
|-----------|---------------|-------------------|
| SSD1306 OLED | 0x3C | 0x3D |
| BMP280 | 0x76 | 0x77 (SDO=HIGH) |
| MPU6050 | 0x68 | 0x69 (AD0=HIGH) |
| AT24C32 EEPROM | 0x50 | 0x50-0x57 (A0-A2) |
| DS3231 RTC | 0x68 | Fixed |
| BH1750 | 0x23 | 0x5C (ADDR=HIGH) |
| PCF8574 LCD | 0x27 | 0x3F |

> **Perhatian:** MPU6050 dan DS3231 memiliki alamat yang sama (0x68)! Jangan gunakan keduanya bersamaan tanpa mengubah alamat MPU6050 ke 0x69 (pin AD0=HIGH).

---

## 4. Langkah Praktikum

> **Aturan Umum:**
> - Selalu pasang **pull-up resistor 4.7kΩ** pada SDA dan SCL (jika modul belum memilikinya)
> - Pastikan semua perangkat mendapat **power supply yang sesuai** (3.3V atau 5V)
> - Periksa **common ground** antara semua perangkat
> - Upload kode menggunakan `pio run -t upload`
> - Buka Serial Monitor pada **115200 baud**

---

### Program 01: I2C_Scanner

**Tujuan:** Memindai bus I2C dan menemukan alamat semua perangkat yang terhubung.

**Rangkaian:**

| Koneksi | ESP32 | STM32 |
|---------|-------|-------|
| SDA | GPIO21 | PB7 |
| SCL | GPIO22 | PB6 |
| Pull-up SDA | 4.7kΩ ke 3.3V | 4.7kΩ ke 3.3V |
| Pull-up SCL | 4.7kΩ ke 3.3V | 4.7kΩ ke 3.3V |
| Perangkat I2C | Hubungkan 1-4 modul I2C | Hubungkan 1-4 modul I2C |

**Langkah-langkah:**

1. Hubungkan minimal 2-3 modul I2C ke bus (misalnya OLED, BMP280, BH1750)
2. Pasang pull-up resistor 4.7kΩ pada SDA dan SCL ke 3.3V
3. Periksa semua koneksi power dan ground
4. Buka folder `ESP32/ESP32_01_I2C_Scanner`
5. Pelajari kode — program melakukan scanning dari alamat 0x01 hingga 0x7F
6. Untuk setiap alamat, program mengirim START + Address dan memeriksa ACK
7. Compile dan upload: `pio run -t upload`
8. Buka Serial Monitor (115200 baud)
9. Amati daftar alamat perangkat yang ditemukan
10. Cocokkan alamat yang ditemukan dengan datasheet setiap modul
11. Coba lepas satu modul dan scan ulang — verifikasi alamat hilang

**Pengamatan:**

| No | Alamat (Hex) | Perangkat | Status |
|----|-------------|-----------|--------|
| 1 | 0x____ | __________ | Terdeteksi / Tidak |
| 2 | 0x____ | __________ | Terdeteksi / Tidak |
| 3 | 0x____ | __________ | Terdeteksi / Tidak |
| 4 | 0x____ | __________ | Terdeteksi / Tidak |

Total perangkat terdeteksi: ______

**Pertanyaan:**
1. Mengapa alamat I2C hanya 7-bit (0x00-0x7F)? Apa fungsi bit ke-8?
2. Apa yang terjadi jika dua perangkat memiliki alamat yang sama pada satu bus?
3. Bagaimana cara mengubah alamat I2C pada modul yang mendukung address pin (misal BMP280)?

---

### Program 02: I2C_OLED_SSD1306

**Tujuan:** Menampilkan teks, grafik, dan data sensor pada display OLED SSD1306 128×64 piksel.

**Rangkaian:**

| Pin OLED | ESP32 | STM32 |
|----------|-------|-------|
| VCC | 3.3V | 3.3V |
| GND | GND | GND |
| SDA | GPIO21 | PB7 |
| SCL | GPIO22 | PB6 |

**Langkah-langkah:**

1. Hubungkan OLED SSD1306 ke bus I2C
2. Pastikan OLED mendapat power 3.3V (beberapa modul toleran 5V, cek datasheet)
3. Buka folder `ESP32/ESP32_02_I2C_OLED_SSD1306`
4. Pelajari kode — program menggunakan library Adafruit SSD1306 dan GFX:
   - `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)` — inisialisasi pada alamat 0x3C
   - `display.clearDisplay()` — bersihkan buffer
   - `display.setTextSize()`, `display.setCursor()`, `display.println()` — tulis teks
   - `display.display()` — kirim buffer ke OLED
5. Compile dan upload
6. Amati teks "Hello World" pada OLED
7. Program juga menampilkan counter dan uptime
8. Perhatikan refresh rate tampilan
9. Coba modifikasi ukuran teks dan posisi kursor

**Pengamatan:**
- Alamat I2C OLED: 0x____
- Resolusi display: ____× ____ piksel
- Teks berhasil ditampilkan: Ya / Tidak
- Refresh rate (kasar): ______ fps
- Kualitas tampilan: ______

**Pertanyaan:**
1. Apa perbedaan OLED dan LCD dalam hal konsumsi daya dan kontras?
2. Berapa byte data yang diperlukan untuk mengirim satu frame penuh ke OLED 128×64?
3. Mengapa kita perlu memanggil `display.display()` setelah menulis ke buffer?

---

### Program 03: I2C_Temp_BMP280

**Tujuan:** Membaca data suhu dan tekanan barometrik dari sensor BMP280.

**Rangkaian:**

| Pin BMP280 | ESP32 | STM32 | Keterangan |
|------------|-------|-------|------------|
| VCC | 3.3V | 3.3V | Cek modul, beberapa punya regulator 5V |
| GND | GND | GND | |
| SDA | GPIO21 | PB7 | |
| SCL | GPIO22 | PB6 | |
| SDO | GND atau NC | GND atau NC | LOW=0x76, HIGH=0x77 |
| CSB | 3.3V | 3.3V | HIGH untuk mode I2C |

**Langkah-langkah:**

1. Hubungkan BMP280 ke bus I2C
2. Pastikan pin CSB terhubung ke 3.3V (mode I2C, bukan SPI)
3. Cek alamat: SDO ke GND = 0x76, SDO ke VCC = 0x77
4. Buka folder `ESP32/ESP32_03_I2C_Temp_BMP280`
5. Pelajari kode — program membaca register suhu dan tekanan:
   - Inisialisasi dengan oversampling yang dikonfigurasi
   - Pembacaan suhu (°C) dan tekanan (hPa / mbar)
   - Perhitungan estimasi ketinggian dari tekanan: $h = 44330 \times \left(1 - \left(\frac{P}{P_0}\right)^{0.1903}\right)$
6. Compile dan upload
7. Buka Serial Monitor — amati pembacaan suhu dan tekanan
8. Bandingkan suhu dengan termometer referensi
9. Coba tiup sensor (napas hangat) dan amati perubahan suhu
10. Catat tekanan atmosfer dan hitung estimasi ketinggian

**Pengamatan:**

| Parameter | Nilai Sensor | Nilai Referensi | Selisih |
|-----------|-------------|-----------------|---------|
| Suhu (°C) | ______ | ______ | ______ |
| Tekanan (hPa) | ______ | ______ | ______ |
| Ketinggian (m) | ______ | ______ (GPS) | ______ |

**Pertanyaan:**
1. Apa fungsi oversampling pada BMP280? Apa trade-off antara resolusi dan kecepatan?
2. Mengapa tekanan atmosfer berkurang seiring bertambahnya ketinggian?
3. Berapa resolusi suhu dan tekanan BMP280 pada setting default?

---

### Program 04: I2C_Accel_MPU6050

**Tujuan:** Membaca data akselerometer 3-axis dan giroskop 3-axis dari sensor MPU6050.

**Rangkaian:**

| Pin MPU6050 | ESP32 | STM32 | Keterangan |
|-------------|-------|-------|------------|
| VCC | 3.3V | 3.3V | |
| GND | GND | GND | |
| SDA | GPIO21 | PB7 | |
| SCL | GPIO22 | PB6 | |
| AD0 | GND | GND | LOW=0x68, HIGH=0x69 |
| INT | NC | NC | Opsional untuk interrupt |

**Langkah-langkah:**

1. Hubungkan MPU6050 ke bus I2C
2. Sambungkan AD0 ke GND (alamat 0x68)
3. Buka folder `ESP32/ESP32_04_I2C_Accel_MPU6050`
4. Pelajari kode:
   - Inisialisasi MPU6050 (wake up dari sleep mode — register 0x6B)
   - Konfigurasi range akselerometer (±2g, ±4g, ±8g, ±16g)
   - Konfigurasi range giroskop (±250, ±500, ±1000, ±2000 °/s)
   - Baca 14 byte data (Accel XYZ, Temp, Gyro XYZ)
5. Compile dan upload
6. Buka Serial Monitor — amati data 6 axis secara real-time
7. Letakkan sensor datar → akselerometer Z ≈ 1g (9.8 m/s²)
8. Miringkan sensor ke berbagai arah → amati perubahan nilai X, Y, Z
9. Putar sensor → amati pembacaan giroskop
10. Hitung sudut inklinasi dari data akselerometer

**Pengamatan:**

| Posisi Sensor | Accel X | Accel Y | Accel Z | Gyro X | Gyro Y | Gyro Z |
|---------------|---------|---------|---------|--------|--------|--------|
| Datar | ______ | ______ | ______ | ______ | ______ | ______ |
| Miring 45° X | ______ | ______ | ______ | ______ | ______ | ______ |
| Miring 45° Y | ______ | ______ | ______ | ______ | ______ | ______ |
| Tegak (Z up) | ______ | ______ | ______ | ______ | ______ | ______ |

**Pertanyaan:**
1. Apa perbedaan akselerometer dan giroskop dalam mengukur orientasi?
2. Mengapa nilai akselerometer Z menunjukkan ~1g saat sensor datar dan diam?
3. Bagaimana cara menghitung sudut roll dan pitch dari data akselerometer?

$$roll = \arctan\left(\frac{a_y}{\sqrt{a_x^2 + a_z^2}}\right)$$

$$pitch = \arctan\left(\frac{-a_x}{\sqrt{a_y^2 + a_z^2}}\right)$$

---

### Program 05: I2C_EEPROM_AT24C32

**Tujuan:** Menyimpan dan membaca data dari EEPROM AT24C32 (4KB) melalui I2C.

**Rangkaian:**

| Pin AT24C32 | ESP32 | STM32 | Keterangan |
|-------------|-------|-------|------------|
| VCC | 3.3V | 3.3V | |
| GND | GND | GND | |
| SDA | GPIO21 | PB7 | |
| SCL | GPIO22 | PB6 | |
| A0, A1, A2 | GND | GND | Alamat 0x50 |

> **Catatan:** Modul DS3231 biasanya sudah dilengkapi AT24C32 on-board.

**Langkah-langkah:**

1. Hubungkan modul EEPROM (atau modul DS3231 yang memiliki AT24C32)
2. Pastikan pin A0, A1, A2 terhubung ke GND (alamat 0x50)
3. Buka folder `ESP32/ESP32_05_I2C_EEPROM_AT24C32`
4. Pelajari kode:
   - **Write byte**: Kirim alamat memori 2-byte + data
   - **Read byte**: Kirim alamat memori, lalu baca data
   - **Page write**: Tulis hingga 32 byte sekaligus (page boundary!)
   - **Write delay**: Tunggu 5ms setelah setiap write operation
5. Compile dan upload
6. Program akan menulis beberapa data uji ke EEPROM
7. Kemudian membaca kembali dan memverifikasi data
8. Coba tulis string "Hello EEPROM" ke alamat 0x0000
9. Reset board — baca kembali data → data harus tetap tersimpan (non-volatile)
10. Coba tulis dan baca pada alamat berbeda

**Pengamatan:**

| Operasi | Alamat | Data Ditulis | Data Dibaca | Cocok? |
|---------|--------|-------------|-------------|--------|
| Byte write | 0x0000 | 0xAB | 0x____ | Ya/Tidak |
| Byte write | 0x0001 | 0xCD | 0x____ | Ya/Tidak |
| Page write | 0x0010 | "Hello" | "______" | Ya/Tidak |
| Setelah reset | 0x0000 | - | 0x____ | Retained? |

**Pertanyaan:**
1. Apa yang dimaksud dengan "page boundary" pada EEPROM? Mengapa penting saat page write?
2. Berapa kapasitas total AT24C32 dalam byte? Berapa range alamat memorinya?
3. Mengapa perlu write delay 5ms setelah operasi write? Apa yang terjadi jika tidak menunggu?

---

### Program 06: I2C_RTC_DS3231

**Tujuan:** Mengatur dan membaca waktu dari modul RTC DS3231 (Real-Time Clock presisi tinggi).

**Rangkaian:**

| Pin DS3231 | ESP32 | STM32 | Keterangan |
|------------|-------|-------|------------|
| VCC | 3.3V | 3.3V | Juga mendukung 5V |
| GND | GND | GND | |
| SDA | GPIO21 | PB7 | |
| SCL | GPIO22 | PB6 | |
| 32K | NC | NC | Output 32.768kHz (opsional) |
| SQW | NC | NC | Alarm/Square wave (opsional) |

**Langkah-langkah:**

1. Hubungkan modul DS3231 ke bus I2C
2. Pastikan baterai coin cell (CR2032) terpasang pada modul
3. Buka folder `ESP32/ESP32_06_I2C_RTC_DS3231`
4. Pelajari kode:
   - Set waktu: tulis jam, menit, detik, tanggal ke register DS3231
   - Baca waktu: baca register secara periodik
   - Data dalam format BCD (Binary-Coded Decimal) — perlu konversi
   - Baca suhu internal DS3231 (resolusi 0.25°C)
5. Compile dan upload
6. Program pertama kali akan mengatur waktu sesuai waktu compile
7. Buka Serial Monitor — amati tampilan waktu real-time
8. Catat waktu yang ditampilkan dan bandingkan dengan jam referensi
9. Cabut USB (power off) selama 1 menit → sambung kembali
10. Verifikasi bahwa RTC tetap menjaga waktu (battery backup)

**Pengamatan:**
- Waktu yang di-set: ____:____:____
- Waktu yang dibaca: ____:____:____
- Setelah power off 1 menit: ____:____:____
- Akurasi: tepat / terlambat ____detik / cepat ____detik
- Suhu internal DS3231: ______°C

**Pertanyaan:**
1. Apa fungsi baterai coin cell pada modul RTC?
2. Apa format BCD dan mengapa DS3231 menggunakan format ini?
3. Berapa akurasi DS3231 per hari/bulan dibandingkan dengan crystal biasa?

---

### Program 07: I2C_Light_BH1750

**Tujuan:** Mengukur intensitas cahaya ambient (lux) menggunakan sensor BH1750.

**Rangkaian:**

| Pin BH1750 | ESP32 | STM32 | Keterangan |
|------------|-------|-------|------------|
| VCC | 3.3V | 3.3V | |
| GND | GND | GND | |
| SDA | GPIO21 | PB7 | |
| SCL | GPIO22 | PB6 | |
| ADDR | GND | GND | LOW=0x23, HIGH=0x5C |

**Langkah-langkah:**

1. Hubungkan sensor BH1750 ke bus I2C
2. Sambungkan pin ADDR ke GND (alamat 0x23)
3. Buka folder `ESP32/ESP32_07_I2C_Light_BH1750`
4. Pelajari kode:
   - Inisialisasi sensor dengan mode pengukuran (continuously / one-time)
   - Mode resolusi: High (1 lux), High2 (0.5 lux), Low (4 lux)
   - Baca 2 byte data cahaya dan konversi ke lux
5. Compile dan upload
6. Buka Serial Monitor — amati pembacaan lux
7. Uji dengan berbagai kondisi cahaya:
   - Tutup sensor dengan tangan (gelap)
   - Cahaya ruangan normal
   - Arahkan lampu senter ke sensor
   - Cahaya matahari langsung (jika memungkinkan)
8. Catat hasil pengukuran untuk setiap kondisi

**Pengamatan:**

| Kondisi Cahaya | Nilai Lux | Keterangan |
|----------------|-----------|------------|
| Gelap total | ______ lux | Sensor ditutupi |
| Ruangan redup | ______ lux | Lampu 1-2 |
| Ruangan terang | ______ lux | Semua lampu nyala |
| Senter langsung | ______ lux | Jarak 10cm |
| Cahaya matahari | ______ lux | Outdoor |

Referensi: Gelap=0-10 lux, Ruangan=100-500 lux, Outdoor=10000-100000 lux

**Pertanyaan:**
1. Apa perbedaan mode resolusi High, High2, dan Low pada BH1750?
2. Berapa waktu pengukuran untuk masing-masing mode resolusi?
3. Apa aplikasi praktis pengukuran lux dalam sistem embedded? Berikan 3 contoh.

---

### Program 08: I2C_LCD_PCF8574

**Tujuan:** Menampilkan teks pada LCD 16×2 karakter menggunakan I2C backpack PCF8574.

**Rangkaian:**

| Pin I2C LCD | ESP32 | STM32 | Keterangan |
|-------------|-------|-------|------------|
| VCC | 5V (Vin) | 5V | LCD memerlukan 5V |
| GND | GND | GND | |
| SDA | GPIO21 | PB7 | Level 3.3V OK (PCF8574 toleran) |
| SCL | GPIO22 | PB6 | Level 3.3V OK |

> **Catatan:** LCD 16×2 memerlukan 5V. Gunakan pin Vin/5V pada board. PCF8574 I2C backpack biasanya toleran terhadap level 3.3V pada SDA/SCL.

**Langkah-langkah:**

1. Hubungkan modul LCD I2C ke bus
2. Pastikan power 5V tersedia untuk LCD
3. Atur kontras dengan trimpot di belakang modul I2C (putar sampai teks terlihat)
4. Buka folder `ESP32/ESP32_08_I2C_LCD_PCF8574`
5. Pelajari kode — library LiquidCrystal_I2C:
   - `lcd.init()` — inisialisasi LCD
   - `lcd.backlight()` — nyalakan backlight
   - `lcd.setCursor(col, row)` — posisi kursor (0-15, 0-1)
   - `lcd.print("text")` — cetak teks
   - `lcd.clear()` — bersihkan tampilan
6. Compile dan upload
7. Amati teks "Hello World!" pada baris 1 dan counter pada baris 2
8. Jika layar kosong → putar trimpot kontras
9. Jika tetap kosong → cek alamat I2C (bisa 0x27 atau 0x3F, scan ulang)
10. Coba tampilkan karakter khusus (custom character)

**Pengamatan:**
- Alamat I2C LCD: 0x____
- Teks berhasil ditampilkan: Ya / Tidak
- Jumlah kolom × baris: ____ × ____
- Backlight berfungsi: Ya / Tidak
- Kontras sudah diatur: Ya / Tidak

**Pertanyaan:**
1. Apa fungsi PCF8574 pada modul I2C LCD? Berapa pin GPIO yang dihemat?
2. Mengapa LCD memerlukan 5V sedangkan ESP32 beroperasi pada 3.3V? Apakah ini menyebabkan masalah?
3. Bagaimana cara membuat custom character pada LCD 16×2? Berapa piksel per karakter?

---

### Program 09: I2C_Multi_Sensor

**Tujuan:** Mengoperasikan beberapa perangkat I2C secara bersamaan pada satu bus.

**Rangkaian:**

Hubungkan **minimal 3 perangkat** I2C ke bus yang sama:

| Perangkat | Alamat | SDA | SCL | Power |
|-----------|--------|-----|-----|-------|
| OLED SSD1306 | 0x3C | GPIO21 | GPIO22 | 3.3V |
| BMP280 | 0x76 | GPIO21 | GPIO22 | 3.3V |
| BH1750 | 0x23 | GPIO21 | GPIO22 | 3.3V |
| Pull-up 4.7kΩ | - | Ke 3.3V | Ke 3.3V | - |

> **Catatan:** Semua perangkat berbagi jalur SDA dan SCL yang sama. Hanya perlu **satu pasang** pull-up resistor.

**Langkah-langkah:**

1. Hubungkan minimal 3 sensor/display I2C pada bus yang sama
2. Pastikan setiap perangkat memiliki alamat yang **berbeda**
3. Pasang pull-up resistor 4.7kΩ (hanya 1 pasang untuk seluruh bus)
4. Buka folder `ESP32/ESP32_09_I2C_Multi_Sensor`
5. Pelajari kode:
   - Inisialisasi semua perangkat secara berurutan
   - Baca data dari BMP280 (suhu, tekanan)
   - Baca data dari BH1750 (lux)
   - Tampilkan semua data pada OLED SSD1306
   - Update data secara periodik (setiap 1 detik)
6. Compile dan upload
7. Amati OLED yang menampilkan data dari berbagai sensor
8. Serial Monitor juga menampilkan semua pembacaan
9. Coba lepas satu sensor → amati apakah sensor lain tetap berfungsi
10. Perhatikan error handling saat sensor tidak tersedia

**Pengamatan:**
- Jumlah perangkat yang terdeteksi: ______
- Semua sensor dapat dibaca bersamaan: Ya / Tidak
- Tampilan OLED menampilkan data lengkap: Ya / Tidak
- Saat 1 sensor dilepas, sensor lain: tetap jalan / error
- Waktu total polling semua sensor: ______ ms

**Pertanyaan:**
1. Berapa jumlah maksimum perangkat I2C yang bisa dihubungkan ke satu bus? Apa faktor pembatasnya?
2. Mengapa hanya perlu satu pasang pull-up resistor meskipun ada banyak perangkat?
3. Apa yang terjadi jika total kapasitansi bus I2C terlalu besar?

---

### Program 10: I2C_Write_Read_Raw

**Tujuan:** Memahami transaksi I2C level rendah — mengirim dan menerima data secara manual tanpa library sensor.

**Rangkaian:** Gunakan salah satu sensor yang sudah terhubung (misal BMP280 pada 0x76).

**Langkah-langkah:**

1. Gunakan sensor yang sudah terhubung dari program sebelumnya
2. Buka folder `ESP32/ESP32_10_I2C_Write_Read_Raw`
3. Pelajari kode — program mengakses register sensor secara langsung:
   ```
   Wire.beginTransmission(address);  // START + Address + Write
   Wire.write(register_address);     // Kirim alamat register
   Wire.endTransmission(false);      // Repeated START (bukan STOP)
   Wire.requestFrom(address, count); // START + Address + Read, baca N byte
   data = Wire.read();               // Ambil data dari buffer
   ```
4. Compile dan upload
5. Program membaca Chip ID register BMP280 (register 0xD0, value harusnya 0x58)
6. Amati raw bytes yang diterima pada Serial Monitor
7. Coba baca register lain: status (0xF3), control (0xF4), config (0xF5)
8. Tulis konfigurasi ke register control measurement
9. Bandingkan dengan pembacaan menggunakan library (Program 03)

**Pengamatan:**

| Register | Alamat | Nilai Baca (Hex) | Nilai Expected | Cocok? |
|----------|--------|-----------------|----------------|--------|
| Chip ID | 0xD0 | 0x____ | 0x58 | Ya/Tidak |
| Status | 0xF3 | 0x____ | - | - |
| Ctrl Meas | 0xF4 | 0x____ | - | - |
| Config | 0xF5 | 0x____ | - | - |

**Pertanyaan:**
1. Apa perbedaan `Wire.endTransmission(true)` dan `Wire.endTransmission(false)`?
2. Mengapa digunakan "repeated start" saat membaca register sensor?
3. Apa keuntungan dan kerugian menggunakan raw I2C vs library sensor?

---

### Program 11: I2C_Clock_Speed_Test

**Tujuan:** Membandingkan kecepatan transfer I2C pada mode Standard (100kHz) vs Fast (400kHz).

**Rangkaian:** Gunakan OLED SSD1306 yang sudah terhubung.

**Langkah-langkah:**

1. Gunakan OLED SSD1306 atau sensor yang sudah terhubung
2. Buka folder `ESP32/ESP32_11_I2C_Clock_Speed_Test`
3. Pelajari kode:
   - `Wire.begin(SDA, SCL)` — inisialisasi I2C
   - `Wire.setClock(100000)` — set kecepatan 100kHz
   - `Wire.setClock(400000)` — set kecepatan 400kHz
   - Program mengukur waktu transfer data yang sama pada kedua kecepatan
4. Compile dan upload
5. Program mengirim data yang sama ke OLED pada 100kHz, lalu 400kHz
6. Serial Monitor menampilkan waktu transfer untuk setiap kecepatan
7. Bandingkan waktu transfer dan hitung rasio percepatan
8. Coba juga kecepatan 50kHz dan 800kHz (jika device mendukung)
9. Amati apakah ada error pada kecepatan tinggi

**Pengamatan:**

| Clock Speed | Waktu Transfer (ms) | Data Rate Efektif | Error Count |
|-------------|--------------------|--------------------|-------------|
| 50 kHz | ______ ms | ______ bytes/s | ______ |
| 100 kHz | ______ ms | ______ bytes/s | ______ |
| 400 kHz | ______ ms | ______ bytes/s | ______ |
| 800 kHz | ______ ms | ______ bytes/s | ______ |

Rasio percepatan 400kHz vs 100kHz: ______ x

**Pertanyaan:**
1. Mengapa rasio kecepatan 400kHz vs 100kHz tidak tepat 4x? Faktor apa yang mempengaruhi?
2. Apa risiko menggunakan clock speed yang terlalu tinggi pada I2C?
3. Bagaimana panjang kabel (kapasitansi bus) mempengaruhi kecepatan I2C maksimum yang stabil?

---

### Program 12: I2C_Error_Recovery

**Tujuan:** Menangani error pada bus I2C dan melakukan recovery dari kondisi bus hang.

**Rangkaian:** Gunakan sensor yang sudah terhubung. Siapkan kabel jumper untuk simulasi error.

**Langkah-langkah:**

1. Hubungkan sensor I2C yang sudah dikonfigurasi
2. Buka folder `ESP32/ESP32_12_I2C_Error_Recovery`
3. Pelajari kode — program mengimplementasikan:
   - **Timeout detection**: Mendeteksi jika perangkat tidak merespons
   - **Bus recovery**: Toggle SCL 9 kali + STOP condition untuk melepas SDA
   - **Error codes**: Menginterpretasi return value dari `Wire.endTransmission()`
     - 0: Success
     - 1: Data too long
     - 2: NACK on address
     - 3: NACK on data
     - 4: Other error
     - 5: Timeout
   - **Retry mechanism**: Coba ulang komunikasi dengan delay
4. Compile dan upload
5. Amati komunikasi normal pada Serial Monitor
6. **Simulasi error**: Cabut kabel SDA saat program berjalan
7. Amati pesan error yang muncul
8. Sambung kembali SDA → amati apakah program berhasil recovery
9. **Simulasi bus hang**: Cabut kabel lalu sambung cepat saat tengah transfer
10. Amati proses bus recovery (9 clock pulses pada SCL)

**Pengamatan:**

| Skenario | Error Code | Pesan | Recovery Berhasil? |
|----------|-----------|-------|-------------------|
| Normal | 0 | Success | - |
| SDA dilepas | ______ | ______ | Ya / Tidak |
| SCL dilepas | ______ | ______ | Ya / Tidak |
| Bus hang | ______ | ______ | Ya / Tidak |
| Setelah recovery | ______ | ______ | - |

**Pertanyaan:**
1. Apa penyebab umum bus I2C "hang" (SDA terjebak LOW)?
2. Mengapa bus recovery memerlukan 9 clock pulses pada SCL?
3. Bagaimana cara mengimplementasikan watchdog untuk mendeteksi I2C timeout pada produk real?

---

## 5. Tugas Tambahan

### Tugas 1: Weather Station
Buat sistem stasiun cuaca mini yang membaca data BMP280 (suhu + tekanan), BH1750 (cahaya), dan menampilkan semua data pada OLED SSD1306. Data di-update setiap 2 detik. Tampilkan juga trend suhu (naik/turun) menggunakan ikon panah.

### Tugas 2: Data Logger
Gunakan BMP280 + BH1750 untuk membaca data sensor, simpan ke EEPROM AT24C32 dengan timestamp dari DS3231. Implementasikan:
- Format penyimpanan: timestamp (4 byte) + suhu (2 byte) + lux (2 byte) = 8 byte per record
- Hitung berapa record yang bisa disimpan di 4KB EEPROM
- Baca kembali semua data yang tersimpan dan tampilkan di Serial Monitor

### Tugas 3: Alarm System
Buat sistem alarm menggunakan BH1750 (deteksi cahaya) dan DS3231 (jadwal):
- Set threshold cahaya untuk trigger alarm
- Set jadwal aktif/nonaktif alarm (misal aktif jam 22:00-06:00)
- Tampilkan status pada LCD 16×2
- Kirim notifikasi via Serial saat alarm aktif

### Tugas 4: I2C Bus Analyzer
Buat program yang bertindak sebagai I2C bus analyzer sederhana:
- Scan dan identifikasi semua perangkat pada bus
- Tampilkan peta alamat I2C (occupied vs free)
- Ukur kecepatan transfer untuk setiap perangkat
- Cek status pull-up resistor (cukup/kurang)
- Tampilkan laporan lengkap pada Serial Monitor

---

## 6. Format Laporan

Laporan praktikum harus mencakup:

1. **Cover** — Judul, nama, NIM, tanggal
2. **Tujuan** — Tujuan praktikum
3. **Dasar Teori** — Ringkasan protokol I2C, diagram timing, penjelasan addressing
4. **Alat dan Bahan** — Daftar komponen + alamat I2C masing-masing
5. **Langkah Kerja** — Prosedur yang dilakukan untuk setiap program
6. **Data Pengamatan** — Tabel lengkap untuk setiap program (minimal 6 dari 12 program)
7. **Analisis** — Penjelasan hasil, analisis error, perbandingan kecepatan I2C
8. **Jawaban Pertanyaan** — Jawaban pertanyaan dari setiap program yang dikerjakan
9. **Kesimpulan** — Rangkuman pengetahuan yang didapat
10. **Lampiran** — Screenshot Serial Monitor, foto rangkaian, kode program, diagram koneksi I2C

---

## 7. Tips Debugging dengan Python

### I2C Data Logger & Visualizer

```python
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from datetime import datetime

# Konfigurasi serial
PORT = '/dev/ttyUSB0'  # Sesuaikan dengan port board
BAUD = 115200
MAX_POINTS = 300

ser = serial.Serial(PORT, BAUD, timeout=1)

# Buffer data
temp_data = deque(maxlen=MAX_POINTS)
press_data = deque(maxlen=MAX_POINTS)
lux_data = deque(maxlen=MAX_POINTS)
time_data = deque(maxlen=MAX_POINTS)

fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))

def update(frame):
    try:
        line = ser.readline().decode().strip()
        if ',' in line:
            parts = line.split(',')
            if len(parts) >= 3:
                temp = float(parts[0])
                press = float(parts[1])
                lux = float(parts[2])

                now = datetime.now().strftime('%H:%M:%S')
                temp_data.append(temp)
                press_data.append(press)
                lux_data.append(lux)
                time_data.append(len(time_data))

                for ax in (ax1, ax2, ax3):
                    ax.clear()

                ax1.plot(list(time_data), list(temp_data), 'r-')
                ax1.set_ylabel('Temperature (°C)')
                ax1.set_title('Multi-Sensor I2C Dashboard')

                ax2.plot(list(time_data), list(press_data), 'b-')
                ax2.set_ylabel('Pressure (hPa)')

                ax3.plot(list(time_data), list(lux_data), 'g-')
                ax3.set_ylabel('Light (lux)')
                ax3.set_xlabel('Sample')
    except (ValueError, UnicodeDecodeError):
        pass

ani = animation.FuncAnimation(fig, update, interval=100)
plt.tight_layout()
plt.show()
ser.close()
```

### I2C Bus Scanner via Python

```python
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=2)
time.sleep(2)  # Tunggu board reset

# Kirim perintah scan (jika firmware mendukung)
ser.write(b'SCAN\n')

known_devices = {
    0x23: 'BH1750 Light Sensor',
    0x27: 'PCF8574 LCD Backpack',
    0x3C: 'SSD1306 OLED',
    0x3F: 'PCF8574A LCD Backpack',
    0x50: 'AT24C32 EEPROM',
    0x68: 'DS3231 RTC / MPU6050',
    0x76: 'BMP280 Sensor',
    0x77: 'BMP280 (alt) / BME280',
}

print("I2C Bus Scan Results:")
print("-" * 45)

while True:
    line = ser.readline().decode().strip()
    if not line:
        break
    if line.startswith('0x'):
        addr = int(line, 16)
        device = known_devices.get(addr, 'Unknown Device')
        print(f"  Address {line} -> {device}")

ser.close()
print("-" * 45)
print("Scan complete.")
```

---

## 8. Referensi

1. NXP I2C-bus Specification and User Manual (UM10204)
2. ESP32 Technical Reference Manual — I2C Controller chapter
3. STM32F1/F4 Reference Manual — I2C Controller chapters
4. Datasheet: SSD1306, BMP280, MPU6050, AT24C32, DS3231, BH1750, PCF8574
5. Adafruit I2C Guide: https://learn.adafruit.com/working-with-i2c-devices
6. ESP-IDF I2C API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html

---

> **Catatan Penting:**
> - ESP32: SDA default = **GPIO21**, SCL default = **GPIO22** (bisa di-remap)
> - STM32: I2C1 SCL = **PB6**, SDA = **PB7** (fixed pada F103)
> - **Pull-up resistor 4.7kΩ wajib** dipasang pada SDA dan SCL (kecuali modul sudah memilikinya)
> - **MPU6050 dan DS3231 sama-sama menggunakan alamat 0x68** — jangan gunakan bersamaan tanpa mengubah alamat MPU6050 (AD0=HIGH → 0x69)
> - Panjang kabel bus I2C sebaiknya **tidak melebihi 1 meter** pada 400kHz
> - Jika bus tidak stabil, coba turunkan clock speed atau kurangi nilai pull-up resistor (misal ke 2.2kΩ)
> - Selalu periksa **common ground** antara semua perangkat pada bus I2C
