# Jobsheet Modul 04: ADC — Analog to Digital Conversion

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami prinsip kerja ADC** — Menjelaskan proses konversi analog ke digital meliputi tahapan sampling, kuantisasi, dan encoding serta parameter penting seperti resolusi, atenuasi, dan referensi tegangan.
2. **Mengkonfigurasi dan membaca ADC pada ESP32 dan STM32** — Menggunakan API ESP-IDF (`adc1_get_raw()`, `esp_adc_cal`) dan HAL STM32 (`HAL_ADC_Start()`, `HAL_ADC_GetValue()`) untuk membaca nilai analog dari sensor dan potensiometer.
3. **Menerapkan teknik pemrosesan sinyal ADC** — Mengimplementasikan filter moving average, kalibrasi ADC, pembacaan multi-channel, dan analisis statistik untuk meningkatkan akurasi dan keandalan hasil pembacaan ADC.
4. **Merancang sistem monitoring berbasis ADC** — Membangun aplikasi monitoring tegangan baterai, sensor cahaya (LDR), sensor suhu internal, dan sistem peringatan berbasis ambang batas (threshold alert) menggunakan ADC.
5. **Menganalisis performa ADC** — Mengukur kecepatan sampling, mengevaluasi linearitas, menghitung SNR (Signal-to-Noise Ratio), dan menggunakan mode continuous DMA untuk akuisisi data berkecepatan tinggi.

---

## 2. Peralatan dan Komponen

### 2.1 Perangkat Keras

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1  | ESP32 DevKit V1 | 1 | Mikrokontroler utama (ADC 12-bit) |
| 2  | STM32 Blue Pill (STM32F103C8T6) | 1 | Mikrokontroler pembanding (ADC 12-bit) |
| 3  | ST-Link V2 | 1 | Programmer untuk STM32 |
| 4  | Kabel USB Micro-B | 2 | Untuk ESP32 dan ST-Link |
| 5  | Breadboard 830 titik | 1 | Papan rangkaian |
| 6  | Potensiometer 10 kΩ | 2 | Sumber tegangan variabel |
| 7  | LDR (Light Dependent Resistor) | 1 | Sensor cahaya |
| 8  | Resistor 10 kΩ | 3 | Untuk voltage divider dan pull-down |
| 9  | LED 5mm (merah/hijau) | 2 | Indikator threshold alert |
| 10 | Resistor 330 Ω | 2 | Pembatas arus LED |
| 11 | Multimeter digital | 1 | Untuk verifikasi tegangan |
| 12 | Kabel jumper male-male | 20 | Koneksi antar komponen |
| 13 | Kabel jumper male-female | 10 | Koneksi ke modul |
| 14 | Baterai Li-Ion 3.7V (opsional) | 1 | Untuk percobaan battery monitor |

### 2.2 Perangkat Lunak

| No | Software | Keterangan |
|----|----------|------------|
| 1  | VS Code + PlatformIO | IDE pengembangan |
| 2  | Serial Monitor (115200 baud) | Menampilkan output program |
| 3  | Python 3.x + matplotlib | Untuk script debug_analysis.py |
| 4  | Driver CP210x / CH340 | Driver USB-to-Serial |

---

## 3. Teori Singkat

### 3.1 Konsep Dasar ADC

Analog to Digital Converter (ADC) merupakan komponen penting dalam sistem embedded yang berfungsi mengubah sinyal analog kontinu menjadi representasi digital diskret. Proses konversi berlangsung dalam tiga tahap utama: **sampling** (pencuplikan nilai analog pada interval waktu tertentu), **kuantisasi** (pemetaan nilai kontinu ke level diskret berdasarkan resolusi bit), dan **encoding** (konversi level kuantisasi menjadi kode biner). ADC pada ESP32 memiliki resolusi 12-bit (rentang 0–4095) dengan tegangan referensi 3.3V, mendukung 18 channel melalui ADC1 (8 channel, GPIO32–39) dan ADC2 (10 channel). STM32F103 juga memiliki ADC 12-bit dengan 10 channel eksternal dan 2 channel internal (sensor suhu dan Vrefint). Rumus konversi dasar: $V_{analog} = \frac{raw \times V_{ref}}{2^n - 1}$, di mana $n$ adalah resolusi bit.

### 3.2 Fitur ADC pada ESP32 dan STM32

ESP32 menyediakan fitur atenuasi untuk memperluas rentang input (0 dB: 0–1.1V, 2.5 dB: 0–1.5V, 6 dB: 0–2.2V, 11 dB: 0–3.3V) serta mendukung kalibrasi menggunakan eFuse untuk kompensasi non-linearitas. ESP32 juga mendukung mode continuous dengan DMA untuk akuisisi data berkecepatan tinggi. STM32F103 memiliki fitur **Analog Watchdog** yang dapat mendeteksi ketika nilai ADC melampaui batas threshold secara hardware, mode scan untuk pembacaan multi-channel otomatis, serta dukungan DMA (Direct Memory Access) untuk transfer data ADC ke memori tanpa intervensi CPU. Kedua mikrokontroler mendukung pembacaan sensor suhu internal — ESP32 melalui sensor suhu yang terintegrasi di chip, sedangkan STM32 melalui channel ADC internal khusus (channel 16).

