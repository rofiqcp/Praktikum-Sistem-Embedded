# Jobsheet Modul 05 - DAC & PWM

## Praktikum Sistem Embedded

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami dan menggunakan DAC** — Mengkonfigurasi output DAC untuk menghasilkan tegangan analog yang presisi pada ESP32 (GPIO25/GPIO26) dan STM32 yang mendukung DAC
2. **Menghasilkan sinyal PWM** — Memprogram modul LEDC (ESP32) dan Timer (STM32) untuk menghasilkan sinyal PWM dengan frekuensi dan duty cycle yang dapat diatur
3. **Mengkonfigurasi LEDC dan Timer** — Memahami arsitektur LEDC controller pada ESP32 (channel, timer, resolusi) dan Timer PWM pada STM32 (prescaler, ARR, CCR)
4. **Mengontrol servo motor dan DC motor** — Mengaplikasikan sinyal PWM untuk mengontrol posisi servo SG90 (0°-180°) dan kecepatan motor DC melalui driver L298N
5. **Membandingkan DAC vs PWM** — Menganalisis perbedaan karakteristik output DAC (true analog) dengan PWM yang difilter menggunakan RC low-pass filter

---

## 2. Peralatan yang Dibutuhkan

### Hardware
| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 (WROOM-32) | 1 | DAC tersedia di GPIO25 & GPIO26 |
| 2 | STM32 Blue Pill (F103C8T6) / Black Pill (F411CE) | 1 | F103 tidak memiliki DAC, F411 memiliki DAC di PA4/PA5 |
| 3 | LED (Merah, Hijau, Biru) | 3 | Untuk eksperimen PWM LED |
| 4 | LED RGB Common Cathode | 1 | Untuk PWM RGB |
| 5 | Resistor 220Ω | 5 | Current limiting LED |
| 6 | Resistor 10kΩ | 2 | RC filter |
| 7 | Kapasitor 100nF (0.1µF) | 2 | RC filter |
| 8 | Kapasitor 1µF | 1 | RC filter |
| 9 | Servo Motor SG90 | 1 | Untuk kontrol sudut |
| 10 | Motor DC 5V | 1 | Untuk kontrol kecepatan |
| 11 | Driver Motor L298N | 1 | H-Bridge driver |
| 12 | Buzzer/Speaker 8Ω | 1 | Untuk output audio |
| 13 | Breadboard | 1 | Full-size |
| 14 | Kabel jumper | ~30 | Male-male dan male-female |
| 15 | Multimeter digital | 1 | Untuk mengukur tegangan |
| 16 | Osiloskop (opsional) | 1 | Untuk mengamati bentuk gelombang |
| 17 | Kabel USB Micro/Type-C | 2 | Programming & power |

### Software
- **PlatformIO** di VS Code
- **Serial Monitor / Plotter** (bawaan PlatformIO)
- **Python 3.x** dengan `pyserial` dan `matplotlib` (untuk debug/visualisasi)
- **Wokwi Simulator** (opsional, untuk simulasi tanpa hardware)

---

## 3. Teori Singkat

### 3.1 Digital-to-Analog Converter (DAC)

DAC mengubah nilai digital menjadi tegangan analog yang kontinu. ESP32 memiliki **2 channel DAC 8-bit** pada:
- **DAC1** → GPIO25 (Channel 1)
- **DAC2** → GPIO26 (Channel 2)

Resolusi 8-bit menghasilkan 256 level tegangan (0-255), dengan rentang output 0V - 3.3V:

$$V_{out} = \frac{DAC\_value}{255} \times 3.3V$$

**Catatan Penting:**
- ESP32-S2 dan ESP32-S3 **tidak memiliki DAC**
- STM32F103 (Blue Pill) **tidak memiliki DAC** — hanya STM32F4xx ke atas
- STM32F411 memiliki DAC 12-bit pada PA4 (DAC_OUT1) dan PA5 (DAC_OUT2)

### 3.2 Pulse Width Modulation (PWM)

PWM adalah teknik menghasilkan sinyal analog menggunakan sinyal digital dengan mengatur **duty cycle** — persentase waktu sinyal HIGH dalam satu periode:

$$Duty\ Cycle\ (\%) = \frac{T_{ON}}{T_{ON} + T_{OFF}} \times 100\%$$

