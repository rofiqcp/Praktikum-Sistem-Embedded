# Rubrik Penilaian Project
## Modul 03: Serial UART Communication - Wireless Sensor Network Gateway

---

## 📋 Informasi Penilaian

| Item | Keterangan |
|------|------------|
| **Nama Project** | Wireless Sensor Network Gateway (STM32 + ESP32) |
| **Total Skor Maksimum** | 100 poin |
| **Passing Grade** | 60 poin (C) |
| **Penilaian** | Kelompok (maks 2 orang) |
| **Framework** | STM32Cube HAL (STM32) dan ESP-IDF (ESP32) |

---

## 📊 Komponen Penilaian

| Komponen | Bobot | Poin Maksimum |
|----------|-------|---------------|
| Fungsionalitas Sistem | 40% | 40 poin |
| Kualitas Kode | 20% | 20 poin |
| Dokumentasi | 15% | 15 poin |
| Video Demonstrasi | 15% | 15 poin |
| Fitur Bonus | 10% | 10 poin |
| **Total** | **100%** | **100 poin** |

---

## 📝 Rubrik Detail

### A. Fungsionalitas Sistem (40 poin)

#### A.1 Komunikasi UART Dasar (16 poin)

| No | Fitur | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| A.1.1 | Konfigurasi UART (HAL & ESP-IDF) | 4 | **Excellent (4):** UART dikonfigurasi dengan benar menggunakan `HAL_UART_Init()` pada STM32 dan `uart_driver_install()` pada ESP32, baud rate konsisten, parameter frame tepat (8N1)<br>**Good (3):** Konfigurasi benar dengan minor parameter issue<br>**Fair (2):** Konfigurasi berjalan tapi ada hardcoded values atau parameter kurang optimal<br>**Poor (0-1):** Konfigurasi tidak bekerja atau salah |
| A.1.2 | Data Transmit/Receive | 4 | **Excellent (4):** `HAL_UART_Transmit()`/`HAL_UART_Receive_IT()` dan `uart_write_bytes()`/`uart_read_bytes()` berfungsi sempurna, komunikasi bidirectional stabil<br>**Good (3):** Unidirectional bekerja sempurna<br>**Fair (2):** Data terkirim/terima tapi sering error<br>**Poor (0-1):** Komunikasi tidak bekerja |
| A.1.3 | Protocol Frame Implementation | 4 | **Excellent (4):** Frame protocol lengkap dengan STX, type, length, payload, CRC, ETX; struct-based packing digunakan<br>**Good (3):** Frame protocol ada tapi tanpa CRC<br>**Fair (2):** Text-based protocol saja (CSV/plain string)<br>**Poor (0-1):** Tidak ada framing protocol |
| A.1.4 | Error Detection (CRC/Checksum) | 4 | **Excellent (4):** CRC-16 atau XOR checksum diimplementasi dan divalidasi di kedua sisi, corrupt packet di-reject dan NAK dikirim<br>**Good (3):** Checksum XOR berfungsi di satu sisi<br>**Fair (2):** Checksum ada tapi tidak divalidasi<br>**Poor (0-1):** Tidak ada error detection |

#### A.2 Fitur Sensor Node - STM32 (12 poin)

| No | Fitur | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| A.2.1 | Pembacaan Sensor (min 3) | 4 | **Excellent (4):** Minimal 3 sensor dibaca dengan benar via HAL ADC/GPIO, nilai realistis, moving average filter diterapkan<br>**Good (3):** 3 sensor terbaca tanpa filtering<br>**Fair (2):** 1-2 sensor terbaca<br>**Poor (0-1):** Sensor tidak berfungsi atau data hardcoded |
| A.2.2 | Non-blocking Operation | 4 | **Excellent (4):** Pengiriman data non-blocking menggunakan HAL timer interrupt (`HAL_TIM_PeriodElapsedCallback`) atau SysTick, tidak ada `HAL_Delay()` blocking di main loop<br>**Good (3):** Mayoritas non-blocking, 1-2 delay kecil<br>**Fair (2):** Campuran blocking dan non-blocking<br>**Poor (0-1):** Semua blocking dengan `HAL_Delay()` |
| A.2.3 | Command RX Handler | 4 | **Excellent (4):** Menerima command dari ESP32 via `HAL_UART_RxCpltCallback()`, parsing dan eksekusi benar (LED control, config update, immediate read request)<br>**Good (3):** Command diterima tapi respons terbatas<br>**Fair (2):** Hanya menerima tanpa eksekusi command<br>**Poor (0-1):** Tidak ada RX handler |

