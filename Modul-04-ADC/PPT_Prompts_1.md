# Prompt untuk Pembuatan PPT - Bagian 1: Teori ADC
## Modul 04: Analog to Digital Converter

---

## 📋 Informasi Presentasi

| Item | Keterangan |
|------|------------|
| **Topik** | Teori Dasar ADC & Arsitektur uC |
| **Jumlah Slide** | 20 Slide |
| **Durasi** | 40 menit |
| **Target Audiens** | Mahasiswa Teknik Elektro/Komputer |

---

## 🎨 Panduan Desain
- **Style:** Clean, Schematic-heavy, Mathematical Visualization.
- **Visual Utama:** Grafik Sampling Sinyal Sinus, Diagram Blok SAR ADC.

---

## 📊 Struktur Slide Detail

### Slide 1: Cover
**Prompt:**
```
Judul: "Analog to Digital Converter (ADC)"
Subtitle: "Menjembatani Dunia Fisik ke Dunia Digital"
Visual: Ilustrasi gelombang suara analog berubah menjadi tangga digital (bit stream).
```

### Slide 2: Mengapa Butuh ADC?
**Prompt:**
```
Judul: "Dunia Nyata vs Dunia Mikrokontroler"
Konten:
- Dunia Nyata = Analog (Suhu, Suara, Cahaya, Tekanan). Sinyal kontinyu dalam waktu dan amplitudo.
- Mikrokontroler = Digital (0 dan 1). Sinyal diskrit.
- ADC berfungsi sebagai "Penerjemah".
Visual: Sensor Suhu -> ADC -> CPU -> Display Angka Digital.
```

### Slide 3: Konsep Sampling
**Prompt:**
```
Judul: "Sampling (Pencuplikan)"
Konten:
- Proses mengambil nilai sesaat sinyal analog pada interval waktu tetap (Ts).
- Sampling Rate (fs) = 1/Ts.
- Teorema Nyquist: fs >= 2 * f_max sinyal input.
Visual: Grafik sinyal sinus mulus dengan titik-titik sampel u/ setiap selang waktu T.
```

### Slide 4: Konsep Kuantisasi
**Prompt:**
```
Judul: "Quantization (Kuantisasi)"
Konten:
- Membulatkan nilai sampel ke level diskrit terdekat.
- Resolusi ditentukan oleh jumlah bit (n).
- Jumlah level = 2^n.
- Error Kuantisasi = Perbedaan antara nilai asli dan nilai digital.
Visual: Grafik "Tangga" yang mendekati kurva sinus.
```

### Slide 5: Resolusi ADC
**Prompt:**
```
Judul: "Resolusi & Step Size"
Konten:
- Rumus Step Size (LSB Voltage) = Vref / (2^n - 1).
- Contoh 10-bit @ 3.3V: 3.3 / 1023 = 3.22 mV per step.
- Contoh 12-bit @ 3.3V: 3.3 / 4095 = 0.8 mV per step.
- Semakin tinggi bit, semakin halus pengukuran.
Visual: Perbandingan tangga kasar (3-bit) vs tangga halus (12-bit).
```

### Slide 6: Jenis-Jenis ADC
**Prompt:**
```
Judul: "Arsitektur ADC Umum"
Konten:
1. Flash ADC: Sangat cepat, mahal, resolusi rendah (Oscilloscope).
2. Sigma-Delta: Resolusi sangat tinggi (24-bit), lambat (Audio, Load cell).
3. SAR (Successive Approximation Register): Seimbang, umum di Mikrokontroler (STM32/ESP32).
Visual: Diagram blok sederhana SAR ADC (Comparator, DAC, SAR logic).
```

### Slide 7: ADC pada STM32F103
**Prompt:**
```
Judul: "Fitur ADC STM32F103 (Blue Pill)"
Konten:
- Tipe: 12-bit SAR ADC.
- Jumlah Channel: 10 Ch Eksternal + Sensor Suhu Internal.
- Speed: 1 us conversion time.
- Mode: Single, Continuous, Scan (Multi-channel), Injected.
- Data Alignment: Right Aligned (Default) vs Left Aligned.
Visual: Pinout STM32 dengan highlight pin ADC (PA0-PA7, PB0-PB1).
```

### Slide 8: ADC pada ESP32
**Prompt:**
```
Judul: "Fitur ADC ESP32"
Konten:
- Tipe: 2x 12-bit SAR ADC (ADC1 & ADC2).
- ADC1: 8 Channel (GPIO 32-39). Aman dipakai.
- ADC2: Digunakan oleh WiFi. JANGAN pakai saat WiFi nyala.
- Attenuation: Programmable gain untuk input range (0-1V, 0-1.5V, 0-3.3V).
```

### Slide 9: Karakteristik Non-Linear ESP32
**Prompt:**
```
Judul: "Isu Linearitas ESP32"
Konten:
- Masalah: ADC ESP32 tidak linear sempurna di ujung bawah (<0.1V) dan atas (>3.1V).
- Konsekuensi: Nilai kecil terbaca 0, nilai tinggi saturasi cepat.
- Solusi: Kalibrasi software (Polynomial fitting) atau hindari range ekstrem.
Visual: Kurva respons ideal (garis lurus) vs kurva ESP32 (lengkung di ujung).
```

### Slide 10: Rumus Konversi
**Prompt:**
```
Judul: "Dari Digital Kembali ke Fisik"
Konten:
- Voltage = (RawValue / MaxADC) * Vref
- Contoh: Bacaan 2048 pada 12-bit ADC (3.3V)
  Voltage = (2048 / 4095) * 3.3V = 1.65V.
- Sensor Scaling:
  Suhu = (Voltage - V_zero) / Scale_factor
Visual: Rumus matematika besar dan jelas.
```