$$V_{avg} = Duty\ Cycle \times V_{max}$$

### 3.3 LEDC Controller (ESP32)

ESP32 memiliki **LEDC (LED Control)** peripheral dengan:
- **16 channel** PWM independen (8 high-speed, 8 low-speed)
- **4 timer** (masing-masing untuk high-speed dan low-speed)
- Resolusi hingga **16-bit** (65536 level)
- Frekuensi yang dapat dikonfigurasi

```
Frekuensi PWM = Clock_Source / (2^resolusi × prescaler)
```

### 3.4 Timer PWM (STM32)

STM32 menggunakan **hardware timer** untuk PWM:
- Timer memiliki **prescaler (PSC)**, **auto-reload register (ARR)**, dan **capture/compare register (CCR)**
- Frekuensi PWM: $f_{PWM} = \frac{f_{CLK}}{(PSC+1) \times (ARR+1)}$
- Duty cycle: $Duty = \frac{CCR}{ARR+1} \times 100\%$

### 3.5 Servo Motor

Servo SG90 dikontrol dengan sinyal PWM:
- **Frekuensi**: 50Hz (periode 20ms)
- **Pulse width**: 0.5ms (0°) — 1.5ms (90°) — 2.5ms (180°)

### 3.6 RC Low-Pass Filter

Untuk mengubah sinyal PWM menjadi tegangan DC analog:

$$f_c = \frac{1}{2\pi RC}$$

Dimana $f_c$ adalah frekuensi cutoff. Pilih $f_c$ jauh di bawah frekuensi PWM.

---

## 4. Langkah Praktikum

> **Aturan Umum:**
> - Baca setiap langkah dengan teliti sebelum memulai
> - Pastikan koneksi kabel benar sebelum menyalakan power
> - Catat semua hasil pengamatan di buku/dokumen laporan
> - Upload kode menggunakan PlatformIO (`pio run -t upload`)
> - Buka Serial Monitor pada baudrate 115200 kecuali disebutkan lain

---

### Program 01: DAC_Voltage_Output

**Tujuan:** Menghasilkan tegangan analog yang presisi menggunakan DAC dan memverifikasi dengan multimeter.

**Rangkaian:**

| Komponen | ESP32 | STM32F411 |
|----------|-------|-----------|
| DAC Output | GPIO25 (DAC1) | PA4 (DAC_OUT1) |
| Multimeter (+) | GPIO25 | PA4 |
| Multimeter (-) | GND | GND |

> **Catatan:** STM32F103 tidak memiliki DAC. Gunakan STM32F411 atau skip ke program PWM.

**Langkah-langkah:**

1. Hubungkan kabel probe multimeter ke pin DAC dan GND
2. Set multimeter ke mode pengukuran tegangan DC (range 0-5V)
3. Buka folder `ESP32/ESP32_01_DAC_Voltage_Output` di PlatformIO
4. Baca kode program — program akan mengeluarkan tegangan bertingkat: 0V, 0.825V, 1.65V, 2.475V, 3.3V
5. Compile dan upload: `pio run -t upload`
6. Buka Serial Monitor (115200 baud)
7. Amati output di Serial Monitor yang menampilkan nilai DAC dan tegangan teoritis
8. Ukur tegangan aktual pada multimeter untuk setiap level
9. Catat dan bandingkan tegangan teoritis vs aktual

**Pengamatan:**

| DAC Value | Tegangan Teoritis | Tegangan Terukur | Error (%) |
|-----------|-------------------|------------------|-----------|
| 0 | 0.000V | ______V | ______% |
| 64 | 0.825V | ______V | ______% |
| 128 | 1.650V | ______V | ______% |
| 192 | 2.475V | ______V | ______% |
| 255 | 3.300V | ______V | ______% |

**Pertanyaan:**
1. Berapa resolusi tegangan minimum DAC 8-bit ESP32? Hitung dengan rumus.
2. Mengapa tegangan terukur mungkin berbeda dari nilai teoritis? Sebutkan minimal 2 faktor.
3. Jika menggunakan DAC 12-bit (STM32F411), berapa resolusi tegangan minimumnya?

---

### Program 02: DAC_Sine_Wave

**Tujuan:** Menghasilkan gelombang sinus menggunakan DAC dan mengamati bentuk gelombang.

