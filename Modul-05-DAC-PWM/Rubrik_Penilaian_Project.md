# 📊 Rubrik Penilaian Project - Modul 05: DAC & PWM (Digital-to-Analog Converter & Pulse Width Modulation)

## 📋 Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 05 - DAC & PWM (Digital-to-Analog Converter & Pulse Width Modulation) |
| **Project** | Sistem Audio Player dan LED Controller Berbasis DAC & PWM |
| **Bobot** | Sesuai kontrak perkuliahan |
| **Pengumpulan** | Source code + Laporan PDF + Video demonstrasi |

---

## 📏 Rubrik Penilaian Detail

### 1. Fungsionalitas Sistem (30%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Generator Gelombang DAC** | Menghasilkan gelombang sinus dan segitiga dengan benar, frekuensi dapat diatur via potensiometer, output ke speaker terdengar jelas | Gelombang sinus atau segitiga saja yang benar, frekuensi bisa diatur | Menghasilkan output DAC tetapi bentuk gelombang tidak sesuai | Tidak ada output DAC atau output salah total |
| **Kontrol LED RGB (PWM)** | Semua efek berfungsi (fade, rainbow, breathing), kecerahan diatur potensiometer, transisi warna halus | 2 dari 3 efek berfungsi dengan baik | Hanya 1 efek berfungsi atau LED tidak menyala sesuai warna | LED RGB tidak berfungsi atau tidak ada kontrol PWM |
| **Kontrol Servo Motor** | Servo merespons potensiometer dengan halus (0°-180°), mode sweep otomatis berjalan, tidak ada jitter | Servo merespons potensiometer tetapi ada sedikit jitter atau sweep kurang halus | Servo bergerak tetapi tidak proporsional atau range terbatas | Servo tidak bergerak atau gerakan tidak terkontrol |
| **Melody Player (Buzzer)** | Minimal 2 melodi terputar dengan benar, tempo dapat diatur, nada jelas dan sesuai | 1 melodi terputar dengan benar, nada sebagian besar sesuai | Buzzer berbunyi tetapi nada tidak sesuai melodi | Buzzer tidak berbunyi atau hanya bunyi monoton |
| **Antarmuka Serial (Menu)** | Menu interaktif lengkap, semua mode dapat dipilih, sub-menu parameter, feedback status jelas | Menu ada dan berfungsi untuk sebagian besar mode | Menu minimal, hanya beberapa mode dapat dipilih | Tidak ada menu serial atau menu tidak berfungsi |

**Poin Bonus Fungsionalitas (+5%):**
- Audio mixer / AM modulation (+2%)
- Visualisasi Serial Plotter (+1%)
- Pattern LED kustom via serial (+1%)
- Python GUI controller (+1%)

---

### 2. Kualitas Kode (25%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Struktur & Organisasi** | Kode modular: fungsi terpisah untuk DAC, PWM, servo, melodi, menu. Lookup table di header file terpisah | Cukup modular, beberapa fungsi terpisah | Sebagian besar kode dalam satu blok loop() | Kode berantakan tanpa struktur |
| **Komentar & Dokumentasi** | Setiap fungsi memiliki header comment, perhitungan frekuensi dijelaskan, lookup table didokumentasi, README lengkap | Komentar cukup tetapi tidak konsisten | Komentar minim, hanya di beberapa bagian | Tanpa komentar sama sekali |
| **Penamaan Variabel** | Nama variabel dan fungsi deskriptif (contoh: `servoAngle`, `sineWaveLookup`, `playMelody`), konsisten | Penamaan cukup deskriptif, sebagian besar konsisten | Beberapa nama kurang jelas | Nama variabel tidak deskriptif (a, b, x1) |
| **Error Handling** | Validasi input menu, bounds checking sudut servo (0-180), validasi duty cycle (0-100%), parameter range check | Beberapa error handling tersedia | Error handling minimal | Tidak ada error handling |
| **Efisiensi & Best Practice** | Menggunakan lookup table untuk sine wave, timing non-blocking (millis), interrupt-based tone, LEDC hardware fade | Menggunakan lookup table dan timing yang baik | Menggunakan salah satu teknik optimasi | Tidak ada optimasi, delay blocking di mana-mana |

---