#### A.3 Fitur Gateway - ESP32 (12 poin)

| No | Fitur | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| A.3.1 | Buffer Management | 4 | **Excellent (4):** Ring buffer diimplementasi dengan benar, tidak ada data loss, overflow handled gracefully, menggunakan `uart_read_bytes()` dengan event queue<br>**Good (3):** Buffer sederhana, jarang overflow<br>**Fair (2):** Buffer ada tapi sering overflow saat burst data<br>**Poor (0-1):** Tidak ada buffering |
| A.3.2 | Data Parsing & Validation | 4 | **Excellent (4):** Parsing protocol frame 100% sukses, CRC validated, NAK dikirim untuk corrupt data, sequence number tracking berfungsi<br>**Good (3):** Parsing bekerja >90%<br>**Fair (2):** Parsing bekerja >70%<br>**Poor (0-1):** Parsing sering gagal |
| A.3.3 | Web Dashboard | 4 | **Excellent (4):** Web server menggunakan ESP-IDF HTTP server (`httpd_start()`), UI responsif, data real-time via WebSocket/AJAX, kontrol balik ke STM32 berfungsi<br>**Good (3):** Web server berfungsi, data tampil, tanpa kontrol balik<br>**Fair (2):** Web server tampil tapi data static<br>**Poor (0-1):** Web server tidak berfungsi |

### B. Kualitas Kode (20 poin)

| No | Aspek | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| B.1 | Struktur & Modularitas | 6 | **Excellent (6):** Kode terpisah dalam modul logis (protocol.c, sensors.c, uart_handler.c), header files terorganisir, fungsi reusable<br>**Good (4):** Struktur cukup baik, beberapa fungsi modular<br>**Fair (2):** Struktur minimal, banyak kode repetitif<br>**Poor (0):** Semua dalam satu file tanpa struktur |
| B.2 | Naming Convention | 4 | **Excellent (4):** Nama variabel/fungsi deskriptif, konsisten (camelCase/snake_case), prefix sesuai modul (uart_, protocol_, sensor_)<br>**Good (3):** Sebagian nama deskriptif<br>**Fair (1-2):** Nama kurang deskriptif<br>**Poor (0):** Nama tidak bermakna (x, y, temp1) |
| B.3 | Comments & Documentation | 4 | **Excellent (4):** Komentar menjelaskan logic penting, Doxygen-style function headers, README per folder project<br>**Good (3):** Komentar cukup<br>**Fair (1-2):** Komentar minimal<br>**Poor (0):** Tidak ada komentar |
| B.4 | Error Handling | 3 | **Excellent (3):** Return value HAL_StatusTypeDef di-check, timeout handled, watchdog recovery, `ESP_ERROR_CHECK()` digunakan<br>**Good (2):** Sebagian error di-handle<br>**Fair (1):** Error handling minimal<br>**Poor (0):** Tidak ada error handling |
| B.5 | Penggunaan API Native | 3 | **Excellent (3):** Konsisten menggunakan STM32Cube HAL API dan ESP-IDF API, tidak ada Arduino API<br>**Good (2):** Mayoritas API native<br>**Fair (1):** Campuran API native dan Arduino<br>**Poor (0):** Mayoritas menggunakan Arduino API |

### C. Dokumentasi (15 poin)

