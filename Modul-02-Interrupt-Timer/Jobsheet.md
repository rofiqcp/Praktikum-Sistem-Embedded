# JOBSHEET BAB 02: Interrupt dan Timer

## 📋 Informasi Praktikum

| Item | Keterangan |
|------|------------|
| **Topik** | External Interrupt dan Hardware Timer |
| **Platform** | STM32F103C8T6 (Blue Pill), ESP32 DevKitC |
| **Framework** | STM32Cube HAL (STM32), ESP-IDF (ESP32) |
| **Jumlah Program STM32** | 12 program |
| **Jumlah Program ESP32** | 12 program |
| **Durasi** | 3 x 50 menit |
| **Tools** | PlatformIO, VS Code, Serial Monitor |

---

## 🎯 Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa mampu:

1. Memahami perbedaan mekanisme polling dan interrupt
2. Mengkonfigurasi External Interrupt pada STM32 (HAL) dan ESP32 (ESP-IDF)
3. Mengimplementasikan Hardware Timer dengan interrupt
4. Mengkonfigurasi Watchdog Timer (IWDG/WWDG dan esp_task_wdt)
5. Menerapkan teknik Timer Cascade (master-slave chaining)
6. Menerapkan teknik debouncing berbasis interrupt
7. Mengembangkan aplikasi real-time dengan timer dan interrupt

---

## 🔧 Alat dan Bahan

### Hardware

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | STM32F103C8T6 (Blue Pill) | 1 | ARM Cortex-M3, 72MHz |
| 2 | ESP32 DevKitC | 1 | Dual-core, 240MHz |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | USB Cable Micro | 2 | Power & programming |
| 5 | Push Button | 4 | Tactile switch |
| 6 | LED 5mm | 4 | Merah, Kuning, Hijau, Biru |
| 7 | Resistor 330Ω | 4 | Current limiting LED |
| 8 | Resistor 10kΩ | 4 | Pull-up/pull-down |
| 9 | Breadboard | 1 | 830 tie-points |
| 10 | Kabel Jumper | 20 | Male-Male |
| 11 | Rotary Encoder | 1 | Untuk program Encoder_Interface |

### Software

| No | Software | Versi | Keterangan |
|----|----------|-------|------------|
| 1 | VS Code | Latest | IDE utama |
| 2 | PlatformIO | Latest | Build system |
| 3 | STM32 Platform | ststm32 | Framework: stm32cube |
| 4 | ESP32 Platform | espressif32 | Framework: espidf |
| 5 | Serial Monitor | Built-in | Debugging output |

---

## 📐 Konfigurasi Pin

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

## 📝 Daftar Program Praktikum

### Program STM32 (STM32Cube HAL)

| No | Direktori | Topik | Tingkat |
|----|-----------|-------|---------|
| 1 | STM32_01_EXTI_Interrupt | External Interrupt dasar dengan HAL | Dasar |
| 2 | STM32_02_EXTI_Debounce | EXTI dengan timer-based debounce | Dasar |
| 3 | STM32_03_Timer_Periodic | Timer periodik (auto-reload) | Dasar |
| 4 | STM32_04_Timer_One_Shot | Timer satu kali (one-shot mode) | Menengah |
| 5 | STM32_05_Timer_PWM_Basic | PWM output via timer OC | Menengah |
| 6 | STM32_06_Watchdog_Timer | IWDG dan WWDG watchdog | Menengah |
| 7 | STM32_07_Timer_Cascade | Master-slave timer chaining | Lanjut |
| 8 | STM32_08_Output_Compare_Toggle | Output Compare toggle GPIO | Menengah |
| 9 | STM32_09_Input_Capture | Mengukur lebar pulsa / frekuensi | Lanjut |
| 10 | STM32_10_Encoder_Interface | Timer encoder mode | Lanjut |
| 11 | STM32_11_Multiple_Timers | Beberapa timer bersamaan | Lanjut |
| 12 | STM32_12_NVIC_Priority | Prioritas interrupt dan nesting | Lanjut |

### Program ESP32 (ESP-IDF)

| No | Direktori | Topik | Tingkat |
|----|-----------|-------|---------|
| 1 | ESP32_01_EXTI_Interrupt | GPIO interrupt dengan ISR service | Dasar |
| 2 | ESP32_02_EXTI_Debounce | GPIO interrupt + esp_timer debounce | Dasar |
| 3 | ESP32_03_Timer_Periodic | GPTimer periodic alarm | Dasar |
| 4 | ESP32_04_Timer_One_Shot | GPTimer one-shot alarm | Menengah |
| 5 | ESP32_05_Timer_PWM_Basic | LEDC PWM via timer | Menengah |
| 6 | ESP32_06_Watchdog_Timer | esp_task_wdt watchdog | Menengah |
| 7 | ESP32_07_Timer_Cascade | Software timer chaining | Lanjut |
| 8 | ESP32_08_Output_Compare_Toggle | GPTimer alarm toggle GPIO | Menengah |
| 9 | ESP32_09_Input_Capture | Pulse counting / PCNT | Lanjut |
| 10 | ESP32_10_Encoder_Interface | PCNT encoder mode | Lanjut |
| 11 | ESP32_11_Multiple_Timers | Multiple GPTimers | Lanjut |
| 12 | ESP32_12_NVIC_Priority | Interrupt priority levels | Lanjut |