---

## 4. Langkah Percobaan

> **Catatan Umum:**
> - Pastikan Serial Monitor diatur ke **115200 baud**
> - Gunakan multimeter untuk memverifikasi tegangan pada pin ADC
> - Setiap program tersedia dalam dua versi: ESP32 (`praktikum/ESP32/`) dan STM32 (`praktikum/STM32/`)
> - Untuk STM32, gunakan ST-Link V2 sebagai programmer
> - Hubungkan VCC potensiometer/sensor ke **3.3V** (BUKAN 5V)

---

### Percobaan 01: ADC Single Read — Pembacaan ADC Tunggal

**Tujuan:** Memahami cara membaca nilai mentah (raw) ADC dari satu channel menggunakan mode single-shot.

#### Rangkaian

**Koneksi Potensiometer:**

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. pin 1 (VCC) | 3.3V | 3.3V |
| Pot. pin 2 (Wiper/Output) | GPIO34 (ADC1_CH6) | PA0 (ADC1_CH0) |
| Pot. pin 3 (GND) | GND | GND |
| UART TX | — (USB) | PA9 |
| UART RX | — (USB) | PA10 |

```
Potensiometer 10kΩ:

  3.3V ──[Pin 1]
                 │
          [Wiper/Pin 2] ── GPIO34 (ESP32) / PA0 (STM32)
                 │
   GND ──[Pin 3]
```

#### Langkah Kerja

1. Pasang potensiometer pada breadboard, hubungkan pin 1 ke 3.3V, pin 3 ke GND, dan pin 2 (wiper) ke **GPIO34** (ESP32) atau **PA0** (STM32).
2. Buka folder project `ESP32_01_ADC_Single_Read` atau `STM32_01_ADC_Single_Read` di PlatformIO.
3. Pelajari kode sumber pada `src/main.c` — perhatikan konfigurasi resolusi ADC (12-bit) dan atenuasi (11 dB).
4. Compile dan upload program ke board.
5. Buka Serial Monitor (115200 baud).
6. Putar potensiometer dari posisi minimum ke maksimum secara perlahan.
7. Amati perubahan nilai raw ADC pada Serial Monitor.
8. Gunakan multimeter untuk mengukur tegangan pada pin wiper dan bandingkan dengan nilai yang ditampilkan.

#### Pengamatan

Catat dalam tabel berikut:

| Posisi Pot. | Tegangan Multimeter (V) | Nilai Raw ADC | Persentase (%) |
|-------------|------------------------|---------------|----------------|
| Minimum (0°) | | | |
| 25% (~90°) | | | |
| 50% (~180°) | | | |
| 75% (~270°) | | | |
| Maksimum (300°) | | | |

#### Pertanyaan

1. Berapa rentang nilai raw ADC yang Anda amati? Apakah sesuai dengan resolusi 12-bit (0–4095)?
2. Apakah nilai ADC berubah linear terhadap posisi potensiometer? Jelaskan!
3. Apa fungsi `adc1_config_width()` dan `adc1_config_channel_atten()` pada ESP32?

---

### Percobaan 02: ADC Voltage Display — Tampilan Tegangan ADC

**Tujuan:** Mengkonversi nilai raw ADC menjadi tegangan (mV) dan menampilkannya melalui Serial Monitor.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. Wiper | GPIO34 (ADC1_CH6) | PA0 (ADC1_CH0) |
| Pot. VCC | 3.3V | 3.3V |
| Pot. GND | GND | GND |

> Rangkaian sama dengan Percobaan 01.

#### Langkah Kerja

1. Gunakan rangkaian yang sama dengan Percobaan 01.
2. Buka project `ESP32_02_ADC_Voltage_Display` atau `STM32_02_ADC_Voltage_Display`.
3. Perhatikan formula konversi: `voltage_mV = (raw * 3300) / 4095`.
4. Compile, upload, dan buka Serial Monitor.
5. Putar potensiometer dan amati nilai tegangan dalam satuan mV yang ditampilkan.
6. Verifikasi ketepatan konversi dengan mengukur tegangan menggunakan multimeter.
7. Hitung persentase error antara nilai ADC dan pembacaan multimeter.

#### Pengamatan

| Posisi Pot. | Nilai Raw | Tegangan ADC (mV) | Tegangan Multimeter (mV) | Error (%) |
|-------------|-----------|-------------------|--------------------------|-----------|
| Minimum | | | | |
| 25% | | | | |
| 50% | | | | |
| 75% | | | | |
| Maksimum | | | | |

**Rumus error:** $Error(\%) = \frac{|V_{ADC} - V_{multimeter}|}{V_{multimeter}} \times 100\%$

#### Pertanyaan

1. Berapa rata-rata error konversi ADC terhadap pembacaan multimeter?
2. Pada rentang tegangan berapa error paling besar? Mengapa demikian?
3. Apa perbedaan pendekatan konversi tegangan antara ESP32 (menggunakan `esp_adc_cal`) dan STM32 (rumus manual)?

---