| No | Aspek | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| C.1 | README | 4 | **Excellent (4):** Lengkap (deskripsi project, setup toolchain ESP-IDF/STM32CubeIDE, build instructions, usage, troubleshooting)<br>**Good (3):** Cukup lengkap<br>**Fair (2):** Minimal<br>**Poor (0):** Tidak ada |
| C.2 | Wiring Diagram | 4 | **Excellent (4):** Skematik profesional (Fritzing/KiCad) + foto close-up jelas, pin assignment lengkap STM32↔ESP32<br>**Good (3):** Diagram hand-drawn + foto<br>**Fair (2):** Foto saja atau diagram saja<br>**Poor (0):** Tidak ada dokumentasi wiring |
| C.3 | Protocol Specification | 3 | **Excellent (3):** Dokumen spesifikasi protokol lengkap: frame format, message types, error codes, state machine diagram<br>**Good (2):** Spesifikasi cukup detail<br>**Fair (1):** Deskripsi protokol minimal<br>**Poor (0):** Tidak ada spesifikasi protokol |
| C.4 | Laporan Teknis | 4 | **Excellent (4):** Analisis mendalam: throughput measurement, error rate calculation, latency analysis, logic analyzer capture, kendala & solusi<br>**Good (3):** Analisis cukup<br>**Fair (2):** Deskriptif saja tanpa analisis kuantitatif<br>**Poor (0):** Tidak ada laporan |

### D. Video Demonstrasi (15 poin)

| No | Aspek | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| D.1 | Kualitas Video | 3 | **Excellent (3):** HD (720p+), stabil, audio narasi jelas, screen recording terbaca<br>**Good (2):** Kualitas cukup, minor issue<br>**Fair (1):** Kualitas rendah tapi masih bisa dilihat dan didengar<br>**Poor (0):** Tidak bisa dilihat/didengar |
| D.2 | Kelengkapan Demo | 6 | **Excellent (6):** Demo lengkap: hardware wiring, serial traffic capture, web dashboard, error handling scenario (cabut kabel), kontrol balik<br>**Good (4-5):** 75% fitur didemonstrasikan<br>**Fair (2-3):** 50% fitur didemonstrasikan<br>**Poor (0-1):** Demo sangat minimal |
| D.3 | Penjelasan Teknis | 4 | **Excellent (4):** Code walkthrough jelas fokus pada protocol handler, ISR callback (`HAL_UART_RxCpltCallback`), buffer management, dan protocol parsing<br>**Good (3):** Penjelasan cukup<br>**Fair (2):** Penjelasan minimal<br>**Poor (0-1):** Tidak ada penjelasan teknis |
| D.4 | Durasi | 2 | **Excellent (2):** Sesuai ketentuan (5-10 menit)<br>**Fair (1):** Sedikit di luar range (±2 menit)<br>**Poor (0):** Jauh dari ketentuan (<3 atau >15 menit) |

### E. Fitur Bonus (10 poin)

| No | Fitur | Poin | Kriteria Penilaian |
|----|-------|------|-------------------|
| E.1 | ACK/Retry Mechanism | 4 | **4:** Retransmission otomatis jika tidak ada ACK dalam timeout, sequence number tracking, max retry limit dengan exponential backoff<br>**2:** Retry sederhana tanpa sequence tracking<br>**0:** Tidak ada retry mechanism |
| E.2 | Auto-Reconnect | 3 | **3:** Sistem auto-detect disconnect (heartbeat timeout), reconnect dan resume tanpa reset manual, LED status indicator<br>**2:** Reconnect manual tapi graceful<br>**0:** Harus power cycle untuk reconnect |
| E.3 | Data Logging to Flash | 3 | **3:** Data log tersimpan di NVS/SPIFFS ESP32, bisa di-retrieve via web API, timestamp akurat<br>**2:** Logging partial atau tanpa retrieval<br>**0:** Tidak ada data logging |

---

## 📈 Konversi Nilai