### 3. Laporan Tertulis (20%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kelengkapan** | Semua bagian ada: pendahuluan, teori DAC & PWM, perancangan, implementasi, pengujian, analisis, kesimpulan (≥10 hal) | Sebagian besar bagian ada (5-6 dari 7) | Beberapa bagian ada (3-4 dari 7) | Sangat tidak lengkap (<3 bagian) |
| **Skema Rangkaian** | Skema dibuat dengan Fritzing/KiCad, lengkap label komponen (speaker, LED RGB, servo, buzzer, pot), rapi dan profesional | Skema dibuat dengan software, cukup lengkap | Skema digambar tangan atau screenshot sederhana | Tidak ada skema rangkaian |
| **Analisis Data** | Tabel data pengukuran (frekuensi output, duty cycle, sudut servo), screenshot osiloskop jika ada, analisis akurasi | Tabel dan screenshot ada tetapi analisis kurang mendalam | Data ada tetapi tanpa analisis | Tidak ada data pengukuran |
| **Perbandingan DAC vs PWM** | Analisis mendalam: kualitas output, ripple, bandwidth, dibuktikan dengan data/screenshot; perbandingan ESP32 vs STM32 | Perbandingan berdasarkan spesifikasi dan sedikit data | Perbandingan hanya berdasarkan spesifikasi tanpa data | Tidak ada perbandingan |
| **Tata Tulis & Format** | Bahasa baku, format konsisten, referensi lengkap dengan daftar pustaka, bebas typo | Format baik, sedikit inkonsistensi | Format kurang konsisten, beberapa typo | Format buruk, banyak kesalahan |

---

### 4. Presentasi / Video Demonstrasi (15%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kelengkapan Demo** | Menunjukkan semua fitur: DAC waveform (speaker), LED RGB effects, servo control, melody player, menu serial | Menunjukkan sebagian besar fitur (3-4 dari 5) | Menunjukkan sebagian fitur (2 poin) | Demo sangat minim atau tidak berjalan |
| **Penjelasan Teknis** | Menjelaskan konfigurasi LEDC/Timer, perhitungan frekuensi PWM, lookup table, pulse width servo dengan jelas | Penjelasan cukup baik tetapi kurang detail | Penjelasan singkat tanpa detail teknis | Tidak ada penjelasan teknis |
| **Kualitas Produksi** | Video jelas (gambar & suara), audio output terdengar, LED terlihat, durasi 5-10 menit, alur logis | Kualitas cukup baik, durasi sesuai | Kualitas kurang (buram/berisik) atau durasi tidak sesuai | Kualitas sangat buruk atau durasi <2 menit |

---

### 5. Kreativitas & Inovasi (10%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Fitur Tambahan** | Mengimplementasikan ≥3 fitur bonus (audio mixer, serial plotter, pattern kustom, Python GUI, LCD) | Mengimplementasikan 2 fitur bonus | Mengimplementasikan 1 fitur bonus | Tidak ada fitur tambahan |
| **Desain Hardware** | Rangkaian rapi di breadboard/PCB, kabel terorganisir, speaker/buzzer terpasang baik, label, housing | Rangkaian cukup rapi | Rangkaian berfungsi tetapi berantakan | Rangkaian tidak rapi dan sulit di-debug |
| **Inovasi** | Menambahkan fitur unik di luar spesifikasi (equalizer, sound effect, light show sinkron audio, MIDI input) | Modifikasi kecil yang berguna | Implementasi standar sesuai spesifikasi | Implementasi di bawah standar minimum |

---

## 📊 Ringkasan Bobot Penilaian

| No | Komponen | Bobot | Nilai Maks |
|----|----------|:-----:|:----------:|
| 1 | Fungsionalitas Sistem | 30% | 100 |
| 2 | Kualitas Kode | 25% | 100 |
| 3 | Laporan Tertulis | 20% | 100 |
| 4 | Presentasi / Video | 15% | 100 |
| 5 | Kreativitas & Inovasi | 10% | 100 |
| | **Total** | **100%** | |

## 📐 Rumus Perhitungan Nilai Akhir

$$\text{Nilai Akhir} = (0.30 \times N_1) + (0.25 \times N_2) + (0.20 \times N_3) + (0.15 \times N_4) + (0.10 \times N_5)$$

Dimana:
- $N_1$ = Nilai Fungsionalitas (0-100)
- $N_2$ = Nilai Kualitas Kode (0-100)
- $N_3$ = Nilai Laporan (0-100)
- $N_4$ = Nilai Presentasi/Video (0-100)
- $N_5$ = Nilai Kreativitas (0-100)

## 📝 Konversi Nilai Huruf

| Range Nilai | Huruf | Keterangan |
|:-----------:|:-----:|------------|
| 86 - 100 | A | Sangat Baik |
| 71 - 85 | B | Baik |
| 56 - 70 | C | Cukup |
| 41 - 55 | D | Kurang |
| 0 - 40 | E | Sangat Kurang |

## ⚠️ Ketentuan Khusus

1. **Plagiarisme**: Kode yang terbukti plagiat (copy-paste tanpa pemahaman) mendapat nilai **0** untuk komponen kode.
2. **Keterlambatan**: Pengurangan **10 poin per hari** keterlambatan (maksimal 3 hari, setelahnya nilai 0).
3. **Tidak Demo**: Jika tidak melakukan demo/presentasi, komponen presentasi mendapat nilai **0**.
4. **Kerja Individu**: Project dikerjakan individu kecuali dinyatakan lain. Diskusi boleh, kode harus mandiri.
5. **Bonus Maksimal**: Total bonus tidak melebihi **+10%** dari nilai akhir.
