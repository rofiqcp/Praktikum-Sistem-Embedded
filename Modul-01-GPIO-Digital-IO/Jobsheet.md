# JOBSHEET BAB 01: GPIO dan Digital I/O

**Program Studi:** Teknologi Rekayasa Otomasi  
**Mata Kuliah:** Sistem Embedded  
**Topik:** GPIO — General Purpose Input/Output  
**Platform:** STM32F103C8T6 Blue Pill & ESP32 DevKit V1  
**Durasi:** 3 × 50 menit

---

## 🎯 Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. **Mengkonfigurasi** pin GPIO sebagai output (Push-Pull & Open-Drain) dan input (Pull-UP/Pull-DOWN eksternal maupun internal) pada STM32 dan ESP32 menggunakan framework HAL dan ESP-IDF
2. **Menganalisis** perbedaan perilaku rangkaian Active-LOW vs Active-HIGH, serta efek resistor pull-up/pull-down terhadap level logika GPIO
3. **Mengimplementasikan** teknik debounce software berbasis state machine non-blocking dan mengintegrasikan periferal rotary encoder, keypad matrix 4×4, serta LCD I²C 16×2 dalam satu sistem digital I/O yang fungsional

---

## 📊 Capaian Pembelajaran Mata Kuliah (CPMK)

| CPMK | Deskripsi | Bobot |
|------|-----------|-------|
| **CPMK 1** | Mampu menjelaskan arsitektur GPIO, register BSRR/ODR/IDR, mode Push-Pull & Open-Drain, serta konsep Active-HIGH/LOW | 30% |
| **CPMK 2** | Mampu mengimplementasikan konfigurasi GPIO input (pull-up/down eksternal & internal) dengan debounce state machine dan membaca periferal encoder kuadratur maupun keypad matrix | 40% |
| **CPMK 3** | Mampu merancang dan mengintegrasikan sistem GPIO lengkap (LED, tombol, encoder, keypad, LCD I²C) pada dua platform berbeda (STM32 & ESP32) dan menganalisis perbandingannya | 30% |

---

## 🔧 Alat dan Bahan

### Hardware
| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 Blue Pill | 1 | MCU utama, 64 MHz Cortex-M3 |
| 2 | ESP32 DevKit V1 | 1 | MCU sekunder, 240 MHz dual-core |
| 3 | ST-Link V2 | 1 | Programmer/Debugger STM32 |
| 4 | LED 5mm (merah, hijau, kuning) | 8 | Indikator output |
| 5 | Resistor 220Ω | 16 | Current limiter LED & pull resistor |
| 6 | Push Button tactile | 4 | Input diskrit |
| 7 | Rotary Encoder 5-pin | 1 | CLK, DT, SW, +, GND |
| 8 | Keypad Matrix 4×4 (8-pin) | 1 | Input matriks 16 tombol |
| 9 | LCD I²C 16×2 | 1 | Modul PCF8574, alamat 0x27 |
| 10 | Breadboard full-size | 1 | Media prototyping |
| 11 | Kabel Jumper M-M | 30 | Koneksi |
| 12 | Kabel USB Micro | 2 | Power & upload |

### Software
| No | Software | Keterangan |
|----|----------|------------|
| 1 | Visual Studio Code + PlatformIO | IDE utama |
| 2 | ST-Link Utility | Flash & debug STM32 |
| 3 | PlatformIO Serial Monitor | Monitor output serial |

---

## 📐 Referensi Pin Assignment

### STM32F103C8T6 Blue Pill

| GPIO | Fungsi | Mode HAL | Percobaan |
|------|--------|----------|-----------|
| PA0–PA7 | LED 1–8 (Active-HIGH, 220Ω) | OUTPUT_PP | P01–P10 |
| PC13 | LED built-in (Active-LOW) | OUTPUT_PP | P01, P02 |
| PB0 | Push Button 1 | INPUT + PULL | P03–P08 |
| PB1 | Push Button 2 | INPUT + PULL | P04–P08 |
| PB3 | Push Button 3 | INPUT PULLUP | P07, P08 |
| PB4 | Push Button 4 | INPUT PULLUP | P07, P08 |
| PB6 | SCL (I2C1) — LCD | AF_OD | P01–P10 |
| PB7 | SDA (I2C1) — LCD | AF_OD | P01–P10 |
| PB8–PB11 | Keypad ROW 1–4 | OUTPUT_PP | P10 |
| PB12–PB15 | Keypad COL 1–4 | INPUT PULLUP | P10 |
| PB12 | Encoder CLK | INPUT PULLUP | P09 |
| PB13 | Encoder DT | INPUT PULLUP | P09 |
| PB14 | Encoder SW | INPUT PULLUP | P09 |

