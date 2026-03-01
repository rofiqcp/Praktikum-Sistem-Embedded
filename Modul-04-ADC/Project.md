# Project Modul 04: Sistem Monitoring Kualitas Udara — ADC

## Informasi Project

| Item | Keterangan |
|------|------------|
| Modul | 04 — ADC (Analog-to-Digital Converter) |
| Platform | STM32F103C8T6 + ESP32 DevKit V1 |
| Durasi | 2 minggu |
| Tipe | Dual-MCU monitoring analog multi-channel |

---

## Deskripsi Umum

Project ini mengembangkan seluruh konsep dari 12 percobaan praktikum ADC ke dalam sebuah sistem terintegrasi **Air Quality Monitoring System**. Mahasiswa harus menggabungkan single read, multi-channel, moving average filter, kalibrasi, continuous DMA, threshold alert, battery monitor, sensor suhu internal, sampling rate analysis, dan statistical analysis ke dalam satu sistem yang fungsional.

---

## Soal Cerita

### Skenario: Sistem Monitoring Udara Gedung Perkantoran Hijau

Sebuah perusahaan startup "GreenAir Indonesia" mendapatkan kontrak untuk memasang sistem monitoring kualitas udara di gedung perkantoran ramah lingkungan. Gedung ini memiliki 3 lantai, dan setiap lantai perlu dipantau untuk parameter: suhu ruangan, kecerahan cahaya (untuk kontrol lampu otomatis), level gas CO₂ (disimulasikan dengan potensiometer), dan tegangan baterai backup UPS.

**ESP32 DevKit V1 (Monitoring Hub)** dipasang di setiap lantai sebagai unit monitoring utama. ESP32 menggunakan **ADC multi-channel** untuk membaca beberapa sensor secara bersamaan. Data mentah dari ADC di-filter menggunakan **moving average** agar stabil, kemudian di-**kalibrasi** menggunakan fungsi kalibrasi ADC bawaan ESP32 untuk mendapatkan nilai tegangan akurat. ESP32 juga membaca **sensor suhu internal** chip untuk monitoring kondisi perangkat itu sendiri.

Dalam mode operasi normal, ESP32 membaca semua channel secara periodik setiap 1 detik menggunakan mode **single read**. Namun jika terdeteksi anomali (gas level tinggi), sistem beralih ke mode **continuous DMA** untuk mendapatkan sampling rate tinggi dan menangkap perubahan cepat. Data dikirim ke Serial Monitor dalam format tabel terstruktur dengan **statistik lengkap** (min, max, average, standar deviasi).

**STM32F103C8T6 (Alert Controller)** berfungsi sebagai unit peringatan dan logging. STM32 menerima data analog dari sensor cadangan melalui ADC-nya sendiri dan menjalankan **threshold alert system** — jika suhu > 35°C atau gas level > 70%, LED merah menyala dan buzzer berbunyi. STM32 juga memonitor **tegangan baterai backup** menggunakan voltage divider dan memberikan peringatan jika tegangan drop di bawah 3.2V.

Kedua mikrokontroler melaporkan data ke komputer melalui **UART** untuk ditampilkan di Serial Monitor. Sistem harus mampu menampilkan **sampling rate** aktual dan melakukan **analisis statistik** untuk menilai kualitas udara secara keseluruhan.

---

## Spesifikasi Teknis

### Mapping Percobaan ke Fitur Sistem

