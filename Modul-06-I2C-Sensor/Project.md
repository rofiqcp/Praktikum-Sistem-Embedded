# 🎯 Project Modul 06: Weather Station I2C Multi-Sensor & Data Logging

## 📋 Deskripsi Project

Mahasiswa diminta merancang dan mengimplementasikan **Weather Station berbasis I2C** yang mengintegrasikan beberapa sensor pada satu bus I2C, menampilkan data pada OLED display, dan mencatat data ke EEPROM dengan timestamp dari modul RTC. Sistem harus mampu menangani multi-device I2C, error recovery, dan menyediakan output serial terstruktur untuk analisis Python.

---

## 🎯 Tujuan Pembelajaran

| No | Tujuan | Bobot |
|----|--------|-------|
| 1 | Menguasai konfigurasi I2C master pada ESP32 (ESP-IDF) dan STM32 (HAL) | 20% |
| 2 | Mampu berkomunikasi dengan minimal 3 device I2C berbeda pada satu bus | 25% |
| 3 | Mengimplementasikan pembacaan sensor, display, dan data logging terintegrasi | 25% |
| 4 | Menerapkan error handling dan recovery pada bus I2C | 15% |
| 5 | Membuat Python tool untuk analisis dan visualisasi data weather station | 15% |

---

## 📐 Arsitektur Sistem

```
┌──────────────────────────────────────────────────────┐
│                  WEATHER STATION                      │
│                                                       │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌────────┐ │
│  │ BMP280  │  │ BH1750  │  │ DS3231  │  │AT24C32 │ │
│  │Temp+Pres│  │  Light  │  │  RTC    │  │ EEPROM │ │
│  │ 0x76/77 │  │  0x23   │  │  0x68   │  │  0x57  │ │
│  └────┬────┘  └────┬────┘  └────┬────┘  └───┬────┘ │
│       │             │            │            │       │
│  ─────┴─────────────┴────────────┴────────────┴───── │
│                 I2C Bus (SDA + SCL)                    │
│              Pull-up 4.7kΩ ke VCC 3.3V                │
│  ─────┬─────────────────────────────────────────┬─── │
│       │                                         │     │
│  ┌────┴────┐                              ┌─────┴───┐│
│  │SSD1306  │                              │MCU      ││
│  │128x64   │                              │ESP32/   ││
│  │ 0x3C    │                              │STM32    ││
│  └─────────┘                              └─────────┘│
└──────────────────────────────────────────────────────┘
```

---

## 🔧 Spesifikasi Hardware

### Komponen Utama

| No | Komponen | I2C Address | Fungsi | Qty |
|----|----------|-------------|--------|-----|
| 1 | ESP32 DevKit / STM32 BluePill | - | Mikrokontroler utama | 1 |
| 2 | BMP280 Module | 0x76 / 0x77 | Sensor suhu & tekanan | 1 |
| 3 | BH1750 Module (GY-302) | 0x23 / 0x5C | Sensor intensitas cahaya | 1 |
| 4 | DS3231 RTC Module | 0x68 | Real-Time Clock | 1 |
| 5 | AT24C32 EEPROM | 0x50-0x57 | Data logging storage (4KB) | 1* |
| 6 | SSD1306 OLED 128×64 | 0x3C / 0x3D | Display | 1 |
| 7 | Resistor 4.7kΩ | - | I2C pull-up | 2 |
| 8 | Breadboard + jumper wire | - | Koneksi | 1 set |

> \* AT24C32 biasanya sudah terintegrasi pada modul DS3231

### Koneksi Pin I2C

| Platform | SDA | SCL | Keterangan |
|----------|-----|-----|------------|
| **ESP32** | GPIO 21 | GPIO 22 | I2C0 default |
| **STM32 BluePill** | PB7 (I2C1_SDA) | PB6 (I2C1_SCL) | I2C1 default |
| **STM32 F401/F411** | PB7 atau PB9 | PB6 atau PB8 | AF4 I2C1 |

### Skema Koneksi

```
VCC (3.3V) ──┬──────────────────────────────┐
             │                              │
            [4.7kΩ]                       [4.7kΩ]
             │                              │
SDA ─────────┼──┬──┬──┬──┬──┬── MCU SDA    │
             │  │  │  │  │  │              │
SCL ─────────┼──┼──┼──┼──┼──┼── MCU SCL ───┘
                │  │  │  │  │
              BMP BH DS AT SSD
              280 17 32 24 13
                  50 31 C  06
                     32

GND ────────────────────────────── Common GND
```

---

## 📋 Fitur yang Harus Diimplementasikan

### Level Dasar (Wajib — 70%)

| No | Fitur | Deskripsi |
|----|-------|-----------|
| 1 | **I2C Bus Scan** | Deteksi semua device di bus, tampilkan address |
| 2 | **Baca BMP280** | Suhu (°C) dan tekanan atmosfer (hPa) |
| 3 | **Baca BH1750** | Intensitas cahaya (lux) |
| 4 | **Baca DS3231** | Waktu (HH:MM:SS) dan tanggal (DD/MM/YYYY) |
| 5 | **Display OLED** | Tampilkan semua data sensor di SSD1306 |
| 6 | **Serial Output** | Format CSV terstruktur untuk Python parsing |

### Level Menengah (Nilai Tambah — 20%)