> ⚠️ **PB2 = BOOT1** — jangan gunakan untuk tombol!  
> ⚠️ **P09 dan P10** berbagi PB12–PB15 — tidak bisa dijalankan bersamaan.

### ESP32 DevKit V1

| GPIO | Fungsi | Mode ESP-IDF | Percobaan |
|------|--------|-------------|-----------|
| GPIO4,5,18,19,21,22,23,25 | LED 1–8 | OUTPUT | P01–P10 |
| GPIO2 | LED built-in (Active-HIGH) | OUTPUT | P01, P02 |
| GPIO32 | Push Button 1 | INPUT PULLUP | P05–P08 |
| GPIO33 | Push Button 2 | INPUT PULLDOWN | P06–P08 |
| GPIO14 | Push Button 3 | INPUT PULLUP | P07, P08 |
| GPIO27 | Push Button 4 | INPUT PULLUP | P07, P08 |
| GPIO34 | BTN ext pull-up (INPUT ONLY) | INPUT tanpa pull | P03 |
| GPIO35 | BTN ext pull-dn (INPUT ONLY) | INPUT tanpa pull | P04 |
| GPIO21 | SDA (I2C) — LCD | — | P01–P10 |
| GPIO22 | SCL (I2C) — LCD | — | P01–P10 |
| GPIO26 | Encoder CLK | INPUT PULLUP | P09 |
| GPIO27 | Encoder DT | INPUT PULLUP | P09 |
| GPIO14 | Encoder SW | INPUT PULLUP | P09 |
| GPIO13,15,16,17 | Keypad ROW 1–4 | OUTPUT | P10 |
| GPIO32,33,25,26 | Keypad COL 1–4 | INPUT PULLUP | P10 |

---

## 📋 Daftar Percobaan

| No | Judul | Konsep Utama |
|----|-------|-------------|
| **P01** | **LED Parade** — GPIO Output Push-Pull & Pola Cahaya Digital | Output PP, Active-HIGH, ODR, 4 pola |
| **P02** | **Shadow & Ghost** — Active-LOW, Open-Drain & Logika Terbalik | Output OD, Active-LOW, Hi-Z, arus OD |
| **P03** | **Sentinel Gate** — Tombol Pull-UP Eksternal (220Ω ke 3.3V) | Input NOPULL, external pull-up, edge detect |
| **P04** | **Ground Guardian** — Tombol Pull-DOWN Eksternal (220Ω ke GND) | Input NOPULL, external pull-down, rising edge |
| **P05** | **Phantom Touch** — Pull-UP Internal & Tombol Tanpa Resistor | Input + PULLUP ~40kΩ, no external resistor |
| **P06** | **Force Field** — Pull-DOWN Internal & Logika Active-HIGH | Input + PULLDOWN ~40kΩ, no external resistor |
| **P07** | **Clean Contact** — Debounce State Machine & Penghitung Akurat | State machine, HAL_GetTick, 4 tombol |
| **P08** | **Speed Racer** — GPIO Slew Rate & Pola LED Multi-Kecepatan | FREQ_LOW/MED/HIGH, runtime GPIO init |
| **P09** | **Twist & Count** — Rotary Encoder Kuadratur & Counter LCD | CLK/DT edge detect, CW/CCW, BSRR |
| **P10** | **Matrix Commander** — Pemindaian Keypad 4×4 & Tampilan LCD | Row/col scanning, KEYMAP, INPUT PULLUP |

---

## 🔬 Langkah Kerja

### Persiapan Umum

1. Buka VS Code + PlatformIO, buka folder `Modul-01-GPIO-Digital-IO/praktikum/`
2. Sambungkan ST-Link V2 ke Blue Pill (SWCLK, SWDIO, GND, 3.3V)
3. Sambungkan Blue Pill via USB untuk power
4. Pasang LCD I²C: VCC→3.3V, GND→GND, **SCL→PB6**, **SDA→PB7**
5. Pasang 8 LED dengan resistor 220Ω seri ke PA0–PA7 (anoda→pin, katoda→GND)

