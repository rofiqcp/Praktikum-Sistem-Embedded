# Project Modul 14: Power Management

## Daftar Isi
- [Project 1: Smart Weather Station Battery-Powered](#project-1-smart-weather-station-battery-powered)
- [Project 2: Industrial Power Monitor System](#project-2-industrial-power-monitor-system)
- [Project 3: Ultra-Low-Power Sensor Network Node](#project-3-ultra-low-power-sensor-network-node)

---

## Project 1: Smart Weather Station Battery-Powered

### 📋 Deskripsi

Pada project ini, mahasiswa akan merancang dan mengimplementasikan **stasiun cuaca portabel bertenaga baterai** yang mampu beroperasi selama **minimal 6 bulan** dengan baterai Li-Ion 3000mAh. Sistem ini memanfaatkan mode deep sleep pada ESP32 dan standby mode pada STM32 untuk meminimalkan konsumsi daya. Setiap 5 menit, mikrokontroler bangun dari mode tidur, membaca data sensor (disimulasikan melalui ADC), menyimpan data ke memori non-volatile (RTC memory pada ESP32, backup register pada STM32), kemudian kembali ke mode tidur.

Selain itu, sistem dilengkapi dengan **simulasi solar charging** yang memonitor level baterai dan mengatur duty cycle pengukuran secara adaptif — saat baterai rendah, interval pengukuran diperpanjang untuk menghemat energi.

### 🎯 Tujuan

1. Memahami dan mengimplementasikan mode deep sleep pada ESP32 serta standby mode pada STM32
2. Menerapkan mekanisme wakeup berbasis timer (ESP32) dan RTC alarm (STM32)
3. Menggunakan RTC memory (ESP32) dan backup register (STM32) untuk penyimpanan data persisten
4. Merancang sistem adaptive duty cycle berdasarkan level baterai
5. Menghitung dan memvalidasi power budget untuk target masa pakai baterai 6+ bulan
6. Membandingkan efisiensi daya antara platform ESP32 dan STM32

### 🔧 Spesifikasi Teknis

#### Platform ESP32

| Parameter | Spesifikasi |
|---|---|
| Mode Sleep | Deep Sleep dengan timer wakeup |
| Interval Wakeup | 5 menit (default), adaptif 10-30 menit saat baterai rendah |
| Sensor | Simulasi sensor suhu & kelembapan via ADC (potentiometer) |
| Penyimpanan Data | RTC memory (8 KB), menyimpan 100 data terakhir |
| Monitoring Baterai | ADC channel terpisah dengan voltage divider |
| Target Arus Sleep | < 10 µA |
| Target Arus Aktif | < 80 mA selama < 2 detik per siklus |
| Framework | ESP-IDF / Arduino Framework |

#### Platform STM32 (STM32F411 / STM32F103)

| Parameter | Spesifikasi |
|---|---|
| Mode Sleep | Standby Mode dengan RTC Alarm wakeup |
| Interval Wakeup | 5 menit (default), adaptif 10-30 menit saat baterai rendah |
| Sensor | Simulasi sensor via ADC (potentiometer) |
| Penyimpanan Data | Backup register (20 × 32-bit pada F411) |
| Monitoring Baterai | ADC channel dengan voltage divider |
| Target Arus Standby | < 3 µA |
| Target Arus Aktif | < 30 mA selama < 1 detik per siklus |
| Framework | STM32 HAL / Arduino Framework |

#### Simulasi Solar Charging

| Parameter | Spesifikasi |
|---|---|
| Input Solar | Simulasi via potentiometer (0-3.3V = 0-100% irradiance) |
| Level Baterai | ADC monitoring dengan threshold 3 level |
| Baterai Penuh (> 80%) | Interval pengukuran: 5 menit |
| Baterai Sedang (30-80%) | Interval pengukuran: 10 menit |
| Baterai Rendah (< 30%) | Interval pengukuran: 30 menit |
| Baterai Kritis (< 10%) | Hanya pengukuran baterai, sensor dimatikan |

#### Perhitungan Power Budget

```
Target: 6 bulan = 4320 jam

Kapasitas Baterai: 3000 mAh
Efisiensi regulator: ~90% → Kapasitas efektif: 2700 mAh

Budget rata-rata: 2700 mAh / 4320 jam = 0.625 mA = 625 µA

Contoh perhitungan ESP32 (interval 5 menit):
- Deep sleep: 10 µA × 298 detik = 2980 µA·s
- Aktif: 80000 µA × 2 detik = 160000 µA·s
- Total per siklus (300 detik): 162980 µA·s
- Arus rata-rata: 162980 / 300 = 543 µA ✓ (di bawah budget)

Contoh perhitungan STM32 (interval 5 menit):
- Standby: 3 µA × 299 detik = 897 µA·s
- Aktif: 30000 µA × 1 detik = 30000 µA·s
- Total per siklus (300 detik): 30897 µA·s
- Arus rata-rata: 30897 / 300 = 103 µA ✓ (jauh di bawah budget)
```

### 📦 Deliverables

1. **Source Code**
   - Firmware ESP32 (deep sleep + ADC + RTC memory + adaptive duty cycle)
   - Firmware STM32 (standby mode + RTC alarm + backup register + adaptive duty cycle)
   - Kode simulasi solar charging untuk kedua platform
   - Komentar kode yang jelas dan terstruktur

2. **Dokumentasi Teknis**
   - Skema rangkaian (Fritzing/KiCad) untuk kedua platform
   - Diagram alur (flowchart) sistem keseluruhan
   - Perhitungan power budget lengkap dengan validasi
   - Tabel perbandingan konsumsi daya ESP32 vs STM32

3. **Laporan Pengujian**
   - Pengukuran arus aktual di setiap mode (multimeter/power analyzer)
   - Grafik konsumsi daya vs waktu (minimal 1 jam pengamatan)
   - Validasi adaptive duty cycle pada berbagai level baterai
   - Screenshot serial monitor yang menunjukkan data logging

4. **Video Demonstrasi**
   - Durasi: 5-8 menit
   - Menunjukkan transisi sleep-wake pada kedua platform
   - Menunjukkan perubahan interval saat level baterai berubah
   - Penjelasan power budget dan estimasi masa pakai baterai

### 📊 Kriteria Penilaian

| Komponen | Bobot | Kriteria |
|---|---|---|
| Implementasi Deep Sleep/Standby | 25% | Mode sleep berfungsi dengan benar, arus sleep sesuai target |
| Adaptive Duty Cycle | 20% | Interval berubah sesuai level baterai, transisi mulus |
| Data Logging & Persistensi | 15% | Data tersimpan di RTC memory/backup register, survive reset |
| Power Budget Analysis | 15% | Perhitungan lengkap, validasi dengan pengukuran aktual |
| Perbandingan ESP32 vs STM32 | 10% | Analisis mendalam perbedaan konsumsi daya kedua platform |
| Dokumentasi & Laporan | 10% | Lengkap, terstruktur, dan profesional |
| Video Demonstrasi | 5% | Jelas, informatif, menunjukkan semua fitur |
| **Total** | **100%** | |

### 📅 Timeline

| Minggu | Kegiatan | Output |
|---|---|---|
| **Minggu 1** | Studi literatur mode sleep ESP32 & STM32. Implementasi basic deep sleep/standby dengan timer wakeup. Pengukuran arus sleep. | Firmware basic sleep-wake berfungsi pada kedua platform |
| **Minggu 2** | Integrasi ADC sensor, data logging ke RTC memory/backup register. Implementasi adaptive duty cycle berdasarkan level baterai. Simulasi solar charging. | Sistem lengkap dengan adaptive duty cycle |
| **Minggu 3** | Pengukuran dan validasi power budget. Perbandingan ESP32 vs STM32. Penyusunan laporan, dokumentasi, dan video demonstrasi. | Laporan lengkap, video demo, presentasi |

---

## Project 2: Industrial Power Monitor System

### 📋 Deskripsi

Project ini mengharuskan mahasiswa membangun **sistem monitor daya industri** yang mampu mengukur konsumsi arus listrik perangkat eksternal secara real-time menggunakan ADC. Sistem dirancang dengan prinsip **self-low-power** — mikrokontroler sendiri harus mengonsumsi daya seminimal mungkin menggunakan teknik peripheral clock gating (STM32) dan dynamic frequency scaling (ESP32).

Sistem mencatat data konsumsi daya secara periodik, melakukan analisis power budget, dan memberikan **alert** ketika konsumsi daya perangkat yang dimonitor melebihi threshold yang ditentukan. Data ditampilkan melalui serial monitor dan disimpan dalam format log terstruktur.

### 🎯 Tujuan

1. Mengimplementasikan pengukuran arus menggunakan ADC dan sensor arus (simulasi shunt resistor)
2. Menerapkan peripheral clock gating pada STM32 untuk meminimalkan konsumsi daya sendiri
3. Menggunakan light sleep dan dynamic frequency scaling pada ESP32
4. Merancang sistem data logging dengan analisis power budget otomatis
5. Mengimplementasikan sistem alert berbasis threshold dengan hysteresis
6. Memahami trade-off antara akurasi sampling dan konsumsi daya

### 🔧 Spesifikasi Teknis

#### Sistem Pengukuran Arus

| Parameter | Spesifikasi |
|---|---|
| Metode Pengukuran | Shunt resistor (simulasi via potentiometer + voltage divider) |
| Range Pengukuran | 0 - 5A (disimulasikan 0 - 3.3V pada ADC) |
| Resolusi ADC | 12-bit (ESP32 & STM32) |
| Sampling Rate | Configurable: 1 Hz, 10 Hz, atau 100 Hz |
| Averaging | Moving average filter (window 10-50 sample) |
| Kalibrasi | Offset dan gain calibration via serial command |

#### Platform ESP32 — Power Optimization

| Fitur | Implementasi |
|---|---|
| Light Sleep | Aktif di antara siklus sampling (auto light sleep) |
| Dynamic Frequency | 240 MHz saat proses data, 80 MHz saat idle, 10 MHz saat sleep |
| WiFi/BT | Dimatikan (modem sleep) saat tidak diperlukan |
| Peripheral | ADC hanya dinyalakan saat sampling, GPIO minimal |
| Target Self-Consumption | < 20 mA rata-rata pada sampling 1 Hz |

#### Platform STM32 — Peripheral Clock Gating

| Fitur | Implementasi |
|---|---|
| Clock Gating | Matikan clock peripheral yang tidak digunakan via RCC register |
| Sleep Mode | Sleep mode (WFI) di antara siklus sampling |
| ADC Clock | Nyalakan hanya saat konversi, matikan setelah selesai |
| GPIO Clock | Hanya enable port yang digunakan |
| UART Clock | Enable saat transmit data, disable saat idle |
| Timer Clock | Satu timer untuk scheduling, sisanya dimatikan |
| Target Self-Consumption | < 8 mA rata-rata pada sampling 1 Hz |

#### Sistem Alert & Logging

| Parameter | Spesifikasi |
|---|---|
| Threshold Level 1 (Warning) | Configurable, default 2A — LED kuning berkedip |
| Threshold Level 2 (Critical) | Configurable, default 4A — LED merah + buzzer |
| Hysteresis | 10% dari threshold untuk mencegah bouncing alert |
| Log Format | Timestamp, arus (mA), daya (mW), status alert |
| Log Storage | Circular buffer di RAM (500 entry terakhir) |
| Report | Statistik per jam: min, max, rata-rata, total energi (mWh) |

#### Power Budget Analysis (Otomatis)

```
Sistem menghitung secara otomatis:

1. Konsumsi daya perangkat yang dimonitor:
   - P_device = V_supply × I_measured
   - Energi kumulatif: E = ΣP × Δt (dalam mWh)

2. Konsumsi daya sistem monitor sendiri:
   - Diestimasi berdasarkan mode operasi aktif
   - Rasio overhead = P_monitor / P_device × 100%
   - Target: overhead < 1% dari daya yang diukur

3. Laporan periodik (setiap jam):
   - Arus rata-rata, minimum, maksimum
   - Total energi yang dikonsumsi (mWh)
   - Estimasi biaya listrik (configurable tarif/kWh)
   - Persentase waktu di atas threshold
```

### 📦 Deliverables

1. **Source Code**
   - Firmware ESP32 dengan light sleep dan dynamic frequency scaling
   - Firmware STM32 dengan peripheral clock gating teroptimasi
   - Modul ADC sampling dengan moving average filter
   - Modul alert system dengan hysteresis
   - Modul data logging dan power budget analysis
   - Konfigurasi parameter via serial command interface

2. **Dokumentasi Teknis**
   - Skema rangkaian simulasi pengukuran arus
   - Diagram blok sistem keseluruhan
   - Register map peripheral clock gating STM32 yang digunakan
   - Diagram state machine untuk sistem alert
   - Tabel konsumsi daya per-peripheral sebelum dan sesudah optimasi

3. **Laporan Pengujian**
   - Perbandingan konsumsi daya STM32 sebelum dan sesudah clock gating
   - Pengukuran akurasi ADC dan linearitas pengukuran arus
   - Validasi sistem alert pada berbagai skenario beban
   - Contoh output log dan laporan power budget
   - Grafik konsumsi daya sistem pada berbagai sampling rate

4. **Video Demonstrasi**
   - Durasi: 5-8 menit
   - Demo pengukuran arus dengan variasi beban (potentiometer)
   - Demo alert system saat threshold terlampaui
   - Menunjukkan perbedaan konsumsi daya sebelum/sesudah optimasi
   - Penjelasan data log dan analisis power budget

### 📊 Kriteria Penilaian

| Komponen | Bobot | Kriteria |
|---|---|---|
| Pengukuran Arus (ADC + Filter) | 20% | Akurasi pengukuran, linearitas, kalibrasi |
| Power Optimization (Clock Gating/Light Sleep) | 25% | Penurunan konsumsi daya terukur, teknik yang digunakan |
| Alert System | 15% | Multi-level alert berfungsi, hysteresis mencegah bouncing |
| Data Logging & Power Budget | 20% | Log terstruktur, analisis otomatis, laporan periodik |
| Serial Command Interface | 5% | Konfigurasi parameter runtime, user-friendly |
| Dokumentasi & Laporan | 10% | Lengkap, analisis mendalam, perbandingan platform |
| Video Demonstrasi | 5% | Jelas, menunjukkan semua fitur dan optimasi |
| **Total** | **100%** | |

### 📅 Timeline

| Minggu | Kegiatan | Output |
|---|---|---|
| **Minggu 1** | Implementasi ADC sampling dengan moving average filter. Setup pengukuran arus (shunt resistor simulasi). Implementasi basic data logging. | ADC membaca dan mencatat data arus dengan akurat |
| **Minggu 2** | Implementasi peripheral clock gating (STM32) dan light sleep (ESP32). Implementasi alert system dengan multi-level threshold dan hysteresis. Serial command interface untuk konfigurasi. | Sistem lengkap dengan power optimization dan alert |
| **Minggu 3** | Implementasi power budget analysis otomatis. Pengukuran dan perbandingan konsumsi daya. Dokumentasi, laporan pengujian, dan video demo. | Laporan lengkap dengan analisis power budget |

---

## Project 3: Ultra-Low-Power Sensor Network Node

### 📋 Deskripsi

Project ini merupakan project paling kompleks yang mengharuskan mahasiswa membangun **node sensor jaringan ultra-low-power** yang menggunakan pendekatan **event-driven** untuk mencapai konsumsi daya seminimal mungkin. Pada ESP32, mahasiswa akan memanfaatkan **ULP (Ultra Low Power) coprocessor** untuk melakukan pemantauan sensor secara periodik tanpa membangunkan CPU utama — CPU utama hanya dibangunkan ketika nilai sensor melampaui threshold tertentu. Pada STM32, mahasiswa menggunakan **Stop Mode** dengan wakeup berbasis **EXTI (External Interrupt)** untuk pendekatan event-driven sensing.

Kedua pendekatan kemudian dibandingkan dengan metode **polling konvensional** untuk menganalisis perbedaan konsumsi daya secara kuantitatif, dan dilakukan **perbandingan menyeluruh** antara kemampuan power management ESP32 dan STM32.

### 🎯 Tujuan

1. Mengimplementasikan ULP coprocessor pada ESP32 untuk monitoring sensor background
2. Menerapkan Stop Mode dengan EXTI wakeup pada STM32 untuk event-driven sensing
3. Membandingkan secara kuantitatif pendekatan polling vs interrupt-driven
4. Menganalisis dan membandingkan fitur power management ESP32 dan STM32
5. Merancang power budget untuk skenario sensor network node
6. Memahami trade-off antara responsivitas dan konsumsi daya

### 🔧 Spesifikasi Teknis

#### ESP32 — ULP Coprocessor Monitoring

| Parameter | Spesifikasi |
|---|---|
| ULP Program | Membaca ADC setiap 500 ms saat main CPU sleep |
| Threshold | Configurable high/low threshold untuk wakeup main CPU |
| Main CPU Sleep | Deep sleep, hanya bangun saat ULP trigger atau setiap 1 jam untuk housekeeping |
| ULP Consumption | ~150 µA saat ULP aktif + deep sleep |
| Main CPU Action | Saat bangun: proses data, simpan ke RTC memory, opsional transmit, lalu kembali tidur |
| Sensor Simulasi | Potentiometer pada ADC pin yang didukung ULP (GPIO32-GPIO39) |
| Data Buffer | RTC slow memory untuk shared data ULP ↔ main CPU |

**Program ULP (Pseudocode):**
```
loop:
    adc_read(channel, result)          // Baca ADC
    store(result, rtc_slow_mem[idx])   // Simpan ke RTC memory
    compare(result, threshold_high)    // Bandingkan dengan threshold
    jump_if_greater(wake_main_cpu)     // Bangunkan CPU jika melebihi
    compare(result, threshold_low)     // Bandingkan dengan threshold bawah
    jump_if_less(wake_main_cpu)        // Bangunkan CPU jika di bawah
    increment(idx)                     // Increment index buffer
    sleep(500ms)                       // Tidur 500ms
    jump(loop)                         // Ulangi

wake_main_cpu:
    wake()                             // Trigger wakeup main CPU
    halt()                             // ULP berhenti
```

#### STM32 — Stop Mode dengan EXTI Wakeup

| Parameter | Spesifikasi |
|---|---|
| Mode Sleep | Stop Mode (semua clock berhenti, RAM retained) |
| Wakeup Source | EXTI line dari comparator atau external interrupt pin |
| Sensor Simulasi | Potentiometer + komparator hardware (atau GPIO toggle manual) |
| Wakeup Latency | ~5 µs (Stop Mode ke Run Mode) |
| Stop Mode Current | ~20 µA (STM32F411), ~6 µA (STM32F103) |
| Main CPU Action | Saat bangun: reconfigure clock, baca ADC, proses, log, kembali ke Stop |
| Threshold Detection | Analog watchdog ADC atau external comparator |

**Konfigurasi Stop Mode:**
```
Langkah masuk Stop Mode:
1. Set SLEEPDEEP bit di SCB->SCR
2. Set PDDS=0 (Stop mode, bukan Standby) di PWR->CR
3. Clear WUF flag di PWR->CSR
4. Configure EXTI line dan NVIC
5. Execute WFI (Wait For Interrupt)

Langkah bangun dari Stop Mode:
1. EXTI interrupt terjadi → CPU bangun
2. Reconfigure system clock (HSE/PLL) — clock kembali ke HSI setelah Stop
3. Baca ADC, proses data
4. Kembali ke Stop Mode
```

#### Mode Perbandingan: Polling vs Interrupt-Driven

| Aspek | Polling | Interrupt-Driven (ULP/EXTI) |
|---|---|---|
| **Metode** | CPU aktif terus, baca sensor periodik | CPU tidur, bangun saat event |
| **Sampling ESP32** | Light sleep + timer wakeup setiap 500 ms | ULP baca setiap 500 ms, CPU tidur |
| **Sampling STM32** | Sleep mode + timer wakeup setiap 500 ms | Stop mode + EXTI wakeup saat threshold |
| **Arus ESP32 Polling** | ~20 mA rata-rata | ~0.15 mA (ULP) + burst saat event |
| **Arus STM32 Polling** | ~8 mA rata-rata | ~0.02 mA (Stop) + burst saat event |
| **Responsivitas** | Tetap (setiap 500 ms) | Variabel (instant saat threshold) |
| **Kompleksitas** | Rendah | Tinggi (ULP programming, EXTI config) |

#### Power Budget Comparison

```
Skenario: Sensor node, event terjadi rata-rata 10× per jam
Baterai: 3000 mAh Li-Ion

═══════════════════════════════════════════════════════
ESP32 — Polling (Light Sleep + Timer 500ms)
═══════════════════════════════════════════════════════
Arus rata-rata     : ~20 mA
Masa pakai baterai : 3000 / 20 = 150 jam = 6.25 hari

═══════════════════════════════════════════════════════
ESP32 — ULP Monitoring (Deep Sleep + ULP)
═══════════════════════════════════════════════════════
ULP aktif          : 150 µA (kontinu)
Main CPU wakeup    : 80 mA × 2 detik × 10/jam = 0.44 mA rata-rata
Arus rata-rata     : ~0.59 mA
Masa pakai baterai : 3000 / 0.59 = 5084 jam = 212 hari ≈ 7 bulan

═══════════════════════════════════════════════════════
STM32 — Polling (Sleep + Timer 500ms)
═══════════════════════════════════════════════════════
Arus rata-rata     : ~8 mA
Masa pakai baterai : 3000 / 8 = 375 jam = 15.6 hari

═══════════════════════════════════════════════════════
STM32 — Stop Mode + EXTI Wakeup
═══════════════════════════════════════════════════════
Stop mode          : 20 µA (kontinu)
Wakeup             : 30 mA × 1 detik × 10/jam = 0.083 mA rata-rata
Arus rata-rata     : ~0.103 mA
Masa pakai baterai : 3000 / 0.103 = 29126 jam = 1213 hari ≈ 3.3 tahun

═══════════════════════════════════════════════════════
RINGKASAN PERBANDINGAN
═══════════════════════════════════════════════════════
| Platform  | Polling    | Event-Driven | Peningkatan |
|-----------|------------|--------------|-------------|
| ESP32     | 6.25 hari  | 212 hari     | 34×         |
| STM32     | 15.6 hari  | 3.3 tahun    | 77×         |
```

### 📦 Deliverables

1. **Source Code**
   - Firmware ESP32 mode polling (light sleep + timer wakeup)
   - Firmware ESP32 mode ULP (ULP coprocessor + deep sleep)
   - Firmware STM32 mode polling (sleep + timer wakeup)
   - Firmware STM32 mode event-driven (Stop mode + EXTI wakeup)
   - Script pengukuran dan pencatatan konsumsi daya
   - Komentar kode yang menjelaskan setiap optimasi daya

2. **Dokumentasi Teknis**
   - Skema rangkaian untuk kedua platform (polling & event-driven)
   - Arsitektur sistem ULP coprocessor ESP32 (diagram blok)
   - Konfigurasi Stop Mode dan EXTI pada STM32 (register-level)
   - Diagram state machine untuk setiap mode operasi
   - Diagram alur program ULP

3. **Laporan Analisis Perbandingan**
   - Tabel perbandingan arus di setiap mode untuk kedua platform
   - Grafik konsumsi daya: polling vs event-driven (kedua platform)
   - Perhitungan power budget lengkap untuk 4 skenario
   - Analisis trade-off: responsivitas vs konsumsi daya
   - Analisis trade-off: kompleksitas implementasi vs penghematan daya
   - Rekomendasi pemilihan platform dan metode berdasarkan use case

4. **Video Demonstrasi**
   - Durasi: 8-12 menit
   - Demo mode polling pada kedua platform
   - Demo mode event-driven (ULP dan Stop+EXTI)
   - Perbandingan pengukuran arus secara live
   - Presentasi hasil analisis dan rekomendasi

### 📊 Kriteria Penilaian

| Komponen | Bobot | Kriteria |
|---|---|---|
| ESP32 ULP Coprocessor | 20% | ULP program berjalan, threshold detection berfungsi, main CPU hanya bangun saat diperlukan |
| STM32 Stop Mode + EXTI | 20% | Stop mode tercapai, wakeup via EXTI berfungsi, clock recovery benar |
| Mode Polling (Baseline) | 10% | Implementasi polling berfungsi sebagai baseline perbandingan |
| Pengukuran & Validasi Daya | 15% | Pengukuran arus aktual, data valid dan reprodusibel |
| Power Budget Analysis | 15% | Perhitungan lengkap 4 skenario, validasi dengan pengukuran |
| Perbandingan ESP32 vs STM32 | 10% | Analisis mendalam, kesimpulan berdasarkan data |
| Dokumentasi & Laporan | 5% | Lengkap, profesional, analisis kritis |
| Video Demonstrasi | 5% | Jelas, komprehensif, menunjukkan semua mode |
| **Total** | **100%** | |

### 📅 Timeline

| Minggu | Kegiatan | Output |
|---|---|---|
| **Minggu 1** | Implementasi mode polling pada ESP32 (light sleep + timer) dan STM32 (sleep + timer). Pengukuran baseline konsumsi daya polling. Setup pengukuran arus. | Baseline polling berfungsi, data konsumsi daya tercatat |
| **Minggu 2** | Implementasi ULP coprocessor pada ESP32 (program ULP, threshold, deep sleep). Implementasi Stop Mode + EXTI pada STM32. Pengukuran konsumsi daya event-driven. | Mode event-driven berfungsi pada kedua platform |
| **Minggu 3** | Analisis perbandingan polling vs event-driven. Power budget calculation 4 skenario. Perbandingan ESP32 vs STM32. Penyusunan laporan dan video demo. | Laporan analisis lengkap, video demo, presentasi |

---

## Petunjuk Umum

### Peralatan yang Dibutuhkan

| Peralatan | Keterangan |
|---|---|
| ESP32 DevKit | ESP32-WROOM-32 atau ESP32-S3 |
| STM32 Board | STM32F411 BlackPill atau STM32F103 BluePill |
| Multimeter Digital | Untuk pengukuran arus (range µA - mA) |
| Potentiometer | 10KΩ untuk simulasi sensor dan solar panel |
| LED + Resistor | Indikator status dan alert |
| Buzzer (opsional) | Alert suara pada Project 2 |
| Breadboard + Kabel | Prototyping |
| USB Power Meter (opsional) | Pengukuran daya yang lebih akurat |

### Tips Pengukuran Daya

1. **Gunakan multimeter pada mode µA/mA** — pastikan range sesuai agar tidak merusak fuse
2. **Seri-kan multimeter** di jalur power supply ke mikrokontroler
3. **Lepas LED power onboard** jika memungkinkan (biasanya 1-3 mA)
4. **Lepas USB-UART bridge** saat mengukur deep sleep (biasanya 10-20 mA)
5. **Gunakan rata-rata** minimal 10 pengukuran untuk arus steady-state
6. **Catat suhu ruangan** karena mempengaruhi konsumsi daya leakage

### Referensi

1. ESP32 Technical Reference Manual — Chapter: ULP Coprocessor, RTC & Power Management
2. STM32F411 Reference Manual — Chapter: Power Control (PWR), RTC, Low-Power Modes
3. STM32F103 Reference Manual — Chapter: Power Control, Backup Registers
4. AN4621: STM32 Low-Power Modes Application Note
5. Espressif: ESP32 ULP Coprocessor Programming Guide
6. Battery Life Calculator: [Oregon Embedded](https://oregonembedded.com/batterycalc.htm)

---

*Modul 14 — Power Management | Praktikum Sistem Embedded*
*Pilih minimal 1 project sesuai tingkat kemampuan dan ketersediaan hardware*
