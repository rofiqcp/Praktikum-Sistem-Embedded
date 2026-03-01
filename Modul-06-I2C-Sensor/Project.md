# Project Modul 06: Weather Station I2C Multi-Sensor

## Informasi Project

| Item | Keterangan |
|------|------------|
| Modul | 06 — I2C Bus dan Sensor Integration |
| Platform | STM32F103C8T6 + ESP32 DevKit V1 |
| Durasi | 2 minggu |
| Tipe | Dual-MCU multi-sensor I2C system |

---

## Deskripsi Umum

Project ini mengintegrasikan seluruh 12 percobaan I2C — meliputi scanner, OLED SSD1306, BMP280, MPU6050, EEPROM AT24C32, RTC DS3231, BH1750, LCD PCF8574, multi-sensor, raw read/write, clock speed test, dan error recovery — ke dalam sistem weather station yang komprehensif.

---

## Soal Cerita

### Skenario: Stasiun Cuaca Otomatis untuk Pertanian Cerdas

Koperasi petani "Tani Maju" di Kabupaten Malang ingin memasang stasiun cuaca otomatis di 5 lahan pertanian mereka. Setiap stasiun harus mampu mengukur suhu, tekanan udara, kelembaban, intensitas cahaya, dan orientasi arah angin (menggunakan akselerometer sebagai proxy). Data harus ditampilkan langsung di layar LCD untuk petani yang mengecek langsung, dan juga disimpan ke memori non-volatile agar tidak hilang saat mati listrik.

**ESP32 DevKit V1 (Main Weather Station)** berfungsi sebagai stasiun utama. Saat pertama kali dinyalakan, ESP32 melakukan **I2C bus scan** untuk mendeteksi semua sensor yang terhubung dan melaporkan alamat yang ditemukan. Kemudian ESP32 membaca data dari **BMP280** (suhu + tekanan), **BH1750** (intensitas cahaya), dan **MPU6050** (akselerometer untuk deteksi guncangan/gempa kecil). Semua data ditampilkan pada **OLED SSD1306** dengan format grafis — menampilkan ikon cuaca, grafik suhu 24 jam terakhir, dan alert jika ada anomali.

Data sensor dibaca setiap 10 detik dan disimpan ke **EEPROM AT24C32** sebagai data log. EEPROM menyimpan hingga 1000 record terakhir secara circular. Setiap record memiliki timestamp dari modul **RTC DS3231** yang memberikan waktu akurat bahkan saat MCU di-restart. ESP32 juga menampilkan data pada **LCD PCF8574** (16×2) yang lebih mudah dibaca dari jarak jauh oleh petani.

Untuk optimasi, ESP32 menggunakan **multi-sensor read** — membaca semua sensor dalam satu siklus I2C tanpa delay berlebihan. Sistem juga melakukan **clock speed test** untuk menentukan kecepatan I2C optimal (100kHz vs 400kHz) untuk masing-masing sensor. Jika terjadi error komunikasi (sensor disconnect, NACK), sistem memiliki **error recovery** yang mencoba re-initialize sensor tanpa me-reset seluruh sistem.

**STM32F103C8T6 (Display & Logger Node)** berfungsi sebagai unit display cadangan dan logger mandiri. STM32 terhubung ke bus I2C yang sama dan membaca sensor menggunakan **raw I2C read/write** tanpa library tingkat tinggi — ini bertujuan agar mahasiswa memahami protokol I2C di level register. STM32 menampilkan data pada LCD terpisah dan berkomunikasi dengan ESP32 via UART untuk sinkronisasi data.

---

## Spesifikasi Teknis

### Mapping Percobaan ke Fitur

| No | Percobaan | Fitur dalam Project |
|----|-----------|---------------------|
| P01 | I2C Scanner | Startup: scan semua device, tampilkan address map |
| P02 | OLED SSD1306 | Display utama data cuaca dengan ikon dan grafik |
| P03 | BMP280 | Pembacaan suhu dan tekanan atmosfer |
| P04 | MPU6050 | Deteksi guncangan/orientasi (proxy arah angin) |
| P05 | EEPROM AT24C32 | Data logging circular 1000 record |
| P06 | RTC DS3231 | Timestamp akurat untuk setiap record |
| P07 | BH1750 | Pengukuran intensitas cahaya (lux) |
| P08 | LCD PCF8574 | Display data jarak jauh untuk petani |
| P09 | Multi-Sensor Read | Baca semua sensor dalam satu siklus efisien |
| P10 | Raw I2C Read/Write | STM32 akses sensor di level register |
| P11 | Clock Speed Test | Optimasi kecepatan I2C per sensor |
| P12 | Error Recovery | Auto re-initialize sensor jika disconnect |

### Arsitektur I2C Bus

```
                    3.3V
                     │
                   [4.7kΩ] ×2 (pull-up SDA & SCL)
                     │
    ┌────────────────┼──── I2C Bus (SDA + SCL) ──────────────────┐
    │                │                                            │
┌───┴───┐  ┌────────┴────────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌┴─────┐
│ ESP32 │  │     STM32       │  │BMP280│  │MPU6050│ │BH1750│  │DS3231│
│Master │  │Master (alt time)│  │ 0x76 │  │ 0x68 │  │ 0x23 │  │ 0x68*│
└───┬───┘  └─────────────────┘  └──────┘  └──────┘  └──────┘  └──────┘
    │                                                
┌───┴────┐  ┌────────┐  ┌────────┐
│SSD1306 │  │AT24C32 │  │PCF8574 │
│ 0x3C   │  │ 0x57   │  │ 0x27   │
└────────┘  └────────┘  └────────┘
```

> *Note: MPU6050 dan DS3231 keduanya 0x68 — gunakan MPU6050 di address 0x69 (AD0=HIGH).

---

## Ketentuan Pengerjaan

1. Minimal 4 sensor I2C harus terhubung di bus yang sama
2. Data harus ditampilkan di OLED dan/atau LCD
3. Data logging ke EEPROM dengan timestamp RTC
4. I2C scanner harus berjalan saat startup
5. Error recovery harus bisa menangani sensor disconnect
6. STM32 harus mengakses minimal 1 sensor menggunakan raw I2C (tanpa library)

---

## Rubrik Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | Multi-sensor I2C berfungsi (BMP280 + BH1750 + MPU6050) | 20% |
| 2 | Display OLED/LCD menampilkan data real-time | 15% |
| 3 | Data logging EEPROM + timestamp RTC | 20% |
| 4 | I2C scanner + error recovery | 15% |
| 5 | Raw I2C pada STM32 | 10% |
| 6 | Clock speed optimization | 5% |
| 7 | Kode modular dan dokumentasi | 10% |
| 8 | Demo video | 5% |

---

## Referensi

1. NXP UM10204 — I2C-bus specification and user manual
2. STM32F103xx Reference Manual (RM0008) — Chapter 26: I2C
3. ESP-IDF Programming Guide — I2C Driver API
4. Datasheet: BMP280, SSD1306, MPU6050, DS3231, AT24C32, BH1750, PCF8574
