# 🎯 Project Modul 04: Sistem Monitoring Kualitas Udara — ADC Multi-Channel

## 📋 Deskripsi Project

Mahasiswa merancang dan mengimplementasikan **Sistem Monitoring Kualitas Udara** yang menggunakan ADC multi-channel untuk membaca 4 sensor analog secara simultan. Sistem menampilkan data secara real-time melalui Serial Monitor, memberikan peringatan threshold, dan mencatat log data menggunakan Python.

## 🎯 Tujuan Pembelajaran

1. Menerapkan pembacaan ADC multi-channel pada mikrokontroler
2. Mengimplementasikan konversi nilai ADC ke satuan fisik (kalibrasi sensor)
3. Membangun sistem monitoring real-time dengan threshold alert
4. Mengintegrasikan mikrokontroler dengan Python untuk data logging
5. Memahami karakteristik dan keterbatasan ADC pada ESP32 dan STM32

## 🔧 Kebutuhan Hardware

### Komponen Utama

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 / STM32F411 BlackPill | 1 | Mikrokontroler utama |
| 2 | Sensor Gas MQ-135 | 1 | Sensor kualitas udara (CO2, NH3, NOx) |
| 3 | Sensor Suhu LM35 | 1 | Sensor suhu analog (10mV/°C) |
| 4 | LDR (Light Dependent Resistor) | 1 | Sensor cahaya analog |
| 5 | Potensiometer 10kΩ | 1 | Simulasi input analog / threshold adjustment |
| 6 | Resistor 10kΩ | 2 | Pull-down untuk LDR dan pembagi tegangan |
| 7 | Resistor 1kΩ | 1 | Pembatas arus LED |
| 8 | LED Merah | 1 | Indikator peringatan threshold |
| 9 | Buzzer Aktif 5V | 1 | Alarm peringatan (opsional) |
| 10 | Breadboard | 1 | Papan rangkaian |
| 11 | Kabel Jumper | Secukupnya | Male-to-male dan male-to-female |
| 12 | Kabel USB | 1 | Koneksi ke komputer |

### Komponen Tambahan (Opsional)

| No | Komponen | Keterangan |
|----|----------|------------|
| 1 | LCD I2C 16x2 | Tampilan lokal data sensor |
| 2 | Sensor MQ-7 | Sensor CO tambahan |
| 3 | Fan DC 5V + Transistor | Aktuator ventilasi otomatis |

## 📌 Konfigurasi Pin

### ESP32 DevKit V1

| Fungsi | Pin ESP32 | Keterangan |
|--------|-----------|------------|
| MQ-135 (Gas) | GPIO 34 (ADC1_CH6) | Input only, 12-bit ADC |
| LM35 (Suhu) | GPIO 35 (ADC1_CH7) | Input only, 12-bit ADC |
| LDR (Cahaya) | GPIO 32 (ADC1_CH4) | 12-bit ADC |
| Potensiometer | GPIO 33 (ADC1_CH5) | 12-bit ADC, threshold adjust |
| LED Peringatan | GPIO 2 | Output digital |
| Buzzer | GPIO 4 | Output digital (opsional) |

> **Catatan ESP32:** Gunakan hanya ADC1 (GPIO 32-39) karena ADC2 bermasalah saat WiFi aktif. Perhatikan non-linearitas ADC ESP32 pada tegangan rendah (<0.1V) dan tinggi (>3.1V). Gunakan attenuation 11dB untuk range 0-3.3V.

### STM32F411 BlackPill

| Fungsi | Pin STM32 | Channel ADC | Keterangan |
|--------|-----------|-------------|------------|
| MQ-135 (Gas) | PA0 | ADC1_IN0 | 12-bit ADC |
| LM35 (Suhu) | PA1 | ADC1_IN1 | 12-bit ADC |
| LDR (Cahaya) | PA4 | ADC1_IN4 | 12-bit ADC |
| Potensiometer | PA5 | ADC1_IN5 | 12-bit ADC, threshold adjust |
| LED Peringatan | PC13 | - | Output digital (on-board LED) |
| Buzzer | PB0 | - | Output digital (opsional) |

> **Catatan STM32:** ADC STM32F411 memiliki linearitas yang lebih baik dibanding ESP32. Gunakan mode scan untuk multi-channel. Perhatikan referensi tegangan VREF+ = 3.3V. Konfigurasi sampling time minimal 15 cycles untuk akurasi optimal.

## 📝 Spesifikasi Fungsional

### Fitur Wajib (Minimum Requirements)

1. **Pembacaan 4 Channel ADC**
   - Membaca keempat sensor analog secara bergantian (polling) atau simultan (DMA/scan mode)
   - Interval pembacaan: setiap 1 detik
   - Menampilkan nilai mentah (raw ADC) dan nilai terkonversi

2. **Konversi & Kalibrasi Sensor**
   - MQ-135: Konversi ke PPM (menggunakan kurva karakteristik dari datasheet)
   - LM35: Konversi ke °C (rumus: `suhu = (adc_value * 3.3 / 4095) * 100`)
   - LDR: Konversi ke Lux (menggunakan tabel lookup atau rumus empiris)
   - Potensiometer: Konversi ke persentase (0-100%)

3. **Tampilan Serial Monitor**
   - Format tampilan terstruktur dan mudah dibaca
   - Menampilkan semua nilai sensor dalam satu frame
   - Contoh format output:
     ```
     ======== MONITORING KUALITAS UDARA ========
     Waktu: 12345 ms
     Gas (MQ-135) : Raw=2048 | 45.2 PPM [NORMAL]
     Suhu (LM35)  : Raw=310  | 25.0 °C  [NORMAL]
     Cahaya (LDR) : Raw=3500 | 850 Lux  [TERANG]
     Threshold     : Raw=2048 | 50%
     Status: AMAN ✓
     ============================================
     ```

