# Rubrik Penilaian Tugas Video
## Modul 03: Serial UART Communication

---

## 📋 Informasi Tugas

| Item | Keterangan |
|------|------------|
| **Jenis Tugas** | Video Pembelajaran/Tutorial |
| **Durasi Video** | 5-8 menit |
| **Format** | MP4 (H.264) atau link YouTube |
| **Resolusi Minimum** | 720p (1280x720) |
| **Penilaian** | Individu |
| **Total Skor** | 100 poin |
| **Framework** | STM32Cube HAL dan ESP-IDF (bukan Arduino) |

---

## 🎯 Tujuan Tugas Video

Mahasiswa membuat video tutorial yang menjelaskan salah satu konsep atau program dari praktikum Serial UART Communication. Video harus mampu mengajarkan konsep tersebut kepada mahasiswa lain yang belum memahami materi. Semua kode harus menggunakan framework ESP-IDF (ESP32) dan STM32Cube HAL (STM32).

---

## 📝 Pilihan Topik Video

Pilih SATU dari topik berikut:

| No | Topik | Program Terkait | Tingkat |
|----|-------|-----------------|---------|
| 1 | UART Echo dan Konfigurasi Dasar | ESP32_01_UART_Echo / STM32_01_UART_Echo | ⭐ |
| 2 | UART Interrupt RX dengan Callback | ESP32_02_UART_Interrupt_RX / STM32_02_UART_Interrupt_RX | ⭐⭐ |
| 3 | Ring Buffer untuk UART | ESP32_03_UART_Ring_Buffer / STM32_03_UART_Ring_Buffer | ⭐⭐⭐ |
| 4 | Printf Redirect via UART | ESP32_04_UART_Printf_Redirect / STM32_04_UART_Printf_Redirect | ⭐⭐ |
| 5 | Command Parser Serial | ESP32_05_UART_Command_Parser / STM32_05_UART_Command_Parser | ⭐⭐ |
| 6 | JSON Protocol via UART | ESP32_06_UART_JSON_Protocol / STM32_06_UART_JSON_Protocol | ⭐⭐⭐ |
| 7 | Line Editor Implementation | ESP32_07_UART_Line_Editor / STM32_07_UART_Line_Editor | ⭐⭐ |
| 8 | Framing Protocol STX/ETX | ESP32_08_UART_Framing_STX_ETX / STM32_08_UART_Framing_STX_ETX | ⭐⭐⭐ |
| 9 | CRC Checksum Validation | ESP32_09_UART_CRC_Checksum / STM32_09_UART_CRC_Checksum | ⭐⭐⭐ |
| 10 | Timeout Parser UART | ESP32_10_UART_Timeout_Parser / STM32_10_UART_Timeout_Parser | ⭐⭐ |
| 11 | Multi-UART Bridge | ESP32_11_UART_Bridge_Multi / STM32_11_UART_Bridge_Multi | ⭐⭐⭐ |
| 12 | Error Statistics & Monitoring | ESP32_12_UART_Error_Statistics / STM32_12_UART_Error_Statistics | ⭐⭐⭐ |

---

## 📊 Komponen Penilaian

| Komponen | Bobot | Poin Maksimum |
|----------|-------|---------------|
| Konten & Kebenaran Materi | 40% | 40 poin |
| Teknis Video | 25% | 25 poin |
| Penyampaian & Komunikasi | 25% | 25 poin |
| Kreativitas & Originalitas | 10% | 10 poin |
| **Total** | **100%** | **100 poin** |

---

## 📝 Rubrik Detail

### A. Konten dan Kebenaran Materi (40 poin)