---

### 🧪 Percobaan 1 — LED Parade

**Judul:** LED Parade — GPIO Output Push-Pull & Pola Cahaya Digital

**Tujuan:** Mengkonfigurasi PA0–PA7 sebagai output Push-Pull dan menampilkan 4 pola LED berbeda, serta memahami konsep Active-HIGH dan Active-LOW pada PC13.

**Rangkaian:**
- PA0–PA7 → [220Ω] → LED+ → LED– → GND *(Active-HIGH)*
- PC13 → [220Ω] → LED+ → LED– → GND *(Active-LOW, built-in)*
- LCD I²C: SCL=PB6, SDA=PB7

**Langkah:**
1. Buka folder `STM32_P01_LED_Output_High/`, lakukan Build & Upload (`Ctrl+Alt+U`)
2. Amati 4 pola yang berganti otomatis tiap 2 detik
3. Perhatikan LCD menampilkan nama pola dan delay saat ini
4. Perhatikan LED PC13 built-in — logikanya terbalik (menyala saat LOW)
5. Ulangi pada ESP32: buka `ESP32_P01_LED_Output_High/`, upload, amati perbedaan GPIO2

**Hasil Pengamatan:**

| Pola | Deskripsi | Nilai ODR PA (hex) | LCD Baris 1 |
|------|-----------|-------------------|-------------|
| ALL ON | 8 LED menyala semua | `0xFF` | |
| ALL OFF | 8 LED mati | `0x00` | |
| Running Light | 1 LED berjalan | `0x01→0x02→...` | |
| Binary Counter | Counter 0–255 | `0x00→0x01→...→0xFF` | |

**Pertanyaan Analisa:**
1. Mengapa PC13 (LED built-in) logikanya terbalik dibanding PA0–PA7?
2. Saat Binary Counter menunjukkan nilai 0xAB (171 desimal), LED mana yang menyala? Tuliskan dalam biner!
3. Apa perbedaan fungsional antara `HAL_GPIO_WritePin()` dan `GPIOA->ODR` untuk output?

---

### 🧪 Percobaan 2 — Shadow & Ghost

**Judul:** Shadow & Ghost — Active-LOW, Open-Drain & Logika Terbalik

**Tujuan:** Memahami perbedaan Output Push-Pull vs Open-Drain dan membuktikan efek Hi-Z pada kecerahan LED.

**Rangkaian:**
- PA0: OUTPUT_PP + NOPULL → [220Ω] → LED hijau → GND
- PA1: OUTPUT_OD + PULLUP (40kΩ internal) → [220Ω] → LED oranye → GND
- PA3: OUTPUT_OD tanpa pull-up → [220Ω] → LED ungu → GND *(LED mati saat Hi-Z)*
- PC13: OUTPUT_PP Active-LOW → [220Ω] → LED merah → GND

**Langkah:**
1. Upload `STM32_P02_LED_Output_Low/`
2. **Phase 1 (2 detik):** PA0 dan PA1 = HIGH — bandingkan kecerahan LED
3. **Phase 2 (2 detik):** PA0 dan PA1 = LOW — bandingkan kecerahan
4. **Phase 3 (2 detik):** PA3 (OD no pull-up) = HIGH → amati LED mati (Hi-Z)
5. Amati LCD menampilkan mode dan keterangan aktif

**Hasil Pengamatan:**

| Kondisi Output | PA0 PP (hijau) | PA1 OD+PullUP (oranye) | PA3 OD No-PU (ungu) |
|----------------|---------------|------------------------|---------------------|
| Output = HIGH | LED \_\_\_ | LED \_\_\_ | LED \_\_\_ |
| Output = LOW  | LED \_\_\_ | LED \_\_\_ | LED \_\_\_ |

Estimasi arus PA1 saat HIGH (Hi-Z) dengan pull-up 40kΩ:  
$I = \dfrac{3.3 - 2.0}{40000 + 220}$ = \_\_\_ mA

**Pertanyaan Analisa:**
1. Jelaskan mengapa Open-Drain tidak bisa mengeluarkan HIGH tanpa resistor pull-up!
2. Dalam aplikasi industri, kapan sebaiknya menggunakan Active-LOW dibanding Active-HIGH?

