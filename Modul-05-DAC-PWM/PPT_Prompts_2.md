# 🎨 Prompt Slide Presentasi — Modul 05: Menghidupkan Sinyal — DAC & PWM (Bagian 2: Slide 21-40)

## Instruksi Umum

Prompt ini merupakan lanjutan dari Bagian 1 (Slide 1-20) yang membahas teori DAC. Bagian 2 fokus pada PWM: teori, duty cycle, LEDC ESP32, Timer PWM STM32, kontrol servo, motor, LED RGB, serta perbandingan DAC vs PWM.

---

## 📋 Prompt Utama

```
Buatkan presentasi PowerPoint dalam Bahasa Indonesia untuk mata kuliah Praktikum Sistem Embedded dengan topik "PWM (Pulse Width Modulation): Teori, Implementasi, dan Aplikasi" sebanyak 20 slide (Bagian 2 dari 2, Slide 21-40). Gunakan desain yang konsisten dengan Bagian 1 (tema warna hijau-abu-abu, profesional). Target audiens: mahasiswa Teknik Elektro/Informatika.

SLIDE 21: Halaman Pembuka Bagian 2
- Judul: "Bagian 2: PWM - Pulse Width Modulation"
- Subtitle: "Teori, LEDC, Timer, Servo, RGB LED, dan Aplikasi"
- Daftar topik:
  1. Prinsip Kerja PWM
  2. Parameter PWM (Frekuensi, Duty Cycle, Resolusi)
  3. LEDC pada ESP32
  4. Timer PWM pada STM32
  5. Kontrol Servo Motor
  6. Kontrol LED RGB
  7. Melody/Buzzer dengan PWM
  8. Perbandingan DAC vs PWM

SLIDE 22: Prinsip Dasar PWM
- Judul: "Prinsip Dasar Pulse Width Modulation"
- Definisi: Teknik modulasi lebar pulsa untuk mengatur daya rata-rata
- Diagram sinyal PWM: pulsa HIGH dan LOW dengan periode tetap
- Konsep kunci:
  • Frekuensi (f): Jumlah siklus per detik (Hz)
  • Periode (T): Waktu satu siklus penuh (T = 1/f)
  • Duty Cycle (D): Rasio waktu HIGH terhadap periode (%)
  • D = t_on / T × 100%
- Diagram 3 contoh: Duty 25%, 50%, 75%
- Tegangan rata-rata: Vavg = D × Vcc

SLIDE 23: Duty Cycle dan Tegangan Rata-Rata
- Judul: "Duty Cycle: Mengatur Daya Output"
- Tabel duty cycle dan tegangan rata-rata (Vcc = 3.3V):
  | Duty Cycle | t_on (T=20ms) | Vavg | Aplikasi |
  |:----------:|:-------------:|:----:|----------|
  | 0% | 0 ms | 0V | LED mati |
  | 25% | 5 ms | 0.825V | LED redup |
  | 50% | 10 ms | 1.65V | LED sedang |
  | 75% | 15 ms | 2.475V | LED terang |
  | 100% | 20 ms | 3.3V | LED full brightness |
- Diagram visual: Sinyal PWM di atas, tegangan rata-rata di bawah (garis horizontal)
- Penekanan: "PWM tidak menghasilkan tegangan analog sesungguhnya, melainkan meniru efeknya melalui switching cepat"

SLIDE 24: Frekuensi PWM
- Judul: "Pemilihan Frekuensi PWM"
- Pengaruh frekuensi terhadap aplikasi:
  | Aplikasi | Frekuensi Ideal | Alasan |
  |----------|:--------------:|--------|
  | LED Dimming | 1-10 kHz | Di atas flicker threshold mata (~100Hz) |
  | Motor DC | 10-25 kHz | Di atas frekuensi audible (~20kHz) |
  | Servo Motor | **50 Hz** | Standar industri servo RC |
  | Audio/Buzzer | Sesuai nada | Nada C4 = 262 Hz, A4 = 440 Hz |
  | PWM-DAC (analog) | 40-100 kHz | Semakin tinggi → ripple semakin kecil |
- Trade-off: Frekuensi tinggi → switching loss meningkat, resolusi bisa berkurang
- ⚠ Servo HARUS 50 Hz, frekuensi lain dapat merusak servo!

SLIDE 25: Resolusi PWM
- Judul: "Resolusi PWM: Berapa Bit?"
- Resolusi menentukan kehalusan kontrol duty cycle
- Tabel resolusi PWM:
  | Resolusi | Level Duty | Step | Contoh |
  |:--------:|:----------:|:----:|--------|
  | 8-bit | 256 | 0.39% | Arduino analogWrite |
  | 10-bit | 1024 | 0.098% | LEDC ESP32 |
  | 13-bit | 8192 | 0.012% | LEDC ESP32 (max@1kHz) |
  | 16-bit | 65536 | 0.0015% | STM32 Timer (ARR=65535) |
- Hubungan frekuensi dan resolusi: Resolusi_max = log2(f_clk / f_PWM)
- Contoh ESP32: f_clk=80MHz, f_PWM=1kHz → max ≈ 16 bit
- Contoh ESP32: f_clk=80MHz, f_PWM=40kHz → max ≈ 11 bit

SLIDE 26: LEDC pada ESP32 - Arsitektur
- Judul: "ESP32 LEDC: LED Control PWM"
- Arsitektur LEDC ESP32:
  • 16 channel PWM independen (8 high-speed + 8 low-speed)
  • Setiap channel terhubung ke salah satu dari 4 timer
  • Clock source: APB_CLK (80 MHz) atau REF_TICK (1 MHz)
  • Resolusi: 1-bit sampai 20-bit (tergantung frekuensi)
  • Hardware fade: Fade otomatis tanpa CPU
- Diagram blok: Timer → Channel → GPIO
- Konfigurasi: Timer (frekuensi + resolusi), Channel (timer + GPIO + duty)
- Keunggulan: Banyak channel, hardware fade, fleksibel

SLIDE 27: LEDC ESP32 - Kode Implementasi
- Judul: "Implementasi LEDC ESP32 (Arduino Framework)"
- Kode contoh LED fade:
  ```cpp
  // Konfigurasi LEDC
  const int ledPin = 16;
  const int channel = 0;
  const int freq = 5000;      // 5 kHz
  const int resolution = 8;   // 8-bit (0-255)
  
  // Setup
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(ledPin, channel);
  
  // Set duty cycle
  ledcWrite(channel, 128);   // 50% duty
  
  // Fade effect
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(channel, duty);
    delay(10);
  }
  ```
- Penjelasan: ledcSetup → ledcAttachPin → ledcWrite
- Catatan API ESP-IDF: ledc_timer_config(), ledc_channel_config()

SLIDE 28: LEDC ESP32 - Hardware Fade
- Judul: "ESP32 LEDC: Hardware Fade (Tanpa CPU)"
- Fitur hardware fade LEDC:
  • Fade otomatis dari duty awal ke duty target
  • Waktu fade dapat dikonfigurasi
  • Tidak membebani CPU (berjalan di background)
  • Mode: LEDC_FADE_NO_WAIT, LEDC_FADE_WAIT_DONE
- Kode contoh:
  ```cpp
  ledcFadeWithTime(channel, 0, 255, 2000);   // Fade 0→255 dalam 2 detik
  ledcFadeWithTime(channel, 255, 0, 2000);   // Fade 255→0 dalam 2 detik
  ```
- Kegunaan: LED breathing effect, transisi halus, hemat CPU
- Diagram timing: Perubahan duty cycle bertahap secara otomatis

SLIDE 29: Timer PWM pada STM32 - Arsitektur
- Judul: "PWM pada STM32: Timer Architecture"
- Arsitektur Timer STM32F411:
  • **TIM1**: Advanced Timer (complementary output, dead time)
  • **TIM2, TIM5**: 32-bit General Purpose Timer
  • **TIM3, TIM4**: 16-bit General Purpose Timer
  • **TIM9-TIM11**: 16-bit General Purpose (terbatas)
  • Clock: APB1 (up to 50 MHz) atau APB2 (up to 100 MHz)
- Diagram blok Timer: Prescaler → Counter → Compare → Output (OCx)
- Konfigurasi PWM: Prescaler (PSC), Auto-Reload (ARR), Compare (CCRx)
- Rumus frekuensi: f_PWM = f_clk / ((PSC+1) × (ARR+1))

SLIDE 30: Timer PWM STM32 - Mode PWM
- Judul: "STM32 Timer: PWM Mode 1 vs Mode 2"
- PWM Mode 1: Output HIGH saat counter < CCR, LOW saat counter ≥ CCR
- PWM Mode 2: Kebalikan dari Mode 1
- Diagram timing kedua mode:
  • Counter naik dari 0 ke ARR (upcounting)
  • Perbandingan dengan CCR menghasilkan output PWM
- Duty cycle = CCR / (ARR+1) × 100%
- Contoh: ARR=999, CCR=500 → Duty = 50%
- Edge-aligned vs Center-aligned mode
- Edge-aligned: Counter 0→ARR→0→ARR... (satu slope)
- Center-aligned: Counter 0→ARR→0→ARR... (dua slope, simetris)

SLIDE 31: Timer PWM STM32 - Kode Implementasi
- Judul: "Implementasi PWM STM32 (HAL Library)"
- Kode contoh:
  ```c
  TIM_HandleTypeDef htim3;
  TIM_OC_InitTypeDef sConfigOC;
  
  // Timer config: f_PWM = 84MHz / (84 × 1000) = 1 kHz
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;        // PSC = 83
  htim3.Init.Period = 999;           // ARR = 999
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  HAL_TIM_PWM_Init(&htim3);
  
  // PWM channel config
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;             // CCR = 500 → 50% duty
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
  
  // Start PWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  
  // Update duty cycle
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 750); // 75% duty
  ```
- Penjelasan setiap parameter dan register

SLIDE 32: Kontrol Servo Motor - Teori
- Judul: "Kontrol Servo Motor dengan PWM"
- Prinsip kerja servo motor:
  • Motor DC + Gearbox + Potensiometer + Controller
  • Posisi ditentukan oleh lebar pulsa (pulse width)
  • Sinyal PWM: Frekuensi **50 Hz** (periode 20 ms)
- Tabel pulse width vs sudut:
  | Pulse Width | Sudut | Duty Cycle |
  |:-----------:|:-----:|:----------:|
  | 500 µs | 0° | 2.5% |
  | 1000 µs | 45° | 5.0% |
  | 1500 µs | 90° (tengah) | 7.5% |
  | 2000 µs | 135° | 10.0% |
  | 2500 µs | 180° | 12.5% |
- Diagram timing sinyal servo
- ⚠ Jangan memutar servo melebihi batas mekanis!

SLIDE 33: Kontrol Servo - Implementasi
- Judul: "Implementasi Kontrol Servo"
- Kode ESP32 (LEDC):
  ```cpp
  // Servo pada GPIO19, 50Hz, 16-bit resolusi
  ledcSetup(3, 50, 16);  // Channel 3, 50Hz, 16-bit
  ledcAttachPin(19, 3);
  
  // Konversi sudut ke duty value
  int angleToDuty(int angle) {
    // 500us-2500us mapped to 16-bit (0-65535) at 50Hz
    int pulseUs = map(angle, 0, 180, 500, 2500);
    return (pulseUs * 65536) / 20000;
  }
  ledcWrite(3, angleToDuty(90)); // Set 90°
  ```
- Kode STM32: Timer PWM 50Hz dengan ARR dan CCR
- Kontrol via potensiometer: ADC → map → servo angle

SLIDE 34: Kontrol LED RGB dengan PWM
- Judul: "Kontrol LED RGB: Pencampuran Warna dengan PWM"
- Prinsip: 3 channel PWM independen untuk Red, Green, Blue
- Tabel warna dasar:
  | Warna | R (duty%) | G (duty%) | B (duty%) |
  |-------|:---------:|:---------:|:---------:|
  | Merah | 100% | 0% | 0% |
  | Hijau | 0% | 100% | 0% |
  | Biru | 0% | 0% | 100% |
  | Kuning | 100% | 100% | 0% |
  | Cyan | 0% | 100% | 100% |
  | Magenta | 100% | 0% | 100% |
  | Putih | 100% | 100% | 100% |
  | Oranye | 100% | 50% | 0% |
- Kode: 3 channel LEDC/Timer, masing-masing mengendalikan satu pin LED
- Efek rainbow: Transisi HSV → RGB

SLIDE 35: Efek LED RGB - Rainbow & Breathing
- Judul: "Efek LED RGB: Rainbow dan Breathing"
- Algoritma Rainbow:
  • Iterasi hue dari 0 sampai 360
  • Konversi HSV (hue, 100%, 100%) ke RGB
  • Set duty cycle masing-masing channel
  ```cpp
  void hsvToRgb(int h, int s, int v, int &r, int &g, int &b) {
    // Konversi HSV ke RGB (0-255 per channel)
  }
  for (int hue = 0; hue < 360; hue++) {
    hsvToRgb(hue, 100, 100, r, g, b);
    ledcWrite(chR, r); ledcWrite(chG, g); ledcWrite(chB, b);
    delay(20);
  }
  ```
- Algoritma Breathing: Sine wave pada brightness → fade in/out halus
- Diagram warna: Color wheel dengan label hue

SLIDE 36: Melody Player dengan PWM
- Judul: "Melody Player: Memainkan Nada dengan PWM"
- Prinsip: Mengubah frekuensi PWM sesuai nada musik
- Duty cycle 50% untuk volume maksimal pada buzzer pasif
- Tabel nada:
  | Nada | Frekuensi | Nada | Frekuensi |
  |------|:---------:|------|:---------:|
  | C4 | 262 Hz | G4 | 392 Hz |
  | D4 | 294 Hz | A4 | 440 Hz |
  | E4 | 330 Hz | B4 | 494 Hz |
  | F4 | 349 Hz | C5 | 523 Hz |
- Kode:
  ```cpp
  // Memainkan satu nada
  void playTone(int pin, int channel, int freq, int duration) {
    ledcSetup(channel, freq, 8);
    ledcAttachPin(pin, channel);
    ledcWrite(channel, 128);  // 50% duty
    delay(duration);
    ledcWrite(channel, 0);    // Diam
  }
  ```
- Contoh melodi: Array of {note, duration}

SLIDE 37: Contoh Melodi - Twinkle Twinkle
- Judul: "Contoh: Memainkan Melodi"
- Kode definisi melodi:
  ```cpp
  #define NOTE_C4  262
  #define NOTE_D4  294
  #define NOTE_E4  330
  #define NOTE_F4  349
  #define NOTE_G4  392
  #define NOTE_A4  440
  #define NOTE_REST 0
  
  int melody[] = {
    NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4,
    NOTE_A4, NOTE_A4, NOTE_G4, NOTE_REST,
    NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4,
    NOTE_D4, NOTE_D4, NOTE_C4, NOTE_REST
  };
  int durations[] = {
    400, 400, 400, 400,
    400, 400, 800, 200,
    400, 400, 400, 400,
    400, 400, 800, 200
  };
  ```
- Diagram notasi musik sederhana yang sesuai
- Tips: Tambahkan jeda pendek antar nada (50-100ms) agar terdengar jelas

SLIDE 38: Perbandingan DAC vs PWM
- Judul: "DAC vs PWM: Kapan Menggunakan Yang Mana?"
- Tabel perbandingan:
  | Aspek | DAC Murni | PWM + Filter |
  |-------|:---------:|:------------:|
  | Sinyal Output | Analog sejati | Quasi-analog (ada ripple) |
  | Resolusi | Tetap (8/12-bit) | Tergantung frekuensi & bit |
  | Bandwidth | Tinggi (settling time) | Terbatas oleh filter |
  | Ketersediaan | Hanya MCU tertentu | Semua MCU memiliki PWM |
  | Kompleksitas | Sederhana (1 pin) | Perlu RC filter eksternal |
  | Audio | Kualitas lebih baik | Cukup untuk tone/buzzer |
  | Kontrol Motor | Tidak langsung | Langsung (H-bridge) |
  | Kontrol LED | Tidak perlu PWM | Ideal untuk dimming |
- Kesimpulan: Gunakan DAC untuk audio/sinyal presisi, PWM untuk motor/LED/servo

SLIDE 39: Best Practices DAC & PWM
- Judul: "Best Practices Penggunaan DAC & PWM"
- Tips DAC:
  1. Gunakan kapasitor kopling (10µF) untuk output audio
  2. Tambahkan buffer op-amp jika perlu drive beban berat
  3. Gunakan DMA untuk waveform kontinu agar tidak membebani CPU
  4. Perhatikan settling time untuk sinyal frekuensi tinggi
- Tips PWM:
  1. Pilih frekuensi sesuai aplikasi (servo=50Hz, LED>1kHz, motor>20kHz)
  2. Gunakan resolusi setinggi mungkin sesuai frekuensi yang dipilih
  3. Jangan lupa pull-down resistor pada pin PWM saat boot
  4. Untuk output analog, pastikan f_cutoff << f_PWM dan f_cutoff > f_signal
  5. Hindari glitch saat mengubah frekuensi (disable → reconfig → enable)

SLIDE 40: Penutup & Tugas
- Judul: "Ringkasan & Tugas Project"
- Ringkasan keseluruhan modul:
  • DAC: Konversi digital ke analog, R-2R ladder, 8-bit ESP32, tidak ada di F411
  • PWM: Modulasi lebar pulsa, duty cycle mengatur daya rata-rata
  • LEDC ESP32: 16 channel, hardware fade, fleksibel
  • Timer STM32: PSC+ARR+CCR, multi-channel per timer
  • Servo: 50 Hz, 500-2500µs pulse width
  • RGB LED: 3 channel PWM, pencampuran warna
  • Buzzer: Frekuensi PWM = nada musik
- Tugas Project: Sistem Audio Player dan LED Controller
  • DAC wave generator + RGB LED + Servo + Melody Player
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
| Warna Primer | Hijau (#2e7d32) |
| Warna Sekunder | Abu-abu (#5f6368) |
| Warna Aksen | Oranye (#f9a825) untuk peringatan |
| Warna Kode | Background abu-abu gelap (#263238), teks terang |
| Background | Putih dengan subtle gradient hijau |

## 📐 Tips Pembuatan

1. **Kode harus terbaca** - Gunakan syntax highlighting dan font monospace
2. **Diagram perbandingan** - Gunakan layout 2 kolom untuk ESP32 vs STM32
3. **Diagram timing PWM** - Gambar sinyal PWM dengan label t_on, T, duty
4. **Animasi bertahap** - Untuk diagram proses dan kode step-by-step
5. **Konsistensi** - Pastikan desain sama dengan Bagian 1