| Aspek | Poin | Kriteria |
|-------|------|----------|
| **Keakuratan Teknis** | 0-15 | **Excellent (13-15):** Semua penjelasan teknis 100% akurat, API ESP-IDF (`uart_driver_install`, `uart_read_bytes`, `uart_write_bytes`) dan STM32 HAL (`HAL_UART_Transmit`, `HAL_UART_Receive_IT`, `HAL_UART_RxCpltCallback`) dijelaskan dengan benar<br>**Good (10-12):** Minor inaccuracy (1-2 kesalahan kecil, misal salah parameter)<br>**Fair (6-9):** Beberapa kesalahan tidak fatal, konsep inti benar<br>**Poor (3-5):** Ada kesalahan konsep penting (misal salah jelaskan baud rate, frame format)<br>**Very Poor (0-2):** Banyak kesalahan atau menggunakan Arduino API bukan ESP-IDF/HAL |
| **Kelengkapan Materi** | 0-10 | **Excellent (9-10):** Mencakup semua aspek penting: teori UART, konfigurasi parameter, implementasi kode ESP-IDF/HAL, testing<br>**Good (7-8):** Sebagian besar tercakup, 1 aspek minor terlewat<br>**Fair (4-6):** Hanya basics, tidak menjelaskan API detail<br>**Poor (0-3):** Tidak lengkap, banyak aspek terlewat |
| **Demonstrasi Praktis** | 0-10 | **Excellent (9-10):** Demo hardware jelas, kode berjalan real-time, output Serial Monitor terlihat, sinyal UART terverifikasi<br>**Good (6-8):** Demo ada tapi kurang detail (misal tidak zoom ke wiring)<br>**Fair (3-5):** Demo minimal, hanya screenshot<br>**Poor (0-2):** Tidak ada demo praktis |
| **Penjelasan Kode** | 0-5 | **Excellent (5):** Walkthrough kode terstruktur, menjelaskan fungsi-fungsi kunci (uart_config_t, HAL_UART_Init, callback), flow data dari TX ke RX<br>**Good (3-4):** Penjelasan umum tanpa detail register/config<br>**Fair (1-2):** Hanya membaca kode tanpa penjelasan logic<br>**Poor (0):** Tidak menjelaskan kode |

### B. Teknis Video (25 poin)

| Aspek | Poin | Kriteria |
|-------|------|----------|
| **Kualitas Visual** | 0-8 | **Excellent (7-8):** HD (720p+), sharp, well-lit, screen recording terbaca jelas (font ≥14pt), zoom pada bagian penting<br>**Good (5-6):** Good quality, minor focus issues<br>**Fair (3-4):** Acceptable quality, text agak kecil tapi masih terbaca<br>**Poor (1-2):** Low quality, text sulit dibaca<br>**Very Poor (0):** Tidak bisa dilihat |
| **Kualitas Audio** | 0-8 | **Excellent (7-8):** Jernih, tidak ada background noise, volume konsisten, menggunakan microphone<br>**Good (5-6):** Baik dengan minor noise<br>**Fair (3-4):** Dapat didengar dengan effort<br>**Poor (1-2):** Sulit didengar, banyak noise<br>**Very Poor (0):** Tidak bisa didengar |
| **Durasi** | 0-5 | **Excellent (5):** Sesuai ketentuan (5-8 menit), pacing tepat<br>**Good (3-4):** Sedikit di luar range (4-9 menit)<br>**Fair (1-2):** Agak jauh dari range (3-10 menit)<br>**Poor (0):** <2 atau >12 menit |
| **Editing** | 0-4 | **Excellent (4):** Smooth transitions, good pacing, zoom pada code, overlay text informatif<br>**Good (2-3):** Basic editing, cut dead air<br>**Fair (1):** Minimal editing<br>**Poor (0):** No editing, banyak dead air/silence |

### C. Penyampaian dan Komunikasi (25 poin)