---

### 🧪 Percobaan 3 — Sentinel Gate

**Judul:** Sentinel Gate — Tombol Pull-UP Eksternal (220Ω ke 3.3V)

**Tujuan:** Memahami fungsi pull-up eksternal dan deteksi falling edge pada GPIO input NOPULL.

**Rangkaian:**
```
3.3V ──[220Ω]── NODE ── PB0 (INPUT, NOPULL)
                  │
             [BTN1]
                  │
                 GND
```

**Langkah:**
1. Pasang rangkaian sesuai skematik P03
2. Upload `STM32_P03_Button_PullUp_Ext/`
3. Tekan BTN1 — amati LED PA0 toggle dan LCD update (press count)
4. Ukur tegangan NODE saat BTN **lepas** dan BTN **ditekan** dengan multimeter

**Hasil Pengamatan:**

| Kondisi BTN | Tegangan NODE (V) | Level PB0 | LED PA0 | Press Count |
|-------------|-------------------|-----------|---------|-------------|
| Lepas | \_\_\_ V | HIGH / LOW | ON / OFF | — |
| Ditekan | \_\_\_ V | HIGH / LOW | ON / OFF | +1 |

Hitung arus pull-up saat BTN ditekan: $I = \dfrac{3.3}{220}$ = \_\_\_ mA

**Pertanyaan Analisa:**
1. Apa yang terjadi jika resistor pull-up tidak dipasang dan BTN langsung ke GND?
2. Bandingkan kekuatan pull (220Ω ext) dengan pull internal (40kΩ). Mana lebih tahan noise?

---

### 🧪 Percobaan 4 — Ground Guardian

**Judul:** Ground Guardian — Tombol Pull-DOWN Eksternal (220Ω ke GND)

**Tujuan:** Memahami fungsi pull-down eksternal dan deteksi rising edge pada GPIO input NOPULL.

**Rangkaian:**
```
3.3V ──[BTN2]── NODE ── PB1 (INPUT, NOPULL)
                  │
             [220Ω]
                  │
                 GND
```

**Langkah:**
1. Pasang rangkaian sesuai skematik P04
2. Upload `STM32_P04_Button_PullDown_Ext/`
3. Tekan BTN2 — amati LED PA1 toggle dan LCD update
4. Ukur tegangan NODE saat BTN lepas dan ditekan

**Hasil Pengamatan:**

| Kondisi BTN | Tegangan NODE (V) | Level PB1 | LED PA1 | Edge |
|-------------|-------------------|-----------|---------|------|
| Lepas | \_\_\_ V | \_\_\_ | \_\_\_ | — |
| Ditekan | \_\_\_ V | \_\_\_ | \_\_\_ | Rising/Falling |

**Pertanyaan Analisa:**
1. Mengapa rising edge digunakan untuk deteksi pada konfigurasi pull-down?
2. Apa bahayanya jika sumber tegangan yang terhubung ke BTN melebihi 3.3V pada STM32?

---

### 🧪 Percobaan 5 — Phantom Touch

**Judul:** Phantom Touch — Pull-UP Internal & Tombol Tanpa Resistor

**Tujuan:** Memanfaatkan pull-up resistor internal ~40kΩ tanpa memerlukan resistor eksternal.

**Rangkaian:**
```
PB0 ──[BTN1]── GND   (LANGSUNG — tidak perlu resistor!)
PB1 ──[BTN2]── GND
GPIO mode: INPUT + GPIO_PULLUP (~40kΩ internal aktif)
```

**Langkah:**
1. Pasang tombol langsung: satu kaki ke PB0/PB1, satu kaki ke GND — **tanpa resistor**
2. Upload `STM32_P05_Button_PullUp_Internal/`
3. Tekan BTN1 dan BTN2 — amati LED toggle, LCD update
4. Bandingkan responsivitas dengan P03

**Hasil Pengamatan:**

| Aspek | P03 (Ext 220Ω) | P05 (Int 40kΩ) |
|-------|---------------|----------------|
| Komponen tambahan | Resistor 220Ω | Tidak perlu |
| Arus pull saat idle | ~15 mA | ~0.08 mA |
| Responsivitas | \_\_\_ | \_\_\_ |
| Bounce teramati (Y/N) | \_\_\_ | \_\_\_ |