**Rangkaian:**

| Komponen | ESP32 | STM32F411 |
|----------|-------|-----------|
| DAC Output | GPIO25 | PA4 |
| Osiloskop CH1 (+) | GPIO25 | PA4 |
| Osiloskop GND | GND | GND |

**Langkah-langkah:**

1. Hubungkan probe osiloskop ke pin DAC output
2. Buka folder `ESP32/ESP32_02_DAC_Sine_Wave`
3. Pelajari kode — program menggunakan tabel lookup sinus dengan 256 sampel
4. Program menghitung nilai sinus: `dac_value = 128 + 127 * sin(2π × i / N)`
5. Compile dan upload ke board
6. Buka Serial Monitor — amati frekuensi gelombang yang dilaporkan
7. Pada osiloskop, atur timebase agar terlihat 2-3 periode gelombang
8. Amati bentuk gelombang — perhatikan apakah ada "tangga" (staircase effect) karena resolusi 8-bit
9. Coba ubah jumlah sampel per periode (64, 128, 256) dan amati efeknya

**Pengamatan:**
- Frekuensi gelombang sinus yang dihasilkan: ______ Hz
- Amplitudo peak-to-peak: ______ V
- Apakah terlihat staircase effect? ______
- Pengaruh jumlah sampel terhadap kualitas gelombang: ______

**Pertanyaan:**
1. Berapa frekuensi maksimum gelombang sinus yang dapat dihasilkan oleh DAC 8-bit ESP32?
2. Mengapa terjadi staircase effect? Bagaimana cara menguranginya?
3. Apa hubungan antara jumlah sampel per periode dengan frekuensi output dan kualitas gelombang?

---

### Program 03: DAC_Triangle_Wave

**Tujuan:** Menghasilkan gelombang segitiga (triangle wave) menggunakan DAC.

**Rangkaian:** Sama dengan Program 02.

**Langkah-langkah:**

1. Buka folder `ESP32/ESP32_03_DAC_Triangle_Wave`
2. Pelajari kode — program membuat gelombang segitiga dengan menaikkan nilai DAC dari 0 ke 255 lalu menurunkannya
3. Compile dan upload
4. Amati pada osiloskop bentuk gelombang segitiga
5. Bandingkan dengan gelombang sinus dari Program 02
6. Coba variasikan kecepatan naik/turun untuk membuat gelombang sawtooth
7. Amati perbedaan gelombang segitiga simetris vs sawtooth

**Pengamatan:**
- Frekuensi gelombang segitiga: ______ Hz
- Tegangan minimum: ______ V, Tegangan maksimum: ______ V
- Linearitas kenaikan/penurunan: baik / kurang baik
- Perbedaan visual dengan gelombang sinus: ______

**Pertanyaan:**
1. Bagaimana cara mengubah frekuensi gelombang segitiga tanpa mengubah resolusi?
2. Apa perbedaan antara gelombang segitiga dan sawtooth dalam domain frekuensi?
3. Aplikasi praktis apa yang menggunakan gelombang segitiga?

---

### Program 04: DAC_Audio_Tone

**Tujuan:** Menghasilkan nada audio 440Hz (A4) dan 880Hz (A5) melalui DAC ke speaker/buzzer.

**Rangkaian:**

| Komponen | ESP32 | Keterangan |
|----------|-------|------------|
| DAC Output | GPIO25 | Sinyal audio |
| Kapasitor 10µF | Seri antara GPIO25 dan speaker | DC blocking capacitor |
| Speaker 8Ω | Setelah kapasitor ke GND | Output suara |

> **Catatan:** Untuk STM32F103 yang tidak memiliki DAC, gunakan PWM pada pin PA8 (TIM1_CH1) dengan RC filter.

**Langkah-langkah:**

1. Rangkai speaker dengan kapasitor DC-blocking ke pin DAC
2. Buka folder `ESP32/ESP32_04_DAC_Audio_Tone`
3. Pelajari kode — program menghasilkan gelombang sinus pada frekuensi audio
4. Compile dan upload
5. Dengarkan nada 440Hz (nada A4 standar tuning)
6. Program akan bergantian antara 440Hz dan 880Hz
7. Perhatikan perbedaan pitch antara kedua frekuensi (1 oktaf)
8. Coba ubah frekuensi ke nada lain (C=262Hz, D=294Hz, E=330Hz, dll)