| Aspek | Poin | Kriteria |
|-------|------|----------|
| **Kejelasan Penjelasan** | 0-10 | **Excellent (9-10):** Sangat jelas, mudah diikuti, logical flow dari teori → konfigurasi → implementasi → testing<br>**Good (6-8):** Jelas dengan beberapa bagian yang kurang smooth<br>**Fair (3-5):** Cukup jelas tapi membingungkan di beberapa bagian<br>**Poor (0-2):** Sulit dimengerti |
| **Struktur Presentasi** | 0-8 | **Excellent (7-8):** Intro (topik & tujuan) → Teori UART → Implementasi kode → Demo → Tips → Summary, flow logis<br>**Good (5-6):** Struktur cukup baik, minor flow issue<br>**Fair (3-4):** Ada struktur tapi tidak konsisten<br>**Poor (0-2):** Tidak terstruktur, loncat-loncat |
| **Penggunaan Visual Aids** | 0-7 | **Excellent (6-7):** Diagram timing UART, wiring diagram, code highlighting, annotations pada screenshot, overlay text penjelasan<br>**Good (4-5):** Beberapa visual aids efektif<br>**Fair (2-3):** Minimal visual aids<br>**Poor (0-1):** Tidak ada visual aids |

### D. Kreativitas dan Originalitas (10 poin)

| Aspek | Poin | Kriteria |
|-------|------|----------|
| **Pendekatan Unik** | 0-5 | **Excellent (5):** Analogi kreatif (misal UART = percakapan telepon), animasi timing diagram, pendekatan teaching yang unik dan engaging<br>**Good (3-4):** Beberapa elemen kreatif<br>**Fair (1-2):** Standar/template-based<br>**Poor (0):** Copy dari tutorial online |
| **Contoh/Aplikasi Real-world** | 0-5 | **Excellent (5):** Contoh aplikasi nyata UART (GPS NMEA, Bluetooth AT commands, sensor industrial), perbandingan STM32 vs ESP32 UART<br>**Good (3-4):** Beberapa contoh relevan<br>**Fair (1-2):** Contoh generic<br>**Poor (0):** Tidak ada contoh aplikasi |

---

## 📋 Struktur Video yang Direkomendasikan

```
1. PEMBUKAAN (30-45 detik)
   - Salam dan perkenalan (Nama, NIM)
   - Topik: "[Nama Program] - Serial UART"
   - Relevansi: Mengapa UART penting di embedded systems

2. TEORI/KONSEP (1-2 menit)
   - Penjelasan konsep UART terkait topik
   - Diagram frame format / timing diagram
   - Parameter: baud rate, data bits, parity, stop bits

3. IMPLEMENTASI (2-3 menit)
   - Tunjukkan hardware setup (STM32/ESP32 wiring)
   - Walkthrough kode ESP-IDF atau STM32 HAL:
     * ESP32: uart_config_t, uart_driver_install(), uart_read_bytes()
     * STM32: HAL_UART_Init(), HAL_UART_Transmit(), HAL_UART_RxCpltCallback()
   - Highlight bagian penting (konfigurasi, ISR, buffer)

4. DEMONSTRASI (1-2 menit)
   - Live demo: upload dan run program
   - Serial Monitor output
   - Interaksi: kirim data, lihat response

5. TIPS & TROUBLESHOOTING (30-60 detik)
   - Common mistakes (baud rate mismatch, TX-RX terbalik)
   - Debugging tips (logic analyzer, loopback test)
   - Best practices

6. PENUTUP (30 detik)
   - Summary key points
   - Ajakan untuk praktek mandiri
```

---

## ⚠️ Penalti

| Pelanggaran | Penalti |
|-------------|---------|
| Terlambat submit (per hari) | -5 poin |
| Video tidak bisa diputar | Tidak dinilai sampai diperbaiki |
| Plagiarisme (copy video orang lain) | -100% (nilai 0) |
| Menggunakan Arduino API (bukan ESP-IDF/HAL) | -15 poin |
| Konten menyesatkan (dangerous wiring, salah voltage) | -20 poin |
| Audio/Video completely unusable | -30 poin |
| Durasi < 3 menit atau > 10 menit | -10 poin |

---

## ✅ Checklist Sebelum Submit

