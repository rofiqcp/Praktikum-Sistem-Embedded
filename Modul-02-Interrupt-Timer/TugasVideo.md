# Tugas Video — Modul 02: Interrupt & Timer

## Informasi Tugas

| Item | Detail |
|------|--------|
| Modul | 02 — Interrupt & Timer |
| Platform | STM32F103C8T6 (Blue Pill) & ESP32 DevKit V1 |
| Format | Video Presentasi + Demonstrasi Hardware |
| Durasi | 15–25 menit |
| Deadline | Sesuai jadwal di e-learning |
| Upload | YouTube (Unlisted) → link dikumpulkan di e-learning |

---

## Deskripsi Tugas

Video ini merupakan **laporan lengkap** dari seluruh kegiatan praktikum dan project Modul 02. Mahasiswa mempresentasikan **pemahaman materi**, **seluruh percobaan** (Percobaan 1–12 untuk masing-masing platform), dan **project akhir** dalam satu video utuh yang mencakup penjelasan teori, demonstrasi kode, dan demonstrasi hardware.

---

## Ketentuan Teknis Video

### Format Rekaman
- **Screen recording** layar VS Code / PlatformIO saat menjelaskan kode dan menjalankan program
- **Webcam** wajib terlihat (picture-in-picture) selama presentasi untuk membuktikan yang menjelaskan adalah mahasiswa bersangkutan
- **Hardware recording** menggunakan kamera HP / webcam terpisah saat mendemonstrasikan rangkaian fisik (LED, tombol sensor, buzzer, dsb.)

### Spesifikasi Teknis
| Parameter | Ketentuan |
|-----------|-----------|
| Resolusi | Minimal 720p (1280×720) |
| Audio | Jelas, tidak bising, gunakan mikrofon jika perlu |
| Webcam | Wajah terlihat jelas, pencahayaan cukup |
| Screen Recording | Teks kode terbaca jelas (zoom jika perlu) |
| Hardware Demo | Komponen dan LED terlihat jelas |

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
- Perkenalan: nama, NIM, kelas, modul
- Gambaran umum modul: Interrupt & Timer pada sistem embedded
- Sebutkan jumlah percobaan dan project yang akan ditampilkan

### 2. Ringkasan Materi (2–3 menit)
Jelaskan konsep kunci dengan singkat dan padat:
- Konsep interrupt: apa itu ISR, vector table, NVIC pada STM32, interrupt matrix pada ESP32
- External interrupt (EXTI): konfigurasi pin, trigger mode (rising, falling, both)
- Nested interrupt dan priority grouping
- Timer architecture: prescaler, auto-reload, counter mode
- Timer modes: basic, PWM, input capture, output compare
- Watchdog timer: IWDG dan WWDG
- Perbedaan arsitektur interrupt dan timer antara STM32 vs ESP32

### 3. Demonstrasi Seluruh Percobaan (8–14 menit)
Untuk **setiap percobaan** (P01–P12, kedua platform):

1. **Sebutkan** judul dan tujuan percobaan
2. **Tampilkan kode** (`main.c`) di layar — jelaskan bagian penting (konfigurasi interrupt/timer, ISR, handler)
3. **Tunjukkan hasil** di Serial Monitor / terminal
4. **Demo hardware** — rekam rangkaian fisik yang bekerja (LED berkedip, tombol ditekan, buzzer berbunyi)
5. **Analisa singkat** — apa yang terjadi dan mengapa

**Daftar Percobaan yang Wajib Ditampilkan:**

| No | Percobaan | Poin Penting |
|----|-----------|-------------|
| P01 | External Interrupt Basic | Tombol trigger ISR → LED toggle, jelaskan flow interupsi |
| P02 | Interrupt Edge Detection | Perbedaan rising vs falling vs both, tampilkan sinyal berbeda |
| P03 | Multi Interrupt Priority | Nested interrupt, tekan 2 tombol bersamaan → prioritas |
| P04 | Debounce Interrupt | Bandingkan dengan/tanpa debounce, tunjukkan bouncing di serial |
| P05 | Basic Timer | Timer interrupt periodik, LED blink timing presisi |
| P06 | Timer Periodic Task | Banyak task dijadwalkan timer, tunjukkan timing konsisten |
| P07 | Timer PWM Output | Sinyal PWM untuk LED brightness, ukur duty cycle |
| P08 | Timer Input Capture | Ukur frekuensi/lebar pulsa sinyal eksternal |
| P09 | Watchdog Timer | System recovery setelah hang, demonstrasi reset |
| P10 | Timer One-Shot | Single-shot delay, hitung mundur |
| P11 | Timer Cascade | Timer chain untuk periode panjang, tunjukkan overflow |
| P12 | Comprehensive Interrupt+Timer | Gabungan interrupt+timer dalam satu sistem |