### Percobaan 03: ADC Moving Average — Filter Rata-Rata Bergerak

**Tujuan:** Menerapkan filter digital moving average untuk menghaluskan pembacaan ADC yang berfluktuasi (noisy).

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. Wiper | GPIO34 | PA0 |
| Pot. VCC | 3.3V | 3.3V |
| Pot. GND | GND | GND |

> Rangkaian sama dengan Percobaan 01.

#### Langkah Kerja

1. Gunakan rangkaian potensiometer yang sama.
2. Buka project `ESP32_03_ADC_Moving_Average` atau `STM32_03_ADC_Moving_Average`.
3. Pelajari implementasi buffer circular dan algoritma moving average pada kode sumber.
4. Compile, upload, dan buka Serial Monitor.
5. Amati dua kolom output: **Raw** (nilai mentah) dan **Filtered** (nilai setelah filter).
6. Biarkan potensiometer pada posisi tetap selama 30 detik — bandingkan fluktuasi raw vs filtered.
7. Putar potensiometer secara cepat — amati bagaimana filter merespons perubahan mendadak.
8. Coba ubah ukuran window filter pada kode (misal dari 10 menjadi 20 atau 5), re-upload, dan bandingkan hasilnya.

#### Pengamatan

**Posisi potensiometer tetap (50%), 20 sampel:**

| Parameter | Nilai Raw | Nilai Filtered |
|-----------|-----------|----------------|
| Nilai minimum | | |
| Nilai maksimum | | |
| Rata-rata | | |
| Standar deviasi | | |
| Rentang (max−min) | | |

#### Pertanyaan

1. Berapa persen pengurangan noise (fluktuasi) setelah filter moving average diterapkan?
2. Apa trade-off antara ukuran window filter yang besar vs kecil?
3. Bagaimana respons filter terhadap perubahan mendadak (step response)?

---

### Percobaan 04: ADC Multi Channel — Pembacaan Multi Channel

**Tujuan:** Membaca dua channel ADC secara berurutan (sekuensial) dari dua sumber analog yang berbeda.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. 1 Wiper | GPIO34 (ADC1_CH6) | PA0 (ADC1_CH0) |
| Pot. 2 Wiper | GPIO35 (ADC1_CH7) | PA1 (ADC1_CH1) |
| Kedua Pot. VCC | 3.3V | 3.3V |
| Kedua Pot. GND | GND | GND |

```
  3.3V ──┬── [Pot.1 Pin1]     3.3V ──┬── [Pot.2 Pin1]
         │       │                    │       │
         │   [Wiper] → GPIO34/PA0    │   [Wiper] → GPIO35/PA1
         │       │                    │       │
   GND ──┴── [Pot.1 Pin3]     GND ──┴── [Pot.2 Pin3]
```

#### Langkah Kerja

1. Pasang **dua buah** potensiometer pada breadboard.
2. Hubungkan wiper potensiometer 1 ke **GPIO34** (ESP32) atau **PA0** (STM32).
3. Hubungkan wiper potensiometer 2 ke **GPIO35** (ESP32) atau **PA1** (STM32).
4. Buka project `ESP32_04_ADC_Multi_Channel` atau `STM32_04_ADC_Multi_Channel`.
5. Compile, upload, dan buka Serial Monitor.
6. Putar masing-masing potensiometer secara bergantian — amati bahwa kedua channel terbaca independen.
7. Atur kedua potensiometer ke posisi yang sama — bandingkan nilai pembacaan keduanya.

#### Pengamatan

| Kondisi | CH1 Raw | CH1 Voltage (mV) | CH2 Raw | CH2 Voltage (mV) |
|---------|---------|-------------------|---------|-------------------|
| Pot1=Min, Pot2=Min | | | | |
| Pot1=Max, Pot2=Min | | | | |
| Pot1=Min, Pot2=Max | | | | |
| Pot1=Max, Pot2=Max | | | | |
| Pot1=50%, Pot2=50% | | | | |

#### Pertanyaan

1. Apakah pembacaan dua channel saling mempengaruhi (crosstalk)? Jelaskan pengamatan Anda.
2. Bagaimana STM32 melakukan channel switching? Apa yang dilakukan `HAL_ADC_ConfigChannel()` pada STM32?
3. Apa perbedaan ADC1 dan ADC2 pada ESP32? Mengapa ADC2 bermasalah saat WiFi aktif?

---

### Percobaan 05: ADC Calibration — Kalibrasi ADC

**Tujuan:** Memahami dan menerapkan kalibrasi ADC untuk meningkatkan akurasi pembacaan menggunakan data kalibrasi eFuse (ESP32) dan kalibrasi manual (STM32).

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. Wiper | GPIO34 | PA0 |
| Pot. VCC | 3.3V | 3.3V |
| Pot. GND | GND | GND |

> Rangkaian sama dengan Percobaan 01.

#### Langkah Kerja

