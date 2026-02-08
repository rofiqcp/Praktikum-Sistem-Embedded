# 🎨 Prompt Slide Presentasi — Modul 04: Menguak Dunia Analog — ADC (Bagian 2: Slide 21-40)

## Instruksi Umum

Prompt ini merupakan lanjutan dari Bagian 1 (Slide 1-20) yang membahas teori dasar ADC. Bagian 2 fokus pada implementasi ADC di ESP32 dan STM32, teknik filtering, serta aplikasi praktis.

---

## 📋 Prompt Utama

```
Buatkan presentasi PowerPoint dalam Bahasa Indonesia untuk mata kuliah Praktikum Sistem Embedded dengan topik "ADC pada ESP32 & STM32: Implementasi dan Aplikasi" sebanyak 20 slide (Bagian 2 dari 2, Slide 21-40). Gunakan desain yang konsisten dengan Bagian 1 (tema warna biru-abu-abu, profesional). Target audiens: mahasiswa Teknik Elektro/Informatika.

SLIDE 21: Halaman Pembuka Bagian 2
- Judul: "Bagian 2: ADC pada ESP32 & STM32"
- Subtitle: "Implementasi, Perbandingan, dan Aplikasi"
- Daftar topik:
  1. Arsitektur ADC ESP32
  2. Arsitektur ADC STM32
  3. Perbandingan ADC ESP32 vs STM32
  4. Teknik Filtering & Noise Reduction
  5. Kalibrasi ADC
  6. Aplikasi Praktis

SLIDE 22: Arsitektur ADC ESP32
- Judul: "ADC pada ESP32: Arsitektur"
- Spesifikasi ADC ESP32:
  • 2 unit ADC: ADC1 (8 channel) dan ADC2 (10 channel)
  • Resolusi: 9, 10, 11, atau 12 bit (konfigurabel)
  • Tipe: SAR (Successive Approximation Register)
  • Tegangan referensi internal: ~1.1V
  • Attenuation: 0dB, 2.5dB, 6dB, 11dB
- Diagram blok internal ADC ESP32
- ⚠ ADC2 tidak dapat digunakan bersamaan dengan WiFi

SLIDE 23: ADC ESP32 - Channel Mapping
- Judul: "ESP32 ADC Channel Mapping"
- Tabel pemetaan pin ke channel ADC:
  | GPIO | ADC Unit | Channel | Catatan |
  |------|----------|---------|---------|
  | GPIO 36 | ADC1 | CH0 | Input only (VP) |
  | GPIO 39 | ADC1 | CH3 | Input only (VN) |
  | GPIO 34 | ADC1 | CH6 | Input only |
  | GPIO 35 | ADC1 | CH7 | Input only |
  | GPIO 32 | ADC1 | CH4 | Bisa I/O |
  | GPIO 33 | ADC1 | CH5 | Bisa I/O |
- Rekomendasi: Gunakan ADC1 untuk proyek yang memerlukan WiFi
- Ilustrasi pinout ESP32 dengan channel ADC yang ditandai warna

SLIDE 24: ADC ESP32 - Attenuation
- Judul: "ESP32 ADC Attenuation"
- Tabel attenuation dan range tegangan:
  | Attenuation | Range Input | Resolusi Efektif | Akurasi Terbaik |
  |-------------|-------------|------------------|-----------------|
  | 0 dB | 0 - 1.1V | Tertinggi | 0.1V - 0.95V |
  | 2.5 dB | 0 - 1.5V | Tinggi | 0.1V - 1.25V |
  | 6 dB | 0 - 2.2V | Sedang | 0.15V - 1.75V |
  | 11 dB | 0 - 3.3V* | Terendah | 0.15V - 2.45V |
- ⚠ Pada 11dB, pembacaan tidak linier di atas 2.6V
- Diagram kurva transfer function ADC ESP32 (menunjukkan non-linearitas)
- Tips: Gunakan kalibrasi eFuse untuk meningkatkan akurasi

SLIDE 25: ADC ESP32 - Kode Implementasi
- Judul: "Implementasi ADC ESP32 (Arduino Framework)"
- Kode contoh pembacaan ADC sederhana:
  ```cpp
  // Konfigurasi
  const int ADC_PIN = 34;  // GPIO34, ADC1_CH6
  analogReadResolution(12); // 12-bit (0-4095)
  analogSetAttenuation(ADC_11db); // Range 0-3.3V
  
  // Pembacaan
  int rawValue = analogRead(ADC_PIN);
  float voltage = rawValue * 3.3 / 4095.0;
  ```
- Penjelasan setiap baris kode
- Catatan tentang analogSetPinAttenuation() untuk per-pin setting

SLIDE 26: Arsitektur ADC STM32F411
- Judul: "ADC pada STM32F411: Arsitektur"
- Spesifikasi ADC STM32F411:
  • 1 unit ADC (ADC1) dengan 16 channel eksternal
  • Resolusi: 6, 8, 10, atau 12 bit (konfigurabel)
  • Tipe: SAR, 12-bit @ 2.4 MSPS
  • Tegangan referensi: VDDA (biasanya 3.3V)
  • Mode: Single, Continuous, Scan, Discontinuous
  • Trigger: Software, Timer, External
- Diagram blok internal ADC STM32
- Fitur: Channel suhu internal dan VREFINT

SLIDE 27: ADC STM32 - Channel & Sampling Time
- Judul: "STM32 ADC Channel & Sampling Time"
- Tabel channel ADC STM32F411:
  | Channel | Pin | Fungsi |
  |---------|-----|--------|
  | IN0 | PA0 | ADC eksternal |
  | IN1 | PA1 | ADC eksternal |
  | IN4 | PA4 | ADC eksternal |
  | IN5 | PA5 | ADC eksternal |
  | IN16 | Internal | Sensor suhu |
  | IN17 | Internal | VREFINT (1.21V) |
- Tabel sampling time dan pengaruhnya:
  | Cycles | Waktu (@ 30 MHz) | Penggunaan |
  |--------|-------------------|------------|
  | 3 | 0.1 µs | Sumber impedansi rendah |
  | 15 | 0.5 µs | Umum |
  | 84 | 2.8 µs | Sumber impedansi tinggi |
  | 480 | 16 µs | Akurasi maksimal |

SLIDE 28: ADC STM32 - Mode Operasi
- Judul: "Mode Operasi ADC STM32"
- Penjelasan 4 mode:
  1. **Single Conversion**: Satu channel, satu kali konversi
  2. **Continuous Conversion**: Satu channel, konversi berulang otomatis
  3. **Scan Mode**: Multi-channel, satu sequence konversi
  4. **Scan + Continuous**: Multi-channel, berulang otomatis
- Diagram timing untuk setiap mode
- Rekomendasi: Scan + DMA untuk multi-channel efisien
- Trigger source: Software, TIM1-TIM5, EXTI

SLIDE 29: ADC STM32 - Kode Implementasi
- Judul: "Implementasi ADC STM32 (HAL Library)"
- Kode contoh pembacaan ADC:
  ```c
  // Inisialisasi (dari CubeMX atau manual)
  ADC_HandleTypeDef hadc1;
  
  // Polling mode
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  uint32_t rawValue = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  
  float voltage = rawValue * 3.3f / 4095.0f;
  ```
- Penjelasan fungsi HAL ADC
- Catatan tentang konfigurasi CubeMX untuk ADC

SLIDE 30: Perbandingan ADC ESP32 vs STM32
- Judul: "Perbandingan ADC: ESP32 vs STM32F411"
- Tabel perbandingan detail:
  | Parameter | ESP32 | STM32F411 |
  |-----------|-------|-----------|
  | Tipe ADC | SAR | SAR |
  | Resolusi Maks | 12-bit | 12-bit |
  | Jumlah ADC Unit | 2 (ADC1, ADC2) | 1 (ADC1) |
  | Channel Eksternal | 18 | 16 |
  | Sample Rate Maks | ~200 kSPS | 2.4 MSPS |
  | Tegangan Ref | 1.1V internal | VDDA (3.3V) |
  | Linearitas | Buruk (perlu kalibrasi) | Baik |
  | DMA Support | Ya (continuous mode) | Ya |
  | Mode | Oneshot, Continuous | Single, Continuous, Scan |
  | Batasan | ADC2 konflik WiFi | Channel lebih sedikit |
- Kesimpulan: STM32 unggul di linearitas & kecepatan, ESP32 di jumlah channel

SLIDE 31: Non-Linearitas ADC ESP32
- Judul: "Masalah Non-Linearitas ADC ESP32"
- Grafik: Kurva transfer ideal vs aktual ADC ESP32
- Area bermasalah:
  • 0V - 0.1V: Pembacaan tidak akurat (dead zone)
  • 2.6V - 3.3V: Saturasi dan non-linear (pada 11dB atten)
  • Variasi antar chip (manufacturing variance)
- Solusi:
  1. Kalibrasi eFuse (esp_adc_cal)
  2. Tabel lookup koreksi
  3. Polynomial fitting
  4. Batasi range operasi ke 0.15V - 2.45V
- Contoh kode kalibrasi ESP32

SLIDE 32: Teknik Filtering - Moving Average
- Judul: "Teknik Filtering: Moving Average Filter"
- Rumus: y[n] = (1/N) × Σ x[n-i] untuk i=0 sampai N-1
- Diagram blok filter
- Contoh implementasi:
  ```cpp
  #define WINDOW_SIZE 10
  int readings[WINDOW_SIZE];
  int readIndex = 0;
  long total = 0;
  
  total -= readings[readIndex];
  readings[readIndex] = analogRead(ADC_PIN);
  total += readings[readIndex];
  readIndex = (readIndex + 1) % WINDOW_SIZE;
  int average = total / WINDOW_SIZE;
  ```
- Grafik: Sinyal mentah (noisy) vs sinyal terfilter (smooth)
- Trade-off: N besar → lebih halus tapi respon lambat

SLIDE 33: Teknik Filtering - Median Filter
- Judul: "Teknik Filtering: Median Filter"
- Cara kerja: Mengambil nilai tengah dari N sampel yang diurutkan
- Kelebihan: Sangat baik untuk menghilangkan spike/outlier
- Implementasi:
  ```cpp
  int medianFilter(int pin, int samples) {
    int values[samples];
    for (int i = 0; i < samples; i++)
      values[i] = analogRead(pin);
    // Sort array
    sort(values, values + samples);
    return values[samples / 2];
  }
  ```
- Perbandingan visual: Data dengan spike → Moving Average vs Median Filter
- Median filter lebih baik untuk noise impulsif

SLIDE 34: Teknik Filtering - Exponential Moving Average (EMA)
- Judul: "Teknik Filtering: Exponential Moving Average"
- Rumus: y[n] = α × x[n] + (1-α) × y[n-1]
- α (alpha): Faktor smoothing (0 < α < 1)
  • α mendekati 1: Respon cepat, sedikit filtering
  • α mendekati 0: Respon lambat, banyak filtering
- Implementasi:
  ```cpp
  float alpha = 0.1;
  float emaValue = 0;
  
  int raw = analogRead(ADC_PIN);
  emaValue = alpha * raw + (1 - alpha) * emaValue;
  ```
- Kelebihan: Hemat memori (hanya 1 variabel), komputasi ringan
- Grafik perbandingan berbagai nilai alpha

SLIDE 35: Oversampling dan Averaging
- Judul: "Oversampling: Meningkatkan Resolusi Efektif"
- Prinsip: Mengambil lebih banyak sampel dan merata-ratakannya
- Rumus peningkatan resolusi: n_extra = log2(oversampling_ratio) / 2
- Tabel:
  | Oversampling | Sampel | Resolusi Tambahan | Resolusi Total (12-bit ADC) |
  |-------------|--------|-------------------|---------------------------|
  | 4× | 4 | +1 bit | 13 bit |
  | 16× | 16 | +2 bit | 14 bit |
  | 64× | 64 | +3 bit | 15 bit |
  | 256× | 256 | +4 bit | 16 bit |
- Syarat: Noise input harus > 1 LSB (dithering alami)
- ⚠ Mengurangi bandwidth efektif

SLIDE 36: Kalibrasi ADC
- Judul: "Kalibrasi ADC: Offset dan Gain"
- Prosedur kalibrasi 2 titik:
  1. Hubungkan input ke GND → catat pembacaan (offset)
  2. Hubungkan input ke VREF yang diketahui → catat pembacaan (gain)
  3. Hitung koreksi: V_actual = (V_raw - offset) × gain_correction
- Diagram: Kurva transfer sebelum dan sesudah kalibrasi
- Implementasi:
  ```cpp
  float calibrate(int rawValue) {
    const float OFFSET = 15;      // Dari pengukuran
    const float GAIN = 3.30 / 3.28; // Koreksi gain
    float voltage = (rawValue - OFFSET) * 3.3 / 4095.0 * GAIN;
    return voltage;
  }
  ```
- Tips: Kalibrasi ulang jika suhu lingkungan berubah signifikan

SLIDE 37: Konversi Nilai ADC ke Satuan Fisik
- Judul: "Konversi ADC ke Satuan Fisik"
- Contoh konversi untuk setiap sensor:
  • **LM35 → °C**: `suhu = (rawADC * 3.3 / 4095) * 100`
  • **LDR → Lux**: `lux = map(rawADC, 0, 4095, 1000, 0)` (aproksimasi)
  • **Potensiometer → %**: `persen = (rawADC * 100) / 4095`
  • **MQ-135 → PPM**: Menggunakan kurva Rs/R0 dari datasheet
- Diagram proses: Raw ADC → Tegangan → Satuan Fisik
- Catatan: Setiap sensor memiliki karakteristik transfer yang berbeda

SLIDE 38: Aplikasi - Sistem Monitoring Multi-Sensor
- Judul: "Aplikasi: Sistem Monitoring Kualitas Udara"
- Diagram sistem lengkap:
  • 4 sensor analog → ADC multi-channel → Mikrokontroler
  • Mikrokontroler → Serial → Python → CSV + Grafik
  • Threshold → LED/Buzzer alarm
- Screenshot Serial Monitor dengan data real-time
- Screenshot grafik Python matplotlib
- Ini adalah project yang akan dikerjakan mahasiswa

SLIDE 39: Best Practices ADC
- Judul: "Best Practices Penggunaan ADC"
- Tips hardware:
  1. Tambahkan kapasitor 100nF di pin ADC (bypass capacitor)
  2. Pisahkan ground analog dan digital jika memungkinkan
  3. Gunakan kabel pendek untuk sinyal analog
  4. Hindari routing sinyal analog dekat sinyal digital/PWM
- Tips software:
  1. Selalu gunakan averaging/filtering
  2. Lakukan kalibrasi sebelum deployment
  3. Perhatikan waktu sampling yang cukup
  4. Validasi range pembacaan (sanity check)
  5. Gunakan DMA untuk multi-channel agar tidak blocking

SLIDE 40: Penutup & Tugas
- Judul: "Ringkasan & Tugas Project"
- Ringkasan keseluruhan modul:
  • Prinsip ADC: Sampling → Quantization → Encoding
  • ESP32: 2 ADC, perhatikan non-linearitas dan batasan ADC2
  • STM32: Linearitas baik, mode scan + DMA untuk multi-channel
  • Filtering: Moving average, median, EMA untuk noise reduction
  • Kalibrasi penting untuk akurasi pembacaan
- Tugas Project: Sistem Monitoring Kualitas Udara
  • 4 sensor analog + threshold alert + Python logging
  • Deadline: [tanggal]
  • Deliverables: Kode + Laporan + Video
- QR code ke repository materi dan referensi
```

## 🎨 Panduan Desain

| Elemen | Spesifikasi |
|--------|-------------|
| Font Judul | Calibri Bold, 28-32pt |
| Font Konten | Calibri Regular, 18-22pt |
| Font Kode | Consolas / Courier New, 14-16pt |
| Warna Primer | Biru (#1a73e8) |
| Warna Sekunder | Abu-abu (#5f6368) |
| Warna Aksen | Oranye (#f9a825) untuk peringatan |
| Warna Kode | Background abu-abu gelap (#263238), teks terang |
| Background | Putih dengan subtle gradient biru |

## 📐 Tips Pembuatan

1. **Kode harus terbaca** - Gunakan syntax highlighting dan font monospace
2. **Diagram perbandingan** - Gunakan layout 2 kolom untuk ESP32 vs STM32
3. **Grafik data** - Gunakan chart yang jelas dengan label sumbu
4. **Animasi bertahap** - Untuk diagram proses dan kode step-by-step
5. **Konsistensi** - Pastikan desain sama dengan Bagian 1
