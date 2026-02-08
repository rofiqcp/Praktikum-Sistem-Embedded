# 🎥 Rubrik Penilaian Tugas Video — Modul 06: Komunikasi Cerdas — I2C & Sensor

## 📋 Deskripsi Tugas

Mahasiswa membuat video tutorial/demo yang menjelaskan **implementasi komunikasi I2C** dengan sensor pada ESP32 (ESP-IDF) dan/atau STM32 (HAL). Video harus mendemonstrasikan pemahaman protokol I2C, kemampuan interfacing sensor, dan analisis data menggunakan Python.

---

## 📐 Spesifikasi Video

| Aspek | Ketentuan |
|-------|-----------|
| **Durasi** | 8-15 menit |
| **Resolusi** | Minimum 720p (1280×720) |
| **Audio** | Narasi jelas, tidak berisik |
| **Format** | MP4 (H.264) |
| **Platform** | Upload ke YouTube/Google Drive |
| **Deadline** | Sesuai jadwal di LMS |

---

## A. Konten Teknis (40%)

### A1. Penjelasan Protokol I2C (12%)

| Skor | Kriteria |
|:----:|----------|
| 10-12 | Menjelaskan dengan benar: SDA/SCL, START/STOP, addressing 7-bit, ACK/NACK, read/write sequence, pull-up resistor. Menggunakan diagram/animasi untuk ilustrasi |
| 7-9 | Penjelasan benar tapi kurang lengkap (beberapa konsep terlewat) |
| 4-6 | Penjelasan dasar, beberapa kesalahan minor |
| 1-3 | Penjelasan sangat singkat atau ada kesalahan konsep |
| 0 | Tidak ada penjelasan protokol |

### A2. Walkthrough Kode (16%)

| Skor | Kriteria |
|:----:|----------|
| 14-16 | Menjelaskan kode step-by-step: konfigurasi I2C, inisialisasi sensor, baca register, konversi data, error handling. Menunjukkan perbedaan ESP-IDF vs HAL API |
| 10-13 | Walkthrough kode baik tapi hanya satu platform atau kurang detail error handling |
| 6-9 | Menunjukkan kode tapi penjelasan kurang mendalam |
| 3-5 | Kode ditunjukkan sekilas tanpa penjelasan berarti |
| 0-2 | Tidak ada walkthrough kode |

### A3. Analisis Data & Python Tool (12%)

| Skor | Kriteria |
|:----:|----------|
| 10-12 | Demo Python tool: parse serial data, realtime plot (suhu/tekanan/cahaya), export CSV, tampilkan statistik. Interpretasi data hasil pengukuran |
| 7-9 | Python tool berfungsi, ada plot tapi tidak semua fitur ditunjukkan |
| 4-6 | Python tool basic (hanya serial read atau hanya plot statis) |
| 1-3 | Hanya menunjukkan serial monitor tanpa Python tool |
| 0 | Tidak ada analisis data |

---

## B. Demonstrasi Hardware (25%)

### B1. Setup & Wiring (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Close-up wiring yang jelas, menunjukkan koneksi SDA/SCL/VCC/GND ke setiap sensor, pull-up resistor, penjelasan pin mapping |
| 5-6 | Wiring terlihat tapi kurang detail (tidak close-up) |
| 3-4 | Hardware terlihat tapi koneksi tidak dijelaskan |
| 1-2 | Sekilas menunjukkan hardware |
| 0 | Tidak menunjukkan hardware |

### B2. Demo Live Sensor Reading (10%)

| Skor | Kriteria |
|:----:|----------|
| 9-10 | Demo live: I2C scan → init sensor → baca BMP280 (suhu+tekanan) → BH1750 (cahaya) → DS3231 (waktu) → tampilkan di OLED → data logging. Semua berfungsi real-time |
| 7-8 | Demo live minimal 3 sensor bekerja |
| 4-6 | Demo live tapi hanya 1-2 sensor |
| 1-3 | Demo dengan rekaman output statis (bukan live) |
| 0 | Tidak ada demo |

### B3. Error Recovery Demo (7%)

| Skor | Kriteria |
|:----:|----------|
| 6-7 | Demo langsung: cabut sensor saat running → sistem deteksi error → log warning → sensor dipasang kembali → recovery otomatis → pembacaan normal lagi |
| 4-5 | Demo error detection tapi tanpa live recovery |
| 2-3 | Menjelaskan error handling tapi tidak demo live |
| 1 | Hanya menyebutkan error handling |
| 0 | Tidak ada demo error handling |

---

## C. Kualitas Presentasi (20%)

### C1. Struktur & Alur (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Video terstruktur: Intro → Teori I2C → Hardware setup → Kode walkthrough → Demo live → Python analysis → Kesimpulan. Transisi smooth |
| 5-6 | Alur jelas tapi ada bagian yang loncat-loncat |
| 3-4 | Struktur kurang terorganisir |
| 1-2 | Tidak ada struktur yang jelas |
| 0 | Sangat berantakan |

### C2. Visual & Editing (6%)

