# 📊 Rubrik Penilaian Project — Modul 06: Komunikasi Cerdas — I2C & Sensor

## 🎯 Project: Weather Station I2C Multi-Sensor dengan Data Logging

---

## A. Fungsionalitas Sistem (30%)

### A1. I2C Bus Scan & Inisialisasi (6%)

| Skor | Kriteria |
|:----:|----------|
| 5-6 | Scan bus berhasil mendeteksi SEMUA device, inisialisasi setiap sensor dengan verifikasi chip ID, error report jika device missing |
| 3-4 | Scan bus berhasil, inisialisasi sensor tanpa verifikasi chip ID |
| 1-2 | Scan bus parsial, hanya beberapa device terdeteksi |
| 0 | Tidak ada implementasi I2C scan |

### A2. Pembacaan Sensor (12%)

| Skor | Kriteria |
|:----:|----------|
| 10-12 | BMP280 (suhu+tekanan), BH1750 (cahaya), DS3231 (waktu) → semua terbaca akurat, data ter-calibrate, format output benar |
| 7-9 | Minimal 2 sensor terbaca dengan benar |
| 4-6 | Hanya 1 sensor terbaca dengan benar |
| 1-3 | Sensor terbaca tapi data tidak akurat / tidak ter-calibrate |
| 0 | Tidak ada pembacaan sensor |

### A3. Display OLED SSD1306 (6%)

| Skor | Kriteria |
|:----:|----------|
| 5-6 | Display menampilkan semua data sensor dengan layout rapi, update periodik, font terbaca jelas |
| 3-4 | Display menampilkan data tapi layout kurang rapi atau tidak semua data ditampilkan |
| 1-2 | Display hanya menampilkan teks statis atau data parsial |
| 0 | OLED tidak berfungsi |

### A4. Data Logging ke EEPROM (6%)

| Skor | Kriteria |
|:----:|----------|
| 5-6 | Data tersimpan ke AT24C32 dengan timestamp, page write benar, bisa dibaca kembali, circular buffer |
| 3-4 | Data tersimpan tapi tanpa timestamp atau tanpa circular buffer |
| 1-2 | Data tersimpan tapi sering corrupt atau incomplete |
| 0 | Tidak ada data logging |

---

## B. Kualitas Kode (25%)

### B1. Struktur & Modularitas (10%)

| Skor | Kriteria |
|:----:|----------|
| 9-10 | Kode terstruktur dalam modul terpisah per device (bmp280.h, bh1750.h, ds3231.h, ssd1306.h, eeprom.h), abstraction layer, clean interface |
| 7-8 | Fungsi terpisah per device tapi masih dalam satu file, interface cukup bersih |
| 4-6 | Ada pemisahan fungsi tapi kurang konsisten |
| 1-3 | Semua kode dalam satu fungsi panjang |
| 0 | Kode tidak terstruktur |

### B2. Error Handling I2C (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Setiap I2C transaction dicek return value, retry mechanism, bus recovery, timeout handling, graceful degradation (satu sensor fail → lainnya tetap jalan) |
| 5-6 | Return value dicek, ada retry, tapi tidak ada bus recovery |
| 3-4 | Ada pengecekan error tapi tidak comprehensive |
| 1-2 | Minimal error checking |
| 0 | Tidak ada error handling |

### B3. Komentar & Dokumentasi Kode (7%)

| Skor | Kriteria |
|:----:|----------|
| 6-7 | Setiap fungsi ada header comment (tujuan, parameter, return), komentar inline pada bagian kompleks, register address dijelaskan |
| 4-5 | Ada komentar tapi tidak konsisten |
| 2-3 | Komentar minimal |
| 0-1 | Tidak ada komentar |

---

## C. Laporan Teknis (20%)

### C1. Analisis Protokol I2C (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Penjelasan mendalam: START/STOP, addressing, ACK/NACK, register read/write sequence untuk setiap sensor, timing diagram |
| 5-6 | Penjelasan protokol baik tapi kurang detail per sensor |
| 3-4 | Penjelasan dasar I2C tanpa analisis per sensor |
| 1-2 | Penjelasan sangat superfisial |
| 0 | Tidak ada analisis protokol |

### C2. Hasil Pengujian & Analisis Data (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Screenshot serial output, grafik Python (suhu/tekanan/cahaya vs waktu), analisis statistik (mean, stddev), interpretasi data, perbandingan ESP32 vs STM32 |
| 5-6 | Ada screenshot dan grafik tapi analisis kurang mendalam |
| 3-4 | Hanya screenshot tanpa analisis |
| 1-2 | Hasil pengujian minimal |
| 0 | Tidak ada hasil pengujian |