**Pengamatan:**
- Apakah nada 440Hz terdengar jelas? ______
- Perbedaan suara 440Hz vs 880Hz: ______
- Kualitas suara (bersih/noise): ______
- Volume output: ______

**Pertanyaan:**
1. Mengapa diperlukan kapasitor DC-blocking antara DAC dan speaker?
2. Berapa jumlah sampel minimum per periode agar nada 440Hz terdengar baik?
3. Mengapa 880Hz terdengar lebih tinggi 1 oktaf dari 440Hz? Jelaskan hubungan matematis frekuensi dan oktaf.

---

### Program 05: PWM_LED_Breathing

**Tujuan:** Membuat efek LED "breathing" (fade in/out) menggunakan PWM.

**Rangkaian:**

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| LED + Resistor 220Ω | GPIO2 | PA0 (TIM2_CH1) |
| LED Anode → Resistor → Pin | Pin → R → LED → GND | Pin → R → LED → GND |

**Langkah-langkah:**

1. Hubungkan LED dengan resistor 220Ω ke pin PWM
2. Buka folder `ESP32/ESP32_05_PWM_LED_Breathing`
3. Pelajari kode — ESP32 menggunakan LEDC API:
   - `ledcAttach(pin, freq, resolution)` — konfigurasi channel
   - `ledcWrite(pin, dutyCycle)` — set duty cycle
4. Compile dan upload
5. Amati LED yang bernapas (terang→redup→terang berulang)
6. Perhatikan transisi yang halus karena resolusi PWM tinggi
7. Coba ubah kecepatan breathing dan resolusi (8-bit vs 12-bit)
8. Bandingkan kehalusan efek pada resolusi berbeda

**Pengamatan:**
- Resolusi PWM: ______ bit (______ level)
- Frekuensi PWM: ______ Hz
- Periode satu siklus breathing: ______ detik
- Perbedaan kehalusan 8-bit vs 12-bit: ______

**Pertanyaan:**
1. Mengapa mata manusia melihat perubahan kecerahan yang tidak linear? Apa hubungannya dengan gamma correction?
2. Berapa frekuensi PWM minimum agar LED tidak terlihat berkedip (flicker)?
3. Jelaskan perbedaan penggunaan `ledcAttach()` di ESP-IDF v5.x vs `ledcSetup()` di versi sebelumnya.

---

### Program 06: PWM_LED_Brightness

**Tujuan:** Mengontrol kecerahan LED melalui perintah Serial Monitor.

**Rangkaian:** Sama dengan Program 05.

**Langkah-langkah:**

1. Gunakan rangkaian yang sama dengan Program 05
2. Buka folder `ESP32/ESP32_06_PWM_LED_Brightness`
3. Pelajari kode — program membaca input dari Serial Monitor
4. Compile dan upload
5. Buka Serial Monitor (115200 baud)
6. Ketik nilai 0-255 dan tekan Enter untuk mengatur kecerahan
7. Amati perubahan kecerahan LED sesuai nilai yang dimasukkan
8. Coba nilai: 0 (mati), 64 (25%), 128 (50%), 192 (75%), 255 (100%)
9. Perhatikan apakah perubahan kecerahan linear secara visual

**Pengamatan:**

| Nilai Input | Duty Cycle (%) | Kecerahan Visual |
|-------------|----------------|------------------|
| 0 | 0% | ______ |
| 64 | 25% | ______ |
| 128 | 50% | ______ |
| 192 | 75% | ______ |
| 255 | 100% | ______ |

**Pertanyaan:**
1. Mengapa duty cycle 50% tidak terlihat seperti setengah kecerahan maksimum?
2. Bagaimana cara membuat kurva kecerahan yang terlihat linear bagi mata manusia?
3. Apa yang terjadi jika frekuensi PWM terlalu rendah (misalnya 10Hz)?

---

### Program 07: PWM_Servo_Control

**Tujuan:** Mengontrol posisi servo motor SG90 dari 0° hingga 180° menggunakan PWM.

**Rangkaian:**

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Servo Signal (Orange) | GPIO13 | PA1 (TIM2_CH2) |
| Servo VCC (Merah) | 5V (Vin) | 5V |
| Servo GND (Coklat) | GND | GND |

