# 🎯 Project Modul 05: Sistem Audio Player dan LED Controller Berbasis DAC & PWM

## 📋 Deskripsi Project

Mahasiswa merancang dan mengimplementasikan **Sistem Audio Player dan LED Controller** yang mengintegrasikan DAC untuk menghasilkan sinyal audio (tone/melodi) dan PWM untuk mengendalikan LED RGB, servo motor, serta buzzer. Sistem memiliki antarmuka serial untuk pemilihan mode operasi dan menggabungkan input analog (potensiometer via ADC) untuk kontrol real-time.

## 🎯 Tujuan Pembelajaran

1. Menerapkan DAC untuk menghasilkan gelombang analog (sine, triangle, sawtooth)
2. Mengimplementasikan PWM untuk kontrol LED RGB dengan efek warna
3. Mengendalikan servo motor SG90 menggunakan sinyal PWM
4. Mengintegrasikan input ADC (potensiometer) untuk kontrol parameter real-time
5. Membangun sistem multi-mode dengan antarmuka serial interaktif
6. Memahami perbedaan DAC dan PWM serta kapan menggunakan masing-masing

## 🔧 Kebutuhan Hardware

### Komponen Utama

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 / STM32F411 BlackPill | 1 | Mikrokontroler utama |
| 2 | Speaker 8Ω 0.5W / Modul Amplifier PAM8403 | 1 | Output audio dari DAC |
| 3 | LED RGB Common Cathode | 1 | Kontrol warna dengan PWM |
| 4 | Servo Motor SG90 | 1 | Motor posisi dengan PWM |
| 5 | Potensiometer 10kΩ | 1 | Input analog (ADC) untuk kontrol |
| 6 | Passive Buzzer | 1 | Output tone/melodi dengan PWM |
| 7 | Resistor 220Ω | 3 | Pembatas arus LED RGB (R, G, B) |
| 8 | Resistor 100Ω | 1 | Pembatas arus speaker (opsional) |
| 9 | Kapasitor 10µF | 1 | Decoupling output DAC ke speaker |
| 10 | Breadboard | 1 | Papan rangkaian |
| 11 | Kabel Jumper | Secukupnya | Male-to-male dan male-to-female |
| 12 | Kabel USB | 1 | Koneksi ke komputer |

### Komponen Tambahan (Opsional)

| No | Komponen | Keterangan |
|----|----------|------------|
| 1 | LCD I2C 16x2 | Tampilan mode dan informasi |
| 2 | Push Button | 3-4 buah untuk kontrol mode tanpa serial |
| 3 | Modul Amplifier PAM8403 | Amplifier audio jika speaker langsung terlalu pelan |

## ⚠️ Catatan Penting tentang Ketersediaan DAC

| Mikrokontroler | DAC Tersedia? | Keterangan |
|----------------|:-------------:|------------|
| **ESP32 (WROOM)** | ✅ Ya | 2 channel DAC 8-bit (GPIO25, GPIO26) |
| **ESP32-S2** | ❌ Tidak | Tidak memiliki peripheral DAC |
| **ESP32-S3** | ❌ Tidak | Tidak memiliki peripheral DAC |
| **ESP32-C3** | ❌ Tidak | Tidak memiliki peripheral DAC |
| **STM32F411** | ❌ Tidak | Tidak memiliki peripheral DAC |
| **STM32F103** | ❌ Tidak | Seri ini tidak memiliki DAC |
| **STM32F407** | ✅ Ya | 2 channel DAC 12-bit (PA4, PA5) |

> **Strategi:** Untuk mikrokontroler tanpa DAC, gunakan **PWM + Low-Pass Filter** (RC filter) sebagai pengganti DAC. Frekuensi PWM tinggi (≥40 kHz) dengan RC filter (R=1kΩ, C=100nF) menghasilkan tegangan analog yang cukup baik untuk audio sederhana.

## 📌 Konfigurasi Pin

### ESP32 DevKit V1

