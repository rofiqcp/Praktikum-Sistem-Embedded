# 🎥 Rubrik Penilaian Tugas Video - Modul 04: ADC (Analog-to-Digital Converter)

## 📋 Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 04 - ADC (Analog-to-Digital Converter) |
| **Jenis Tugas** | Video Demonstrasi & Penjelasan Praktikum ADC |
| **Durasi** | 5-10 menit |
| **Format** | MP4 / MKV (resolusi minimal 720p) |
| **Pengumpulan** | Upload ke Google Drive / YouTube (unlisted) + link di laporan |

---

## 🎯 Tujuan Tugas Video

Mahasiswa membuat video yang mendemonstrasikan pemahaman konsep ADC dan kemampuan implementasi pada mikrokontroler (ESP32/STM32), meliputi penjelasan teori, demo hardware, dan analisis hasil.

---

## 📏 Rubrik Penilaian Detail

### 1. Konten Teknis (35%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Penjelasan Konsep ADC** | Menjelaskan prinsip ADC (sampling, quantization, encoding), resolusi, dan tipe SAR dengan benar dan mendalam | Menjelaskan konsep dasar ADC dengan benar tetapi kurang mendalam | Penjelasan ADC kurang tepat atau sangat dangkal | Tidak menjelaskan konsep ADC atau penjelasan salah |
| **Konfigurasi ADC** | Menjelaskan konfigurasi ADC pada mikrokontroler: channel, resolusi, attenuation (ESP32) atau sampling time (STM32) dengan detail | Menjelaskan sebagian besar konfigurasi ADC | Menjelaskan konfigurasi secara minimal | Tidak menjelaskan konfigurasi ADC |
| **Penjelasan Kode** | Menjelaskan alur program, fungsi-fungsi utama, konversi ADC ke satuan fisik, dan logika filtering | Menjelaskan sebagian besar kode dengan cukup jelas | Penjelasan kode sangat singkat | Tidak menjelaskan kode program |
| **Konversi & Kalibrasi** | Menjelaskan dan mendemonstrasikan konversi raw ADC ke satuan fisik (°C, PPM, Lux, %) serta teknik kalibrasi | Menjelaskan konversi untuk sebagian sensor | Menyebutkan konversi tetapi tidak detail | Tidak membahas konversi atau kalibrasi |
| **Analisis Hasil** | Menganalisis hasil pembacaan: akurasi, noise, perbandingan data mentah vs filter, sumber error | Melakukan analisis sederhana terhadap hasil | Menyebutkan hasil tanpa analisis | Tidak ada analisis hasil |

---

### 2. Demonstrasi Praktik (30%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Rangkaian Hardware** | Menunjukkan rangkaian lengkap (4 sensor + LED + mikrokontroler), rapi, dan menjelaskan koneksi setiap komponen | Rangkaian ditunjukkan dan dijelaskan sebagian | Rangkaian ditunjukkan tanpa penjelasan koneksi | Rangkaian tidak ditunjukkan |
| **Demo Pembacaan Sensor** | Mendemonstrasikan pembacaan keempat sensor secara real-time di Serial Monitor, menunjukkan respons terhadap perubahan input | Mendemonstrasikan 3 sensor dengan respons | Mendemonstrasikan 1-2 sensor | Demo pembacaan gagal atau tidak dilakukan |
| **Demo Threshold Alert** | Mendemonstrasikan sistem peringatan: LED menyala saat threshold terlampaui, pesan peringatan di serial, threshold adjustable via potensiometer | Demo threshold untuk sebagian sensor | Demo threshold minimal | Tidak ada demo threshold |
| **Demo Python Logging** | Mendemonstrasikan script Python: koneksi serial, pencatatan data CSV, dan/atau visualisasi grafik real-time | Script Python berjalan dan mencatat data | Menunjukkan script tetapi tidak berjalan sempurna | Tidak ada demo Python |

---