| Rentang Poin | Huruf | Keterangan |
|--------------|-------|------------|
| 85 - 100 | A | Sangat Baik |
| 80 - 84 | A- | |
| 75 - 79 | B+ | Baik |
| 70 - 74 | B | |
| 65 - 69 | B- | |
| 60 - 64 | C+ | Cukup |
| 55 - 59 | C | |
| 50 - 54 | C- | |
| 40 - 49 | D | Kurang |
| 0 - 39 | E | Tidak Lulus |

---

## ⚠️ Penalti

| Pelanggaran | Penalti |
|-------------|---------|
| Terlambat submit (per hari) | -5 poin |
| Plagiarisme kode (copy-paste tanpa modifikasi signifikan) | -50% total nilai |
| Video tidak bisa diputar | -10 poin |
| Kode tidak dapat di-compile | -20 poin |
| Hardware tidak berfungsi saat demo | -15 poin |
| Menggunakan Arduino framework (bukan ESP-IDF/HAL) | -15 poin |
| Data sensor hardcoded (palsu) | -20 poin |

---

## ✅ Checklist Pengumpulan

### File yang Harus Dikumpulkan:

```
□ Source Code STM32 (folder lengkap dengan platformio.ini, framework = stm32cube)
□ Source Code ESP32 (folder lengkap dengan platformio.ini, framework = espidf)
□ README.md (setup & build instructions)
□ Wiring Diagram (gambar/PDF)
□ Protocol Specification (format dokumen)
□ Laporan Teknis (PDF, max 5 halaman)
□ Video Demonstrasi (MP4/link YouTube, 5-10 menit)
```

### Format Nama File:

```
Kelompok_[XX]_[Nama1]_[Nama2]_UART_Project.zip
Kelompok_[XX]_Video.mp4 atau link YouTube
```

---

## 📝 Form Penilaian

### Kelompok: _______________ | Tanggal: _______________

| Komponen | Poin Maks | Poin | Catatan |
|----------|-----------|------|---------|
| **A. Fungsionalitas** | | | |
| A.1.1 Konfigurasi UART | 4 | | |
| A.1.2 Data TX/RX | 4 | | |
| A.1.3 Protocol Frame | 4 | | |
| A.1.4 Error Detection | 4 | | |
| A.2.1 Sensor Reading | 4 | | |
| A.2.2 Non-blocking | 4 | | |
| A.2.3 Command RX | 4 | | |
| A.3.1 Buffer Management | 4 | | |
| A.3.2 Data Parsing | 4 | | |
| A.3.3 Web Dashboard | 4 | | |
| **Subtotal A** | **40** | | |
| **B. Kualitas Kode** | | | |
| B.1 Struktur | 6 | | |
| B.2 Naming | 4 | | |
| B.3 Comments | 4 | | |
| B.4 Error Handling | 3 | | |
| B.5 API Native | 3 | | |
| **Subtotal B** | **20** | | |
| **C. Dokumentasi** | | | |
| C.1 README | 4 | | |
| C.2 Wiring Diagram | 4 | | |
| C.3 Protocol Spec | 3 | | |
| C.4 Laporan | 4 | | |
| **Subtotal C** | **15** | | |
| **D. Video** | | | |
| D.1 Kualitas | 3 | | |
| D.2 Kelengkapan | 6 | | |
| D.3 Penjelasan | 4 | | |
| D.4 Durasi | 2 | | |
| **Subtotal D** | **15** | | |
| **E. Bonus** | | | |
| E.1 ACK/Retry | 4 | | |
| E.2 Auto-Reconnect | 3 | | |
| E.3 Data Logging | 3 | | |
| **Subtotal E** | **10** | | |
| **TOTAL** | **100** | | |
| **Penalti** | | | |
| **NILAI AKHIR** | | | |

### Tanda Tangan Penilai:

___________________________ | Tanggal: _______________

---

*Rubrik Penilaian Project Modul 03 - Praktikum Sistem Embedded*
*Versi 1.0 - Februari 2026*
