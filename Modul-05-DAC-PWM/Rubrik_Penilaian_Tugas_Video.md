# 🎥 Rubrik Penilaian Tugas Video - Modul 05: DAC & PWM (Digital-to-Analog Converter & Pulse Width Modulation)

## 📋 Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 05 - DAC & PWM (Digital-to-Analog Converter & Pulse Width Modulation) |
| **Jenis Tugas** | Video Demonstrasi & Penjelasan Praktikum DAC & PWM |
| **Durasi** | 5-10 menit |
| **Format** | MP4 / MKV (resolusi minimal 720p) |
| **Pengumpulan** | Upload ke Google Drive / YouTube (unlisted) + link di laporan |

---

## 🎯 Tujuan Tugas Video

Mahasiswa membuat video yang mendemonstrasikan pemahaman konsep DAC dan PWM serta kemampuan implementasi pada mikrokontroler (ESP32/STM32), meliputi penjelasan teori, demo hardware (speaker, LED RGB, servo, buzzer), dan analisis hasil.

---

## 📏 Rubrik Penilaian Detail

### 1. Konten Teknis (35%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Penjelasan Konsep DAC** | Menjelaskan prinsip DAC (R-2R, resolusi 8-bit), perbedaan DAC murni vs PWM+filter, lookup table sine wave dengan benar dan mendalam | Menjelaskan konsep dasar DAC dengan benar tetapi kurang mendalam | Penjelasan DAC kurang tepat atau sangat dangkal | Tidak menjelaskan konsep DAC atau penjelasan salah |
| **Penjelasan Konsep PWM** | Menjelaskan duty cycle, frekuensi, resolusi, perhitungan parameter LEDC/Timer, relasi PWM-servo/LED/buzzer dengan detail | Menjelaskan sebagian besar konsep PWM dengan benar | Penjelasan PWM kurang tepat atau sangat dangkal | Tidak menjelaskan konsep PWM atau penjelasan salah |
| **Penjelasan Kode** | Menjelaskan alur program, konfigurasi LEDC/Timer, lookup table, perhitungan frekuensi, logika menu serial | Menjelaskan sebagian besar kode dengan cukup jelas | Penjelasan kode sangat singkat | Tidak menjelaskan kode program |
| **Konfigurasi Hardware** | Menjelaskan koneksi speaker+kapasitor, LED RGB+resistor, servo+power, buzzer, potensiometer dengan detail pin | Menjelaskan koneksi sebagian komponen | Menyebutkan komponen tetapi tidak detail | Tidak membahas koneksi hardware |
| **Analisis Hasil** | Menganalisis kualitas output: bentuk gelombang, kehalusan warna LED, akurasi sudut servo, ketepatan nada melodi | Melakukan analisis sederhana terhadap hasil | Menyebutkan hasil tanpa analisis | Tidak ada analisis hasil |

---

### 2. Demonstrasi Praktik (30%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Demo Generator Gelombang** | Mendemonstrasikan output DAC ke speaker: sine wave dan triangle wave terdengar jelas, frekuensi diatur potensiometer real-time | Demo output DAC terdengar tetapi hanya 1 jenis gelombang | Output DAC terdengar tetapi tidak jelas atau tanpa kontrol frekuensi | Demo DAC gagal atau tidak dilakukan |
| **Demo LED RGB** | Mendemonstrasikan semua efek (fade, rainbow, breathing) dengan transisi warna yang terlihat jelas, kecerahan diatur potensiometer | Demo 2 efek LED RGB | Demo 1 efek LED saja | Demo LED RGB gagal atau tidak dilakukan |
| **Demo Servo Motor** | Mendemonstrasikan kontrol servo via potensiometer (0°-180°), mode sweep otomatis, gerakan halus tanpa jitter | Servo merespons potensiometer tetapi kurang halus | Servo bergerak tetapi tanpa demo kontrol potensiometer | Demo servo gagal atau tidak dilakukan |
| **Demo Melody & Menu** | Mendemonstrasikan minimal 2 melodi via buzzer, menu serial lengkap, pemilihan mode berjalan lancar | Demo 1 melodi dan menu sebagian berfungsi | Buzzer berbunyi monoton, menu minimal | Demo melody/menu gagal atau tidak dilakukan |

---

### 3. Kualitas Presentasi (20%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kejelasan Komunikasi** | Penjelasan jelas, sistematis, percaya diri, menggunakan istilah teknis (duty cycle, LEDC, ARR, CCR) dengan tepat, tempo bicara baik | Penjelasan cukup jelas dan sistematis | Penjelasan kurang jelas atau terlalu cepat/lambat | Penjelasan membingungkan atau tidak terstruktur |
| **Alur Presentasi** | Alur logis: Pendahuluan → Teori singkat DAC & PWM → Demo hardware → Demo setiap mode → Analisis → Kesimpulan | Alur cukup logis dengan sedikit lompatan | Alur kurang terstruktur | Tidak ada alur yang jelas |
| **Penggunaan Visual** | Menggunakan zoom pada komponen, highlight kode, anotasi, diagram timing PWM, atau slide pendukung | Beberapa visual pendukung digunakan | Visual minim, hanya tangkapan layar | Tidak ada visual pendukung |
| **Durasi & Pacing** | Durasi 5-10 menit, setiap bagian mendapat porsi waktu yang proporsional | Durasi sesuai tetapi distribusi kurang proporsional | Durasi terlalu singkat (<5 mnt) atau panjang (>12 mnt) | Durasi sangat tidak sesuai (<3 mnt atau >15 mnt) |