1. Gunakan rangkaian potensiometer yang sama.
2. Buka project `ESP32_05_ADC_Calibration` atau `STM32_05_ADC_Calibration`.
3. Pada kode ESP32, perhatikan penggunaan `esp_adc_cal_characterize()` dan `esp_adc_cal_raw_to_voltage()`.
4. Compile, upload, dan buka Serial Monitor.
5. Amati output yang menampilkan: nilai raw, tegangan tanpa kalibrasi, dan tegangan setelah kalibrasi.
6. Atur potensiometer ke beberapa posisi tetap — gunakan multimeter sebagai referensi.
7. Bandingkan error antara nilai **tanpa kalibrasi** dan **dengan kalibrasi**.
8. Catat jenis kalibrasi yang terdeteksi (eFuse Two Point, eFuse Vref, atau Default Vref).

#### Pengamatan

| Posisi | V Multimeter (mV) | V Tanpa Kalibrasi (mV) | V Dengan Kalibrasi (mV) | Error Tanpa (%) | Error Dengan (%) |
|--------|-------------------|------------------------|--------------------------|-----------------|------------------|
| 25% | | | | | |
| 50% | | | | | |
| 75% | | | | | |
| 100% | | | | | |

#### Pertanyaan

1. Jenis kalibrasi apa yang terdeteksi pada board ESP32 Anda? (Two Point / Vref / Default)
2. Berapa peningkatan akurasi (penurunan error) setelah kalibrasi diterapkan?
3. Mengapa kalibrasi penting dalam aplikasi pengukuran presisi?

---

### Percobaan 06: ADC Continuous DMA — ADC Berkelanjutan dengan DMA

**Tujuan:** Menggunakan mode ADC continuous dengan DMA (Direct Memory Access) untuk akuisisi data berkecepatan tinggi tanpa intervensi CPU.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. Wiper | GPIO34 | PA0 |
| Pot. VCC | 3.3V | 3.3V |
| Pot. GND | GND | GND |

> Rangkaian sama dengan Percobaan 01.

#### Langkah Kerja

1. Gunakan rangkaian potensiometer standar.
2. Buka project `ESP32_06_ADC_Continuous_DMA` atau `STM32_06_ADC_Continuous_DMA`.
3. Pelajari konfigurasi DMA pada kode — perhatikan buffer size, sampling frequency, dan callback.
4. Pada STM32, perhatikan penggunaan `HAL_ADC_Start_DMA()` dan `HAL_ADC_ConvCpltCallback()`.
5. Compile, upload, dan buka Serial Monitor.
6. Amati kecepatan pengiriman data — bandingkan dengan mode single-shot (Percobaan 01).
7. Putar potensiometer secara cepat — perhatikan apakah ada data yang terlewat.
8. Perhatikan penggunaan CPU saat mode DMA aktif vs polling manual.

#### Pengamatan

| Parameter | Single-Shot (Prg.01) | Continuous DMA (Prg.06) |
|-----------|----------------------|-------------------------|
| Jumlah sampel per detik | | |
| Apakah ada sampel terlewat? | | |
| Beban CPU (estimasi) | | |
| Latency respons | | |

#### Pertanyaan

1. Apa keuntungan menggunakan DMA dibandingkan polling dalam pembacaan ADC?
2. Berapa ukuran buffer DMA yang digunakan? Apa yang terjadi jika buffer terlalu kecil?
3. Jelaskan mekanisme interrupt callback pada mode DMA!

---

### Percobaan 07: ADC Threshold Alert — Peringatan Ambang Batas

**Tujuan:** Membuat sistem peringatan berbasis ambang batas ADC yang menyalakan LED ketika nilai ADC melebihi threshold tertentu.

#### Rangkaian

**ESP32:**

| Komponen | Pin ESP32 |
|----------|-----------|
| Pot. Wiper | GPIO34 (ADC1_CH6) |
| LED (built-in) | GPIO2 |
| Pot. VCC | 3.3V |
| Pot. GND | GND |

**STM32:**

| Komponen | Pin STM32 |
|----------|-----------|
| Pot. Wiper | PA0 (ADC1_CH0) |
| LED (built-in) | PC13 (active low) |
| Pot. VCC | 3.3V |
| Pot. GND | GND |

```
  3.3V ──[Pot Pin1]
               │
         [Wiper] ── GPIO34 (ESP32) / PA0 (STM32)
               │
   GND ──[Pot Pin3]

  LED built-in: GPIO2 (ESP32) / PC13 (STM32, active low)
```

> **Opsional:** Hubungkan LED eksternal dengan resistor 330Ω ke GPIO yang tersedia untuk indikator tambahan.

#### Langkah Kerja

1. Pasang potensiometer pada breadboard dengan wiper ke pin ADC.
2. Buka project `ESP32_07_ADC_Threshold_Alert` atau `STM32_07_ADC_Threshold_Alert`.
3. Perhatikan nilai threshold pada kode: `LOW_THRESHOLD = 1000` (~0.8V) dan `HIGH_THRESHOLD = 3000` (~2.4V).
4. Compile, upload, dan buka Serial Monitor.
5. Putar potensiometer secara perlahan dari minimum ke maksimum.
6. Amati kapan LED menyala dan kapan pesan peringatan muncul di Serial Monitor.
7. Amati klasifikasi level: **LOW**, **NORMAL**, **HIGH**.
8. Pada STM32, perhatikan penggunaan fitur **Analog Watchdog** (`HAL_ADC_AnalogWDGConfig()`).

