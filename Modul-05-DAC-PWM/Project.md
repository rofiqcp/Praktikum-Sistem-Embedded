# Project Modul 05: Audio & Light Controller — DAC & PWM

## Informasi Project

| Item | Keterangan |
|------|------------|
| Modul | 05 — DAC dan PWM Output |
| Platform | STM32F103C8T6 + ESP32 DevKit V1 |
| Durasi | 2 minggu |
| Tipe | Dual-MCU audio & aktuator kontrol |

---

## Deskripsi Umum

Project ini mengintegrasikan seluruh 12 percobaan praktikum DAC dan PWM — meliputi output tegangan DAC, waveform generation (sine, triangle), audio tone, PWM LED breathing, brightness control, servo motor, frequency sweep, motor speed, RGB LED, buzzer melody, dan perbandingan DAC vs PWM — ke dalam satu sistem terintegrasi.

---

## Soal Cerita

### Skenario: Sistem Presentasi Interaktif Museum Sains

Museum Sains Nusantara sedang merenovasi ruang pameran "Gelombang dan Cahaya". Mereka membutuhkan instalasi interaktif di mana pengunjung dapat memanipulasi gelombang suara dan cahaya secara langsung melalui panel kontrol.

**ESP32 DevKit V1 (Audio & Wave Controller)** mengendalikan bagian audio dan generasi gelombang. ESP32 menggunakan **DAC internal** (GPIO25/GPIO26) untuk menghasilkan **gelombang sinus** yang terdengar sebagai nada murni melalui speaker kecil, dan **gelombang segitiga** yang divisualisasikan di osiloskop display. Pengunjung memutar potensiometer untuk mengubah frekuensi nada (**audio tone**) dari 200Hz hingga 2000Hz. ESP32 juga memainkan **melodi** sederhana (do-re-mi-fa-sol-la-si-do) melalui **buzzer PWM** saat tombol "Play" ditekan. Untuk efek cahaya, ESP32 mengontrol **LED RGB** yang berubah warna secara smooth menggunakan 3 channel PWM, dan **LED breathing** yang bernapas perlahan sebagai ambient lighting.

**STM32F103C8T6 (Motion & Light Controller)** mengendalikan bagian gerakan dan pencahayaan. Karena STM32F103 **tidak memiliki DAC hardware**, STM32 menggunakan **PWM murni** untuk semua output analog. STM32 mengontrol **servo motor** yang menggerakkan pointer pada display skala besar (0°–180°) sesuai input potensiometer pengunjung. STM32 juga mengatur **kecepatan motor DC** kecil yang memutar piringan reflektor cahaya menggunakan PWM + driver L298N. **Brightness LED panel** diatur secara otomatis berdasarkan ambient light — semakin gelap ruangan, semakin terang LED.

Kedua mikrokontroler terhubung via UART untuk sinkronisasi. Saat pengunjung menekan tombol **"Frequency Sweep"**, ESP32 melakukan sweep frekuensi dari 100Hz ke 5000Hz melalui DAC, sementara STM32 secara sinkron menggerakkan servo dari 0° ke 180° mengikuti frekuensi tersebut. Sistem juga memiliki mode **"DAC vs PWM Compare"** yang menampilkan perbandingan output DAC murni vs PWM filtered pada osiloskop.

---

## Spesifikasi Teknis

### Mapping Percobaan ke Fitur Sistem

| No | Percobaan Praktikum | Fitur dalam Project |
|----|---------------------|---------------------|
| P01 | DAC Voltage Output | ESP32 menghasilkan tegangan referensi untuk kalibrasi |
| P02 | Sine Wave (DAC) | ESP32 generate gelombang sinus untuk speaker |
| P03 | Triangle Wave (DAC) | ESP32 generate gelombang segitiga untuk display osiloskop |
| P04 | Audio Tone (DAC) | ESP32 generate nada frekuensi variabel via potensiometer |
| P05 | PWM LED Breathing | Ambient LED breathing effect pada kedua platform |
| P06 | LED Brightness Control | STM32 brightness otomatis berdasarkan ambient light |
| P07 | Servo Motor Control | STM32 menggerakkan servo pointer display 0°–180° |
| P08 | Frequency Sweep | ESP32 sweep DAC frekuensi, STM32 sinkron gerakkan servo |
| P09 | Motor Speed Control | STM32 mengontrol kecepatan motor DC piringan reflektor |
| P10 | RGB LED Color Mixing | ESP32 kontrol warna LED RGB smooth transition |
| P11 | Buzzer Melody | ESP32 memainkan melodi do-re-mi via buzzer PWM |
| P12 | DAC vs PWM Compare | Mode perbandingan output DAC vs PWM filtered |

### Mode Operasi

```
    ┌──────────────────┐
    │   MODE AMBIENT   │ ◄── LED breathing + RGB color cycle
    │   (Default)      │     Motor idle, servo 90°
    └────────┬─────────┘
             │ Pengunjung putar potensio
             ▼
    ┌──────────────────┐
    │  MODE INTERAKTIF │ ◄── Tone frekuensi variabel
    │  (Manual)        │     Servo mengikuti input
    └────────┬─────────┘     Motor speed variabel
             │ Tekan tombol "Sweep"
             ▼
    ┌──────────────────┐
    │   MODE SWEEP     │ ◄── Frequency sweep 100–5000 Hz
    │   (Auto)         │     Servo sinkron 0°–180°
    └────────┬─────────┘     Durasi 10 detik
             │ Tekan tombol "Melody"
             ▼
    ┌──────────────────┐
    │   MODE MELODY    │ ◄── Mainkan melodi via buzzer
    │   (Playback)     │     RGB LED berubah per nada
    └──────────────────┘
```

---

## Ketentuan Pengerjaan

1. ESP32 harus menggunakan DAC hardware untuk waveform generation (GPIO25/26).
2. STM32 menggunakan PWM karena tidak ada DAC — jelaskan perbedaannya.
3. Servo harus bergerak smooth (bukan langsung lompat).
4. Audio tone harus terdengar jelas melalui speaker/buzzer.
5. RGB LED harus bisa smooth color transition.
6. Kedua MCU terhubung via UART untuk sinkronisasi mode.

---

## Rubrik Penilaian

| No | Komponen | Bobot |
|----|----------|-------|
| 1 | DAC waveform generation (sine, triangle, tone) | 20% |
| 2 | PWM LED control (breathing, brightness, RGB) | 20% |
| 3 | Servo + motor control | 15% |
| 4 | Buzzer melody + frequency sweep | 15% |
| 5 | Sinkronisasi dual-MCU via UART | 10% |
| 6 | Mode switching dan user interface | 10% |
| 7 | Kode modular dan dokumentasi | 10% |

---

## Referensi

1. STM32F103xx Reference Manual (RM0008) — Timer/PWM chapters
2. ESP-IDF Programming Guide — DAC, LEDC (PWM) API
3. AN3126 — Audio/waveform generation using DAC in STM32
4. Mastering STM32, Carmine Noviello — Chapter 11: Timers & PWM