### C3. Dokumentasi Hardware (4%)

| Skor | Kriteria |
|:----:|----------|
| 4 | Skematik koneksi lengkap (semua sensor + pull-up), foto hardware, penjelasan pin mapping |
| 3 | Skematik ada tapi kurang lengkap |
| 2 | Hanya foto tanpa skematik |
| 1 | Dokumentasi hardware minimal |
| 0 | Tidak ada dokumentasi hardware |

---

## D. Video Demo (15%)

### D1. Demonstrasi Fungsionalitas (8%)

| Skor | Kriteria |
|:----:|----------|
| 7-8 | Demo live: scan bus, baca semua sensor, tampilan OLED, data logging, Python dashboard, error recovery (cabut sensor → recovery → reconnect) |
| 5-6 | Demo semua sensor + display tapi tanpa error recovery demo |
| 3-4 | Demo parsial (tidak semua fitur ditunjukkan) |
| 1-2 | Demo singkat tanpa penjelasan |
| 0 | Tidak ada video |

### D2. Penjelasan Teknis (7%)

| Skor | Kriteria |
|:----:|----------|
| 6-7 | Penjelasan kode dan arsitektur yang jelas, menunjukkan pemahaman I2C protocol, bisa menjelaskan kenapa kode ditulis demikian |
| 4-5 | Penjelasan cukup tapi kurang mendalam |
| 2-3 | Penjelasan minimal, membaca dari catatan |
| 0-1 | Tidak ada penjelasan |

---

## E. Presentasi & Q&A (10%)

### E1. Kualitas Slide (4%)

| Skor | Kriteria |
|:----:|----------|
| 4 | Slide profesional (max 15), diagram jelas, tidak terlalu padat teks, visual menarik |
| 3 | Slide baik tapi beberapa terlalu padat |
| 2 | Slide standar, kurang visual |
| 1 | Slide minimal atau terlalu banyak teks |
| 0 | Tidak ada slide |

### E2. Kemampuan Menjawab (6%)

| Skor | Kriteria |
|:----:|----------|
| 5-6 | Menjawab pertanyaan teknis dengan benar dan percaya diri, memahami detail implementasi dan protokol I2C |
| 3-4 | Menjawab dengan benar tapi kurang percaya diri atau kurang detail |
| 1-2 | Jawaban kurang tepat atau sangat singkat |
| 0 | Tidak bisa menjawab |

---

## 📊 Rekapitulasi Bobot

| Komponen | Bobot | Skor Max |
|----------|:-----:|:--------:|
| A. Fungsionalitas Sistem | 30% | 30 |
| B. Kualitas Kode | 25% | 25 |
| C. Laporan Teknis | 20% | 20 |
| D. Video Demo | 15% | 15 |
| E. Presentasi & Q&A | 10% | 10 |
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

## ⚠️ Ketentuan Khusus

1. **Plagiarisme**: Kode copy-paste tanpa pemahaman → nilai 0 pada komponen B
2. **Tidak compile**: Program tidak bisa di-compile (`pio run` fail) → maksimal C pada komponen A
3. **Hardware tidak jalan**: Boleh demo via simulasi tapi nilai A maksimal 70%
4. **Keterlambatan**: Pengumpulan terlambat → -10% per hari (max -30%)
5. **Bonus**:
   - I2C multiplexer (TCA9548A) → +5%
   - DMA I2C transfer → +5%
   - Custom I2C slave pada MCU kedua → +5%
   - Maximum bonus: +10%

---

## 📋 Contoh Pertanyaan Q&A

1. Jelaskan perbedaan `HAL_I2C_Mem_Read()` dan `HAL_I2C_Master_Receive()`.
2. Mengapa address I2C di STM32 HAL harus di-shift left 1 bit?
3. Apa yang terjadi jika pull-up resistor terlalu besar (100kΩ)?
4. Bagaimana cara mengatasi address conflict pada bus I2C?
5. Jelaskan proses kalibrasi BMP280 (compensation formula).
6. Mengapa EEPROM AT24C32 butuh delay 5ms setelah write?
7. Apa perbedaan BMP280 forced mode dan normal mode?
8. Bagaimana clock stretching bekerja dan kapan slave menggunakannya?
9. Jelaskan format BCD pada DS3231 dan cara konversinya.
10. Bagaimana recovery jika SDA stuck LOW?

---

*Modul 06 — Praktikum Sistem Embedded*
*Rubrik Penilaian Project I2C Multi-Sensor*
