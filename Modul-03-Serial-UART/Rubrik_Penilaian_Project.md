# Rubrik Penilaian Project Modul 03
## Topik: Wireless Sensor Network Gateway (STM32 + ESP32)

Project ini dinilai berdasarkan kemampuan mahasiswa mengintegrasikan dua mikrokontroler berbeda platform menggunakan komunikasi serial UART yang reliable.

## 📊 Bobot Penilaian
Total Skor Maksimum: **100 Poin**

| Komponen | Bobot | Deskripsi |
|----------|-------|-----------|
| **D1: Source Code** | 40% | Kualitas, struktur, dan fungsionalitas kode program |
| **D2: Demo Sistem** | 30% | Pembuktian fungsi sistem secara langsung/video |
| **D3: Laporan** | 20% | Dokumentasi teknis dan analisis |
| **D4: Timework (Indiv)** | 10% | Pemahaman individu (jika kelompok) / Kerapian repo |

---

## 📝 Detail Kriteria Penilaian

### 1. Source Code Implementation (40 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Protocol Implementation** | - Menggunakan struct packet (bukan string parsing) <br> - Terdapat Start Byte & End Byte/Length <br> - Implementasi Checksum/CRC berfungsi benar | 15 |
| **STM32 Firmware** | - Membaca minimal 3 sensor (simulasi/real) <br> - Pengiriman data non-blocking (millis) <br> - Handing command RX dari Gateway | 10 |
| **ESP32 Gateway** | - Buffer management (tidak ada data loss) <br> - Parsing data sukses 100% <br> - Web Server menampilkan data real-time | 10 |
| **Code Quality** | - Modular (file terpisah untuk protocol/driver) <br> - Komentar informatif <br> - Variabel naming convention konsisten | 5 |

### 2. Demonstrasi & Fungsionalitas (30 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Basic Communication** | - Data dari sensor STM32 tampil di Serial Monitor ESP32 <br> - Tidak ada karakter sampah (garbage) | 10 |
| **Reliability Test** | - Sistem tetap berjalan saat kabel dicabut-pasang (Auto-recover) <br> - Validasi error detection (CRC reject corrupt packet) | 10 |
| **Web Dashboard** | - UI rapi dan responsif <br> - Fitur kontrol balik (e.g., turn on LED di STM32 dari Web) berfungsi | 10 |

### 3. Laporan & Analisis (20 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Metodologi** | - Diagram blok sistem jelas <br> - Skematik wiring lengkap (Fritzing/CAD) <br> - Penjelasan format protokol yang digunakan | 10 |
| **Analisis Data** | - Screenshot sinyal logic analyzer/serial monitor <br> - Analisis throughput/reliability <br> - Pembahasan kendala dan solusi yang diterapkan | 10 |

### 4. Extra Features (Bonus up to +10)

Mahasiswa dapat mendapatkan nilai tambah jika mengimplementasikan fitur di bawah:
- **ACK/Retry Mechanism:** Mengirim ulang data jika tidak ada acknowledge dari gateway (+5).
- **OTA Update:** Kemampuan update firmware via WiFi (+5).
- **Database:** Menyimpan log history data ke flash memory/SD Card/Cloud Database (+5).

---

## ❌ Faktor Pengurang Nilai

- **Terlambat:** -5 poin per hari keterlambatan.
- **Plagiasi:** Salinan identik kode teman/internet tanpa modifikasi signifikan = **Nilai 0**.
- **Tidak Kompile:** Kode yang dikumpulkan error saat dicompile = **Maksimal Nilai 50**.
- **Hardcoded:** Data sensor palsu (hardcoded) tanpa pembacaan nyata = **-20 poin**.

---

## 📋 Lembar Penilaian (Untuk Asisten)

**Nama Mahasiswa / NIM:** ____________________

| No | Kriteria | Skor Max | Skor Perolehan | Catatan |
|----|----------|----------|----------------|---------|
| 1 | Protokol Biner & CRC | 15 | | |
| 2 | Firmware STM32 | 10 | | |
| 3 | Firmware ESP32 & Web | 10 | | |
| 4 | Kualitas Kode | 5 | | |
| 5 | Fungsionalitas Dasar | 10 | | |
| 6 | Reliability & Recovery | 10 | | |
| 7 | Web Dashboard Feature | 10 | | |
| 8 | Laporan Teknis | 20 | | |
| 9 | Softskill/Repo | 10 | | |
| **TOTAL** | | **100** | | |
