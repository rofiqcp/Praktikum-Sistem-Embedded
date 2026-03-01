# JOBSHEET BAB 02: Interrupt dan Timer

## Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | External Interrupt dan Hardware Timer |
| **Platform** | STM32F103C8T6 (Blue Pill), ESP32 DevKitC |
| **Framework** | STM32Cube HAL (STM32), ESP-IDF (ESP32) |
| **Jumlah Program** | 12 program per platform |
| **Durasi** | 3 × 50 menit |

---

## Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. Mengkonfigurasi External Interrupt (EXTI) pada STM32 dan ESP32 untuk mendeteksi edge pada GPIO
2. Mengimplementasikan teknik debounce berbasis timer pada interrupt
3. Mengkonfigurasi Hardware Timer dalam mode periodic (auto-reload) dan one-shot
4. Menghasilkan sinyal PWM dasar menggunakan timer hardware
5. Mengimplementasikan Watchdog Timer (IWDG/WWDG pada STM32, esp_task_wdt pada ESP32)
6. Menerapkan Timer Cascade (master-slave chaining) untuk extended timing
7. Menggunakan Output Compare untuk toggle GPIO pada frekuensi presisi
8. Mengukur lebar pulsa dan frekuensi sinyal dengan Input Capture
9. Membaca rotary encoder menggunakan Timer Encoder Interface
10. Menjalankan multiple timer secara bersamaan
11. Mengelola prioritas interrupt dan nested interrupt behavior

---

## Alat dan Bahan

### Hardware

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 Blue Pill | 1 | ARM Cortex-M3, 72 MHz |
| 2 | ESP32 DevKitC | 1 | Dual-core, 240 MHz |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | USB Cable Micro | 2 | Power & programming |
| 5 | Push Button | 4 | Tactile switch |
| 6 | LED 5mm | 4 | Merah, Kuning, Hijau, Biru |
| 7 | Resistor 330Ω | 4 | Current limiting LED |
| 8 | Resistor 10kΩ | 4 | Pull-up/pull-down |
| 9 | Breadboard | 1 | 830 tie-points |
| 10 | Kabel Jumper | 20 | Male-Male |
| 11 | Rotary Encoder 5-pin | 1 | Untuk percobaan Encoder Interface |

### Software

| No | Software | Keterangan |
|----|----------|------------|
| 1 | VS Code + PlatformIO | IDE utama |
| 2 | Serial Monitor | Debugging output |

---

## Konfigurasi Pin

### STM32F103C8T6

| Fungsi | Pin | Mode | Keterangan |
|--------|-----|------|------------|
| LED_BUILTIN | PC13 | OUTPUT | Active LOW |
| LED1 | PB3 | OUTPUT | Active HIGH |
| LED2 | PB4 | OUTPUT | Active HIGH |
| LED3 | PB5 | OUTPUT | Active HIGH |
| BTN1 | PA0 | INPUT (EXTI) | Pull-up, Falling edge |
| BTN2 | PA1 | INPUT (EXTI) | Pull-up, Falling edge |

### ESP32 DevKitC

| Fungsi | Pin | Mode | Keterangan |
|--------|-----|------|------------|
| LED_BUILTIN | GPIO2 | OUTPUT | Active HIGH |
| LED1 | GPIO4 | OUTPUT | Active HIGH |
| LED2 | GPIO5 | OUTPUT | Active HIGH |
| LED3 | GPIO18 | OUTPUT | Active HIGH |
| BTN1 | GPIO0 | INPUT | BOOT button |
| BTN2 | GPIO13 | INPUT | External |

---

## Persiapan Umum

1. Buka VS Code + PlatformIO
2. Sambungkan ST-Link V2 ke Blue Pill (SWCLK, SWDIO, GND, 3.3V)
3. Pasang LED ke pin output dengan resistor 330Ω seri
4. Pasang push button ke pin input dengan pull-up 10kΩ
5. Hubungkan USB untuk power dan serial monitor

---

## Langkah Kerja Percobaan

### Percobaan 1 — EXTI Interrupt

**Tujuan:** Memahami mekanisme External Interrupt dasar pada kedua platform.

**Langkah Kerja:**

