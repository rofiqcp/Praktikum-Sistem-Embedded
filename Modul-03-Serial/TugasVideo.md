# Tugas Video — Modul 03: Komunikasi Serial UART

## Informasi Tugas

| Item | Detail |
|------|--------|
| Modul | 03 — Komunikasi Serial UART |
| Platform | STM32F103C8T6 (Blue Pill) & ESP32 DevKit V1 |
| Format | Video Presentasi + Demonstrasi Hardware |
| Durasi | 15–25 menit |
| Deadline | Sesuai jadwal di e-learning |
| Upload | YouTube (Unlisted) → link dikumpulkan di e-learning |

---

## Deskripsi Tugas

Video ini merupakan **laporan lengkap** dari seluruh kegiatan praktikum dan project Modul 03. Mahasiswa mempresentasikan **pemahaman materi**, **seluruh percobaan** (Percobaan 1–12 untuk masing-masing platform), dan **project akhir** dalam satu video utuh yang mencakup penjelasan teori, demonstrasi kode, dan demonstrasi hardware.

---

## Ketentuan Teknis Video

### Format Rekaman
- **Screen recording** layar VS Code / PlatformIO saat menjelaskan kode dan menjalankan program
- **Webcam** wajib terlihat (picture-in-picture) selama presentasi
- **Hardware recording** menggunakan kamera HP / webcam terpisah saat mendemonstrasikan rangkaian fisik

### Spesifikasi Teknis

| Parameter | Ketentuan |
|-----------|-----------|
| Resolusi | Minimal 720p (1280×720) |
| Audio | Jelas, tidak bising |
| Webcam | Wajah terlihat jelas |
| Screen Recording | Teks kode dan Serial Monitor terbaca jelas |
| Hardware Demo | Komponen dan koneksi terlihat jelas |

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan: nama, NIM, kelas, modul
- Gambaran umum modul: Komunikasi Serial UART pada embedded system

### 2. Ringkasan Materi (2–3 menit)
- Frame data UART: start bit, data bits, parity, stop bit
- Konfigurasi 8N1 dan baud rate
- Perbedaan polling vs interrupt vs DMA
- UART pada ESP32 (3 port, pin remappable) vs STM32 (3 USART, fixed alternate function)
- Konsep ring buffer, framing, CRC, timeout parsing

### 3. Demonstrasi Seluruh Percobaan (8–14 menit)
Untuk **setiap percobaan** (P01–P12, kedua platform):

1. Sebutkan judul dan tujuan percobaan
2. Tampilkan kode (`main.c`) — jelaskan bagian inti
3. Tunjukkan hasil di Serial Monitor
4. Demo hardware jika ada (LED, buzzer)
5. Analisa singkat

| No | Percobaan | Poin Penting Demo |
|----|-----------|-------------------|
| P01 | UART Echo (Polling) | Ketik karakter → echo kembali, tunjukkan delay |
| P02 | Interrupt RX | Bandingkan responsivitas dengan P01 |
| P03 | Ring Buffer | Tunjukkan statistik buffer, uji overflow |
| P04 | Printf Redirect | Tabel sensor periodik, perintah LED |
| P05 | Command Parser | Demo semua perintah: LED, BEEP, STATUS |
| P06 | JSON Protocol | Kirim JSON → respons JSON, uji invalid input |
| P07 | Line Editor | Arrow up/down history, backspace |
| P08 | STX/ETX Framing | Hex dump frame, byte stuffing verification |
| P09 | CRC-8 Checksum | Self-test, data korup → CRC fail |
| P10 | Timeout Parser | Paket terdeteksi berdasarkan jeda waktu |
| P11 | Multi-UART Bridge | Dua Serial Monitor, data forwarding bidirectional |
| P12 | Error Statistics | Trigger parity error, laporan health status |

### 4. Demonstrasi Project (3–5 menit)
- Jelaskan skenario Monitoring Lab Kimia
- Tunjukkan koneksi hardware STM32 ↔ ESP32
- Demo: node mengirim data sensor → gateway menerima, validasi CRC, display JSON
- Demo: operator mengetik perintah via line editor → node merespons
- Demo: threshold exceeded → alarm aktif
- Tunjukkan error monitoring per node

### 5. Penutup (1–2 menit)
- Kesimpulan pembelajaran dari seluruh praktikum
- Perbandingan UART pada STM32 vs ESP32
- Kesulitan dan solusi

---

## Komponen Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | Pemahaman materi teori | 15% |
| 2 | Demonstrasi percobaan (kelengkapan + penjelasan) | 35% |
| 3 | Demonstrasi project | 20% |
| 4 | Demo hardware (rangkaian fisik berfungsi) | 15% |
| 5 | Kualitas video dan presentasi | 15% |

---

## Rubrik Penilaian

### Pemahaman Materi (15%)
| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Menjelaskan frame UART, polling vs interrupt, framing, CRC dengan benar dan mendalam |
| B (75–89) | Sebagian besar konsep benar, ada kekurangan minor |
| C (60–74) | Hanya konsep dasar, kurang mendalam |
| D (< 60) | Penjelasan salah atau sangat minim |

### Demonstrasi Percobaan (35%)
| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Semua 12 percobaan ditampilkan untuk kedua platform; kode dijelaskan; hasil ditunjukkan |
| B (75–89) | Minimal 10 percobaan; penjelasan cukup baik |
| C (60–74) | Minimal 8 percobaan; penjelasan kurang mendalam |
| D (< 60) | Kurang dari 8 percobaan |

### Demonstrasi Project (20%)
| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Project berfungsi lengkap; dual-MCU terkoordinasi; semua fitur bekerja |
| B (75–89) | Fitur utama berfungsi; ada kekurangan minor |
| C (60–74) | Project dasar berfungsi; beberapa fitur tidak bekerja |
| D (< 60) | Project tidak berfungsi |

### Demo Hardware (15%)
| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Rangkaian rapi; semua komponen terlihat; demonstrasi jelas |
| B (75–89) | Rangkaian berfungsi; cukup jelas |
| C (60–74) | Hardware terlihat tapi kurang jelas |
| D (< 60) | Tidak ada demo hardware |

### Kualitas Video (15%)
| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Video jernih ≥720p; audio jelas; webcam terlihat; editing rapi |
| B (75–89) | Kualitas cukup baik; kekurangan minor |
| C (60–74) | Kualitas rendah tapi bisa dipahami |
| D (< 60) | Video tidak jelas / audio buruk / webcam tidak terlihat |

---

## Penalti

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −20% dari total |
| Tidak ada demo hardware | −20% dari total |
| Durasi < 10 menit | −10% dari total |
| Durasi > 30 menit | −5% dari total |
| Terlambat submit | −10% per hari |

---

## Checklist Sebelum Submit

- [ ] Video di-upload ke YouTube (Unlisted)
- [ ] Durasi 15–25 menit
- [ ] Webcam terlihat sepanjang presentasi
- [ ] Screen recording terbaca jelas
- [ ] Seluruh 12 percobaan didemonstrasikan (kedua platform)
- [ ] Project didemonstrasikan lengkap
- [ ] Ada demo hardware fisik
- [ ] Audio jelas
- [ ] Link YouTube dikumpulkan di e-learning