| No | Fitur | Deskripsi |
|----|-------|-----------|
| 7 | **Data Logging** | Simpan data ke AT24C32 EEPROM per interval |
| 8 | **Configurable Rate** | Sampling rate bisa diubah via serial command |
| 9 | **Error Handling** | Deteksi sensor disconnect, timeout, NACK |
| 10 | **Python Dashboard** | Realtime plot suhu, tekanan, cahaya |

### Level Lanjut (Bonus — 10%)

| No | Fitur | Deskripsi |
|----|-------|-----------|
| 11 | **Alarm System** | Alarm jika suhu/tekanan di luar threshold |
| 12 | **EEPROM Dump** | Baca semua data tersimpan, export ke CSV |
| 13 | **Watchdog I2C** | Recovery otomatis jika bus hang |

---

## 📊 Format Output Serial

```
# Weather Station Data Output
# Format: TIMESTAMP,TEMP_C,PRESSURE_HPA,LUX,STATUS
# Status: OK | SENSOR_ERR | BUS_ERR

[WS] I2C Bus Scan Complete: 5 devices found
[WS] Sensors initialized successfully

DATA,2024-01-15 10:30:00,25.4,1013.25,450.5,OK
DATA,2024-01-15 10:30:05,25.5,1013.20,448.2,OK
DATA,2024-01-15 10:30:10,25.4,1013.22,452.1,OK
LOG,Saved record #142 to EEPROM at addr 0x0470
WARN,BH1750 read timeout - retrying
DATA,2024-01-15 10:30:15,25.6,1013.18,0.0,SENSOR_ERR
```

---

## 🐍 Python Analysis Tool

### Fitur Minimum

```python
# weather_station_analyzer.py
# 1. Serial parser - baca data dari COM port
# 2. Realtime plot - matplotlib dengan 3 subplot (temp, pressure, light)
# 3. CSV export - simpan semua data ke file
# 4. Statistics - min, max, avg, stddev per parameter
# 5. Alert log - catat semua warning/error dari device
```

### Contoh Penggunaan

```bash
# Monitoring realtime
python weather_station_analyzer.py --port /dev/ttyUSB0 --baud 115200 --plot

# Export data (5 menit)
python weather_station_analyzer.py --port /dev/ttyUSB0 --duration 300 --export data.csv

# Analisis file yang sudah disimpan
python weather_station_analyzer.py --analyze data.csv --stats --plot
```

---

## 📅 Timeline Pengerjaan

| Minggu | Aktivitas | Deliverable |
|--------|-----------|-------------|
| 1 | Setup hardware, I2C scan, baca 1 sensor | Foto wiring + serial log |
| 2 | Integrasi semua sensor + OLED display | Video demo pembacaan |
| 3 | Data logging + error handling + Python tool | Source code + laporan |
| 4 | Presentasi + demo live | Video presentasi |

---

## 📝 Deliverables

### 1. Source Code (40%)
- [ ] Project PlatformIO lengkap (compilable, `pio run` sukses)
- [ ] Kode terstruktur dengan komentar yang jelas
- [ ] Implementasi error handling
- [ ] Python analysis tool berfungsi

### 2. Laporan Teknis (30%)
- [ ] Deskripsi arsitektur sistem
- [ ] Analisis protokol I2C (timing, addressing, data frame)
- [ ] Skematik koneksi hardware
- [ ] Screenshot/log hasil pengujian
- [ ] Analisis data dari Python tool

### 3. Video Demo (20%)
- [ ] Demo live weather station berjalan
- [ ] Penjelasan kode dan arsitektur
- [ ] Demo error recovery (cabut sensor, pasang kembali)
- [ ] Demo Python dashboard

### 4. Presentasi (10%)
- [ ] Slide presentasi (max 15 slide)
- [ ] Q&A kemampuan menjawab pertanyaan

---

## ⚠️ Catatan Platform

### ESP32 (ESP-IDF)
- Gunakan `driver/i2c.h` (legacy) atau `esp_idf_i2c` (new driver)
- I2C0: SDA=GPIO21, SCL=GPIO22
- Maximum clock: 400 kHz (Fast Mode)
- ESP32-S2/S3: I2C pins bisa di-remap ke GPIO manapun

### STM32 (HAL)
- Gunakan `HAL_I2C_Mem_Read()` / `HAL_I2C_Mem_Write()` untuk sensor registers
- STM32F103: I2C1 (PB6/PB7), I2C2 (PB10/PB11)
- STM32F401/F411: I2C1 (PB6/PB7 AF4), I2C2, I2C3 tersedia
- Perhatikan: Address di HAL di-shift left 1 bit (7-bit → 8-bit)

### Pull-up Resistor
- **WAJIB** 4.7kΩ pull-up pada SDA dan SCL ke VCC 3.3V
- Tanpa pull-up, bus I2C TIDAK akan berfungsi
- Beberapa modul breakout sudah memiliki pull-up on-board

---

## 📏 Kriteria Penilaian Singkat

| Aspek | Bobot | Kriteria A (85-100) |
|-------|-------|---------------------|
| Fungsionalitas | 30% | Semua sensor terbaca, display & logging OK |
| Kode | 25% | Bersih, modular, error handling lengkap |
| Laporan | 20% | Analisis mendalam, data Python divisualisasi |
| Video & Demo | 15% | Profesional, demo error recovery |
| Presentasi | 10% | Menguasai materi, jawab pertanyaan baik |

---

*Modul 06 — Praktikum Sistem Embedded*
*Weather Station I2C Multi-Sensor Project*