> **Peringatan:** Servo dapat menarik arus hingga 500mA. Gunakan power supply eksternal 5V jika servo bergetar atau tidak stabil.

**Langkah-langkah:**

1. Hubungkan servo motor sesuai tabel di atas
2. Pastikan sumber daya 5V cukup untuk servo
3. Buka folder `ESP32/ESP32_07_PWM_Servo_Control`
4. Pelajari kode — servo menggunakan PWM 50Hz:
   - Pulse 0.5ms → 0° (duty = 0.5/20 × 100% = 2.5%)
   - Pulse 1.5ms → 90° (duty = 1.5/20 × 100% = 7.5%)
   - Pulse 2.5ms → 180° (duty = 2.5/20 × 100% = 12.5%)
5. Compile dan upload
6. Amati servo yang bergerak sweep dari 0° ke 180° dan kembali
7. Buka Serial Monitor — ketik sudut (0-180) untuk mengontrol posisi
8. Verifikasi akurasi posisi dengan busur derajat jika tersedia
9. Coba gerakkan servo ke posisi-posisi tertentu dan catat akurasinya

**Pengamatan:**

| Sudut Target | Pulse Width (ms) | Sudut Aktual | Error |
|--------------|------------------|--------------|-------|
| 0° | 0.5ms | ______° | ______° |
| 45° | 1.0ms | ______° | ______° |
| 90° | 1.5ms | ______° | ______° |
| 135° | 2.0ms | ______° | ______° |
| 180° | 2.5ms | ______° | ______° |

**Pertanyaan:**
1. Mengapa servo motor memerlukan frekuensi PWM tepat 50Hz?
2. Hitung resolusi sudut minimum jika menggunakan PWM 16-bit pada frekuensi 50Hz.
3. Apa yang terjadi jika pulse width di luar range 0.5ms-2.5ms?

---

### Program 08: PWM_Frequency_Sweep

**Tujuan:** Melakukan sweep frekuensi PWM dari 100Hz hingga 20kHz dan mengamati efeknya.

**Rangkaian:**

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Buzzer/Speaker | GPIO25 | PA8 (TIM1_CH1) |
| Osiloskop (opsional) | GPIO25 | PA8 |

**Langkah-langkah:**

1. Hubungkan buzzer/speaker ke pin PWM
2. Buka folder `ESP32/ESP32_08_PWM_Frequency_Sweep`
3. Pelajari kode — program mengubah frekuensi PWM secara bertahap
4. Compile dan upload
5. Dengarkan perubahan nada dari rendah ke tinggi
6. Perhatikan pada frekuensi berapa suara mulai tidak terdengar (~15-20kHz)
7. Jika tersedia osiloskop, amati perubahan periode sinyal
8. Serial Monitor menampilkan frekuensi aktual
9. Catat frekuensi batas pendengaran Anda

**Pengamatan:**
- Frekuensi terendah yang terdengar: ______ Hz
- Frekuensi tertinggi yang terdengar: ______ Hz
- Frekuensi yang paling nyaring: ______ Hz
- Perubahan karakter suara pada frekuensi berbeda: ______

**Pertanyaan:**
1. Mengapa manusia tidak bisa mendengar frekuensi di atas ~20kHz?
2. Apa hubungan antara resolusi PWM dan frekuensi maksimum yang dapat dihasilkan?
3. Pada ESP32, jika clock source 80MHz dan resolusi 10-bit, berapa frekuensi PWM maksimum?

---

### Program 09: PWM_Motor_Speed

**Tujuan:** Mengontrol kecepatan motor DC menggunakan PWM melalui driver L298N.

**Rangkaian:**

| Komponen | ESP32 | STM32 | Keterangan |
|----------|-------|-------|------------|
| L298N ENA | GPIO14 | PA0 (TIM2_CH1) | PWM speed control |
| L298N IN1 | GPIO27 | PB0 | Direction 1 |
| L298N IN2 | GPIO26 | PB1 | Direction 2 |
| L298N OUT1 | Motor (+) | Motor (+) | Ke motor |
| L298N OUT2 | Motor (-) | Motor (-) | Ke motor |
| L298N 12V | Power Supply (+) | Power Supply (+) | 5-12V sesuai motor |
| L298N GND | GND (shared) | GND (shared) | Common ground |