| Skor | Kriteria |
|:----:|----------|
| 5-6 | Kamera jelas, screen recording tajam, zoom pada bagian penting, split screen (kode + hardware), annotation/highlight pada kode |
| 3-4 | Visual baik tapi kurang editing (tanpa zoom/annotation) |
| 1-2 | Visual standar, kadang blur atau gelap |
| 0 | Kualitas visual buruk |

### C3. Narasi & Komunikasi (6%)

| Skor | Kriteria |
|:----:|----------|
| 5-6 | Narasi jelas, tempo tepat, menggunakan istilah teknis dengan benar, percaya diri, tidak banyak "eee...", volume konsisten |
| 3-4 | Narasi cukup jelas tapi kadang ragu atau terlalu cepat/lambat |
| 1-2 | Narasi kurang jelas, banyak jeda, suara terlalu pelan |
| 0 | Tidak ada narasi / tidak terdengar |

---

## D. Kedalaman Teknis (15%)

### D1. Pemahaman I2C Protocol (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Menunjukkan pemahaman mendalam: bisa jelaskan kapan clock stretching terjadi, kenapa perlu repeated START untuk read, perbedaan polling vs interrupt I2C, address conflict resolution |
| 5-6 | Pemahaman baik tapi kurang mendalam pada beberapa konsep |
| 3-4 | Pemahaman dasar I2C, mengerti cara pakai tapi kurang mengerti kenapa |
| 1-2 | Pemahaman sangat surface-level |
| 0 | Tidak menunjukkan pemahaman |

### D2. Perbandingan Platform (7%)

| Skor | Kriteria |
|:----:|----------|
| 6-7 | Perbandingan jelas ESP-IDF vs HAL: perbedaan API (command link vs Mem_Read), addressing (7-bit vs shifted), konfigurasi pin, kelebihan masing-masing |
| 4-5 | Perbandingan ada tapi kurang detail |
| 2-3 | Hanya menyebutkan perbedaan sekilas |
| 0-1 | Tidak ada perbandingan |

---

## 📊 Rekapitulasi Bobot

| Komponen | Bobot | Skor Max |
|----------|:-----:|:--------:|
| A. Konten Teknis | 40% | 40 |
| B. Demonstrasi Hardware | 25% | 25 |
| C. Kualitas Presentasi | 20% | 20 |
| D. Kedalaman Teknis | 15% | 15 |
| **Total** | **100%** | **100** |

---

## 📏 Konversi Nilai

| Range Skor | Nilai | Predikat |
|:----------:|:-----:|----------|
| 85-100 | A | Sangat Baik |
| 75-84 | B+ | Baik Sekali |
| 65-74 | B | Baik |
| 55-64 | C+ | Cukup Baik |
| 45-54 | C | Cukup |
| 35-44 | D | Kurang |
| 0-34 | E | Sangat Kurang |

---

## ✅ Checklist Sebelum Submit

### Konten Wajib
- [ ] Penjelasan protokol I2C (SDA, SCL, START, STOP, ACK)
- [ ] Close-up hardware wiring dengan penjelasan pin
- [ ] I2C bus scan menunjukkan device terdeteksi
- [ ] Demo pembacaan minimal 2 sensor berbeda
- [ ] Walkthrough kode (konfigurasi + baca sensor)
- [ ] Serial output menunjukkan data sensor
- [ ] Python tool demo (minimal plot atau CSV export)

### Konten Bonus
- [ ] Demo OLED display menampilkan data (+3%)
- [ ] Demo error recovery live (+3%)
- [ ] Perbandingan ESP32 vs STM32 side-by-side (+2%)
- [ ] Logic analyzer trace I2C communication (+2%)
- [ ] Maximum bonus: +8%

### Teknis Video
- [ ] Durasi 8-15 menit
- [ ] Resolusi minimal 720p
- [ ] Audio jelas, narasi terdengar
- [ ] Screen recording bisa dibaca (font cukup besar)
- [ ] Tidak ada bagian video hitam/silence panjang

---

## ⚠️ Ketentuan

1. **Video 100% original** — bukan copy dari tutorial YouTube
2. **Wajah terlihat** minimal saat intro dan penutup
3. **Kode sendiri** — harus bisa menjelaskan setiap baris
4. **Hardware sendiri** — foto/video hardware asli (bukan simulasi)
5. **Keterlambatan**: -10% per hari (max -30%)
6. **Durasi < 5 menit**: Maksimal nilai C
7. **Durasi > 20 menit**: Tidak ada penalti tapi usahakan efisien

---

## 📋 Template Struktur Video yang Direkomendasikan

```
[0:00 - 0:30]  Opening: Intro diri, judul project
[0:30 - 2:00]  Teori: Penjelasan singkat I2C protocol
[2:00 - 3:30]  Hardware: Tunjukkan wiring + komponen
[3:30 - 6:00]  Kode: Walkthrough main.c (init, read, display)
[6:00 - 8:00]  Demo: Live sensor reading + OLED display
[8:00 - 9:30]  Error: Demo error recovery (cabut sensor)
[9:30 - 11:00] Python: Demo analysis tool + plot
[11:00- 12:00] Closing: Kesimpulan + lessons learned
```

---

*Modul 06 — Praktikum Sistem Embedded*
*Rubrik Penilaian Tugas Video I2C & Sensor*
