# Rubrik Penilaian Project Modul 04
## Topik: Smart Battery Monitoring System (BMS)

Project ini menilai kemampuan mahasiswa dalam melakukan akuisisi data analog presisi, pengondisian sinyal digital, dan integrasi sistem monitoring.

## 📊 Bobot Penilaian
Total Skor Maksimum: **100 Poin**

| Komponen | Bobot | Deskripsi |
|----------|-------|-----------|
| **D1: Signal Quality** | 35% | Akurasi, stabilitas, dan filtering data ADC |
| **D2: System Integration** | 25% | Komunikasi STM32-ESP32 & Logic Alert |
| **D3: Dashboard UI** | 20% | Visualisasi data pada Web Interface |
| **D4: Laporan/Analisis** | 20% | Analisis error dan kalibrasi |

---

## 📝 Detail Kriteria Penilaian

### 1. Kualitas Sinyal & Firmware STM32 (35 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Akurasi Tegangan** | - Error pembacaan < 2% dibanding Multimeter (pada range 5-12V) | 15 |
| **Filtering Digital** | - Implementasi Moving Average berfungsi (Output stabil saat input constant) <br> - Respons step signal < 1 detik | 15 |
| **Multi-channel** | - Sukses membaca 3 parameter secara simultan (Volt, Ampere, Suhu) tanpa saling interferensi (crosstalk) | 5 |

### 2. Integrasi Sistem & ESP32 (25 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Data Transmission** | - Data floating point terkirim utuh via UART (struct/json) | 10 |
| **Alert Logic** | - LED/Buzzer aktif sesuai threshold tegangan yang ditentukan <br> - State transition (Normal -> Low -> Critical) berjalan mulus | 10 |
| **SoC Estimation** | - Terdapat kalkulasi % baterai (bukan sekedar menampilkan raw voltage) | 5 |

### 3. Web Dashboard (20 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Real-time Data** | - Angka tegangan/arus update otomatis tanpa refresh page (AJAX/WebSocket) | 10 |
| **Visualisasi** | - Tampilan menarik (Gauge/Bar/Chart) <br> - Indikator status baterai jelas (Warna Merah/Hijau) | 10 |

### 4. Laporan & Analisis (20 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Data Kalibrasi** | - Menyertakan tabel perbandingan nilai RAW ADC vs Tegangan Sebenarnya <br> - Menghitung konstanta kalibrasi (Slope/Offset) | 10 |
| **Analisis Noise** | - Menampilkan perbandingan grafik sinyal sebelum dan sesudah filtering (Serial Plotter screenshot) | 10 |

---

## 🌟 Bonus Points (Fitur Tambahan)

- **Kalibrasi Otomatis:** Sistem memiliki mode kalibrasi yang menyimpan nilai offset ke EEPROM/Flash (+5).
- **History Graph:** Web dashboard menampilkan grafik riwayat tegangan 1 jam terakhir (+5).
- **Safety Cut-off:** Simulasi sinyal output untuk memutus beban (Relay) saat tegangan Critical (+5).

---

## Awas Plagiasi & Hardcoding!
- Menggunakan `random()` untuk meniru fluktuasi sinyal sensor = **NILAI 0**.
- Menggunakan data dummy statis = **NILAI 0**.
- Wajib menggunakan input analog fisik (Potensio/Sensor). 
