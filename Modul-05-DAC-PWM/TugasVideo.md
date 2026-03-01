# Tugas Video — Modul 05: DAC dan PWM Output

## Informasi Tugas

| Item | Detail |
|------|--------|
| Modul | 05 — DAC & PWM |
| Platform | STM32F103C8T6 & ESP32 DevKit V1 |
| Format | Video Presentasi + Demonstrasi Hardware |
| Durasi | 15–25 menit |
| Upload | YouTube (Unlisted) → link di e-learning |

---

## Deskripsi Tugas

Video laporan lengkap praktikum dan project Modul 05 — mencakup materi DAC/PWM, 12 percobaan pada kedua platform, dan project akhir.

---

## Ketentuan Teknis Video

- **Screen recording** + **Webcam** (picture-in-picture) wajib
- **Hardware recording** menunjukkan LED, servo, motor, speaker/buzzer
- Resolusi minimal 720p, audio jelas

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
### 2. Ringkasan Materi (2–3 menit)
- Teori DAC: R-2R, resolusi, output range
- PWM: duty cycle, frekuensi, filter RC
- DAC vs PWM: kapan pakai yang mana
- ESP32: DAC 8-bit (GPIO25/26), LEDC PWM
- STM32: Tidak ada DAC, Timer PWM (capture/compare)
- Aplikasi: audio, servo, motor, LED dimming

### 3. Demonstrasi Percobaan (8–14 menit)

| No | Percobaan | Poin Penting Demo |
|----|-----------|-------------------|
| P01 | DAC Voltage Output | Ukur tegangan DAC dengan multimeter |
| P02 | Sine Wave | Waveform di osiloskop/serial plotter |
| P03 | Triangle Wave | Bandingkan bentuk gelombang dengan sine |
| P04 | Audio Tone | Dengarkan nada, ubah frekuensi |
| P05 | LED Breathing | Efek breathing smooth |
| P06 | LED Brightness | Putar potensio → brightness berubah |
| P07 | Servo Control | Servo bergerak 0°–180° |
| P08 | Frequency Sweep | Suara naik dari rendah ke tinggi |
| P09 | Motor Speed | Motor berputar cepat/lambat |
| P10 | RGB LED | Warna berubah smooth |
| P11 | Buzzer Melody | Mainkan melodi |
| P12 | DAC vs PWM Compare | Bandingkan output keduanya |

### 4. Demonstrasi Project (3–5 menit)
- Skenario Museum Sains
- Mode ambient, interaktif, sweep, melody
- Sinkronisasi ESP32 + STM32

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
| Durasi < 10 / > 30 menit | −10% / −5% |
| Terlambat submit | −10% per hari |

---

## Checklist

- [ ] YouTube Unlisted, 15–25 menit
- [ ] Webcam + audio jelas
- [ ] 12 percobaan (kedua platform)
- [ ] Project demo lengkap
- [ ] Hardware demo (servo, motor, LED, speaker)
- [ ] Link di e-learning