| Fungsi | Pin ESP32 | Keterangan |
|--------|-----------|------------|
| DAC Output 1 (Speaker) | GPIO 25 (DAC1) | Output analog 8-bit |
| DAC Output 2 (Cadangan) | GPIO 26 (DAC2) | Output analog 8-bit |
| PWM LED Merah | GPIO 16 | LEDC Channel 0 |
| PWM LED Hijau | GPIO 17 | LEDC Channel 1 |
| PWM LED Biru | GPIO 18 | LEDC Channel 2 |
| PWM Servo | GPIO 19 | LEDC Channel 3 (50Hz) |
| PWM Buzzer | GPIO 23 | LEDC Channel 4 |
| ADC Potensiometer | GPIO 34 (ADC1_CH6) | Input analog, kontrol parameter |

> **Catatan ESP32:** DAC ESP32 hanya 8-bit (0-255) dengan output 0-3.3V. Untuk audio, gunakan kapasitor kopling (10µF) antara pin DAC dan speaker untuk menghilangkan komponen DC. LEDC mendukung hingga 16 channel PWM independen.

### STM32F411 BlackPill (Tanpa DAC)

| Fungsi | Pin STM32 | Timer/Channel | Keterangan |
|--------|-----------|---------------|------------|
| PWM Speaker (pengganti DAC) | PA8 | TIM1_CH1 | PWM tinggi + RC filter |
| PWM LED Merah | PA6 | TIM3_CH1 | PWM 1 kHz |
| PWM LED Hijau | PA7 | TIM3_CH2 | PWM 1 kHz |
| PWM LED Biru | PB0 | TIM3_CH3 | PWM 1 kHz |
| PWM Servo | PB6 | TIM4_CH1 | PWM 50 Hz |
| PWM Buzzer | PB7 | TIM4_CH2 | PWM frekuensi variabel |
| ADC Potensiometer | PA0 | ADC1_IN0 | Input analog, kontrol parameter |

> **Catatan STM32F411:** Karena tidak memiliki DAC, gunakan PWM frekuensi tinggi (40-80 kHz) pada PA8 dengan RC low-pass filter (R=1kΩ, C=100nF, fc ≈ 1.6 kHz) untuk menghasilkan output analog. Konfigurasi Timer melalui prescaler dan ARR.

## 📝 Spesifikasi Fungsional

### Fitur Wajib (Minimum Requirements)

1. **Generator Gelombang DAC (atau PWM+Filter)**
   - Menghasilkan gelombang sinus (sine wave) pada frekuensi yang dapat diatur
   - Menghasilkan gelombang segitiga (triangle wave)
   - Frekuensi dapat diatur melalui potensiometer (100 Hz - 2 kHz)
   - Output ke speaker melalui kapasitor kopling
   - Lookup table untuk sine wave (minimal 64 titik)

2. **Kontrol LED RGB dengan PWM**
   - Efek fade in/fade out untuk setiap warna
   - Efek rainbow (transisi warna pelangi)
   - Efek breathing (LED menyala-meredup perlahan)
   - Kecerahan diatur via potensiometer
   - Duty cycle PWM 0-100% dengan resolusi minimal 8-bit

3. **Kontrol Servo Motor**
   - Servo dikendalikan oleh posisi potensiometer (ADC input)
   - Range gerakan: 0° - 180°
   - PWM 50 Hz dengan pulse width 500µs - 2500µs
   - Gerakan halus (smooth) tanpa jitter
   - Mode sweep otomatis (0° → 180° → 0° berulang)

4. **Melody Player (Buzzer PWM)**
   - Memainkan minimal 2 melodi berbeda (contoh: Twinkle-Twinkle, Ode to Joy)
   - Setiap nada didefinisikan dengan frekuensi dan durasi
   - Tempo dapat diatur
   - Kontrol volume melalui duty cycle PWM (jika buzzer pasif)

5. **Antarmuka Serial (Menu Interaktif)**
   - Menu utama dengan pilihan mode:
     ```
     ======== DAC & PWM CONTROLLER ========
     1. DAC Wave Generator (Sine/Triangle)
     2. RGB LED Effects
     3. Servo Motor Control
     4. Melody Player
     5. Demo All (Sequential)
     0. Stop / Reset
     =======================================
     Pilih mode [0-5]:
     ```
   - Sub-menu untuk setiap mode (pilihan parameter)
   - Feedback status di Serial Monitor

