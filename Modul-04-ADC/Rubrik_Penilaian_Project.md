# 📊 Rubrik Penilaian Project — Modul 04: Menguak Dunia Analog — ADC

## 📋 Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 04 - ADC (Analog-to-Digital Converter) |
| **Project** | Sistem Monitoring Kualitas Udara Berbasis ADC Multi-Channel |
| **Bobot** | Sesuai kontrak perkuliahan |
| **Pengumpulan** | Source code + Laporan PDF + Video demonstrasi |

---

## 📏 Rubrik Penilaian Detail

### 1. Fungsionalitas Sistem (30%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Pembacaan ADC Multi-Channel** | Keempat sensor (MQ-135, LM35, LDR, potensiometer) terbaca dengan benar dan stabil secara simultan | 3 dari 4 sensor terbaca dengan benar | 2 dari 4 sensor terbaca dengan benar | Hanya 1 atau tidak ada sensor yang terbaca |
| **Konversi Nilai ADC** | Semua konversi ke satuan fisik (PPM, °C, Lux, %) benar dengan kalibrasi | Konversi benar untuk 3 sensor, atau tanpa kalibrasi | Konversi benar untuk 2 sensor | Konversi salah atau tidak dilakukan |
| **Tampilan Serial** | Format tampilan rapi, terstruktur, informatif, dan mudah dibaca sesuai spesifikasi | Format cukup baik tetapi kurang terstruktur | Menampilkan data tetapi format berantakan | Tidak menampilkan data atau error |
| **Sistem Threshold & Alert** | Threshold berfungsi untuk semua sensor, LED menyala/berkedip sesuai kondisi, pesan peringatan tepat | Threshold berfungsi untuk sebagian sensor | Threshold hanya berfungsi untuk 1 sensor | Tidak ada sistem threshold |
| **Python Data Logging** | Script Python berjalan, membaca serial, menyimpan CSV dengan timestamp, logging ≥5 menit | Script berjalan dan menyimpan CSV tetapi kurang dari 5 menit | Script berjalan tetapi CSV tidak lengkap | Script Python tidak ada atau error |

**Poin Bonus Fungsionalitas (+5%):**
- Visualisasi real-time matplotlib (+2%)
- Mode kalibrasi via serial command (+1.5%)
- Multi-mode tampilan (ringkas/detail/JSON) (+1.5%)

---

### 2. Kualitas Kode (25%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Struktur & Organisasi** | Kode modular dengan fungsi-fungsi terpisah (baca sensor, konversi, tampilan, alert), file terorganisir | Cukup modular, beberapa fungsi terpisah | Sebagian besar kode dalam satu blok | Kode berantakan tanpa struktur |
| **Komentar & Dokumentasi** | Setiap fungsi memiliki header comment, variabel penting diberi komentar, README lengkap | Komentar cukup tetapi tidak konsisten | Komentar minim, hanya di beberapa bagian | Tanpa komentar sama sekali |
| **Penamaan Variabel** | Nama variabel dan fungsi deskriptif, mengikuti konvensi (camelCase/snake_case) konsisten | Penamaan cukup deskriptif, sebagian besar konsisten | Beberapa nama kurang jelas | Nama variabel tidak deskriptif (a, b, x1) |
| **Error Handling** | Menangani error pembacaan ADC, validasi range, timeout handling, bounds checking | Beberapa error handling tersedia | Error handling minimal | Tidak ada error handling |
| **Efisiensi & Best Practice** | Menggunakan filtering (averaging/median/EMA), timing non-blocking (millis), DMA jika memungkinkan | Menggunakan filtering dan timing yang baik | Menggunakan salah satu teknik | Tidak ada optimasi, menggunakan delay() blocking |

---

### 3. Laporan Tertulis (20%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kelengkapan** | Semua bagian ada: pendahuluan, teori, perancangan, implementasi, pengujian, analisis, kesimpulan (≥10 hal) | Sebagian besar bagian ada (5-6 dari 7) | Beberapa bagian ada (3-4 dari 7) | Sangat tidak lengkap (<3 bagian) |
| **Skema Rangkaian** | Skema dibuat dengan Fritzing/KiCad, lengkap label komponen dan nilai, rapi dan profesional | Skema dibuat dengan software, cukup lengkap | Skema digambar tangan atau screenshot sederhana | Tidak ada skema rangkaian |
| **Analisis Data** | Tabel data pengukuran lengkap, grafik perbandingan, analisis error, pembahasan mendalam | Tabel dan grafik ada tetapi analisis kurang mendalam | Data ada tetapi tanpa analisis atau grafik | Tidak ada data pengukuran |
| **Perbandingan ESP32 vs STM32** | Analisis mendalam: linearitas, kecepatan, akurasi, noise, dibuktikan dengan data pengukuran | Perbandingan berdasarkan spesifikasi dan sedikit data | Perbandingan hanya berdasarkan spesifikasi tanpa data | Tidak ada perbandingan |
| **Tata Tulis & Format** | Bahasa baku, format konsisten, referensi lengkap dengan daftar pustaka, bebas typo | Format baik, sedikit inkonsistensi | Format kurang konsisten, beberapa typo | Format buruk, banyak kesalahan |

---

### 4. Presentasi / Video Demonstrasi (15%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kelengkapan Demo** | Menunjukkan semua fitur: hardware, pembacaan 4 sensor, threshold alert, Python logging | Menunjukkan sebagian besar fitur (3 dari 4 poin) | Menunjukkan sebagian fitur (2 poin) | Demo sangat minim atau tidak berjalan |
| **Penjelasan Teknis** | Menjelaskan cara kerja kode, konfigurasi ADC, konversi, dan filtering dengan jelas | Penjelasan cukup baik tetapi kurang detail | Penjelasan singkat tanpa detail teknis | Tidak ada penjelasan teknis |
| **Kualitas Produksi** | Video jelas (gambar & suara), durasi 5-10 menit, alur logis, well-edited | Kualitas cukup baik, durasi sesuai | Kualitas kurang (buram/berisik) atau durasi tidak sesuai | Kualitas sangat buruk atau durasi <2 menit |

---

### 5. Kreativitas & Inovasi (10%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Fitur Tambahan** | Mengimplementasikan ≥3 fitur bonus (visualisasi matplotlib, kalibrasi, multi-mode, LCD, aktuator) | Mengimplementasikan 2 fitur bonus | Mengimplementasikan 1 fitur bonus | Tidak ada fitur tambahan |
| **Desain Hardware** | Rangkaian rapi di breadboard/PCB, kabel terorganisir, label, housing/enclosure | Rangkaian cukup rapi | Rangkaian berfungsi tetapi berantakan | Rangkaian tidak rapi dan sulit di-debug |
| **Inovasi** | Menambahkan fitur unik di luar spesifikasi (dashboard web, database, machine learning, notifikasi) | Modifikasi kecil yang berguna | Implementasi standar sesuai spesifikasi | Implementasi di bawah standar minimum |

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