---

## 📚 Tugas Praktikum

### Tugas 1: External Interrupt (30 menit)

**Tujuan:** Memahami dan mengimplementasikan external interrupt

**Langkah Kerja:**

1. **Persiapan Hardware (10 menit)**
   - Hubungkan button ke PA0 (STM32) atau GPIO0 (ESP32)
   - Hubungkan LED ke PB3 (STM32) atau GPIO4 (ESP32)
   - Verifikasi koneksi

2. **STM32 — HAL EXTI (10 menit)**
   - Buka program `STM32_01_EXTI_Interrupt`
   - Compile dan upload
   - Tekan button, amati LED toggle
   - Perhatikan penggunaan `HAL_GPIO_EXTI_Callback()`

3. **ESP32 — ESP-IDF GPIO ISR (10 menit)**
   - Buka program `ESP32_01_EXTI_Interrupt`
   - Compile dan upload
   - Tekan button, amati LED toggle
   - Perhatikan penggunaan `gpio_isr_handler_add()` dan `IRAM_ATTR`

**Pertanyaan Analisis:**
1. Apa perbedaan konfigurasi EXTI antara HAL dan ESP-IDF?
2. Mengapa ESP32 memerlukan `IRAM_ATTR` pada ISR?
3. Apa yang terjadi jika tidak clear interrupt flag di STM32?

---

### Tugas 2: Hardware Timer (30 menit)

**Tujuan:** Mengkonfigurasi dan menggunakan hardware timer

**Langkah Kerja:**

1. **Timer Periodic (15 menit)**
   - **STM32:** Buka `STM32_03_Timer_Periodic` — gunakan `HAL_TIM_Base_Start_IT()`
   - **ESP32:** Buka `ESP32_03_Timer_Periodic` — gunakan `gptimer` API

2. **Timer One-Shot (15 menit)**
   - **STM32:** Buka `STM32_04_Timer_One_Shot` — disable auto-reload
   - **ESP32:** Buka `ESP32_04_Timer_One_Shot` — one-shot alarm config

**Perhitungan Timer STM32:**
```
Clock = 72 MHz, Target = 1 second
PSC = 7199, ARR = 9999
Timer_Freq = 72MHz / 7200 = 10kHz
Period = 10000 / 10kHz = 1 second ✓
```

---

### Tugas 3: Watchdog Timer (20 menit)

**Tujuan:** Mengimplementasikan watchdog untuk deteksi system hang

**Langkah Kerja:**

1. **STM32 IWDG (10 menit)**
   - Buka `STM32_06_Watchdog_Timer`
   - Amati system reset saat WDT timeout
   - Verifikasi `HAL_IWDG_Refresh()` mencegah reset

2. **ESP32 Task WDT (10 menit)**
   - Buka `ESP32_06_Watchdog_Timer`
   - Amati panic saat `esp_task_wdt_reset()` tidak dipanggil
   - Coba simulasi hang (infinite loop tanpa kick)

**Pertanyaan Analisis:**
1. Apa perbedaan IWDG dan WWDG pada STM32?
2. Kapan menggunakan watchdog timer dalam aplikasi real?
3. Bagaimana cara mendeteksi apakah reset disebabkan oleh WDT?

---

### Tugas 4: Timer Cascade (20 menit)

**Tujuan:** Menghubungkan beberapa timer untuk extended timing

**Langkah Kerja:**

1. **STM32 Master-Slave (10 menit)**
   - Buka `STM32_07_Timer_Cascade`
   - Amati TIM2 (master) memicu TIM3 (slave)
   - Verifikasi periode cascade = master × slave

2. **ESP32 Software Chain (10 menit)**
   - Buka `ESP32_07_Timer_Cascade`
   - Amati timer chaining via software counter
   - Bandingkan dengan hardware cascade STM32

---

### Tugas 5: Integrasi (30 menit)

**Tujuan:** Menggabungkan interrupt, timer, dan watchdog

**Langkah Kerja:**

1. **Multiple Timers**
   - Buka `STM32_11_Multiple_Timers` atau `ESP32_11_Multiple_Timers`
   - Amati beberapa timer berjalan bersamaan

2. **NVIC Priority**
   - Buka `STM32_12_NVIC_Priority` atau `ESP32_12_NVIC_Priority`
   - Amati nested interrupt behavior