### Fitur Tambahan (Bonus)

1. **Audio Mixer**
   - Menggabungkan 2 gelombang dengan frekuensi berbeda
   - Efek AM (Amplitude Modulation) sederhana

2. **Visualisasi Serial Plotter**
   - Output format CSV untuk Arduino Serial Plotter
   - Tampilan gelombang real-time

3. **Pattern LED Kustom**
   - User mendefinisikan warna dan pola via serial
   - Pola tersimpan di EEPROM/NVS

4. **Kontrol via Python GUI**
   - Script Python dengan slider untuk kontrol parameter
   - Visualisasi gelombang dan warna LED

## 📊 Deliverables (Yang Harus Dikumpulkan)

### 1. Source Code
- **Firmware mikrokontroler** (file `.cpp` / `.c` dengan komentar lengkap)
- **Lookup table** gelombang sinus (header file terpisah)
- **File `platformio.ini`** yang sudah dikonfigurasi
- **Script Python** (jika mengimplementasikan bonus GUI)
- Semua file dikumpulkan dalam satu repository/folder terstruktur

### 2. Laporan Tertulis
Laporan dalam format PDF (minimal 10 halaman) berisi:

| Bagian | Konten |
|--------|--------|
| Pendahuluan | Latar belakang, tujuan, ruang lingkup |
| Dasar Teori | Prinsip DAC (R-2R, tipe), PWM (duty cycle, frekuensi), servo, audio digital |
| Perancangan | Skema rangkaian (Fritzing/KiCad), diagram alir program, lookup table |
| Implementasi | Penjelasan kode, konfigurasi LEDC/Timer, perhitungan frekuensi |
| Pengujian | Hasil pengukuran (osiloskop jika ada), tabel data, screenshot serial |
| Analisis | Perbandingan DAC vs PWM+filter, kualitas audio, akurasi servo |
| Kesimpulan | Rangkuman hasil dan saran pengembangan |

### 3. Video Demonstrasi
- Durasi: 5-10 menit
- Menunjukkan rangkaian hardware yang sudah terpasang
- Demo setiap mode: generator gelombang, LED RGB, servo, melodi
- Demo kontrol via potensiometer dan serial
- Penjelasan singkat kode dan cara kerja
- Menunjukkan output audio dan efek LED secara jelas

## 📅 Timeline Pengerjaan

| Minggu | Aktivitas |
|--------|-----------|
| Minggu 1 | Perakitan hardware, implementasi DAC wave generator |
| Minggu 2 | Implementasi PWM LED RGB dan servo motor control |
| Minggu 3 | Melody player, integrasi menu serial, dan ADC control |
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

1. **Arus Speaker:** Jangan menghubungkan speaker langsung ke pin DAC/GPIO tanpa pembatas arus. Gunakan resistor atau modul amplifier.
2. **Arus Servo:** Servo SG90 membutuhkan arus hingga 500mA. Gunakan sumber daya terpisah (bukan dari pin 3.3V mikrokontroler) jika servo berbeban.
3. **Frekuensi PWM Servo:** Harus tepat 50 Hz. Frekuensi yang salah dapat merusak servo.
4. **DAC 8-bit:** Output DAC ESP32 hanya 8-bit (256 level). Untuk audio berkualitas tinggi, gunakan external DAC (I2S + DAC chip).
5. **RC Filter:** Jika menggunakan PWM sebagai pengganti DAC, pastikan frekuensi cutoff RC filter sesuai (fc << f_PWM, fc > f_audio_max).
6. **Grounding:** Gunakan ground bersama untuk semua komponen, terutama antara mikrokontroler dan speaker/amplifier.

## 📚 Referensi Pendukung

- ESP-IDF LEDC Documentation: Konfigurasi PWM pada ESP32
- ESP-IDF DAC Documentation: Output analog ESP32
- STM32 Timer PWM Application Notes
- Datasheet SG90 Servo Motor: Timing diagram dan spesifikasi
- Lookup table generator: Membuat tabel sine wave untuk DAC
