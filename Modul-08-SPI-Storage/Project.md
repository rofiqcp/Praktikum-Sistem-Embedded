# Project Modul 07: Universal Data Logger System — SPI & Storage

## Informasi Project

| Item | Keterangan |
|------|------------|
| Modul | 07 — SPI Bus dan Storage |
| Platform | STM32F103C8T6 + ESP32 DevKit V1 |
| Durasi | 2 minggu |
| Tipe | Dual-MCU data acquisition dan persistent storage |

---

## Deskripsi Umum

Project ini mengintegrasikan seluruh 12 percobaan SPI dan storage — SPI loopback, OLED display, flash memory W25Q32, SD card, ADC/DAC eksternal, multi-slave, NVS, SPIFFS, speed benchmark, interrupt mode, dan data logger — ke dalam satu sistem data acquisition terintegrasi.

---

## Soal Cerita

### Skenario: Sistem Data Logger Portable untuk Ekspedisi Gunung Berapi

Tim vulkanologi Universitas Brawijaya mendapat hibah penelitian untuk memantau aktivitas Gunung Semeru. Mereka membutuhkan data logger portable yang bisa dipasang di pos pemantauan tanpa akses listrik stabil (baterai + solar panel). Data harus tersimpan aman meskipun perangkat restart karena petir atau gangguan power.

**ESP32 DevKit V1 (Data Hub)** berfungsi sebagai unit utama yang membaca sensor via **SPI MCP3208 ADC** (8 channel: suhu tanah di 4 kedalaman, vibration, kelembaban, tekanan gas, sensor dummy). Data diproses dan ditampilkan pada **SPI OLED SSD1306** yang menunjukkan status terkini dan grafik mini trend suhu.

Data disimpan dalam 3 tingkat redundansi:
1. **NVS** — menyimpan konfigurasi sistem (sampling rate, threshold, calibration) yang persistent
2. **SPIFFS** — menyimpan log data CSV untuk 7 hari terakhir secara internal
3. **SD Card FAT** — menyimpan arsip data jangka panjang untuk diambil oleh tim saat kunjungan berkala

ESP32 juga menghasilkan output analog melalui **SPI DAC MCP4921** untuk drive alarm buzzer proporsional (makin kuat getaran vulkanik, makin keras bunyi). Sistem melakukan **SPI speed benchmark** otomatis saat startup untuk menentukan kecepatan optimal masing-masing slave device.

**STM32F103C8T6 (Backup Logger)** berfungsi sebagai unit cadangan dan display lokal. STM32 membaca sensor yang sama menggunakan SPI bus shared (**multi-slave** dengan CS terpisah) dan menyimpan data ke **internal flash** menggunakan circular page management. STM32 juga melakukan **interrupt-based SPI** transfer agar CPU bisa tetap memproses input tombol dari petugas pos tanpa blocking.

Data dari STM32 juga disimpan ke **W25Q32 flash** sebagai backup tambahan — jika SD card penuh atau rusak, data tetap ada di flash eksternal. Kedua MCU berkomunikasi via UART untuk sinkronisasi waktu dan cross-validation data (jika ada perbedaan pembacaan sensor > 5%, log warning).

---

## Spesifikasi Teknis

### Mapping Percobaan ke Fitur

| No | Percobaan | Fitur dalam Project |
|----|-----------|---------------------|
| P01 | SPI Loopback | Startup self-test SPI bus sebelum inisialisasi slave |
| P02 | SPI OLED | Display real-time: status, data terkini, grafik mini trend |
| P03 | Flash W25Q32 | Backup storage di STM32 (redundansi level 3) |
| P04 | SD Card FAT | Arsip data jangka panjang di ESP32 (file CSV per hari) |
| P05 | MCP3208 ADC | Pembacaan 8 channel sensor vulkanologi |
| P06 | MCP4921 DAC | Output alarm proporsional berdasarkan intensitas getaran |
| P07 | Multi-Slave | STM32 shared bus dengan flash + ADC (CS switching) |
| P08 | NVS Key-Value | Konfigurasi persistent: sampling rate, threshold, calibration |
| P09 | SPIFFS | Log data 7 hari terakhir di internal flash ESP32 |
| P10 | Speed Benchmark | Auto-detect kecepatan SPI optimal per slave saat startup |
| P11 | Interrupt Mode | STM32 non-blocking SPI transfer untuk responsif UI |
| P12 | Data Logger | Arsitektur logging: ring buffer → storage multi-tier |

### Arsitektur Storage Multi-Tier

```
    Sensor Data
        │
        ▼
    ┌───────────┐
    │ RAM Ring  │ ◄── Buffer sementara (cepat, volatile)
    │ Buffer    │
    └─────┬─────┘
          │
    ┌─────┼──────────────────┐
    │     │                  │
    ▼     ▼                  ▼
┌───────┐ ┌────────┐  ┌──────────┐
│  NVS  │ │ SPIFFS │  │ SD Card  │
│Config │ │7-day   │  │ Archive  │
│       │ │  CSV   │  │CSV+Binary│
└───────┘ └────────┘  └──────────┘
(ESP32)    (ESP32)     (ESP32)

    ┌──────────┐  ┌────────────┐
    │Int. Flash│  │ W25Q32     │
    │Circular  │  │ Backup     │
    │Logger    │  │ Storage    │
    └──────────┘  └────────────┘
     (STM32)       (STM32)
```

---

## Ketentuan Pengerjaan

1. ESP32 harus membaca minimal 4 channel dari MCP3208
2. Data harus tersimpan di minimal 2 media storage berbeda
3. OLED harus menampilkan data real-time yang updated
4. NVS/flash config harus persist setelah restart (demo: ubah config → restart → config tetap)
5. SPI loopback self-test harus berjalan saat startup
6. Kedua MCU harus berkomunikasi via UART untuk sinkronisasi

---

## Rubrik Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | SPI sensor reading (MCP3208) + OLED display | 20% |
| 2 | Storage multi-tier (NVS + SPIFFS + SD Card) | 25% |
| 3 | Backup storage (W25Q32 + internal flash STM32) | 15% |
| 4 | Multi-slave SPI + speed benchmark | 10% |
| 5 | DAC alarm output + interrupt mode | 10% |
| 6 | SPI self-test + dual-MCU sinkronisasi | 10% |
| 7 | Kode modular dan dokumentasi | 10% |

---

## Referensi

1. STM32F103xx Reference Manual (RM0008) — SPI, Flash Programming
2. ESP-IDF Programming Guide — SPI Master, NVS, SPIFFS, SD/MMC
3. W25Q32 Datasheet, MCP3208 Datasheet, MCP4921 Datasheet
4. SD Specifications Part 1 — Physical Layer Simplified Specification
