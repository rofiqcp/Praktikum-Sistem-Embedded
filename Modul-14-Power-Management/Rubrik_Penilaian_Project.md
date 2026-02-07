# Rubrik Penilaian Project Modul 14: Power Management & Low-Power Design

## Informasi Umum

| Item | Keterangan |
|------|------------|
| **Nama Project** | Solar-Powered Weather Station |
| **Modul** | 14 - Power Management & Low-Power Design |
| **Platform** | ESP32 (ESP-IDF) / STM32 (STM32Cube HAL) |
| **Total Nilai** | 100 poin (+10 bonus) |
| **Batas Waktu** | 4 minggu |

---

## A. Fungsionalitas Sistem (40 poin)

### A1. Sensor Reading & Data Collection (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Pembacaan sensor (real/dummy) benar | 5 | Tidak ada | Error banyak | Baca 1 sensor | Baca multiple sensor | Akurat + averaging filter |
| Penyimpanan data ke RTC memory / Backup register | 5 | Tidak ada | Compile error | Simpan 1 variable | Multiple variable | Buffer lengkap + overflow handling |

### A2. Low-Power Mode Implementation (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Deep sleep / Standby mode berfungsi | 5 | Tidak ada | Mode salah | Sleep tapi tidak bangun | Sleep + wake timer | Sleep + multiple wake source |
| Wake-up source dikonfigurasi dengan benar | 5 | Tidak ada | Error config | Timer saja | Timer + GPIO | Timer + GPIO + fallback |

### A3. Adaptive Duty Cycling (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Interval berubah berdasarkan level baterai | 5 | Tidak ada | Hardcoded interval | 2 level | 3-4 level | 5+ level + smooth transition |
| Logic adaptive berjalan benar | 5 | Tidak ada | Logic salah | Sebagian benar | Benar mayoritas | Sempurna + edge case handling |

### A4. Battery Monitoring (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Pembacaan tegangan baterai via ADC | 5 | Tidak ada | ADC error | Raw ADC saja | Konversi ke voltage | Voltage + kalibrasi |
| Konversi ke persentase + status | 5 | Tidak ada | Persentase salah | Linear mapping | Lookup table | Lookup + smoothing + status |

---

## B. Kualitas Kode (25 poin)

### B1. Struktur & Organisasi Kode (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Kode terstruktur (fungsi, modular) | 5 | Spaghetti code | Minimal fungsi | Beberapa fungsi | Modular baik | Arsitektur bersih |
| Penggunaan API framework yang benar (ESP-IDF / HAL) | 5 | API salah | Mix framework | Sebagian benar | Mayoritas benar | Best practice |

### B2. Error Handling & Robustness (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Error checking pada operasi kritis | 5 | Tidak ada | Minimal | ADC/sleep check | + recovery | Comprehensive |
| Watchdog / failsafe mechanism | 5 | Tidak ada | Partial | Watchdog saja | + graceful degradation | + auto-recovery |

### B3. Dokumentasi Kode (5 poin)

| Kriteria | Bobot | 0 | 1 | 2-3 | 4 | 5 |
|----------|-------|---|---|-----|---|---|
| Komentar dan dokumentasi dalam kode | 5 | Tidak ada | Minimal | Fungsi utama | Setiap blok | Lengkap + header |

---

## C. Analisis Power Budget (20 poin)

### C1. Pengukuran & Kalkulasi (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Tabel konsumsi daya per mode | 5 | Tidak ada | 1 mode | 2-3 mode | Semua mode | + perbandingan |
| Kalkulasi battery life | 5 | Tidak ada | Formula salah | 1 skenario | 2-3 skenario | Multiple + analisis |

### C2. Optimasi & Perbandingan (10 poin)

| Kriteria | Bobot | 0 | 1-3 | 4-6 | 7-8 | 9-10 |
|----------|-------|---|-----|-----|-----|------|
| Before/after optimization comparison | 5 | Tidak ada | Deskripsi saja | 1 metrik | Multiple metrik | Grafik + analisis |
| Rekomendasi optimasi lanjutan | 5 | Tidak ada | 1 rekomendasi | 2-3 umum | Spesifik + detail | Actionable + bukti |

---

## D. Fitur Tambahan (15 poin)

### D1. Data Batching / Buffering (5 poin)

| Kriteria | Bobot | 0 | 1-2 | 3-4 | 5 |
|----------|-------|---|-----|-----|---|
| Kumpulkan data lalu kirim batch | 5 | Tidak ada | Attempt | Partial | Lengkap + overflow |

### D2. Multiple Wake-up Sources (5 poin)

| Kriteria | Bobot | 0 | 1-2 | 3-4 | 5 |
|----------|-------|---|-----|-----|---|
| Timer + GPIO/Touch/ULP | 5 | Timer saja | Attempt | 2 source | 3+ source |

### D3. Status Display / Reporting (5 poin)

| Kriteria | Bobot | 0 | 1-2 | 3-4 | 5 |
|----------|-------|---|-----|-----|---|
| Boot count, uptime, battery bar | 5 | Tidak ada | Minimal | 2-3 info | Comprehensive dashboard |

---

## E. Dokumentasi & Presentasi (Bonus 10 poin)

| Kriteria | Bobot |
|----------|-------|
| Video demo sistem berjalan | 5 |
| Diagram blok + flowchart | 5 |

---

## 📊 Ringkasan Penilaian

| Komponen | Bobot | Nilai |
|----------|-------|-------|
| A. Fungsionalitas Sistem | 40 | /40 |
| B. Kualitas Kode | 25 | /25 |
| C. Analisis Power Budget | 20 | /20 |
| D. Fitur Tambahan | 15 | /15 |
| **Total** | **100** | **/100** |
| E. Bonus | 10 | /10 |
| **Grand Total** | **110** | **/110** |

---

## 📋 Konversi Nilai

| Range | Grade | Keterangan |
|-------|-------|------------|
| 90 - 100 | A | Sangat Baik |
| 80 - 89 | B+ | Baik Sekali |
| 70 - 79 | B | Baik |
| 60 - 69 | C+ | Cukup Baik |
| 50 - 59 | C | Cukup |
| < 50 | D/E | Kurang |

---

## ✍️ Catatan Penilai

| Item | Isi |
|------|-----|
| Nama Mahasiswa | |
| NIM | |
| Platform Dipilih | ESP32 (ESP-IDF) / STM32 (STM32Cube) |
| Tanggal Penilaian | |
| Penilai | |
| Catatan Khusus | |