> **Penting:** Hubungkan GND ESP32/STM32 dengan GND L298N (common ground).

**Langkah-langkah:**

1. Rangkai L298N dengan motor DC sesuai tabel
2. Hubungkan power supply eksternal ke L298N (5-12V sesuai rating motor)
3. **Pastikan jumper 5V regulator pada L298N terpasang** jika VCC motor ≤ 12V
4. Buka folder `ESP32/ESP32_09_PWM_Motor_Speed`
5. Pelajari kode — program mengontrol ENA dengan PWM, IN1/IN2 untuk arah
6. Compile dan upload
7. Motor akan berputar dengan kecepatan bertingkat (25%, 50%, 75%, 100%)
8. Buka Serial Monitor — ketik kecepatan (0-255) dan arah (F/R)
9. Amati perubahan kecepatan motor
10. Test motor di kedua arah rotasi

**Pengamatan:**

| Duty Cycle | Tegangan Efektif | Kecepatan Motor | Arah |
|------------|-----------------|-----------------|------|
| 0% | 0V | Berhenti | - |
| 25% | ______V | ______ RPM | CW |
| 50% | ______V | ______ RPM | CW |
| 75% | ______V | ______ RPM | CW |
| 100% | ______V | ______ RPM | CW |

**Pertanyaan:**
1. Mengapa dibutuhkan driver motor (L298N) dan tidak bisa langsung dari GPIO?
2. Apa fungsi dioda flyback pada driver motor? Apa yang terjadi tanpa dioda?
3. Mengapa kecepatan motor tidak berbanding lurus (linear) dengan duty cycle?

---

### Program 10: PWM_RGB_LED

**Tujuan:** Mengontrol LED RGB untuk menghasilkan efek rainbow color cycle menggunakan 3 channel PWM.

**Rangkaian:**

| Komponen | ESP32 | STM32 | Keterangan |
|----------|-------|-------|------------|
| LED R + R220Ω | GPIO16 | PA0 | PWM Channel 0 |
| LED G + R220Ω | GPIO17 | PA1 | PWM Channel 1 |
| LED B + R220Ω | GPIO18 | PA2 | PWM Channel 2 |
| Common Cathode | GND | GND | Untuk CC type |

**Langkah-langkah:**

1. Hubungkan LED RGB common cathode dengan resistor 220Ω di setiap pin warna
2. Buka folder `ESP32/ESP32_10_PWM_RGB_LED`
3. Pelajari kode — program menggunakan konversi HSV→RGB untuk rainbow effect
4. Tiga channel LEDC mengontrol R, G, B secara independen
5. Compile dan upload
6. Amati LED yang berubah warna mengikuti spektrum pelangi
7. Buka Serial Monitor — coba masukkan nilai RGB manual (format: R,G,B)
8. Eksperimen pencampuran warna:
   - Merah: 255,0,0
   - Kuning: 255,255,0
   - Cyan: 0,255,255
   - Putih: 255,255,255
   - Ungu: 128,0,255

**Pengamatan:**
- Warna yang dihasilkan saat R=255,G=0,B=0: ______
- Warna yang dihasilkan saat R=0,G=255,B=255: ______
- Warna yang dihasilkan saat R=255,G=255,B=255: ______
- Apakah transisi warna rainbow halus? ______

**Pertanyaan:**
1. Jelaskan prinsip pencampuran warna aditif (additive color mixing) pada LED RGB.
2. Apa perbedaan LED RGB common cathode vs common anode dalam hal kontrol PWM?
3. Mengapa model warna HSV lebih mudah digunakan untuk efek rainbow dibanding RGB langsung?

---

### Program 11: PWM_Buzzer_Melody

**Tujuan:** Memainkan melodi sederhana menggunakan PWM buzzer dengan nada-nada musikal.

**Rangkaian:**

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| Buzzer pasif | GPIO25 | PA8 (TIM1_CH1) |
| GND Buzzer | GND | GND |

> **Catatan:** Gunakan **buzzer pasif** (passive buzzer), bukan buzzer aktif. Buzzer aktif memiliki osilator internal dan hanya bisa ON/OFF.

**Langkah-langkah:**