**Pertanyaan Analisa:**
1. Dalam kondisi lingkungan ber-noise tinggi (industri), mana yang lebih aman — pull-up eksternal atau internal?
2. Tuliskan kode HAL untuk inisialisasi PB0 sebagai input pull-up internal!

---

### 🧪 Percobaan 6 — Force Field

**Judul:** Force Field — Pull-DOWN Internal & Logika Active-HIGH

**Tujuan:** Memanfaatkan pull-down resistor internal ~40kΩ untuk logika Active-HIGH tanpa resistor eksternal.

**Rangkaian:**
```
3.3V ──[BTN1]── PB1   (LANGSUNG — tidak perlu resistor!)
3.3V ──[BTN3]── PB3
GPIO mode: INPUT + GPIO_PULLDOWN (~40kΩ internal ke GND)
```

> ⚠️ Jangan hubungkan 5V ke pin GPIO STM32 — max 3.3V!  
> ⚠️ Skip PB2 = BOOT1 pada Blue Pill.

**Langkah:**
1. Pasang tombol langsung: satu kaki ke PB1/PB3, satu kaki ke 3.3V
2. Upload `STM32_P06_Button_PullDown_Internal/`
3. Tekan BTN — amati LED toggle, LCD update
4. Bandingkan dengan P05: catat perbedaan logika

**Hasil Pengamatan:**

| | P05 Pull-UP Internal | P06 Pull-DOWN Internal |
|--|---------------------|----------------------|
| Level default (idle) | HIGH | LOW |
| Level saat ditekan | LOW | HIGH |
| Jenis logika | Active-LOW | Active-HIGH |
| Edge yang dideteksi | Falling | Rising |

**Pertanyaan Analisa:**
1. Apakah bisa mengaktifkan PULLUP dan PULLDOWN bersamaan pada satu pin? Mengapa?
2. PULLUP vs PULLDOWN internal: mana lebih umum digunakan dalam embedded sistem dan mengapa?

---

### 🧪 Percobaan 7 — Clean Contact

**Judul:** Clean Contact — Debounce State Machine & Penghitung Akurat

**Tujuan:** Mengimplementasikan debounce berbasis state machine non-blocking untuk 4 tombol independen.

**Rangkaian:**
- PB0, PB1, PB3, PB4 masing-masing → [BTNx] → GND *(INPUT PULLUP)*
- PA0–PA3: 4 LED output, LCD I²C

**Langkah:**
1. Pasang 4 tombol ke PB0, PB1, PB3, PB4 → GND
2. Upload `STM32_P07_Button_Debounce/`
3. Tekan masing-masing tombol 10× dengan cepat — catat press count di LCD
4. Ubah `DEBOUNCE_MS = 5` (di kode), rebuild, tekan tombol — amati bounce
5. Kembalikan ke `DEBOUNCE_MS = 50`, rebuild — amati stabilitas

**Hasil Pengamatan:**

| Pengujian | DEBOUNCE_MS | Count per 10 tekan | Konsisten? |
|-----------|-------------|-------------------|-----------|
| Normal | 50 ms | \_\_\_ | Ya / Tidak |
| Terlalu singkat | 5 ms | \_\_\_ | Ya / Tidak |
| Terlalu lama | 200 ms | \_\_\_ | Ya / Tidak |

**Pertanyaan Analisa:**
1. Gambarkan diagram state machine debounce yang diimplementasikan!
2. Mengapa `HAL_Delay(50)` tidak tepat digunakan untuk debounce di sistem real-time?
3. Apa peran `HAL_GetTick()` dalam implementasi non-blocking ini?

---

### 🧪 Percobaan 8 — Speed Racer

**Judul:** Speed Racer — GPIO Slew Rate & Pola LED Multi-Kecepatan

**Tujuan:** Memahami konsep GPIO Slew Rate dan mengimplementasikan perubahan speed GPIO saat runtime.

**Rangkaian:** PB0–PB4 tombol (INPUT PULLUP), PA0–PA7 LED (220Ω), LCD I²C

**Langkah:**
1. Upload `STM32_P08_LED_Patterns/`
2. Tekan **BTN1**: Running Light (SPEED_LOW — 2MHz)
3. Tekan **BTN2**: Knight Rider ping-pong (SPEED_MEDIUM — 10MHz)
4. Tekan **BTN3**: Alternating Flash (SPEED_HIGH — 50MHz)
5. Tekan **BTN4**: Binary Counter (SPEED_MEDIUM)
6. Amati LCD menampilkan speed aktif dan nama pola

