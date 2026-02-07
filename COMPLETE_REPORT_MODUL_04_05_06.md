# LAPORAN LENGKAP - MODUL 04, 05, 06

## Praktikum Sistem Embedded
---


========================================================
=== Modul-04-ADC ===
========================================================


-----------------------------------------------------------
--- Jobsheet.md ---
-----------------------------------------------------------

# Jobsheet Modul 04: Analog to Digital Converter (ADC)

## 🎯 Tujuan Praktikum
Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:
1.  Mengonfigurasi dan membaca data analog menggunakan ADC pada STM32 dan ESP32.
2.  Mengakses multiple channel ADC secara bergantian.
3.  Mengonversi nilai RAW ADC menjadi besaran tegangan (Voltage) dan fisik (Suhu/Cahaya).
4.  Mengatasi masalah noise dengan teknik averaging sederhana.
5.  Memahami karakteristik non-linearitas (khususnya pada ESP32).

## ⚠️ Keselamatan Kerja
1.  **Level Tegangan:** Pin ADC STM32 dan ESP32 bekerja maksimal pada **3.3V**. JANGAN menghubungkan sinyal 5V langsung ke pin ADC (dapat merusak pin permanen).
2.  **Wiring:** Pastikan rangkaian kabel (terutama VCC dan GND) benar sebelum menyalakan mikrokontroler.
3.  **Short Circuit:** Hati-hati saat menggunakan potensiometer, jangan sampai kaki tengah (wiper) terhubung langsung ke VCC/GND tanpa resistansi saat diputar ke ujung.

## 🛠️ Alat dan Bahan
-   1x Development Board STM32F103C8T6 (Blue Pill) + ST-Link
-   1x Development Board ESP32 DevKitC
-   2x Potensiometer 10kΩ
-   1x Sensor Cahaya (LDR)
-   1x Resistor 10kΩ (untuk pembagi tegangan LDR)
-   1x LED + Resistor 330Ω
-   Kabel Jumper & Breadboard
-   Multimeter (Opsional, untuk validasi)

---

## 🔬 Percobaan 1: Pembacaan Dasar ADC Single Channel

### Tujuan
Membaca nilai tegangan dari potensiometer dan menampilkannya ke Serial Monitor.

### 1.1 Skema Rangkaian
Gunakan potensiometer sebagai pembagi tegangan variable.
-   Kaki 1 Potensiometer -> 3.3V
-   Kaki 3 Potensiometer -> GND
-   Kaki 2 (Tengah) -> Pin ADC Target

**Pin Map:**
-   STM32: **PA0**
-   ESP32: **GPIO 34** (ADC1_CH6)

### 1.2 Kode Program STM32
```cpp
/* Percobaan 1 - STM32 ADC Basic */
#define PIN_POT PA0

void setup() {
  Serial.begin(115200);
  pinMode(PIN_POT, INPUT_ANALOG);
  analogReadResolution(12); // Set 12-bit (0-4095)
}

void loop() {
  int rawValue = analogRead(PIN_POT);
  float voltage = (float)rawValue / 4095.0 * 3.3;

  Serial.print("Raw: ");
  Serial.print(rawValue);
  Serial.print("\t Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");

  delay(500);
}
```

### 1.3 Kode Program ESP32
```cpp
/* Percobaan 1 - ESP32 ADC Basic */
#define PIN_POT 34

void setup() {
  Serial.begin(115200);
  // ESP32 ADC Resolution default is usually 12-bit
  // Attenuation default is 11db (measure up to ~3.3V)
  analogReadResolution(12);
}

void loop() {
  int rawValue = analogRead(PIN_POT);
  // Kalibrasi sederhana tegangan (ESP32 ADC ref bisa bervariasi)
  float voltage = (float)rawValue / 4095.0 * 3.3;

  Serial.print("ESP Raw: ");
  Serial.print(rawValue);
  Serial.print("\t Voltage: ");
  Serial.println(voltage);

  delay(500);
}
```

### 📋 Tugas Analisis 1:
1.  Putar potensiometer dari min ke max. Catat nilai RAW terendah dan tertinggi. Apakah tepat 0 dan 4095?
2.  Bandingkan pembacaan tegangan di Serial Monitor dengan pengukuran Multimeter pada posisi wiper potensiometer. Berapa selisihnya?

---

## 🔬 Percobaan 2: ADC Multi-Channel & Sensor Cahaya (LDR)

### Tujuan
Membaca dua input analog secara bergantian: Potensiometer dan Sensor Cahaya (LDR).

### 2.1 Skema Rangkaian
Tambahkan rangkaian pembagi tegangan LDR:
-   VCC (3.3V) -> LDR -> Titik A -> Resistor 10k -> GND.
-   Hubungkan Titik A ke Pin ADC.

**Pin Map (STM32):**
-   Potensiometer: **PA0**
-   LDR Divider: **PA1**

**Pin Map (ESP32):**
-   Potensiometer: **GPIO 34**
-   LDR Divider: **GPIO 35**

*Note: Jangan gunakan ADC2 pada ESP32 jika WiFi digunakan. GPIO 34 & 35 adalah ADC1 (Safe).*

### 2.2 Kode Program (Universal STM32/ESP32)
Sesuaikan `PIN_POT` dan `PIN_LDR` sesuai board.

```cpp
/* Percobaan 2 - Multi Channel ADC */
// STM32
#define PIN_POT PA0
#define PIN_LDR PA1
// ESP32 (Uncomment jika pakai ESP32)
// #define PIN_POT 34
// #define PIN_LDR 35

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  // Teknik: Pembacaan berurutan sederhana (Polling)
  int valPot = analogRead(PIN_POT);
  delay(10); // Jeda kecil untuk settling time ADC Mux
  int valLdr = analogRead(PIN_LDR);

  Serial.print("POT: ");
  Serial.print(valPot);
  Serial.print(" | LDR: ");
  Serial.println(valLdr);

  delay(500);
}
```

### 📋 Tugas Analisis 2:
1.  Tutup sensor LDR dengan tangan (gelap) dan beri cahaya senter (terang). Bagaimana perubahan nilai ADC-nya? Naik atau turun? Jelaskan berdasarkan hukum pembagi tegangan $V_{out} = V_{in} \times \frac{R_2}{R_1+R_2}$.

---

## 🔬 Percobaan 3: Simple Filtering (Moving Average)

### Tujuan
Mengurangi fluktuasi (noise) pada pembacaan sensor menggunakan algoritma *Moving Average*.

### 3.1 Kode Program (Fungsi Tambahan)
Gunakan rangkaian Percobaan 1.

```cpp
/* Percobaan 3 - Smooth ADC */
#define PIN_INPUT PA0 // atau 34
#define NUM_SAMPLES 20

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

int readSmooth(int pin) {
  long sum = 0;
  for(int i=0; i<NUM_SAMPLES; i++) {
    sum += analogRead(pin);
    delay(2); // Short delay antar sample
  }
  return sum / NUM_SAMPLES;
}

void loop() {
  int raw = analogRead(PIN_INPUT);
  int smooth = readSmooth(PIN_INPUT);

  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print("\t Smooth: ");
  Serial.println(smooth);

  delay(100);
}
```

### 📋 Tugas Analisis 3:
1.  Biarkan potensiometer pada posisi tetap. Lihat nilai Raw, pasti ada jitter (misal: 2045, 2042, 2048).
2.  Bandingkan dengan nilai Smooth. Apakah lebih stabil?
3.  Ubah `NUM_SAMPLES` menjadi 100. Apa efeknya terhadap kestabilan dan respon waktu (lag) saat potensiometer diputar cepat?

---

## 🔬 Tugas Mandiri (Pre-Project)

Buatlah sistem "Smart Night Light" sederhana:
1.  Gunakan **LDR** untuk mendeteksi tingkat kegelapan.
2.  Gunakan **Potensiometer** untuk mengatur *threshold* (ambang batas) nyala lampu.
3.  Kontrol sebuah **LED**:
    -   Jika Nilai LDR < Nilai Potensiometer (Gelap), LED Menyala.
    -   Jika Nilai LDR > Nilai Potensiometer (Terang), LED Mati.
4.  Tampilkan status threshold dan nilai LDR di Serial Monitor.

*Tips: Gunakan hysteresis (rentang toleransi) agar LED tidak berkedip-kedip saat cahaya berada tepat di ambang batas.*

---

## 📝 Format Laporan Praktikum
1.  **Cover**: Judul, Nama, NIM, Tanggal.
2.  **Hasil Percobaan 1-3**: Screenshot Serial Monitor & Foto Rangkaian.
3.  **Jawaban Analisis**: Jawaban dari pertanyaan-pertanyaan di atas.
4.  **Kode Tugas Mandiri**: Sertakan kode program lengkap dan penjelasan alur logika (Flowchart).
5.  **Kesimpulan**: Apa yang dipelajari tentang karakteristik ADC STM32 vs ESP32.

-----------------------------------------------------------
--- Materi.md ---
-----------------------------------------------------------

# BAB 04: Analog-to-Digital Converter

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami konsep dasar Analog-to-Digital Converter
2. Mengimplementasikan program Analog-to-Digital Converter pada STM32 dan ESP32
3. Melakukan debugging dan troubleshooting
4. Menerapkan best practices dalam pengembangan embedded systems

---

## 📚 Materi Pembelajaran

### Pendahuluan

# Modul 04: Analog to Digital Converter (ADC)

## 1. Konsep Dasar Sinyal Analog dan Digital

Di dunia nyata, hampir semua besaran fisik bersifat **analog** (kontinyu), seperti suhu, cahaya, suara, dan tekanan. Namun, mikrokontroler adalah perangkat **digital** yang hanya mengerti logika 0 dan 1 (diskrit). Untuk dapat memproses sinyal analog tersebut, diperlukan antarmuka yang disebut **Analog to Digital Converter (ADC)**.

### 1.1 Sampling dan Kuantisasi

Proses konversi sinyal analog ke digital melibatkan dua tahap utama:

1.  **Sampling (Pencuplikan):** Mengambil nilai sinyal analog pada interval waktu tertentu (Perioda Sampling, $T_s$). Frekuensi sampling ($f_s$) harus memenuhi **Teorema Nyquist**, yaitu minimal 2x frekuensi maksimum sinyal input ($f_s \ge 2f_{max}$) untuk menghindari *aliasing*.
2.  **Quantization (Kuantisasi):** Memetakan nilai amplitudo yang dicuplik ke level diskrit terdekat berdasarkan resolusi ADC.

### 1.2 Resolusi ADC

Resolusi menentukan seberapa presisi ADC dapat membedakan perubahan tegangan input. Resolusi dinyatakan dalam **bit ($n$)**. Jumlah level diskrit adalah $2^n$.

Rumus dasar pembacaan ADC:

$$ ADC\_Value = \frac{V_{in}}{V_{ref}} \times (2^n - 1) $$

Atau untuk mencari tegangan dari nilai ADC:

$$ V_{in} = \frac{ADC\_Value}{2^n - 1} \times V_{ref} $$

**Contoh:**
ADC 12-bit ($2^{12} = 4096$ level, range 0-4095) dengan $V_{ref} = 3.3V$.
Jika $V_{in} = 1.65V$, maka:
$$ ADC = \frac{1.65}{3.3} \times 4095 \approx 2047 $$
Step size (Voltage per bit) = $3.3V / 4095 \approx 0.8 mV$.

---

## 2. ADC pada STM32F103C8T6 (Blue Pill)

STM32F103 memiliki dua unit ADC 12-bit (ADC1 dan ADC2) yang canggih dengan arsitektur **Successive Approximation Register (SAR)**.

### 2.1 Spesifikasi ADC STM32
-   **Resolusi:** 12-bit (Konfigurasi: 12, 10, 8, atau 6 bit).
-   **Channel:** Hingga 16 channel eksternal + 2 internal (Temperature sensor, Vrefint).
-   **Range tegangan:** $0V$ sampai $V_{DDA}$ ($3.3V$).
-   **Waktu Konversi:** ~1 $\mu s$ pada clock ADC 14 MHz.
-   **Mode Konversi:**
    -   *Single Mode:* Konversi satu kali lalu stop.
    -   *Continuous Mode:* Konversi terus menerus.
    -   *Scan Mode:* Mengkonversi grup channel secara berurutan.
    -   *Injected Mode:* Konversi prioritas tinggi (seperti interrupt untuk ADC).

### 2.2 Alignment Data
Hasil konversi 12-bit disimpan dalam register 16-bit, bisa **Right Aligned** (standar) atau **Left Aligned**.

### 2.3 Metode Pembacaan di Arduino Framework
Library STM32duino memudahkan pembacaan ADC.

```cpp
// Konfigurasi resolusi (default Arduino biasanya 10-bit, STM32 bisa 12-bit)
analogReadResolution(12); // Set output 0-4095

int val = analogRead(PA0);
float voltage = val * (3.3 / 4095.0);
```

Pin ADC STM32F103C8T6: PA0-PA7 (ADC12_IN0 - ADC12_IN7), PB0-PB1 (ADC12_IN8 - ADC12_IN9).

---

## 3. ADC pada ESP32

ESP32 memiliki dua unit SAR ADC 12-bit: **ADC1** dan **ADC2**. Namun, ADC ESP32 memiliki karakteristik non-linear dan batasan tertentu yang perlu diperhatikan.

### 3.1 Peta Pin ADC ESP32
-   **ADC1:** 8 Channel (GPIO 32-39). **Aman digunakan kapan saja.**
-   **ADC2:** 10 Channel (GPIO 0, 2, 4, 12-15, 25-27). **Tidak bisa digunakan saat WiFi aktif.** Driver WiFi memonopoli ADC2.

### 3.2 Attenuation (Peredaman)
Tegangan input ESP32 ADC secara default memiliki range terbatas (~0 - 1.1V). Untuk membaca tegangan hingga 3.3V, kita harus mengatur **attenuation**.

| Attenuation | Input Range (Approx) | Fungsi `analogSetAttenuation()` |
|:---:|:---:|:---:|
| 0dB | 0 - 1.1V | `ADC_0db` |
| 2.5dB | 0 - 1.5V | `ADC_2_5db` |
| 6dB | 0 - 2.2V | `ADC_6db` |
| 11dB | 0 - 3.3V | `ADC_11db` (Default di Arduino) |

### 3.3 Isu Non-Linearity
ADC ESP32 terkenal tidak linear sempurna, terutama di batas bawah (dekat 0V) dan batas atas (dekat 3.3V).
-   Input < 0.1V sering terbaca 0.
-   Input > 3.2V sering terbaca 4095.
Solusi: Gunakan kalibrasi (polynomial correction) atau gunakan range kerja aman (0.1V - 3.1V).

```cpp
// Setup (biasanya default sudah 11db/12-bit di core terbaru)
analogReadResolution(12);
analogSetAttenuation(ADC_11db);

int val = analogRead(34);
```

---

## 4. Sampling Rate & DMA (Direct Memory Access)

Untuk aplikasi yang membutuhkan kecepatan tinggi (seperti sampling audio atau sinyal AC), metode `analogRead()` (polling) terlalu lambat karena CPU harus menunggu proses konversi selesai.

### 4.1 DMA (Direct Memory Access)
DMA memungkinkan peripheral (ADC) menulis data langsung ke memori (RAM) tanpa intervensi CPU.
-   **Keuntungan:** Beban CPU minimal, sampling rate tinggi dan stabil.
-   **Aplikasi:** FFT (Fast Fourier Transform), pembacaan multi-channel simultan.

*Catatan: Topik DMA secara mendalam akan dibahas pada Modul 08, namun konsep dasarnya dikenalkan di sini hubungannya dengan ADC continuous scan.*

---

## 5. Kalibrasi dan Konversi Unit Fisik

Nilai mentah (RAW) dari ADC tidak memiliki makna fisis sebelum dikonversi.

### 5.1 Simple Scaling (Linear)
Untuk sensor linear (misal LM35, Potensiometer):
$$ Nilai\_Fisik = \frac{ADC}{ADC_{max}} \times (Max\_Fisik - Min\_Fisik) + Min\_Fisik $$

### 5.2 Sensor Non-Linear (NTC Thermistor, LDR)
Beberapa sensor memiliki respons logaritmik atau eksponensial.
Contoh NTC Thermistor memerlukan persamaan **Steinhart-Hart**:

$$ \frac{1}{T} = A + B \ln(R) + C (\ln(R))^3 $$

Di mana $R$ dihitung dari rangkaian pembagi tegangan (Voltage Divider) yang dibaca ADC.

---

## 6. Tips Hardware ADC
1.  **Gunakan Kapasitor Decoupling:** Pasang 100nF dekat pin ADC untuk memfilter noise frekuensi tinggi.
2.  **Impedansi Input:** ADC memiliki impedansi input tertentu. Jika sumber sinyal memiliki impedansi output tinggi (>10kΩ), tegangan akan drop saat sampling. Gunakan **Op-Amp Buffer (Voltage Follower)**.
3.  **VREF Stabil:** Ketelitian ADC sangat bergantung pada stabilitas tegangan referensi (VCC 3.3V). Gunakan regulator LDO yang bagus.

---

## 7. Referensi Lanjutan
1.  AN2834 Application Note: *How to get the best ADC accuracy in STM32F10xxx devices*.
2.  Espressif Docs: *Analog to Digital Converter (ADC)*.


### Teori Dasar

(Isi teori dasar di sini)

### Implementasi

(Isi implementasi di sini)

---

## 📖 Referensi

1. STM32 Reference Manual
2. ESP32 Technical Reference Manual
3. FreeRTOS Documentation



-----------------------------------------------------------
--- PPT_Prompts_1.md ---
-----------------------------------------------------------

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



-----------------------------------------------------------
--- PPT_Prompts_2.md ---
-----------------------------------------------------------

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



-----------------------------------------------------------
--- Project.md ---
-----------------------------------------------------------

# Project Modul 04: Smart Battery Monitoring System

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Judul** | Dual-MCU Smart Battery Management System (BMS) Monitor |
| **Modul** | 04 - Analog to Digital Converter |
| **Platform** | STM32F103C8T6 + ESP32 DevKitC |
| **Tingkat Kesulitan** | ⭐⭐⭐ (Menengah) |
| **Durasi Pengerjaan** | 2 minggu |
| **Fokus Utama** | Akurasi Sampling Data & Signal Conditioning |

---

## 🎯 Deskripsi Project

Project ini bertujuan membuat sistem pemantauan kesehatan baterai (Battery Health Monitoring) real-time. Sistem ini mensimulasikan monitoring baterai UPS atau Solar Panel storage.

**Pembagian Tugas MCU:**
1.  **STM32 (Precision DAQ):** Bertugas membaca parameter fisik (Tegangan, Arus, Suhu) dengan kecepatan sirkular menggunakan ADC Multi-channel. Melakukan filtering digital (Moving Average) untuk menstabilkan data.
2.  **ESP32 (Internet Gateway):** Menerima data matang dari STM32, melakukan kalkulasi estimasi sisa daya (SoC - State of Charge), dan menampilkan data ke Web Dashboard.

---

## 🔧 Spesifikasi Hardware & Wiring

### Komponen
-   STM32F103C8T6 (Blue Pill)
-   ESP32 DevKitC V4
-   2x Potensiometer (Simulasi Tegangan Baterai 0-12V [scaled] & Arus Charge/Discharge)
-   1x LM35 / NTC Thermistor (Sensor Suhu Baterai)
-   LED Indikator (Merah=Low, Hijau=Normal, Biru=Charging)

### Skema Input Analog (STM32)
1.  **Channel 1 (Voltage Sim):** Potensio 1 dihubungkan ke PA0. (Asumsikan ini V_Bat yang diskalakan).
2.  **Channel 2 (Current Sim):** Potensio 2 dihubungkan ke PA1. (Asumsikan titik tengah 1.65V = 0 Ampere).
3.  **Channel 3 (Temp):** LM35/NTC dihubungkan ke PA2.

---

## 📋 Spesifikasi Fungsional

### F1: Data Acquisition (STM32)
-   Sampling Rate: Minimal 10 Hz per channel.
-   Resolution: 12-bit (0-4095).
-   **Filtering:** Wajib mengimplementasikan **Moving Average Filter** (Window size = 10 samples) sebelum data dikirim ke ESP32.
-   Konversi satuan fisik dilakukan di STM32 (kirim data dalam Volt, Ampere, Celcius).

### F2: Inter-MCU Communication
-   STM32 mengirim data paket struct ke ESP32 via UART (Serial 2) setiap 500ms.
-   Format Data: `{Voltage (float), Current (float), Temp (float)}`.

### F3: Processing & Warning (ESP32)
-   **Tegangan:**
    -   < 11.0V : Alert "Battery Critical" (LED Merah Blink).
    -   11.0V - 12.0V : Warning "Low Battery" (LED Merah On).
    -   > 12.0V : Normal.
-   **Arus:**
    -   Positif: "Discharging".
    -   Negatif: "Charging".
-   **State of Charge (SoC):** Estimasi % baterai berdasarkan tegangan (Lookup Table sederhana atau Linear Mapping).

---

## 🧪 Kriteria Pengujian

1.  **Akurasi Tegangan:** Bandingkan pembacaan STM32 dengan Multimeter. Toleransi error maks 2%.
2.  **Stabilitas:** Saat potensio diam, nilai pembacaan tidak boleh *jitter* lebih dari +/- 0.05V (Bukti filter berhasil).
3.  **Real-time:** Delay perubahan potensio sampai tampil di Serial Monitor maksimal 1 detik.

---

## 📝 Deliverables

1.  **Source Code:**
    -   Project STM32 (PlatformIO).
    -   Project ESP32 (PlatformIO).
2.  **Laporan Project:**
    -   Bab 1: Pendahuluan & Dasar Teori ADC.
    -   Bab 2: Desain Hardware & Rangkaian.
    -   Bab 3: Algoritma Filtering & Konversi Data.
    -   Bab 4: Hasil Pengujian Akurasi (Tabel Perbandingan ADC vs Multimeter).
3.  **Video Demo:**
    -   Demo perubahan tegangan/arus.
    -   Demo fitur filtering (noise reduction).
    -   Demo alarm batas tegangan.

---

## ⚠️ Tantangan (Bonus Points)
-   Gunakan **DMA (Direct Memory Access)** pada STM32 untuk pembacaan ADC (Nilai A+).
-   Implementasikan **Kalibrasi 2-Titik** untuk meningkatkan akurasi pembacaan LM35.
-   Tampilkan grafik data (Plotter) pada Web Dashboard ESP32.



-----------------------------------------------------------
--- Referensi.md ---
-----------------------------------------------------------

# Referensi Pembelajaran
## Modul 04: Analog to Digital Converter (ADC)

Berikut adalah daftar referensi untuk mendukung pemahaman teori dan implementasi praktikum ADC.

## 📚 Dokumentasi Resmi (Datasheets & App Notes)