| No | Percobaan Praktikum | Fitur dalam Project |
|----|---------------------|---------------------|
| P01 | ADC Single Read | Pembacaan dasar setiap sensor secara periodik (mode normal) |
| P02 | Voltage Display | Konversi raw ADC ke tegangan untuk semua channel, ditampilkan di serial |
| P03 | Moving Average Filter | Filter 10-sample untuk semua channel sensor agar pembacaan stabil |
| P04 | Multi Channel | Pembacaan simultan 4 channel: suhu, cahaya, gas, baterai |
| P05 | ADC Calibration | Kalibrasi ESP32 ADC menggunakan eFuse untuk akurasi tegangan |
| P06 | Continuous DMA | Mode high-speed sampling saat anomali terdeteksi (gas tinggi) |
| P07 | Threshold Alert | Alarm LED + buzzer jika parameter melebihi batas aman |
| P08 | Battery Monitor | Monitoring tegangan baterai backup via voltage divider |
| P09 | Temperature Internal | Monitoring suhu chip ESP32/STM32 untuk deteksi overheating |
| P10 | Sampling Rate Test | Pengukuran dan pelaporan sampling rate aktual sistem |
| P11 | Light Sensor (LDR) | Pembacaan sensor cahaya untuk kontrol pencahayaan |
| P12 | Statistical Analysis | Perhitungan min, max, avg, stddev untuk setiap parameter |

### Pin Configuration

**ESP32:**
| Pin | Fungsi | Sensor/Komponen |
|-----|--------|----------------|
| GPIO34 (ADC1_CH6) | Suhu (LM35/potensiometer) | Input analog 0–3.3V |
| GPIO35 (ADC1_CH7) | Cahaya (LDR + divider) | Input analog 0–3.3V |
| GPIO32 (ADC1_CH4) | Gas level (potensiometer) | Simulasi sensor gas |
| GPIO33 (ADC1_CH5) | Tegangan baterai (divider) | Monitoring baterai |
| GPIO2 | LED alert (merah) | Output digital |
| GPIO4 | Buzzer | Output digital |

**STM32:**
| Pin | Fungsi | Sensor/Komponen |
|-----|--------|----------------|
| PA0 (ADC1_CH0) | Suhu | Input analog |
| PA1 (ADC1_CH1) | Gas level | Input analog |
| PA4 (ADC1_CH4) | Tegangan baterai | Voltage divider |
| PB0 | LED alert (merah) | Output |
| PB1 | Buzzer | Output |
| PC13 | LED status | Built-in LED |

### Mode Operasi

```
    ┌─────────────────┐
    │   MODE NORMAL   │ ◄── Sampling setiap 1 detik
    │  (Single Read)  │     Filter moving average
    └────────┬────────┘     Display tabel periodik
             │
             │ Gas level > 70% atau Temp > 35°C
             ▼
    ┌─────────────────┐
    │  MODE ANOMALI   │ ◄── Sampling continuous (DMA)
    │  (High Speed)   │     LED merah + buzzer ON
    └────────┬────────┘     Logging intensif
             │
             │ Semua parameter kembali normal selama 30 detik
             ▼
    ┌─────────────────┐
    │   MODE NORMAL   │
    └─────────────────┘
```

---

## Ketentuan Pengerjaan

1. Kedua MCU harus membaca sensor analog dan menampilkan data ke Serial Monitor.
2. Data harus difilter (moving average minimal 10 sample).
3. Threshold alert harus aktif pada kedua platform.
4. ESP32 harus bisa beralih antara mode normal dan continuous DMA.
5. Statistik (min, max, avg, stddev) harus ditampilkan periodik.
6. Kode harus modular — fungsi ADC, filter, alert, display di file terpisah.

---

## Rubrik Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | ADC multi-channel berfungsi (4 channel) | 20% |
| 2 | Filter moving average + kalibrasi | 15% |
| 3 | Threshold alert (LED + buzzer) | 15% |
| 4 | Mode switching (normal ↔ continuous DMA) | 15% |
| 5 | Battery monitor + suhu internal | 10% |
| 6 | Statistical analysis (min/max/avg/stddev) | 10% |
| 7 | Kode modular dan dokumentasi | 10% |
| 8 | Demo video | 5% |

---

## Referensi

1. STM32F103xx Reference Manual (RM0008) — Chapter 11: ADC
2. ESP-IDF Programming Guide — ADC Oneshot & Continuous Mode
3. AN2834 — How to get the best ADC accuracy in STM32 microcontrollers
4. Kolban's Book on ESP32 — ADC Chapter