**Hasil Pengamatan:**

| BTN | Pola | GPIO_SPEED | Slew Rate | Perbedaan Visual |
|----|------|------------|-----------|-----------------|
| BTN1 | Running Light | FREQ_LOW | 2 MHz | |
| BTN2 | Knight Rider | FREQ_MEDIUM | 10 MHz | |
| BTN3 | Alternating | FREQ_HIGH | 50 MHz | |
| BTN4 | Binary Counter | FREQ_MEDIUM | 10 MHz | |

**Pertanyaan Analisa:**
1. Apakah perbedaan visual antara SPEED_LOW dan SPEED_HIGH terlihat jelas pada LED? Mengapa?
2. Dalam kasus nyata apa GPIO Speed tinggi benar-benar diperlukan? Berikan 2 contoh!
3. Apa risiko menggunakan GPIO Speed HIGH pada semua pin secara sembarangan?

---

### 🧪 Percobaan 9 — Twist & Count

**Judul:** Twist & Count — Rotary Encoder Kuadratur & Counter LCD

**Tujuan:** Membaca encoder 5-pin menggunakan deteksi edge kuadratur dan menampilkan count di LCD.

**Rangkaian:** GND→GND, +→3.3V, **CLK→PB12**, **DT→PB13**, **SW→PB14** *(semua INPUT PULLUP)*  
LED binary PA0–PA7, LCD I²C

**Langkah:**
1. Sambungkan encoder 5-pin sesuai pin assignment
2. Upload `STM32_P09_Encoder_5Pin/`
3. Putar **Clockwise (CW)** — amati count naik di LCD dan LED biner
4. Putar **Counter-Clockwise (CCW)** — amati count turun
5. Tekan **SW encoder** — amati count reset ke 128
6. Putar cepat 20× CW — catat count terakhir

**Hasil Pengamatan:**

| Aksi | Count Awal | Count Akhir | Perubahan |
|------|-----------|------------|-----------|
| Putar CW 10× perlahan | 128 | \_\_\_ | \_\_\_ |
| Putar CCW 5× | \_\_\_ | \_\_\_ | \_\_\_ |
| Tekan SW | \_\_\_ | \_\_\_ | Reset ke 128 |
| Putar CW cepat 20× | 128 | \_\_\_ | \_\_\_ |

**Pertanyaan Analisa:**
1. Gambarkan timing diagram CLK dan DT untuk rotasi CW dan CCW!
2. Mengapa menggunakan `GPIOA->BSRR` dan bukan `HAL_GPIO_TogglePin()` untuk update 8 LED sekaligus?
3. Apa yang terjadi jika hanya CLK dibaca tanpa DT? Apakah masih bisa membedakan arah rotasi?

---

### 🧪 Percobaan 10 — Matrix Commander

**Judul:** Matrix Commander — Pemindaian Keypad 4×4 & Tampilan LCD

**Tujuan:** Mengimplementasikan algoritma scanning keypad matrix 4×4 menggunakan ROW-COL method.

**Rangkaian:**
- ROW 1–4 → PB8–PB11 *(OUTPUT_PP)*
- COL 1–4 → PB12–PB15 *(INPUT PULLUP)*
- LED binary PA0–PA7, LCD I²C

**Langkah:**
1. Sambungkan keypad: Row1-4 ke PB8-PB11, Col1-4 ke PB12-PB15
2. Upload `STM32_P10_Keypad_8Pin/`
3. Tekan setiap tombol satu per satu — catat karakter di LCD
4. Tekan tombol **1**, **5**, **9**, **D** — catat nilai biner pada LED PA0–PA7
5. Tekan dan tahan satu tombol — amati repeat detection

**Hasil Pengamatan:**

| Tombol Fisik | Karakter LCD | Nilai Biner LED (PA7–PA0) |
|-------------|--------------|--------------------------|
| 1 | \_\_\_ | \_\_\_\_\_\_\_\_ |
| 5 | \_\_\_ | \_\_\_\_\_\_\_\_ |
| A | \_\_\_ | \_\_\_\_\_\_\_\_ |
| # | \_\_\_ | \_\_\_\_\_\_\_\_ |
| D | \_\_\_ | \_\_\_\_\_\_\_\_ |