### 3. Kualitas Presentasi (20%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kejelasan Komunikasi** | Penjelasan jelas, sistematis, percaya diri, menggunakan istilah teknis dengan tepat, tempo bicara baik | Penjelasan cukup jelas dan sistematis | Penjelasan kurang jelas atau terlalu cepat/lambat | Penjelasan membingungkan atau tidak terstruktur |
| **Alur Presentasi** | Alur logis: Pendahuluan → Teori singkat → Demo hardware → Demo software → Analisis → Kesimpulan | Alur cukup logis dengan sedikit lompatan | Alur kurang terstruktur | Tidak ada alur yang jelas |
| **Penggunaan Visual** | Menggunakan zoom pada komponen, highlight kode, anotasi, grafik data, atau slide pendukung | Beberapa visual pendukung digunakan | Visual minim, hanya tangkapan layar | Tidak ada visual pendukung |
| **Durasi & Pacing** | Durasi 5-10 menit, setiap bagian mendapat porsi waktu yang proporsional | Durasi sesuai tetapi distribusi kurang proporsional | Durasi terlalu singkat (<5 mnt) atau panjang (>12 mnt) | Durasi sangat tidak sesuai (<3 mnt atau >15 mnt) |

---

### 4. Kualitas Produksi Video (15%)

| Aspek | Excellent (86-100) | Good (71-85) | Satisfactory (56-70) | Poor (0-55) |
|-------|:-------------------:|:------------:|:--------------------:|:-----------:|
| **Kualitas Gambar** | Resolusi ≥720p, pencahayaan baik, rangkaian dan layar terlihat jelas, fokus tajam | Resolusi cukup, sebagian besar terlihat jelas | Gambar agak buram tetapi masih bisa dilihat | Gambar sangat buram, tidak terlihat jelas |
| **Kualitas Audio** | Suara jelas tanpa noise latar berlebihan, volume konsisten, menggunakan mic eksternal | Suara cukup jelas, sedikit noise | Suara kurang jelas, noise cukup mengganggu | Suara tidak terdengar atau sangat bising |
| **Editing** | Video diedit dengan baik: transisi smooth, teks overlay, zoom ke detail penting, intro/outro | Editing cukup baik dengan beberapa elemen | Editing minimal, video mentah | Tidak ada editing, banyak bagian tidak relevan |

---

## 📊 Ringkasan Bobot Penilaian

| No | Komponen | Bobot | Deskripsi |
|----|----------|:-----:|-----------|
| 1 | Konten Teknis | 35% | Pemahaman ADC, kode, konversi, kalibrasi, analisis |
| 2 | Demonstrasi Praktik | 30% | Demo hardware, sensor, threshold, Python logging |
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
- [ ] Menjelaskan konsep dasar ADC (sampling, quantization, resolusi)
- [ ] Menjelaskan konfigurasi ADC pada mikrokontroler yang digunakan
- [ ] Menunjukkan rangkaian hardware dengan 4 sensor analog
- [ ] Mendemonstrasikan pembacaan sensor real-time di Serial Monitor
- [ ] Menunjukkan konversi raw ADC ke satuan fisik
- [ ] Mendemonstrasikan sistem peringatan threshold
- [ ] Mendemonstrasikan Python data logging
- [ ] Menjelaskan bagian kode yang penting
- [ ] Ada pembukaan (identitas) dan penutup (kesimpulan)
- [ ] Video sudah di-upload dan link dapat diakses

## ⚠️ Ketentuan Khusus

1. **Identitas**: Video harus mencantumkan nama lengkap, NIM, dan kelas di awal video.
2. **Orisinalitas**: Video harus dibuat sendiri. Video yang terbukti plagiat mendapat nilai **0**.
3. **Deadline**: Pengumpulan link video sesuai jadwal. Keterlambatan dikenakan pengurangan **10 poin/hari**.
4. **Aksesibilitas**: Pastikan link video dapat diakses oleh dosen (permission sharing benar).
5. **Bahasa**: Video menggunakan **Bahasa Indonesia** yang baik dan benar.
6. **Larangan**: Tidak diperkenankan menggunakan AI-generated voiceover untuk seluruh video.