1.  **STM32F103x8 Datasheet & Reference Manual**
    *   *Reference:* Chapter 11 (ADC) pada RM0008.
    *   *Topik:* SAR Architecture, Scan Mode, Continuous Conversion, Calibration.
    *   [Link ST.com - RM0008](https://www.st.com/resource/en/reference_manual/cd00171190.pdf)

2.  **AN2834 Application Note: How to get the best ADC accuracy in STM32F10xxx**
    *   *Deskripsi:* Dokumen wajib baca untuk memahami sumber error pada ADC (noise, impedansi sumber) dan cara mengatasinya.
    *   [Link AN2834](https://www.st.com/resource/en/application_note/cd00211314-how-to-get-the-best-adc-accuracy-in-stm32fx-series-microcontrollers-stmicroelectronics.pdf)

3.  **ESP32 Technical Reference Manual (ADC Chapter)**
    *   *Deskripsi:* Penjelasan detail tentang ADC1 vs ADC2, Attenuation, dan Vref calibration.
    *   [Link Espressif TRM](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

## 💻 Tutorial & Artikel Teknis

1.  **Deep Blue Embedded - STM32 ADC Tutorial**
    *   Tutorial lengkap konfigurasi ADC Single Channel, Multi-channel, dan DMA menggunakan HAL/Arduino.
    *   [DeepBlueEmbedded - STM32 ADC](https://deepbluembedded.com/stm32-adc-tutorial-complete-guide-with-examples/)

2.  **Random Nerd Tutorials - ESP32 ADC Analog Read**
    *   Panduan praktis membaca tegangan analog pada ESP32, isu non-linearitas, dan penggunaan pin yang aman.
    *   [RNT - ESP32 ADC](https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/)

3.  **Moving Average Filter Implementation**
    *   Penjelasan konsep dan implementasi filter rata-rata bergerak di C++ untuk menghaluskan pembacaan sensor.
    *   [GeeksForGeeks - Moving Average](https://www.geeksforgeeks.org/program-find-simple-moving-average/)

## 🔋 Referensi Project BMS

1.  **Battery Management System Basics**
    *   Penjelasan parameter baterai: SoC (State of Charge), SoH (State of Health), OCV (Open Circuit Voltage).
    *   [DigiKey - BMS Basics](https://www.digikey.com/en/articles/a-basic-introduction-to-battery-management-systems)

2.  **Measuring Voltage and Current with Arduino/STM32**
    *   Tutorial dasar penggunaan Voltage Divider dan Current Shunt/ACS712.
    *   [DroneBot Workshop - Voltage & Current](https://dronebotworkshop.com/dc-voltage-current/)

## 📹 Video Pembelajaran

1.  **EEVblog #59 - ADC Aliasing & Nyquist**
    *   Penjelasan mendalam tapi santai tentang teori sampling dan aliasing.
    *   [YouTube - EEVblog](https://www.youtube.com/watch?v=vVjV-dhkmbA)

2.  **How ADC Works (SAR Architecture)**
    *   Animasi visual cara kerja Successive Approximation Register (SAR) ADC.
    *   [YouTube Reference](https://www.youtube.com/watch?v=...)


-----------------------------------------------------------
--- Rubrik_Penilaian_Project.md ---
-----------------------------------------------------------

# Rubrik Penilaian Project Modul 04
## Topik: Smart Battery Monitoring System (BMS)

Project ini menilai kemampuan mahasiswa dalam melakukan akuisisi data analog presisi, pengondisian sinyal digital, dan integrasi sistem monitoring.

## 📊 Bobot Penilaian
Total Skor Maksimum: **100 Poin**

| Komponen | Bobot | Deskripsi |
|----------|-------|-----------|
| **D1: Signal Quality** | 35% | Akurasi, stabilitas, dan filtering data ADC |
| **D2: System Integration** | 25% | Komunikasi STM32-ESP32 & Logic Alert |
| **D3: Dashboard UI** | 20% | Visualisasi data pada Web Interface |
| **D4: Laporan/Analisis** | 20% | Analisis error dan kalibrasi |

---

## 📝 Detail Kriteria Penilaian

### 1. Kualitas Sinyal & Firmware STM32 (35 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Akurasi Tegangan** | - Error pembacaan < 2% dibanding Multimeter (pada range 5-12V) | 15 |
| **Filtering Digital** | - Implementasi Moving Average berfungsi (Output stabil saat input constant) <br> - Respons step signal < 1 detik | 15 |
| **Multi-channel** | - Sukses membaca 3 parameter secara simultan (Volt, Ampere, Suhu) tanpa saling interferensi (crosstalk) | 5 |

### 2. Integrasi Sistem & ESP32 (25 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Data Transmission** | - Data floating point terkirim utuh via UART (struct/json) | 10 |
| **Alert Logic** | - LED/Buzzer aktif sesuai threshold tegangan yang ditentukan <br> - State transition (Normal -> Low -> Critical) berjalan mulus | 10 |
| **SoC Estimation** | - Terdapat kalkulasi % baterai (bukan sekedar menampilkan raw voltage) | 5 |

### 3. Web Dashboard (20 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Real-time Data** | - Angka tegangan/arus update otomatis tanpa refresh page (AJAX/WebSocket) | 10 |
| **Visualisasi** | - Tampilan menarik (Gauge/Bar/Chart) <br> - Indikator status baterai jelas (Warna Merah/Hijau) | 10 |

### 4. Laporan & Analisis (20 Poin)

| Aspek | Kriteria Penilaian | Skor |
|-------|-------------------|------|
| **Data Kalibrasi** | - Menyertakan tabel perbandingan nilai RAW ADC vs Tegangan Sebenarnya <br> - Menghitung konstanta kalibrasi (Slope/Offset) | 10 |
| **Analisis Noise** | - Menampilkan perbandingan grafik sinyal sebelum dan sesudah filtering (Serial Plotter screenshot) | 10 |

---

## 🌟 Bonus Points (Fitur Tambahan)

- **Kalibrasi Otomatis:** Sistem memiliki mode kalibrasi yang menyimpan nilai offset ke EEPROM/Flash (+5).
- **History Graph:** Web dashboard menampilkan grafik riwayat tegangan 1 jam terakhir (+5).
- **Safety Cut-off:** Simulasi sinyal output untuk memutus beban (Relay) saat tegangan Critical (+5).

---

## Awas Plagiasi & Hardcoding!
- Menggunakan `random()` untuk meniru fluktuasi sinyal sensor = **NILAI 0**.
- Menggunakan data dummy statis = **NILAI 0**.
- Wajib menggunakan input analog fisik (Potensio/Sensor). 


-----------------------------------------------------------
--- Rubrik_Penilaian_Tugas_Video.md ---
-----------------------------------------------------------

# Rubrik Penilaian Tugas Video
## Modul 04: Smart Battery Monitor (ADC)

Video demonstrasi Project Modul 04 fokus pada verifikasi keakuratan pembacaan sensor dan stabilitas sistem monitoring baterai.

## 📹 Spesifikasi Teknis Video

- **Durasi:** 5 - 8 Menit
- **Resolusi:** Min. 720p
- **Platform:** YouTube (Unlisted) / GDrive
- **Wajah:** Wajib tampil saat intro dan closing

---

## 📊 Detail Kriteria (Total: 100 Poin)

### 1. Demonstrasi Hardware & Akurasi (40 Poin)

| Scene Wajib | Deskripsi Aktivitas | Poin |
|-------------|---------------------|------|
| **Validation Test** | - Tunjukkan pembacaan Voltmeter/Multimeter fisik pada kaki Potensiometer. <br> - Tunjukkan nilai yang tampil di Dashboard Web secara bersamaan. <br> - **Goal:** Buktikan nilainya sama/mirip (toleransi < 2%). | 20 |
| **Stability Test** | - Set potensio pada posisi tetap. <br> - Zoom-in ke Serial Monitor/Dashboard. <br> - **Goal:** Buktikan angka stabil (tidak loncat-loncat) berkat digital filtering. | 10 |
| **Response Test** | - Putar potensio dengan cepat. <br> - **Goal:** Tunjukkan sistem merespons perubahan tanpa delay berlebihan. | 10 |

### 2. Penjelasan Teknis (30 Poin)

| Topik | Deskripsi | Poin |
|-------|-----------|------|
| **ADC Architecture** | Jelaskan singkat perbedaan konfigurasi ADC di STM32 (12-bit SAR) vs ESP32 (Attenuation). | 10 |
| **Signal Conditioning** | Jelaskan rumus konversi: Dari `Raw Value` -> `Voltage Divider Formula` -> `Real Voltage`. | 10 |
| **Filter Logic** | Tunjukkan potongan kode *Moving Average* dan jelaskan cara kerjanya. | 10 |

### 3. Fungsionalitas Sistem (20 Poin)

| Fitur | Deskripsi | Poin |
|-------|-----------|------|
| **Alerting** | Demo saat tegangan diturunkan simulasi Low Battery (LED berubah warna / Warning muncul). | 10 |
| **Dashboard** | Tunjukkan UI Web yang menampilkan Volt, Amphere, dan Status Baterai. | 10 |

### 4. Kualitas Video (10 Poin)

| Aspek | Deskripsi | Poin |
|-------|-----------|------|
| **Visual & Audio** | Gambar jelas (tidak blur saat zoom ke angka multimeter), Suara narasi terdengar jelas. | 10 |

---

## 💡 Tips & Trik Video Modul 04

1.  **Split Screen:** Jika memungkinkan, gunakan editing split screen. Kiri: Kamera ke Multimeter fisik. Kanan: Screen Record Dashboard Web. Ini sangat meyakinkan!
2.  **Multimeter Jelas:** Pastikan angka di multimeter tidak tertutup kabel atau silau lampu.
3.  **Tunjukkan Noise:** Boleh juga mendemokan perbandingan "Filter OFF" vs "Filter ON" untuk nilai tambah (menunjukkan urgensi filtering).


========================================================
=== Modul-05-DAC-PWM ===
========================================================


-----------------------------------------------------------
--- Jobsheet.md ---
-----------------------------------------------------------

# JOBSHEET BAB 05: DAC dan PWM Output

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | DAC (Digital-to-Analog Converter) dan PWM (Pulse Width Modulation) |
| **Platform** | STM32F103C8T6 (Blue Pill), ESP32 DevKit V1 |
| **Jumlah Program STM32** | 5 Program |
| **Jumlah Program ESP32** | 7 Program |
| **Durasi** | 3 x 50 menit |
| **Tools** | PlatformIO, STM32CubeIDE, Serial Monitor, Oscilloscope (optional) |

---

## 🎯 Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. Memahami prinsip kerja DAC dan PWM pada mikrokontroler
2. Mengkonfigurasi dan memprogram DAC pada STM32 (12-bit) dan ESP32 (8-bit)
3. Mengimplementasikan PWM untuk berbagai aplikasi (LED dimming, motor, servo)
4. Membandingkan karakteristik DAC vs PWM sebagai output analog
5. Menggunakan DMA untuk waveform generation
6. Menerapkan hardware fade pada ESP32 LEDC
7. Melakukan debugging dan analisis sinyal output

---

## 🔧 Alat dan Komponen

### Hardware
| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | Mikrokontroler utama |
| 2 | ESP32 DevKit V1 | 1 | Mikrokontroler utama |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | USB Cable Micro/Type-C | 2 | Koneksi dan programming |
| 5 | LED 5mm (Merah, Hijau, Biru) | 3 | Indikator output |
| 6 | Resistor 330Ω | 3 | Current limiting LED |
| 7 | Resistor 10kΩ | 2 | RC filter |
| 8 | Kapasitor 100nF | 2 | RC filter |
| 9 | Motor DC 3-6V | 1 | Aktuator PWM |
| 10 | Module L298N / L293D | 1 | Motor driver |
| 11 | Servo Motor SG90 | 1 | Servo control |
| 12 | Potentiometer 10kΩ | 1 | Input analog |
| 13 | Speaker/Buzzer 8Ω | 1 | Audio output (optional) |
| 14 | Breadboard | 1 | Prototyping |
| 15 | Kabel Jumper | 20+ | Koneksi |

### Software
- PlatformIO IDE / VS Code
- STM32CubeIDE (optional)
- Serial Monitor / PuTTY
- Oscilloscope software (optional)

---

## 📐 Skema Rangkaian

### Rangkaian DAC Output (STM32)
```
STM32F103C8T6
      │
      ├── PA4 (DAC_OUT1) ──┬── Oscilloscope
      │                    │
      │                    ├── 10kΩ ──┬── Analog Out (filtered)
      │                    │          │
      │                    │         100nF
      │                    │          │
      │                    │         GND
      │
      ├── PA5 (DAC_OUT2) ────── LED + 330Ω ── GND
      │
      └── GND ─────────────────── GND
```

### Rangkaian DAC Output (ESP32)
```
ESP32 DevKit
      │
      ├── GPIO25 (DAC1) ──┬── Oscilloscope
      │                   │
      │                   ├── 10kΩ ──┬── Analog Out (filtered)
      │                   │          │
      │                   │         100nF
      │                   │          │
      │                   │         GND
      │
      ├── GPIO26 (DAC2) ────── LED + 330Ω ── GND
      │
      └── GND ──────────────────── GND
```

### Rangkaian PWM LED Dimming
```
STM32/ESP32
      │
      ├── PA6/GPIO25 (PWM) ────── LED + 330Ω ── GND
      │
      ├── PA7/GPIO26 (PWM) ────── LED + 330Ω ── GND
      │
      ├── PB0/GPIO27 (PWM) ────── LED + 330Ω ── GND
      │
      └── GND ─────────────────────────────── GND
```

### Rangkaian Motor Control
```
STM32/ESP32                    L298N Module
      │                              │
      ├── PA6/GPIO25 (PWM) ────────► ENA
      │                              │
      ├── PA0/GPIO26 (DIR_A) ──────► IN1
      │                              │
      ├── PA1/GPIO27 (DIR_B) ──────► IN2
      │                              │
      ├── 5V ────────────────────── 5V Logic
      │                              │
      ├── GND ────────────────────── GND
      │                              │
      │                              ├── OUT1 ──┬── Motor DC
      │                              └── OUT2 ──┘      │
      │                                               12V
      │                                              Power
```

### Rangkaian Servo Control
```
STM32/ESP32                    Servo SG90
      │                              │
      ├── PA6/GPIO25 (PWM) ────────► Signal (Orange)
      │                              │
      ├── 5V ─────────────────────── VCC (Red)
      │                              │
      └── GND ────────────────────── GND (Brown)
```

---

## 📝 Percobaan

### Bagian A: DAC Output

---

#### Program 1: DAC Basic Output (STM32)
**File:** `praktikum/STM32/STM32_01_DAC_Output/src/main.c`

**Tujuan:** Menghasilkan tegangan analog menggunakan DAC 12-bit

**Langkah:**
1. Buat project baru dengan PlatformIO untuk STM32F103C8
2. Ketik kode program berikut:

```c
/**
 * Program 1: DAC Basic Output - STM32
 * Menghasilkan tegangan analog ramp 0-3.3V
 * Pin: PA4 (DAC_OUT1)
 */

#include "stm32f1xx_hal.h"

DAC_HandleTypeDef hdac;

void SystemClock_Config(void);
void DAC_Init(void);
void Error_Handler(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    DAC_Init();
    
    uint16_t dac_value = 0;
    
    while (1) {
        // Ramp up 0V to 3.3V
        for (dac_value = 0; dac_value < 4096; dac_value += 16) {
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
            HAL_Delay(10);
        }
        
        // Ramp down 3.3V to 0V
        for (dac_value = 4095; dac_value > 0; dac_value -= 16) {
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
            HAL_Delay(10);
        }
    }
}

void DAC_Init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA4 as analog
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure DAC
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    
    // Start DAC
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void Error_Handler(void) {
    while(1);
}
```

**Analisis:**
- DAC 12-bit menghasilkan 4096 level tegangan (0-4095)
- Tegangan output: Vout = (3.3V × DAC_Value) / 4095
- Output buffer mencegah loading effect

---

#### Program 2: DAC Sine Wave Generator (STM32)
**File:** `praktikum/STM32/STM32_02_DAC_Sine/src/main.c`

**Tujuan:** Menghasilkan gelombang sinus menggunakan DAC dengan lookup table

```c
/**
 * Program 2: DAC Sine Wave Generator - STM32
 * Menghasilkan gelombang sinus dengan lookup table
 * Pin: PA4 (DAC_OUT1)
 */

#include "stm32f1xx_hal.h"
#include <math.h>

#define SINE_SAMPLES 100
#define PI 3.14159265359

DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim6;
uint16_t sine_table[SINE_SAMPLES];

void SystemClock_Config(void);
void DAC_Init(void);
void TIM6_Init(void);
void Generate_SineTable(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    Generate_SineTable();
    DAC_Init();
    TIM6_Init();
    
    // Start DAC with DMA
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)sine_table, 
                      SINE_SAMPLES, DAC_ALIGN_12B_R);
    
    // Start Timer
    HAL_TIM_Base_Start(&htim6);
    
    while (1) {
        // Sine wave generated automatically by DMA
        HAL_Delay(1000);
    }
}

void Generate_SineTable(void) {
    for (int i = 0; i < SINE_SAMPLES; i++) {
        // Generate sine: amplitude 2048, offset 2048 (center at 1.65V)
        float angle = (2.0 * PI * i) / SINE_SAMPLES;
        sine_table[i] = (uint16_t)(2048 + 2000 * sin(angle));
    }
}

void DAC_Init(void) {
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    
    // Configure PA4
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure DAC
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
}

void TIM6_Init(void) {
    __HAL_RCC_TIM6_CLK_ENABLE();
    
    // Timer untuk sample rate 10kHz (100 samples × 100Hz sine)
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 72 - 1;        // 72MHz / 72 = 1MHz
    htim6.Init.Period = 100 - 1;          // 1MHz / 100 = 10kHz
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&htim6);
    
    // Configure TRGO
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);
}
```

---

#### Program 3: DAC Basic Output (ESP32)
**File:** `praktikum/ESP32/ESP32_01_DAC_Output/src/main.cpp`

**Tujuan:** Menghasilkan tegangan analog menggunakan DAC 8-bit ESP32

```cpp
/**
 * Program 3: DAC Basic Output - ESP32
 * Menghasilkan tegangan analog ramp 0-3.3V
 * Pin: GPIO25 (DAC1)
 */

#include <Arduino.h>

#define DAC_PIN 25  // DAC1 = GPIO25, DAC2 = GPIO26

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=================================");
    Serial.println("Program: DAC Basic Output - ESP32");
    Serial.println("=================================");
    Serial.println("DAC on GPIO25 (0-255 = 0-3.3V)");
    Serial.println();
}

void loop() {
    Serial.println("Ramp UP: 0V -> 3.3V");
    
    // Ramp up
    for (int i = 0; i < 256; i++) {
        dacWrite(DAC_PIN, i);
        
        if (i % 32 == 0) {
            float voltage = (i / 255.0) * 3.3;
            Serial.printf("DAC Value: %3d, Voltage: %.2f V\n", i, voltage);
        }
        delay(10);
    }
    
    Serial.println("\nRamp DOWN: 3.3V -> 0V");
    
    // Ramp down
    for (int i = 255; i >= 0; i--) {
        dacWrite(DAC_PIN, i);
        
        if (i % 32 == 0) {
            float voltage = (i / 255.0) * 3.3;
            Serial.printf("DAC Value: %3d, Voltage: %.2f V\n", i, voltage);
        }
        delay(10);
    }
    
    Serial.println("\n--- Cycle Complete ---\n");
    delay(1000);
}
```

---

#### Program 4: DAC Sine Wave dengan Cosine Generator (ESP32)
**File:** `praktikum/ESP32/ESP32_02_DAC_Sine_Wave/src/main.cpp`

**Tujuan:** Menggunakan hardware cosine wave generator ESP32

```cpp
/**
 * Program 4: DAC Sine Wave dengan Cosine Generator - ESP32
 * Menggunakan hardware cosine wave generator
 * Pin: GPIO25 (DAC1)
 */

#include <Arduino.h>
#include <driver/dac.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=====================================");
    Serial.println("Program: DAC Cosine Wave Generator");
    Serial.println("=====================================");
    
    // Configure cosine wave generator
    dac_cw_config_t cw_config = {
        .en_ch = DAC_CHANNEL_1,          // Use DAC1 (GPIO25)
        .scale = DAC_CW_SCALE_1,         // Full amplitude
        .phase = DAC_CW_PHASE_0,         // 0 degree phase
        .freq = 1000,                     // 1 kHz frequency
        .offset = 0                       // No DC offset
    };
    
    // Apply configuration
    ESP_ERROR_CHECK(dac_cw_generator_config(&cw_config));
    
    // Enable cosine wave generator
    ESP_ERROR_CHECK(dac_cw_generator_enable());
    
    // Enable DAC output
    ESP_ERROR_CHECK(dac_output_enable(DAC_CHANNEL_1));
    
    Serial.println("Cosine wave generator started!");
    Serial.println("Frequency: 1000 Hz");
    Serial.println("Output: GPIO25");
    Serial.println("Use oscilloscope to view waveform");
}

void loop() {
    // Demonstrate frequency change
    static uint32_t frequencies[] = {100, 500, 1000, 2000, 5000};
    static int freq_index = 0;
    
    delay(3000);
    
    freq_index = (freq_index + 1) % 5;
    
    dac_cw_config_t cw_config = {
        .en_ch = DAC_CHANNEL_1,
        .scale = DAC_CW_SCALE_1,
        .phase = DAC_CW_PHASE_0,
        .freq = frequencies[freq_index],
        .offset = 0
    };
    
    dac_cw_generator_config(&cw_config);
    
    Serial.printf("Frequency changed to: %d Hz\n", frequencies[freq_index]);
}
```

---

### Bagian B: PWM Output

---

#### Program 5: PWM LED Dimming (STM32)
**File:** `praktikum/STM32/STM32_03_PWM_LED/src/main.c`

**Tujuan:** Mengontrol kecerahan LED menggunakan PWM

```c
/**
 * Program 5: PWM LED Dimming - STM32
 * Mengontrol kecerahan LED dengan PWM
 * Pin: PA6 (TIM3_CH1)
 */

#include "stm32f1xx_hal.h"

TIM_HandleTypeDef htim3;

void SystemClock_Config(void);
void PWM_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    PWM_Init();
    
    uint16_t duty = 0;
    int8_t direction = 1;
    
    while (1) {
        // Update duty cycle
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
        
        // Fade effect
        duty += direction * 10;
        
        if (duty >= 1000) {
            direction = -1;
            duty = 1000;
        } else if (duty <= 0) {
            direction = 1;
            duty = 0;
        }
        
        HAL_Delay(20);
    }
}

void PWM_Init(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA6 as alternate function
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Timer configuration for 1kHz PWM
    // PWM Freq = 72MHz / (72 * 1000) = 1kHz
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    
    // PWM Channel configuration
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    
    // Start PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}
```

---

#### Program 6: PWM Motor Control (STM32)
**File:** `praktikum/STM32/STM32_04_PWM_Motor/src/main.c`

**Tujuan:** Mengontrol kecepatan dan arah motor DC

```c
/**
 * Program 6: PWM Motor Control - STM32
 * Mengontrol kecepatan dan arah motor DC dengan H-Bridge
 * Pin: PA6 (PWM), PA0 (DIR_A), PA1 (DIR_B)
 */

#include "stm32f1xx_hal.h"

TIM_HandleTypeDef htim3;

#define DIR_A_PIN GPIO_PIN_0
#define DIR_B_PIN GPIO_PIN_1
#define DIR_PORT  GPIOA

void SystemClock_Config(void);
void PWM_Init(void);
void GPIO_Init(void);
void Motor_SetSpeed(int16_t speed);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    PWM_Init();
    
    while (1) {
        // Forward acceleration
        for (int speed = 0; speed <= 100; speed += 5) {
            Motor_SetSpeed(speed);
            HAL_Delay(100);
        }
        
        HAL_Delay(2000);  // Run at full speed
        
        // Deceleration
        for (int speed = 100; speed >= 0; speed -= 5) {
            Motor_SetSpeed(speed);
            HAL_Delay(100);
        }
        
        HAL_Delay(1000);
        
        // Reverse acceleration
        for (int speed = 0; speed >= -100; speed -= 5) {
            Motor_SetSpeed(speed);
            HAL_Delay(100);
        }
        
        HAL_Delay(2000);
        
        // Deceleration
        for (int speed = -100; speed <= 0; speed += 5) {
            Motor_SetSpeed(speed);
            HAL_Delay(100);
        }
        
        HAL_Delay(1000);
    }
}

void Motor_SetSpeed(int16_t speed) {
    // speed: -100 to +100 (percentage)
    
    if (speed >= 0) {
        // Forward
        HAL_GPIO_WritePin(DIR_PORT, DIR_A_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(DIR_PORT, DIR_B_PIN, GPIO_PIN_RESET);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, speed * 10);
    } else {
        // Reverse
        HAL_GPIO_WritePin(DIR_PORT, DIR_A_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(DIR_PORT, DIR_B_PIN, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (-speed) * 10);
    }
}

void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DIR_A_PIN | DIR_B_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DIR_PORT, &GPIO_InitStruct);
}

void PWM_Init(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 20kHz PWM for motor (above audible range)
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 36 - 1;       // 72MHz / 36 = 2MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;        // 2MHz / 1000 = 20kHz
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}
```

---

#### Program 7: Servo Control (STM32)
**File:** `praktikum/STM32/STM32_05_Servo/src/main.c`

**Tujuan:** Mengontrol posisi servo motor

```c
/**
 * Program 7: Servo Control - STM32
 * Mengontrol posisi servo motor SG90
 * Pin: PA6 (TIM3_CH1)
 * Servo: 50Hz, 0.5ms-2.5ms pulse
 */

#include "stm32f1xx_hal.h"

TIM_HandleTypeDef htim3;

void SystemClock_Config(void);
void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    Servo_Init();
    
    while (1) {
        // Sweep 0 to 180 degrees
        for (int angle = 0; angle <= 180; angle += 5) {
            Servo_SetAngle(angle);
            HAL_Delay(50);
        }
        
        HAL_Delay(1000);
        
        // Sweep 180 to 0 degrees
        for (int angle = 180; angle >= 0; angle -= 5) {
            Servo_SetAngle(angle);
            HAL_Delay(50);
        }
        
        HAL_Delay(1000);
        
        // Test specific positions
        Servo_SetAngle(0);    HAL_Delay(1000);
        Servo_SetAngle(45);   HAL_Delay(1000);
        Servo_SetAngle(90);   HAL_Delay(1000);
        Servo_SetAngle(135);  HAL_Delay(1000);
        Servo_SetAngle(180);  HAL_Delay(1000);
    }
}

void Servo_SetAngle(uint8_t angle) {
    // Servo pulse: 0.5ms (0°) to 2.5ms (180°)
    // Period: 20ms (50Hz)
    // Timer period: 20000 (1µs resolution)
    // Pulse range: 500 to 2500
    
    if (angle > 180) angle = 180;
    
    uint16_t pulse = 500 + ((uint32_t)angle * 2000) / 180;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
}

void Servo_Init(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // 50Hz PWM for servo
    // 72MHz / 72 = 1MHz, 1MHz / 20000 = 50Hz
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;       // 1µs resolution
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 20000 - 1;       // 20ms period
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 1500;              // 90° (center)
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}
```

---

#### Program 8: PWM LED Dimming (ESP32)
**File:** `praktikum/ESP32/ESP32_03_PWM_LED_Control/src/main.cpp`

**Tujuan:** LED dimming dengan LEDC peripheral ESP32

```cpp
/**
 * Program 8: PWM LED Dimming - ESP32
 * Menggunakan LEDC peripheral untuk LED dimming
 * Pin: GPIO25
 */

#include <Arduino.h>

#define LED_PIN       25
#define PWM_CHANNEL   0
#define PWM_FREQ      5000
#define PWM_RESOLUTION 8    // 8-bit (0-255)

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("================================");
    Serial.println("Program: PWM LED Dimming - ESP32");
    Serial.println("================================");
    
    // Configure LEDC PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);
    
    Serial.printf("PWM Frequency: %d Hz\n", PWM_FREQ);
    Serial.printf("PWM Resolution: %d bit (0-%d)\n", PWM_RESOLUTION, (1 << PWM_RESOLUTION) - 1);
    Serial.println();
}

void loop() {
    Serial.println("Fade IN...");
    
    // Fade in
    for (int duty = 0; duty <= 255; duty++) {
        ledcWrite(PWM_CHANNEL, duty);
        
        if (duty % 32 == 0) {
            int percentage = (duty * 100) / 255;
            Serial.printf("Duty: %3d/255 (%3d%%)\n", duty, percentage);
        }
        delay(10);
    }
    
    delay(500);
    Serial.println("\nFade OUT...");
    
    // Fade out
    for (int duty = 255; duty >= 0; duty--) {
        ledcWrite(PWM_CHANNEL, duty);
        
        if (duty % 32 == 0) {
            int percentage = (duty * 100) / 255;
            Serial.printf("Duty: %3d/255 (%3d%%)\n", duty, percentage);
        }
        delay(10);
    }
    
    delay(500);
    Serial.println("\n--- Cycle Complete ---\n");
}
```

---

#### Program 9: Hardware Fade (ESP32)
**File:** `praktikum/ESP32/ESP32_04_PWM_Motor_Control/src/main.cpp`

**Tujuan:** Menggunakan hardware fade LEDC untuk efek smooth

```cpp
/**
 * Program 9: PWM Motor Control dengan Hardware Fade - ESP32
 * Menggunakan LEDC hardware fade untuk motor control smooth
 * Pin: GPIO25 (PWM), GPIO26 (DIR_A), GPIO27 (DIR_B)
 */

#include <Arduino.h>
#include <driver/ledc.h>

#define PWM_PIN     25
#define DIR_A_PIN   26
#define DIR_B_PIN   27

#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_TIMER   LEDC_TIMER_0
#define PWM_MODE    LEDC_HIGH_SPEED_MODE
#define PWM_FREQ    20000   // 20kHz
#define PWM_RESOLUTION LEDC_TIMER_10_BIT

void Motor_Init(void);
void Motor_SetSpeed(int speed, int fade_time_ms);
void Motor_Stop(void);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("====================================");
    Serial.println("Program: PWM Motor Control - ESP32");
    Serial.println("====================================");
    
    Motor_Init();
    
    Serial.println("Motor initialized!");
    Serial.println();
}

void loop() {
    Serial.println("Forward: Accelerate 0 -> 100%");
    digitalWrite(DIR_A_PIN, HIGH);
    digitalWrite(DIR_B_PIN, LOW);
    Motor_SetSpeed(100, 2000);  // Fade to 100% in 2 seconds
    delay(3000);
    
    Serial.println("Forward: Decelerate 100 -> 0%");
    Motor_SetSpeed(0, 2000);    // Fade to 0% in 2 seconds
    delay(1000);
    
    Serial.println("Reverse: Accelerate 0 -> 100%");
    digitalWrite(DIR_A_PIN, LOW);
    digitalWrite(DIR_B_PIN, HIGH);
    Motor_SetSpeed(100, 2000);
    delay(3000);
    
    Serial.println("Reverse: Decelerate 100 -> 0%");
    Motor_SetSpeed(0, 2000);
    delay(1000);
    
    Serial.println("\n--- Cycle Complete ---\n");
}

void Motor_Init(void) {
    // Configure direction pins
    pinMode(DIR_A_PIN, OUTPUT);
    pinMode(DIR_B_PIN, OUTPUT);
    digitalWrite(DIR_A_PIN, LOW);
    digitalWrite(DIR_B_PIN, LOW);
    
    // Configure LEDC timer
    ledc_timer_config_t timer_config = {
        .speed_mode = PWM_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);
    
    // Configure LEDC channel
    ledc_channel_config_t channel_config = {
        .gpio_num = PWM_PIN,
        .speed_mode = PWM_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_config);
    
    // Install fade function
    ledc_fade_func_install(0);
}

void Motor_SetSpeed(int speed, int fade_time_ms) {
    // speed: 0-100 (percentage)
    if (speed < 0) speed = 0;
    if (speed > 100) speed = 100;
    
    uint32_t duty = (speed * 1023) / 100;  // 10-bit resolution
    
    ledc_set_fade_time_and_start(
        PWM_MODE,
        PWM_CHANNEL,
        duty,
        fade_time_ms,
        LEDC_FADE_WAIT_DONE
    );
    
    Serial.printf("Speed set to %d%% (duty: %d)\n", speed, duty);
}

void Motor_Stop(void) {
    digitalWrite(DIR_A_PIN, LOW);
    digitalWrite(DIR_B_PIN, LOW);
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}
```

---

#### Program 10: Servo Control (ESP32)
**File:** `praktikum/ESP32/ESP32_05_Servo_Control/src/main.cpp`

**Tujuan:** Mengontrol servo dengan library ESP32Servo

```cpp
/**
 * Program 10: Servo Control - ESP32
 * Mengontrol posisi servo motor SG90
 * Pin: GPIO25
 */

#include <Arduino.h>
#include <ESP32Servo.h>

#define SERVO_PIN 25

Servo myServo;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("============================");
    Serial.println("Program: Servo Control - ESP32");
    Serial.println("============================");
    
    // Allow allocation of all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    // Attach servo (standard 500-2500µs range)
    myServo.setPeriodHertz(50);           // Standard 50Hz servo
    myServo.attach(SERVO_PIN, 500, 2500); // Min/max pulse width
    
    Serial.println("Servo attached to GPIO25");
    Serial.println();
}

void loop() {
    Serial.println("Sweep: 0° -> 180°");
    
    // Sweep from 0 to 180
    for (int angle = 0; angle <= 180; angle += 5) {
        myServo.write(angle);
        Serial.printf("Angle: %3d°\n", angle);
        delay(50);
    }
    
    delay(1000);
    
    Serial.println("\nSweep: 180° -> 0°");
    
    // Sweep from 180 to 0
    for (int angle = 180; angle >= 0; angle -= 5) {
        myServo.write(angle);
        Serial.printf("Angle: %3d°\n", angle);
        delay(50);
    }
    
    delay(1000);
    
    // Test specific positions
    Serial.println("\nTest specific positions:");
    
    int positions[] = {0, 45, 90, 135, 180};
    for (int i = 0; i < 5; i++) {
        Serial.printf("Moving to %d°\n", positions[i]);
        myServo.write(positions[i]);
        delay(1000);
    }
    
    Serial.println("\n--- Cycle Complete ---\n");
    delay(2000);
}
```

---

#### Program 11: PWM Pseudo-DAC dengan Filter (ESP32)
**File:** `praktikum/ESP32/ESP32_06_PWM_Pseudo_DAC/src/main.cpp`

**Tujuan:** Menggunakan PWM + RC filter sebagai pseudo-DAC

```cpp
/**
 * Program 11: PWM Pseudo-DAC - ESP32
 * Menggunakan PWM high-frequency + RC filter untuk pseudo-DAC
 * Pin: GPIO25 (PWM Output -> RC Filter -> Analog Out)
 * Filter: R=10kΩ, C=100nF (fc ≈ 159Hz)
 */

#include <Arduino.h>

#define PWM_PIN       25
#define PWM_CHANNEL   0
#define PWM_FREQ      100000   // 100kHz for smooth filtering
#define PWM_RESOLUTION 10      // 10-bit (0-1023)

#define ADC_PIN       34       // To measure filtered output

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("===================================");
    Serial.println("Program: PWM Pseudo-DAC - ESP32");
    Serial.println("===================================");
    Serial.println("Connect RC filter: GPIO25 -> 10k -> [ADC34] -> 100nF -> GND");
    Serial.println();
    
    // Configure high-frequency PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    
    // Configure ADC for measuring output
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    
    Serial.printf("PWM Frequency: %d Hz\n", PWM_FREQ);
    Serial.printf("PWM Resolution: %d bit\n", PWM_RESOLUTION);
    Serial.println();
}

void loop() {
    Serial.println("Generating voltage steps...\n");
    Serial.println("Target V | PWM Duty | Measured V");
    Serial.println("---------|----------|----------");
    
    // Generate voltage steps
    for (float target = 0.0; target <= 3.3; target += 0.33) {
        // Calculate duty cycle for target voltage
        int duty = (int)((target / 3.3) * 1023);
        
        // Set PWM
        ledcWrite(PWM_CHANNEL, duty);
        
        // Wait for RC filter to settle
        delay(100);
        
        // Read filtered voltage
        int adc_raw = analogRead(ADC_PIN);
        float measured = (adc_raw / 4095.0) * 3.3;
        
        Serial.printf(" %.2f V   |  %4d    |  %.2f V\n", target, duty, measured);
    }
    
    Serial.println("\nGenerating sine wave approximation...");
    
    // Generate sine wave using PWM
    for (int t = 0; t < 360; t += 5) {
        float angle = t * 3.14159 / 180.0;
        float value = (sin(angle) + 1.0) / 2.0;  // 0 to 1
        int duty = (int)(value * 1023);
        
        ledcWrite(PWM_CHANNEL, duty);
        delay(10);
    }
    
    Serial.println("\n--- Cycle Complete ---\n");
    delay(2000);
}
```

---

#### Program 12: RGB LED Color Mixing (ESP32)
**File:** `praktikum/ESP32/ESP32_07_RGB_LED_PWM/src/main.cpp`

**Tujuan:** Mengontrol LED RGB dengan 3 channel PWM

```cpp
/**
 * Program 12: RGB LED Color Mixing - ESP32
 * Mengontrol LED RGB dengan 3 channel PWM
 * Pin: GPIO25 (Red), GPIO26 (Green), GPIO27 (Blue)
 */

#include <Arduino.h>

#define RED_PIN     25
#define GREEN_PIN   26
#define BLUE_PIN    27

#define RED_CHANNEL   0
#define GREEN_CHANNEL 1
#define BLUE_CHANNEL  2

#define PWM_FREQ      5000
#define PWM_RESOLUTION 8

// Predefined colors (R, G, B)
struct Color {
    const char* name;
    uint8_t r, g, b;
};

Color colors[] = {
    {"Red",     255, 0,   0  },
    {"Green",   0,   255, 0  },
    {"Blue",    0,   0,   255},
    {"Yellow",  255, 255, 0  },
    {"Cyan",    0,   255, 255},
    {"Magenta", 255, 0,   255},
    {"White",   255, 255, 255},
    {"Orange",  255, 128, 0  },
    {"Purple",  128, 0,   255},
    {"Pink",    255, 192, 203}
};

void setRGB(uint8_t r, uint8_t g, uint8_t b);
void fadeToColor(uint8_t r, uint8_t g, uint8_t b, int duration_ms);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("================================");
    Serial.println("Program: RGB LED PWM - ESP32");
    Serial.println("================================");
    
    // Configure PWM channels
    ledcSetup(RED_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(GREEN_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(BLUE_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    
    // Attach pins
    ledcAttachPin(RED_PIN, RED_CHANNEL);
    ledcAttachPin(GREEN_PIN, GREEN_CHANNEL);
    ledcAttachPin(BLUE_PIN, BLUE_CHANNEL);
    
    Serial.println("RGB LED initialized!");
    Serial.println();
}

void loop() {
    // Display predefined colors
    Serial.println("Showing predefined colors...\n");
    
    int numColors = sizeof(colors) / sizeof(colors[0]);
    
    for (int i = 0; i < numColors; i++) {
        Serial.printf("Color: %-8s (R:%3d, G:%3d, B:%3d)\n",
                      colors[i].name,
                      colors[i].r, colors[i].g, colors[i].b);
        
        fadeToColor(colors[i].r, colors[i].g, colors[i].b, 500);
        delay(1500);
    }
    
    // Rainbow effect
    Serial.println("\nRainbow effect...");
    
    for (int hue = 0; hue < 360; hue += 2) {
        // HSV to RGB conversion (simplified)
        float h = hue / 60.0;
        int i = (int)h;
        float f = h - i;
        
        uint8_t r, g, b;
        
        switch (i % 6) {
            case 0: r = 255; g = 255 * f;     b = 0;           break;
            case 1: r = 255 * (1-f); g = 255; b = 0;           break;
            case 2: r = 0;   g = 255; b = 255 * f;             break;
            case 3: r = 0;   g = 255 * (1-f); b = 255;         break;
            case 4: r = 255 * f;     g = 0;   b = 255;         break;
            case 5: r = 255; g = 0;   b = 255 * (1-f);         break;
        }
        
        setRGB(r, g, b);
        delay(20);
    }
    
    Serial.println("\n--- Cycle Complete ---\n");
    delay(2000);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(RED_CHANNEL, r);
    ledcWrite(GREEN_CHANNEL, g);
    ledcWrite(BLUE_CHANNEL, b);
}

void fadeToColor(uint8_t r, uint8_t g, uint8_t b, int duration_ms) {
    static uint8_t current_r = 0, current_g = 0, current_b = 0;
    
    int steps = duration_ms / 10;
    
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        
        uint8_t new_r = current_r + (r - current_r) * t;
        uint8_t new_g = current_g + (g - current_g) * t;
        uint8_t new_b = current_b + (b - current_b) * t;
        
        setRGB(new_r, new_g, new_b);
        delay(10);
    }
    
    current_r = r;
    current_g = g;
    current_b = b;
}
```

---

## 📊 Tabel Perbandingan DAC vs PWM

| Karakteristik | DAC | PWM + Filter |
|--------------|-----|--------------|
| **Resolusi STM32** | 12-bit (4096 level) | Timer dependent |
| **Resolusi ESP32** | 8-bit (256 level) | 1-20 bit |
| **Output Ripple** | Sangat rendah | Tergantung filter |
| **Settling Time** | ~3µs | Tergantung RC |
| **Pin Requirement** | Dedicated (PA4/PA5, GPIO25/26) | Any GPIO |
| **CPU Load** | DMA available | Timer based |
| **Best For** | Audio, precision | LED, motor, power |

---

## 📝 Tugas Praktikum

### Tugas 1: Analisis DAC
1. Ukur tegangan output DAC pada setiap level (0, 64, 128, 192, 255 untuk ESP32)
2. Hitung error antara nilai teoritis dan terukur
3. Plot grafik linearity DAC
4. **Deliverable:** Tabel pengukuran dan analisis error

### Tugas 2: Karakterisasi PWM
1. Ukur frekuensi dan duty cycle PWM dengan oscilloscope/logic analyzer
2. Bandingkan PWM 1kHz, 5kHz, dan 20kHz untuk LED dimming
3. Amati dan dokumentasikan flicker pada setiap frekuensi
4. **Deliverable:** Screenshot waveform dan analisis

### Tugas 3: Implementasi Servo
1. Modifikasi program servo untuk mengikuti input potentiometer
2. Implementasikan smooth movement dengan interpolasi
3. Buat fungsi untuk mencatat posisi dan replay movement
4. **Deliverable:** Video demonstrasi dan kode program

### Tugas 4: Audio Generation (Tantangan)
1. Gunakan DAC untuk menghasilkan tone audio sederhana
2. Implementasikan fungsi untuk memainkan nada C, D, E, F, G, A, B
3. Buat melody sederhana
4. **Deliverable:** Video demonstrasi audio

---

## ❓ Pertanyaan Analisis

1. Mengapa ESP32 DAC hanya 8-bit sedangkan STM32 12-bit? Apa implikasinya?

2. Jelaskan mengapa frekuensi PWM 20kHz lebih baik untuk motor DC dibanding 1kHz!

3. Hitung nilai R dan C untuk filter low-pass dengan cutoff 100Hz. Mengapa cutoff ini cocok untuk audio?

4. Apa yang terjadi jika servo menerima sinyal PWM dengan periode bukan 20ms?

5. Bagaimana cara meningkatkan resolusi efektif DAC 8-bit ESP32?

---

## 🔍 Troubleshooting

| Problem | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| DAC output 0V | Pin tidak dikonfigurasi analog | Cek konfigurasi GPIO |
| PWM tidak keluar | Timer tidak start | Panggil HAL_TIM_PWM_Start() |
| Servo jitter | Interrupt mengganggu | Gunakan hardware timer |
| Motor noise | PWM freq < 20kHz | Tingkatkan frekuensi |
| LED flicker | PWM freq terlalu rendah | Minimal 100Hz untuk mata |
| DAC stepping terlihat | Resolusi kurang | Gunakan dithering/interpolasi |

---

## 📊 Rubrik Penilaian Praktikum

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| Implementasi Program | 40% | Semua 12 program berjalan dengan benar |
| Laporan & Dokumentasi | 25% | Kelengkapan, analisis, screenshot |
| Pemahaman Konsep | 20% | Jawaban pertanyaan analisis |
| Tugas Tambahan | 15% | Kreativitas, modifikasi program |

---

## 📚 Referensi Tambahan

1. [STM32 DAC Application Note AN3126](https://www.st.com/resource/en/application_note/an3126.pdf)
2. [ESP32 LEDC PWM Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html)
3. [ESP32 DAC Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html)
4. "Mastering STM32" - Chapter 11: DAC
5. PWM Application in Motor Control - Texas Instruments



-----------------------------------------------------------
--- Materi.md ---
-----------------------------------------------------------

# BAB 05: DAC (Digital-to-Analog Converter) dan PWM Output

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami prinsip kerja DAC (Digital-to-Analog Converter) dan arsitekturnya
2. Memahami konsep PWM (Pulse Width Modulation) dan aplikasinya
3. Mengkonfigurasi dan memprogram DAC pada STM32 dan ESP32
4. Mengimplementasikan PWM untuk berbagai aplikasi (LED dimming, motor control, servo)
5. Membandingkan kelebihan dan kekurangan DAC vs PWM
6. Menerapkan teknik filtering untuk menghasilkan sinyal analog dari PWM
7. Melakukan debugging dan optimasi sistem DAC/PWM

---

## 📚 Materi Pembelajaran

### 1. Pendahuluan

Dalam sistem embedded, seringkali kita perlu menghasilkan sinyal analog dari mikrokontroler yang bekerja secara digital. Ada dua metode utama untuk menghasilkan output analog:

1. **DAC (Digital-to-Analog Converter)** - Konversi langsung nilai digital ke tegangan analog
2. **PWM (Pulse Width Modulation)** - Menggunakan sinyal digital dengan duty cycle variabel yang dapat difilter menjadi analog

Kedua metode memiliki karakteristik, kelebihan, dan aplikasi yang berbeda.

### 2. Teori Dasar DAC

#### 2.1 Prinsip Kerja DAC

DAC mengkonversi nilai digital (binary) menjadi tegangan analog proporsional.

```
Vout = Vref × (Digital_Value / 2^n)

Dimana:
- Vout = Tegangan output
- Vref = Tegangan referensi
- n = Resolusi bit DAC
- Digital_Value = Nilai digital input (0 sampai 2^n - 1)
```

**Contoh:** DAC 8-bit dengan Vref = 3.3V
- Input = 0 → Vout = 0V
- Input = 127 → Vout = 1.64V
- Input = 255 → Vout = 3.3V

#### 2.2 Arsitektur DAC

##### a) R-2R Ladder DAC
```
                    Vref
                     │
              ┌──────┼──────┐
              │      R      │
              │      │      │
         D3───┤     2R      │
              │      │      │
              │      R      │
              │      │      │
         D2───┤     2R      │
              │      │      │
              │      R      │
              │      │      │
         D1───┤     2R      │
              │      │      │
              │      R      │
              │      │      │
         D0───┤     2R      │
              │      │      │
              └──────┼──────┘
                     │
                   Vout
```

**Keuntungan R-2R:**
- Hanya memerlukan 2 nilai resistor
- Akurat dan linear
- Mudah dikaskade untuk resolusi lebih tinggi

##### b) Weighted Resistor DAC
```
        R
D3 ────/\/\/──┐
       2R     │
D2 ────/\/\/──┤
       4R     │    ┌────┐
D1 ────/\/\/──┼────│    │─── Vout
       8R     │    │Op  │
D0 ────/\/\/──┘    │Amp │
                   └────┘
```

##### c) Delta-Sigma DAC
- Menggunakan oversampling dan noise shaping
- Resolusi tinggi dengan komponen sederhana
- Bandwidth lebih rendah

#### 2.3 Spesifikasi Penting DAC

| Parameter | Deskripsi |
|-----------|-----------|
| **Resolution** | Jumlah bit (8, 10, 12-bit) |
| **INL (Integral Non-Linearity)** | Deviasi dari garis transfer ideal |
| **DNL (Differential Non-Linearity)** | Error step size antar kode |
| **Settling Time** | Waktu output mencapai nilai stabil |
| **Glitch Energy** | Transient saat perubahan kode |
| **SFDR (Spurious Free Dynamic Range)** | Rasio fundamental ke spurious terbesar |

### 3. DAC pada STM32F103

#### 3.1 Fitur DAC STM32F103

**STM32F103C8T6** memiliki:
- 2 channel DAC (DAC1 dan DAC2)
- Resolusi 12-bit
- Output pada PA4 (DAC_OUT1) dan PA5 (DAC_OUT2)
- Output buffer terintegrasi
- DMA support
- Trigger dari Timer atau Software

```
┌─────────────────────────────────────────────────┐
│                   STM32F103                      │
│                                                  │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐   │
│  │  DHR12R1 │───►│  DAC     │───►│  Buffer  │──►│──PA4
│  │(Data Reg)│    │ Converter│    │ (Op-Amp) │   │
│  └──────────┘    └──────────┘    └──────────┘   │
│       ▲                │                        │
│       │                │                        │
│  ┌────┴────┐     ┌─────▼─────┐                  │
│  │ DMA/CPU │     │ Trigger   │                  │
│  └─────────┘     │ (TIM/EXTI)│                  │
│                  └───────────┘                  │
└─────────────────────────────────────────────────┘
```

#### 3.2 Register DAC STM32

| Register | Fungsi |
|----------|--------|
| DAC_CR | Control Register - enable, trigger, wave gen |
| DAC_SWTRIGR | Software Trigger Register |
| DAC_DHR12R1 | Data Holding Register 12-bit right-aligned Ch1 |
| DAC_DHR12L1 | Data Holding Register 12-bit left-aligned Ch1 |
| DAC_DHR8R1 | Data Holding Register 8-bit right-aligned Ch1 |
| DAC_DOR1 | Data Output Register Channel 1 |

#### 3.3 Mode Trigger DAC

```c
// Sumber Trigger DAC STM32
typedef enum {
    DAC_TRIGGER_NONE     = 0x00,  // Tanpa trigger, langsung convert
    DAC_TRIGGER_T6_TRGO  = 0x00,  // Timer 6 TRGO
    DAC_TRIGGER_T8_TRGO  = 0x01,  // Timer 8 TRGO
    DAC_TRIGGER_T7_TRGO  = 0x02,  // Timer 7 TRGO
    DAC_TRIGGER_T5_TRGO  = 0x03,  // Timer 5 TRGO
    DAC_TRIGGER_T2_TRGO  = 0x04,  // Timer 2 TRGO
    DAC_TRIGGER_T4_TRGO  = 0x05,  // Timer 4 TRGO
    DAC_TRIGGER_EXTI9    = 0x06,  // External interrupt line 9
    DAC_TRIGGER_SOFTWARE = 0x07   // Software trigger
} DAC_Trigger_TypeDef;
```

#### 3.4 Konfigurasi DAC STM32 dengan HAL

```c
// Struktur konfigurasi DAC
DAC_HandleTypeDef hdac;
DAC_ChannelConfTypeDef sConfig;

void DAC_Init(void) {
    // Enable clock
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA4 sebagai Analog
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Konfigurasi DAC
    hdac.Instance = DAC;
    HAL_DAC_Init(&hdac);
    
    // Konfigurasi Channel 1
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
    
    // Start DAC
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
}

// Set nilai DAC (0-4095 untuk 12-bit)
void DAC_SetValue(uint16_t value) {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
}
```

### 4. DAC pada ESP32

#### 4.1 Fitur DAC ESP32

ESP32 memiliki:
- 2 channel DAC 8-bit
- DAC1 pada GPIO25
- DAC2 pada GPIO26
- Cosine Wave Generator terintegrasi
- DMA support untuk audio

```
┌─────────────────────────────────────────────────┐
│                     ESP32                        │
│                                                  │
│  ┌──────────────┐     ┌──────────┐              │
│  │ Digital Value│────►│ 8-bit    │────►GPIO25    │
│  │   (0-255)    │     │   DAC1   │              │
│  └──────────────┘     └──────────┘              │
│                                                  │
│  ┌──────────────┐     ┌──────────┐              │
│  │ Digital Value│────►│ 8-bit    │────►GPIO26    │
│  │   (0-255)    │     │   DAC2   │              │
│  └──────────────┘     └──────────┘              │
│                                                  │
│  ┌──────────────┐                                │
│  │Cosine Wave   │───► DAC1 atau DAC2            │
│  │ Generator    │                                │
│  └──────────────┘                                │
└─────────────────────────────────────────────────┘
```

#### 4.2 Kode DAC ESP32 (Arduino)

```cpp
#include <Arduino.h>
#include <driver/dac.h>

#define DAC_CHANNEL DAC_CHANNEL_1  // GPIO25

void setup() {
    Serial.begin(115200);
    dac_output_enable(DAC_CHANNEL);
}

void loop() {
    // Ramp up (0V to 3.3V)
    for (int i = 0; i < 256; i++) {
        dac_output_voltage(DAC_CHANNEL, i);
        delay(10);
    }
    
    // Ramp down (3.3V to 0V)
    for (int i = 255; i >= 0; i--) {
        dac_output_voltage(DAC_CHANNEL, i);
        delay(10);
    }
}
```

#### 4.3 Cosine Wave Generator ESP32

```cpp
#include <driver/dac.h>

void generateCosineWave(uint8_t channel, uint32_t frequency) {
    dac_cw_config_t cw_config = {
        .en_ch = (dac_channel_t)channel,
        .scale = DAC_CW_SCALE_1,     // Amplitude: full scale
        .phase = DAC_CW_PHASE_0,     // Phase: 0 degrees
        .freq = frequency,            // Frequency in Hz
        .offset = 0                   // DC offset
    };
    
    dac_cw_generator_config(&cw_config);
    dac_cw_generator_enable();
    dac_output_enable((dac_channel_t)channel);
}
```

### 5. Teori Dasar PWM

#### 5.1 Konsep PWM

PWM adalah teknik modulasi dimana lebar pulsa (duty cycle) diatur untuk mengontrol daya rata-rata yang dikirim ke beban.

```
Duty Cycle 25%:
    ┌──┐      ┌──┐      ┌──┐      ┌──┐
    │  │      │  │      │  │      │  │
────┘  └──────┘  └──────┘  └──────┘  └──────

Duty Cycle 50%:
    ┌────┐    ┌────┐    ┌────┐    ┌────┐
    │    │    │    │    │    │    │    │
────┘    └────┘    └────┘    └────┘    └────

Duty Cycle 75%:
    ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
    │      │  │      │  │      │  │      │
────┘      └──┘      └──┘      └──┘      └──
```

**Formula:**
```
Duty Cycle (%) = (Ton / T) × 100%
Vavg = Vmax × Duty Cycle

Dimana:
- Ton = Waktu HIGH
- T = Periode (Ton + Toff)
- Vavg = Tegangan rata-rata
```

#### 5.2 Frekuensi PWM

Pemilihan frekuensi PWM sangat penting:

| Aplikasi | Frekuensi | Alasan |
|----------|-----------|--------|
| LED Dimming | 1-10 kHz | Tidak terlihat flicker (>100Hz) |
| Motor DC | 10-20 kHz | Mengurangi noise audio |
| Audio | 44.1 kHz+ | Sampling rate audio standard |
| Servo | 50 Hz | Standard hobby servo |
| Power Supply | 100 kHz+ | Komponen filter lebih kecil |

#### 5.3 Resolusi PWM

```
Resolusi = log2(Max_Count + 1)

Contoh:
- 8-bit: 0-255 (256 level)
- 10-bit: 0-1023 (1024 level)
- 16-bit: 0-65535 (65536 level)

Trade-off: Resolusi tinggi vs Frekuensi tinggi
Max_Freq = Clock / (2^Resolusi)
```

### 6. PWM pada STM32

#### 6.1 Timer untuk PWM

STM32 menggunakan Timer untuk generate PWM:

```
┌───────────────────────────────────────────────────────┐
│                    Timer Block                         │
│                                                        │
│  ┌─────────┐    ┌─────────┐    ┌──────────────────┐   │
│  │  Clock  │───►│ Prescaler│───►│   Counter (CNT)  │   │
│  │  (APB)  │    │  (PSC)   │    │                  │   │
│  └─────────┘    └─────────┘    └────────┬─────────┘   │
│                                         │              │
│                     ┌───────────────────┼───────────┐ │
│                     ▼                   ▼           │ │
│              ┌──────────┐        ┌──────────┐       │ │
│              │   ARR    │        │   CCR1   │       │ │
│              │(Period)  │        │(Compare) │       │ │
│              └────┬─────┘        └────┬─────┘       │ │
│                   │                   │             │ │
│              ┌────▼───────────────────▼────┐        │ │
│              │      Output Compare          │        │ │
│              │      Mode: PWM1/PWM2         │────────┼─┼─► PWM Output
│              └──────────────────────────────┘        │ │
└───────────────────────────────────────────────────────┘
```

#### 6.2 Konfigurasi PWM STM32 dengan HAL

```c
TIM_HandleTypeDef htim3;

void PWM_Init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    // Enable clocks
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Configure PA6 (TIM3_CH1) sebagai Alternate Function
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Timer Configuration
    // PWM Frequency = Timer_Clock / ((PSC + 1) * (ARR + 1))
    // Example: 72MHz / (72 * 1000) = 1 kHz
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;          // 72MHz / 72 = 1MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;           // 1MHz / 1000 = 1kHz
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);
    
    // PWM Channel Configuration
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;                   // 50% duty cycle
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    
    // Start PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

// Set duty cycle (0-1000 untuk period 1000)
void PWM_SetDutyCycle(uint16_t duty) {
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}
```

#### 6.3 Mode PWM

```c
// PWM Mode 1: Active saat CNT < CCR
// PWM Mode 2: Active saat CNT >= CCR

/*
PWM Mode 1 (TIM_OCMODE_PWM1):
    ┌────────┐
    │        │
    │   CCR  │
────┘        └────────────────
    |<--Ton->|<----Toff----->|
    |<-------Period--------->|

PWM Mode 2 (TIM_OCMODE_PWM2):
             ┌────────────────
             │
    CCR      │
────────────┘
    |<-Toff->|<----Ton------>|
*/
```

### 7. PWM pada ESP32

#### 7.1 LEDC (LED Control) Peripheral

ESP32 memiliki LEDC peripheral yang didesain untuk LED control tetapi dapat digunakan untuk PWM umum:

```
┌───────────────────────────────────────────────────┐
│                 ESP32 LEDC                         │
│                                                    │
│  High Speed Channels (0-7):                        │
│  ┌────────┐    ┌────────┐    ┌────────┐           │
│  │Timer 0 │───►│ Ch 0-1 │───►│GPIO Out│           │
│  └────────┘    └────────┘    └────────┘           │
│  ┌────────┐    ┌────────┐    ┌────────┐           │
│  │Timer 1 │───►│ Ch 2-3 │───►│GPIO Out│           │
│  └────────┘    └────────┘    └────────┘           │
│  ...                                               │
│                                                    │
│  Low Speed Channels (8-15):                        │
│  ┌────────┐    ┌────────┐    ┌────────┐           │
│  │Timer 0 │───►│ Ch 8-9 │───►│GPIO Out│           │
│  └────────┘    └────────┘    └────────┘           │
│  ...                                               │
└───────────────────────────────────────────────────┘
```

**Fitur LEDC ESP32:**
- 16 channel independen
- 4 timer (0-3) per speed mode
- Resolusi 1-20 bit
- Hardware fade support
- Frekuensi hingga 40 MHz

#### 7.2 Kode PWM ESP32 (Arduino)

```cpp
#include <Arduino.h>

#define PWM_PIN      25
#define PWM_CHANNEL  0
#define PWM_FREQ     5000
#define PWM_RESOLUTION 8  // 0-255

void setup() {
    // Configure PWM
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
}

void loop() {
    // Fade in
    for (int duty = 0; duty <= 255; duty++) {
        ledcWrite(PWM_CHANNEL, duty);
        delay(10);
    }
    
    // Fade out
    for (int duty = 255; duty >= 0; duty--) {
        ledcWrite(PWM_CHANNEL, duty);
        delay(10);
    }
}
```

#### 7.3 Hardware Fade ESP32

```cpp
#include <driver/ledc.h>

void setup() {
    // Configure timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    // Configure channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = 25,
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    
    // Install fade function
    ledc_fade_func_install(0);
}

void fadeToTarget(uint32_t target_duty, int fade_time_ms) {
    ledc_set_fade_time_and_start(
        LEDC_HIGH_SPEED_MODE,
        LEDC_CHANNEL_0,
        target_duty,
        fade_time_ms,
        LEDC_FADE_WAIT_DONE
    );
}
```

### 8. Aplikasi PWM

#### 8.1 Kontrol Motor DC

```cpp
// H-Bridge motor control dengan PWM
#define MOTOR_PWM_PIN  25
#define MOTOR_DIR_A    26
#define MOTOR_DIR_B    27

void motorSetSpeed(int speed) {
    // speed: -255 to 255
    if (speed >= 0) {
        // Forward
        digitalWrite(MOTOR_DIR_A, HIGH);
        digitalWrite(MOTOR_DIR_B, LOW);
        ledcWrite(0, speed);
    } else {
        // Reverse
        digitalWrite(MOTOR_DIR_A, LOW);
        digitalWrite(MOTOR_DIR_B, HIGH);
        ledcWrite(0, -speed);
    }
}
```

#### 8.2 Kontrol Servo Motor

```
Servo Signal Timing:
┌────┐                              ┌────┐
│    │                              │    │
│0.5ms│ = 0° (min)                  │    │
└────┴──────────────────────────────┘    └───

┌────────┐                          ┌────────┐
│        │                          │        │
│ 1.5ms  │ = 90° (center)           │        │
└────────┴──────────────────────────┘        └─

┌────────────┐                      ┌───────────
│            │                      │
│   2.5ms    │ = 180° (max)         │
└────────────┴──────────────────────┘

Period = 20ms (50Hz)
```

```cpp
// ESP32 Servo Control
#include <ESP32Servo.h>

Servo myServo;

void setup() {
    myServo.attach(25);  // Servo pada GPIO25
}

void loop() {
    // Sweep 0 to 180 degrees
    for (int angle = 0; angle <= 180; angle++) {
        myServo.write(angle);
        delay(15);
    }
    
    // Sweep 180 to 0 degrees
    for (int angle = 180; angle >= 0; angle--) {
        myServo.write(angle);
        delay(15);
    }
}
```

#### 8.3 PWM sebagai Pseudo-DAC

```cpp
// PWM + Low-pass filter = Pseudo DAC
// RC Filter: R = 10kΩ, C = 100nF
// Cutoff frequency: fc = 1/(2πRC) = 159 Hz

/*
PWM Out ─────/\/\/\──────┬────── Analog Out
              R          │
                         C
                         │
                        GND
*/

// Untuk audio quality yang baik:
// - Gunakan PWM frequency >> audio frequency
// - Gunakan multiple RC stages untuk filtering lebih baik
// - Atau gunakan active low-pass filter
```

### 9. Perbandingan DAC vs PWM

| Aspek | DAC | PWM |
|-------|-----|-----|
| **Resolusi** | Biasanya 8-12 bit | Dapat >16 bit |
| **Bandwidth** | DC hingga MHz | Terbatas oleh filter |
| **Ripple** | Sangat rendah | Ada ripple (perlu filter) |
| **Pin Requirement** | Dedicated DAC pin | Any GPIO |
| **Power Efficiency** | Moderate | Tinggi (switching) |
| **Cost** | Perlu DAC peripheral | Hanya timer |
| **Audio Quality** | Lebih baik | Perlu filtering |

### 10. Best Practices

#### 10.1 DAC Best Practices

```c
// 1. Gunakan buffer untuk menghindari loading effect
HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

// 2. Gunakan DMA untuk waveform generation
HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)sine_wave, 
                  SINE_SAMPLES, DAC_ALIGN_12B_R);

// 3. Synchronize dengan timer untuk timing akurat
sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;

// 4. Pertimbangkan settling time
// STM32F103: ~3µs settling time
```

#### 10.2 PWM Best Practices

```c
// 1. Pilih frekuensi sesuai aplikasi
// LED: 1-10 kHz, Motor: 10-20 kHz, Audio: >40 kHz

// 2. Gunakan dead-time untuk H-bridge
// Mencegah shoot-through current
TIM_BDTRInitStruct.DeadTime = 100;  // Dead time cycles

// 3. Soft-start untuk motor
void motorSoftStart(int targetSpeed) {
    for (int i = 0; i <= targetSpeed; i += 5) {
        PWM_SetDutyCycle(i);
        delay(10);
    }
}

// 4. Gunakan complementary output untuk full-bridge
HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
```

### 11. Troubleshooting

| Problem | Kemungkinan Penyebab | Solusi |
|---------|---------------------|--------|
| DAC output tidak stabil | Loading effect | Enable output buffer |
| PWM glitch saat update | Non-atomic update | Gunakan shadow register |
| Motor noise | PWM freq dalam audio range | Tingkatkan ke >20kHz |
| Servo jitter | Interrupt mengganggu timing | Gunakan hardware PWM |
| DAC stepping | Resolusi kurang | Gunakan dithering |

### 12. Rangkuman

1. **DAC** mengkonversi nilai digital langsung ke tegangan analog dengan kualitas tinggi
2. **PWM** menggunakan duty cycle untuk mengontrol daya rata-rata
3. **STM32F103** memiliki 2-channel 12-bit DAC dan multiple timer untuk PWM
4. **ESP32** memiliki 2-channel 8-bit DAC dan 16-channel LEDC untuk PWM
5. Pilihan antara DAC dan PWM tergantung pada aplikasi:
   - DAC: Audio, precision analog, fast response
   - PWM: LED dimming, motor control, power efficient
6. PWM dapat difilter menjadi pseudo-DAC dengan RC filter

---

## 📊 Diagram Perbandingan Platform

```
┌────────────────────────────────────────────────────────────────┐
│                    DAC & PWM Comparison                         │
├──────────────────────┬─────────────────────┬───────────────────┤
│      Feature         │     STM32F103       │      ESP32        │
├──────────────────────┼─────────────────────┼───────────────────┤
│ DAC Channels         │         2           │        2          │
│ DAC Resolution       │      12-bit         │      8-bit        │
│ DAC Pins             │    PA4, PA5         │   GPIO25, 26      │
│ DAC DMA              │        Yes          │       Yes         │
│ Waveform Generator   │    Triangle/Noise   │   Cosine          │
├──────────────────────┼─────────────────────┼───────────────────┤
│ PWM Timers           │    TIM1-4,5,8       │    LEDC           │
│ PWM Channels         │    Up to 32         │       16          │
│ PWM Resolution       │     16-bit          │    1-20 bit       │
│ Hardware Fade        │        No           │       Yes         │
│ Complementary PWM    │   TIM1, TIM8        │       No          │
└──────────────────────┴─────────────────────┴───────────────────┘
```

---

## 📖 Referensi

1. STM32F103 Reference Manual (RM0008)
2. ESP32 Technical Reference Manual
3. "Mastering STM32" by Carmine Noviello - Chapter 11: DAC
4. "PWM Techniques: A Pure Sine Wave Inverter" - Application Note
5. ESP-IDF Programming Guide - LEDC PWM Controller
6. AN3126: Audio and waveform generation using DAC in STM32

---

## 🔗 Link Terkait

- [Modul 04: ADC](../Modul-04-ADC/Materi.md) - Input Analog
- [Modul 06: I2C](../Modul-06-I2C-Sensor/Materi.md) - Komunikasi dengan sensor
- [Modul 08: DMA](../Modul-08-DMA/Materi.md) - Transfer data efisien untuk DAC



-----------------------------------------------------------
--- PPT_Prompts_1.md ---
-----------------------------------------------------------

# Prompt untuk Pembuatan PPT - Bagian 1
## Modul 05: DAC & PWM Output

### 📌 Informasi Umum
- **Total Slide:** 25-30 slide
- **Durasi Presentasi:** 45-50 menit
- **Target Audiens:** Mahasiswa Teknik Elektro/Informatika semester 4-5

---

## SLIDE 1: Judul
**Prompt:**
"Buatkan slide judul dengan desain modern dan profesional untuk materi kuliah 'BAB 05: DAC dan PWM Output'. Sertakan:
- Judul utama: 'DAC (Digital-to-Analog Converter) dan PWM (Pulse Width Modulation)'
- Subtitle: 'Praktikum Sistem Embedded'
- Logo institusi (placeholder)
- Informasi: 'Pertemuan 5 | Platform: STM32F103 & ESP32'
- Warna tema: Biru elektrik dan oranye
- Gambar ilustrasi: Waveform analog dan digital"

---

## SLIDE 2: Capaian Pembelajaran
**Prompt:**
"Buatkan slide Capaian Pembelajaran dengan layout yang jelas dan icon untuk setiap poin:
1. 🎯 Memahami prinsip kerja DAC dan arsitekturnya
2. 📊 Memahami konsep PWM dan aplikasinya
3. 🔧 Mengkonfigurasi DAC pada STM32 (12-bit) dan ESP32 (8-bit)
4. ⚡ Mengimplementasikan PWM untuk LED dimming, motor, servo
5. 📈 Membandingkan DAC vs PWM untuk output analog
6. 🔬 Menerapkan teknik filtering PWM
Gunakan desain dengan progress bar atau checklist visual"

---

## SLIDE 3: Outline Materi
**Prompt:**
"Buatkan slide outline/daftar isi dengan timeline visual:
1. Teori Dasar DAC (15 menit)
   - Prinsip Konversi D/A
   - Arsitektur DAC (R-2R, Weighted)
2. DAC pada Mikrokontroler (10 menit)
   - STM32 DAC 12-bit
   - ESP32 DAC 8-bit
3. Teori Dasar PWM (10 menit)
   - Konsep Duty Cycle
   - Frekuensi dan Resolusi
4. PWM pada Mikrokontroler (10 menit)
   - Timer STM32
   - LEDC ESP32
5. Aplikasi & Praktikum (15 menit)
Desain dengan roadmap atau flowchart horizontal"

---

## SLIDE 4: Mengapa Output Analog?
**Prompt:**
"Buatkan slide pengantar dengan ilustrasi perbandingan:
- Judul: 'Mengapa Perlu Output Analog?'
- Tampilkan diagram mikrokontroler digital yang perlu mengontrol:
  - 💡 Kecerahan LED (0-100%)
  - ⚙️ Kecepatan Motor (0-max RPM)
  - 🔊 Audio/Speaker (waveform)
  - 🎚️ Posisi Servo (0°-180°)
- Dua solusi: DAC (True Analog) vs PWM (Pseudo Analog)
- Gunakan animasi perbandingan sinyal digital vs analog"

---

## SLIDE 5: Prinsip Kerja DAC
**Prompt:**
"Buatkan slide dengan diagram blok DAC:
- Judul: 'Prinsip Kerja DAC'
- Diagram konversi: Digital Input (Binary) → DAC → Analog Output (Voltage)
- Formula dengan penjelasan visual:
  ```
  Vout = Vref × (Digital_Value / 2^n)
  ```
- Contoh konkret: DAC 8-bit, Vref=3.3V
  - Input 0 → 0V
  - Input 127 → 1.64V
  - Input 255 → 3.3V
- Gunakan grafik tangga (staircase) untuk menunjukkan level diskrit"

---

## SLIDE 6: Arsitektur R-2R Ladder DAC
**Prompt:**
"Buatkan slide dengan diagram skematik R-2R Ladder:
- Judul: 'R-2R Ladder DAC'
- Gambar rangkaian R-2R 4-bit lengkap dengan:
  - Resistor R dan 2R
  - Input D0-D3
  - Output Vout
- Penjelasan keuntungan:
  ✓ Hanya 2 nilai resistor
  ✓ Akurasi tinggi
  ✓ Mudah dikaskade
- Animasi step-by-step aliran arus"

---

## SLIDE 7: Spesifikasi DAC
**Prompt:**
"Buatkan slide tabel spesifikasi DAC dengan visualisasi:
| Parameter | Simbol | Deskripsi | Ilustrasi |
|-----------|--------|-----------|-----------|
| Resolution | n-bit | Jumlah level output | Grafik step |
| INL | ±LSB | Linearity error | Kurva deviasi |
| DNL | ±LSB | Step size error | Diagram step |
| Settling Time | µs | Waktu stabilisasi | Waveform |
| Glitch Energy | nV·s | Transient energy | Spike |
Gunakan ikon dan mini-grafik untuk setiap parameter"

---

## SLIDE 8: DAC pada STM32F103
**Prompt:**
"Buatkan slide dengan diagram blok DAC STM32:
- Judul: 'DAC STM32F103C8T6'
- Spesifikasi:
  - 2 Channel DAC
  - Resolusi 12-bit (0-4095)
  - Pin: PA4 (DAC1), PA5 (DAC2)
  - Output Buffer terintegrasi
  - DMA Support
  - Timer Trigger
- Diagram internal: Data Register → Converter → Buffer → Pin
- Highlight fitur utama dengan badge/label"

---

## SLIDE 9: Register DAC STM32
**Prompt:**
"Buatkan slide dengan tabel register DAC:
- Judul: 'Register DAC STM32'
- Tabel dengan highlight warna:
  | Register | Alamat | Fungsi |
  |----------|--------|--------|
  | DAC_CR | 0x400x | Control (Enable, Trigger) |
  | DAC_DHR12R1 | 0x400x | Data 12-bit right-aligned |
  | DAC_DHR12L1 | 0x400x | Data 12-bit left-aligned |
  | DAC_DOR1 | 0x400x | Data Output |
- Diagram bit-field untuk DAC_CR
- Warna berbeda untuk read/write register"

---

## SLIDE 10: Kode DAC STM32 (HAL)
**Prompt:**
"Buatkan slide dengan code snippet dan penjelasan:
- Judul: 'Konfigurasi DAC STM32 dengan HAL'
- Kode dengan syntax highlighting:
```c
// 1. Enable Clock
__HAL_RCC_DAC_CLK_ENABLE();

// 2. Configure GPIO sebagai Analog
GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

// 3. Configure DAC Channel
sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

// 4. Set Value (0-4095)
HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, 
                 DAC_ALIGN_12B_R, value);
```
- Annotasi pada setiap bagian penting
- Flow diagram di samping kode"

---

## SLIDE 11: DAC pada ESP32
**Prompt:**
"Buatkan slide perbandingan DAC ESP32:
- Judul: 'DAC ESP32'
- Spesifikasi:
  - 2 Channel DAC 8-bit
  - DAC1: GPIO25
  - DAC2: GPIO26
  - Cosine Wave Generator built-in
  - DMA untuk audio
- Diagram pin mapping ESP32
- Perbandingan vs STM32:
  | Feature | STM32 | ESP32 |
  |---------|-------|-------|
  | Resolution | 12-bit | 8-bit |
  | Channels | 2 | 2 |
  | Wave Gen | Triangle/Noise | Cosine |"

---

## SLIDE 12: Kode DAC ESP32
**Prompt:**
"Buatkan slide dengan code snippet ESP32:
- Judul: 'DAC ESP32 - Arduino Framework'
- Kode dengan syntax highlighting:
```cpp
#include <Arduino.h>

#define DAC_PIN 25  // GPIO25 = DAC1

void setup() {
    // DAC tidak perlu konfigurasi khusus
}

void loop() {
    // Ramp up 0V -> 3.3V
    for (int i = 0; i < 256; i++) {
        dacWrite(DAC_PIN, i);  // 8-bit: 0-255
        delay(10);
    }
}
```
- Bandingkan dengan ESP-IDF native API
- Note: Keterbatasan 8-bit resolution"

---

## SLIDE 13: Cosine Wave Generator ESP32
**Prompt:**
"Buatkan slide fitur unik ESP32:
- Judul: 'Hardware Cosine Wave Generator'
- Diagram blok cosine generator
- Kode konfigurasi:
```cpp
dac_cw_config_t config = {
    .en_ch = DAC_CHANNEL_1,
    .scale = DAC_CW_SCALE_1,
    .phase = DAC_CW_PHASE_0,
    .freq = 1000,  // 1 kHz
};
dac_cw_generator_config(&config);
dac_cw_generator_enable();
```
- Visualisasi parameter: scale, phase, frequency
- Use case: Signal generator, audio synthesis"

---

## SLIDE 14: Konsep PWM
**Prompt:**
"Buatkan slide penjelasan PWM dengan animasi:
- Judul: 'Pulse Width Modulation (PWM)'
- Diagram waveform untuk berbagai duty cycle:
  - 25% duty: ▁█▁▁▁█▁▁
  - 50% duty: ▁██▁▁██▁
  - 75% duty: ▁███▁███
- Formula:
  - Duty Cycle (%) = (Ton / T) × 100%
  - Vavg = Vmax × Duty Cycle
- Visualisasi tegangan rata-rata dengan area shading
- Animasi perubahan duty cycle"

---

## SLIDE 15: Frekuensi dan Resolusi PWM
**Prompt:**
"Buatkan slide dengan tabel aplikasi:
- Judul: 'Memilih Frekuensi PWM'
- Tabel aplikasi:
  | Aplikasi | Frekuensi | Alasan |
  |----------|-----------|--------|
  | LED | 1-10 kHz | Anti-flicker |
  | Motor DC | 10-20 kHz | Above audible |
  | Servo | 50 Hz | Standard |
  | Audio | >44 kHz | CD quality |
- Grafik trade-off: Resolusi vs Frekuensi
- Formula: Max_Freq = Clock / 2^Resolution
- Diagram visual resolusi 8-bit vs 16-bit"

---

## SLIDE 16: PWM pada STM32
**Prompt:**
"Buatkan slide dengan diagram timer STM32:
- Judul: 'PWM STM32 - Timer Architecture'
- Diagram blok lengkap:
  - Clock Source → Prescaler (PSC)
  - Counter (CNT) → Compare (CCR)
  - Output Compare → PWM Pin
- Formula:
  - PWM_Freq = Clock / ((PSC+1) × (ARR+1))
- Contoh perhitungan:
  - 72MHz / (72 × 1000) = 1 kHz
- Highlight timer yang support PWM: TIM1-4, TIM5, TIM8"

---

## SLIDE 17: Konfigurasi PWM STM32
**Prompt:**
"Buatkan slide dengan code dan diagram:
- Judul: 'Konfigurasi PWM STM32 HAL'
```c
// Timer Configuration
htim3.Init.Prescaler = 72 - 1;    // 1MHz
htim3.Init.Period = 1000 - 1;     // 1kHz PWM
HAL_TIM_PWM_Init(&htim3);

// Channel Configuration
sConfigOC.OCMode = TIM_OCMODE_PWM1;
sConfigOC.Pulse = 500;            // 50% duty
HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);

// Start PWM
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

// Update duty cycle
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, new_duty);
```
- Diagram timing dengan ARR dan CCR"

---

## SLIDE 18: LEDC ESP32
**Prompt:**
"Buatkan slide LEDC peripheral ESP32:
- Judul: 'ESP32 LEDC (LED Controller)'
- Diagram arsitektur:
  - High Speed (8 channel)
  - Low Speed (8 channel)
  - 4 Timer per mode
- Fitur unggulan:
  - ✓ 16 channel independen
  - ✓ Resolusi 1-20 bit
  - ✓ Hardware fade
  - ✓ Frekuensi hingga 40MHz
- Perbandingan dengan STM32 timer"

---

## SLIDE 19: Kode PWM ESP32
**Prompt:**
"Buatkan slide dengan code ESP32:
- Judul: 'PWM ESP32 - LEDC'
```cpp
#define PWM_PIN       25
#define PWM_CHANNEL   0
#define PWM_FREQ      5000   // 5 kHz
#define PWM_RESOLUTION 8     // 8-bit

void setup() {
    // Setup
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
}

void loop() {
    // Fade
    for (int duty = 0; duty <= 255; duty++) {
        ledcWrite(PWM_CHANNEL, duty);
        delay(10);
    }
}
```
- Highlight kemudahan dibanding STM32
- Note tentang ESP32 Arduino Core API"

---

## SLIDE 20: Hardware Fade ESP32
**Prompt:**
"Buatkan slide fitur hardware fade:
- Judul: 'ESP32 Hardware Fade'
- Penjelasan: CPU-free fade operation
- Kode:
```cpp
// Install fade function
ledc_fade_func_install(0);

// Fade to target
ledc_set_fade_time_and_start(
    LEDC_HIGH_SPEED_MODE,
    LEDC_CHANNEL_0,
    target_duty,
    fade_time_ms,
    LEDC_FADE_WAIT_DONE
);
```
- Diagram timing fade linear
- Use case: Smooth LED transitions, motor soft-start"

---

## SLIDE 21: Aplikasi - Motor Control
**Prompt:**
"Buatkan slide aplikasi motor:
- Judul: 'PWM Motor DC Control'
- Diagram H-Bridge dengan PWM:
  - PWM → Enable
  - DIR_A, DIR_B → Direction
- Kode pseudo:
```
if (speed >= 0) {
    DIR_A = HIGH, DIR_B = LOW
    PWM = speed
} else {
    DIR_A = LOW, DIR_B = HIGH
    PWM = -speed
}
```
- Waveform PWM 20kHz untuk motor
- Tips: Soft-start untuk mengurangi inrush current"

---

## SLIDE 22: Aplikasi - Servo Control
**Prompt:**
"Buatkan slide servo control:
- Judul: 'PWM Servo Motor Control'
- Diagram timing servo:
  - 50Hz (20ms period)
  - 0.5ms = 0°
  - 1.5ms = 90°
  - 2.5ms = 180°
- Ilustrasi posisi servo vs pulse width
- Kode konversi angle ke pulse:
```c
pulse = 500 + (angle * 2000 / 180);  // µs
```
- Warning tentang timing accuracy"

---

## SLIDE 23: DAC vs PWM Comparison
**Prompt:**
"Buatkan slide perbandingan komprehensif:
- Judul: 'DAC vs PWM: Kapan Menggunakan?'
- Tabel perbandingan dengan ikon:
  | Aspek | DAC | PWM |
  |-------|-----|-----|
  | Resolution | 8-12 bit | Unlimited |
  | Ripple | ✓ Low | ✗ High |
  | Pin | Dedicated | Any GPIO |
  | Efficiency | Medium | High |
  | Audio | ✓ Better | Needs filter |
  | Motor | ✗ | ✓ Best |
- Diagram use case untuk masing-masing"

---

## SLIDE 24: PWM sebagai Pseudo-DAC
**Prompt:**
"Buatkan slide teknik filtering:
- Judul: 'PWM + RC Filter = Pseudo-DAC'
- Diagram rangkaian:
  PWM → R (10kΩ) → [Output] → C (100nF) → GND
- Formula cutoff:
  fc = 1 / (2πRC) = 159 Hz
- Waveform sebelum dan sesudah filter
- Tips:
  - PWM freq >> cutoff freq
  - Multiple RC stages untuk quality lebih baik
- Perbandingan quality vs True DAC"

---

## SLIDE 25: Demo & Praktikum
**Prompt:**
"Buatkan slide overview praktikum:
- Judul: 'Praktikum: DAC & PWM'
- Daftar percobaan dengan progress tracker:
  □ DAC Ramp Output (STM32 & ESP32)
  □ DAC Sine Wave Generator
  □ PWM LED Dimming
  □ PWM Motor Control
  □ Servo Control
  □ PWM Pseudo-DAC
  □ RGB LED Color Mixing
- Deliverables:
  - Laporan dengan screenshot waveform
  - Video demonstrasi
  - Analisis perbandingan"

---

## SLIDE 26: Kesimpulan
**Prompt:**
"Buatkan slide kesimpulan dengan summary visual:
- Judul: 'Kesimpulan'
- Key takeaways dengan ikon:
  1. 📊 DAC: Konversi digital → analog langsung
  2. 📈 PWM: Duty cycle untuk kontrol daya
  3. 🔧 STM32: DAC 12-bit, Timer untuk PWM
  4. ⚡ ESP32: DAC 8-bit, LEDC dengan hardware fade
  5. 🎯 Pilihan tergantung aplikasi
- Diagram decision tree: Kapan DAC vs PWM
- QR code ke referensi tambahan"

---

## SLIDE 27: Q&A
**Prompt:**
"Buatkan slide Q&A yang interaktif:
- Judul: 'Pertanyaan & Diskusi'
- Beberapa pertanyaan pemicu:
  - 'Mengapa servo perlu tepat 50Hz?'
  - 'Bagaimana meningkatkan resolusi DAC 8-bit?'
  - 'Kapan PWM filtering tidak cukup?'
- Space untuk catatan
- Ikon tangan terangkat dan speech bubble
- Contact info untuk pertanyaan lanjutan"

---

## 🎨 Panduan Desain

### Warna Tema
- Primary: #2196F3 (Biru)
- Secondary: #FF9800 (Oranye)
- Accent: #4CAF50 (Hijau)
- Background: #FAFAFA (Light gray)
- Text: #212121 (Dark gray)

### Font
- Heading: Roboto Bold, 32-44pt
- Body: Open Sans Regular, 18-24pt
- Code: Fira Code / JetBrains Mono, 14-16pt

### Elemen Visual
- Gunakan diagram waveform untuk setiap konsep
- Animasi untuk proses konversi
- Code dengan syntax highlighting
- Perbandingan side-by-side STM32 vs ESP32

### Tips Presentasi
1. Demo langsung dengan oscilloscope jika tersedia
2. Tunjukkan waveform real-time
3. Biarkan mahasiswa mengubah parameter
4. Bandingkan output DAC vs PWM filtered


-----------------------------------------------------------
--- PPT_Prompts_2.md ---
-----------------------------------------------------------

# Prompt untuk Pembuatan PPT - Bagian 2
## Modul 05: DAC & PWM - Lanjutan & Aplikasi

### 📌 Informasi
- **Slide:** 28-50 (Lanjutan dari PPT_Prompts_1)
- **Fokus:** Aplikasi lanjutan, troubleshooting, dan studi kasus
- **Durasi:** 30-40 menit

---

## SLIDE 28: Waveform Generation dengan DAC
**Prompt:**
"Buatkan slide tentang waveform generation:
- Judul: 'DAC Waveform Generation'
- Jenis waveform yang dapat dihasilkan:
  - Sine Wave (lookup table)
  - Triangle Wave
  - Sawtooth Wave
  - Square Wave
  - Custom Waveform
- Diagram lookup table untuk sine:
```
Index:  0   1   2   3   ...  99
Value: 2048 2176 2303 2426 ... 2048
```
- Formula: value = 2048 + 2000 × sin(2πi/N)
- Ilustrasi DMA circular buffer untuk continuous output"

---

## SLIDE 29: DAC dengan DMA
**Prompt:**
"Buatkan slide DMA-driven DAC:
- Judul: 'DAC + DMA: Efisiensi Maksimal'
- Diagram aliran data:
  Memory [Waveform Table] → DMA → DAC → Analog Out
- Keuntungan:
  ✓ CPU free saat generate waveform
  ✓ Timing presisi
  ✓ Continuous output
- Kode konfigurasi:
```c
HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, 
                  (uint32_t*)sine_table, 
                  SAMPLES, DAC_ALIGN_12B_R);
```
- Perbandingan CPU load: Polling vs DMA"

---

## SLIDE 30: Audio DAC
**Prompt:**
"Buatkan slide aplikasi audio:
- Judul: 'DAC untuk Audio Output'
- Spesifikasi audio standar:
  - Sample Rate: 44.1 kHz (CD quality)
  - Bit Depth: 16-bit ideal, 12-bit acceptable
- Diagram sistem audio DAC:
  Audio Data → Buffer → DAC → Amplifier → Speaker
- Challenge pada mikrokontroler:
  - STM32: 12-bit cukup untuk voice
  - ESP32: 8-bit, perlu dithering
- Tips quality improvement:
  - Oversampling
  - Interpolation
  - External DAC (I2S)"

---

## SLIDE 31: Multi-Channel PWM
**Prompt:**
"Buatkan slide multi-channel PWM:
- Judul: 'PWM Multi-Channel: RGB LED & Motor'
- Diagram 3 channel PWM untuk RGB:
  - Channel 0 → Red LED
  - Channel 1 → Green LED  
  - Channel 2 → Blue LED
- Color mixing visualization
- Kode ESP32:
```cpp
ledcSetup(0, 5000, 8);  // Red
ledcSetup(1, 5000, 8);  // Green
ledcSetup(2, 5000, 8);  // Blue

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(0, r);
    ledcWrite(1, g);
    ledcWrite(2, b);
}
```
- HSV to RGB conversion concept"

---

## SLIDE 32: Complementary PWM
**Prompt:**
"Buatkan slide PWM komplementer:
- Judul: 'Complementary PWM untuk H-Bridge'
- Diagram H-Bridge dengan:
  - PWM_H (High-side)
  - PWM_L (Low-side)
  - Dead-time protection
- Waveform dengan dead-time:
```
PWM_H: ▁▁█████▁▁▁▁▁▁▁▁█████▁▁
PWM_L: ▁▁▁▁▁▁▁██████▁▁▁▁▁▁▁██
       ↑      ↑
    Dead-time gaps
```
- STM32 TIM1/TIM8 support
- Konfigurasi dead-time:
```c
TIM_BDTRInitStruct.DeadTime = 100;
```
- Mencegah shoot-through current"

---

## SLIDE 33: PWM Input Capture
**Prompt:**
"Buatkan slide PWM input:
- Judul: 'PWM Input Capture & Measurement'
- Diagram pengukuran PWM:
  - Rising edge → Start capture
  - Falling edge → High time
  - Next rising → Period
- Formula:
  - Frequency = Timer_Clock / Captured_Period
  - Duty = High_Time / Period × 100%
- Use case:
  - RC receiver signal decode
  - Fan speed feedback (tachometer)
  - Sensor dengan output PWM"

---

## SLIDE 34: Servo Advanced
**Prompt:**
"Buatkan slide servo lanjutan:
- Judul: 'Servo Motor: Beyond Basic Control'
- Jenis servo:
  - Standard (180°): 0.5-2.5ms
  - Continuous rotation: Speed control
  - High-precision digital: Feedback
- Multi-servo control:
  - Max 8-16 servo per timer
  - Software PWM untuk lebih banyak
- Diagram 6-DOF robot arm dengan servo
- Library ESP32Servo features:
  - attach(pin, min, max)
  - writeMicroseconds(us)
- Tips: External power untuk multiple servo"

---

## SLIDE 35: PWM untuk Power Control
**Prompt:**
"Buatkan slide power control:
- Judul: 'PWM Power Control Applications'
- Aplikasi:
  1. LED Driver (Buck converter)
  2. Motor Speed Control
  3. Heater Control (SSR)
  4. Battery Charging (CC/CV)
- Diagram Buck Converter dengan PWM:
```
Vin → [MOSFET] → L → Vout
         ↑        ↓
       PWM       C
```
- Frekuensi switching: 100kHz - 1MHz
- Duty cycle vs output voltage:
  Vout = Vin × Duty_Cycle"

---

## SLIDE 36: Studi Kasus - LED Dimmer
**Prompt:**
"Buatkan slide studi kasus:
- Judul: 'Studi Kasus: Smart LED Dimmer'
- Spesifikasi project:
  - Input: Potentiometer / Button
  - Output: LED brightness
  - Feature: Fade effect, memory
- Block diagram sistem
- Perbandingan implementasi:
  | Method | Pro | Con |
  |--------|-----|-----|
  | DAC | Smooth | Limited pins |
  | PWM | Any GPIO | Flicker risk |
- Kode dengan smooth transition
- Demo video placeholder"

---

## SLIDE 37: Studi Kasus - Motor Speed
**Prompt:**
"Buatkan slide motor control project:
- Judul: 'Studi Kasus: DC Motor Speed Controller'
- Requirements:
  - Speed: 0-100% with soft start
  - Direction: Forward/Reverse
  - Display: Speed percentage
- Hardware diagram:
  MCU → PWM → H-Bridge → Motor
  ↓
  OLED Display
- PID control concept (basic):
  - Setpoint vs Actual speed
  - Tachometer feedback
- Safety: Current limiting, thermal protection"

---

## SLIDE 38: Studi Kasus - Function Generator
**Prompt:**
"Buatkan slide function generator:
- Judul: 'Mini Project: Function Generator'
- Fitur:
  - Waveform: Sine, Square, Triangle, Sawtooth
  - Frequency: 1Hz - 10kHz
  - Amplitude: 0-3.3V
- Block diagram:
  UI → MCU → DAC → Buffer → Output
- Waveform selection dengan rotary encoder
- Display: Waveform preview pada OLED
- STM32 lebih cocok (12-bit DAC)
- ESP32: Gunakan external DAC untuk quality"

---

## SLIDE 39: Perbandingan Platform Lengkap
**Prompt:**
"Buatkan slide perbandingan komprehensif:
- Judul: 'STM32 vs ESP32: DAC & PWM'
- Tabel besar:
```
| Feature | STM32F103 | ESP32 |
|---------|-----------|-------|
| DAC Resolution | 12-bit | 8-bit |
| DAC Channels | 2 | 2 |
| DAC DMA | Yes | Yes |
| Built-in Wave | Triangle/Noise | Cosine |
| PWM Channels | 32+ | 16 |
| PWM Resolution | 16-bit | 1-20 bit |
| Hardware Fade | No | Yes |
| Complementary | TIM1/8 | No |
| Max PWM Freq | MHz | 40MHz |
```
- Rekomendasi use case untuk masing-masing"

---

## SLIDE 40: Troubleshooting Guide
**Prompt:**
"Buatkan slide troubleshooting:
- Judul: 'Troubleshooting DAC & PWM'
- Tabel problem-solution:
  | Problem | Cause | Solution |
  |---------|-------|----------|
  | DAC stuck at 0V | Pin not analog | Check GPIO mode |
  | PWM no output | Timer not started | Call Start() |
  | Servo jitter | Interrupt timing | Hardware timer |
  | Motor noise | Low PWM freq | Increase to 20kHz |
  | LED flicker | Freq < 100Hz | Increase frequency |
  | DAC stepping | Low resolution | Use dithering |
- Flowchart debugging untuk setiap issue"

---

## SLIDE 41: Best Practices Checklist
**Prompt:**
"Buatkan slide best practices:
- Judul: 'Best Practices: DAC & PWM'
- Checklist dengan ikon:
  DAC:
  ☑ Enable output buffer untuk load handling
  ☑ Gunakan DMA untuk waveform kontinyu
  ☑ Synchronize dengan timer untuk timing presisi
  ☑ Pertimbangkan settling time
  
  PWM:
  ☑ Pilih frekuensi sesuai aplikasi
  ☑ Gunakan dead-time untuk H-bridge
  ☑ Implementasi soft-start untuk motor
  ☑ Hardware PWM lebih presisi dari software
  
- Warning box untuk common mistakes"

---

## SLIDE 42: Pengukuran dengan Oscilloscope
**Prompt:**
"Buatkan slide teknik pengukuran:
- Judul: 'Verifikasi dengan Oscilloscope'
- Setup pengukuran:
  - Probe attenuation: 10X
  - Timebase sesuai frekuensi
  - Trigger: Edge, rising
- Parameter yang diukur:
  - Frequency & Period
  - Duty Cycle
  - Rise/Fall time
  - Ripple (filtered PWM)
- Screenshot contoh waveform:
  - DAC ramp
  - PWM 50%
  - Servo signal
- Tip: Gunakan MATH function untuk FFT"

---

## SLIDE 43: Integrasi dengan Sensor
**Prompt:**
"Buatkan slide integrasi:
- Judul: 'DAC/PWM + Sensor Integration'
- Contoh aplikasi terintegrasi:
  1. Temperature → PWM → Fan Speed
  2. Light Sensor → PWM → LED Brightness
  3. Distance → DAC → Analog Meter
  4. Potentiometer → Servo Position
- Block diagram closed-loop system
- Kode contoh:
```cpp
void loop() {
    int temp = readTemperature();
    int fanSpeed = map(temp, 25, 50, 0, 255);
    ledcWrite(FAN_CHANNEL, fanSpeed);
}
```
- Concept: Sensor → Process → Actuator"

---

## SLIDE 44: Project Ideas
**Prompt:**
"Buatkan slide ide project:
- Judul: 'Project Ideas: DAC & PWM'
- Level Easy:
  - LED Breathing effect
  - Simple servo tester
  - PWM fan controller
- Level Medium:
  - RGB mood lamp
  - DC motor speed control
  - Audio tone generator
- Level Advanced:
  - Function generator
  - Robot arm controller
  - Audio player dengan DAC
- Setiap project dengan complexity rating ⭐"

---

## SLIDE 45: Tugas dan Deliverables
**Prompt:**
"Buatkan slide tugas:
- Judul: 'Tugas & Deliverables'
- Tugas Praktikum:
  1. ✏️ Implementasi semua program (40%)
  2. 📝 Laporan dengan analisis (30%)
  3. 🎥 Video demonstrasi 3-5 menit (20%)
  4. 💡 Modifikasi kreatif (10%)
- Format laporan:
  - Teori singkat
  - Prosedur
  - Hasil & Screenshot
  - Analisis
  - Kesimpulan
- Deadline dan submission method"

---

## SLIDE 46: Rubrik Penilaian
**Prompt:**
"Buatkan slide rubrik:
- Judul: 'Rubrik Penilaian'
- Tabel rubrik detail:
  | Kriteria | 4 (Excellent) | 3 (Good) | 2 (Fair) | 1 (Poor) |
  |----------|---------------|----------|----------|----------|
  | Program | Semua jalan | 80% jalan | 60% jalan | <60% |
  | Analisis | Mendalam | Cukup | Dangkal | Tidak ada |
  | Video | Profesional | Baik | Cukup | Kurang |
  | Kreativitas | Inovatif | Ada mod | Copy | Tidak |
- Bobot: Program 40%, Laporan 30%, Video 20%, Kreatif 10%"

---

## SLIDE 47: Pertanyaan Evaluasi
**Prompt:**
"Buatkan slide evaluasi:
- Judul: 'Pertanyaan Evaluasi'
- Soal pilihan:
  1. Resolusi DAC ESP32 adalah... (8-bit)
  2. Frekuensi PWM untuk motor sebaiknya... (>20kHz)
  3. Servo standard membutuhkan frekuensi... (50Hz)
  4. Untuk audio output, lebih baik... (DAC)
- Soal analisis:
  - Hitung tegangan output DAC 12-bit, input 2048, Vref 3.3V
  - Hitung duty cycle untuk pulse 1.5ms pada 50Hz PWM
- Space untuk jawaban singkat"

---

## SLIDE 48: Referensi Lengkap
**Prompt:**
"Buatkan slide referensi:
- Judul: 'Referensi & Resources'
- Dokumentasi Resmi:
  - STM32F103 Reference Manual (RM0008)
  - ESP32 Technical Reference Manual
  - ESP-IDF LEDC Documentation
- Application Notes:
  - AN3126: Audio and waveform generation
  - AN4013: STM32 Timer cookbook
- Books:
  - 'Mastering STM32' - Carmine Noviello
- Online:
  - GitHub repository course
  - Forum diskusi
- QR codes untuk quick access"

---

## SLIDE 49: Preview Modul Berikutnya
**Prompt:**
"Buatkan slide preview:
- Judul: 'Coming Up: I2C Sensor Communication'
- Teaser Modul 06:
  - Protokol I2C (Two-Wire Interface)
  - Sensor BME280 (Temperature, Humidity, Pressure)
  - OLED Display SSD1306
  - RTC DS3231
  - EEPROM 24LC256
- Keterkaitan:
  'DAC output dapat ditampilkan ke OLED via I2C'
  'Sensor reading dapat mengontrol PWM'
- Diagram integrasi modul"

---

## SLIDE 50: Terima Kasih
**Prompt:**
"Buatkan slide penutup:
- Judul: 'Terima Kasih'
- Quote inspiratif tentang embedded systems
- Summary icons:
  ✅ DAC untuk output analog presisi
  ✅ PWM untuk kontrol daya efisien
  ✅ Pilih sesuai kebutuhan aplikasi
- Contact information
- Social media / repository links
- Animasi sederhana untuk closing"

---

## 🎨 Panduan Desain Lanjutan

### Animasi yang Disarankan
1. **Waveform Animation**
   - PWM duty cycle berubah
   - Sine wave generation step-by-step
   - RC filter smoothing effect

2. **Build-up Animation**
   - Block diagram muncul satu-per-satu
   - Code highlighting line-by-line
   - Comparison table cell-by-cell

3. **Transition**
   - Slide ke slide: Fade/Push
   - Section change: Zoom
   - Demo section: Cut to video

### Assets yang Dibutuhkan
- [ ] Foto oscilloscope waveform
- [ ] Video demo LED dimming
- [ ] Video demo motor control
- [ ] Video demo servo sweep
- [ ] Diagram skematik H-bridge
- [ ] Foto setup praktikum

### Template Consistency
- Gunakan header yang sama di semua slide
- Footer: Nomor slide, nama modul
- Warna code block konsisten
- Icon set yang unified


-----------------------------------------------------------
--- Project.md ---
-----------------------------------------------------------

# Project Modul 05: DAC & PWM
## 🎵 Smart Audio-Visual Controller dengan Komunikasi Dual-Platform

### 📋 Informasi Project

| Item | Detail |
|------|--------|
| **Nama Project** | Smart Audio-Visual Controller |
| **Platform** | STM32F103C8T6 + ESP32 DevKit (Dual MCU) |
| **Tingkat Kesulitan** | ⭐⭐⭐⭐ (Advanced) |
| **Durasi Pengerjaan** | 2-3 minggu |
| **Kelompok** | 2-3 orang |

---

## 🎯 Deskripsi Project

Membangun sistem Smart Audio-Visual Controller yang menggunakan **dua mikrokontroler** (STM32 dan ESP32) yang berkomunikasi melalui UART. Sistem ini menggabungkan:

1. **STM32** sebagai **Audio Generator** menggunakan DAC 12-bit untuk menghasilkan berbagai waveform audio
2. **ESP32** sebagai **Visual Controller** menggunakan PWM untuk mengontrol LED RGB dan motor dengan remote control WiFi

Kedua MCU saling berkomunikasi untuk sinkronisasi audio-visual, dimana perubahan audio akan mempengaruhi efek visual secara real-time.

---

## 🎯 Tujuan Project

### Tujuan Utama
1. Mengimplementasikan DAC 12-bit STM32 untuk audio waveform generation
2. Mengimplementasikan PWM multi-channel ESP32 untuk visual effects
3. Membuat komunikasi antar MCU melalui UART
4. Mengintegrasikan kontrol WiFi pada ESP32

### Learning Outcomes
- Menguasai penggunaan DAC untuk aplikasi audio
- Menguasai PWM untuk kontrol LED dan motor
- Memahami komunikasi serial antar mikrokontroler
- Mengembangkan kemampuan sistem terintegrasi

---

## 📐 Arsitektur Sistem

```
┌─────────────────────────────────────────────────────────────────────┐
│                    SMART AUDIO-VISUAL CONTROLLER                     │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌─────────────────────┐         ┌─────────────────────────────┐    │
│  │     STM32F103       │  UART   │         ESP32               │    │
│  │   (Audio Master)    │◄───────►│    (Visual Controller)      │    │
│  │                     │         │                             │    │
│  │  ┌───────────────┐  │         │  ┌─────────────────────┐    │    │
│  │  │  Waveform     │  │         │  │    WiFi Module      │    │    │
│  │  │  Generator    │  │         │  │   (Web Interface)   │    │    │
│  │  │  - Sine       │  │         │  └─────────────────────┘    │    │
│  │  │  - Square     │  │         │            │                │    │
│  │  │  - Triangle   │  │         │  ┌─────────▼─────────┐      │    │
│  │  │  - Sawtooth   │  │         │  │   PWM Controller  │      │    │
│  │  └───────┬───────┘  │         │  │   - LED RGB       │      │    │
│  │          │          │         │  │   - Motor Speed   │      │    │
│  │  ┌───────▼───────┐  │         │  │   - Servo Angle   │      │    │
│  │  │   DAC 12-bit  │  │         │  └───────────────────┘      │    │
│  │  │   (PA4/PA5)   │  │         │                             │    │
│  │  └───────┬───────┘  │         │  ┌─────────────────────┐    │    │
│  │          │          │         │  │   Visual Output     │    │    │
│  │  ┌───────▼───────┐  │         │  │   - RGB LED Strip   │    │    │
│  │  │  Amplifier    │  │         │  │   - DC Motor        │    │    │
│  │  │  + Speaker    │  │         │  │   - Servo           │    │    │
│  │  └───────────────┘  │         │  └─────────────────────┘    │    │
│  └─────────────────────┘         └─────────────────────────────┘    │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                    Synchronization Protocol                  │    │
│  │  STM32 → ESP32: Audio Level, Frequency, Waveform Type       │    │
│  │  ESP32 → STM32: Control Commands, Mode Selection            │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Spesifikasi Hardware

### Komponen Utama

| No | Komponen | Qty | Fungsi |
|----|----------|-----|--------|
| 1 | STM32F103C8T6 | 1 | Audio waveform generator |
| 2 | ESP32 DevKit V1 | 1 | Visual controller + WiFi |
| 3 | ST-Link V2 | 1 | STM32 programmer |
| 4 | Speaker 8Ω 0.5W | 1 | Audio output |
| 5 | PAM8403 Amplifier | 1 | Audio amplifier |
| 6 | LED RGB WS2812B (8 pixel) | 1 | Visual effect |
| 7 | LED RGB Common Cathode | 3 | Direct PWM control |
| 8 | DC Motor 3-6V | 1 | Speed visual |
| 9 | Servo SG90 | 1 | Position visual |
| 10 | L298N Module | 1 | Motor driver |
| 11 | Rotary Encoder | 2 | Input control |
| 12 | Push Button | 4 | Mode selection |
| 13 | OLED 0.96" I2C | 1 | Display status |
| 14 | Potentiometer 10kΩ | 2 | Volume/parameter |

### Skema Koneksi

#### STM32 Connections
```
STM32F103C8T6
├── PA4 (DAC_OUT1) ──────► Amplifier Input
├── PA5 (DAC_OUT2) ──────► Optional second channel
├── PA9 (UART1_TX) ──────► ESP32 RX2 (GPIO16)
├── PA10 (UART1_RX) ◄───── ESP32 TX2 (GPIO17)
├── PB6 ◄─────────────────  Rotary Encoder A
├── PB7 ◄─────────────────  Rotary Encoder B
├── PB8 ◄─────────────────  Rotary Encoder Button
├── PA0 ◄─────────────────  Potentiometer (Frequency)
├── 3.3V ─────────────────► VCC components
└── GND ──────────────────► GND components
```

#### ESP32 Connections
```
ESP32 DevKit V1
├── GPIO25 ───────────────► LED Red (PWM Ch0)
├── GPIO26 ───────────────► LED Green (PWM Ch1)
├── GPIO27 ───────────────► LED Blue (PWM Ch2)
├── GPIO32 ───────────────► Motor PWM (via L298N ENA)
├── GPIO33 ───────────────► Motor DIR_A (L298N IN1)
├── GPIO14 ───────────────► Motor DIR_B (L298N IN2)
├── GPIO13 ───────────────► Servo Signal
├── GPIO16 (RX2) ◄─────── STM32 TX
├── GPIO17 (TX2) ────────► STM32 RX
├── GPIO21 (SDA) ─────────► OLED SDA
├── GPIO22 (SCL) ─────────► OLED SCL
├── GPIO4 ◄───────────────  Button Mode
├── GPIO5 ◄───────────────  Rotary Encoder A
├── GPIO18 ◄──────────────  Rotary Encoder B
├── 3.3V ─────────────────► VCC components
└── GND ──────────────────► GND components
```

---

## 📝 Spesifikasi Fungsional

### Mode Operasi

#### Mode 1: Manual Audio-Visual
- Kontrol langsung menggunakan rotary encoder dan button
- STM32: Adjust frequency (20Hz - 2kHz) dan waveform
- ESP32: LED mengikuti audio level dan frequency
- Motor kecepatan proporsional dengan volume
- Servo position mengikuti phase waveform

#### Mode 2: Preset Patterns
- 5 preset pattern audio-visual tersimpan
- LED pattern synchronized dengan audio
- Sequence pattern dengan timing
- Smooth transition antar preset

#### Mode 3: WiFi Remote Control
- Web interface untuk kontrol parameter
- Real-time update tanpa reload
- Responsive design (mobile-friendly)
- Status display di web

#### Mode 4: Music Reactive
- Audio input analysis (beat detection)
- Visual effect mengikuti beat
- Frequency spectrum pada LED strip
- Auto-sensitivity adjustment

### Protokol Komunikasi UART

```
Communication Protocol (115200 baud):

STM32 → ESP32 (Audio Data Packet):
┌────────┬──────┬──────────┬─────────┬──────────┬────────┐
│ START  │ TYPE │ LEVEL    │ FREQ    │ WAVEFORM │ CHKSUM │
│ 0xAA   │ 0x01 │ 0-255    │ 2 bytes │ 0-3      │ XOR    │
└────────┴──────┴──────────┴─────────┴──────────┴────────┘

ESP32 → STM32 (Command Packet):
┌────────┬──────┬──────────┬──────────┬────────┐
│ START  │ TYPE │ COMMAND  │ PARAM    │ CHKSUM │
│ 0x55   │ 0x02 │ 0-255    │ 2 bytes  │ XOR    │
└────────┴──────┴──────────┴──────────┴────────┘

Commands:
0x01 - Set Frequency
0x02 - Set Waveform
0x03 - Set Volume
0x04 - Play/Stop
0x05 - Load Preset
```

---

## 💻 Implementasi Software

### STM32 - Audio Generator (main.c)

```c
/**
 * STM32 Audio Generator
 * DAC-based waveform synthesis with DMA
 */

#include "stm32f1xx_hal.h"
#include <math.h>
#include <string.h>

// Waveform Configuration
#define SAMPLE_RATE     44100
#define SAMPLES_PER_CYCLE 256
#define PI              3.14159265359

// Waveform types
typedef enum {
    WAVE_SINE = 0,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SAWTOOTH
} WaveformType;

// Global variables
DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_dac1;

uint16_t waveform_buffer[SAMPLES_PER_CYCLE];
volatile uint16_t current_frequency = 440;  // Hz
volatile WaveformType current_waveform = WAVE_SINE;
volatile uint8_t volume = 128;
volatile uint8_t playing = 1;

// Function prototypes
void SystemClock_Config(void);
void DAC_Init(void);
void TIM6_Init(uint16_t frequency);
void UART_Init(void);
void GenerateWaveform(WaveformType type);
void SendAudioData(void);
void ProcessCommand(uint8_t* data);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    DAC_Init();
    UART_Init();
    
    GenerateWaveform(WAVE_SINE);
    TIM6_Init(current_frequency);
    
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, 
                      (uint32_t*)waveform_buffer, 
                      SAMPLES_PER_CYCLE, DAC_ALIGN_12B_R);
    HAL_TIM_Base_Start(&htim6);
    
    uint8_t rx_buffer[8];
    uint32_t last_send = 0;
    
    while (1) {
        // Receive commands from ESP32
        if (HAL_UART_Receive(&huart1, rx_buffer, 8, 10) == HAL_OK) {
            if (rx_buffer[0] == 0x55 && rx_buffer[1] == 0x02) {
                ProcessCommand(rx_buffer);
            }
        }
        
        // Send audio data every 50ms
        if (HAL_GetTick() - last_send >= 50) {
            SendAudioData();
            last_send = HAL_GetTick();
        }
    }
}

void GenerateWaveform(WaveformType type) {
    for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
        float t = (float)i / SAMPLES_PER_CYCLE;
        float value = 0;
        
        switch (type) {
            case WAVE_SINE:
                value = sin(2 * PI * t);
                break;
                
            case WAVE_SQUARE:
                value = (t < 0.5) ? 1.0 : -1.0;
                break;
                
            case WAVE_TRIANGLE:
                value = (t < 0.5) ? (4 * t - 1) : (3 - 4 * t);
                break;
                
            case WAVE_SAWTOOTH:
                value = 2 * t - 1;
                break;
        }
        
        // Scale by volume and convert to 12-bit (0-4095)
        uint16_t dac_value = (uint16_t)((value + 1.0) * 
                             (volume / 255.0) * 2000 + 48);
        waveform_buffer[i] = dac_value;
    }
}

void TIM6_Init(uint16_t frequency) {
    __HAL_RCC_TIM6_CLK_ENABLE();
    
    // Calculate prescaler for desired frequency
    // Timer trigger rate = SAMPLES_PER_CYCLE * frequency
    uint32_t timer_freq = SAMPLES_PER_CYCLE * frequency;
    uint16_t prescaler = (72000000 / timer_freq / 100) - 1;
    
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = prescaler;
    htim6.Init.Period = 100 - 1;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&htim6);
    
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);
}

void SendAudioData(void) {
    // Calculate current audio level (RMS approximation)
    uint32_t sum = 0;
    for (int i = 0; i < SAMPLES_PER_CYCLE; i++) {
        int16_t val = waveform_buffer[i] - 2048;
        sum += val * val;
    }
    uint8_t level = (uint8_t)(sqrt(sum / SAMPLES_PER_CYCLE) / 16);
    
    // Send packet to ESP32
    uint8_t packet[8];
    packet[0] = 0xAA;  // Start byte
    packet[1] = 0x01;  // Type: audio data
    packet[2] = level;
    packet[3] = (current_frequency >> 8) & 0xFF;
    packet[4] = current_frequency & 0xFF;
    packet[5] = current_waveform;
    packet[6] = 0;     // Reserved
    packet[7] = packet[1] ^ packet[2] ^ packet[3] ^ 
                packet[4] ^ packet[5];  // Checksum
    
    HAL_UART_Transmit(&huart1, packet, 8, 100);
}

void ProcessCommand(uint8_t* data) {
    uint8_t cmd = data[2];
    uint16_t param = (data[3] << 8) | data[4];
    
    switch (cmd) {
        case 0x01:  // Set Frequency
            current_frequency = param;
            TIM6_Init(current_frequency);
            break;
            
        case 0x02:  // Set Waveform
            current_waveform = (WaveformType)(param & 0x03);
            GenerateWaveform(current_waveform);
            break;
            
        case 0x03:  // Set Volume
            volume = param & 0xFF;
            GenerateWaveform(current_waveform);
            break;
            
        case 0x04:  // Play/Stop
            playing = param & 0x01;
            if (playing) {
                HAL_TIM_Base_Start(&htim6);
            } else {
                HAL_TIM_Base_Stop(&htim6);
            }
            break;
    }
}

// Additional init functions...
void DAC_Init(void) {
    // Full DAC initialization with DMA
    // (Same as jobsheet implementation)
}

void UART_Init(void) {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;  // TX
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_10;  // RX
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart1);
}

void SystemClock_Config(void) {
    // 72MHz configuration
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
```

### ESP32 - Visual Controller (main.cpp)

```cpp
/**
 * ESP32 Visual Controller
 * PWM-based LED and motor control with WiFi
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// Pin Definitions
#define LED_RED_PIN     25
#define LED_GREEN_PIN   26
#define LED_BLUE_PIN    27
#define MOTOR_PWM_PIN   32
#define MOTOR_DIR_A     33
#define MOTOR_DIR_B     14
#define SERVO_PIN       13
#define BTN_MODE_PIN    4

// PWM Channels
#define RED_CHANNEL     0
#define GREEN_CHANNEL   1
#define BLUE_CHANNEL    2
#define MOTOR_CHANNEL   3

// OLED
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Servo
Servo visualServo;

// WiFi Configuration
const char* ssid = "AudioVisual_Controller";
const char* password = "12345678";
WebServer server(80);

// Audio data from STM32
struct AudioData {
    uint8_t level;
    uint16_t frequency;
    uint8_t waveform;
    uint32_t lastUpdate;
} audioData;

// Visual settings
struct VisualSettings {
    uint8_t mode;           // 0=Manual, 1=Preset, 2=WiFi, 3=Reactive
    uint8_t brightness;
    uint8_t motorSpeed;
    uint8_t servoAngle;
    bool motorEnabled;
} settings;

// Function prototypes
void setupWiFi(void);
void setupWebServer(void);
void processSTM32Data(void);
void updateVisuals(void);
void updateDisplay(void);
void handleRoot(void);
void handleAPI(void);

// HTML Page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Audio-Visual Controller</title>
    <style>
        body { font-family: Arial; text-align: center; margin: 20px; background: #1a1a2e; color: white; }
        .container { max-width: 600px; margin: auto; }
        .card { background: #16213e; border-radius: 10px; padding: 20px; margin: 10px 0; }
        .slider { width: 100%; height: 30px; }
        .btn { background: #e94560; border: none; color: white; padding: 15px 30px; 
               border-radius: 5px; font-size: 16px; cursor: pointer; margin: 5px; }
        .btn:hover { background: #ff6b6b; }
        .status { padding: 10px; background: #0f3460; border-radius: 5px; }
        h1 { color: #e94560; }
        h3 { color: #00d9ff; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎵 Audio-Visual Controller</h1>
        
        <div class="card status">
            <h3>Status</h3>
            <p>Audio Level: <span id="level">--</span></p>
            <p>Frequency: <span id="freq">--</span> Hz</p>
            <p>Waveform: <span id="wave">--</span></p>
        </div>
        
        <div class="card">
            <h3>Audio Control</h3>
            <p>Frequency: <span id="freqVal">440</span> Hz</p>
            <input type="range" min="20" max="2000" value="440" 
                   class="slider" id="freqSlider" onchange="setFreq(this.value)">
            <br><br>
            <button class="btn" onclick="setWave(0)">Sine</button>
            <button class="btn" onclick="setWave(1)">Square</button>
            <button class="btn" onclick="setWave(2)">Triangle</button>
            <button class="btn" onclick="setWave(3)">Sawtooth</button>
        </div>
        
        <div class="card">
            <h3>Visual Control</h3>
            <p>Brightness: <span id="brightVal">100</span>%</p>
            <input type="range" min="0" max="100" value="100" 
                   class="slider" id="brightSlider" onchange="setBright(this.value)">
            <br><br>
            <p>Motor Speed: <span id="motorVal">0</span>%</p>
            <input type="range" min="0" max="100" value="0" 
                   class="slider" id="motorSlider" onchange="setMotor(this.value)">
        </div>
        
        <div class="card">
            <h3>Mode</h3>
            <button class="btn" onclick="setMode(0)">Manual</button>
            <button class="btn" onclick="setMode(1)">Preset</button>
            <button class="btn" onclick="setMode(2)">WiFi</button>
            <button class="btn" onclick="setMode(3)">Reactive</button>
        </div>
    </div>
    
    <script>
        function updateStatus() {
            fetch('/api?cmd=status')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('level').textContent = d.level;
                    document.getElementById('freq').textContent = d.frequency;
                    const waves = ['Sine', 'Square', 'Triangle', 'Sawtooth'];
                    document.getElementById('wave').textContent = waves[d.waveform];
                });
        }
        function setFreq(v) { 
            document.getElementById('freqVal').textContent = v;
            fetch('/api?cmd=freq&val=' + v); 
        }
        function setWave(v) { fetch('/api?cmd=wave&val=' + v); }
        function setBright(v) { 
            document.getElementById('brightVal').textContent = v;
            fetch('/api?cmd=bright&val=' + v); 
        }
        function setMotor(v) { 
            document.getElementById('motorVal').textContent = v;
            fetch('/api?cmd=motor&val=' + v); 
        }
        function setMode(v) { fetch('/api?cmd=mode&val=' + v); }
        setInterval(updateStatus, 500);
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17);  // RX2, TX2
    
    // Initialize PWM
    ledcSetup(RED_CHANNEL, 5000, 8);
    ledcSetup(GREEN_CHANNEL, 5000, 8);
    ledcSetup(BLUE_CHANNEL, 5000, 8);
    ledcSetup(MOTOR_CHANNEL, 20000, 10);
    
    ledcAttachPin(LED_RED_PIN, RED_CHANNEL);
    ledcAttachPin(LED_GREEN_PIN, GREEN_CHANNEL);
    ledcAttachPin(LED_BLUE_PIN, BLUE_CHANNEL);
    ledcAttachPin(MOTOR_PWM_PIN, MOTOR_CHANNEL);
    
    // Motor direction pins
    pinMode(MOTOR_DIR_A, OUTPUT);
    pinMode(MOTOR_DIR_B, OUTPUT);
    pinMode(BTN_MODE_PIN, INPUT_PULLUP);
    
    // Servo
    visualServo.attach(SERVO_PIN, 500, 2500);
    visualServo.write(90);
    
    // OLED
    Wire.begin();
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("Audio-Visual");
        display.println("Controller");
        display.display();
    }
    
    // WiFi
    setupWiFi();
    setupWebServer();
    
    // Initialize settings
    settings.mode = 0;
    settings.brightness = 100;
    settings.motorSpeed = 0;
    settings.servoAngle = 90;
    settings.motorEnabled = false;
    
    Serial.println("ESP32 Visual Controller Ready!");
}

void loop() {
    server.handleClient();
    processSTM32Data();
    updateVisuals();
    updateDisplay();
    
    // Mode button check
    static uint32_t lastBtnCheck = 0;
    if (millis() - lastBtnCheck > 200) {
        if (digitalRead(BTN_MODE_PIN) == LOW) {
            settings.mode = (settings.mode + 1) % 4;
        }
        lastBtnCheck = millis();
    }
    
    delay(10);
}

void setupWiFi() {
    WiFi.softAP(ssid, password);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/api", handleAPI);
    server.begin();
}

void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void handleAPI() {
    String cmd = server.arg("cmd");
    String val = server.arg("val");
    
    if (cmd == "status") {
        String json = "{\"level\":" + String(audioData.level) +
                     ",\"frequency\":" + String(audioData.frequency) +
                     ",\"waveform\":" + String(audioData.waveform) + "}";
        server.send(200, "application/json", json);
        return;
    }
    
    if (cmd == "freq") {
        sendCommandToSTM32(0x01, val.toInt());
    } else if (cmd == "wave") {
        sendCommandToSTM32(0x02, val.toInt());
    } else if (cmd == "bright") {
        settings.brightness = val.toInt();
    } else if (cmd == "motor") {
        settings.motorSpeed = val.toInt();
    } else if (cmd == "mode") {
        settings.mode = val.toInt();
    }
    
    server.send(200, "text/plain", "OK");
}

void processSTM32Data() {
    if (Serial2.available() >= 8) {
        uint8_t buffer[8];
        Serial2.readBytes(buffer, 8);
        
        if (buffer[0] == 0xAA && buffer[1] == 0x01) {
            // Verify checksum
            uint8_t checksum = buffer[1] ^ buffer[2] ^ buffer[3] ^ 
                              buffer[4] ^ buffer[5];
            if (checksum == buffer[7]) {
                audioData.level = buffer[2];
                audioData.frequency = (buffer[3] << 8) | buffer[4];
                audioData.waveform = buffer[5];
                audioData.lastUpdate = millis();
            }
        }
    }
}

void sendCommandToSTM32(uint8_t cmd, uint16_t param) {
    uint8_t packet[8];
    packet[0] = 0x55;
    packet[1] = 0x02;
    packet[2] = cmd;
    packet[3] = (param >> 8) & 0xFF;
    packet[4] = param & 0xFF;
    packet[5] = 0;
    packet[6] = 0;
    packet[7] = packet[1] ^ packet[2] ^ packet[3] ^ packet[4];
    
    Serial2.write(packet, 8);
}

void updateVisuals() {
    uint8_t r, g, b;
    
    switch (settings.mode) {
        case 0:  // Manual mode
            r = g = b = (settings.brightness * 255) / 100;
            break;
            
        case 1:  // Preset patterns
            // Rotating colors based on time
            {
                int hue = (millis() / 20) % 360;
                hsvToRgb(hue, 255, settings.brightness * 255 / 100, r, g, b);
            }
            break;
            
        case 2:  // WiFi controlled - use settings
            r = g = b = (settings.brightness * 255) / 100;
            break;
            
        case 3:  // Reactive mode
            // Color based on frequency
            {
                int hue = map(audioData.frequency, 20, 2000, 0, 360);
                int brightness = map(audioData.level, 0, 255, 0, 
                                    settings.brightness * 255 / 100);
                hsvToRgb(hue, 255, brightness, r, g, b);
            }
            break;
    }
    
    // Apply to LEDs
    ledcWrite(RED_CHANNEL, r);
    ledcWrite(GREEN_CHANNEL, g);
    ledcWrite(BLUE_CHANNEL, b);
    
    // Motor control
    if (settings.mode == 3) {
        // Reactive: motor speed follows audio level
        int speed = map(audioData.level, 0, 255, 0, 1023);
        ledcWrite(MOTOR_CHANNEL, speed);
    } else {
        ledcWrite(MOTOR_CHANNEL, (settings.motorSpeed * 1023) / 100);
    }
    
    digitalWrite(MOTOR_DIR_A, HIGH);
    digitalWrite(MOTOR_DIR_B, LOW);
    
    // Servo follows waveform phase in reactive mode
    if (settings.mode == 3) {
        int angle = map(audioData.level, 0, 255, 0, 180);
        visualServo.write(angle);
    } else {
        visualServo.write(settings.servoAngle);
    }
}

void hsvToRgb(int h, int s, int v, uint8_t& r, uint8_t& g, uint8_t& b) {
    int i = h / 60;
    int f = h % 60;
    int p = (v * (255 - s)) / 255;
    int q = (v * (255 - (s * f) / 60)) / 255;
    int t = (v * (255 - (s * (60 - f)) / 60)) / 255;
    
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
}

void updateDisplay() {
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate < 100) return;
    lastUpdate = millis();
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Mode: %d\n", settings.mode);
    display.printf("Freq: %d Hz\n", audioData.frequency);
    display.printf("Level: %d\n", audioData.level);
    display.printf("Wave: %d\n", audioData.waveform);
    display.printf("Motor: %d%%\n", settings.motorSpeed);
    display.printf("IP: %s", WiFi.softAPIP().toString().c_str());
    display.display();
}
```

---

## 📊 Deliverables

### 1. Source Code
- [ ] STM32 project lengkap (main.c + header files)
- [ ] ESP32 project lengkap (main.cpp + libraries)
- [ ] Konfigurasi PlatformIO (platformio.ini)
- [ ] Dokumentasi kode (comments)

### 2. Hardware
- [ ] Skema rangkaian (Fritzing/KiCad)
- [ ] PCB layout (opsional, bonus)
- [ ] Foto prototype
- [ ] Daftar komponen (BOM)

### 3. Dokumentasi
- [ ] Laporan teknis (10-15 halaman)
- [ ] User manual
- [ ] Diagram blok sistem
- [ ] Flowchart program

### 4. Video Demonstrasi
- [ ] Video demo (5-7 menit)
- [ ] Penjelasan fitur
- [ ] Troubleshooting yang dihadapi
- [ ] Future improvements

---

## 📅 Timeline Pengerjaan

| Minggu | Task | Deliverable |
|--------|------|-------------|
| 1 | Persiapan & Design | Skema, flowchart |
| 1 | STM32 DAC Implementation | Audio generator working |
| 2 | ESP32 PWM Implementation | Visual controller working |
| 2 | UART Communication | Protokol terintegrasi |
| 3 | WiFi & Web Interface | Remote control working |
| 3 | Testing & Documentation | Laporan, video |

---

## 🏆 Kriteria Penilaian

| Komponen | Bobot | Detail |
|----------|-------|--------|
| Fungsionalitas | 35% | Semua fitur bekerja |
| Kode & Dokumentasi | 25% | Clean code, comments, laporan |
| Integrasi Dual-MCU | 20% | Komunikasi UART reliable |
| Kreativitas | 10% | Fitur tambahan, UI/UX |
| Presentasi | 10% | Video demo, penjelasan |

---

## 💡 Tips Pengerjaan

1. **Mulai dari yang simple** - Test DAC dan PWM terpisah dulu
2. **Test komunikasi UART** - Pastikan protokol benar sebelum integrasi
3. **Gunakan Serial Monitor** - Debug dengan print statements
4. **Incremental development** - Tambah fitur satu per satu
5. **Version control** - Gunakan Git untuk tracking changes

---

## 🔗 Resources

- [STM32 DAC Application Note](https://www.st.com/resource/en/application_note/an3126.pdf)
- [ESP32 LEDC Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html)
- [ESP32 WebServer Example](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer)
- [Audio Synthesis Fundamentals](https://www.soundonsound.com/techniques/synth-secrets)


-----------------------------------------------------------
--- Referensi.md ---
-----------------------------------------------------------

# Referensi Lengkap
## Modul 05: DAC (Digital-to-Analog Converter) & PWM (Pulse Width Modulation)

---

## 📚 Dokumentasi Resmi

### STM32
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **RM0008** | STM32F103 Reference Manual | [ST.com](https://www.st.com/resource/en/reference_manual/rm0008.pdf) |
| **AN3126** | Audio and Waveform Generation using DAC | [ST.com](https://www.st.com/resource/en/application_note/an3126.pdf) |
| **AN4013** | STM32 Timer Cookbook | [ST.com](https://www.st.com/resource/en/application_note/an4013.pdf) |
| **UM1785** | STM32Cube HAL and LL Drivers | [ST.com](https://www.st.com/resource/en/user_manual/um1785.pdf) |
| **DS5319** | STM32F103x8/B Datasheet | [ST.com](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf) |

### ESP32
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **Technical Reference** | ESP32 Technical Reference Manual | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) |
| **LEDC API Guide** | LED Control (PWM) API Reference | [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html) |
| **DAC API Guide** | Digital-to-Analog Converter API | [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html) |
| **Arduino-ESP32 Docs** | Arduino Core for ESP32 | [GitHub](https://docs.espressif.com/projects/arduino-esp32/) |

---

## 📖 Buku Referensi

### Embedded Systems
| Judul | Penulis | Chapter Relevan |
|-------|---------|-----------------|
| **Mastering STM32** | Carmine Noviello | Chapter 11: DAC, Chapter 10: Timers |
| **The Definitive Guide to ARM Cortex-M3** | Joseph Yiu | Chapter 8-9: Peripherals |
| **Programming with STM32** | Donald Norris | Chapter 7: PWM & DAC |
| **ESP32 Technical Tutorials** | Neil Cameron | Chapter on LEDC & DAC |
| **Make: AVR Programming** | Elliot Williams | PWM concepts (applicable) |

### Signal Processing & Electronics
| Judul | Penulis | Topik |
|-------|---------|-------|
| **The Art of Electronics** | Horowitz & Hill | DAC/ADC Fundamentals |
| **Digital Signal Processing** | Proakis & Manolakis | Sampling, Quantization |
| **Practical Electronics for Inventors** | Scherz & Monk | PWM Circuits |

---

## 🎓 Tutorial & Online Courses

### Video Tutorials
| Platform | Judul | Link |
|----------|-------|------|
| **YouTube - Controllers Tech** | STM32 DAC Tutorial | [Link](https://www.youtube.com/watch?v=DAC_STM32) |
| **YouTube - Phil's Lab** | STM32 PWM Generation | [Link](https://www.youtube.com/c/PhilsLab) |
| **YouTube - DroneBot Workshop** | ESP32 PWM Tutorial | [Link](https://www.youtube.com/c/Dronebotworkshop) |
| **YouTube - Random Nerd Tutorials** | ESP32 LEDC PWM | [Link](https://www.youtube.com/c/RandomNerdTutorials) |
| **YouTube - Andreas Spiess** | ESP32 DAC for Audio | [Link](https://www.youtube.com/c/AndreasSpiess) |

### Written Tutorials
| Website | Topik | Link |
|---------|-------|------|
| **DeepBlue Embedded** | STM32 DAC HAL Tutorial | [Link](https://deepbluembedded.com/stm32-dac/) |
| **Random Nerd Tutorials** | ESP32 PWM Guide | [Link](https://randomnerdtutorials.com/esp32-pwm-arduino-ide/) |
| **ControllersTech** | STM32 PWM Tutorial | [Link](https://controllerstech.com/pwm-in-stm32/) |
| **Last Minute Engineers** | Servo Motor Control | [Link](https://lastminuteengineers.com/servo-motor-arduino-tutorial/) |
| **Circuit Digest** | DAC Tutorial | [Link](https://circuitdigest.com/article/digital-to-analog-converter-dac) |

### Online Courses
| Platform | Course | Deskripsi |
|----------|--------|-----------|
| **Udemy** | Mastering Microcontroller with Embedded Driver Development | Comprehensive STM32 |
| **Coursera** | Introduction to Embedded Systems | ARM Basics |
| **edX** | Embedded Systems - Shape The World | TI LaunchPad (concepts applicable) |

---

## 📄 Application Notes & White Papers

### STM32 Application Notes
| Code | Title | Focus |
|------|-------|-------|
| **AN3126** | Audio and waveform generation using DAC | Audio synthesis with DAC |
| **AN4013** | STM32 cross-series timer overview | Timer configurations |
| **AN4277** | Using STM32 Discovery kit as audio recorder | Audio applications |
| **AN3265** | LED lamp control with STM32 | PWM LED control |
| **AN2592** | How to achieve the best ADC accuracy | DAC/ADC calibration |

### Application Notes Umum
| Publisher | Title | Topic |
|-----------|-------|-------|
| **Texas Instruments** | SLAA533 | LED Lighting with PWM |
| **Analog Devices** | MT-013 | Evaluating DAC Accuracy |
| **Microchip** | AN539 | Using PWM to Generate Analog Output |
| **NXP** | AN3174 | PWM Motor Control |

---

## 🔧 Libraries & Code Examples

### STM32
```
GitHub Repositories:
├── STM32CubeF1 (Official HAL/LL Drivers)
│   └── https://github.com/STMicroelectronics/STM32CubeF1
├── stm32-dac-examples
│   └── https://github.com/topics/stm32-dac
└── stm32-pwm-examples
    └── https://github.com/topics/stm32-pwm
```

### ESP32
```
GitHub Repositories:
├── arduino-esp32 (Official Arduino Core)
│   └── https://github.com/espressif/arduino-esp32
├── esp-idf (Official ESP-IDF Framework)
│   └── https://github.com/espressif/esp-idf
├── ESP32Servo (Servo Library)
│   └── https://github.com/madhephaestus/ESP32Servo
└── ESP32-audioI2S (Audio with DAC)
    └── https://github.com/schreibfaul1/ESP32-audioI2S
```

---

## 🧪 Tools & Software

### Development
| Tool | Purpose | Platform |
|------|---------|----------|
| **STM32CubeIDE** | STM32 Development | Windows/Linux/Mac |
| **STM32CubeMX** | Configuration Generator | Windows/Linux/Mac |
| **PlatformIO** | Multi-platform IDE | VS Code Extension |
| **Arduino IDE** | ESP32/Arduino Development | All platforms |
| **ESP-IDF** | ESP32 Native Framework | All platforms |

### Debugging & Analysis
| Tool | Purpose | Link |
|------|---------|------|
| **PulseView** | Logic Analyzer Software | [Link](https://sigrok.org/wiki/PulseView) |
| **Audacity** | Audio Analysis | [Link](https://www.audacityteam.org/) |
| **Oscilloscope Apps** | Mobile Oscilloscope | Various |
| **Serial Studio** | Serial Data Visualization | [Link](https://serial-studio.github.io/) |

### Simulation
| Tool | Purpose | Link |
|------|---------|------|
| **Wokwi** | ESP32 Online Simulator | [Link](https://wokwi.com/) |
| **Proteus** | STM32 Simulation | Commercial |
| **LTspice** | Circuit Simulation | [Link](https://www.analog.com/ltspice) |
| **Falstad Circuit Simulator** | Online Circuit Sim | [Link](https://www.falstad.com/circuit/) |

---

## 📊 Datasheet Komponen Pendukung

### Audio
| Komponen | Tipe | Datasheet |
|----------|------|-----------|
| **PAM8403** | Audio Amplifier | [Link](https://www.diodes.com/assets/Datasheets/PAM8403.pdf) |
| **LM386** | Audio Amplifier | [Link](https://www.ti.com/lit/ds/symlink/lm386.pdf) |
| **MAX98357A** | I2S Audio DAC | [Link](https://datasheets.maximintegrated.com/en/ds/MAX98357A.pdf) |

### Motor Control
| Komponen | Tipe | Datasheet |
|----------|------|-----------|
| **L298N** | Dual H-Bridge | [Link](https://www.st.com/resource/en/datasheet/l298.pdf) |
| **L293D** | Quad Half H-Bridge | [Link](https://www.ti.com/lit/ds/symlink/l293.pdf) |
| **TB6612FNG** | Motor Driver | [Link](https://toshiba.semicon-storage.com/info/docget.jsp?did=10660&prodName=TB6612FNG) |

### Servo
| Komponen | Spesifikasi | Datasheet |
|----------|-------------|-----------|
| **SG90** | Micro Servo 180° | [Link](http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf) |
| **MG996R** | High Torque Servo | Various sources |
| **DS3218** | High Voltage Servo | Various sources |

---

## 🌐 Forum & Komunitas

### Forums
| Platform | Community | Link |
|----------|-----------|------|
| **ST Community** | STM32 Official Forum | [Link](https://community.st.com/) |
| **ESP32 Forum** | Espressif Forum | [Link](https://esp32.com/) |
| **Arduino Forum** | Arduino Community | [Link](https://forum.arduino.cc/) |
| **EEVBlog Forum** | Electronics Forum | [Link](https://www.eevblog.com/forum/) |
| **Stack Overflow** | Programming Q&A | Tag: stm32, esp32 |

### Reddit Communities
| Subreddit | Focus |
|-----------|-------|
| r/embedded | Embedded Systems |
| r/stm32 | STM32 Specific |
| r/esp32 | ESP32 Specific |
| r/AskElectronics | Electronics Help |

### Discord Servers
| Server | Focus |
|--------|-------|
| **ESP32 Developers** | ESP32 Community |
| **Embedded Systems** | General Embedded |
| **Electronics** | Electronics Hobbyists |

---

## 📱 Mobile Apps (Tools)

| App | Platform | Purpose |
|-----|----------|---------|
| **Oscilloscope Pro** | Android | Audio oscilloscope |
| **Function Generator** | Android/iOS | Signal generation |
| **Electronics Toolkit** | Android/iOS | Calculator & references |
| **Fritzing** | Desktop | Schematic drawing |

---

## 📝 Catatan Penggunaan Referensi

### Prioritas Referensi
1. **Dokumentasi Resmi** - Selalu cek datasheet dan reference manual
2. **Application Notes** - Untuk implementasi praktis
3. **Buku Teks** - Untuk pemahaman konsep mendalam
4. **Tutorial Online** - Untuk quick start dan troubleshooting

### Tips Mencari Referensi
```
Search Keywords:
├── STM32: "STM32F103 DAC HAL", "STM32 PWM Timer tutorial"
├── ESP32: "ESP32 LEDC PWM", "ESP32 DAC waveform"
├── Konsep: "R-2R ladder DAC", "PWM duty cycle calculation"
└── Troubleshooting: "DAC output stuck", "PWM frequency calculation"
```

### Verifikasi Informasi
- Cross-check dengan datasheet resmi
- Test implementasi pada hardware
- Perhatikan versi software/library
- Cek tanggal publikasi (terutama untuk tutorial)

---

## 📅 Update Log

| Tanggal | Update |
|---------|--------|
| 2024-01 | Initial reference compilation |
| 2024-02 | Added ESP32 LEDC documentation |
| 2024-03 | Added audio-related references |

---

**Catatan:** Selalu cek versi terbaru dari dokumentasi resmi karena API dan fitur dapat berubah.


-----------------------------------------------------------
--- Rubrik_Penilaian_Project.md ---
-----------------------------------------------------------

# Rubrik Penilaian Project
## Modul 05: DAC & PWM - Smart Audio-Visual Controller

### 📋 Informasi Penilaian

| Item | Detail |
|------|--------|
| **Nama Project** | Smart Audio-Visual Controller |
| **Platform** | STM32F103 + ESP32 (Dual MCU) |
| **Bobot dalam Nilai Akhir** | 30% |
| **Penilai** | Dosen Pengampu + Asisten Lab |

---

## 🎯 Komponen Penilaian

### A. Fungsionalitas Sistem (35%)

#### A1. DAC Audio Generator - STM32 (15%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Waveform Generation** | Semua 4 waveform (sine, square, triangle, sawtooth) bekerja sempurna | 3 waveform bekerja | 2 waveform bekerja | 1 atau tidak ada | |
| **Frequency Control** | Range 20Hz-2kHz, smooth transition | Range terbatas tapi berfungsi | Frequency tetap | Tidak berfungsi | |
| **Volume Control** | Level 0-100% dengan resolusi baik | Volume control ada, resolusi terbatas | Volume hanya on/off | Tidak ada kontrol | |
| **DMA Operation** | DAC dengan DMA, CPU-free | DAC dengan polling tapi smooth | DAC dengan delay | Tidak ada output | |
| **Audio Quality** | Output bersih, minimal noise | Sedikit noise | Noise cukup banyak | Sangat noisy/distorsi | |

**Subtotal A1:** ___/20 × 0.75 = ___/15

#### A2. PWM Visual Controller - ESP32 (12%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **LED RGB Control** | Full RGB dengan smooth color transition | RGB bekerja, transisi kurang smooth | Hanya single color | Tidak berfungsi | |
| **Motor Control** | Speed 0-100% dengan soft-start | Speed control ada | Motor hanya on/off | Tidak berfungsi | |
| **Servo Control** | 0-180° dengan posisi akurat | Servo bergerak tapi kurang akurat | Servo terbatas | Tidak berfungsi | |
| **Hardware Fade** | Menggunakan LEDC hardware fade | Software fade smooth | Software fade jerky | Tidak ada fade | |

**Subtotal A2:** ___/16 × 0.75 = ___/12

#### A3. Komunikasi & Integrasi (8%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **UART Protocol** | Protokol custom dengan checksum, reliable | Protokol sederhana, reliable | Komunikasi kadang error | Tidak bisa berkomunikasi | |
| **Real-time Sync** | Visual sync dengan audio < 50ms latency | Sync dengan latency noticeable | Sync tapi tidak real-time | Tidak sync | |

**Subtotal A3:** ___/8

---

### B. Kode & Dokumentasi (25%)

#### B1. Kualitas Kode (15%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Code Structure** | Modular, fungsi terpisah jelas, header files | Struktur baik, kurang modular | Semua dalam satu file | Kode berantakan | |
| **Naming Convention** | Konsisten, deskriptif, mengikuti standard | Cukup konsisten | Inkonsisten | Tidak deskriptif | |
| **Comments** | Setiap fungsi ada doxygen comments | Comment cukup lengkap | Comment minimal | Tidak ada comment | |
| **Error Handling** | Semua error ditangani, graceful degradation | Error handling ada | Minimal error handling | Tidak ada | |
| **Efficiency** | Optimal, tidak ada blocking yang tidak perlu | Cukup efisien | Ada blocking | Banyak busy-wait | |

**Subtotal B1:** ___/20 × 0.75 = ___/15

#### B2. Dokumentasi (10%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Laporan Teknis** | Lengkap 10-15 halaman, format profesional | Lengkap, format kurang rapi | Kurang lengkap | Tidak ada/sangat minim | |
| **Diagram & Skema** | Skema rangkaian, block diagram, flowchart semua ada | 2 dari 3 ada | 1 ada | Tidak ada | |
| **User Manual** | Manual penggunaan lengkap | Manual singkat | Hanya readme | Tidak ada | |

**Subtotal B2:** ___/12 × 0.83 = ___/10

---

### C. Integrasi Dual-MCU (20%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Hardware Integration** | Wiring rapi, koneksi reliable | Wiring fungsional | Ada loose connection | Tidak terhubung dengan benar | |
| **Protocol Design** | Protokol well-designed, extensible | Protokol berfungsi | Protokol minimal | Tidak ada protokol | |
| **Bidirectional Comm** | Data dan command dua arah bekerja | Satu arah bekerja sempurna | Satu arah kadang error | Tidak berkomunikasi | |
| **Error Recovery** | Auto-reconnect, data validation | Basic error detection | Tidak ada recovery | Crash saat error | |
| **Timing Synchronization** | Perfect sync, no visible delay | Slight delay acceptable | Noticeable delay | Not synchronized | |

**Subtotal C:** ___/20

---

### D. Kreativitas & Inovasi (10%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Fitur Tambahan** | 3+ fitur tambahan bermakna | 2 fitur tambahan | 1 fitur tambahan | Sesuai spesifikasi saja | |
| **UI/UX Web Interface** | Desain menarik, responsive, intuitif | Desain baik, kurang polish | Desain basic | Tidak ada web interface | |
| **Problem Solving** | Solusi kreatif untuk challenge | Solusi standard | Solusi copy-paste | Tidak ada solusi | |

**Subtotal D:** ___/12 × 0.83 = ___/10

---

### E. Presentasi & Demo (10%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Video Quality** | HD, audio jelas, editing profesional | Kualitas baik | Kualitas cukup | Kualitas buruk | |
| **Demonstration** | Semua fitur didemonstrasikan jelas | Mayoritas fitur didemonstrasikan | Beberapa fitur saja | Demo minimal | |
| **Explanation** | Penjelasan mendalam, menjawab pertanyaan | Penjelasan cukup | Penjelasan surface level | Tidak bisa menjelaskan | |

**Subtotal E:** ___/12 × 0.83 = ___/10

---

## 📊 Rekapitulasi Nilai

| Komponen | Bobot | Nilai | Weighted |
|----------|-------|-------|----------|
| A. Fungsionalitas | 35% | ___/35 | ___ |
| B. Kode & Dokumentasi | 25% | ___/25 | ___ |
| C. Integrasi Dual-MCU | 20% | ___/20 | ___ |
| D. Kreativitas | 10% | ___/10 | ___ |
| E. Presentasi | 10% | ___/10 | ___ |
| **TOTAL** | **100%** | | **___/100** |

---

## 📝 Konversi Nilai

| Range Nilai | Grade | Predikat |
|-------------|-------|----------|
| 85 - 100 | A | Excellent |
| 80 - 84 | A- | Very Good |
| 75 - 79 | B+ | Good |
| 70 - 74 | B | Above Average |
| 65 - 69 | B- | Average |
| 60 - 64 | C+ | Below Average |
| 55 - 59 | C | Fair |
| 50 - 54 | D | Poor |
| < 50 | E | Failed |

---

## 🏆 Bonus Points (Max +10)

| Kriteria Bonus | Points |
|----------------|--------|
| Implementasi beat detection | +3 |
| Multi-zone LED control | +2 |
| Mobile app (Android/iOS) | +5 |
| PCB custom design | +3 |
| 3D printed enclosure | +2 |
| Fitur preset save/load ke EEPROM | +2 |
| Spectrum analyzer display | +3 |

**Total Bonus:** ___/10

---

## ⚠️ Penalty Points

| Pelanggaran | Penalty |
|-------------|---------|
| Terlambat submit (per hari) | -5 |
| Plagiarisme kode (>50% similarity) | -50 atau 0 |
| Tidak ada source code | -30 |
| Tidak ada video demo | -15 |
| Hardware tidak berfungsi saat demo | -20 |

---

## 📋 Catatan Penilai

### Kelebihan:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

### Kekurangan:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

### Saran Improvement:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## ✅ Checklist Kelengkapan Submission

| Item | Ada | Tidak |
|------|-----|-------|
| Source code STM32 | ☐ | ☐ |
| Source code ESP32 | ☐ | ☐ |
| platformio.ini | ☐ | ☐ |
| Skema rangkaian | ☐ | ☐ |
| Laporan PDF | ☐ | ☐ |
| Video demo | ☐ | ☐ |
| README.md | ☐ | ☐ |

---

**Tanggal Penilaian:** ________________

**Penilai:** ________________________

**Tanda Tangan:** ___________________


-----------------------------------------------------------
--- Rubrik_Penilaian_Tugas_Video.md ---
-----------------------------------------------------------

# Rubrik Penilaian Tugas Video
## Modul 05: DAC & PWM Output

### 📋 Informasi Tugas

| Item | Detail |
|------|--------|
| **Nama Tugas** | Video Dokumentasi Praktikum DAC & PWM |
| **Durasi Video** | 5-8 menit |
| **Format** | MP4, minimum 720p |
| **Platform Upload** | YouTube (Unlisted) / Google Drive |
| **Bobot dalam Nilai Akhir** | 20% |

---

## 🎯 Tujuan Tugas Video

1. Mendokumentasikan proses dan hasil praktikum
2. Menjelaskan pemahaman konsep DAC dan PWM
3. Mendemonstrasikan implementasi pada STM32 dan ESP32
4. Menganalisis perbedaan dan kegunaan masing-masing metode

---

## 📝 Struktur Video yang Diharapkan

### Bagian 1: Pembukaan (30-45 detik)
- Perkenalan (nama, NIM, kelompok)
- Judul praktikum
- Overview singkat apa yang akan didemonstrasikan

### Bagian 2: Penjelasan Teori (1-2 menit)
- Konsep DAC (Digital-to-Analog Converter)
- Konsep PWM (Pulse Width Modulation)
- Perbedaan fundamental keduanya

### Bagian 3: Demo STM32 (1.5-2 menit)
- Setup hardware
- DAC output demonstration
- PWM output demonstration
- Penjelasan kode singkat

### Bagian 4: Demo ESP32 (1.5-2 menit)
- Setup hardware
- DAC output demonstration
- PWM dengan LEDC demonstration
- Penjelasan kode singkat

### Bagian 5: Analisis & Perbandingan (1-1.5 menit)
- Perbandingan output oscilloscope (jika ada)
- Kapan menggunakan DAC vs PWM
- Aplikasi praktis

### Bagian 6: Penutup (30-45 detik)
- Kesimpulan pembelajaran
- Challenges yang dihadapi
- Credits

---

## 🎯 Rubrik Penilaian Detail

### A. Konten & Substansi (40%)

#### A1. Penjelasan Teori (15%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Pemahaman DAC** | Menjelaskan prinsip kerja, resolusi, formula dengan benar | Penjelasan cukup, minor error | Penjelasan dasar saja | Tidak menjelaskan/salah | |
| **Pemahaman PWM** | Menjelaskan duty cycle, frequency, aplikasi dengan benar | Penjelasan cukup | Penjelasan dasar | Tidak menjelaskan/salah | |
| **Perbandingan** | Membandingkan DAC vs PWM dengan insight mendalam | Perbandingan cukup | Perbandingan surface | Tidak ada perbandingan | |

**Subtotal A1:** ___/12 × 1.25 = ___/15

#### A2. Demonstrasi Praktikum (15%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Demo STM32** | Semua program DAC & PWM didemonstrasikan dengan jelas | Mayoritas program didemonstrasikan | Beberapa program saja | Demo minimal/tidak ada | |
| **Demo ESP32** | Semua program DAC & PWM didemonstrasikan dengan jelas | Mayoritas program didemonstrasikan | Beberapa program saja | Demo minimal/tidak ada | |
| **Penjelasan Kode** | Menjelaskan bagian kunci kode dengan jelas | Penjelasan kode cukup | Hanya menunjukkan kode | Tidak ada penjelasan kode | |

**Subtotal A2:** ___/12 × 1.25 = ___/15

#### A3. Analisis & Kesimpulan (10%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Analisis Hasil** | Menganalisis dengan data/measurement | Analisis kualitatif baik | Analisis dangkal | Tidak ada analisis | |
| **Kesimpulan** | Kesimpulan komprehensif, learning takeaways jelas | Kesimpulan cukup | Kesimpulan singkat | Tidak ada kesimpulan | |

**Subtotal A3:** ___/8 × 1.25 = ___/10

---

### B. Kualitas Teknis Video (30%)

#### B1. Kualitas Visual (15%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Resolusi Video** | 1080p atau lebih tinggi | 720p | 480p | Di bawah 480p | |
| **Pencahayaan** | Terang, jelas, semua detail terlihat | Cukup terang | Agak gelap | Gelap/tidak jelas | |
| **Framing** | Hardware dan layar terlihat jelas, angle optimal | Framing baik | Kadang tidak fokus | Framing buruk | |
| **Stabilitas** | Stabil, tidak goyang | Sedikit goyang | Cukup goyang | Sangat goyang | |

**Subtotal B1:** ___/16 × 0.94 = ___/15

#### B2. Kualitas Audio (10%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Kejelasan Suara** | Narasi jelas, mudah dipahami | Cukup jelas | Kadang tidak jelas | Sulit dipahami | |
| **Background Noise** | Tidak ada noise mengganggu | Minimal noise | Noise cukup banyak | Noise sangat mengganggu | |

**Subtotal B2:** ___/8 × 1.25 = ___/10

#### B3. Editing (5%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Flow & Transisi** | Smooth, professional transitions | Editing baik | Editing minimal | Tidak ada editing | |
| **Text/Overlay** | Ada label, subtitle, highlight yang membantu | Beberapa text overlay | Minimal | Tidak ada | |

**Subtotal B3:** ___/8 × 0.625 = ___/5

---

### C. Penyampaian (20%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Kejelasan Bicara** | Artikulasi jelas, pace tepat | Cukup jelas | Kadang tidak jelas | Sulit dipahami | |
| **Confidence** | Percaya diri, natural | Cukup percaya diri | Agak nervous | Sangat nervous/reading script | |
| **Engagement** | Menarik, antusias | Cukup engaging | Monoton | Membosankan | |
| **Bahasa** | Bahasa Indonesia/Inggris baik dan benar | Minor grammatical errors | Cukup banyak error | Banyak error | |
| **Durasi** | Sesuai (5-8 menit) | Sedikit over/under (±1 menit) | Cukup berbeda (±2 menit) | Sangat berbeda | |

**Subtotal C:** ___/20

---

### D. Kelengkapan & Kepatuhan (10%)

| Kriteria | Excellent (4) | Good (3) | Fair (2) | Poor (1) | Score |
|----------|---------------|----------|----------|----------|-------|
| **Struktur Video** | Mengikuti struktur yang diminta lengkap | Mayoritas struktur ada | Beberapa bagian hilang | Tidak terstruktur | |
| **Deadline** | Tepat waktu | Terlambat 1 hari | Terlambat 2-3 hari | Terlambat >3 hari | |

**Subtotal D:** ___/8 × 1.25 = ___/10

---

## 📊 Rekapitulasi Nilai

| Komponen | Bobot | Nilai | Weighted |
|----------|-------|-------|----------|
| A. Konten & Substansi | 40% | ___/40 | ___ |
| B. Kualitas Teknis | 30% | ___/30 | ___ |
| C. Penyampaian | 20% | ___/20 | ___ |
| D. Kelengkapan | 10% | ___/10 | ___ |
| **TOTAL** | **100%** | | **___/100** |

---

## 🏆 Bonus Points (Max +10)

| Kriteria Bonus | Points |
|----------------|--------|
| Penggunaan oscilloscope untuk verifikasi | +3 |
| Animasi/grafik penjelasan original | +2 |
| Subtitle/caption lengkap | +2 |
| Perbandingan dengan datasheet | +2 |
| Creative intro/outro | +1 |
| B-roll footage profesional | +2 |

**Total Bonus:** ___/10

---

## ⚠️ Penalty Points

| Pelanggaran | Penalty |
|-------------|---------|
| Video > 10 menit | -5 |
| Video < 3 menit | -10 |
| Tidak ada demo hardware | -20 |
| Plagiarisme video | -50 atau 0 |
| Link video tidak dapat diakses | -100 (tidak dinilai) |
| Audio tidak ada | -30 |

---

## 📋 Checklist Sebelum Submit

### Konten
- [ ] Ada penjelasan teori DAC
- [ ] Ada penjelasan teori PWM
- [ ] Demo STM32 DAC terlihat jelas
- [ ] Demo STM32 PWM terlihat jelas
- [ ] Demo ESP32 DAC terlihat jelas
- [ ] Demo ESP32 PWM terlihat jelas
- [ ] Ada penjelasan kode
- [ ] Ada analisis/perbandingan
- [ ] Ada kesimpulan

### Teknis
- [ ] Resolusi minimal 720p
- [ ] Audio jelas dan tidak ada noise berlebih
- [ ] Durasi 5-8 menit
- [ ] Format MP4
- [ ] Link dapat diakses (test dengan browser incognito)

### Identitas
- [ ] Nama dan NIM disebutkan
- [ ] Judul praktikum disebutkan
- [ ] Thumbnail dengan informasi yang jelas (untuk YouTube)

---

## 📝 Catatan Penilai

### Kelebihan Video:
```
_________________________________________________________________
_________________________________________________________________
```

### Area yang Perlu Diperbaiki:
```
_________________________________________________________________
_________________________________________________________________
```

### Feedback Khusus:
```
_________________________________________________________________
_________________________________________________________________
```

---

## 📎 Informasi Submission

**Nama/NIM:** _______________________

**Kelompok:** _______________________

**Link Video:** _______________________

**Tanggal Submit:** _______________________

**Tanggal Penilaian:** _______________________

**Penilai:** _______________________

**Nilai Akhir:** _______/100

**Tanda Tangan:** ___________________


========================================================
=== Modul-06-I2C-Sensor ===
========================================================


-----------------------------------------------------------
--- Jobsheet.md ---
-----------------------------------------------------------

# JOBSHEET BAB 06: I2C Bus dan Sensor Integration

## 📋 Informasi Umum

| Item | Keterangan |
|------|------------|
| **Mata Kuliah** | Praktikum Sistem Embedded |
| **Topik** | I2C Bus dan Integrasi Sensor |
| **Waktu** | 3 × 170 menit |
| **Tools** | PlatformIO, STM32CubeIDE, Logic Analyzer |

### Persiapan Hardware

**Komponen yang Dibutuhkan:**

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | Microcontroller utama |
| 2 | ESP32 DevKit V1 | 1 | Microcontroller utama |
| 3 | BME280 Module | 1 | Temp/Humidity/Pressure sensor |
| 4 | SSD1306 OLED 128×64 | 1 | Display I2C |
| 5 | DS3231 RTC Module | 1 | Real-Time Clock |
| 6 | 24LC256 EEPROM | 1 | External storage |
| 7 | Resistor 4.7kΩ | 4 | Pull-up resistors |
| 8 | Breadboard | 1 | Prototyping |
| 9 | Jumper Wires | ~30 | Koneksi |
| 10 | ST-Link V2 | 1 | Programmer STM32 |
| 11 | USB Cable | 2 | Power dan programming |

### Skema Koneksi

**STM32F103 I2C Connections:**
```
STM32F103        I2C Devices (BME280, OLED, RTC, EEPROM)
┌─────────┐      ┌──────────────────┐
│      PB6├──────┤SCL (All Devices) │
│      PB7├──────┤SDA (All Devices) │
│     3.3V├──────┤VCC               │
│      GND├──────┤GND               │
└─────────┘      └──────────────────┘
                 Note: 4.7kΩ pull-ups on SDA and SCL
```

**ESP32 I2C Connections:**
```
ESP32            I2C Devices
┌─────────┐      ┌──────────────────┐
│    GPIO22├─────┤SCL (All Devices) │
│    GPIO21├─────┤SDA (All Devices) │
│      3.3V├─────┤VCC               │
│       GND├─────┤GND               │
└──────────┘     └──────────────────┘
```

---

## 🔬 Daftar Praktikum

### Jumlah Program STM32: 12
### Jumlah Program ESP32: 12

---

## PROGRAM 1: I2C Bus Scanner (STM32)

### 📝 Deskripsi
Membuat scanner untuk mendeteksi semua perangkat I2C yang terhubung pada bus.

### 🎯 Tujuan
- Memahami cara kerja addressing I2C
- Mendeteksi perangkat yang terhubung pada bus
- Troubleshooting koneksi I2C

### 📊 Diagram Blok
```
┌─────────────┐     ┌─────────┐     ┌─────────┐
│   STM32     │────►│  I2C    │────►│ Device  │
│  (Master)   │◄────│   Bus   │◄────│ (Slave) │
└─────────────┘     └─────────┘     └─────────┘
                         │
                    ┌────┴────┐
                   Rp        Rp
                    │         │
                   Vcc       Vcc
```

### 💻 Kode Program

**File: `STM32_01_I2C_Bus_Scan/src/main.c`**

```c
/**
 * @file main.c
 * @brief I2C Bus Scanner for STM32F103
 * @details Scans I2C bus and reports all connected devices
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Private variables */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
void I2C_Scan(void);

/* UART redirect for printf */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* Known I2C device database */
typedef struct {
    uint8_t address;
    const char* name;
} I2C_Device_t;

const I2C_Device_t known_devices[] = {
    {0x3C, "SSD1306 OLED (Addr 0)"},
    {0x3D, "SSD1306 OLED (Addr 1)"},
    {0x50, "24LC EEPROM (A0=A1=A2=0)"},
    {0x51, "24LC EEPROM (A0=1)"},
    {0x52, "24LC EEPROM (A1=1)"},
    {0x57, "24LC EEPROM (All=1)"},
    {0x68, "DS3231 RTC / MPU6050"},
    {0x69, "MPU6050 (AD0=1)"},
    {0x76, "BME280/BMP280 (SDO=0)"},
    {0x77, "BME280/BMP280 (SDO=1)"},
    {0x27, "PCF8574 LCD I2C"},
    {0x20, "PCF8574A"},
    {0x48, "ADS1115 ADC"},
    {0x00, NULL}  // End marker
};

const char* get_device_name(uint8_t addr) {
    for (int i = 0; known_devices[i].name != NULL; i++) {
        if (known_devices[i].address == addr) {
            return known_devices[i].name;
        }
    }
    return "Unknown Device";
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();
    
    printf("\r\n========================================\r\n");
    printf("   STM32F103 I2C Bus Scanner\r\n");
    printf("========================================\r\n\n");
    
    while (1) {
        printf("Press Enter to scan I2C bus...\r\n");
        
        // Wait for user input
        uint8_t ch;
        HAL_UART_Receive(&huart1, &ch, 1, HAL_MAX_DELAY);
        
        I2C_Scan();
        
        HAL_Delay(1000);
    }
}

void I2C_Scan(void) {
    uint8_t devices_found = 0;
    HAL_StatusTypeDef result;
    
    printf("\r\nScanning I2C bus (addresses 0x01 - 0x7F)...\r\n\n");
    printf("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");
    
    for (uint8_t row = 0; row < 8; row++) {
        printf("%02X: ", row * 16);
        
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = row * 16 + col;
            
            if (addr < 0x03 || addr > 0x77) {
                printf("   ");  // Reserved addresses
                continue;
            }
            
            result = HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 2, 10);
            
            if (result == HAL_OK) {
                printf("%02X ", addr);
                devices_found++;
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }
    
    printf("\r\n----------------------------------------\r\n");
    printf("Total devices found: %d\r\n\n", devices_found);
    
    // List found devices with names
    if (devices_found > 0) {
        printf("Device List:\r\n");
        for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
            result = HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10);
            if (result == HAL_OK) {
                printf("  0x%02X - %s\r\n", addr, get_device_name(addr));
            }
        }
    }
    printf("----------------------------------------\r\n\n");
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;  // 100 kHz Standard Mode
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // Configure LED on PC13
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* I2C MSP Initialization */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (hi2c->Instance == I2C1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();
        
        // PB6 -> SCL, PB7 -> SDA
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (huart->Instance == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();
        
        // PA9 -> TX, PA10 -> RX
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
```

### 📈 Hasil yang Diharapkan
```
========================================
   STM32F103 I2C Bus Scanner
========================================

Scanning I2C bus (addresses 0x01 - 0x7F)...

     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- 3C -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
70: -- -- -- -- -- -- 76 --

----------------------------------------
Total devices found: 4

Device List:
  0x3C - SSD1306 OLED (Addr 0)
  0x50 - 24LC EEPROM (A0=A1=A2=0)
  0x68 - DS3231 RTC / MPU6050
  0x76 - BME280/BMP280 (SDO=0)
----------------------------------------
```

### ❓ Pertanyaan Analisis
1. Mengapa scanning dimulai dari address 0x03 dan bukan 0x00?
2. Apa yang terjadi jika dua device memiliki address yang sama?
3. Bagaimana pengaruh pull-up resistor terhadap hasil scanning?

---

## PROGRAM 2: I2C Bus Scanner (ESP32)

### 📝 Deskripsi
Implementasi I2C scanner pada ESP32 dengan Wire library.

### 💻 Kode Program

**File: `ESP32_01_I2C_Bus_Scan/src/main.cpp`**

```cpp
/**
 * @file main.cpp
 * @brief I2C Bus Scanner for ESP32
 * @details Scans I2C bus using Wire library
 */

#include <Arduino.h>
#include <Wire.h>

// I2C Pin definitions
#define I2C_SDA 21
#define I2C_SCL 22

// Known device database
struct I2CDevice {
    uint8_t address;
    const char* name;
};

const I2CDevice knownDevices[] = {
    {0x3C, "SSD1306 OLED"},
    {0x3D, "SSD1306 OLED (Alt)"},
    {0x50, "24LC EEPROM"},
    {0x68, "DS3231 RTC / MPU6050"},
    {0x76, "BME280 (SDO=GND)"},
    {0x77, "BME280 (SDO=VCC)"},
    {0x27, "LCD I2C (PCF8574)"},
    {0x20, "PCF8574A"},
    {0x48, "ADS1115 ADC"},
    {0x29, "VL53L0X ToF"},
    {0x1E, "HMC5883L Compass"},
    {0x00, nullptr}  // End marker
};

const char* getDeviceName(uint8_t addr) {
    for (int i = 0; knownDevices[i].name != nullptr; i++) {
        if (knownDevices[i].address == addr) {
            return knownDevices[i].name;
        }
    }
    return "Unknown Device";
}

void scanI2C() {
    Serial.println("\n========================================");
    Serial.println("     ESP32 I2C Bus Scanner");
    Serial.println("========================================\n");
    
    Serial.printf("SDA Pin: GPIO%d\n", I2C_SDA);
    Serial.printf("SCL Pin: GPIO%d\n", I2C_SCL);
    Serial.println();
    
    uint8_t devicesFound = 0;
    
    Serial.println("Scanning...\n");
    Serial.println("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
    
    for (uint8_t row = 0; row < 8; row++) {
        Serial.printf("%02X: ", row * 16);
        
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = row * 16 + col;
            
            if (addr < 0x03 || addr > 0x77) {
                Serial.print("   ");
                continue;
            }
            
            Wire.beginTransmission(addr);
            uint8_t error = Wire.endTransmission();
            
            if (error == 0) {
                Serial.printf("%02X ", addr);
                devicesFound++;
            } else if (error == 4) {
                Serial.print("?? ");  // Unknown error
            } else {
                Serial.print("-- ");
            }
        }
        Serial.println();
    }
    
    Serial.println();
    Serial.println("----------------------------------------");
    Serial.printf("Total devices found: %d\n\n", devicesFound);
    
    if (devicesFound > 0) {
        Serial.println("Detected Devices:");
        Serial.println("----------------------------------------");
        
        for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("  0x%02X - %s\n", addr, getDeviceName(addr));
            }
        }
        Serial.println("----------------------------------------");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n");
    Serial.println("================================");
    Serial.println("  I2C Scanner Initialization");
    Serial.println("================================");
    
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);  // 100 kHz
    
    Serial.println("I2C initialized successfully!");
    Serial.println("Type 's' to scan I2C bus\n");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        
        if (cmd == 's' || cmd == 'S') {
            scanI2C();
        } else if (cmd == 'f') {
            // Fast mode scan
            Wire.setClock(400000);
            Serial.println("Switched to Fast Mode (400 kHz)");
            scanI2C();
            Wire.setClock(100000);
        }
    }
    
    // Auto scan every 10 seconds
    static unsigned long lastScan = 0;
    if (millis() - lastScan > 10000) {
        lastScan = millis();
        scanI2C();
    }
}
```

### 📈 Hasil yang Diharapkan
Output serupa dengan STM32 scanner, menampilkan semua device yang terdeteksi.

---

## PROGRAM 3: BME280 Sensor Reading (STM32)

### 📝 Deskripsi
Membaca data temperatur, kelembaban, dan tekanan dari sensor BME280.

### 💻 Kode Program

**File: `STM32_02_BME280_Sensor/src/main.c`**

```c
/**
 * @file main.c
 * @brief BME280 Sensor Reading for STM32F103
 * @details Reads temperature, humidity, and pressure
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <math.h>

/* BME280 I2C Address */
#define BME280_ADDR     0x76

/* BME280 Registers */
#define BME280_REG_ID           0xD0
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_STATUS       0xF3
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_PRESS_MSB    0xF7
#define BME280_REG_CALIB00      0x88
#define BME280_REG_CALIB26      0xE1

/* BME280 Chip ID */
#define BME280_CHIP_ID          0x60

/* Handles */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

/* BME280 Calibration Data */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} BME280_CalibData_t;

BME280_CalibData_t calib_data;
int32_t t_fine;

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* BME280 Functions */
HAL_StatusTypeDef BME280_ReadReg(uint8_t reg, uint8_t* data, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, BME280_ADDR << 1, reg, 
                            I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

HAL_StatusTypeDef BME280_WriteReg(uint8_t reg, uint8_t data) {
    return HAL_I2C_Mem_Write(&hi2c1, BME280_ADDR << 1, reg,
                             I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

uint8_t BME280_Init(void) {
    uint8_t chip_id;
    uint8_t calib[26];
    uint8_t calib_h[7];
    
    // Read chip ID
    if (BME280_ReadReg(BME280_REG_ID, &chip_id, 1) != HAL_OK) {
        printf("Failed to read BME280 ID\r\n");
        return 0;
    }
    
    if (chip_id != BME280_CHIP_ID) {
        printf("Invalid chip ID: 0x%02X (expected 0x%02X)\r\n", chip_id, BME280_CHIP_ID);
        return 0;
    }
    
    printf("BME280 detected! Chip ID: 0x%02X\r\n", chip_id);
    
    // Read calibration data
    BME280_ReadReg(BME280_REG_CALIB00, calib, 26);
    BME280_ReadReg(BME280_REG_CALIB26, calib_h, 7);
    
    // Parse calibration data
    calib_data.dig_T1 = (uint16_t)(calib[1] << 8) | calib[0];
    calib_data.dig_T2 = (int16_t)(calib[3] << 8) | calib[2];
    calib_data.dig_T3 = (int16_t)(calib[5] << 8) | calib[4];
    
    calib_data.dig_P1 = (uint16_t)(calib[7] << 8) | calib[6];
    calib_data.dig_P2 = (int16_t)(calib[9] << 8) | calib[8];
    calib_data.dig_P3 = (int16_t)(calib[11] << 8) | calib[10];
    calib_data.dig_P4 = (int16_t)(calib[13] << 8) | calib[12];
    calib_data.dig_P5 = (int16_t)(calib[15] << 8) | calib[14];
    calib_data.dig_P6 = (int16_t)(calib[17] << 8) | calib[16];
    calib_data.dig_P7 = (int16_t)(calib[19] << 8) | calib[18];
    calib_data.dig_P8 = (int16_t)(calib[21] << 8) | calib[20];
    calib_data.dig_P9 = (int16_t)(calib[23] << 8) | calib[22];
    
    calib_data.dig_H1 = calib[25];
    calib_data.dig_H2 = (int16_t)(calib_h[1] << 8) | calib_h[0];
    calib_data.dig_H3 = calib_h[2];
    calib_data.dig_H4 = (int16_t)(calib_h[3] << 4) | (calib_h[4] & 0x0F);
    calib_data.dig_H5 = (int16_t)(calib_h[5] << 4) | ((calib_h[4] >> 4) & 0x0F);
    calib_data.dig_H6 = (int8_t)calib_h[6];
    
    // Configure sensor
    BME280_WriteReg(BME280_REG_CTRL_HUM, 0x01);
    BME280_WriteReg(BME280_REG_CONFIG, 0xA0);
    BME280_WriteReg(BME280_REG_CTRL_MEAS, 0x27);
    
    return 1;
}

int32_t BME280_CompensateTemp(int32_t adc_T) {
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * 
            ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * 
             ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * 
            ((int32_t)calib_data.dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

uint32_t BME280_CompensatePress(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + 
           ((var1 * (int64_t)calib_data.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
    return (uint32_t)p;
}

uint32_t BME280_CompensateHum(int32_t adc_H) {
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - 
                   (((int32_t)calib_data.dig_H5) * v_x1_u32r)) + 
                  ((int32_t)16384)) >> 15) * 
                (((((((v_x1_u32r * ((int32_t)calib_data.dig_H6)) >> 10) * 
                    (((v_x1_u32r * ((int32_t)calib_data.dig_H3)) >> 11) + 
                     ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * 
                  ((int32_t)calib_data.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                               ((int32_t)calib_data.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t)(v_x1_u32r >> 12);
}

void BME280_ReadData(float* temperature, float* pressure, float* humidity) {
    uint8_t data[8];
    int32_t adc_T, adc_P, adc_H;
    BME280_ReadReg(BME280_REG_PRESS_MSB, data, 8);
    adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);
    adc_H = ((int32_t)data[6] << 8) | (int32_t)data[7];
    *temperature = BME280_CompensateTemp(adc_T) / 100.0f;
    *pressure = BME280_CompensatePress(adc_P) / 25600.0f;
    *humidity = BME280_CompensateHum(adc_H) / 1024.0f;
}

float BME280_CalculateAltitude(float pressure, float seaLevelPressure) {
    return 44330.0f * (1.0f - powf(pressure / seaLevelPressure, 0.1903f));
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();
    
    printf("\r\n========================================\r\n");
    printf("   STM32F103 BME280 Sensor Demo\r\n");
    printf("========================================\r\n\n");
    
    if (!BME280_Init()) {
        printf("BME280 initialization failed!\r\n");
        while(1) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(100);
        }
    }
    
    printf("BME280 initialized successfully!\r\n\n");
    
    while (1) {
        float temperature, pressure, humidity;
        BME280_ReadData(&temperature, &pressure, &humidity);
        float altitude = BME280_CalculateAltitude(pressure, 1013.25f);
        
        printf("----------------------------------------\r\n");
        printf("Temperature: %.2f C\r\n", temperature);
        printf("Humidity:    %.2f %%RH\r\n", humidity);
        printf("Pressure:    %.2f hPa\r\n", pressure);
        printf("Altitude:    %.2f m (approx)\r\n", altitude);
        printf("----------------------------------------\r\n\n");
        
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(2000);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hi2c->Instance == I2C1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
```

### 📈 Hasil yang Diharapkan
```
========================================
   STM32F103 BME280 Sensor Demo
========================================

BME280 detected! Chip ID: 0x60
BME280 initialized successfully!

----------------------------------------
Temperature: 25.43 C
Humidity:    65.21 %RH
Pressure:    1013.25 hPa
Altitude:    0.00 m (approx)
----------------------------------------
```

---

## PROGRAM 4: BME280 Sensor Reading (ESP32)

### 📝 Deskripsi
Membaca sensor BME280 menggunakan library Adafruit pada ESP32.

### 💻 Kode Program

**File: `ESP32_02_BME280_Sensor/src/main.cpp`**

```cpp
/**
 * @file main.cpp
 * @brief BME280 Environmental Sensor on ESP32
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define I2C_SDA 21
#define I2C_SCL 22
#define BME280_ADDR 0x76
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n================================");
    Serial.println("  ESP32 BME280 Sensor Demo");
    Serial.println("================================\n");
    
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    
    if (!bme.begin(BME280_ADDR, &Wire)) {
        Serial.println("Could not find BME280!");
        while (1);
    }
    
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X16,
                    Adafruit_BME280::SAMPLING_X16,
                    Adafruit_BME280::SAMPLING_X16,
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_0_5);
    
    Serial.println("BME280 initialized!\n");
}

void loop() {
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float press = bme.readPressure() / 100.0F;
    float alt = bme.readAltitude(SEALEVELPRESSURE_HPA);
    
    Serial.println("----------------------------------------");
    Serial.printf("Temperature: %.2f C\n", temp);
    Serial.printf("Humidity:    %.2f %%RH\n", hum);
    Serial.printf("Pressure:    %.2f hPa\n", press);
    Serial.printf("Altitude:    %.2f m\n", alt);
    Serial.println("----------------------------------------\n");
    
    delay(2000);
}
```

**platformio.ini:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    adafruit/Adafruit BME280 Library@^2.2.2
    adafruit/Adafruit Unified Sensor@^1.1.9
```

---

## PROGRAM 5-6: SSD1306 OLED Display (STM32 & ESP32)

### 📝 Deskripsi
Menampilkan teks dan grafik pada OLED display SSD1306 128×64.

### 💻 Kode ESP32

**File: `ESP32_03_OLED_Display/src/main.cpp`**

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed!");
        while (1);
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 10);
    display.println("ESP32");
    display.setCursor(10, 30);
    display.println("OLED");
    display.display();
}

void loop() {
    // Demo animations
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, WHITE);
    display.drawCircle(64, 32, 20, WHITE);
    display.display();
    delay(2000);
    
    display.clearDisplay();
    for (int i = 0; i < 128; i += 8) {
        display.drawLine(0, 32, i, 0, WHITE);
        display.drawLine(0, 32, i, 63, WHITE);
    }
    display.display();
    delay(2000);
}
```

---

## PROGRAM 7-8: DS3231 RTC (STM32 & ESP32)

### 💻 Kode ESP32

**File: `ESP32_04_DS3231_RTC/src/main.cpp`**

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC!");
        while (1);
    }
    
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void loop() {
    DateTime now = rtc.now();
    
    Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
    Serial.printf("Temperature: %.2f C\n\n", rtc.getTemperature());
    
    delay(1000);
}
```

---

## PROGRAM 9-10: 24LC256 EEPROM (STM32 & ESP32)

### 💻 Kode ESP32

**File: `ESP32_05_EEPROM_24LC256/src/main.cpp`**

```cpp
#include <Arduino.h>
#include <Wire.h>

#define EEPROM_ADDR 0x50

void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.write(data);
    Wire.endTransmission();
    delay(5);
}

uint8_t EEPROM_ReadByte(uint16_t addr) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_ADDR, 1);
    return Wire.read();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    
    // Write test
    EEPROM_WriteByte(0x0000, 0xAB);
    Serial.printf("Written: 0xAB\n");
    
    // Read back
    uint8_t data = EEPROM_ReadByte(0x0000);
    Serial.printf("Read: 0x%02X\n", data);
}

void loop() {
    delay(1000);
}
```

---

## PROGRAM 11-12: Multi-Device & Bus Recovery

### 💻 Kode ESP32

**File: `ESP32_11_Multi_Device/src/main.cpp`**

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

Adafruit_BME280 bme;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
RTC_DS3231 rtc;

void i2cBusRecovery() {
    Wire.end();
    pinMode(21, INPUT_PULLUP);
    pinMode(22, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(22, LOW);
        delayMicroseconds(5);
        digitalWrite(22, HIGH);
        delayMicroseconds(5);
    }
    Wire.begin(21, 22);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    
    bme.begin(0x76);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    rtc.begin();
}

void loop() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    DateTime now = rtc.now();
    display.setCursor(0, 0);
    display.printf("%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    
    display.setCursor(0, 16);
    display.printf("T:%.1fC H:%.0f%%", bme.readTemperature(), bme.readHumidity());
    
    display.setCursor(0, 32);
    display.printf("P:%.0f hPa", bme.readPressure()/100.0);
    
    display.display();
    delay(1000);
}
```

---

## 📝 Tugas dan Latihan

### Tugas 1: I2C Device Integration
Buat sistem monitoring lingkungan yang membaca BME280, menampilkan pada OLED, dan menyimpan timestamp ke EEPROM menggunakan DS3231.

### Tugas 2: Multi-Sensor Dashboard
Implementasikan dashboard dengan grafik suhu 1 menit terakhir pada OLED dengan alarm threshold.

### Tugas 3: Bus Recovery Implementation
Implementasikan mekanisme recovery otomatis dengan deteksi timeout dan re-initialization.

---

## ✅ Checklist Evaluasi

| No | Kriteria | Bobot |
|----|----------|-------|
| 1 | I2C Scanner berfungsi | 10% |
| 2 | BME280 data akurat | 15% |
| 3 | OLED display grafik | 15% |
| 4 | RTC waktu akurat | 15% |
| 5 | EEPROM read/write | 15% |
| 6 | Multi-device integration | 15% |
| 7 | Bus recovery | 10% |
| 8 | Dokumentasi | 5% |

---

## 📚 Referensi

1. NXP I2C-bus Specification (UM10204)
2. BME280 Datasheet - Bosch Sensortec
3. SSD1306 Datasheet - Solomon Systech
4. DS3231 Datasheet - Maxim Integrated
5. 24LC256 Datasheet - Microchip



-----------------------------------------------------------
--- Materi.md ---
-----------------------------------------------------------

# BAB 06: I2C Bus dan Sensor Integration

## 🎯 Capaian Pembelajaran

Setelah menyelesaikan bab ini, mahasiswa diharapkan mampu:

1. Memahami prinsip kerja protokol I2C (Inter-Integrated Circuit)
2. Mengidentifikasi karakteristik sinyal SDA dan SCL pada I2C
3. Mengkonfigurasi I2C pada STM32 dan ESP32 sebagai Master
4. Membaca data dari berbagai sensor I2C (BME280, DS3231, OLED SSD1306)
5. Mengakses EEPROM eksternal melalui I2C
6. Menangani multiple device pada I2C bus
7. Melakukan troubleshooting masalah komunikasi I2C

---

## 📚 Materi Pembelajaran

### 1. Pendahuluan I2C

#### 1.1 Sejarah dan Latar Belakang

I2C (Inter-Integrated Circuit) dikembangkan oleh Philips Semiconductor (sekarang NXP) pada tahun 1982. Protokol ini dirancang untuk komunikasi antar chip dalam satu PCB dengan kebutuhan pin minimal.

**Karakteristik Utama I2C:**
- Hanya membutuhkan 2 wire: SDA (Data) dan SCL (Clock)
- Multi-master dan multi-slave capable
- Addressing system untuk membedakan device
- Kecepatan hingga 3.4 Mbps (High Speed Mode)
- Open-drain output dengan pull-up resistor

#### 1.2 Perbandingan dengan Protokol Lain

| Fitur | I2C | SPI | UART |
|-------|-----|-----|------|
| **Jumlah Wire** | 2 (+ GND) | 4+ | 2 (+ GND) |
| **Topology** | Bus | Point-to-point/Bus | Point-to-point |
| **Max Device** | 127 (7-bit addr) | Unlimited (CS pins) | 1 |
| **Duplex** | Half | Full | Full |
| **Max Speed** | 3.4 Mbps | 10+ Mbps | ~1 Mbps |
| **Complexity** | Medium | Low | Low |

### 2. Teori Dasar I2C

#### 2.1 Arsitektur I2C Bus

```
                    Vcc (3.3V or 5V)
                        │
                  ┌─────┴─────┐
                 Rp          Rp     (Pull-up Resistors: 4.7kΩ typical)
                  │           │
        ┌─────────┼───────────┼─────────┐
        │         │           │         │
   ┌────┴────┐┌───┴───┐  ┌────┴────┐┌───┴───┐
   │  Master ││Slave 1│  │ Slave 2 ││Slave 3│
   │  (MCU)  ││(Sensor)│ │ (EEPROM)││ (RTC) │
   └────┬────┘└───┬───┘  └────┬────┘└───┬───┘
        │         │           │         │
        └─────────┴───────────┴─────────┘
                        │
                       GND

        SDA ─────────────────────────────
        SCL ─────────────────────────────
```

#### 2.2 Sinyal I2C

**SDA (Serial Data):**
- Bidirectional data line
- Open-drain output
- Data valid saat SCL HIGH
- Dapat berubah saat SCL LOW

**SCL (Serial Clock):**
- Clock line dari Master
- Open-drain output
- Menentukan timing transfer data
- Clock stretching supported

#### 2.3 Timing Diagram

```
START Condition:
        ┌───────────────────────
SDA ────┘         
            ┌───────────────────
SCL ────────┘

Data Transfer (1 bit):
        ┌───────┐       ┌───────┐
SDA ────┘  D7   └───────┘  D6   └───
            ┌───┐           ┌───┐
SCL ────────┘   └───────────┘   └───
        │   │   │       │   │   │
        Setup  Hold     Setup  Hold

STOP Condition:
                    ┌───────────────
SDA ────────────────┘
        ────────────────────────────
SCL 

Repeated START:
            ┌───────┐
SDA ────────┘       └───
        ────────────────┐
SCL             ┌───────┘
```

#### 2.4 Frame Format

**7-bit Address Mode:**
```
┌─────┬───────────────────┬─────┬─────┬────────────────┬─────┬──────┐
│START│    7-bit Address  │ R/W │ ACK │   8-bit Data   │ ACK │ STOP │
│  1  │ A6 A5 A4 A3 A2 A1 A0│  1  │  1  │ D7...D0       │  1  │  1   │
└─────┴───────────────────┴─────┴─────┴────────────────┴─────┴──────┘
```

**R/W Bit:**
- 0 = Write (Master → Slave)
- 1 = Read (Slave → Master)

**ACK/NACK:**
- ACK (Acknowledge): SDA LOW saat SCL pulse ke-9
- NACK (Not Acknowledge): SDA HIGH saat SCL pulse ke-9

#### 2.5 I2C Speed Modes

| Mode | Speed | Aplikasi |
|------|-------|----------|
| Standard Mode | 100 kbps | Sensor, EEPROM |
| Fast Mode | 400 kbps | Display, IMU |
| Fast Mode Plus | 1 Mbps | High-speed sensors |
| High Speed | 3.4 Mbps | Special applications |

### 3. I2C pada STM32F103

#### 3.1 Fitur I2C STM32F103

- 2 I2C peripheral (I2C1, I2C2)
- 7-bit dan 10-bit addressing
- Multi-master capable
- DMA support
- SMBus compatible
- Clock stretching support

**Pin Mapping:**

| Peripheral | SCL | SDA | Remap SCL | Remap SDA |
|------------|-----|-----|-----------|-----------|
| I2C1 | PB6 | PB7 | PB8 | PB9 |
| I2C2 | PB10 | PB11 | - | - |

#### 3.2 Konfigurasi I2C STM32 (HAL)

```c
I2C_HandleTypeDef hi2c1;

void I2C1_Init(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // Configure GPIO: PB6 (SCL), PB7 (SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;      // Open-drain
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Configure I2C
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;              // 400 kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}
```

#### 3.3 Operasi I2C STM32

**Scan I2C Bus:**
```c
void I2C_Scan(void) {
    printf("Scanning I2C bus...\n");
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
            printf("Device found at 0x%02X\n", addr);
        }
    }
}
```

**Write Data:**
```c
HAL_StatusTypeDef I2C_Write(uint8_t dev_addr, uint8_t reg, uint8_t* data, uint16_t len) {
    return HAL_I2C_Mem_Write(&hi2c1, dev_addr << 1, reg, 
                             I2C_MEMADD_SIZE_8BIT, data, len, 100);
}
```

**Read Data:**
```c
HAL_StatusTypeDef I2C_Read(uint8_t dev_addr, uint8_t reg, uint8_t* data, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, data, len, 100);
}
```

### 4. I2C pada ESP32

#### 4.1 Fitur I2C ESP32

- 2 I2C controller (I2C_NUM_0, I2C_NUM_1)
- Flexible GPIO mapping (any GPIO)
- 7-bit dan 10-bit addressing
- Master dan Slave mode
- Clock stretching support
- Arbitration support

#### 4.2 Konfigurasi I2C ESP32 (Arduino)

```cpp
#include <Wire.h>

void setup() {
    // Default: SDA=21, SCL=22
    Wire.begin();
    
    // Or specify pins
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Set clock frequency
    Wire.setClock(400000);  // 400 kHz
}
```

#### 4.3 I2C ESP32 dengan ESP-IDF

```cpp
#include <driver/i2c.h>

void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}
```

### 5. Sensor I2C Populer

#### 5.1 BME280 - Temperature, Humidity, Pressure Sensor

**Spesifikasi:**
| Parameter | Range | Accuracy |
|-----------|-------|----------|
| Temperature | -40°C to +85°C | ±1.0°C |
| Humidity | 0-100% RH | ±3% RH |
| Pressure | 300-1100 hPa | ±1 hPa |

**I2C Address:** 0x76 atau 0x77 (tergantung SDO pin)

**Register Map:**
```
0xD0 - Chip ID (should read 0x60 for BME280)
0xF2 - ctrl_hum (humidity control)
0xF4 - ctrl_meas (temperature & pressure control)
0xF5 - config (rate, filter, interface)
0xF7-0xFE - Data registers (pressure, temperature, humidity)
```

**Kode Pembacaan BME280:**
```cpp
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
    Serial.begin(115200);
    
    if (!bme.begin(0x76)) {
        Serial.println("BME280 not found!");
        while(1);
    }
    
    // Configure oversampling
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X16,  // temp
                    Adafruit_BME280::SAMPLING_X16,  // pressure
                    Adafruit_BME280::SAMPLING_X16,  // humidity
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_0_5);
}

void loop() {
    float temp = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;  // hPa
    float altitude = bme.readAltitude(1013.25);    // Sea level pressure
    
    Serial.printf("Temp: %.2f°C, Hum: %.2f%%, Press: %.2f hPa, Alt: %.2f m\n",
                  temp, humidity, pressure, altitude);
    delay(1000);
}
```

#### 5.2 SSD1306 - OLED Display

**Spesifikasi:**
- Resolusi: 128×64 atau 128×32 pixels
- Monochrome
- I2C Address: 0x3C atau 0x3D
- Voltage: 3.3V atau 5V

**Kode OLED SSD1306:**
```cpp
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while(1);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Hello World!");
    display.display();
}
```

#### 5.3 DS3231 - Real-Time Clock

**Spesifikasi:**
- Accuracy: ±2ppm (±1 min/year)
- Battery backup (CR2032)
- Temperature compensated crystal
- I2C Address: 0x68 (fixed)

**Register Map:**
```
0x00 - Seconds (BCD)
0x01 - Minutes (BCD)
0x02 - Hours (BCD)
0x03 - Day of Week
0x04 - Date (BCD)
0x05 - Month (BCD)
0x06 - Year (BCD)
0x07-0x0D - Alarms
0x0E - Control
0x0F - Status
0x11 - Temperature (MSB)
```

**Kode DS3231:**
```cpp
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
    Serial.begin(115200);
    
    if (!rtc.begin()) {
        Serial.println("RTC not found!");
        while(1);
    }
    
    // Set time if lost power
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void loop() {
    DateTime now = rtc.now();
    
    Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
                  now.year(), now.month(), now.day(),
                  now.hour(), now.minute(), now.second());
    
    // Read temperature from DS3231
    float temp = rtc.getTemperature();
    Serial.printf("RTC Temp: %.2f°C\n", temp);
    
    delay(1000);
}
```

#### 5.4 24LC256 - I2C EEPROM

**Spesifikasi:**
- Capacity: 256 Kbit (32KB)
- Page size: 64 bytes
- I2C Address: 0x50-0x57 (A0, A1, A2 pins)
- Write cycle: 5ms max

**Kode EEPROM 24LC256:**
```cpp
#include <Wire.h>

#define EEPROM_ADDR 0x50

void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));    // MSB
    Wire.write((uint8_t)(addr & 0xFF));  // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(5);  // Wait for write cycle
}

uint8_t EEPROM_ReadByte(uint16_t addr) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.endTransmission();
    
    Wire.requestFrom(EEPROM_ADDR, 1);
    return Wire.read();
}

void EEPROM_WritePage(uint16_t addr, uint8_t* data, uint8_t len) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.write(data, len);
    Wire.endTransmission();
    delay(5);
}
```

### 6. Multi-Device I2C

#### 6.1 Scanning Multiple Devices

```cpp
void scanI2CDevices() {
    Serial.println("I2C Scanner");
    Serial.println("Scanning...");
    
    int devices = 0;
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("Device found at 0x%02X", addr);
            
            // Identify known devices
            switch (addr) {
                case 0x3C:
                case 0x3D: Serial.print(" (SSD1306 OLED)"); break;
                case 0x50: Serial.print(" (24LC EEPROM)"); break;
                case 0x68: Serial.print(" (DS3231 RTC)"); break;
                case 0x76:
                case 0x77: Serial.print(" (BME280)"); break;
            }
            Serial.println();
            devices++;
        }
    }
    
    Serial.printf("Found %d device(s)\n", devices);
}
```

#### 6.2 Integrated System Example

```cpp
// Multiple I2C devices working together
void updateDisplay() {
    // Read sensor data
    float temp = bme.readTemperature();
    float humidity = bme.readHumidity();
    DateTime now = rtc.now();
    
    // Update OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Time: %02d:%02d:%02d", 
                   now.hour(), now.minute(), now.second());
    display.setCursor(0, 16);
    display.printf("Temp: %.1fC", temp);
    display.setCursor(0, 32);
    display.printf("Hum: %.1f%%", humidity);
    display.display();
    
    // Log to EEPROM
    static uint16_t logAddr = 0;
    EEPROM_WriteByte(logAddr++, (uint8_t)temp);
    if (logAddr >= 32768) logAddr = 0;  // Wrap around
}
```

### 7. I2C Bus Recovery

#### 7.1 Common I2C Problems

| Problem | Symptom | Cause | Solution |
|---------|---------|-------|----------|
| SDA stuck LOW | No communication | Slave holding SDA | Bus recovery |
| No ACK | NACK received | Wrong address/device unpowered | Check connections |
| Timeout | Operation hangs | Clock stretching too long | Increase timeout |
| Data corruption | Wrong data | EMI, wrong pull-up | Add filtering |

#### 7.2 Bus Recovery Algorithm

```cpp
void i2c_bus_recovery(int sda_pin, int scl_pin) {
    pinMode(sda_pin, INPUT_PULLUP);
    pinMode(scl_pin, OUTPUT);
    
    // Generate 9 clock pulses to release SDA
    for (int i = 0; i < 9; i++) {
        digitalWrite(scl_pin, LOW);
        delayMicroseconds(5);
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(5);
        
        // Check if SDA is released
        if (digitalRead(sda_pin) == HIGH) break;
    }
    
    // Generate STOP condition
    pinMode(sda_pin, OUTPUT);
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(5);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(5);
    digitalWrite(sda_pin, HIGH);
    delayMicroseconds(5);
    
    // Reinitialize I2C
    Wire.begin(sda_pin, scl_pin);
}
```

### 8. Pull-up Resistor Calculation

#### 8.1 Calculation Formula

```
Rp_min = (Vcc - Vol) / Iol
Rp_max = tr / (0.8473 × Cb)

Where:
- Vol = 0.4V (max low level output)
- Iol = 3mA (max sink current)
- tr = rise time (300ns for Standard, 100ns for Fast)
- Cb = bus capacitance (pF)
```

#### 8.2 Recommended Values

| Mode | Bus Cap | Rp Value |
|------|---------|----------|
| Standard (100kHz) | <100pF | 4.7kΩ |
| Standard (100kHz) | <200pF | 2.2kΩ |
| Fast (400kHz) | <100pF | 2.2kΩ |
| Fast (400kHz) | <200pF | 1kΩ |

### 9. Best Practices

#### 9.1 Hardware Design
- Use appropriate pull-up resistors
- Keep I2C traces short (<30cm)
- Use decoupling capacitors near devices
- Consider separate I2C buses for high-speed and low-speed devices

#### 9.2 Software Design
- Always check return values
- Implement timeout handling
- Use bus recovery mechanism
- Validate data with CRC when available

#### 9.3 Debugging Tips
```cpp
// Debug wrapper for I2C operations
HAL_StatusTypeDef I2C_Debug_Write(uint8_t addr, uint8_t reg, uint8_t* data, uint16_t len) {
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg,
                                                  I2C_MEMADD_SIZE_8BIT, 
                                                  data, len, 100);
    if (status != HAL_OK) {
        printf("I2C Write Error: addr=0x%02X, reg=0x%02X, status=%d\n",
               addr, reg, status);
        printf("Error code: 0x%08X\n", hi2c1.ErrorCode);
    }
    return status;
}
```

### 10. Rangkuman

1. **I2C** adalah protokol serial synchronous dengan 2 wire (SDA, SCL)
2. **Addressing** 7-bit memungkinkan hingga 127 device pada satu bus
3. **STM32F103** memiliki 2 I2C peripheral dengan DMA support
4. **ESP32** memiliki 2 I2C controller dengan flexible GPIO mapping
5. **Sensor populer**: BME280 (environment), DS3231 (RTC), SSD1306 (OLED)
6. **Pull-up resistors** penting untuk integritas sinyal
7. **Bus recovery** diperlukan untuk menangani kondisi error

---

## 📊 Diagram Perbandingan Platform

```
┌────────────────────────────────────────────────────────────────┐
│                    I2C Comparison                               │
├──────────────────────┬─────────────────────┬───────────────────┤
│      Feature         │     STM32F103       │      ESP32        │
├──────────────────────┼─────────────────────┼───────────────────┤
│ I2C Peripherals      │         2           │        2          │
│ Default Pins         │   PB6/7, PB10/11    │   GPIO21/22       │
│ Pin Remapping        │    Limited          │    Full flexible  │
│ Max Speed            │    400kHz (std)     │    1MHz           │
│ DMA Support          │        Yes          │       Yes         │
│ Clock Stretching     │        Yes          │       Yes         │
│ Multi-Master         │        Yes          │       Yes         │
│ Internal Pull-up     │        No           │       Yes         │
└──────────────────────┴─────────────────────┴───────────────────┘
```

---

## 📖 Referensi

1. I2C-bus Specification and User Manual (NXP UM10204)
2. STM32F103 Reference Manual (RM0008) - Chapter 26: I2C
3. ESP32 Technical Reference Manual - Chapter 11: I2C Controller
4. BME280 Datasheet (Bosch)
5. SSD1306 Datasheet (Solomon Systech)
6. DS3231 Datasheet (Maxim Integrated)
7. "Mastering STM32" by Carmine Noviello - Chapter 13: I2C



-----------------------------------------------------------
--- PPT_Prompts_1.md ---
-----------------------------------------------------------

# Prompt untuk Pembuatan PPT - Bagian 1
## Modul 06: I2C Bus dan Sensor Integration

### Instruksi Umum untuk AI Image Generator
- Style: Modern, professional, technical illustration
- Color scheme: Blue (#0066CC), Green (#00AA55), White background
- Resolution: 1920x1080 (16:9 aspect ratio)
- Font style: Clean sans-serif (Roboto, Open Sans)

---

## SLIDE 1 - Judul Utama

**Prompt:**
```
Create a professional presentation title slide with:
- Main title "I2C Bus dan Sensor Integration" in large bold blue text
- Subtitle "Modul 06 - Praktikum Sistem Embedded" 
- Background: Modern circuit board pattern with I2C data lines highlighted
- Visual elements: BME280, OLED SSD1306, DS3231 RTC, EEPROM chip icons
- Two microcontroller boards: STM32 Blue Pill and ESP32 DevKit
- I2C bus connections shown as two parallel lines (SDA, SCL) connecting all devices
- University/institution logo placeholder in corner
- Clean, professional, educational style
```

---

## SLIDE 2 - Capaian Pembelajaran

**Prompt:**
```
Create an educational slide showing learning objectives with:
- Title "Capaian Pembelajaran" at top
- 7 numbered objectives with checkmark icons:
  1. Memahami prinsip kerja protokol I2C
  2. Mengidentifikasi sinyal SDA dan SCL
  3. Mengkonfigurasi I2C sebagai Master
  4. Membaca data dari sensor I2C (BME280, DS3231, OLED)
  5. Mengakses EEPROM eksternal
  6. Menangani multiple device pada I2C bus
  7. Troubleshooting komunikasi I2C
- Background: Soft gradient with subtle circuit patterns
- Icons representing each objective (sensor, clock, display, memory)
```

---

## SLIDE 3 - Apa itu I2C?

**Prompt:**
```
Create an infographic explaining I2C protocol:
- Title "Inter-Integrated Circuit (I2C)"
- Developed by Philips (NXP) in 1982
- Key features in visual boxes:
  • 2-wire communication (SDA + SCL)
  • Multi-master, multi-slave capable
  • 7-bit addressing (127 devices)
  • Speed: up to 3.4 Mbps
- Simple diagram showing two wires connecting multiple devices
- Historical timeline element
- Philips/NXP logo reference
```

---

## SLIDE 4 - I2C vs SPI vs UART

**Prompt:**
```
Create a comparison table slide with:
- Title "Perbandingan Protokol Serial"
- Three columns: I2C, SPI, UART
- Rows comparing:
  • Number of wires (2, 4+, 2)
  • Topology (Bus, Point-to-point, P2P)
  • Max devices (127, unlimited, 1)
  • Duplex (Half, Full, Full)
  • Max speed (3.4M, 10M+, 1M)
  • Complexity (Medium, Low, Low)
- Visual icons for each protocol
- Highlight I2C column with accent color
- Use checkmarks and X marks for pros/cons
```

---

## SLIDE 5 - Arsitektur I2C Bus

**Prompt:**
```
Create a detailed I2C bus architecture diagram:
- Title "Arsitektur I2C Bus"
- Show Vcc power rail at top (3.3V)
- Two pull-up resistors (4.7kΩ labeled) connecting to SDA and SCL lines
- Horizontal bus lines for SDA and SCL
- Connected devices:
  • Master (MCU) - highlighted
  • Slave 1 (Sensor)
  • Slave 2 (EEPROM)
  • Slave 3 (RTC)
- Ground rail at bottom
- Open-drain output symbol
- Color coding: SDA=green, SCL=blue
- Labels for all components
```

---

## SLIDE 6 - Sinyal I2C

**Prompt:**
```
Create a slide explaining I2C signals:
- Title "Sinyal SDA dan SCL"
- Two sections:
  1. SDA (Serial Data):
     - Bidirectional data line
     - Open-drain output
     - Data valid when SCL HIGH
  2. SCL (Serial Clock):
     - Clock from Master
     - Open-drain output
     - Controls timing
- Simple waveform showing both signals
- Pull-up resistor symbol
- Bidirectional arrow for SDA
- Unidirectional arrow for SCL
```

---

## SLIDE 7 - I2C Timing Diagram

**Prompt:**
```
Create a detailed I2C timing diagram showing:
- Title "I2C Timing Diagram"
- Three sections:
  1. START Condition: SDA goes LOW while SCL is HIGH
  2. Data Transfer: Multiple bits with setup/hold times labeled
  3. STOP Condition: SDA goes HIGH while SCL is HIGH
- Clear labeling of:
  • Setup time
  • Hold time
  • Data valid region
- SDA and SCL as two separate waveforms
- Annotations with arrows pointing to key transitions
- Clean, technical drawing style
```

---

## SLIDE 8 - I2C Frame Format

**Prompt:**
```
Create a slide showing I2C frame structure:
- Title "Format Frame I2C (7-bit Address)"
- Visual frame diagram with boxes:
  | START | 7-bit Address | R/W | ACK | 8-bit Data | ACK | STOP |
  |   1   |  A6...A0      |  1  |  1  |   D7...D0  |  1  |   1  |
- Color coding for each field type
- R/W bit explanation: 0=Write, 1=Read
- ACK/NACK explanation with SDA LOW/HIGH
- Bit counting for each field
- Total bits calculation
```

---

## SLIDE 9 - I2C Speed Modes

**Prompt:**
```
Create an infographic showing I2C speed modes:
- Title "Mode Kecepatan I2C"
- Horizontal speed scale/meter visualization
- Four modes with icons:
  1. Standard Mode: 100 kbps (Sensors, EEPROM)
  2. Fast Mode: 400 kbps (Display, IMU)
  3. Fast Mode Plus: 1 Mbps (High-speed sensors)
  4. High Speed: 3.4 Mbps (Special applications)
- Application examples for each mode
- Speed comparison bar chart
- Color gradient from slow (cool) to fast (warm)
```

---

## SLIDE 10 - I2C pada STM32F103

**Prompt:**
```
Create a technical slide for STM32 I2C:
- Title "I2C pada STM32F103"
- STM32F103 chip diagram/icon
- Features list with icons:
  • 2 I2C peripherals (I2C1, I2C2)
  • 7-bit and 10-bit addressing
  • Multi-master capable
  • DMA support
  • SMBus compatible
- Pin mapping table:
  | Peripheral | SCL | SDA | Remap SCL | Remap SDA |
  | I2C1       | PB6 | PB7 | PB8       | PB9       |
  | I2C2       | PB10| PB11| -         | -         |
- STM32 Blue Pill board image reference
```

---

## SLIDE 11 - I2C pada ESP32

**Prompt:**
```
Create a technical slide for ESP32 I2C:
- Title "I2C pada ESP32"
- ESP32 chip/board diagram
- Features list with icons:
  • 2 I2C controllers (I2C_NUM_0, I2C_NUM_1)
  • Flexible GPIO mapping (any GPIO)
  • Master and Slave mode
  • Internal pull-up support
  • Clock stretching
- Default pins highlight: SDA=GPIO21, SCL=GPIO22
- ESP32 DevKit pinout reference
- Comparison note with STM32
```

---

## SLIDE 12 - BME280 Sensor Overview

**Prompt:**
```
Create a product overview slide for BME280:
- Title "BME280 - Environmental Sensor"
- BME280 module photo/illustration
- Specification boxes:
  • Temperature: -40°C to +85°C, ±1.0°C
  • Humidity: 0-100% RH, ±3% RH
  • Pressure: 300-1100 hPa, ±1 hPa
- I2C Address: 0x76 or 0x77
- Applications icons: Weather station, HVAC, IoT
- Pinout diagram of BME280 module
- Bosch logo reference
```

---

## SLIDE 13 - BME280 Register Map

**Prompt:**
```
Create a technical slide showing BME280 registers:
- Title "BME280 Register Map"
- Table format:
  | Register | Address | Description |
  | ID       | 0xD0    | Chip ID (0x60) |
  | ctrl_hum | 0xF2    | Humidity control |
  | ctrl_meas| 0xF4    | Temp & Press control |
  | config   | 0xF5    | Rate, filter, interface |
  | Data     | 0xF7-0xFE | Measurement data |
- Memory map visualization
- Highlighting data registers
- Read/Write indicators
```

---

## SLIDE 14 - SSD1306 OLED Display

**Prompt:**
```
Create a product overview slide for SSD1306:
- Title "SSD1306 - OLED Display"
- OLED display module photo showing 128x64 pixels
- Specifications:
  • Resolution: 128×64 pixels
  • Type: Monochrome OLED
  • I2C Address: 0x3C or 0x3D
  • Voltage: 3.3V or 5V
- Display showing sample graphics
- Pinout diagram
- Solomon Systech logo reference
- Application examples: IoT dashboard, status display
```

---

## SLIDE 15 - DS3231 Real-Time Clock

**Prompt:**
```
Create a product overview slide for DS3231:
- Title "DS3231 - Real-Time Clock"
- DS3231 module photo with battery
- Key features:
  • Accuracy: ±2ppm (±1 min/year)
  • Battery backup (CR2032)
  • Temperature compensated crystal
  • I2C Address: 0x68 (fixed)
- Register overview for time keeping
- Module pinout
- Maxim Integrated logo
- Applications: Data logging, scheduling, timestamps
```

---

## SLIDE 16 - 24LC256 EEPROM

**Prompt:**
```
Create a product overview slide for 24LC256:
- Title "24LC256 - I2C EEPROM"
- EEPROM chip DIP package illustration
- Specifications:
  • Capacity: 256 Kbit (32KB)
  • Page size: 64 bytes
  • I2C Address: 0x50-0x57
  • Write cycle: 5ms max
- Address configuration with A0, A1, A2 pins
- Memory organization diagram
- Microchip logo
- Applications: Configuration storage, data logging
```

---

## SLIDE 17 - Konfigurasi I2C STM32

**Prompt:**
```
Create a code walkthrough slide:
- Title "Konfigurasi I2C STM32 (HAL)"
- Code snippet with syntax highlighting:
  I2C_HandleTypeDef hi2c1;
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  HAL_I2C_Init(&hi2c1);
- Key parameters highlighted with arrows
- GPIO configuration note (PB6, PB7 as AF_OD)
- Visual flow diagram showing init sequence
```

---

## SLIDE 18 - Konfigurasi I2C ESP32

**Prompt:**
```
Create a code walkthrough slide:
- Title "Konfigurasi I2C ESP32 (Arduino)"
- Two approaches:
  1. Simple (Wire library):
     Wire.begin(21, 22);
     Wire.setClock(400000);
  2. Advanced (ESP-IDF):
     i2c_config_t conf;
     i2c_param_config(I2C_NUM_0, &conf);
     i2c_driver_install();
- Flexible GPIO mapping highlight
- Internal pull-up option
- Comparison with STM32 approach
```

---

## SLIDE 19 - I2C Bus Scanning

**Prompt:**
```
Create a slide about I2C bus scanning:
- Title "I2C Bus Scanner"
- Purpose: Detect all connected devices
- Scanning process visualization:
  1. Send address (0x03 to 0x77)
  2. Check for ACK
  3. List found devices
- Sample output grid showing addresses
- Common addresses highlighted with device names
- Both STM32 and ESP32 code snippets
- Troubleshooting tips
```

---

## SLIDE 20 - Pull-up Resistor Design

**Prompt:**
```
Create a technical slide about pull-up resistors:
- Title "Perhitungan Pull-up Resistor"
- Formula diagram:
  Rp_min = (Vcc - Vol) / Iol
  Rp_max = tr / (0.8473 × Cb)
- Variables explanation with icons
- Recommended values table:
  | Mode | Bus Cap | Rp Value |
  | Standard | <100pF | 4.7kΩ |
  | Standard | <200pF | 2.2kΩ |
  | Fast | <100pF | 2.2kΩ |
- Visual showing resistor placement in circuit
- Rule of thumb: 4.7kΩ for most applications
```

---

## SLIDE 21 - Multi-Device I2C

**Prompt:**
```
Create a slide showing multi-device I2C setup:
- Title "Multi-Device pada I2C Bus"
- Bus diagram with 4 devices:
  • BME280 (0x76)
  • SSD1306 OLED (0x3C)
  • DS3231 RTC (0x68)
  • 24LC256 EEPROM (0x50)
- Single SDA/SCL lines connecting all
- Address labels for each device
- Data flow arrows
- Conflict resolution note
- Practical wiring tips
```

---

## SLIDE 22 - I2C Read Operation

**Prompt:**
```
Create a timing diagram for I2C read:
- Title "Operasi Read I2C"
- Complete read sequence:
  1. START
  2. Device Address + Write
  3. Register Address
  4. Repeated START
  5. Device Address + Read
  6. Data byte(s)
  7. NACK + STOP
- Both master and slave perspectives
- ACK/NACK indicators
- Data direction arrows
- Typical use case: Reading sensor data
```

---

## SLIDE 23 - I2C Write Operation

**Prompt:**
```
Create a timing diagram for I2C write:
- Title "Operasi Write I2C"
- Complete write sequence:
  1. START
  2. Device Address + Write
  3. Register Address
  4. Data byte(s)
  5. STOP
- Master to slave data flow
- ACK after each byte
- Typical use case: Configuration register
- Page write for EEPROM
```

---

## SLIDE 24 - Troubleshooting I2C

**Prompt:**
```
Create a troubleshooting guide slide:
- Title "Troubleshooting I2C"
- Common problems table:
  | Problem | Symptom | Solution |
  | SDA stuck | No comm | Bus recovery |
  | No ACK | NACK | Check address/power |
  | Timeout | Hangs | Increase timeout |
  | Data corruption | Wrong data | Check pull-ups |
- Visual checklist icons
- Logic analyzer screenshot example
- Debug code snippet
- Decision flowchart
```

---

## SLIDE 25 - I2C Bus Recovery

**Prompt:**
```
Create a slide about bus recovery procedure:
- Title "I2C Bus Recovery"
- Problem: SDA stuck LOW
- Recovery algorithm visualization:
  1. Send 9 clock pulses on SCL
  2. Check if SDA released
  3. Generate STOP condition
  4. Reinitialize I2C
- Code snippet for recovery
- Before/after waveforms
- When to use recovery
- Prevention tips
```

---

## SLIDE 26 - Best Practices

**Prompt:**
```
Create a best practices slide:
- Title "Best Practices I2C"
- Hardware section:
  • Use appropriate pull-up resistors
  • Keep traces short (<30cm)
  • Add decoupling capacitors
  • Consider separate buses
- Software section:
  • Always check return values
  • Implement timeout handling
  • Use bus recovery
  • Validate with CRC
- Icons for each practice
- Do's and Don'ts format
```

---

## SLIDE 27 - Praktikum Overview

**Prompt:**
```
Create a practicum overview slide:
- Title "Praktikum I2C Sensor"
- 12 programs for each platform (STM32 & ESP32)
- Program list with icons:
  1-2. I2C Bus Scanner
  3-4. BME280 Sensor Reading
  5-6. SSD1306 OLED Display
  7-8. DS3231 RTC Operations
  9-10. 24LC256 EEPROM
  11-12. Multi-Device Integration
- Hardware requirements summary
- Time allocation
- Learning progression diagram
```

---

### Catatan untuk Pembuat PPT:

1. **Konsistensi Visual**: Gunakan template yang sama untuk semua slide
2. **Animasi**: Tambahkan animasi sederhana untuk diagram timing
3. **Code Highlighting**: Gunakan syntax highlighting untuk semua code
4. **Waveforms**: Animasikan sinyal I2C untuk menunjukkan komunikasi
5. **Interactive Elements**: Tambahkan hyperlink untuk navigasi antar slide
6. **Font Size**: Minimum 24pt untuk body text, 36pt untuk judul
7. **Color Coding**: Konsisten gunakan warna untuk SDA (hijau) dan SCL (biru)


-----------------------------------------------------------
--- PPT_Prompts_2.md ---
-----------------------------------------------------------

# Prompt untuk Pembuatan PPT - Bagian 2
## Modul 06: I2C Bus dan Sensor Integration (Slide 28-50)

### Instruksi Lanjutan
- Melanjutkan dari PPT_Prompts_1.md (Slide 1-27)
- Style dan color scheme tetap konsisten
- Focus pada aplikasi praktis dan advanced topics

---

## SLIDE 28 - Demo: I2C Scanner

**Prompt:**
```
Create a demonstration slide for I2C Scanner:
- Title "Demo: I2C Bus Scanner"
- Split screen layout:
  Left: Code snippet (highlighted)
  Right: Serial monitor output
- Sample output showing:
     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
  00:          -- -- -- -- -- -- -- -- -- -- -- -- --
  30: -- -- -- -- -- -- -- -- -- -- -- -- 3C -- -- --
  50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
  70: -- -- -- -- -- -- 76 --
- Device identification legend
- Both STM32 and ESP32 versions shown
```

---

## SLIDE 29 - Demo: BME280 Reading

**Prompt:**
```
Create a demonstration slide for BME280:
- Title "Demo: Pembacaan BME280"
- Real sensor data display:
  • Temperature: 25.43 °C
  • Humidity: 65.21 %RH
  • Pressure: 1013.25 hPa
  • Altitude: 0.00 m
- Graph showing data over time
- Calibration process visualization
- Oversampling configuration
- Heat index and dew point calculations
```

---

## SLIDE 30 - Demo: OLED Display

**Prompt:**
```
Create a demonstration slide for SSD1306:
- Title "Demo: SSD1306 OLED Display"
- OLED screen mockups showing:
  1. Text display "Hello World"
  2. Graphics (lines, circles, rectangles)
  3. Animated bouncing ball
  4. Progress bar
  5. Scrolling text
- Display buffer concept
- Resolution 128x64 pixels grid
- Font rendering example
```

---

## SLIDE 31 - Demo: DS3231 RTC

**Prompt:**
```
Create a demonstration slide for DS3231:
- Title "Demo: DS3231 RTC"
- Clock display showing current time
- Features demonstrated:
  • Date/time reading
  • Temperature from RTC
  • Alarm setting
  • Battery backup indication
- BCD format explanation
- Register read/write visualization
- Timestamp format examples
```

---

## SLIDE 32 - Demo: EEPROM Operations

**Prompt:**
```
Create a demonstration slide for EEPROM:
- Title "Demo: 24LC256 EEPROM"
- Memory operations visualization:
  • Byte write/read
  • Page write (64 bytes)
  • String storage
- Memory dump output format
- Write cycle timing (5ms)
- Address calculation example
- Wear leveling concept introduction
```

---

## SLIDE 33 - Demo: Multi-Device System

**Prompt:**
```
Create a demonstration slide for integrated system:
- Title "Demo: Multi-Device Integration"
- System diagram showing all devices connected
- OLED displaying:
  • Time from RTC (top)
  • Temperature/Humidity from BME280 (middle)
  • Data logging status (bottom)
- EEPROM storing sensor data with timestamps
- Real-time update visualization
- Code architecture overview
```

---

## SLIDE 34 - Aplikasi: Weather Station

**Prompt:**
```
Create an application slide:
- Title "Aplikasi: Weather Station"
- Complete weather station design:
  • BME280 for environmental data
  • DS3231 for timestamps
  • SSD1306 for local display
  • EEPROM for data logging
- Block diagram
- Data flow visualization
- Sample user interface on OLED
- 24-hour history graph
```

---

## SLIDE 35 - Aplikasi: Data Logger

**Prompt:**
```
Create an application slide:
- Title "Aplikasi: Data Logger"
- Data logging system:
  • Sensor readings at intervals
  • RTC timestamps
  • EEPROM storage
  • Wear leveling algorithm
- Memory management visualization
- Circular buffer concept
- Data retrieval process
- Export format (CSV)
```

---

## SLIDE 36 - Aplikasi: Smart Home Display

**Prompt:**
```
Create an application slide:
- Title "Aplikasi: Smart Home Display"
- Home dashboard on OLED:
  • Time and date
  • Indoor temperature/humidity
  • Weather icons
  • Alert notifications
- Multiple rooms data concept
- Periodic screen refresh
- Power saving modes
- User interaction buttons
```

---

## SLIDE 37 - Clock Stretching

**Prompt:**
```
Create a technical slide about clock stretching:
- Title "Clock Stretching"
- Definition and purpose
- Timing diagram showing:
  • Master sends clock
  • Slave holds SCL LOW
  • Master waits
  • Slave releases SCL
  • Communication continues
- Use cases:
  • Slow slave processing
  • EEPROM write cycles
- Timeout considerations
- STM32 vs ESP32 handling
```

---

## SLIDE 38 - Bus Arbitration

**Prompt:**
```
Create a technical slide about bus arbitration:
- Title "Arbitration Multi-Master"
- Multi-master scenario diagram
- Arbitration process:
  1. Both masters start simultaneously
  2. Monitor SDA line
  3. Loser detects conflict
  4. Winner continues
- Timing diagram with conflict
- Wired-AND logic explanation
- Practical considerations
- When to use multi-master
```

---

## SLIDE 39 - 10-bit Addressing

**Prompt:**
```
Create a technical slide about 10-bit addressing:
- Title "10-bit I2C Addressing"
- Address format comparison:
  • 7-bit: 127 devices
  • 10-bit: 1024 devices
- Frame format diagram:
  | 1111 0XX | R/W | ACK | XX XXXX XX | ACK |
- Reserved addresses explanation
- When to use 10-bit
- Compatibility with 7-bit devices
- Code implementation notes
```

---

## SLIDE 40 - I2C Level Shifting

**Prompt:**
```
Create a technical slide about level shifting:
- Title "Level Shifting I2C"
- Problem: 3.3V and 5V devices on same bus
- Solutions:
  1. MOSFET-based bidirectional
  2. Dedicated level shifter IC
  3. Resistor voltage divider (limited)
- Circuit diagrams for each method
- Component recommendations
- Timing considerations
- Best practices
```

---

## SLIDE 41 - I2C DMA Transfer

**Prompt:**
```
Create a technical slide about DMA with I2C:
- Title "I2C dengan DMA"
- Benefits:
  • CPU offloading
  • Continuous data transfer
  • Efficient for large data
- DMA configuration diagram
- STM32 DMA channels for I2C
- ESP32 DMA considerations
- Performance comparison graph
- Use cases: Display updates, sensor arrays
```

---

## SLIDE 42 - I2C Interrupt Handling

**Prompt:**
```
Create a technical slide about interrupts:
- Title "I2C Interrupt Handling"
- Interrupt events:
  • Transfer complete
  • Error detected
  • Address match (slave)
  • NACK received
- Interrupt service routine flow
- Priority configuration
- State machine diagram
- Non-blocking I2C operations
- Example code structure
```

---

## SLIDE 43 - Error Handling Strategies

**Prompt:**
```
Create a slide about error handling:
- Title "Strategi Error Handling"
- Error types and responses:
  | Error | Detection | Recovery |
  | NACK | ACK bit | Retry/abort |
  | Timeout | Timer | Bus recovery |
  | Bus busy | Status | Wait/reset |
  | Arbitration lost | SDA | Restart |
- Retry mechanism flowchart
- Logging and diagnostics
- Graceful degradation
```

---

## SLIDE 44 - Performance Optimization

**Prompt:**
```
Create a slide about I2C optimization:
- Title "Optimasi Performa I2C"
- Optimization techniques:
  • Use Fast Mode (400kHz) when possible
  • Batch multiple reads
  • Reduce transactions
  • Use DMA for large transfers
  • Optimize pull-up resistors
- Benchmark comparison chart
- Power consumption considerations
- Code optimization tips
```

---

## SLIDE 45 - Project Preview

**Prompt:**
```
Create a project preview slide:
- Title "Project: Smart Environmental Monitor"
- Dual-MCU system:
  • STM32: Sensor hub (BME280, more sensors)
  • ESP32: Display & connectivity
- Communication: I2C between MCUs
- Features list:
  • Multi-point sensing
  • Data logging with timestamps
  • OLED dashboard
  • Alert system
- System architecture diagram
- Challenge levels indicated
```

---

## SLIDE 46 - Perbandingan STM32 vs ESP32 I2C

**Prompt:**
```
Create a comparison slide:
- Title "STM32 vs ESP32: I2C Implementation"
- Side-by-side comparison:
  | Feature | STM32F103 | ESP32 |
  | Peripherals | 2 | 2 |
  | Default pins | Fixed | Flexible |
  | Max speed | 400kHz | 1MHz |
  | Internal pull-up | No | Yes |
  | DMA support | Yes | Yes |
- Pros and cons for each
- When to choose which
- Code complexity comparison
```

---

## SLIDE 47 - Common Mistakes

**Prompt:**
```
Create a slide about common mistakes:
- Title "Kesalahan Umum I2C"
- Mistake cards with solutions:
  1. Missing pull-up resistors
  2. Wrong address (not shifted)
  3. Incorrect voltage levels
  4. Too long bus wires
  5. Not checking return values
  6. Blocking code with no timeout
  7. Address conflicts
- Visual icons showing X for mistake, ✓ for solution
- Code examples of correct vs incorrect
```

---

## SLIDE 48 - Resources & Tools

**Prompt:**
```
Create a resources slide:
- Title "Sumber Belajar & Tools"
- Documentation links:
  • NXP I2C Specification
  • Datasheets
  • HAL documentation
- Tools:
  • Logic analyzer for debugging
  • I2C scanner utility
  • Protocol decoder
- Libraries:
  • Wire.h (Arduino)
  • HAL I2C (STM32)
  • ESP-IDF I2C
- Online simulators
```

---

## SLIDE 49 - Rangkuman

**Prompt:**
```
Create a summary slide:
- Title "Rangkuman"
- Key points with icons:
  1. I2C: 2-wire protocol (SDA, SCL)
  2. 7-bit addressing: 127 devices
  3. STM32: 2 I2C, HAL library
  4. ESP32: 2 I2C, flexible GPIO
  5. Pull-up resistors essential
  6. Multiple sensors on one bus
  7. Error handling critical
- Visual recap of the module
- Connection to next module preview
```

---

## SLIDE 50 - Penutup & Tugas

**Prompt:**
```
Create a closing slide:
- Title "Tugas & Evaluasi"
- Praktikum tasks:
  1. Complete all 12 programs (STM32 & ESP32)
  2. Document results with screenshots
  3. Answer analysis questions
- Project requirements:
  • Smart Environmental Monitor
  • Both MCUs required
  • Video demonstration
- Evaluation criteria summary
- Deadline information
- Contact for questions
- "Terima Kasih" closing
```

---

### Catatan Tambahan untuk Bagian 2:

1. **Demo Slides**: Sertakan screenshot asli atau mockup realistis
2. **Application Slides**: Tunjukkan use case nyata yang relevan
3. **Technical Slides**: Pastikan diagram timing akurat
4. **Code Examples**: Highlight bagian penting dari kode
5. **Comparisons**: Gunakan format tabel untuk kemudahan pemahaman
6. **Summary**: Recap poin utama dengan visual yang memorable

### Integrasi dengan Praktikum:

- Setiap demo slide harus sesuai dengan program di Jobsheet
- Gunakan hasil output yang konsisten dengan kode
- Referensikan nomor program untuk kemudahan navigasi
- Sertakan QR code untuk akses cepat ke repository kode


-----------------------------------------------------------
--- Project.md ---
-----------------------------------------------------------

# Project Modul 06: Smart Environmental Monitor dengan Dual-MCU I2C

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Mata Kuliah** | Praktikum Sistem Embedded |
| **Modul** | 06 - I2C Bus dan Sensor Integration |
| **Tingkat Kesulitan** | ⭐⭐⭐⭐ (Advanced) |
| **Waktu Pengerjaan** | 2-3 minggu |
| **Platform** | STM32F103C8T6 + ESP32 DevKit V1 |

---

## 🎯 Deskripsi Project

Membangun **Smart Environmental Monitor** yang menggunakan arsitektur **Dual-MCU** dimana:

- **STM32F103** berperan sebagai **Sensor Hub** yang mengumpulkan data dari multiple sensor I2C
- **ESP32** berperan sebagai **Display Controller** yang menampilkan data pada OLED dan menyediakan user interface

Kedua MCU berkomunikasi melalui **I2C Bus** dimana ESP32 sebagai Master dan STM32 sebagai Slave, membentuk sistem multi-master capable dengan sensor I2C lainnya.

---

## 🎓 Tujuan Pembelajaran

Setelah menyelesaikan project ini, mahasiswa mampu:

1. Mengimplementasikan komunikasi I2C multi-device kompleks
2. Mengkonfigurasi MCU sebagai I2C Slave (STM32)
3. Menangani multiple I2C bus pada satu sistem
4. Mengintegrasikan berbagai sensor I2C (BME280, DS3231, dsb)
5. Membuat real-time data logging dengan timestamps
6. Mendesain user interface pada OLED display
7. Menerapkan error handling dan recovery pada I2C

---

## 📐 Arsitektur Sistem

### Diagram Blok

```
                    ┌─────────────────────────────────────────────────────────┐
                    │                  SMART ENVIRONMENTAL MONITOR             │
                    └─────────────────────────────────────────────────────────┘
                    
    ┌──────────────────────────────┐         ┌──────────────────────────────┐
    │       STM32F103C8T6          │         │        ESP32 DevKit          │
    │        (Sensor Hub)          │         │    (Display Controller)      │
    │                              │   I2C   │                              │
    │  ┌─────────────────────┐    │◄───────►│    ┌─────────────────────┐   │
    │  │   I2C1 (Master)     │    │  Slave  │    │   I2C0 (Master)     │   │
    │  │   - BME280 (0x76)   │    │  0x08   │    │   - SSD1306 (0x3C)  │   │
    │  │   - DS3231 (0x68)   │    │         │    │   - STM32 (0x08)    │   │
    │  │   - AHT20 (0x38)    │    │         │    └─────────────────────┘   │
    │  └─────────────────────┘    │         │                              │
    │                              │         │    ┌─────────────────────┐   │
    │  ┌─────────────────────┐    │         │    │   I2C1 (Master)     │   │
    │  │   I2C2 (Slave)      │    │         │    │   - 24LC256 (0x50)  │   │
    │  │   Address: 0x08     │────┼─────────┼───►│                     │   │
    │  └─────────────────────┘    │         │    └─────────────────────┘   │
    │                              │         │                              │
    │  ┌─────────────────────┐    │         │    ┌─────────────────────┐   │
    │  │   GPIO              │    │         │    │   GPIO              │   │
    │  │   - Status LED (PC13)│    │         │    │   - Button (GPIO0)  │   │
    │  │   - Alert LED (PA0)  │    │         │    │   - Buzzer (GPIO25) │   │
    │  └─────────────────────┘    │         │    └─────────────────────┘   │
    └──────────────────────────────┘         └──────────────────────────────┘
```

### Koneksi I2C

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                              I2C BUS TOPOLOGY                                 │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│    STM32 I2C1 Bus (PB6=SCL, PB7=SDA) - Sensor Bus                           │
│    ─────────────────────────────────────────────                            │
│         │         │         │         │                                      │
│     ┌───┴───┐ ┌───┴───┐ ┌───┴───┐ ┌───┴───┐                                │
│     │BME280 │ │DS3231 │ │ AHT20 │ │ BH1750│                                │
│     │ 0x76  │ │ 0x68  │ │ 0x38  │ │ 0x23  │                                │
│     └───────┘ └───────┘ └───────┘ └───────┘                                │
│                                                                              │
│    ESP32 I2C0 Bus (GPIO22=SCL, GPIO21=SDA) - Main Bus                       │
│    ─────────────────────────────────────────────                            │
│         │         │         │                                                │
│     ┌───┴───┐ ┌───┴───┐ ┌───┴───┐                                          │
│     │SSD1306│ │STM32  │ │24LC256│                                          │
│     │ 0x3C  │ │ 0x08  │ │ 0x50  │                                          │
│     └───────┘ └─Slave─┘ └───────┘                                          │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔧 Hardware Requirements

### Komponen Utama

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | Sensor Hub |
| 2 | ESP32 DevKit V1 | 1 | Display Controller |
| 3 | BME280 Module | 1 | Temp/Hum/Press sensor |
| 4 | DS3231 RTC Module | 1 | Real-time clock |
| 5 | SSD1306 OLED 128×64 | 1 | Display |
| 6 | 24LC256 EEPROM | 1 | Data storage |
| 7 | Resistor 4.7kΩ | 4 | I2C pull-ups |
| 8 | Resistor 10kΩ | 2 | Button pull-ups |
| 9 | LED 5mm (2 warna) | 2 | Status indicators |
| 10 | Push Button | 2 | User input |
| 11 | Buzzer 5V | 1 | Audio alert |
| 12 | Breadboard | 2 | Prototyping |
| 13 | Jumper Wires | ~50 | Connections |

### Komponen Opsional (Bonus)

| No | Komponen | Keterangan |
|----|----------|------------|
| 1 | AHT20/AHT21 | Additional temp/hum sensor |
| 2 | BH1750 | Light intensity sensor |
| 3 | MQ-135 | Air quality (via ADC) |
| 4 | PCF8574 | I2C GPIO expander |

---

## 📝 Spesifikasi Fungsional

### 1. Sensor Hub (STM32)

**Fungsi Utama:**
- Membaca data dari semua sensor I2C setiap 1 detik
- Menyimpan data dalam buffer internal
- Merespon request dari ESP32 Master
- Mengirim data dalam format terstruktur

**Data yang Dikumpulkan:**
```c
typedef struct {
    uint32_t timestamp;      // Unix timestamp dari DS3231
    float temperature;       // °C dari BME280
    float humidity;          // %RH dari BME280
    float pressure;          // hPa dari BME280
    float altitude;          // m (calculated)
    uint8_t status;          // Sensor status flags
    uint16_t checksum;       // Data integrity
} SensorData_t;
```

**I2C Slave Protocol:**
| Register | Address | Description | Size |
|----------|---------|-------------|------|
| STATUS | 0x00 | Sensor hub status | 1 byte |
| TIMESTAMP | 0x01 | Current timestamp | 4 bytes |
| TEMPERATURE | 0x05 | Temperature value | 4 bytes |
| HUMIDITY | 0x09 | Humidity value | 4 bytes |
| PRESSURE | 0x0D | Pressure value | 4 bytes |
| ALTITUDE | 0x11 | Altitude value | 4 bytes |
| ALL_DATA | 0x20 | Complete struct | 18 bytes |
| CONFIG | 0x30 | Configuration | 4 bytes |

### 2. Display Controller (ESP32)

**Fungsi Utama:**
- Request data dari STM32 secara periodik
- Menampilkan data pada OLED dengan UI yang informatif
- Menyimpan data ke EEPROM dengan timestamps
- Menangani user input (button)
- Alert system (buzzer, LED)

**Display Screens:**
1. **Main Dashboard** - Current readings
2. **History Graph** - Temperature trend
3. **Statistics** - Min/Max/Average
4. **Settings** - Threshold configuration
5. **Data Log** - Stored records

**User Interface State Machine:**
```
                    ┌─────────────┐
                    │   STARTUP   │
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
            ┌──────►│  DASHBOARD  │◄──────┐
            │       └──────┬──────┘       │
            │              │              │
    ┌───────┴──────┐   Button    ┌───────┴──────┐
    │   HISTORY    │◄──────────►│   SETTINGS   │
    └───────┬──────┘             └───────┬──────┘
            │                            │
    ┌───────▼──────┐             ┌───────▼──────┐
    │  STATISTICS  │             │   DATA LOG   │
    └──────────────┘             └──────────────┘
```

### 3. Data Logging

**Storage Format (EEPROM):**
```c
// Header at address 0x0000
typedef struct {
    uint32_t magic;          // 0xDEADBEEF
    uint16_t version;        // Format version
    uint16_t record_count;   // Number of records
    uint16_t head_ptr;       // Circular buffer head
    uint16_t tail_ptr;       // Circular buffer tail
} EEPROM_Header_t;

// Records start at address 0x0020
typedef struct {
    uint32_t timestamp;
    int16_t temperature;     // °C × 100
    uint16_t humidity;       // %RH × 100
    uint16_t pressure;       // (hPa - 900) × 10
} EEPROM_Record_t;          // 10 bytes per record
```

**Capacity:**
- EEPROM: 32KB (24LC256)
- Header: 32 bytes
- Per record: 10 bytes
- Max records: ~3,200 records
- At 1 minute interval: ~53 hours of data

### 4. Alert System

**Configurable Thresholds:**
| Parameter | Default Min | Default Max | Alert Type |
|-----------|-------------|-------------|------------|
| Temperature | 15°C | 35°C | Buzzer + LED |
| Humidity | 30% | 80% | LED only |
| Pressure | 980 hPa | 1030 hPa | Log only |

---

## 💻 Implementasi Kode

### STM32 - Sensor Hub (I2C Slave)

**File: `stm32_sensor_hub/src/main.c`**

```c
/**
 * @file main.c
 * @brief STM32 Sensor Hub - I2C Slave with Multiple Sensors
 */

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

/* I2C Addresses */
#define I2C_SLAVE_ADDR      0x08
#define BME280_ADDR         0x76
#define DS3231_ADDR         0x68

/* Register Map */
#define REG_STATUS          0x00
#define REG_TIMESTAMP       0x01
#define REG_TEMPERATURE     0x05
#define REG_HUMIDITY        0x09
#define REG_PRESSURE        0x0D
#define REG_ALTITUDE        0x11
#define REG_ALL_DATA        0x20
#define REG_CONFIG          0x30

/* Handles */
I2C_HandleTypeDef hi2c1;  // Master - Sensors
I2C_HandleTypeDef hi2c2;  // Slave - ESP32
UART_HandleTypeDef huart1;

/* Sensor Data Structure */
typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    float temperature;
    float humidity;
    float pressure;
    float altitude;
    uint8_t status;
    uint16_t checksum;
} SensorData_t;

volatile SensorData_t sensor_data;
volatile uint8_t i2c_register = 0;
volatile uint8_t data_ready = 0;

/* BME280 Functions (simplified) */
extern uint8_t BME280_Init(void);
extern void BME280_ReadData(float* temp, float* hum, float* press);

/* DS3231 Functions */
extern uint8_t DS3231_Init(void);
extern uint32_t DS3231_GetTimestamp(void);

/* Checksum calculation */
uint16_t calculate_checksum(SensorData_t* data) {
    uint8_t* ptr = (uint8_t*)data;
    uint16_t sum = 0;
    for (int i = 0; i < sizeof(SensorData_t) - 2; i++) {
        sum += ptr[i];
    }
    return sum;
}

/* I2C Slave callbacks */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, 
                          uint16_t AddrMatchCode) {
    if (hi2c->Instance == I2C2) {
        if (TransferDirection == I2C_DIRECTION_TRANSMIT) {
            // Master is writing - receive register address
            HAL_I2C_Slave_Seq_Receive_IT(hi2c, (uint8_t*)&i2c_register, 1, 
                                         I2C_FIRST_FRAME);
        } else {
            // Master is reading - send data based on register
            uint8_t* data_ptr = NULL;
            uint16_t data_len = 0;
            
            switch (i2c_register) {
                case REG_STATUS:
                    data_ptr = (uint8_t*)&sensor_data.status;
                    data_len = 1;
                    break;
                case REG_TIMESTAMP:
                    data_ptr = (uint8_t*)&sensor_data.timestamp;
                    data_len = 4;
                    break;
                case REG_TEMPERATURE:
                    data_ptr = (uint8_t*)&sensor_data.temperature;
                    data_len = 4;
                    break;
                case REG_HUMIDITY:
                    data_ptr = (uint8_t*)&sensor_data.humidity;
                    data_len = 4;
                    break;
                case REG_PRESSURE:
                    data_ptr = (uint8_t*)&sensor_data.pressure;
                    data_len = 4;
                    break;
                case REG_ALL_DATA:
                    data_ptr = (uint8_t*)&sensor_data;
                    data_len = sizeof(SensorData_t);
                    break;
                default:
                    data_len = 0;
                    break;
            }
            
            if (data_len > 0) {
                HAL_I2C_Slave_Seq_Transmit_IT(hi2c, data_ptr, data_len, 
                                              I2C_LAST_FRAME);
            }
        }
    }
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
    HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    HAL_I2C_EnableListen_IT(hi2c);
}

/* Update sensor readings */
void update_sensors(void) {
    float temp, hum, press;
    
    // Read BME280
    BME280_ReadData(&temp, &hum, &press);
    
    sensor_data.temperature = temp;
    sensor_data.humidity = hum;
    sensor_data.pressure = press;
    sensor_data.altitude = 44330.0f * (1.0f - powf(press / 1013.25f, 0.1903f));
    
    // Read DS3231 timestamp
    sensor_data.timestamp = DS3231_GetTimestamp();
    
    // Update status
    sensor_data.status = 0x01;  // Data valid
    
    // Calculate checksum
    sensor_data.checksum = calculate_checksum((SensorData_t*)&sensor_data);
    
    data_ready = 1;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();      // Master for sensors
    MX_I2C2_Init_Slave(); // Slave for ESP32
    MX_USART1_UART_Init();
    
    printf("\r\n=== STM32 Sensor Hub Started ===\r\n");
    
    // Initialize sensors
    if (!BME280_Init()) {
        printf("BME280 init failed!\r\n");
        sensor_data.status |= 0x80;  // Error flag
    }
    
    if (!DS3231_Init()) {
        printf("DS3231 init failed!\r\n");
        sensor_data.status |= 0x40;  // Error flag
    }
    
    // Enable I2C slave listening
    HAL_I2C_EnableListen_IT(&hi2c2);
    
    uint32_t last_update = 0;
    
    while (1) {
        // Update sensors every second
        if (HAL_GetTick() - last_update >= 1000) {
            last_update = HAL_GetTick();
            
            update_sensors();
            
            // Debug output
            printf("T:%.1f H:%.1f P:%.1f A:%.1f\r\n",
                   sensor_data.temperature,
                   sensor_data.humidity,
                   sensor_data.pressure,
                   sensor_data.altitude);
            
            // Toggle LED
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}

/* I2C2 Slave Initialization */
static void MX_I2C2_Init_Slave(void) {
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 100000;
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = I2C_SLAVE_ADDR << 1;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c2);
}

/* GPIO for I2C2 (PB10=SCL, PB11=SDA) */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    if (hi2c->Instance == I2C1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (hi2c->Instance == I2C2) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C2_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        
        HAL_NVIC_SetPriority(I2C2_EV_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
        HAL_NVIC_SetPriority(I2C2_ER_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
    }
}
```

### ESP32 - Display Controller

**File: `esp32_display_controller/src/main.cpp`**

```cpp
/**
 * @file main.cpp
 * @brief ESP32 Display Controller - I2C Master with OLED and EEPROM
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// I2C Addresses
#define STM32_SLAVE_ADDR    0x08
#define OLED_ADDR           0x3C
#define EEPROM_ADDR         0x50

// Pins
#define I2C_SDA             21
#define I2C_SCL             22
#define BUTTON_PIN          0
#define BUZZER_PIN          25
#define LED_PIN             2

// Display
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64

// STM32 Registers
#define REG_STATUS          0x00
#define REG_ALL_DATA        0x20

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor Data Structure (must match STM32)
struct __attribute__((packed)) SensorData {
    uint32_t timestamp;
    float temperature;
    float humidity;
    float pressure;
    float altitude;
    uint8_t status;
    uint16_t checksum;
};

SensorData currentData;
SensorData historyData[60];  // 1 hour of data at 1 min intervals
int historyIndex = 0;

// Thresholds
float tempMin = 15.0, tempMax = 35.0;
float humMin = 30.0, humMax = 80.0;

// UI State
enum Screen { DASHBOARD, HISTORY, STATISTICS, SETTINGS, DATALOG };
Screen currentScreen = DASHBOARD;

// Button handling
volatile bool buttonPressed = false;
unsigned long lastDebounce = 0;

void IRAM_ATTR buttonISR() {
    if (millis() - lastDebounce > 200) {
        buttonPressed = true;
        lastDebounce = millis();
    }
}

// Read data from STM32 Slave
bool readSTM32Data() {
    Wire.beginTransmission(STM32_SLAVE_ADDR);
    Wire.write(REG_ALL_DATA);
    if (Wire.endTransmission() != 0) {
        Serial.println("STM32 write error");
        return false;
    }
    
    delay(5);
    
    uint8_t bytesRead = Wire.requestFrom(STM32_SLAVE_ADDR, sizeof(SensorData));
    if (bytesRead != sizeof(SensorData)) {
        Serial.printf("Read error: got %d bytes\n", bytesRead);
        return false;
    }
    
    uint8_t* ptr = (uint8_t*)&currentData;
    for (int i = 0; i < sizeof(SensorData); i++) {
        ptr[i] = Wire.read();
    }
    
    // Verify checksum
    uint16_t calcSum = 0;
    for (int i = 0; i < sizeof(SensorData) - 2; i++) {
        calcSum += ptr[i];
    }
    
    if (calcSum != currentData.checksum) {
        Serial.println("Checksum mismatch!");
        return false;
    }
    
    return true;
}

// EEPROM functions
void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.write(data);
    Wire.endTransmission();
    delay(5);
}

uint8_t EEPROM_ReadByte(uint16_t addr) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_ADDR, 1);
    return Wire.read();
}

void saveToEEPROM(SensorData* data) {
    // Simplified - implement circular buffer in production
    static uint16_t writeAddr = 0x0020;
    
    uint8_t* ptr = (uint8_t*)data;
    for (int i = 0; i < 10; i++) {  // Save compact record
        EEPROM_WriteByte(writeAddr++, ptr[i]);
    }
    
    if (writeAddr >= 32768 - 32) writeAddr = 0x0020;  // Wrap
}

// Alert checking
void checkAlerts() {
    bool alert = false;
    
    if (currentData.temperature < tempMin || currentData.temperature > tempMax) {
        alert = true;
        digitalWrite(LED_PIN, HIGH);
    }
    
    if (currentData.humidity < humMin || currentData.humidity > humMax) {
        alert = true;
    }
    
    if (alert) {
        tone(BUZZER_PIN, 2000, 100);
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}

// Display functions
void drawDashboard() {
    display.clearDisplay();
    display.setTextColor(WHITE);
    
    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ENVIRONMENTAL MONITOR");
    display.drawLine(0, 10, 127, 10, WHITE);
    
    // Temperature (large)
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.printf("%.1fC", currentData.temperature);
    
    // Humidity
    display.setCursor(75, 14);
    display.printf("%.0f%%", currentData.humidity);
    
    // Pressure
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.printf("Press: %.1f hPa", currentData.pressure);
    
    // Altitude
    display.setCursor(0, 45);
    display.printf("Alt: %.0f m", currentData.altitude);
    
    // Status bar
    display.drawLine(0, 54, 127, 54, WHITE);
    display.setCursor(0, 56);
    
    // Time from timestamp
    uint32_t ts = currentData.timestamp;
    int h = (ts / 3600) % 24;
    int m = (ts / 60) % 60;
    int s = ts % 60;
    display.printf("%02d:%02d:%02d", h, m, s);
    
    // Status indicator
    display.setCursor(100, 56);
    display.print(currentData.status == 0x01 ? "OK" : "ERR");
    
    display.display();
}

void drawHistory() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("TEMPERATURE HISTORY");
    display.drawLine(0, 10, 127, 10, WHITE);
    
    // Draw graph axes
    display.drawLine(10, 15, 10, 55, WHITE);   // Y axis
    display.drawLine(10, 55, 120, 55, WHITE);  // X axis
    
    // Find min/max for scaling
    float minT = 100, maxT = -40;
    for (int i = 0; i < 60; i++) {
        if (historyData[i].temperature < minT) minT = historyData[i].temperature;
        if (historyData[i].temperature > maxT) maxT = historyData[i].temperature;
    }
    
    // Draw temperature line
    for (int i = 1; i < 60; i++) {
        int x1 = 10 + ((i-1) * 110 / 60);
        int x2 = 10 + (i * 110 / 60);
        int y1 = 55 - (int)((historyData[i-1].temperature - minT) / (maxT - minT) * 38);
        int y2 = 55 - (int)((historyData[i].temperature - minT) / (maxT - minT) * 38);
        display.drawLine(x1, y1, x2, y2, WHITE);
    }
    
    // Labels
    display.setCursor(0, 56);
    display.printf("%.0f-%.0fC", minT, maxT);
    
    display.display();
}

void drawStatistics() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("STATISTICS");
    display.drawLine(0, 10, 127, 10, WHITE);
    
    // Calculate stats
    float sumT = 0, sumH = 0, sumP = 0;
    float minT = 100, maxT = -40;
    float minH = 100, maxH = 0;
    int count = 0;
    
    for (int i = 0; i < 60; i++) {
        if (historyData[i].status == 0x01) {
            sumT += historyData[i].temperature;
            sumH += historyData[i].humidity;
            sumP += historyData[i].pressure;
            if (historyData[i].temperature < minT) minT = historyData[i].temperature;
            if (historyData[i].temperature > maxT) maxT = historyData[i].temperature;
            if (historyData[i].humidity < minH) minH = historyData[i].humidity;
            if (historyData[i].humidity > maxH) maxH = historyData[i].humidity;
            count++;
        }
    }
    
    if (count > 0) {
        display.setCursor(0, 14);
        display.printf("Temp Avg: %.1fC", sumT / count);
        display.setCursor(0, 24);
        display.printf("Temp Min/Max: %.1f/%.1fC", minT, maxT);
        display.setCursor(0, 34);
        display.printf("Hum Avg: %.1f%%", sumH / count);
        display.setCursor(0, 44);
        display.printf("Hum Min/Max: %.0f/%.0f%%", minH, maxH);
        display.setCursor(0, 54);
        display.printf("Samples: %d", count);
    } else {
        display.setCursor(0, 30);
        display.print("No data yet");
    }
    
    display.display();
}

void handleButton() {
    if (buttonPressed) {
        buttonPressed = false;
        currentScreen = (Screen)((currentScreen + 1) % 5);
        Serial.printf("Screen: %d\n", currentScreen);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP32 Display Controller ===\n");
    
    // GPIO setup
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
    
    // I2C setup
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    
    // OLED setup
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 init failed!");
        while (1);
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 20);
    display.println("ENV MON");
    display.setTextSize(1);
    display.setCursor(20, 45);
    display.println("Initializing...");
    display.display();
    delay(2000);
    
    // Initialize history
    memset(historyData, 0, sizeof(historyData));
    
    Serial.println("System ready!");
}

void loop() {
    static unsigned long lastRead = 0;
    static unsigned long lastHistory = 0;
    
    handleButton();
    
    // Read from STM32 every second
    if (millis() - lastRead >= 1000) {
        lastRead = millis();
        
        if (readSTM32Data()) {
            Serial.printf("T:%.1f H:%.1f P:%.1f\n",
                         currentData.temperature,
                         currentData.humidity,
                         currentData.pressure);
            
            checkAlerts();
        }
    }
    
    // Store history every minute
    if (millis() - lastHistory >= 60000) {
        lastHistory = millis();
        
        historyData[historyIndex] = currentData;
        historyIndex = (historyIndex + 1) % 60;
        
        saveToEEPROM(&currentData);
    }
    
    // Update display
    switch (currentScreen) {
        case DASHBOARD:
            drawDashboard();
            break;
        case HISTORY:
            drawHistory();
            break;
        case STATISTICS:
            drawStatistics();
            break;
        case SETTINGS:
            // Draw settings screen
            break;
        case DATALOG:
            // Draw data log screen
            break;
    }
    
    delay(100);
}
```

---

## 📊 Kriteria Penilaian

### Komponen Nilai

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| **Hardware Assembly** | 15% | Rangkaian benar, rapi, dan berfungsi |
| **STM32 Sensor Hub** | 20% | I2C Slave berfungsi, semua sensor terbaca |
| **ESP32 Display** | 20% | UI responsif, semua screen berfungsi |
| **Data Communication** | 15% | I2C antar MCU stabil, checksum valid |
| **Data Logging** | 10% | EEPROM read/write benar, persistent |
| **Alert System** | 5% | Threshold detection dan notification |
| **Error Handling** | 5% | Recovery dari error, graceful degradation |
| **Dokumentasi** | 5% | Laporan lengkap dan jelas |
| **Video Demo** | 5% | Demonstrasi fitur lengkap |

### Level Pencapaian

| Grade | Skor | Kriteria |
|-------|------|----------|
| **A** | 85-100 | Semua fitur berfungsi + bonus features |
| **B** | 70-84 | Semua fitur utama berfungsi |
| **C** | 55-69 | Sebagian besar fitur berfungsi |
| **D** | 40-54 | Minimal sensor dan display bekerja |
| **E** | <40 | Tidak memenuhi minimum requirements |

### Bonus Points (+5% each)

- [ ] Sensor tambahan (AHT20, BH1750, dll)
- [ ] WiFi connectivity dengan web dashboard
- [ ] SD Card logging sebagai backup
- [ ] Power management (sleep modes)
- [ ] Custom PCB design

---

## 📦 Deliverables

### 1. Source Code
- [ ] STM32 project (PlatformIO/CubeIDE)
- [ ] ESP32 project (PlatformIO)
- [ ] Well-commented code
- [ ] README dengan instruksi build

### 2. Dokumentasi
- [ ] Schematic diagram
- [ ] Wiring photos
- [ ] Block diagram
- [ ] Flow charts

### 3. Laporan
- [ ] Pendahuluan dan tujuan
- [ ] Teori singkat I2C Multi-device
- [ ] Desain sistem
- [ ] Implementasi
- [ ] Hasil pengujian
- [ ] Analisis dan kesimpulan

### 4. Video Demonstrasi
- [ ] Durasi: 5-10 menit
- [ ] Hardware overview
- [ ] All features demonstration
- [ ] I2C communication proof
- [ ] Error handling demo

---

## 📅 Timeline

| Minggu | Aktivitas |
|--------|-----------|
| 1 | Hardware assembly, sensor testing individual |
| 2 | STM32 Slave implementation, ESP32 Master implementation |
| 3 | Integration, debugging, documentation |

---

## 💡 Tips Pengerjaan

1. **Start Simple**: Test setiap sensor secara individual dulu
2. **I2C Debugging**: Gunakan logic analyzer atau serial monitor
3. **Checksum**: Selalu validasi data yang diterima
4. **Error Handling**: Implementasikan timeout dan retry
5. **Modular Code**: Pisahkan driver sensor dari logic utama
6. **Version Control**: Gunakan Git untuk tracking perubahan

---

## 📚 Referensi Tambahan

1. STM32 I2C Slave HAL Tutorial
2. ESP32 Wire Library Documentation
3. Adafruit SSD1306 Library Examples
4. BME280 Datasheet - Calibration Section
5. "Mastering STM32" - I2C Chapter


-----------------------------------------------------------
--- Referensi.md ---
-----------------------------------------------------------

# Referensi Modul 06: I2C Bus dan Sensor Integration

## 📚 Dokumentasi Resmi

### STM32 Documentation
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **RM0008** | STM32F1 Reference Manual - I2C Chapter | [ST.com](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| **AN4235** | I2C Timing Configuration Tool | [ST.com](https://www.st.com/resource/en/application_note/an4235-i2c-timing-configuration-tool-for-stm32f3xxxx-and-stm32f0xxxx-microcontrollers-stmicroelectronics.pdf) |
| **HAL I2C Driver** | STM32 HAL I2C Documentation | [ST GitHub](https://github.com/STMicroelectronics/stm32f1xx_hal_driver) |
| **AN2824** | STM32 I2C Optimized Examples | [ST.com](https://www.st.com/resource/en/application_note/an2824-stm32f10xxx-i2c-optimized-examples-stmicroelectronics.pdf) |

### ESP32 Documentation
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **ESP32 TRM** | Technical Reference Manual - I2C | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf) |
| **ESP-IDF I2C** | ESP-IDF I2C Driver API | [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html) |
| **Arduino Wire** | ESP32 Wire Library | [Arduino Reference](https://www.arduino.cc/reference/en/language/functions/communication/wire/) |

### I2C Standard
| Dokumen | Deskripsi | Link |
|---------|-----------|------|
| **I2C Specification** | NXP I2C-bus Specification v6 | [NXP](https://www.nxp.com/docs/en/user-guide/UM10204.pdf) |
| **SMBus Spec** | System Management Bus Specification | [SMBus.org](http://smbus.org/specs/) |

---

## 📖 Datasheet Komponen

### BME280 - Environmental Sensor
| Item | Link |
|------|------|
| Datasheet | [Bosch Sensortec](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf) |
| Arduino Library | [Adafruit BME280](https://github.com/adafruit/Adafruit_BME280_Library) |
| Official Driver | [Bosch GitHub](https://github.com/BoschSensortec/BME280_driver) |

### SSD1306 - OLED Display
| Item | Link |
|------|------|
| Datasheet | [Solomon Systech](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) |
| Arduino Library | [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) |
| U8g2 Alternative | [U8g2 Library](https://github.com/olikraus/u8g2) |

### DS3231 - RTC
| Item | Link |
|------|------|
| Datasheet | [Maxim/Analog](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf) |
| Arduino Library | [RTClib](https://github.com/adafruit/RTClib) |

### 24LC256 - EEPROM
| Item | Link |
|------|------|
| Datasheet | [Microchip](https://ww1.microchip.com/downloads/en/DeviceDoc/24AA256-24LC256-24FC256-Data-Sheet-20001203W.pdf) |

---

## 🎓 Tutorial

### I2C Fundamentals
1. **Sparkfun I2C Tutorial** - [learn.sparkfun.com/tutorials/i2c](https://learn.sparkfun.com/tutorials/i2c)
2. **Understanding I2C** - [Analog.com](https://www.analog.com/en/technical-articles/i2c-primer-what-is-i2c-part-1.html)

### STM32 I2C
1. **STM32 I2C Tutorial** - [DeepBlue Embedded](https://deepbluembedded.com/stm32-i2c-tutorial-hal-examples-slave-dma/)
2. **STM32 HAL I2C** - [Controllers Tech](https://controllerstech.com/stm32-i2c-configuration-using-registers/)

### ESP32 I2C
1. **ESP32 I2C Tutorial** - [RandomNerdTutorials](https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/)
2. **ESP32 Multiple I2C** - [LastMinuteEngineers](https://lastminuteengineers.com/esp32-i2c-tutorial/)

### Sensor Integration
1. **BME280 with ESP32** - [RandomNerdTutorials](https://randomnerdtutorials.com/esp32-bme280-arduino-ide-pressure-temperature-humidity/)
2. **SSD1306 OLED** - [LastMinuteEngineers](https://lastminuteengineers.com/oled-display-esp32-tutorial/)
3. **DS3231 RTC** - [LastMinuteEngineers](https://lastminuteengineers.com/ds3231-rtc-arduino-tutorial/)

---

## 📝 Paper/Artikel

### Academic Resources
1. "Analysis of I2C Protocol for Embedded Systems" - IEEE
2. "Multi-Sensor Data Fusion using I2C" - Sensors Journal
3. "Low-Power I2C Communication Optimization" - ACM

### Books
| Judul | Penulis | Topik |
|-------|---------|-------|
| **Mastering STM32** (2nd Ed) | Carmine Noviello | Chapter 13-14: I2C |
| **Programming with STM32** | Donald Norris | I2C Communication |
| **I2C Bus: Theory to Practice** | Dominique Paret | Complete I2C Reference |

---

## 🎥 Video

### I2C Basics
1. **Phil's Lab - I2C Protocol Explained** - [YouTube](https://www.youtube.com/watch?v=_fgWQ3TIhyE)
2. **EEVblog - I2C Tutorial** - [YouTube](https://www.youtube.com/watch?v=ERLj4D3gA_w)

### STM32 I2C
1. **Controllerstech - STM32 I2C Masterclass** - [YouTube Playlist](https://www.youtube.com/playlist?list=PLfIJKC1ud8ggRvaEsMjSEDazoBvt4IPqU)
2. **DigiKey - STM32 HAL I2C** - [YouTube](https://www.youtube.com/watch?v=isOekyygpR8)

### ESP32 I2C
1. **DroneBot Workshop - ESP32 I2C** - [YouTube](https://www.youtube.com/watch?v=7kYvbMR0OAA)
2. **Andreas Spiess - ESP32 Multiple I2C** - [YouTube](https://www.youtube.com/watch?v=2_pDdGdknPA)

---

## 🛠️ Tools

| Tool | Deskripsi | Link |
|------|-----------|------|
| **PlatformIO** | IDE untuk STM32 & ESP32 | [platformio.org](https://platformio.org/) |
| **STM32CubeIDE** | Official ST IDE | [ST.com](https://www.st.com/en/development-tools/stm32cubeide.html) |
| **Wokwi** | ESP32 Simulator | [wokwi.com](https://wokwi.com/) |
| **Logic Analyzer** | Saleae Logic | [saleae.com](https://www.saleae.com/) |

---

## 📋 I2C Address Quick Reference

| Device | 7-bit Address | Read | Write |
|--------|---------------|------|-------|
| BME280 | 0x76/0x77 | 0xED/0xEF | 0xEC/0xEE |
| SSD1306 | 0x3C/0x3D | 0x79/0x7B | 0x78/0x7A |
| DS3231 | 0x68 | 0xD1 | 0xD0 |
| 24LC256 | 0x50-0x57 | 0xA1-0xAF | 0xA0-0xAE |

---

*Terakhir diperbarui: 2024*


-----------------------------------------------------------
--- Rubrik_Penilaian_Project.md ---
-----------------------------------------------------------

# Rubrik Penilaian Project
## Modul 06: I2C Bus dan Sensor Integration - Smart Environmental Monitor

---

## 📋 Informasi Penilaian

| Item | Keterangan |
|------|------------|
| **Nama Project** | Smart Environmental Monitor dengan Dual-MCU I2C |
| **Bobot Total** | 100 poin |
| **Passing Grade** | 55 poin |

---

## 📊 Komponen Penilaian

### A. Hardware dan Assembly (15 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Rangkaian I2C sesuai skematik (pull-up resistor, wiring benar) | 5 | |
| 2 | Koneksi STM32-ESP32 terhubung dengan baik | 4 | |
| 3 | Semua komponen terpasang (BME280, SSD1306, DS3231, EEPROM) | 4 | |
| 4 | Kerapian dan keamanan wiring | 2 | |
| | **Subtotal A** | **15** | |

**Panduan Penilaian:**
- 5 poin: Semua kriteria terpenuhi sempurna
- 3-4 poin: Sebagian besar terpenuhi dengan minor issues
- 1-2 poin: Banyak issues tetapi masih fungsional
- 0 poin: Tidak fungsional

---

### B. STM32 Sensor Hub Implementation (20 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | I2C Master untuk sensor berfungsi (BME280/DS3231 terbaca) | 6 | |
| 2 | I2C Slave mode implementasi (address 0x08 merespon) | 6 | |
| 3 | Data structure dan register map sesuai spesifikasi | 4 | |
| 4 | Checksum calculation dan validation | 2 | |
| 5 | Error handling dan status reporting | 2 | |
| | **Subtotal B** | **20** | |

**Panduan Penilaian I2C Slave:**
- 6 poin: Merespon read/write dengan data correct
- 4-5 poin: Merespon tetapi ada data inconsistency
- 2-3 poin: Hanya merespon address, data incomplete
- 0-1 poin: Tidak merespon atau crash

---

### C. ESP32 Display Controller (20 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | I2C Master request ke STM32 berhasil | 5 | |
| 2 | OLED Display menampilkan data dengan format baik | 5 | |
| 3 | Multiple screen UI (Dashboard, History, Statistics) | 5 | |
| 4 | Button navigation berfungsi | 3 | |
| 5 | User interface responsif dan informatif | 2 | |
| | **Subtotal C** | **20** | |

**Panduan Penilaian Display:**
- 5 poin: All screens implemented dengan UI yang baik
- 3-4 poin: Main screens work, minor UI issues
- 1-2 poin: Basic display works, limited functionality
- 0 poin: Display tidak berfungsi

---

### D. Data Communication - I2C antar MCU (15 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Komunikasi Master-Slave stabil (tidak error/hang) | 5 | |
| 2 | Data transfer rate sesuai (1 second update) | 3 | |
| 3 | Checksum validation berfungsi | 3 | |
| 4 | Protokol register-based access benar | 2 | |
| 5 | Bus arbitration handling (jika multi-master) | 2 | |
| | **Subtotal D** | **15** | |

**Panduan Penilaian Communication:**
- 5 poin: >95% success rate, no errors
- 3-4 poin: 80-95% success rate
- 1-2 poin: 50-80% success rate
- 0 poin: <50% atau tidak berfungsi

---

### E. Data Logging (10 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | EEPROM write berfungsi (data tersimpan) | 3 | |
| 2 | EEPROM read berfungsi (data terbaca kembali) | 3 | |
| 3 | Circular buffer implementation | 2 | |
| 4 | Data persistent setelah power cycle | 2 | |
| | **Subtotal E** | **10** | |

---

### F. Alert System (5 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Temperature threshold detection | 2 | |
| 2 | Humidity threshold detection | 1 | |
| 3 | LED/Buzzer alert berfungsi | 2 | |
| | **Subtotal F** | **5** | |

---

### G. Error Handling (5 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | I2C bus recovery mechanism | 2 | |
| 2 | Sensor failure detection dan reporting | 1 | |
| 3 | Graceful degradation (sistem tetap berjalan partial) | 2 | |
| | **Subtotal G** | **5** | |

---

### H. Dokumentasi (5 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | README dengan instruksi lengkap | 2 | |
| 2 | Schematic/wiring diagram | 1 | |
| 3 | Code comments dan struktur | 1 | |
| 4 | Laporan pengujian | 1 | |
| | **Subtotal H** | **5** | |

---

### I. Video Demonstrasi (5 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Hardware overview jelas | 1 | |
| 2 | Semua fitur didemonstrasikan | 2 | |
| 3 | I2C communication proof (serial/analyzer) | 1 | |
| 4 | Durasi sesuai (5-10 menit) | 1 | |
| | **Subtotal I** | **5** | |

---

## 🌟 Bonus Points (Maximum +15 poin)

| No | Kriteria Bonus | Poin | Skor |
|----|----------------|------|------|
| 1 | Sensor tambahan terintegrasi (AHT20, BH1750, dll) | +5 | |
| 2 | WiFi connectivity dengan web dashboard | +5 | |
| 3 | Power management (sleep modes) | +3 | |
| 4 | Custom PCB design | +2 | |
| | **Total Bonus** | **(+15)** | |

---

## 📈 Rekapitulasi Nilai

| Komponen | Bobot | Skor | Nilai |
|----------|-------|------|-------|
| A. Hardware dan Assembly | 15 | | |
| B. STM32 Sensor Hub | 20 | | |
| C. ESP32 Display Controller | 20 | | |
| D. Data Communication | 15 | | |
| E. Data Logging | 10 | | |
| F. Alert System | 5 | | |
| G. Error Handling | 5 | | |
| H. Dokumentasi | 5 | | |
| I. Video Demonstrasi | 5 | | |
| **Total Base** | **100** | | |
| Bonus Points | (+15) | | |
| **TOTAL AKHIR** | **Max 115** | | |

---

## 📊 Konversi Grade

| Skor | Grade | Predikat |
|------|-------|----------|
| 85-100+ | A | Excellent - Semua fitur + bonus |
| 75-84 | B+ | Very Good |
| 70-74 | B | Good - Semua fitur utama |
| 65-69 | C+ | Above Average |
| 55-64 | C | Average - Memenuhi minimum |
| 45-54 | D | Below Average |
| <45 | E | Fail |

---

## 📝 Catatan Penilai

**Kekuatan:**
```
[Tuliskan aspek yang dikerjakan dengan baik]
```

**Area Perbaikan:**
```
[Tuliskan aspek yang perlu ditingkatkan]
```

**Feedback:**
```
[Berikan feedback konstruktif untuk mahasiswa]
```

---

| | |
|----------|------------|
| **Penilai** | _________________ |
| **Tanggal** | _________________ |
| **Tanda Tangan** | _________________ |


-----------------------------------------------------------
--- Rubrik_Penilaian_Tugas_Video.md ---
-----------------------------------------------------------

# Rubrik Penilaian Tugas Video
## Modul 06: I2C Bus dan Sensor Integration

---

## 📋 Informasi Tugas

| Item | Keterangan |
|------|------------|
| **Topik** | I2C Protocol dan Multi-Sensor Integration |
| **Platform** | STM32F103C8T6 + ESP32 DevKit V1 |
| **Durasi Video** | 8-12 menit |
| **Format** | MP4 (H.264), 720p minimum |
| **Bobot Total** | 100 poin |

---

## 📹 Struktur Video yang Diharapkan

### Timeline Rekomendasi

| Segmen | Durasi | Konten |
|--------|--------|--------|
| Opening | 0:30 | Intro, judul, nama praktikan |
| Teori I2C | 2:00 | Penjelasan protokol dan konsep |
| Hardware | 1:30 | Overview rangkaian dan komponen |
| STM32 Demo | 2:30 | Demonstrasi sensor reading |
| ESP32 Demo | 2:30 | Demonstrasi display & communication |
| Multi-Device | 1:30 | Demo integrasi semua device |
| Closing | 1:30 | Kesimpulan dan insights |
| **Total** | **~12:00** | |

---

## 📊 Komponen Penilaian

### A. Konten Teori I2C (25 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Penjelasan dasar protokol I2C (Master-Slave, SDA-SCL) | 5 | |
| 2 | Penjelasan address format (7-bit/10-bit) | 4 | |
| 3 | Penjelasan timing diagram (Start, Stop, ACK/NACK) | 5 | |
| 4 | Perbedaan I2C vs protokol lain (SPI, UART) | 4 | |
| 5 | Penjelasan pull-up resistor dan electrical requirements | 4 | |
| 6 | Konsep multi-device pada satu bus | 3 | |
| | **Subtotal A** | **25** | |

**Panduan Penilaian:**
- 5 poin: Penjelasan sangat jelas, contoh tepat, visual aid
- 3-4 poin: Penjelasan cukup jelas, minor inaccuracies
- 1-2 poin: Penjelasan dasar saja, ada kesalahan
- 0 poin: Tidak dijelaskan atau salah total

---

### B. Demonstrasi Praktikum (35 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | I2C Scanner - mendeteksi address device | 4 | |
| 2 | BME280 sensor reading (temperature, humidity, pressure) | 6 | |
| 3 | SSD1306 OLED display output | 6 | |
| 4 | DS3231 RTC reading (time, date) | 5 | |
| 5 | 24LC256 EEPROM read/write | 5 | |
| 6 | Multi-device integration demo | 5 | |
| 7 | Serial monitor output terlihat jelas | 4 | |
| | **Subtotal B** | **35** | |

**Panduan Penilaian Demo:**
- Full poin: Demo berjalan lancar, output sesuai expectation
- 70% poin: Demo berjalan dengan minor issues
- 50% poin: Demo berjalan partial
- <50% poin: Demo gagal atau tidak ditunjukkan

---

### C. Kualitas Teknis Video (20 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Resolusi minimal 720p, fokus tajam | 5 | |
| 2 | Audio jelas, tidak ada noise berlebihan | 5 | |
| 3 | Pencahayaan cukup, komponen terlihat jelas | 4 | |
| 4 | Screen capture/recording berkualitas baik | 3 | |
| 5 | Editing smooth, transisi appropriate | 3 | |
| | **Subtotal C** | **20** | |

**Panduan Penilaian Teknis:**
| Aspek | Excellent (100%) | Good (75%) | Fair (50%) | Poor (<50%) |
|-------|-----------------|------------|------------|-------------|
| Video | 1080p, crystal clear | 720p, clear | 480p, acceptable | Blur, pixelated |
| Audio | Clear, professional | Clear, minor noise | Understandable | Hard to hear |
| Lighting | Perfect | Good | Acceptable | Too dark/bright |

---

### D. Penyampaian dan Komunikasi (15 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Narasi jelas dan mudah dipahami | 5 | |
| 2 | Sistematika penyampaian terstruktur | 4 | |
| 3 | Penggunaan istilah teknis tepat | 3 | |
| 4 | Tempo bicara appropriate (tidak terlalu cepat/lambat) | 3 | |
| | **Subtotal D** | **15** | |

---

### E. Kelengkapan dan Kreativitas (5 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Durasi sesuai (8-12 menit) | 2 | |
| 2 | Visual aids (diagram, animasi, overlay) | 2 | |
| 3 | Kesimpulan dan insights bermakna | 1 | |
| | **Subtotal E** | **5** | |

---

## 🌟 Bonus Points (Maximum +10 poin)

| No | Kriteria Bonus | Poin | Skor |
|----|----------------|------|------|
| 1 | Analisis timing dengan logic analyzer | +4 | |
| 2 | Perbandingan performa STM32 vs ESP32 | +3 | |
| 3 | Troubleshooting demo (recovery dari error) | +3 | |
| | **Total Bonus** | **(+10)** | |

---

## ⚠️ Penalty Points

| No | Pelanggaran | Pengurangan |
|----|-------------|-------------|
| 1 | Durasi kurang dari 5 menit | -15 |
| 2 | Durasi lebih dari 15 menit | -5 |
| 3 | Tidak ada demo hardware nyata | -20 |
| 4 | Plagiarism/copy video lain | -100 (Fail) |
| 5 | Audio/video quality sangat buruk | -10 |

---

## 📈 Rekapitulasi Nilai

| Komponen | Bobot | Skor | Nilai |
|----------|-------|------|-------|
| A. Konten Teori I2C | 25 | | |
| B. Demonstrasi Praktikum | 35 | | |
| C. Kualitas Teknis Video | 20 | | |
| D. Penyampaian | 15 | | |
| E. Kelengkapan & Kreativitas | 5 | | |
| **Total Base** | **100** | | |
| Bonus Points | (+10) | | |
| Penalty Points | (-) | | |
| **TOTAL AKHIR** | | | |

---

## 📊 Konversi Grade

| Skor | Grade | Predikat |
|------|-------|----------|
| 85-100+ | A | Excellent - Video berkualitas profesional |
| 75-84 | B+ | Very Good |
| 70-74 | B | Good - Semua aspek terpenuhi |
| 65-69 | C+ | Above Average |
| 55-64 | C | Average - Memenuhi minimum |
| 45-54 | D | Below Average |
| <45 | E | Fail |

---

## 📝 Checklist Sebelum Submit

### Konten
- [ ] Intro dengan identitas praktikan
- [ ] Penjelasan teori I2C
- [ ] Demo I2C Scanner
- [ ] Demo minimal 3 device I2C berbeda
- [ ] Demo pada STM32 DAN ESP32
- [ ] Kesimpulan dan lessons learned

### Teknis
- [ ] Format video MP4/MOV
- [ ] Resolusi minimal 720p
- [ ] Audio jelas
- [ ] File size reasonable (<500MB)
- [ ] Durasi 8-12 menit

### Etika
- [ ] Video adalah karya original
- [ ] Semua sumber disebutkan
- [ ] Tidak mengandung konten inappropriate

---

## 💬 Catatan Penilai

**Kekuatan Video:**
```
[Aspek yang baik dari video]
```

**Area Perbaikan:**
```
[Saran untuk video yang lebih baik]
```

**Feedback:**
```
[Feedback konstruktif untuk praktikan]
```

---

| | |
|----------|------------|
| **Penilai** | _________________ |
| **Tanggal** | _________________ |
| **Tanda Tangan** | _________________ |