1. Hubungkan buzzer pasif ke pin PWM
2. Buka folder `ESP32/ESP32_11_PWM_Buzzer_Melody`
3. Pelajari kode — program mendefinisikan frekuensi nada musikal:
   - C4=262, D4=294, E4=330, F4=349, G4=392, A4=440, B4=494, C5=523
4. Melodi didefinisikan sebagai array of notes dengan durasi
5. ESP32 menggunakan `ledcWriteTone(pin, frequency)` untuk menghasilkan nada
6. Compile dan upload
7. Dengarkan melodi yang dimainkan (default: "Twinkle Twinkle Little Star" atau lagu sederhana)
8. Coba modifikasi melodi dengan nada dan durasi yang berbeda
9. Eksperimen dengan tempo (kecepatan) melodi

**Pengamatan:**
- Melodi yang dimainkan: ______
- Nada yang paling jelas terdengar: ______
- Nada yang kurang jelas: ______
- Kualitas suara buzzer dibanding speaker: ______

**Pertanyaan:**
1. Apa perbedaan buzzer aktif dan pasif? Mengapa kita gunakan buzzer pasif?
2. Bagaimana hubungan matematis frekuensi antar nada dalam satu oktaf (equal temperament)?
3. Mengapa beberapa nada terdengar lebih keras dari yang lain pada buzzer yang sama?

---

### Program 12: DAC_vs_PWM_Compare

**Tujuan:** Membandingkan output DAC murni dengan PWM yang difilter menggunakan RC low-pass filter.

**Rangkaian:**

| Komponen | Koneksi | Keterangan |
|----------|---------|------------|
| ESP32 GPIO25 (DAC) | Osiloskop CH1 | Output DAC langsung |
| ESP32 GPIO26 (PWM) | R=10kΩ → titik tengah | Input RC filter |
| Kapasitor 1µF | Titik tengah → GND | Bagian RC filter |
| Titik tengah | Osiloskop CH2 | Output PWM terfilter |

RC Filter: $f_c = \frac{1}{2\pi \times 10k\Omega \times 1\mu F} = 15.9 Hz$

**Langkah-langkah:**

1. Rangkai RC low-pass filter (R=10kΩ, C=1µF) pada output PWM
2. Hubungkan DAC output (GPIO25) ke osiloskop CH1
3. Hubungkan output RC filter ke osiloskop CH2
4. Buka folder `ESP32/ESP32_12_DAC_vs_PWM_Compare`
5. Pelajari kode — program menghasilkan sinyal yang sama di DAC dan PWM
6. Compile dan upload
7. Set kedua channel pada skala yang sama
8. Bandingkan kedua sinyal pada osiloskop
9. Perhatikan ripple pada output PWM yang terfilter
10. Coba berbagai frekuensi PWM dan amati efeknya pada ripple

**Pengamatan:**

| Parameter | DAC Output | PWM + RC Filter |
|-----------|-----------|-----------------|
| Tegangan DC (V) | ______ | ______ |
| Ripple (mV p-p) | ______ | ______ |
| Response time (ms) | ______ | ______ |
| Akurasi level (%) | ______ | ______ |

**Pertanyaan:**
1. Apa kelebihan dan kekurangan DAC dibanding PWM+filter untuk menghasilkan tegangan analog?
2. Bagaimana cara mengurangi ripple pada output PWM yang difilter?
3. Dalam aplikasi apa DAC lebih tepat digunakan dan kapan PWM lebih cocok?

---

## 5. Tugas Tambahan

### Tugas 1: Waveform Generator
Buat program yang dapat menghasilkan 4 jenis gelombang (sinus, segitiga, kotak, sawtooth) melalui DAC dengan frekuensi yang dapat diatur via Serial Monitor. Tampilkan informasi gelombang aktif pada Serial Monitor.

### Tugas 2: LED Dimmer dengan Potentiometer
Hubungkan potentiometer ke pin ADC. Baca nilai ADC dan gunakan untuk mengontrol duty cycle PWM LED. Implementasikan gamma correction agar perubahan kecerahan terlihat linear.

### Tugas 3: Music Player
Buat program yang menyimpan minimal 3 melodi berbeda dan dapat dipilih melalui Serial Monitor. Implementasikan kontrol tempo dan volume (duty cycle).