#### Pengamatan

| Rentang ADC | Tegangan Perkiraan | Klasifikasi | Status LED |
|-------------|-------------------|-------------|------------|
| 0 – 999 | 0 – 0.8V | LOW | |
| 1000 – 2999 | 0.8 – 2.4V | NORMAL | |
| 3000 – 4095 | 2.4 – 3.3V | HIGH | |

#### Pertanyaan

1. Apa keuntungan menggunakan Analog Watchdog (STM32) dibandingkan pengecekan threshold secara software?
2. Bagaimana cara mengubah nilai threshold agar cocok untuk aplikasi spesifik?
3. Sebutkan contoh aplikasi nyata yang memerlukan threshold alert pada ADC!

---

### Percobaan 08: ADC Battery Monitor — Monitor Baterai

**Tujuan:** Membangun sistem monitoring tegangan baterai Li-Ion menggunakan voltage divider dan ADC.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Titik tengah divider | GPIO34 (ADC1_CH6) | PA0 (ADC1_CH0) |
| R1 (10 kΩ) | Dari V_bat ke titik tengah | Dari V_bat ke titik tengah |
| R2 (10 kΩ) | Dari titik tengah ke GND | Dari titik tengah ke GND |

```
  V_bat ──[R1 = 10kΩ]──┬──[R2 = 10kΩ]── GND
                        │
                    GPIO34 / PA0

  V_adc = V_bat × R2 / (R1 + R2) = V_bat / 2
  V_bat = V_adc × 2
```

> **PERINGATAN:** Jika tidak memiliki baterai, simulasikan dengan potensiometer yang terhubung langsung ke pin ADC (tanpa voltage divider). Nilai tegangan yang ditampilkan akan sesuai rentang 0–3.3V.

#### Langkah Kerja

1. Rangkai voltage divider dengan dua resistor 10 kΩ pada breadboard.
2. Hubungkan sumber tegangan (baterai atau potensiometer) ke R1.
3. Hubungkan titik tengah ke **GPIO34** (ESP32) atau **PA0** (STM32).
4. Buka project `ESP32_08_ADC_Battery_Monitor` atau `STM32_08_ADC_Battery_Monitor`.
5. Compile, upload, dan buka Serial Monitor.
6. Amati tampilan: tegangan baterai, persentase, dan status (Full/Normal/Low/Critical).
7. Jika menggunakan potensiometer, putar untuk mensimulasikan tegangan baterai yang menurun.
8. Catat pada tegangan berapa status berubah (misal Full → Normal → Low → Critical).

#### Pengamatan

| Tegangan Baterai (V) | Persentase (%) | Status | Keterangan |
|----------------------|----------------|--------|------------|
| 4.2 | | Full | Baterai penuh |
| 3.9 | | | |
| 3.7 | | | Tegangan nominal |
| 3.5 | | | |
| 3.3 | | | |
| 3.0 | | Critical | Baterai habis |

#### Pertanyaan

1. Mengapa diperlukan voltage divider untuk monitoring baterai Li-Ion? Apa yang terjadi jika tegangan baterai langsung masuk ke pin ADC?
2. Bagaimana rumus perhitungan persentase baterai dari tegangan yang terbaca?
3. Apa kelemahan estimasi persentase secara linear? Bagaimana kurva discharge Li-Ion yang sebenarnya?

---

### Percobaan 09: ADC Temperature Internal — Sensor Suhu Internal

**Tujuan:** Membaca sensor suhu internal yang terintegrasi dalam chip ESP32/STM32 tanpa memerlukan sensor eksternal.

#### Rangkaian

> **Tidak ada koneksi hardware eksternal.** Percobaan ini menggunakan sensor suhu internal yang sudah ada di dalam chip mikrokontroler.

| Platform | Keterangan |
|----------|------------|
| ESP32 | Menggunakan API `temp_sensor` (ESP32-S2/S3) atau `esp_adc_cal` (ESP32 original) |
| STM32 | Membaca ADC channel 16 (internal temperature sensor) |

#### Langkah Kerja

1. **Tidak perlu merangkai komponen** — sensor suhu sudah built-in dalam chip.
2. Buka project `ESP32_09_ADC_Temperature_Internal` atau `STM32_09_ADC_Temperature_Internal`.
3. Pada STM32, perhatikan bahwa sensor suhu menggunakan ADC channel internal (channel 16).
4. Compile, upload, dan buka Serial Monitor.
5. Amati suhu chip yang ditampilkan dalam °C.
6. Sentuh chip mikrokontroler dengan jari selama 30 detik — amati kenaikan suhu.
7. Tiup chip dengan udara dingin — amati penurunan suhu.
8. Jalankan program yang berat (misalnya loop cepat) pada core lain dan amati efeknya terhadap suhu.

#### Pengamatan

| Kondisi | Suhu Terbaca (°C) | Waktu Pengamatan |
|---------|-------------------|------------------|
| Idle (awal) | | |
| Setelah sentuhan jari (30 dtk) | | |
| Setelah ditiup udara dingin | | |
| Saat menjalankan beban berat | | |