4. **Sistem Peringatan Threshold**
   - Gas > threshold PPM → LED menyala + pesan "BAHAYA GAS!"
   - Suhu > 40°C → LED berkedip + pesan "SUHU TINGGI!"
   - Cahaya < threshold Lux → pesan "GELAP"
   - Threshold dapat diatur melalui potensiometer
   - Status keseluruhan: AMAN / PERINGATAN / BAHAYA

5. **Data Logging dengan Python**
   - Script Python membaca data dari Serial port
   - Menyimpan data ke file CSV dengan timestamp
   - Format CSV: `timestamp, gas_raw, gas_ppm, suhu_raw, suhu_c, cahaya_raw, cahaya_lux, pot_persen, status`
   - Minimal durasi logging: 5 menit

### Fitur Tambahan (Bonus)

1. **Visualisasi Real-time dengan Python**
   - Grafik matplotlib real-time untuk semua sensor
   - Garis threshold pada grafik
   - Update setiap 1 detik

2. **Averaging & Filtering**
   - Moving average filter (N=10 sampel)
   - Median filter untuk menghilangkan noise spike
   - Perbandingan data mentah vs data terfilter

3. **Mode Kalibrasi**
   - Mode khusus untuk kalibrasi setiap sensor
   - Menyimpan offset dan gain ke EEPROM/NVS
   - Perintah serial untuk masuk mode kalibrasi

4. **Multi-Mode Tampilan**
   - Mode ringkas (1 baris per pembacaan)
   - Mode detail (seperti contoh di atas)
   - Mode JSON (untuk parsing otomatis)
   - Pemilihan mode via perintah serial

## 📊 Deliverables (Yang Harus Dikumpulkan)

### 1. Source Code
- **Firmware mikrokontroler** (file `.cpp` / `.c` dengan komentar lengkap)
- **Script Python** untuk data logging dan visualisasi
- **File `platformio.ini`** yang sudah dikonfigurasi
- Semua file dikumpulkan dalam satu repository/folder terstruktur

### 2. Laporan Tertulis
Laporan dalam format PDF (minimal 10 halaman) berisi:

| Bagian | Konten |
|--------|--------|
| Pendahuluan | Latar belakang, tujuan, ruang lingkup |
| Dasar Teori | Prinsip ADC, karakteristik sensor, kalibrasi |
| Perancangan | Skema rangkaian (Fritzing/KiCad), diagram alir program |
| Implementasi | Penjelasan kode, konfigurasi ADC, algoritma konversi |
| Pengujian | Hasil pengukuran, tabel data, grafik, analisis error |
| Analisis | Perbandingan ESP32 vs STM32 ADC, sumber error, solusi |
| Kesimpulan | Rangkuman hasil dan saran pengembangan |

### 3. Video Demonstrasi
- Durasi: 5-10 menit
- Menunjukkan rangkaian hardware yang sudah terpasang
- Demo pembacaan sensor secara real-time
- Demo sistem peringatan threshold
- Demo data logging Python
- Penjelasan singkat kode dan cara kerja

## 📅 Timeline Pengerjaan

| Minggu | Aktivitas |
|--------|-----------|
| Minggu 1 | Perakitan hardware, konfigurasi ADC single channel |
| Minggu 2 | Implementasi multi-channel, konversi & kalibrasi sensor |
| Minggu 3 | Sistem threshold, integrasi Python logging |
| Minggu 4 | Pengujian, debugging, pembuatan laporan & video |

## 📏 Kriteria Penilaian

| Komponen | Bobot | Keterangan |
|----------|-------|------------|
| Fungsionalitas | 30% | Semua fitur wajib berjalan dengan benar |
| Kualitas Kode | 25% | Struktur, komentar, best practice, modularitas |
| Laporan | 20% | Kelengkapan, analisis, kedalaman pembahasan |
| Presentasi/Video | 15% | Kejelasan demo, penjelasan teknis |
| Kreativitas & Bonus | 10% | Fitur tambahan, inovasi, desain hardware |

## ⚠️ Catatan Penting

1. **Keselamatan:** Sensor MQ-135 membutuhkan pemanasan (burn-in) 24-48 jam untuk pembacaan akurat. Hati-hati dengan panas pada elemen pemanas sensor.
2. **Tegangan:** Pastikan semua sensor beroperasi pada tegangan yang sesuai (3.3V atau 5V dengan pembagi tegangan).
3. **Referensi ADC:** Perhatikan perbedaan VREF pada ESP32 (1.1V internal + attenuation) dan STM32 (VDDA = 3.3V).
4. **Grounding:** Gunakan ground bersama (common ground) untuk semua sensor agar pembacaan stabil.
5. **Noise:** Tambahkan kapasitor 100nF dekat pin VREF dan pin analog sensor untuk mengurangi noise.

## 📚 Referensi Pendukung

- Datasheet MQ-135: Kurva sensitivitas Rs/R0 vs PPM
- Datasheet LM35: Karakteristik transfer 10mV/°C
- ESP-IDF ADC Documentation: Konfigurasi attenuation dan kalibrasi
- STM32 HAL ADC Reference: Mode scan, DMA, sampling time
- Application Note AN2834: How to get the best ADC accuracy in STM32