### Tugas 4: Servo Position Recorder
Buat program yang dapat merekam beberapa posisi servo (via Serial Monitor), lalu memutar ulang (playback) urutan posisi tersebut secara otomatis dengan kecepatan yang dapat diatur.

---

## 6. Format Laporan

Laporan praktikum harus mencakup:

1. **Cover** — Judul, nama, NIM, tanggal
2. **Tujuan** — Tujuan praktikum
3. **Dasar Teori** — Ringkasan teori DAC dan PWM (dengan rumus)
4. **Alat dan Bahan** — Daftar komponen yang digunakan
5. **Langkah Kerja** — Prosedur yang dilakukan
6. **Data Pengamatan** — Tabel hasil pengukuran untuk setiap program
7. **Analisis** — Penjelasan hasil, perbandingan teori vs praktik
8. **Jawaban Pertanyaan** — Jawaban dari setiap pertanyaan per program
9. **Kesimpulan** — Rangkuman hasil pembelajaran
10. **Lampiran** — Screenshot Serial Monitor, foto rangkaian, kode program

---

## 7. Tips Debugging dengan Python

Gunakan script Python berikut untuk memvisualisasikan data dari Serial Monitor:

### Membaca dan Plot Data Serial

```python
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# Konfigurasi
PORT = '/dev/ttyUSB0'  # Sesuaikan dengan port board Anda
BAUD = 115200
MAX_POINTS = 500

ser = serial.Serial(PORT, BAUD, timeout=1)
data = deque(maxlen=MAX_POINTS)

fig, ax = plt.subplots()
line, = ax.plot([], [])
ax.set_ylim(0, 3.3)
ax.set_xlim(0, MAX_POINTS)
ax.set_xlabel('Sample')
ax.set_ylabel('Voltage (V)')
ax.set_title('DAC/PWM Output Monitor')

def update(frame):
    try:
        raw = ser.readline().decode().strip()
        if raw:
            value = float(raw)
            data.append(value)
            line.set_data(range(len(data)), list(data))
    except (ValueError, UnicodeDecodeError):
        pass
    return line,

ani = animation.FuncAnimation(fig, update, interval=10, blit=True)
plt.tight_layout()
plt.show()
ser.close()
```

### Membandingkan DAC vs PWM

```python
import serial
import matplotlib.pyplot as plt

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
dac_data = []
pwm_data = []

print("Collecting data... (10 seconds)")
import time
start = time.time()

while time.time() - start < 10:
    line = ser.readline().decode().strip()
    if ',' in line:
        try:
            dac, pwm = line.split(',')
            dac_data.append(float(dac))
            pwm_data.append(float(pwm))
        except ValueError:
            pass

ser.close()

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
ax1.plot(dac_data, label='DAC Output')
ax1.set_title('DAC vs PWM Comparison')
ax1.set_ylabel('DAC Voltage (V)')
ax1.legend()

ax2.plot(pwm_data, color='orange', label='PWM+Filter Output')
ax2.set_xlabel('Sample')
ax2.set_ylabel('PWM Voltage (V)')
ax2.legend()

plt.tight_layout()
plt.savefig('dac_vs_pwm.png', dpi=150)
plt.show()
```

---

## 8. Referensi

1. ESP32 Technical Reference Manual — DAC & LEDC Controller chapters
2. STM32F4 Reference Manual (RM0090) — DAC & Timer chapters
3. ESP-IDF LEDC API Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html
4. Servo Motor Control Theory — PWM 50Hz standard
5. RC Low-Pass Filter Design Calculator

---

> **Catatan Penting:**
> - ESP32 DAC hanya tersedia pada **GPIO25** (DAC1) dan **GPIO26** (DAC2) untuk varian **ESP32 WROOM/WROVER** saja
> - **ESP32-S2** dan **ESP32-S3** **tidak memiliki DAC**
> - **STM32F103** (Blue Pill) **tidak memiliki peripheral DAC** — untuk program DAC, gunakan STM32F411 atau skip ke program PWM
> - Selalu gunakan **common ground** antara board, sensor, dan perangkat eksternal
> - Frekuensi PWM untuk servo **harus 50Hz** — frekuensi lain dapat merusak servo
> - Motor DC memerlukan **power supply terpisah** — jangan ambil daya dari pin USB board