#### Pertanyaan

1. Berapa suhu idle chip ESP32/STM32 Anda? Apakah wajar?
2. Apa keterbatasan sensor suhu internal? Apakah cocok untuk mengukur suhu lingkungan?
3. Pada STM32, bagaimana rumus konversi raw ADC ke suhu pada channel 16? Apa itu nilai V₂₅ dan Avg_Slope?

---

### Percobaan 10: ADC Sampling Rate — Kecepatan Sampling ADC

**Tujuan:** Mengukur dan membandingkan kecepatan sampling (sample per detik) ADC pada ESP32 dan STM32.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. Wiper | GPIO34 | PA0 |
| Pot. VCC | 3.3V | 3.3V |
| Pot. GND | GND | GND |

> Rangkaian sama dengan Percobaan 01.

#### Langkah Kerja

1. Gunakan rangkaian potensiometer standar.
2. Buka project `ESP32_10_ADC_Sampling_Rate` atau `STM32_10_ADC_Sampling_Rate`.
3. Perhatikan metode pengukuran sampling rate pada kode: menghitung jumlah sampel dalam interval waktu tertentu.
4. Compile, upload, dan buka Serial Monitor.
5. Catat jumlah sampel per detik (SPS — Samples Per Second) yang ditampilkan.
6. Bandingkan kecepatan sampling antara ESP32 dan STM32.
7. Jika memungkinkan, ubah konfigurasi (resolusi, clock divider) dan ukur dampaknya terhadap SPS.

#### Pengamatan

| Parameter | ESP32 | STM32 |
|-----------|-------|-------|
| Samples per second (SPS) | | |
| Waktu per sampel (μs) | | |
| Resolusi yang digunakan | 12-bit | 12-bit |
| Mode pembacaan | | |

#### Pertanyaan

1. Berapa SPS maksimum yang dapat dicapai oleh ESP32 dan STM32?
2. Faktor apa saja yang mempengaruhi kecepatan sampling ADC?
3. Menurut Teorema Nyquist, berapa frekuensi sinyal maksimum yang dapat didigitalisasi dengan SPS yang Anda ukur?

---

### Percobaan 11: ADC Light Sensor — Sensor Cahaya LDR

**Tujuan:** Membaca intensitas cahaya menggunakan LDR (Light Dependent Resistor) melalui rangkaian voltage divider dan mengklasifikasikan level cahaya.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Titik tengah (LDR + R) | GPIO34 (ADC1_CH6) | PA0 (ADC1_CH0) |

```
  3.3V ──[LDR]──┬──[R = 10kΩ]── GND
                │
            GPIO34 / PA0

  Prinsip kerja:
  - Cahaya terang → R_LDR turun → V_adc naik
  - Cahaya gelap  → R_LDR naik  → V_adc turun
```

#### Langkah Kerja

1. Pasang LDR dan resistor 10 kΩ sebagai voltage divider pada breadboard.
2. Hubungkan satu ujung LDR ke **3.3V**, ujung lainnya ke titik tengah.
3. Hubungkan resistor 10 kΩ dari titik tengah ke **GND**.
4. Hubungkan titik tengah ke **GPIO34** (ESP32) atau **PA0** (STM32).
5. Buka project `ESP32_11_ADC_Light_Sensor` atau `STM32_11_ADC_Light_Sensor`.
6. Compile, upload, dan buka Serial Monitor.
7. Amati nilai ADC, tegangan, dan klasifikasi level cahaya (Dark, Dim, Normal, Bright).
8. Coba variasikan intensitas cahaya:
   - Tutup LDR dengan tangan (gelap)
   - Sorot LDR dengan senter HP (terang)
   - Biarkan pada cahaya ruangan (normal)
9. Catat perubahan nilai ADC dan klasifikasi untuk setiap kondisi.

#### Pengamatan

| Kondisi Cahaya | Nilai Raw ADC | Tegangan (mV) | Perkiraan Lux | Klasifikasi |
|----------------|---------------|---------------|---------------|-------------|
| Gelap total (tutup tangan) | | | | |
| Redup (jari menutupi sebagian) | | | | |
| Cahaya ruangan normal | | | | |
| Senter HP dekat | | | | |
| Cahaya matahari langsung | | | | |

#### Pertanyaan

1. Bagaimana hubungan antara intensitas cahaya dan resistansi LDR?
2. Mengapa perlu voltage divider untuk membaca LDR? Bisakah LDR langsung terhubung ke pin ADC?
3. Apakah konversi ke lux yang ditampilkan akurat? Bagaimana cara melakukan kalibrasi yang lebih baik?

---

### Percobaan 12: ADC Statistical Analysis — Analisis Statistik ADC

**Tujuan:** Melakukan analisis statistik terhadap data ADC meliputi rata-rata, standar deviasi, nilai minimum/maksimum, dan distribusi noise.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Pot. Wiper | GPIO34 | PA0 |
| Pot. VCC | 3.3V | 3.3V |
| Pot. GND | GND | GND |

> Rangkaian sama dengan Percobaan 01.

#### Langkah Kerja

