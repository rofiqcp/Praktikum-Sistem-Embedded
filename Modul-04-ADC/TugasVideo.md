# Tugas Video — Modul 04: ADC (Analog-to-Digital Converter)

## Informasi Tugas

| Item | Detail |
|------|--------|
| Modul | 04 — ADC |
| Platform | STM32F103C8T6 & ESP32 DevKit V1 |
| Format | Video Presentasi + Demonstrasi Hardware |
| Durasi | 15–25 menit |
| Upload | YouTube (Unlisted) → link di e-learning |

---

## Deskripsi Tugas

Video ini merupakan **laporan lengkap** praktikum dan project Modul 04 — mencakup penjelasan materi ADC, demonstrasi seluruh 12 percobaan pada kedua platform, dan demonstrasi project akhir.

---

## Ketentuan Teknis Video

- **Screen recording** layar VS Code / Serial Monitor saat menjelaskan kode dan hasil
- **Webcam** wajib terlihat (picture-in-picture) selama presentasi
- **Hardware recording** menunjukkan rangkaian fisik (potensiometer, LDR, LED, multimeter)
- Resolusi minimal 720p, audio jelas

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan dan gambaran umum modul ADC

### 2. Ringkasan Materi (2–3 menit)
- Prinsip ADC: sampling, kuantisasi, encoding
- Resolusi 12-bit, referensi tegangan, rumus konversi
- ADC ESP32: atenuasi, kalibrasi eFuse, ADC1 vs ADC2
- ADC STM32: channel, scan mode, analog watchdog
- Teknik filter dan DMA

### 3. Demonstrasi Seluruh Percobaan (8–14 menit)

| No | Percobaan | Poin Penting Demo |
|----|-----------|-------------------|
| P01 | ADC Single Read | Putar potensio → nilai raw berubah |
| P02 | Voltage Display | Raw → tegangan, verifikasi dengan multimeter |
| P03 | Moving Average | Bandingkan raw vs filtered, stabilitas |
| P04 | Multi Channel | 4 channel simultan, tabel di serial |
| P05 | ADC Calibration | Sebelum vs sesudah kalibrasi |
| P06 | Continuous DMA | Sampling rate tinggi, buffer data |
| P07 | Threshold Alert | Putar potensio melewati threshold → LED menyala |
| P08 | Battery Monitor | Voltage divider, level baterai |
| P09 | Temperature Internal | Suhu chip berubah saat beban tinggi |
| P10 | Sampling Rate Test | Berapa kHz tercapai |
| P11 | Light Sensor (LDR) | Tutup/buka cahaya → nilai berubah |
| P12 | Statistical Analysis | Min, max, avg, stddev ditampilkan |

### 4. Demonstrasi Project (3–5 menit)
- Skenario monitoring gedung perkantoran
- Demo multi-channel + filter + threshold alert + mode switching
- Statistik real-time

### 5. Penutup (1–2 menit)

---

## Komponen Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | Pemahaman materi | 15% |
| 2 | Demonstrasi percobaan | 35% |
| 3 | Demonstrasi project | 20% |
| 4 | Demo hardware | 15% |
| 5 | Kualitas video | 15% |

---

## Penalti

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −20% |
| Tidak ada demo hardware | −20% |
| Durasi < 10 menit | −10% |
| Durasi > 30 menit | −5% |
| Terlambat submit | −10% per hari |

---

## Checklist

- [ ] Video YouTube Unlisted, 15–25 menit
- [ ] Webcam terlihat, audio jelas
- [ ] 12 percobaan didemonstrasikan (kedua platform)
- [ ] Project didemonstrasikan
- [ ] Hardware demo (potensio, LDR, LED, multimeter)
- [ ] Link dikumpulkan di e-learning