> **Tips:** Tidak perlu menjelaskan setiap baris kode. Fokus pada bagian konfigurasi interrupt, handler ISR, dan pengaturan timer. Untuk percobaan yang mirip, boleh menjelaskan lebih singkat dan menyebutkan perbedaannya saja.

### 4. Demonstrasi Project (3–5 menit)
- Jelaskan skenario soal cerita project (Sistem Monitoring Keamanan Gudang)
- Tunjukkan arsitektur sistem dan koneksi hardware
- Demo kode: jelaskan bagian utama (interrupt priority, timer timestamp, komunikasi)
- **Demo hardware lengkap**: tunjukkan setiap sensor dan aktuator bekerja
- Tunjukkan skenario pengujian: trigger sensor → respons real-time → log data
- Tampilkan response time measurement

### 5. Penutup (1–2 menit)
- Rangkuman: apa yang dipelajari dari seluruh praktikum
- Kesulitan yang dihadapi dan cara mengatasinya
- Kesimpulan perbandingan STM32 vs ESP32 dalam hal interrupt dan timer
- Saran perbaikan

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

## Rubrik Penilaian Detail

### 1. Pemahaman Materi (15%)

| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Menjelaskan konsep interrupt, NVIC priority, timer architecture, watchdog dengan benar dan mendalam; mampu membandingkan STM32 vs ESP32 |
| B (75–89) | Menjelaskan sebagian besar konsep dengan benar, ada sedikit kekurangan |
| C (60–74) | Hanya menjelaskan konsep dasar, kurang mendalam |
| D (< 60) | Penjelasan salah atau sangat minim |

### 2. Demonstrasi Percobaan (35%)

| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Semua 12 percobaan ditampilkan untuk kedua platform; kode dijelaskan; hasil ditunjukkan; analisa tepat |
| B (75–89) | Minimal 10 percobaan ditampilkan; penjelasan cukup baik |
| C (60–74) | Minimal 8 percobaan; penjelasan kurang mendalam |
| D (< 60) | Kurang dari 8 percobaan atau hanya menampilkan tanpa penjelasan |

### 3. Demonstrasi Project (20%)

| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Project berfungsi lengkap; dual-MCU bekerja terkoordinasi; interrupt priority terbukti benar; timing measurement akurat |
| B (75–89) | Project berfungsi dengan fitur utama; ada sedikit kekurangan |
| C (60–74) | Project dasar berfungsi; beberapa fitur tidak bekerja |
| D (< 60) | Project tidak berfungsi atau tidak dikerjakan |

### 4. Demo Hardware (15%)

| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Rangkaian rapi dan mudah dipahami; semua komponen terlihat; demonstrasi jelas untuk setiap percobaan dan project |
| B (75–89) | Rangkaian terlihat dan berfungsi; demonstrasi cukup jelas |
| C (60–74) | Hardware terlihat tapi kurang jelas; beberapa demo tidak ditunjukkan |
| D (< 60) | Tidak ada demo hardware atau rangkaian tidak berfungsi |

### 5. Kualitas Video (15%)

| Nilai | Kriteria |
|-------|----------|
| A (90–100) | Video jernih (≥720p); audio jelas; webcam terlihat sepanjang video; editing rapi; transisi smooth |
| B (75–89) | Kualitas cukup baik; ada kekurangan minor |
| C (60–74) | Kualitas rendah tapi masih bisa dipahami |
| D (< 60) | Video tidak jelas / audio buruk / webcam tidak terlihat |

---

## Penalti

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −20% dari total |
| Tidak ada demo hardware | −20% dari total |
| Durasi < 10 menit | −10% dari total |
| Durasi > 30 menit | −5% dari total |
| Video bukan unlisted YouTube | −5% dari total |
| Terlambat submit | −10% per hari |

---

## Checklist Sebelum Submit

- [ ] Video sudah di-upload ke YouTube (Unlisted)
- [ ] Durasi 15–25 menit
- [ ] Webcam terlihat sepanjang presentasi
- [ ] Screen recording kode terbaca jelas
- [ ] Seluruh 12 percobaan didemonstrasikan (kedua platform)
- [ ] Project didemonstrasikan lengkap
- [ ] Ada demo hardware fisik
- [ ] Audio jelas dan tidak bising
- [ ] Link YouTube sudah dikumpulkan di e-learning

---

## Tips Pengerjaan

1. **Siapkan script** — tulis poin-poin yang akan disampaikan agar tidak lupa
2. **Rekam per bagian** — rekam pembukaan, materi, percobaan, project, penutup secara terpisah lalu gabungkan
3. **Gunakan OBS Studio** — gratis, mendukung screen recording + webcam overlay
4. **Demo hardware terpisah** — rekam hardware demo dengan kamera HP terpisah dari screen recording
5. **Zoom in pada kode** — pastikan teks kode di editor terbaca jelas di resolusi video
6. **Cek audio** — rekam sampel pendek dulu, dengarkan, pastikan sudah oke
7. **Percobaan yang mirip boleh dipersingkat** — cukup jelaskan perbedaannya saja
