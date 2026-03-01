# Tugas Video — Modul 06: I2C Bus dan Sensor Integration

## Informasi Tugas

| Item | Detail |
|------|--------|
| Modul | 06 — I2C Sensor |
| Platform | STM32F103C8T6 & ESP32 DevKit V1 |
| Format | Video Presentasi + Demonstrasi Hardware |
| Durasi | 15–25 menit |
| Upload | YouTube (Unlisted) → link di e-learning |

---

## Deskripsi Tugas

Video laporan lengkap praktikum dan project Modul 06 — materi I2C, 12 percobaan, dan project weather station.

---

## Ketentuan Teknis Video

- **Screen recording** + **Webcam** (PiP) wajib
- **Hardware recording** menunjukkan sensor, OLED, LCD, koneksi I2C
- Resolusi minimal 720p, audio jelas

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
### 2. Ringkasan Materi (2–3 menit)
- Protokol I2C: SDA, SCL, start/stop condition, ACK/NACK
- Addressing 7-bit, read/write bit
- Pull-up resistor, clock stretching
- I2C pada ESP32 vs STM32
- Sensor populer: BMP280, OLED, MPU6050, RTC

### 3. Demonstrasi Percobaan (8–14 menit)

| No | Percobaan | Poin Penting Demo |
|----|-----------|-------------------|
| P01 | I2C Scanner | Tunjukkan address map semua device |
| P02 | OLED SSD1306 | Tampilkan teks/grafik di OLED |
| P03 | BMP280 | Baca suhu + tekanan, tiup sensor |
| P04 | MPU6050 | Gerakkan sensor → nilai berubah |
| P05 | EEPROM AT24C32 | Tulis lalu baca data, verifikasi |
| P06 | RTC DS3231 | Set waktu, baca waktu akurat |
| P07 | BH1750 | Tutup sensor cahaya → lux berubah |
| P08 | LCD PCF8574 | Tampilkan teks di LCD 16×2 |
| P09 | Multi-Sensor | Semua sensor dibaca bersamaan |
| P10 | Raw I2C | Akses register langsung tanpa library |
| P11 | Clock Speed Test | Bandingkan 100kHz vs 400kHz |
| P12 | Error Recovery | Cabut sensor → sistem recovery |

### 4. Demonstrasi Project (3–5 menit)
- Skenario pertanian cerdas
- Multi-sensor + display + data logging + RTC
- Error recovery demo

### 5. Penutup (1–2 menit)

---

## Penilaian & Penalti

| Komponen | Bobot |
|----------|-------|
| Pemahaman materi | 15% |
| Demonstrasi percobaan | 35% |
| Demonstrasi project | 20% |
| Demo hardware | 15% |
| Kualitas video | 15% |

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −20% |
| Tidak ada demo hardware | −20% |
| Durasi < 10 / > 30 menit | −10% / −5% |
| Terlambat | −10% per hari |

---

## Checklist

- [ ] YouTube Unlisted, 15–25 menit
- [ ] Webcam + audio jelas
- [ ] 12 percobaan (kedua platform)
- [ ] Project demo
- [ ] Hardware demo (sensor, OLED, LCD)
- [ ] Link di e-learning