**Pertanyaan Analisa:**
1. Jelaskan algoritma scanning keypad 4×4 secara rinci dalam pseudocode!
2. Mengapa semua ROW di-set HIGH terlebih dahulu sebelum menurunkan satu ROW ke LOW?
3. Apakah sistem dapat mendeteksi dua tombol ditekan bersamaan? Jelaskan!

---

## 📊 Tabel Komparatif STM32 vs ESP32

Setelah menjalankan percobaan pada STM32, ulangi pada ESP32 dan isi tabel:

| Aspek | STM32 (HAL) | ESP32 (ESP-IDF) |
|-------|-------------|-----------------|
| Fungsi init GPIO | `HAL_GPIO_Init()` | `gpio_config()` |
| Pin pull-up | `GPIO_PULLUP` (dalam init) | `GPIO_PULLUP_ENABLE` |
| Set output HIGH | `HAL_GPIO_WritePin(...SET)` | `gpio_set_level(pin, 1)` |
| Set output LOW | `HAL_GPIO_WritePin(...RESET)` | `gpio_set_level(pin, 0)` |
| Baca input | `HAL_GPIO_ReadPin()` | `gpio_get_level()` |
| Drive strength | `GPIO_SPEED_FREQ_xxx` | `gpio_set_drive_capability()` |
| Pin INPUT ONLY | Tidak ada | GPIO34–39 |
| Atomic bit set | `GPIOA->BSRR = bit` | `GPIO.out_w1ts = bit` |
| Atomic bit clear | `GPIOA->BSRR = bit<<16` | `GPIO.out_w1tc = bit` |
| SCL/SDA LCD | PB6 / PB7 | GPIO22 / GPIO21 |
| LED built-in | PC13 Active-LOW | GPIO2 Active-HIGH |

---

## 📝 Analisa

Jawab pertanyaan berikut berdasarkan seluruh percobaan:

1. **Bandingkan** P03, P04, P05, P06 dari segi kekuatan pull resistor, konsumsi arus, dan kemudahan implementasi. Kapan masing-masing tepat digunakan dalam sistem otomasi?

2. **Jelaskan** mengapa debounce diperlukan dan mengapa state machine (P07) lebih baik dari `HAL_Delay()` dalam konteks sistem embedded real-time?

3. **Analisa** perbedaan GPIO Speed STM32 (slew rate) dengan Drive Capability ESP32. Apakah keduanya mengukur hal yang sama?

4. **Dari P09**, jelaskan algoritma decoding kuadratur. Apa yang terjadi jika encoder diputar sangat cepat (melebih kemampuan polling)?

5. **Dari P10**, berikan skenario dua tombol keypad ditekan bersamaan. Apa yang terjadi? Bagaimana solusi hardware-nya?

---

## ✅ Kesimpulan

Tuliskan kesimpulan mencakup:

1. Perbedaan GPIO output mode **Push-Pull** dan **Open-Drain** beserta implikasinya terhadap level tegangan dan arus
2. Perbedaan pull-up/down **eksternal vs internal** dan kapan masing-masing lebih tepat digunakan
3. Pentingnya **debounce** dan keunggulan state machine non-blocking dibanding delay blocking
4. Fungsi **GPIO Speed (slew rate)** dan pengaruhnya pada konsumsi arus serta EMI
5. Cara kerja **encoder kuadratur** dan **keypad matrix scanning**
6. Perbandingan implementasi GPIO antara **STM32 HAL** dan **ESP32 ESP-IDF**

---

## 📚 Daftar Pustaka

1. Ferretti, C. (2020). *Mastering STM32, 2nd Edition*, Bab 6: GPIO Management. Leanpub.
2. Kolban, N. (2018). *Kolban's Book on ESP32*, Chapter: GPIO. Self-published.
3. STMicroelectronics. (2021). *STM32F103x8/xB Reference Manual (RM0008)*. ST, Rev 21.
4. Espressif Systems. (2024). *ESP32 Technical Reference Manual v5.3*. Espressif.
5. Ganssle, J. (2008). *The Art of Designing Embedded Systems, 2nd Ed.* Newnes.
6. Williams, J. (1990). "Signal Sources, Conditioners, and Power Circuitry." *Linear Technology AN26*.