1. Buka folder `STM32_01_EXTI_Interrupt/`, build dan upload
2. Buka Serial Monitor (115200 baud)
3. Tekan BTN1 (PA0) — amati LED toggle dan pesan di serial
4. Amati bahwa LED berubah state **hanya** saat tombol ditekan, bukan polling
5. Ulangi pada ESP32: buka `ESP32_01_EXTI_Interrupt/`, upload, tekan BTN1 (GPIO4)
6. Perhatikan penggunaan `IRAM_ATTR` pada ISR ESP32

**Hasil Pengamatan:**

| Platform | Pin BTN | Trigger Edge | Respons LED | Serial Output |
|----------|---------|-------------|-------------|---------------|
| STM32 | PA0 | Falling | Toggle | \_\_\_\_\_ |
| ESP32 | GPIO4 | Falling | Toggle | \_\_\_\_\_ |

**Analisa:**
1. Apa perbedaan konfigurasi EXTI antara `HAL_GPIO_EXTI_Callback()` (STM32) dan `gpio_isr_handler_add()` (ESP32)?
2. Mengapa ESP32 memerlukan `IRAM_ATTR` pada ISR sedangkan STM32 tidak?
3. Apa yang terjadi jika interrupt flag di STM32 tidak di-clear?

---

### Percobaan 2 — EXTI Debounce

**Tujuan:** Mengatasi masalah bouncing tombol pada interrupt menggunakan timer-based debounce.

**Langkah Kerja:**

1. Upload `STM32_02_EXTI_Debounce/` dan buka Serial Monitor
2. Tekan BTN1 cepat 10× — catat jumlah press count yang terdeteksi
3. Bandingkan dengan Percobaan 1 (tanpa debounce) — apakah count lebih akurat?
4. Ulangi pada ESP32: upload `ESP32_02_EXTI_Debounce/`
5. Perhatikan penggunaan `esp_timer_get_time()` untuk timestamp debounce (200ms threshold)

**Hasil Pengamatan:**

| Pengujian | Tanpa Debounce (P01) | Dengan Debounce (P02) |
|-----------|---------------------|----------------------|
| 10× tekan → count | \_\_\_ | \_\_\_ |
| False trigger | Ya / Tidak | Ya / Tidak |

**Analisa:**
1. Berapa waktu debounce yang digunakan (dalam ms)? Apakah cukup untuk tombol tactile?
2. Mengapa debounce berbasis timer lebih efektif dibanding blocking delay di dalam ISR?
3. Apa risiko jika threshold debounce terlalu besar (misalnya 500ms)?

---

### Percobaan 3 — Timer Periodic

**Tujuan:** Mengkonfigurasi hardware timer dalam mode periodic auto-reload.

**Langkah Kerja:**

1. Upload `STM32_03_Timer_Periodic/`
2. Amati LED berkedip dengan interval presisi 1 detik dari timer interrupt
3. Buka Serial Monitor — amati counter yang increment setiap 1 detik
4. Upload `ESP32_03_Timer_Periodic/` — amati GPTimer periodic alarm
5. Bandingkan akurasi timer STM32 vs ESP32 (hitung 60 kedipan, apakah tepat 60 detik?)

**Perhitungan Timer STM32:**
```
Clock = 72 MHz
PSC = 7199 → Timer clock = 72MHz / 7200 = 10 kHz
ARR = 9999 → Period = 10000 / 10kHz = 1.000 detik
```

**Hasil Pengamatan:**

| Platform | PSC | ARR | Target Period | Actual (ukur) | Error |
|----------|-----|-----|---------------|---------------|-------|
| STM32 | 7199 | 9999 | 1000 ms | \_\_\_ ms | \_\_\_ % |
| ESP32 | — | — | 1000 ms | \_\_\_ ms | \_\_\_ % |

**Analisa:**
1. Tuliskan rumus perhitungan timer untuk PSC=3599 dan ARR=19999. Berapa periodenya?
2. Mengapa ESP32 GPTimer menggunakan resolusi 1MHz (1µs) sebagai acuan?
3. Apa keuntungan auto-reload dibanding manual restart timer?

---

### Percobaan 4 — Timer One-Shot

**Tujuan:** Mengimplementasikan timer yang hanya berjalan sekali (one-shot mode).

**Langkah Kerja:**

1. Upload `STM32_04_Timer_One_Shot/`
2. Tekan BTN — LED menyala, setelah 3 detik LED mati otomatis (one-shot timer)
3. Tekan BTN lagi — siklus berulang
4. Upload `ESP32_04_Timer_One_Shot/` — amati behavior yang sama
5. Perhatikan bahwa timer tidak auto-reload, hanya berjalan sekali per trigger

