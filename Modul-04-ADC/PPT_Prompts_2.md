# Prompt untuk Pembuatan PPT - Bagian 2: Praktikum & Project ADC
## Modul 04: Implementasi & Project BMS

---

## 📋 Informasi Presentasi

| Item | Keterangan |
|------|------------|
| **Topik** | Praktikum Lab & Project Brief |
| **Jumlah Slide** | 15 Slide |
| **Durasi** | 30 menit |
| **Fokus** | Wiring, Coding, Project BMS |

---

## 📊 Struktur Slide Detail

### Slide 1: Cover Praktikum
**Prompt:**
```
Judul: "Praktikum Modul 04: ADC Implementation"
Subtitle: "STM32 & ESP32 Analog Reading"
Visual: Breadboard dengan Potensiometer dan LDR terhubung ke STM32.
```

### Slide 2: Rangkaian Percobaan Dasar
**Prompt:**
```
Judul: "Percobaan 1: Potentiometer Reading"
Konten:
- Potensiometer sebagai Voltage Divider.
- Kaki 1 -> 3.3V, Kaki 3 -> GND, Kaki 2 (Wiper) -> Pin ADC.
- Pin ADC: PA0 (STM32) atau GPIO34 (ESP32).
Warning: Kaki wiper jangan langsung ke VCC/GND tanpa resistansi hambatan!
Visual: Diagram Fritzing sederhana.
```

### Slide 3: Code Snippet - Basic Read
**Prompt:**
```
Judul: "Kode Dasar: analogRead()"
Konten:
- Setup: analogReadResolution(12);
- Loop: int val = analogRead(PIN);
- Print: Serial.println(val);
- Penjelasan: Nilai 0 - 4095 merepresentasikan 0 - 3.3V.
```

### Slide 4: Isu Sinyal Noise
**Prompt:**
```
Judul: "Masalah: Noise Sinyal Analog"
Konten:
- Sinyal analog rentan gangguan listrik.
- Gejala: Jitter pada pembacaan (misal: 2045, 2050, 2042 berubah-ubah cepat).
- Penyebab: Kabel panjang, decoupling kurang, power supply ripple.
Visual: Grafik sinyal "kotor" penuh spike.
```

### Slide 5: Digital Filtering - Moving Average
**Prompt:**
```
Judul: "Solusi: Moving Average Filter"
Konten:
- Konsep: Mengambil N sampel rata-rata.
- Rumus: Average = (Sample1 + ... + SampleN) / N.
- Trade-off: Semakin banyak N, semakin halus (smooth) tapi respon makin lambat (lag).
- Coding: Loop for N times -> Sum -> Divide.
Visual: Grafik sinyal kotor vs sinyal hasil filter yang mulus.
```

### Slide 6: Overview Project BMS
**Prompt:**
```
Judul: "Project: Smart Battery Monitor"
Konten:
- Tujuan: Memonitor tegangan dan arus baterai (Simulasi).
- Arsitektur:
  1. STM32: Sensor Reader (Precise ADC).
  2. ESP32: WiFi Gateway & Alert Logic.
- Komunikasi: UART (seperti Modul 03).
Visual: Blok diagram STM32 (connected to sensors) -> UART -> ESP32 -> Web Dashboard.
```

### Slide 7: Rangkaian Simulasi Sensor
**Prompt:**
```
Judul: "Wiring Project BMS"
Konten:
- Sensor Tegangan: Simulasi pakai Potensio 1 (0-3.3V merepresentasikan 0-15V).
- Sensor Arus: Simulasi pakai Potensio 2 (Tengah 1.65V = 0 Ampere).
- Sensor Suhu: LM35 / NTC ke PA2.
- Alert: LED Merah & Hijau di ESP32.
Visual: Skematik lengkap interkoneksi.
```

### Slide 8: Kalibrasi Tegangan (Voltage Divider)
**Prompt:**
```
Judul: "Teori: Sensor Tegangan (Voltage Divider)"
Konten:
- Baterai 12V tidak bisa masuk langsung ke ADC 3.3V.
- Solusi: Pembagi Tegangan (R1 & R2).
- V_out = V_bat * R2 / (R1 + R2).
- V_bat = V_out * (R1 + R2) / R2.
- V_out masuk ke ADC. MCU menghitung balik V_bat.
Visual: Rangkaian R1-R2 dengan rumus perhitungan.
```

### Slide 9: Logika Alert (ESP32)
**Prompt:**
```
Judul: "Logika Monitoring & Alert"
Konten:
- Kritis (< 10.5V): LED Blink Cepat, Buzzer On.
- Low (< 11.5V): LED Merah Nyala.
- Normal (> 11.5V): LED Hijau Nyala.
- Charging: Deteksi jika Arus Negatif.
Visual: Flowchart logika `if-else` untuk penentuan status baterai.
```

### Slide 10: Tantangan Pengembang
**Prompt:**
```
Judul: "Tantangan Kualitas Data"
Konten:
1. Akurasi: Apakah nilai volt di Serial Monitor sama dengan Multimeter?
2. Stabilitas: Apakah angka melompat-lompat saat diam?
3. Responsivitas: Seberapa cepat alarm berbunyi saat voltase drop?
4. Kalibrasi: Menemukan faktor pengali tegangan yang tepat.
```

