# Project Modul 02: Interrupt dan Timer

## Informasi Project

| Item | Keterangan |
|------|------------|
| **Modul** | 02 — Interrupt dan Timer |
| **Platform** | STM32F103C8T6 & ESP32 DevKitC |
| **Durasi Pengerjaan** | 2 minggu setelah praktikum |

---

## Deskripsi Umum

Project ini merupakan pengembangan dari seluruh 12 percobaan praktikum Interrupt dan Timer. Mahasiswa diminta menyelesaikan **soal cerita** berikut yang mengintegrasikan EXTI interrupt, debounce, timer periodic, one-shot, PWM, watchdog, timer cascade, output compare, input capture, encoder interface, multiple timers, dan NVIC priority ke dalam satu sistem terpadu.

---

## Soal Cerita Project

### Skenario: Sistem Monitoring Keamanan Gudang Otomatis

Anda bekerja sebagai engineer di perusahaan logistik yang memiliki gudang besar. Manajemen meminta Anda merancang **Sistem Monitoring Keamanan Gudang** yang mampu merespons berbagai kejadian secara real-time tanpa ada delay yang mengganggu.

Gudang memiliki 4 zona keamanan, masing-masing dengan sensor berbeda:

1. **Zona Pintu Utama** — Sensor magnetic door switch terhubung ke **EXTI interrupt prioritas tertinggi**. Setiap kali pintu dibuka, sistem mencatat timestamp presisi (menggunakan **timer free-running** sebagai microsecond counter). Jika pintu dibuka di luar jam operasional (18:00–06:00), **alarm buzzer** berbunyi menggunakan sinyal **PWM 2kHz** dari timer hardware.

2. **Zona Jendela** — 2 sensor jendela terhubung ke interrupt dengan **prioritas lebih rendah**. Sistem harus mendemonstrasikan **nested interrupt**: saat handler jendela sedang berjalan dan pintu utama terdeteksi terbuka, handler pintu harus bisa meng-interrupt handler jendela (karena prioritasnya lebih tinggi).

3. **Zona Motion Detector** — Sensor PIR terhubung ke interrupt. Karena PIR sering memberikan false trigger, sistem menggunakan **debounce berbasis one-shot timer** — setelah trigger pertama, sistem menunggu 3 detik sebelum menerima trigger berikutnya. Jika terdeteksi gerakan valid, **LED peringatan** berkedip dengan frekuensi presisi menggunakan **output compare toggle**.

4. **Zona Panic Button** — Tombol darurat dengan interrupt prioritas tertinggi. Saat ditekan, semua LED menyala, buzzer berbunyi, dan sistem menghitung **reaction time** (waktu dari alarm hingga security menekan tombol konfirmasi) menggunakan **input capture timer**.

**Fitur Tambahan:**

5. **Heartbeat Monitor** — **Timer periodic (1 detik)** mengkedipkan LED hijau sebagai indikator sistem aktif. Menggunakan **timer cascade**: fast timer (100ms) sebagai master, cascade counter menghitung hingga 10 sebelum toggle LED — total 1 detik.

6. **Watchdog Safety** — Sistem dilengkapi **watchdog timer** dengan timeout 5 detik. Jika main loop hang (misalnya karena bug), watchdog akan me-reset sistem. Log terakhir sebelum reset harus terdeteksi saat boot.

7. **Rotary Encoder Interface** — Security guard menggunakan **rotary encoder** untuk mengatur sensitivity level sensor (1–10). Encoder dibaca menggunakan **timer encoder mode** agar tidak kehilangan step meski diputar cepat.

8. **Multi-Timer Dashboard** — **3 timer independen** berjalan bersamaan:
   - Timer A (200ms): Update status sensor di serial monitor
   - Timer B (500ms): Cek status baterai (simulasi)
   - Timer C (1000ms): Log ringkasan ke serial

---

## Spesifikasi Teknis

### Fitur dan Konsep dari Percobaan

| No | Fitur | Konsep dari Percobaan |
|----|-------|-----------------------|
| 1 | Sensor pintu → EXTI interrupt | P01 — EXTI Interrupt |
| 2 | Debounce sensor PIR | P02 — EXTI Debounce |
| 3 | Heartbeat LED periodic | P03 — Timer Periodic |
| 4 | Delay alarm one-shot | P04 — Timer One-Shot |
| 5 | Buzzer alarm PWM 2kHz | P05 — Timer PWM Basic |
| 6 | Auto-reset saat hang | P06 — Watchdog Timer |
| 7 | Extended timing heartbeat | P07 — Timer Cascade |
| 8 | LED peringatan presisi | P08 — Output Compare Toggle |
| 9 | Pengukuran reaction time | P09 — Input Capture |
| 10 | Pengatur sensitivity | P10 — Encoder Interface |
| 11 | Dashboard 3 timer | P11 — Multiple Timers |
| 12 | Priority handling 4 zona | P12 — NVIC Priority |

### State Machine Sistem

```
                    ┌────────────┐
     Power ON ─────►│   IDLE     │ (Hijau berkedip, semua sensor aktif)
                    └─────┬──────┘
                          │ Sensor trigger
                    ┌─────▼──────┐
              ┌────►│  ALERT     │ (LED kuning, log event)
              │     └─────┬──────┘
              │           │ Threshold exceeded / Panic
              │     ┌─────▼──────┐
              │     │  ALARM     │ (Buzzer PWM, LED merah, semua LED ON)
              │     └─────┬──────┘
              │           │ Guard confirm (BTN)
              │     ┌─────▼──────┐
              └─────│  REVIEW    │ (Tampilkan reaction time, log)
                    └────────────┘
```

---

## Ketentuan Pengerjaan

1. Program harus berjalan pada **kedua platform** (STM32 dan ESP32)
2. Minimal 4 interrupt source dengan **prioritas berbeda**
3. Wajib ada **nested interrupt** yang dapat didemonstrasikan
4. Watchdog harus aktif dan dapat dibuktikan (simulasi hang → auto reset)
5. Semua timer menggunakan **hardware timer** (bukan software delay)
6. Serial monitor menampilkan log event dengan timestamp (microsecond precision)

---

## Rubrik Penilaian

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| Fungsionalitas | 40% | Semua fitur bekerja sesuai spesifikasi |
| Integrasi 12 percobaan | 20% | Seluruh konsep terintegrasi dalam satu sistem |
| Kode & arsitektur | 20% | Non-blocking, terstruktur, ISR singkat, komentar jelas |
| Dokumentasi | 10% | Diagram, penjelasan design decision |
| Dual platform | 10% | Berjalan di STM32 dan ESP32 |

---

## Referensi

1. Noviello, C. (2020). *Mastering STM32*, Ch7: Interrupts, Ch11: Timers.
2. Kolban, N. (2018). *Kolban's Book on ESP32*, ISR & Timers.
3. STMicroelectronics. (2017). *AN4776 Timer Cookbook for STM32*.
4. Espressif Systems. (2024). *ESP-IDF Programming Guide*: GPIO, GPTimer, Task WDT.