**Hasil Pengamatan:**

| Aksi | LED State | Durasi ON | Auto-repeat? |
|------|-----------|-----------|-------------|
| Tekan BTN | ON → OFF | \_\_\_ detik | Ya / Tidak |
| Tekan BTN lagi | ON → OFF | \_\_\_ detik | Ya / Tidak |
| Tanpa tekan | OFF | — | — |

**Analisa:**
1. Apa perbedaan konfigurasi timer one-shot vs periodic di STM32 HAL?
2. Dalam skenario apa one-shot timer lebih tepat digunakan dibanding periodic?
3. Bagaimana cara mengubah durasi one-shot dari 3 detik menjadi 5 detik?

---

### Percobaan 5 — Timer PWM Basic

**Tujuan:** Menghasilkan sinyal PWM (Pulse Width Modulation) dasar menggunakan timer hardware.

**Langkah Kerja:**

1. Upload `STM32_05_Timer_PWM_Basic/`
2. Hubungkan oscilloscope atau LED ke pin PWM output
3. Amati sinyal PWM 50Hz dengan duty cycle 50%
4. Upload `ESP32_05_Timer_PWM_Basic/` — amati LEDC PWM pada GPIO2
5. Perhatikan penggunaan LEDC hardware timer ESP32 (13-bit resolution)

**Hasil Pengamatan:**

| Parameter | STM32 | ESP32 |
|-----------|-------|-------|
| Frekuensi PWM | \_\_\_ Hz | 50 Hz |
| Duty Cycle | \_\_\_ % | 50% |
| Resolusi | 16-bit | 13-bit |
| Pin Output | \_\_\_ | GPIO2 |

**Analisa:**
1. Bagaimana hubungan antara frekuensi PWM, prescaler, dan auto-reload register?
2. Jika resolusi 13-bit, berapa level duty cycle yang tersedia?
3. Untuk mengontrol servo motor (1-2ms pulse pada 50Hz), berapa nilai CCR yang tepat?

---

### Percobaan 6 — Watchdog Timer

**Tujuan:** Mengimplementasikan watchdog timer untuk deteksi system hang dan auto-recovery.

**Langkah Kerja:**

1. Upload `STM32_06_Watchdog_Timer/`
2. Amati LED berjalan normal — watchdog di-kick periodik
3. Tekan BTN untuk simulasi hang (loop tanpa kick) — tunggu ~1 detik
4. Amati sistem reset otomatis oleh IWDG
5. Upload `ESP32_06_Watchdog_Timer/`
6. Amati normal operation, lalu tekan BTN untuk starve WDT
7. Amati panic reset setelah 5 detik timeout

**Hasil Pengamatan:**

| Skenario | Platform | Timeout | Aksi Reset |
|----------|----------|---------|-----------|
| Normal (kick aktif) | STM32 | — | Tidak ada reset |
| Simul hang | STM32 | \_\_\_ detik | Auto reset |
| Normal (feed aktif) | ESP32 | — | Tidak ada reset |
| Simul hang (BTN) | ESP32 | \_\_\_ detik | Panic reset |

**Analisa:**
1. Apa perbedaan IWDG dan WWDG pada STM32? Kapan masing-masing digunakan?
2. Bagaimana cara mendeteksi apakah reset terakhir disebabkan oleh WDT?
3. Dalam aplikasi industri, mengapa watchdog timer sangat penting?

---

### Percobaan 7 — Timer Cascade

**Tujuan:** Menghubungkan dua timer (master-slave) untuk memperluas range timing.

**Langkah Kerja:**

1. Upload `STM32_07_Timer_Cascade/`
2. Amati LED1 berkedip cepat (fast timer / master) setiap 100ms
3. Amati LED2 berkedip lambat (cascade / slave) — hanya toggle setiap N tick dari master
4. Hitung rasio: berapa kali LED1 toggle sebelum LED2 toggle?
5. Upload `ESP32_07_Timer_Cascade/` — amati software timer chaining

**Hasil Pengamatan:**

| Timer | LED | Interval | Rasio |
|-------|-----|----------|-------|
| Fast (Master) | LED1 | \_\_\_ ms | 1× |
| Cascade (Slave) | LED2 | \_\_\_ ms | \_\_\_× |