1. Gunakan rangkaian potensiometer standar.
2. Atur potensiometer ke posisi **50%** (sekitar 1.65V) dan **jangan disentuh** selama percobaan.
3. Buka project `ESP32_12_ADC_Statistical_Analysis` atau `STM32_12_ADC_Statistical_Analysis`.
4. Compile, upload, dan buka Serial Monitor.
5. Biarkan program mengumpulkan sampel (biasanya 1000 sampel atau lebih).
6. Amati output statistik: **Mean**, **Std Dev**, **Min**, **Max**, **Range**, dan **SNR**.
7. Ulangi percobaan dengan potensiometer pada posisi 25% dan 75%.
8. Bandingkan hasil statistik pada ketiga posisi — apakah noise konsisten?

#### Pengamatan

| Parameter Statistik | Posisi 25% | Posisi 50% | Posisi 75% |
|---------------------|------------|------------|------------|
| Mean (rata-rata) | | | |
| Std Dev (simpangan baku) | | | |
| Min | | | |
| Max | | | |
| Range (Max − Min) | | | |
| SNR (dB) | | | |
| Jumlah sampel (N) | | | |

#### Pertanyaan

1. Berapa nilai standar deviasi yang Anda amati? Apakah tingkat noise ini dapat diterima?
2. Apakah noise ADC bersifat konstan di semua titik pengukuran atau bervariasi?
3. Bagaimana cara menghitung SNR dari data statistik? Apa artinya SNR dalam konteks ADC?
4. Jelaskan hubungan antara resolusi efektif (ENOB) dan SNR!

---

## 5. Ringkasan Koneksi Pin

### 5.1 ESP32 DevKit V1

| Percobaan | GPIO34 | GPIO35 | GPIO2 | 3.3V | GND |
|-----------|--------|--------|-------|------|-----|
| 01–03, 05, 06, 10, 12 | Pot. Wiper | — | — | Pot. VCC | Pot. GND |
| 04 | Pot.1 Wiper | Pot.2 Wiper | — | Pot. VCC | Pot. GND |
| 07 | Pot. Wiper | — | LED (built-in) | Pot. VCC | Pot. GND |
| 08 | Divider output | — | — | R1 → V_bat | R2 → GND |
| 09 | — | — | — | — | — |
| 11 | LDR + R divider | — | — | LDR VCC | R 10kΩ → GND |

### 5.2 STM32 Blue Pill

| Percobaan | PA0 | PA1 | PC13 | PA9 (TX) | PA10 (RX) |
|-----------|-----|-----|------|----------|-----------|
| 01–03, 05–08, 10–12 | Pot./Sensor | — | — | UART TX | UART RX |
| 04 | Pot.1 Wiper | Pot.2 Wiper | — | UART TX | UART RX |
| 07 | Pot. Wiper | — | LED (active low) | UART TX | UART RX |
| 09 | — (internal) | — | — | UART TX | UART RX |

---

## 6. Tugas Tambahan

### Tugas 1: Voltmeter Digital Dual-Range

Rancang dan implementasikan voltmeter digital sederhana menggunakan ADC yang mampu mengukur dua rentang tegangan:
- **Rentang rendah:** 0–3.3V (langsung ke pin ADC)
- **Rentang tinggi:** 0–12V (menggunakan voltage divider)

**Ketentuan:**
- Gunakan dua channel ADC (satu per rentang)
- Tampilkan tegangan dengan 2 digit desimal
- Tambahkan indikator LED jika tegangan melebihi batas aman
- Validasi akurasi dengan multimeter (error < 5%)

### Tugas 2: Data Logger Sensor Lingkungan

Buat program data logger yang merekam data dari minimal 2 sensor analog (misalnya LDR + potensiometer sebagai simulasi sensor suhu) secara periodik:

**Ketentuan:**
- Sampling setiap 1 detik
- Tampilkan timestamp (detik sejak mulai), nilai ADC, dan tegangan
- Terapkan filter moving average pada data
- Format output CSV-compatible agar bisa dianalisis di Excel/Python
- Jalankan selama minimal 5 menit dan analisis hasilnya

### Tugas 3: Sistem Alarm Multi-Level

Implementasikan sistem alarm berbasis ADC dengan tiga tingkat peringatan:

**Ketentuan:**
- **Level 1 (Info):** Nilai ADC 2000–2500 → LED hijau berkedip lambat
- **Level 2 (Warning):** Nilai ADC 2500–3500 → LED kuning berkedip cepat
- **Level 3 (Danger):** Nilai ADC > 3500 → LED merah menyala terus + pesan serial
- Gunakan minimal 2 LED eksternal
- Tambahkan hysteresis pada perubahan level (agar tidak berpindah-pindah saat di batas)
- Tampilkan status level saat ini di Serial Monitor

---

## 7. Laporan Praktikum

### 7.1 Format Laporan

Laporan praktikum harus mencakup:

1. **Halaman Judul** — Judul, nama, NIM, kelompok, tanggal
2. **Tujuan** — Tujuan praktikum (dari Bagian 1)
3. **Dasar Teori** — Ringkasan teori ADC (1–2 halaman, paraphrase)
4. **Hasil Percobaan** — Untuk setiap percobaan:
   - Foto rangkaian yang telah dipasang
   - Screenshot output Serial Monitor
   - Tabel pengamatan yang telah diisi lengkap
   - Jawaban pertanyaan
