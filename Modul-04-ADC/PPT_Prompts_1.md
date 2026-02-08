# 🎨 Prompt Slide Presentasi — Modul 04: Menguak Dunia Analog — ADC (Bagian 1: Slide 1-20)

## Instruksi Umum

Prompt ini digunakan untuk men-generate slide presentasi menggunakan AI (ChatGPT/Gemini/Claude) atau sebagai panduan pembuatan manual di PowerPoint/Google Slides.

---

## 📋 Prompt Utama

```
Buatkan presentasi PowerPoint dalam Bahasa Indonesia untuk mata kuliah Praktikum Sistem Embedded dengan topik "Analog-to-Digital Converter (ADC)" sebanyak 20 slide (Bagian 1 dari 2). Gunakan desain profesional dengan tema warna biru-abu-abu. Setiap slide harus memiliki header, konten yang jelas, dan visual pendukung (diagram/ilustrasi). Target audiens adalah mahasiswa Teknik Elektro/Informatika semester 5-6.

SLIDE 1: Halaman Judul
- Judul: "Modul 04: Analog-to-Digital Converter (ADC)"
- Subtitle: "Praktikum Sistem Embedded"
- Informasi: Nama institusi, semester, tahun ajaran
- Logo institusi di pojok kanan atas
- Desain clean dan profesional

SLIDE 2: Capaian Pembelajaran
- Judul: "Capaian Pembelajaran Modul"
- Daftar 5-6 capaian pembelajaran:
  • Memahami prinsip kerja dan arsitektur ADC
  • Menjelaskan konsep sampling, quantization, dan encoding
  • Membedakan jenis-jenis ADC dan karakteristiknya
  • Mengkonfigurasi ADC pada ESP32 dan STM32
  • Mengimplementasikan pembacaan sensor analog multi-channel
  • Menganalisis sumber error ADC dan teknik mitigasinya

SLIDE 3: Outline Materi
- Judul: "Outline Materi"
- Daftar topik yang akan dibahas dengan ikon:
  1. Pendahuluan: Sinyal Analog vs Digital
  2. Prinsip Kerja ADC
  3. Proses Sampling & Teorema Nyquist
  4. Proses Quantization & Resolusi
  5. Jenis-Jenis ADC
  6. Parameter & Karakteristik ADC
  7. ADC pada ESP32 & STM32
  8. Implementasi & Aplikasi

SLIDE 4: Sinyal Analog vs Digital
- Judul: "Sinyal Analog vs Sinyal Digital"
- Tabel perbandingan 2 kolom:
  • Analog: kontinu, infinite resolusi, rentan noise, contoh: suhu, tekanan
  • Digital: diskrit, resolusi terbatas, tahan noise, contoh: data sensor ADC
- Diagram gelombang sinyal analog (sinusoidal) di samping sinyal digital (step)
- Penekanan: "Dunia nyata bersifat ANALOG, komputer bekerja secara DIGITAL"

SLIDE 5: Mengapa Perlu ADC?
- Judul: "Mengapa Mikrokontroler Memerlukan ADC?"
- Diagram blok: Sensor Analog → ADC → Mikrokontroler → Pengolahan → Aksi
- Contoh aplikasi nyata:
  • Sensor suhu (LM35) → ADC → Tampilan °C
  • Sensor gas (MQ-135) → ADC → Alarm kualitas udara
  • Microphone → ADC → Pemrosesan audio
  • Sensor tekanan → ADC → Monitoring industri
- Pesan kunci: "ADC adalah jembatan antara dunia analog dan digital"

SLIDE 6: Prinsip Dasar Konversi ADC
- Judul: "Prinsip Dasar Konversi Analog ke Digital"
- Diagram 3 tahap konversi:
  1. **Sampling** - Mengambil sampel sinyal pada interval waktu tertentu
  2. **Quantization** - Memetakan nilai sampel ke level diskrit terdekat
  3. **Encoding** - Mengkonversi level diskrit menjadi kode biner
- Ilustrasi visual setiap tahap dengan sinyal sinusoidal sebagai contoh
- Rumus dasar: Digital_Value = (Vin / Vref) × (2^n - 1)

SLIDE 7: Sampling - Pengambilan Sampel
- Judul: "Sampling: Pengambilan Sampel Sinyal"
- Definisi sampling rate (fs) dan periode sampling (Ts = 1/fs)
- Diagram sinyal kontinu dengan titik-titik sampel yang ditandai
- Contoh: Sinyal audio 4 kHz → fs minimal 8 kHz
- Ilustrasi sample-and-hold circuit
- Satuan: Hz, kHz, kSPS (kilo Samples Per Second), MSPS

SLIDE 8: Teorema Nyquist-Shannon
- Judul: "Teorema Nyquist-Shannon"
- Rumus: fs ≥ 2 × fmax (Nyquist Rate)
- Diagram 3 kasus:
  1. fs > 2×fmax → Sinyal terekonstruksi dengan baik ✓
  2. fs = 2×fmax → Batas minimum (nyquist rate) ⚠
  3. fs < 2×fmax → Aliasing terjadi ✗
- Ilustrasi aliasing: sinyal asli vs sinyal alias yang salah
- Penekanan: "Aliasing menyebabkan informasi hilang secara permanen"

SLIDE 9: Anti-Aliasing Filter
- Judul: "Anti-Aliasing Filter"
- Mengapa diperlukan: Mencegah aliasing sebelum proses sampling
- Jenis: Low-Pass Filter (LPF) analog
- Diagram blok: Sinyal Input → LPF → Sample & Hold → ADC
- Contoh: fmax = 1 kHz, fs = 10 kHz → Cut-off LPF = 5 kHz
- Respons frekuensi filter ideal vs real
- Catatan: Filter anti-aliasing harus berupa filter ANALOG, bukan digital

SLIDE 10: Quantization - Kuantisasi
- Judul: "Quantization: Proses Kuantisasi"
- Definisi: Memetakan nilai kontinu ke level diskrit terdekat
- Diagram tangga (staircase) quantization
- Konsep quantization level: 2^n level untuk ADC n-bit
- Contoh ADC 3-bit: 8 level (000 sampai 111)
- Quantization step size (LSB): q = Vref / 2^n
- Contoh: Vref=3.3V, 12-bit → q = 3.3/4096 = 0.806 mV

SLIDE 11: Quantization Error
- Judul: "Quantization Error (Noise Kuantisasi)"
- Definisi: Selisih antara nilai analog asli dan nilai terkuantisasi
- Range error: -q/2 sampai +q/2 (untuk mid-tread quantizer)
- Diagram error vs input signal (bentuk sawtooth)
- RMS quantization noise: q / √12
- SNR (Signal-to-Noise Ratio): SNR ≈ 6.02n + 1.76 dB (untuk n-bit ADC)
- Contoh: 12-bit ADC → SNR ≈ 74 dB

SLIDE 12: Resolusi ADC
- Judul: "Resolusi ADC: Berapa Bit yang Dibutuhkan?"
- Tabel perbandingan resolusi:
  | Resolusi | Level | Step Size (3.3V) | Aplikasi |
  |----------|-------|-------------------|----------|
  | 8-bit    | 256   | 12.89 mV          | Audio sederhana |
  | 10-bit   | 1024  | 3.22 mV           | Arduino UNO |
  | 12-bit   | 4096  | 0.806 mV          | ESP32, STM32 |
  | 16-bit   | 65536 | 50.3 µV           | Instrumentasi presisi |
  | 24-bit   | 16.7M | 0.197 µV          | Audio profesional |
- Penekanan: Resolusi lebih tinggi ≠ selalu lebih baik (noise floor, kecepatan)

SLIDE 13: Jenis ADC - Flash ADC
- Judul: "Tipe 1: Flash ADC (Parallel ADC)"
- Diagram arsitektur: N-1 komparator untuk N-bit
- Cara kerja: Semua komparator membandingkan secara paralel
- Kelebihan: Tercepat (1 clock cycle), cocok untuk video/RF
- Kekurangan: Banyak komparator (2^n - 1), mahal, konsumsi daya tinggi
- Contoh: ADC 8-bit Flash membutuhkan 255 komparator
- Kecepatan tipikal: >100 MSPS

SLIDE 14: Jenis ADC - SAR ADC
- Judul: "Tipe 2: Successive Approximation Register (SAR) ADC"
- Diagram arsitektur: Komparator + DAC + SAR Logic + Register
- Cara kerja binary search: MSB dulu, bandingkan, lanjut ke bit berikutnya
- Animasi/step-by-step contoh konversi 4-bit
- Kelebihan: Keseimbangan kecepatan dan resolusi, daya rendah
- Kekurangan: Kecepatan sedang, tidak cocok untuk sinyal sangat cepat
- Digunakan pada: ESP32 dan STM32F411 ← PENTING!
- Kecepatan tipikal: 100 kSPS - 5 MSPS

SLIDE 15: Jenis ADC - Sigma-Delta ADC
- Judul: "Tipe 3: Sigma-Delta (ΣΔ) ADC"
- Diagram arsitektur: Integrator + Komparator + Feedback DAC
- Cara kerja: Oversampling + Noise shaping + Decimation filter
- Kelebihan: Resolusi sangat tinggi (16-24 bit), noise rendah
- Kekurangan: Lambat, tidak cocok untuk sinyal cepat
- Digunakan pada: Audio codec, sensor presisi, timbangan digital
- Kecepatan tipikal: 10 SPS - 100 kSPS

SLIDE 16: Jenis ADC - Pipeline ADC
- Judul: "Tipe 4: Pipeline ADC"
- Diagram arsitektur: Beberapa stage, setiap stage mengkonversi beberapa bit
- Cara kerja: Konversi bertahap dengan residue amplifier
- Kelebihan: Throughput tinggi dengan resolusi tinggi
- Kekurangan: Latency beberapa clock cycle
- Digunakan pada: Komunikasi, imaging, oscilloscope
- Kecepatan tipikal: 10 MSPS - 500 MSPS

SLIDE 17: Perbandingan Jenis ADC
- Judul: "Perbandingan Empat Tipe ADC"
- Tabel perbandingan komprehensif:
  | Parameter | Flash | SAR | Sigma-Delta | Pipeline |
  |-----------|-------|-----|-------------|----------|
  | Kecepatan | ★★★★★ | ★★★ | ★ | ★★★★ |
  | Resolusi | ★ | ★★★ | ★★★★★ | ★★★★ |
  | Daya | ★ | ★★★★ | ★★★ | ★★ |
  | Biaya | ★ | ★★★★★ | ★★★★ | ★★ |
  | Ukuran | ★ | ★★★★★ | ★★★ | ★★★ |
- Grafik scatter: Speed vs Resolution untuk keempat tipe
- Highlight: SAR ADC adalah pilihan terbaik untuk mikrokontroler umum

SLIDE 18: Parameter Penting ADC
- Judul: "Parameter Karakteristik ADC"
- Daftar parameter statis:
  • DNL (Differential Non-Linearity): Deviasi step size dari ideal
  • INL (Integral Non-Linearity): Deviasi total dari garis ideal
  • Offset Error: Pergeseran nol dari nilai ideal
  • Gain Error: Perbedaan kemiringan dari transfer function ideal
  • Missing Codes: Level kuantisasi yang tidak pernah muncul
- Diagram transfer function ADC ideal vs real

SLIDE 19: Parameter Dinamis ADC
- Judul: "Parameter Dinamis ADC"
- Daftar parameter dinamis:
  • SNR (Signal-to-Noise Ratio): Rasio sinyal terhadap noise
  • SINAD (Signal-to-Noise and Distortion): SNR termasuk distorsi
  • THD (Total Harmonic Distortion): Distorsi harmonik total
  • SFDR (Spurious-Free Dynamic Range): Range bebas spurious
  • ENOB (Effective Number of Bits): Bit efektif = (SINAD - 1.76) / 6.02
- Contoh: ADC 12-bit dengan ENOB = 10.5 bit → resolusi efektif hanya 10.5 bit

SLIDE 20: Ringkasan Bagian 1
- Judul: "Ringkasan: Teori Dasar ADC"
- Poin-poin kunci yang telah dipelajari:
  1. ADC mengkonversi sinyal analog menjadi digital melalui 3 tahap
  2. Teorema Nyquist: fs ≥ 2 × fmax untuk menghindari aliasing
  3. Resolusi menentukan presisi: q = Vref / 2^n
  4. SAR ADC digunakan pada ESP32 dan STM32 (keseimbangan speed-resolusi)
  5. Parameter DNL, INL, ENOB menentukan kualitas ADC sebenarnya
- Preview: "Selanjutnya: ADC pada ESP32 & STM32, Implementasi, dan Aplikasi"
- QR code ke referensi online (opsional)
```

## 🎨 Panduan Desain

| Elemen | Spesifikasi |
|--------|-------------|
| Font Judul | Calibri Bold, 28-32pt |
| Font Konten | Calibri Regular, 18-22pt |
| Warna Primer | Biru (#1a73e8) |
| Warna Sekunder | Abu-abu (#5f6368) |
| Warna Aksen | Oranye (#f9a825) untuk highlight penting |
| Background | Putih dengan subtle gradient biru |
| Diagram | Gunakan warna kontras, garis tebal 2pt |
| Ikon | Material Design Icons atau Font Awesome |

## 📐 Tips Pembuatan

1. **Satu ide per slide** - Jangan memuat terlalu banyak informasi
2. **Visual > Teks** - Gunakan diagram dan ilustrasi sebanyak mungkin
3. **Animasi sederhana** - Fade in untuk poin-poin, appear untuk diagram bertahap
4. **Konsisten** - Gunakan layout dan format yang sama di seluruh slide
5. **Catatan presenter** - Tambahkan catatan detail di bagian notes setiap slide
