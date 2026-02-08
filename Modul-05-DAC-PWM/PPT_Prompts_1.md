# 🎨 Prompt Pembuatan Slide Presentasi - Modul 05 DAC & PWM (Bagian 1: Slide 1-20)

## Instruksi Umum

Prompt ini digunakan untuk men-generate slide presentasi menggunakan AI (ChatGPT/Gemini/Claude) atau sebagai panduan pembuatan manual di PowerPoint/Google Slides. Bagian 1 membahas teori DAC, jenis-jenis DAC, arsitektur DAC pada ESP32 dan STM32.

---

## 📋 Prompt Utama

```
Buatkan presentasi PowerPoint dalam Bahasa Indonesia untuk mata kuliah Praktikum Sistem Embedded dengan topik "Digital-to-Analog Converter (DAC)" sebanyak 20 slide (Bagian 1 dari 2). Gunakan desain profesional dengan tema warna hijau-abu-abu. Setiap slide harus memiliki header, konten yang jelas, dan visual pendukung (diagram/ilustrasi). Target audiens adalah mahasiswa Teknik Elektro/Informatika semester 5-6.

SLIDE 1: Halaman Judul
- Judul: "Modul 05: DAC & PWM (Digital-to-Analog Converter & Pulse Width Modulation)"
- Subtitle: "Praktikum Sistem Embedded"
- Informasi: Nama institusi, semester, tahun ajaran
- Logo institusi di pojok kanan atas
- Desain clean dan profesional

SLIDE 2: Capaian Pembelajaran
- Judul: "Capaian Pembelajaran Modul"
- Daftar 6 capaian pembelajaran:
  • Memahami prinsip kerja DAC dan arsitekturnya
  • Menjelaskan konsep PWM: duty cycle, frekuensi, dan resolusi
  • Mengimplementasikan output DAC pada ESP32 (8-bit, GPIO25/GPIO26)
  • Mengkonfigurasi PWM menggunakan LEDC (ESP32) dan Timer (STM32)
  • Mengendalikan servo motor, LED RGB, dan buzzer dengan PWM
  • Membandingkan DAC murni vs PWM+filter untuk output analog

SLIDE 3: Outline Materi
- Judul: "Outline Materi"
- Daftar topik dengan ikon:
  1. Pendahuluan: Mengapa Perlu Output Analog?
  2. Prinsip Kerja DAC
  3. Jenis-Jenis DAC (Binary Weighted, R-2R Ladder)
  4. Parameter & Karakteristik DAC
  5. DAC pada ESP32 (8-bit, 2 channel)
  6. DAC pada STM32 (12-bit, seri tertentu)
  7. Pengenalan PWM sebagai Alternatif DAC
  8. Implementasi & Aplikasi

SLIDE 4: Mengapa Perlu Output Analog?
- Judul: "Mengapa Mikrokontroler Memerlukan Output Analog?"
- Diagram blok: Data Digital → DAC/PWM → Sinyal Analog → Aktuator
- Contoh aplikasi:
  • Audio: Menghasilkan suara/musik melalui speaker
  • Kontrol motor: Mengatur kecepatan motor DC
  • Dimming LED: Mengatur kecerahan lampu
  • Kontrol posisi: Servo motor untuk robotika
  • Generasi sinyal: Gelombang sinus untuk pengujian
- Pesan kunci: "DAC & PWM adalah jembatan dari dunia digital ke analog"

SLIDE 5: Prinsip Dasar DAC
- Judul: "Prinsip Dasar Digital-to-Analog Converter"
- Definisi: Mengubah data biner menjadi tegangan/arus analog proporsional
- Diagram blok sederhana: Input Biner (n-bit) → DAC → Output Analog (Vout)
- Rumus dasar: Vout = (Digital_Value / 2^n) × Vref
- Contoh: DAC 8-bit, Vref=3.3V, Input=128 → Vout = (128/256) × 3.3V = 1.65V
- Diagram transfer function: tangga (staircase) dari 0 ke Vref

SLIDE 6: Resolusi DAC
- Judul: "Resolusi DAC: Berapa Bit yang Dibutuhkan?"
- Tabel perbandingan resolusi DAC:
  | Resolusi | Level | Step Size (3.3V Ref) | Aplikasi Tipikal |
  |----------|-------|---------------------|------------------|
  | 8-bit | 256 | 12.89 mV | Audio sederhana, ESP32 DAC |
  | 10-bit | 1024 | 3.22 mV | Kontrol motor |
  | 12-bit | 4096 | 0.806 mV | STM32 DAC, instrumentasi |
  | 16-bit | 65536 | 50.3 µV | Audio hi-fi |
  | 24-bit | 16.7M | 0.197 µV | Audio profesional (I2S DAC) |
- Step size (LSB) = Vref / 2^n
- Penekanan: Resolusi lebih tinggi → output lebih halus

SLIDE 7: Jenis DAC - Binary Weighted Resistor
- Judul: "Tipe 1: Binary Weighted Resistor DAC"
- Diagram rangkaian: n resistor bernilai R, 2R, 4R, 8R... disambungkan ke op-amp summing
- Cara kerja: Setiap bit mengendalikan switch, arus proporsional terhadap bobot bit
- Kelebihan: Sederhana, cepat
- Kekurangan: Membutuhkan resistor presisi dengan range nilai sangat lebar (R sampai 2^(n-1)×R)
- Contoh: DAC 4-bit dengan R=10kΩ → membutuhkan 10k, 20k, 40k, 80k
- Masalah: Untuk 12-bit, range resistor 1:4096 → tidak praktis

SLIDE 8: Jenis DAC - R-2R Ladder
- Judul: "Tipe 2: R-2R Ladder DAC"
- Diagram rangkaian: Hanya menggunakan 2 nilai resistor (R dan 2R)
- Cara kerja: Pembagian tegangan bertingkat menggunakan jaringan resistor
- Kelebihan: Hanya 2 nilai resistor, mudah dibuat presisi, scalable
- Kekurangan: Kecepatan terbatas oleh settling time RC
- Penekanan: **R-2R adalah tipe DAC paling umum di mikrokontroler**
- Contoh perhitungan tegangan output untuk input 4-bit

SLIDE 9: Jenis DAC - Sigma-Delta DAC
- Judul: "Tipe 3: Sigma-Delta (ΣΔ) DAC"
- Diagram blok: Interpolation filter → ΣΔ Modulator → 1-bit DAC → Analog LPF
- Cara kerja: Oversampling + noise shaping + low-pass filtering
- Kelebihan: Resolusi sangat tinggi (16-24 bit), murah
- Kekurangan: Membutuhkan filter analog, latency
- Digunakan pada: Audio codec (I2S DAC chip), CD player
- Prinsip: Mengubah sinyal multi-bit menjadi pulse density (PDM)

SLIDE 10: Jenis DAC - PWM sebagai DAC
- Judul: "Tipe 4: PWM + Low-Pass Filter sebagai DAC"
- Diagram: PWM Output → RC Low-Pass Filter → Tegangan Analog (rata-rata)
- Cara kerja: Tegangan rata-rata PWM = Duty Cycle × Vcc
- Contoh: Duty 50%, Vcc=3.3V → Vavg = 1.65V
- Kelebihan: Tidak perlu hardware DAC, tersedia di semua mikrokontroler
- Kekurangan: Ripple pada output, bandwidth terbatas oleh filter
- Desain filter: R=1kΩ, C=100nF → fc = 1/(2π×R×C) ≈ 1.6 kHz
- **Penting untuk STM32F411 yang tidak memiliki DAC!**

SLIDE 11: Perbandingan Jenis DAC
- Judul: "Perbandingan Empat Tipe DAC"
- Tabel perbandingan:
  | Parameter | Binary Weighted | R-2R | Sigma-Delta | PWM+Filter |
  |-----------|:-:|:-:|:-:|:-:|
  | Resolusi | ★★ | ★★★ | ★★★★★ | ★★★ |
  | Kecepatan | ★★★★ | ★★★★ | ★★ | ★★ |
  | Kompleksitas | ★★★ | ★★★★ | ★★ | ★★★★★ |
  | Biaya | ★★★ | ★★★★ | ★★★ | ★★★★★ |
  | Linearitas | ★★ | ★★★★ | ★★★★★ | ★★★ |
- Highlight: R-2R dan Sigma-Delta paling umum di IC DAC, PWM+Filter paling mudah

SLIDE 12: Parameter DAC
- Judul: "Parameter Karakteristik DAC"
- Parameter statis:
  • **DNL (Differential Non-Linearity)**: Deviasi step size dari ideal
  • **INL (Integral Non-Linearity)**: Deviasi total dari garis transfer ideal
  • **Offset Error**: Pergeseran titik nol
  • **Gain Error**: Perbedaan kemiringan transfer function
  • **Monotonicity**: Apakah output selalu naik saat input naik
- Diagram transfer function ideal vs real (menunjukkan DNL, INL)
- Penting: DAC yang baik harus monotonic (tidak ada "langkah mundur")

SLIDE 13: Parameter Dinamis DAC
- Judul: "Parameter Dinamis DAC"
- Parameter dinamis:
  • **Settling Time**: Waktu output mencapai nilai akhir (±½ LSB)
  • **Slew Rate**: Kecepatan perubahan output (V/µs)
  • **Glitch Energy**: Transient spike saat perubahan kode (area × waktu)
  • **SNR**: Signal-to-Noise Ratio output
  • **THD**: Total Harmonic Distortion
  • **SFDR**: Spurious-Free Dynamic Range
- Diagram settling time dan glitch pada transisi kode
- Penekanan: Settling time penting untuk generasi gelombang berkecepatan tinggi

SLIDE 14: DAC pada ESP32 - Arsitektur
- Judul: "DAC pada ESP32: Arsitektur dan Spesifikasi"
- Spesifikasi DAC ESP32:
  • 2 channel DAC independen: DAC1 (GPIO25) dan DAC2 (GPIO26)
  • Resolusi: **8-bit** (0-255) → step size ≈ 12.9 mV
  • Output range: 0V - 3.3V (rail-to-rail)
  • Impedansi output: Rendah (dapat drive beban ringan langsung)
  • Fitur: Cosine Wave Generator (CW) built-in
  • Fitur: DMA support untuk output waveform kontinu
- Diagram blok internal DAC ESP32
- ⚠ Hanya tersedia pada ESP32 (WROOM/WROVER), TIDAK pada S2, S3, C3

SLIDE 15: DAC ESP32 - Cosine Wave Generator
- Judul: "ESP32 DAC: Cosine Wave Generator (CW)"
- Fitur hardware CW generator built-in:
  • Menghasilkan gelombang kosinus tanpa intervensi CPU
  • Frekuensi: Dapat diatur via register (130 Hz - 100+ kHz)
  • Amplitude: 4 level (penuh, ½, ¼, ⅛ dari Vref)
  • DC offset: Dapat dikonfigurasi
- Rumus frekuensi: f = RTC_FAST_CLK / (65536 × freq_step)
- Kegunaan: Tone generator, sinyal test, carrier signal
- Diagram output CW di osiloskop

SLIDE 16: DAC ESP32 - Kode Implementasi
- Judul: "Implementasi DAC ESP32 (Arduino Framework)"
- Kode contoh output DAC sederhana:
  ```cpp
  // Output nilai tetap
  dacWrite(25, 128);  // GPIO25, nilai 128 (≈1.65V)
  
  // Generasi sine wave dengan lookup table
  const uint8_t sineTable[64] = { /* 64 nilai sine 0-255 */ };
  for (int i = 0; i < 64; i++) {
    dacWrite(25, sineTable[i]);
    delayMicroseconds(100);  // ~156 Hz
  }
  ```
- Penjelasan: dacWrite(pin, value) → 0-255 maps ke 0-3.3V
- Tips: Gunakan timer interrupt untuk timing yang presisi
- Catatan tentang API ESP-IDF: dac_output_voltage(), dac_cw_generator_config()

SLIDE 17: DAC ESP32 - Lookup Table Sine Wave
- Judul: "Membuat Lookup Table Gelombang Sinus"
- Rumus: value[i] = 127 + 127 × sin(2π × i / N)
- Contoh tabel 64 titik (N=64):
  ```
  128, 140, 152, 165, 176, 187, 197, 206,
  213, 219, 224, 227, 229, 229, 228, 225,
  ...
  ```
- Diagram: Titik-titik diskrit pada kurva sinus
- Trade-off: N lebih besar → gelombang lebih halus, frekuensi lebih rendah
- Frekuensi output: f = 1 / (N × T_sample)
- Tools: Script Python untuk generate lookup table

SLIDE 18: DAC pada STM32 - Ketersediaan
- Judul: "DAC pada STM32: Ketersediaan per Seri"
- Tabel ketersediaan DAC STM32:
  | Seri STM32 | DAC? | Resolusi | Channel | Catatan |
  |------------|:----:|:--------:|:-------:|---------|
  | STM32F0 | Sebagian | 12-bit | 1-2 | Tergantung varian |
  | STM32F1 (F103) | ❌ | - | - | Tidak ada DAC |
  | STM32F3 | ✅ | 12-bit | 1-2 | Dual DAC |
  | STM32F4 (F407) | ✅ | 12-bit | 2 | PA4 (DAC1), PA5 (DAC2) |
  | **STM32F4 (F411)** | **❌** | **-** | **-** | **Tidak ada DAC!** |
  | STM32F7 | ✅ | 12-bit | 2 | Dengan DMA |
  | STM32H7 | ✅ | 12-bit | 2 | High performance |
- ⚠ **STM32F411 BlackPill TIDAK memiliki DAC → gunakan PWM + RC filter**
- Alternatif: External I2C/SPI DAC (MCP4725, MCP4822)

SLIDE 19: DAC STM32 - Implementasi (F407 sebagai Referensi)
- Judul: "Implementasi DAC STM32F407 (sebagai Referensi)"
- Kode contoh HAL:
  ```c
  DAC_HandleTypeDef hdac;
  DAC_ChannelConfTypeDef sConfig;
  
  // Inisialisasi
  hdac.Instance = DAC;
  HAL_DAC_Init(&hdac);
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
  
  // Set nilai output (12-bit: 0-4095)
  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048); // ~1.65V
  ```
- Catatan: Kode ini untuk referensi, F411 menggunakan PWM sebagai pengganti
- Mode: Polling, Interrupt, DMA (DMA untuk waveform kontinu)

SLIDE 20: Ringkasan Bagian 1
- Judul: "Ringkasan: Teori DAC"
- Poin-poin kunci:
  1. DAC mengkonversi data digital menjadi sinyal analog (Vout = D/2^n × Vref)
  2. R-2R Ladder adalah tipe paling umum di mikrokontroler
  3. ESP32 memiliki DAC 8-bit pada GPIO25 dan GPIO26 (hanya ESP32 WROOM)
  4. STM32F411 TIDAK memiliki DAC → gunakan PWM + RC filter sebagai alternatif
  5. Cosine Wave Generator ESP32 dapat menghasilkan gelombang tanpa CPU
  6. Parameter penting: DNL, INL, settling time, monotonicity
- Preview: "Selanjutnya: PWM - Pulse Width Modulation, LEDC, Timer, Servo, RGB LED"
- QR code ke referensi online (opsional)
```

## 🎨 Panduan Desain

| Elemen | Spesifikasi |
|--------|-------------|
| Font Judul | Calibri Bold, 28-32pt |
| Font Konten | Calibri Regular, 18-22pt |
| Font Kode | Consolas / Courier New, 14-16pt |
| Warna Primer | Hijau (#2e7d32) |
| Warna Sekunder | Abu-abu (#5f6368) |
| Warna Aksen | Oranye (#f9a825) untuk highlight penting |
| Warna Peringatan | Merah (#d32f2f) untuk catatan penting |
| Background | Putih dengan subtle gradient hijau |
| Diagram | Gunakan warna kontras, garis tebal 2pt |
| Ikon | Material Design Icons atau Font Awesome |

## 📐 Tips Pembuatan

1. **Satu ide per slide** - Jangan memuat terlalu banyak informasi
2. **Visual > Teks** - Gunakan diagram rangkaian dan grafik transfer function
3. **Animasi sederhana** - Fade in untuk poin-poin, appear untuk diagram bertahap
4. **Konsisten** - Gunakan layout dan format yang sama di seluruh slide
5. **Catatan presenter** - Tambahkan catatan detail di bagian notes setiap slide