```
□ Video dapat diputar (test di device lain)
□ Audio jelas terdengar
□ Screen recording terbaca (font cukup besar, ≥14pt)
□ Durasi 5-8 menit
□ Ada intro (nama, NIM, topik) dan outro (summary)
□ Kode yang ditunjukkan menggunakan ESP-IDF atau STM32 HAL (bukan Arduino)
□ Demo hardware terlihat jelas (wiring, LED, serial output)
□ Nama dan NIM disebutkan di video
□ Link video accessible (YouTube unlisted/public atau Google Drive open)
```

---

## 📱 Tips Teknis Pembuatan Video

### Recording Tools (Gratis)
| Tool | Platform | Fungsi |
|------|----------|--------|
| OBS Studio | Win/Mac/Linux | Screen recording + webcam |
| Loom | Web/Desktop | Simple screen recording |
| Zoom | All | Recording dengan share screen |
| DaVinci Resolve | Win/Mac | Free video editing |
| Kdenlive | Linux | Free video editing |

### Tips Recording
1. **Screen recording:** Resolusi 1920x1080, font IDE minimal 14pt, zoom pada kode penting
2. **Webcam:** Posisi di corner saat intro/outro, tidak menghalangi kode
3. **Audio:** Gunakan microphone external (headset mic minimal), rekam di ruangan tenang
4. **Lighting:** Pastikan wajah terlihat jelas saat intro
5. **Hardware demo:** Close-up pada wiring STM32↔ESP32, tunjukkan TX-RX cross connection

### Tips Editing
1. Cut dead air, loading time, dan kesalahan
2. Add zoom-in pada kode penting (`uart_driver_install`, `HAL_UART_Init`)
3. Add text overlay untuk key points (baud rate, pin assignment)
4. Add timing diagram overlay saat menjelaskan frame UART
5. Background music optional (volume sangat rendah)

---

## 📈 Konversi Nilai

| Rentang Poin | Huruf | Keterangan |
|--------------|-------|------------|
| 85 - 100 | A | Sangat Baik - Video berkualitas tinggi |
| 80 - 84 | A- | |
| 75 - 79 | B+ | Baik - Video informatif |
| 70 - 74 | B | |
| 65 - 69 | B- | |
| 60 - 64 | C+ | Cukup - Memenuhi minimum |
| 55 - 59 | C | |
| 50 - 54 | C- | |
| 40 - 49 | D | Kurang - Perlu perbaikan signifikan |
| 0 - 39 | E | Tidak Lulus |

---

## 📝 Form Penilaian Video

### Mahasiswa: _______________ | NIM: _______________ | Topik: _______________

| Komponen | Poin Maks | Poin | Catatan |
|----------|-----------|------|---------|
| **A. Konten & Kebenaran** | | | |
| A.1 Keakuratan Teknis | 15 | | |
| A.2 Kelengkapan Materi | 10 | | |
| A.3 Demonstrasi Praktis | 10 | | |
| A.4 Penjelasan Kode | 5 | | |
| **Subtotal A** | **40** | | |
| **B. Teknis Video** | | | |
| B.1 Kualitas Visual | 8 | | |
| B.2 Kualitas Audio | 8 | | |
| B.3 Durasi | 5 | | |
| B.4 Editing | 4 | | |
| **Subtotal B** | **25** | | |
| **C. Penyampaian** | | | |
| C.1 Kejelasan | 10 | | |
| C.2 Struktur | 8 | | |
| C.3 Visual Aids | 7 | | |
| **Subtotal C** | **25** | | |
| **D. Kreativitas** | | | |
| D.1 Pendekatan Unik | 5 | | |
| D.2 Contoh Real-world | 5 | | |
| **Subtotal D** | **10** | | |
| **TOTAL** | **100** | | |
| **Penalti** | | | |
| **NILAI AKHIR** | | | |

### Komentar/Feedback:

_______________________________________________________________

_______________________________________________________________

_______________________________________________________________

### Tanda Tangan Penilai: _______________ | Tanggal: _______________

---

*Rubrik Penilaian Video Modul 03 - Praktikum Sistem Embedded*
*Versi 1.0 - Februari 2026*