**Tugas Pengembangan:**
Modifikasi program untuk membuat Reaction Time Tester:
- LED menyala random setelah 1-5 detik (timer)
- Ukur waktu user menekan button (interrupt + timestamp)
- Tampilkan hasil via ESP_LOGI / HAL UART

---

## 📊 Rubrik Penilaian Praktikum

| Komponen | Bobot | Kriteria |
|----------|-------|----------|
| **Implementasi** | 40% | Semua program berjalan dengan benar |
| **Laporan** | 30% | Dokumentasi lengkap dan analisis mendalam |
| **Pemahaman** | 20% | Menjawab pertanyaan dengan benar |
| **Keaktifan** | 10% | Partisipasi dan inisiatif |

---

## ⚠️ Troubleshooting

### Masalah Umum Interrupt

| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| ISR tidak terpanggil | NVIC tidak di-enable | `HAL_NVIC_EnableIRQ()` / `gpio_install_isr_service()` |
| ISR terpanggil terus | Flag tidak di-clear | Gunakan `HAL_GPIO_EXTI_IRQHandler()` |
| ESP32 crash | IRAM_ATTR hilang | Tambahkan `IRAM_ATTR` pada ISR |
| Double trigger | Bouncing | Implementasi debounce (program 02) |

### Masalah Umum Timer

| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| Timer tidak jalan | Clock tidak enabled | `__HAL_RCC_TIMx_CLK_ENABLE()` |
| Periode tidak akurat | PSC/ARR salah | Hitung ulang dengan rumus |
| WDT reset terus | Lupa kick | Panggil refresh/reset periodik |

---

## 📚 Referensi

1. *Mastering STM32* — Ch7 (Interrupts), Ch11 (Timers)
2. *Kolban's Book on ESP32* — p267-268 (ISR), p300-302 (Timers)
3. STM32F103 Reference Manual (RM0008)
4. [ESP-IDF GPIO API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
5. [ESP-IDF GPTimer API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gptimer.html)
6. [ESP-IDF Task WDT](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html)
---

## 📐 Skema Rangkaian

### Rangkaian Button dengan Pull-up Internal

```
STM32/ESP32:
                    ┌─────────────────────┐
                    │     MCU             │
    ┌───────────────┤ GPIO (INPUT_PULLUP) │
    │               └─────────────────────┘
    │                        │
    │                       ─┴─ Internal Pull-up
    │                        │
   ─┴─
   │ │ Push Button
   ─┬─
    │
   ─┴─
   GND

Saat tidak ditekan: GPIO = HIGH (pulled up)
Saat ditekan: GPIO = LOW (connected to GND)
Trigger: FALLING edge
```

### Rangkaian LED dengan Current Limiting Resistor

```
                    ┌─────────────────┐
                    │     MCU         │
                    │ GPIO (OUTPUT)   ├────[R 330Ω]──▶│LED├── GND
                    └─────────────────┘
```

---

## 📝 Format Laporan Praktikum

### Struktur Laporan

1. **Cover** (1 halaman) — Judul, Nama, NIM, Tanggal
2. **Tujuan** (0.5 halaman) — List tujuan dari jobsheet
3. **Dasar Teori** (1-2 halaman) — Ringkasan interrupt, timer, watchdog, cascade
4. **Metodologi** (1 halaman) — Alat, diagram, prosedur
5. **Hasil dan Analisis** (3-4 halaman)
   - Screenshot hasil setiap tugas
   - Tabel pengukuran timing
   - Analisis perbandingan STM32 vs ESP32
   - Jawaban pertanyaan analisis
6. **Kesimpulan** (0.5 halaman)
7. **Lampiran** — Source code modifikasi

---

## 📊 Tabel Dokumentasi Hasil

### Tabel 1: External Interrupt Response

| Percobaan | Platform | Response Time | False Trigger |
|-----------|----------|---------------|---------------|
| EXTI tanpa debounce | STM32 | | |
| EXTI tanpa debounce | ESP32 | | |
| EXTI dengan debounce | STM32 | | |
| EXTI dengan debounce | ESP32 | | |

### Tabel 2: Timer Accuracy

| Timer Config | Target Period | Measured Period | Error (%) |
|-------------|---------------|-----------------|-----------|
| STM32 1Hz | 1000 ms | | |
| ESP32 1Hz | 1000 ms | | |
| STM32 10Hz | 100 ms | | |
| ESP32 10Hz | 100 ms | | |

### Tabel 3: Watchdog Timer

| Platform | WDT Type | Timeout Setting | Actual Reset Time |
|----------|----------|-----------------|-------------------|
| STM32 | IWDG | 1s | |
| STM32 | WWDG | - | |
| ESP32 | TWDT | 5s | |