5. **Analisis & Pembahasan** — Analisis perbandingan ESP32 vs STM32 dalam hal:
   - Akurasi pembacaan ADC
   - Fitur kalibrasi
   - Kecepatan sampling
   - Kemudahan penggunaan API
6. **Tugas Tambahan** — Dokumentasi implementasi (minimal 1 tugas)
7. **Kesimpulan** — Rangkuman hasil dan pembelajaran

### 7.2 Ketentuan Pengumpulan

- Format: **PDF** (laporan) + **ZIP** (source code tugas tambahan)
- Deadline: **1 minggu** setelah praktikum
- Pengumpulan melalui e-learning / platform yang ditentukan
- Kode sumber tugas tambahan harus bisa di-compile tanpa error

---

## 8. Panduan Python Debug (debug_analysis.py)

Setiap project percobaan menyertakan script Python `debug_analysis.py` di dalam folder `python_debug/`. Script ini berguna untuk:

### 8.1 Fungsi Utama

- **Serial Monitor** — Membaca data dari port serial dan menampilkannya secara real-time
- **Data Logging** — Menyimpan data ADC ke file CSV untuk analisis lebih lanjut
- **Grafik Real-Time** — Menampilkan grafik live menggunakan matplotlib

### 8.2 Cara Penggunaan

1. Pastikan Python 3.x dan library yang diperlukan telah terinstall:
   ```
   pip install pyserial matplotlib
   ```

2. Sesuaikan port serial pada script:
   ```python
   SERIAL_PORT = '/dev/ttyUSB0'   # Linux
   SERIAL_PORT = 'COM3'           # Windows
   ```

3. Jalankan script dari terminal (bukan dari PlatformIO):
   ```
   cd praktikum/ESP32/ESP32_01_ADC_Single_Read/python_debug/
   python debug_analysis.py
   ```

4. Script akan otomatis:
   - Membuka koneksi serial ke board
   - Mem-parsing data ADC dari output serial
   - Menyimpan data ke file CSV
   - Menampilkan grafik real-time (jika matplotlib tersedia)

### 8.3 Tips Troubleshooting

- Pastikan Serial Monitor di PlatformIO **sudah ditutup** sebelum menjalankan script (port serial tidak bisa diakses oleh dua program sekaligus)
- Jika grafik tidak muncul, coba jalankan dengan `python -m matplotlib` terlebih dahulu untuk memverifikasi instalasi
- Periksa baud rate: harus sama antara program firmware (115200) dan script Python

---

## 9. Referensi Cepat

### 9.1 Formula Penting

| Formula | Keterangan |
|---------|------------|
| $V = \frac{raw \times V_{ref}}{2^n - 1}$ | Konversi raw ADC ke tegangan |
| $raw = \frac{V \times (2^n - 1)}{V_{ref}}$ | Konversi tegangan ke raw ADC |
| $V_{out} = V_{in} \times \frac{R_2}{R_1 + R_2}$ | Voltage divider |
| $SNR = 20 \times \log_{10}\left(\frac{mean}{\sigma}\right)$ | Signal-to-Noise Ratio |
| $ENOB = \frac{SNR - 1.76}{6.02}$ | Effective Number of Bits |

### 9.2 Atenuasi ADC ESP32

| Atenuasi | Rentang Input | Keterangan |
|----------|---------------|------------|
| 0 dB | 0 – 1.1V | Sensitivitas tinggi |
| 2.5 dB | 0 – 1.5V | — |
| 6 dB | 0 – 2.2V | — |
| 11 dB | 0 – 3.3V | Rentang penuh (paling sering digunakan) |

### 9.3 Channel ADC STM32F103

| Channel | Pin | Keterangan |
|---------|-----|------------|
| CH0 | PA0 | Analog input |
| CH1 | PA1 | Analog input |
| CH2 | PA2 | Analog input |
| CH3 | PA3 | Analog input |
| CH4 | PA4 | Analog input |
| CH5 | PA5 | Analog input |
| CH6 | PA6 | Analog input |
| CH7 | PA7 | Analog input |
| CH8 | PB0 | Analog input |
| CH9 | PB1 | Analog input |
| CH16 | Internal | Sensor suhu |
| CH17 | Internal | Vrefint (1.2V) |

---

## Catatan Keselamatan

1. **JANGAN** menghubungkan tegangan lebih dari **3.3V** ke pin ADC ESP32 atau STM32 tanpa voltage divider.
2. **JANGAN** menghubungkan baterai langsung ke pin ADC — selalu gunakan voltage divider.
3. Pastikan **polaritas** komponen benar sebelum menyalakan daya.
4. Cabut kabel USB sebelum mengubah rangkaian pada breadboard.
5. Gunakan **multimeter** untuk memverifikasi tegangan sebelum menghubungkan ke mikrokontroler.

---

*Jobsheet Modul 04 — ADC | Praktikum Sistem Embedded | 2025/2026*