---

### 4. Kualitas Produksi Video (15%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kualitas Gambar** | Resolusi ≥720p, pencahayaan baik, LED RGB dan rangkaian terlihat jelas, Serial Monitor terbaca, fokus tajam | Resolusi cukup, sebagian besar terlihat jelas | Gambar agak buram tetapi masih bisa dilihat | Gambar sangat buram, tidak terlihat jelas |
| **Kualitas Audio** | Suara presenter jelas, output speaker/buzzer terdengar, noise latar minimal, volume konsisten | Suara cukup jelas, sedikit noise | Suara kurang jelas, noise cukup mengganggu | Suara tidak terdengar atau sangat bising |
| **Editing** | Video diedit dengan baik: transisi smooth, teks overlay, zoom ke detail penting (LED menyala, servo berputar), intro/outro | Editing cukup baik dengan beberapa elemen | Editing minimal, video mentah | Tidak ada editing, banyak bagian tidak relevan |

---

## 📊 Ringkasan Bobot Penilaian

| No | Komponen | Bobot | Deskripsi |
|----|----------|:-----:|-----------|
| 1 | Konten Teknis | 35% | Pemahaman DAC, PWM, kode, konfigurasi, analisis |
| 2 | Demonstrasi Praktik | 30% | Demo gelombang DAC, LED RGB, servo, melody, menu |
| 3 | Kualitas Presentasi | 20% | Komunikasi, alur, visual, durasi |
| 4 | Kualitas Produksi | 15% | Gambar, audio, editing |
| | **Total** | **100%** | |

## 📐 Rumus Perhitungan Nilai

$$\text{Nilai Video} = (0.35 \times N_1) + (0.30 \times N_2) + (0.20 \times N_3) + (0.15 \times N_4)$$

Dimana:
- $N_1$ = Nilai Konten Teknis (0-100)
- $N_2$ = Nilai Demonstrasi Praktik (0-100)
- $N_3$ = Nilai Kualitas Presentasi (0-100)
- $N_4$ = Nilai Kualitas Produksi (0-100)

## 📝 Konversi Nilai Huruf

| Range Nilai | Huruf | Keterangan |
|:-----------:|:-----:|------------|
| 86 - 100 | A | Sangat Baik |
| 71 - 85 | B | Baik |
| 56 - 70 | C | Cukup |
| 41 - 55 | D | Kurang |
| 0 - 40 | E | Sangat Kurang |

---

## 📝 Checklist Sebelum Submit

Pastikan video Anda memenuhi kriteria berikut:

- [ ] Durasi video antara 5-10 menit
- [ ] Resolusi minimal 720p, gambar dan suara jelas
- [ ] Wajah presenter terlihat minimal di pembukaan dan penutup
- [ ] Menjelaskan konsep dasar DAC (prinsip, resolusi, tipe R-2R)
- [ ] Menjelaskan konsep PWM (duty cycle, frekuensi, resolusi)
- [ ] Menunjukkan rangkaian hardware (speaker, LED RGB, servo, buzzer, potensiometer)
- [ ] Mendemonstrasikan generator gelombang (output audio ke speaker)
- [ ] Mendemonstrasikan efek LED RGB (fade/rainbow/breathing)
- [ ] Mendemonstrasikan kontrol servo via potensiometer
- [ ] Mendemonstrasikan melody player (buzzer)
- [ ] Mendemonstrasikan menu serial interaktif
- [ ] Menjelaskan bagian kode yang penting (LEDC config, lookup table, servo mapping)
- [ ] Ada pembukaan (identitas) dan penutup (kesimpulan)
- [ ] Video sudah di-upload dan link dapat diakses

## ⚠️ Ketentuan Khusus

1. **Identitas**: Video harus mencantumkan nama lengkap, NIM, dan kelas di awal video.
2. **Orisinalitas**: Video harus dibuat sendiri. Video yang terbukti plagiat mendapat nilai **0**.
3. **Deadline**: Pengumpulan link video sesuai jadwal. Keterlambatan dikenakan pengurangan **10 poin/hari**.
4. **Aksesibilitas**: Pastikan link video dapat diakses oleh dosen (permission sharing benar).
5. **Bahasa**: Video menggunakan **Bahasa Indonesia** yang baik dan benar.
6. **Larangan**: Tidak diperkenankan menggunakan AI-generated voiceover untuk seluruh video.
