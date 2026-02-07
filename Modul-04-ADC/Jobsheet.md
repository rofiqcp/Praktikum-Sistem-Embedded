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