**Analisa:**
1. STM32 mendukung hardware timer cascade (TRGO). Apa keuntungannya dibanding software chaining?
2. Berapa range timing maksimal yang bisa dicapai dengan cascade 2 timer 16-bit pada 72MHz?
3. Dalam aplikasi apa timer cascade diperlukan?

---

### Percobaan 8 — Output Compare Toggle

**Tujuan:** Menggunakan Output Compare (OC) timer untuk toggle GPIO pada frekuensi presisi.

**Langkah Kerja:**

1. Upload `STM32_08_Output_Compare_Toggle/`
2. Hubungkan LED atau oscilloscope ke pin OC output
3. Amati toggle pada frekuensi yang ditentukan (tanpa software intervention)
4. Upload `ESP32_08_Output_Compare_Toggle/` — amati GPTimer alarm toggle GPIO via ISR

**Hasil Pengamatan:**

| Platform | Pin OC | Frekuensi Toggle | Metode |
|----------|--------|-----------------|--------|
| STM32 | \_\_\_ | \_\_\_ Hz | Hardware OC |
| ESP32 | \_\_\_ | \_\_\_ Hz | GPTimer alarm + ISR |

**Analisa:**
1. Apa perbedaan antara Output Compare dan simple timer interrupt toggle?
2. Mengapa hardware OC lebih presisi daripada software toggle di ISR?
3. Sebutkan 2 aplikasi nyata yang memerlukan Output Compare!

---

### Percobaan 9 — Input Capture

**Tujuan:** Mengukur lebar pulsa dan frekuensi sinyal eksternal menggunakan Input Capture timer.

**Langkah Kerja:**

1. Upload `STM32_09_Input_Capture/`
2. Hubungkan sinyal dari timer PWM percobaan 5 ke pin input capture
3. Amati serial monitor — lebar pulsa dan frekuensi terukur
4. Upload `ESP32_09_Input_Capture/` — amati pengukuran menggunakan ANYEDGE interrupt + timestamp
5. Ubah frekuensi input — amati perubahan pengukuran

**Hasil Pengamatan:**

| Sinyal Input | Pulse Width Terukur | Frekuensi Terukur | Error |
|-------------|--------------------|--------------------|-------|
| PWM 50Hz 50% | \_\_\_ µs | \_\_\_ Hz | \_\_\_ % |
| PWM 100Hz 50% | \_\_\_ µs | \_\_\_ Hz | \_\_\_ % |

**Analisa:**
1. Jelaskan mekanisme Input Capture — bagaimana rising/falling edge dicapture oleh timer?
2. Apa batasan frekuensi minimum dan maksimum yang bisa diukur?
3. Bagaimana jika ada noise pada sinyal input? Apa solusinya?

---

### Percobaan 10 — Encoder Interface

**Tujuan:** Membaca rotary encoder menggunakan Timer Encoder Interface mode.

**Langkah Kerja:**

1. Sambungkan rotary encoder: CLK dan DT ke pin timer STM32
2. Upload `STM32_10_Encoder_Interface/`
3. Putar encoder CW 10× — amati counter naik di serial monitor
4. Putar encoder CCW 10× — amati counter turun
5. Upload `ESP32_10_Encoder_Interface/` — amati PCNT encoder mode

**Hasil Pengamatan:**

| Aksi | Count STM32 | Count ESP32 | Sesuai? |
|------|------------|------------|---------|
| CW 10 step | \_\_\_ | \_\_\_ | Ya / Tidak |
| CCW 10 step | \_\_\_ | \_\_\_ | Ya / Tidak |
| CW cepat 20 step | \_\_\_ | \_\_\_ | Ya / Tidak |

**Analisa:**
1. Apa keuntungan Timer Encoder Mode dibanding polling manual (seperti di Modul 01 P09)?
2. Jelaskan mode X1, X2, X4 pada encoder interface!
3. Mengapa encoder mode tidak kehilangan step meski diputar cepat?

---

### Percobaan 11 — Multiple Timers

**Tujuan:** Menjalankan beberapa timer hardware secara bersamaan dengan interval berbeda.

**Langkah Kerja:**

1. Upload `STM32_11_Multiple_Timers/`
2. Amati 3 LED berkedip dengan interval berbeda secara independen
3. Verifikasi setiap LED: LED1 (200ms), LED2 (500ms), LED3 (1000ms)
4. Upload `ESP32_11_Multiple_Timers/` — amati 3 GPTimer independen
5. Hitung: dalam 10 detik, berapa kali masing-masing LED toggle?

**Hasil Pengamatan:**

| Timer | LED | Interval | Toggle per 10 detik |
|-------|-----|----------|---------------------|
| Timer A | LED1 | 200 ms | \_\_\_ kali |
| Timer B | LED2 | 500 ms | \_\_\_ kali |
| Timer C | LED3 | 1000 ms | \_\_\_ kali |

**Analisa:**
1. Berapa jumlah hardware timer yang tersedia pada STM32F103 dan ESP32?
2. Apa yang terjadi jika semua timer sudah terpakai dan kita butuh timer tambahan?
3. Kapan sebaiknya menggunakan software timer (FreeRTOS) vs hardware timer?

---

### Percobaan 12 — NVIC Priority (Prioritas Interrupt)

**Tujuan:** Memahami sistem prioritas interrupt dan nested interrupt behavior.

**Langkah Kerja:**

1. Upload `STM32_12_NVIC_Priority/`
2. BTN1 memiliki prioritas tinggi, BTN2 prioritas rendah — masing-masing toggle LED berbeda
3. Tekan BTN2 (tahan) lalu tekan BTN1 — amati apakah BTN1 meng-interrupt handler BTN2
4. Upload `ESP32_12_NVIC_Priority/` — amati priority behavior
5. Catat behavior nested interrupt

**Hasil Pengamatan:**

| Skenario | LED1 (High Prio) | LED2 (Low Prio) | Nested? |
|----------|------------------|-----------------|---------|
| BTN1 saja | Toggle | — | — |
| BTN2 saja | — | Toggle | — |
| BTN2 lalu BTN1 | \_\_\_ | \_\_\_ | Ya / Tidak |

**Analisa:**
1. Jelaskan konsep preemption priority dan sub-priority pada NVIC STM32!
2. Apa yang dimaksud priority inversion dalam konteks interrupt?
3. Bagaimana cara menentukan prioritas interrupt yang tepat dalam sistem multi-sensor?

---

## Tabel Komparatif STM32 vs ESP32

| Aspek | STM32 (HAL) | ESP32 (ESP-IDF) |
|-------|-------------|-----------------|
| Interrupt controller | NVIC | Interrupt Matrix |
| EXTI config | `HAL_GPIO_EXTI_Callback()` | `gpio_isr_handler_add()` |
| ISR attribute | Tidak perlu | `IRAM_ATTR` wajib |
| Timer config | TIM2–TIM4 (16-bit) | GPTimer (64-bit) |
| Timer start | `HAL_TIM_Base_Start_IT()` | `gptimer_start()` |
| Watchdog | IWDG + WWDG | `esp_task_wdt` |
| Timer cascade | Hardware master-slave | Software chaining |
| Priority levels | 16 levels (4-bit) | 7 levels |
| Encoder mode | Timer Encoder Interface | PCNT |
| PWM | Timer OC | LEDC |

---

## Kesimpulan

Tuliskan kesimpulan mencakup:
1. Perbedaan mekanisme polling vs interrupt dan kapan masing-masing tepat digunakan
2. Konfigurasi timer periodic vs one-shot, termasuk rumus perhitungan PSC dan ARR
3. Pentingnya watchdog timer dalam sistem embedded real-time
4. Keuntungan timer cascade untuk extended timing
5. Perbedaan implementasi interrupt dan timer antara STM32 HAL dan ESP-IDF

---

## Referensi

1. Noviello, C. (2020). *Mastering STM32, 2nd Edition*, Ch7: Interrupts, Ch11: Timers. Leanpub.
2. Kolban, N. (2018). *Kolban's Book on ESP32*, p267–302: ISR & Timers.
3. Yiu, J. (2010). *The Definitive Guide to ARM Cortex-M3 and Cortex-M4 Processors*. Elsevier.
4. STMicroelectronics. (2021). *RM0008 Reference Manual*, Ch10: EXTI, Ch13–15: Timers.
5. STMicroelectronics. (2017). *AN4776 Timer Cookbook for STM32*.
6. Espressif Systems. (2024). *ESP-IDF Programming Guide*: GPIO, GPTimer, Task WDT APIs.

