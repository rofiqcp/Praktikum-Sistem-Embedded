# Jobsheet Modul 14: Power Management pada Sistem Embedded

## Informasi Umum

| Item | Detail |
|------|--------|
| **Modul** | 14 — Power Management |
| **Jumlah Percobaan** | 24 (12 ESP32 + 12 STM32) |
| **Platform** | ESP32 (ESP-IDF) & STM32F103 (STM32Cube HAL) |
| **Tools** | PlatformIO, Serial Monitor, Python (matplotlib, serial) |
| **Durasi** | 4 × pertemuan @ 150 menit |

---

## Daftar Percobaan

### Platform ESP32 (ESP-IDF Framework)

| No | Folder | Judul Percobaan |
|----|--------|-----------------|
| 1 | `ESP32_01_Current_Measurement` | Pengukuran Arus pada Berbagai Mode Operasi |
| 2 | `ESP32_02_Light_Sleep_Basic` | Light Sleep dengan Timer Wakeup |
| 3 | `ESP32_03_Deep_Sleep_Timer` | Deep Sleep dengan Timer Wakeup |
| 4 | `ESP32_04_Deep_Sleep_GPIO` | Deep Sleep dengan GPIO (ext0/ext1) Wakeup |
| 5 | `ESP32_05_Deep_Sleep_RTC` | Deep Sleep dengan RTC Memory Persistence |
| 6 | `ESP32_06_Sleep_Data_Retention` | Data Retention — Menyimpan Data Antar Deep Sleep |
| 7 | `ESP32_07_Touch_Wakeup` | Touch Pad Wakeup dari Deep Sleep |
| 8 | `ESP32_08_ULP_Coprocessor` | ULP Coprocessor — Monitoring Saat CPU Tidur |
| 9 | `ESP32_09_Dynamic_Frequency` | Dynamic Frequency Scaling (DFS) |
| 10 | `ESP32_10_Peripheral_Power_Gate` | Peripheral Power Gating & Clock Control |
| 11 | `ESP32_11_Battery_Powered_Logger` | Sistem Data Logger Battery-Powered |
| 12 | `ESP32_12_Power_Budget_Analysis` | Power Budget Analysis — Estimasi Battery Life |

### Platform STM32F103 (STM32Cube HAL Framework)

| No | Folder | Judul Percobaan |
|----|--------|-----------------|
| 13 | `STM32_01_Current_Measurement` | Pengukuran Arus pada Berbagai Mode Operasi |
| 14 | `STM32_02_Light_Sleep_Basic` | Sleep Mode (WFI) — CPU Clock Off |
| 15 | `STM32_03_Deep_Sleep_Timer` | Stop Mode dengan RTC Alarm Wakeup |
| 16 | `STM32_04_Deep_Sleep_GPIO` | Stop Mode dengan EXTI GPIO Wakeup |
| 17 | `STM32_05_Deep_Sleep_RTC` | Standby Mode dengan RTC Wakeup |
| 18 | `STM32_06_Sleep_Data_Retention` | Data Retention — Backup Register |
| 19 | `STM32_07_Touch_Wakeup` | EXTI Multi-Pin Wakeup (Simulasi Touch) |
| 20 | `STM32_08_ULP_Coprocessor` | Peripheral Clock Gating — Hemat Daya Periodik |
| 21 | `STM32_09_Dynamic_Frequency` | Dynamic Frequency — Switching Clock Speed |
| 22 | `STM32_10_Peripheral_Power_Gate` | Peripheral Power Gate — Matikan Modul Tak Terpakai |
| 23 | `STM32_11_Battery_Powered_Logger` | Sistem Data Logger Battery-Powered |
| 24 | `STM32_12_Power_Budget_Analysis` | Power Budget Analysis — Estimasi Battery Life |

---

## Percobaan 1: ESP32 — Pengukuran Arus pada Berbagai Mode Operasi

### Tujuan
- Memahami cara mengukur konsumsi arus mikrokontroler
- Membandingkan konsumsi arus pada mode Active, Light Sleep, dan Deep Sleep
- Menggunakan shunt resistor dan ADC untuk pengukuran arus

### Alat dan Bahan
- ESP32 DevKit V1
- Resistor shunt 1Ω (untuk pengukuran arus)
- Multimeter
- Kabel jumper

### Langkah Kerja
1. Build dan upload program: `cd praktikum/ESP32/ESP32_01_Current_Measurement && pio run -t upload`
2. Buka Serial Monitor (115200 baud)
3. Amati output yang menunjukkan mode operasi dan estimasi konsumsi
4. Gunakan multimeter untuk mengukur arus aktual pada setiap mode
5. Catat dan bandingkan hasil pengukuran dengan estimasi program
6. Jalankan `python3 debug_analysis.py` untuk analisis grafis

### Tabel Pengamatan

| Mode | Arus Estimasi (mA) | Arus Terukur (mA) | Durasi (s) |
|------|--------------------|--------------------|------------|
| Active (240MHz) | | | |
| Active (80MHz) | | | |
| Light Sleep | | | |
| Deep Sleep | | | |

### Pertanyaan
1. Mengapa arus pada mode Active bervariasi tergantung frekuensi CPU?
2. Berapa rasio penghematan daya antara Active dan Deep Sleep?
3. Apa komponen yang masih aktif saat Deep Sleep?

---

## Percobaan 2: ESP32 — Light Sleep dengan Timer Wakeup

### Tujuan
- Memahami mekanisme Light Sleep pada ESP32
- Mengimplementasikan timer-based wakeup
- Menganalisis wakeup latency dan power saving

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_02_Light_Sleep_Basic && pio run -t upload`
2. Buka Serial Monitor (115200 baud)
3. Amati siklus tidur-bangun dengan log timestamp
4. Catat waktu tidur, waktu bangun, dan selisihnya
5. Jalankan `python3 debug_analysis.py` untuk visualisasi

### Tabel Pengamatan

| Siklus | Waktu Sebelum Sleep (ms) | Waktu Setelah Wakeup (ms) | Selisih (ms) | Wakeup Cause |
|--------|--------------------------|---------------------------|--------------|--------------|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |

### Pertanyaan
1. Apakah variabel lokal tetap bernilai setelah wakeup dari Light Sleep? Mengapa?
2. Berapa overhead waktu wakeup dari Light Sleep?
3. Apa perbedaan Light Sleep dan Deep Sleep dari sisi data retention?

---

## Percobaan 3: ESP32 — Deep Sleep dengan Timer Wakeup

### Tujuan
- Mengimplementasikan Deep Sleep pada ESP32
- Menggunakan RTC timer sebagai wakeup source
- Memahami perilaku reset setelah deep sleep

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_03_Deep_Sleep_Timer && pio run -t upload`
2. Buka Serial Monitor (115200 baud)
3. Amati boot count yang increment setiap wakeup (menggunakan RTC_DATA_ATTR)
4. Perhatikan bahwa variabel non-RTC kembali ke nilai awal
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Boot ke- | Wakeup Cause | Waktu Aktif (ms) | Boot Count (RTC) | Variabel Normal |
|----------|-------------|-------------------|-------------------|-----------------|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |

### Pertanyaan
1. Mengapa `boot_count` (RTC_DATA_ATTR) tetap increment tapi variabel biasa selalu reset?
2. Apa yang terjadi pada GPIO state saat ESP32 masuk deep sleep?
3. Bagaimana cara menentukan wakeup cause setelah bangun?

---

## Percobaan 4: ESP32 — Deep Sleep dengan GPIO Wakeup

### Tujuan
- Menggunakan ext0 dan ext1 wakeup dari deep sleep
- Memahami perbedaan ext0 (1 pin) dan ext1 (multi pin)
- Mengidentifikasi pin mana yang membangunkan MCU

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_04_Deep_Sleep_GPIO && pio run -t upload`
2. Buka Serial Monitor
3. Tunggu ESP32 masuk deep sleep (LED indikator mati)
4. Tekan push button pada pin yang dikonfigurasi
5. Amati log wakeup cause dan pin yang mentrigger
6. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Wakeup ke- | Source (ext0/ext1) | Pin GPIO | Level Trigger | Wakeup Time (ms) |
|------------|-------------------|----------|---------------|-------------------|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |

### Pertanyaan
1. Apa perbedaan ext0 dan ext1 wakeup? Kapan menggunakan masing-masing?
2. Mengapa ext0 hanya bisa 1 pin sedangkan ext1 bisa multiple pin?
3. Pin GPIO apa saja yang bisa digunakan untuk ext0/ext1 (batasan RTC GPIO)?

---

## Percobaan 5: ESP32 — Deep Sleep dengan RTC Memory Persistence

### Tujuan
- Menyimpan data sensor ke RTC memory antar deep sleep cycle
- Mengimplementasikan rolling buffer di RTC memory
- Menghitung rata-rata data yang tersimpan

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_05_Deep_Sleep_RTC && pio run -t upload`
2. Buka Serial Monitor
3. Amati data yang disimpan di RTC memory setiap wakeup
4. Perhatikan bahwa data bertahan meskipun CPU reset
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Boot ke- | Data Baru | Total Data RTC | Rata-rata | Memory Used (bytes) |
|----------|-----------|----------------|-----------|---------------------|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |

### Pertanyaan
1. Berapa kapasitas maksimum RTC SLOW memory pada ESP32?
2. Apa yang terjadi jika data RTC melebihi kapasitas memory?
3. Bandingkan RTC_DATA_ATTR dan RTC_NOINIT_ATTR — apa perbedaannya?

---

## Percobaan 6: ESP32 — Data Retention — Menyimpan Data Antar Deep Sleep

### Tujuan
- Mengimplementasikan penyimpanan state kompleks di RTC memory
- Menggunakan struct di RTC memory
- Mendeteksi cold boot vs warm boot

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_06_Sleep_Data_Retention && pio run -t upload`
2. Buka Serial Monitor
3. Amati perbedaan cold boot (power-on) dan warm boot (dari deep sleep)
4. Cabut power untuk menghapus RTC memory, lalu nyalakan kembali
5. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Bagaimana cara membedakan cold boot dan warm boot (dari deep sleep)?
2. Apa keuntungan menggunakan struct di RTC memory dibanding variabel terpisah?
3. Apa terjadi pada RTC memory saat hibernation mode?

---

## Percobaan 7: ESP32 — Touch Pad Wakeup dari Deep Sleep

### Tujuan
- Menggunakan kapasitif touch pad sebagai wakeup source
- Mengkonfigurasi threshold touch sensitivity
- Mengidentifikasi touch pad mana yang mentrigger wakeup

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_07_Touch_Wakeup && pio run -t upload`
2. Buka Serial Monitor
3. Sentuh pin touch pad (GPIO4 = T0) saat ESP32 dalam deep sleep
4. Amati log wakeup cause dan touch pad number
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Wakeup ke- | Touch Pad | Nilai Threshold | Nilai Terukur | Response Time |
|------------|-----------|-----------------|---------------|---------------|
| 1 | | | | |
| 2 | | | | |

### Pertanyaan
1. Bagaimana cara menentukan threshold yang optimal untuk touch detection?
2. Apa pengaruh kelembaban terhadap sensitivitas touch pad?
3. Berapa banyak touch pad yang tersedia pada ESP32?

---

## Percobaan 8: ESP32 — ULP Coprocessor — Monitoring Saat CPU Tidur

### Tujuan
- Memahami konsep ULP Coprocessor
- Mensimulasikan operasi ULP (monitoring periodik saat deep sleep)
- Membandingkan konsumsi daya ULP vs main CPU

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_08_ULP_Coprocessor && pio run -t upload`
2. Buka Serial Monitor
3. Amati simulasi ULP yang membaca sensor secara periodik
4. Perhatikan kapan main CPU dibangunkan oleh ULP
5. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Apa keuntungan ULP dibanding main CPU untuk monitoring periodik?
2. Instruksi apa saja yang bisa dijalankan oleh ULP FSM?
3. Berapa konsumsi daya ULP dibanding main CPU dalam mode Active?

---

## Percobaan 9: ESP32 — Dynamic Frequency Scaling (DFS)

### Tujuan
- Mengimplementasikan DFS pada ESP32
- Mengukur pengaruh frekuensi terhadap konsumsi daya
- Memahami PM lock untuk critical section

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_09_Dynamic_Frequency && pio run -t upload`
2. Buka Serial Monitor
3. Amati perubahan frekuensi CPU saat beban berubah
4. Perhatikan transisi antara frekuensi max dan min
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Mode | Frekuensi (MHz) | Estimasi Arus (mA) | Kecepatan Eksekusi | Catatan |
|------|-----------------|--------------------|--------------------|---------|
| Max Frequency | | | | |
| Min Frequency | | | | |
| Auto DFS | | | | |

### Pertanyaan
1. Bagaimana hubungan frekuensi CPU dengan konsumsi daya?
2. Apa fungsi PM lock dan kapan harus digunakan?
3. Mengapa DFS lebih efisien daripada selalu menggunakan frekuensi rendah?

---

## Percobaan 10: ESP32 — Peripheral Power Gating & Clock Control

### Tujuan
- Mematikan peripheral yang tidak digunakan
- Mengontrol power domain pada ESP32
- Mengukur penghematan daya dari clock gating

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_10_Peripheral_Power_Gate && pio run -t upload`
2. Buka Serial Monitor
3. Amati arus saat semua peripheral aktif vs saat di-gate
4. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Peripheral apa yang mengonsumsi daya terbesar pada ESP32?
2. Apa perbedaan clock gating dan power gating?
3. Mengapa WiFi dan Bluetooth harus di-deinit sebelum deep sleep?

---

## Percobaan 11: ESP32 — Sistem Data Logger Battery-Powered

### Tujuan
- Membangun sistem data logger lengkap dengan battery management
- Mengimplementasikan duty cycling (sleep-wake-log-sleep)
- Monitoring level baterai via ADC

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_11_Battery_Powered_Logger && pio run -t upload`
2. Buka Serial Monitor
3. Amati siklus: bangun → baca sensor → log data → cek baterai → tidur
4. Perhatikan penanganan low battery (menambah interval sleep)
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Cycle | Sensor Value | Battery (V) | Battery (%) | Sleep Duration (s) | Mode |
|-------|-------------|-------------|-------------|-------------------|------|
| 1 | | | | | |
| 2 | | | | | |
| 3 | | | | | |

### Pertanyaan
1. Mengapa interval sleep diperbesar saat baterai rendah?
2. Bagaimana cara mengoptimalkan duty cycle untuk memaksimalkan battery life?
3. Apa strategi logging yang efisien dari sisi power?

---

## Percobaan 12: ESP32 — Power Budget Analysis

### Tujuan
- Melakukan power budget analysis lengkap
- Menghitung estimasi battery life
- Membandingkan strategi optimasi daya

### Langkah Kerja
1. Build dan upload: `cd praktikum/ESP32/ESP32_12_Power_Budget_Analysis && pio run -t upload`
2. Buka Serial Monitor
3. Amati kalkulasi power budget yang dilakukan otomatis
4. Bandingkan skenario tanpa dan dengan optimasi
5. Jalankan `python3 debug_analysis.py` untuk visualisasi

### Tabel Pengamatan

| Skenario | I_active (mA) | I_sleep (μA) | Duty Cycle (%) | I_avg (mA) | Battery Life (hari) |
|----------|---------------|-------------|----------------|------------|---------------------|
| Tanpa Optimasi | | | | | |
| DFS Only | | | | | |
| Deep Sleep | | | | | |
| Full Optimasi | | | | | |

### Pertanyaan
1. Komponen mana yang paling banyak mengonsumsi energi dalam satu siklus?
2. Strategi optimasi mana yang memberikan penghematan terbesar?
3. Bagaimana cara menghitung battery life jika menggunakan solar panel?

---

## Percobaan 13: STM32 — Pengukuran Arus pada Berbagai Mode Operasi

### Tujuan
- Mengukur konsumsi arus STM32F103 pada mode Run, Sleep, Stop, Standby
- Membandingkan dengan datasheet
- Memahami kontributor utama konsumsi daya

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_01_Current_Measurement && pio run -t upload`
2. Buka Serial Monitor (115200 baud)
3. Gunakan multimeter serial untuk mengukur arus
4. Program akan berpindah mode secara otomatis
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Mode | Arus Datasheet (mA) | Arus Terukur (mA) | Clock Speed | Catatan |
|------|---------------------|--------------------| ------------|---------|
| Run @72MHz | ~30 | | 72MHz | |
| Run @8MHz | ~10 | | 8MHz (HSI) | |
| Sleep | ~10-15 | | CPU off | |
| Stop | ~0.02 | | All off | |
| Standby | ~0.002 | | All off | |

### Pertanyaan
1. Mengapa arus mode Run @8MHz jauh lebih rendah dari @72MHz?
2. Apa perbedaan mendasar antara Stop dan Standby dari sisi hardware?
3. Komponen apa yang masih aktif di Standby mode?

---

## Percobaan 14: STM32 — Sleep Mode (WFI)

### Tujuan
- Mengimplementasikan Sleep mode dengan WFI (Wait For Interrupt)
- Memahami bahwa hanya CPU clock yang dimatikan
- Menggunakan interrupt untuk wakeup

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_02_Light_Sleep_Basic && pio run -t upload`
2. Buka Serial Monitor
3. Amati siklus sleep-wakeup yang dipicu oleh interrupt
4. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Apa perbedaan WFI dan WFE?
2. Mengapa harus `HAL_SuspendTick()` sebelum sleep?
3. Interrupt apa saja yang bisa membangunkan dari Sleep mode?

---

## Percobaan 15: STM32 — Stop Mode dengan RTC Alarm Wakeup

### Tujuan
- Mengimplementasikan Stop mode (low-power regulator)
- Menggunakan RTC Alarm sebagai wakeup source
- Rekonfigurasi clock setelah wakeup

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_03_Deep_Sleep_Timer && pio run -t upload`
2. Buka Serial Monitor
3. Amati proses: active → Stop → RTC wakeup → reconfigure clock → active
4. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Mengapa clock harus dikonfigurasi ulang setelah keluar dari Stop mode?
2. Apa yang terjadi pada RAM dan register saat Stop mode?
3. Berapa waktu yang dibutuhkan untuk wakeup dari Stop mode?

---

## Percobaan 16: STM32 — Stop Mode dengan EXTI GPIO Wakeup

### Tujuan
- Menggunakan EXTI line (GPIO) sebagai wakeup dari Stop mode
- Mengkonfigurasi GPIO interrupt untuk wakeup
- Mengamati debounce saat wakeup dari button

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_04_Deep_Sleep_GPIO && pio run -t upload`
2. Buka Serial Monitor
3. Tunggu STM32 masuk Stop mode (LED mati)
4. Tekan push button pada PA0
5. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Pin EXTI mana saja yang bisa digunakan untuk wakeup dari Stop mode?
2. Apa perbedaan wakeup via EXTI di Stop mode vs Standby mode?
3. Mengapa perlu debounce pada button wakeup?

---

## Percobaan 17: STM32 — Standby Mode dengan RTC Wakeup

### Tujuan
- Mengimplementasikan Standby mode (konsumsi terendah)
- Memahami bahwa Standby = full reset (RAM hilang)
- Menggunakan RTC alarm untuk periodic wakeup

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_05_Deep_Sleep_RTC && pio run -t upload`
2. Buka Serial Monitor
3. Amati bahwa setiap wakeup = fresh start (boot counter menggunakan backup register)
4. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Mengapa Standby mode memerlukan VBAT untuk RTC?
2. Apa yang terjadi pada backup register saat Standby?
3. Bandingkan Standby mode STM32 dengan Deep Sleep ESP32 dari sisi data retention.

---

## Percobaan 18: STM32 — Data Retention — Backup Register

### Tujuan
- Menggunakan Backup Register untuk menyimpan data antar Standby cycle
- Menyimpan boot counter dan status di backup register
- Memahami keterbatasan (hanya 20 bytes)

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_06_Sleep_Data_Retention && pio run -t upload`
2. Buka Serial Monitor
3. Amati boot counter yang bertahan di backup register
4. Cabut power untuk melihat data hilang (kecuali VBAT tersambung)
5. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Berapa jumlah dan ukuran backup register pada STM32F103?
2. Bagaimana cara membagi data 32-bit ke dalam register 16-bit?
3. Apa syarat agar backup register tetap aktif saat power off?

---

## Percobaan 19: STM32 — EXTI Multi-Pin Wakeup

### Tujuan
- Mengkonfigurasi multiple EXTI pin untuk wakeup
- Mengidentifikasi pin mana yang membangunkan MCU
- Membandingkan dengan touch pad wakeup ESP32

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_07_Touch_Wakeup && pio run -t upload`
2. Buka Serial Monitor
3. Tekan button pada berbagai pin EXTI
4. Amati log yang menunjukkan pin mana yang mentrigger
5. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Berapa banyak EXTI line yang tersedia pada STM32F103?
2. Apakah dua pin berbeda bisa berbagi EXTI line yang sama?
3. Bandingkan mekanisme touch wakeup ESP32 dengan EXTI wakeup STM32.

---

## Percobaan 20: STM32 — Peripheral Clock Gating

### Tujuan
- Mematikan clock ke peripheral yang tidak digunakan
- Mengukur penghematan daya dari clock gating
- Memahami register RCC untuk kontrol clock

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_08_ULP_Coprocessor && pio run -t upload`
2. Buka Serial Monitor
3. Amati arus saat semua peripheral aktif vs minimal
4. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Peripheral apa yang mengonsumsi clock terbesar pada STM32F103?
2. Bagaimana cara mematikan clock ke GPIO port yang tidak digunakan?
3. Apa dampak mematikan clock ke peripheral yang sedang digunakan?

---

## Percobaan 21: STM32 — Dynamic Frequency — Switching Clock Speed

### Tujuan
- Mengubah frekuensi system clock secara runtime
- Mengukur pengaruh frekuensi terhadap konsumsi arus
- Memahami konfigurasi PLL dan clock source

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_09_Dynamic_Frequency && pio run -t upload`
2. Buka Serial Monitor
3. Amati transisi frekuensi: 72MHz → 36MHz → 8MHz → 72MHz
4. Perhatikan perubahan baud rate UART pada setiap frekuensi
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Frekuensi | Sumber Clock | Arus Terukur (mA) | LED Blink Speed | UART OK? |
|-----------|-------------|--------------------| ----------------|----------|
| 72 MHz | HSE + PLL | | | |
| 36 MHz | HSE + PLL/2 | | | |
| 8 MHz | HSI | | | |

### Pertanyaan
1. Mengapa UART baud rate berubah saat frekuensi diganti?
2. Bagaimana cara menjaga UART tetap bekerja setelah ganti frekuensi?
3. Apa trade-off antara frekuensi rendah dan kecepatan eksekusi?

---

## Percobaan 22: STM32 — Peripheral Power Gate

### Tujuan
- Mematikan tegangan ke modul yang tidak digunakan
- Mengontrol GPIO untuk power switch modul eksternal
- Mengukur total penghematan daya

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_10_Peripheral_Power_Gate && pio run -t upload`
2. Buka Serial Monitor
3. Amati log yang menunjukkan peripheral di-enable dan di-disable
4. Jalankan `python3 debug_analysis.py`

### Pertanyaan
1. Apa perbedaan clock gating dan power gating dari sisi implementasi?
2. Mengapa GPIO harus dikonfigurasi ke analog mode saat sleep?
3. Bagaimana cara mengontrol power supply modul eksternal via MOSFET?

---

## Percobaan 23: STM32 — Sistem Data Logger Battery-Powered

### Tujuan
- Membangun data logger lengkap dengan power management STM32
- Mengimplementasikan duty cycling: wakeup → read → log → sleep
- Monitoring baterai via ADC

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_11_Battery_Powered_Logger && pio run -t upload`
2. Buka Serial Monitor
3. Amati siklus data logging dengan deep sleep
4. Perhatikan respons saat baterai rendah
5. Jalankan `python3 debug_analysis.py`

### Tabel Pengamatan

| Cycle | ADC Value | Battery (V) | Status | Sleep Mode | Sleep Duration |
|-------|-----------|-------------|--------|------------|----------------|
| 1 | | | | | |
| 2 | | | | | |
| 3 | | | | | |

### Pertanyaan
1. Mengapa data logger harus menggunakan mode sleep paling dalam?
2. Bagaimana cara menyimpan log data tanpa RAM (Standby mode)?
3. Apa keuntungan menggunakan backup register untuk boot counter?

---

## Percobaan 24: STM32 — Power Budget Analysis

### Tujuan
- Melakukan power budget analysis untuk STM32
- Menghitung estimasi battery life
- Membandingkan power profile STM32 vs ESP32

### Langkah Kerja
1. Build dan upload: `cd praktikum/STM32/STM32_12_Power_Budget_Analysis && pio run -t upload`
2. Buka Serial Monitor
3. Amati kalkulasi power budget otomatis
4. Bandingkan skenario optimasi
5. Jalankan `python3 debug_analysis.py`

### Tabel Perbandingan ESP32 vs STM32

| Parameter | ESP32 | STM32F103 |
|-----------|-------|-----------|
| Active Current | | |
| Sleep Current | | |
| Deep Sleep Current | | |
| Wakeup Time | | |
| RTC Memory | | |
| Battery Life (estimasi) | | |

### Pertanyaan
1. Platform mana yang lebih hemat daya secara keseluruhan? Mengapa?
2. Dalam skenario apa STM32 lebih baik dari ESP32 untuk battery-powered?
3. Bagaimana cara mengoptimalkan power budget jika target battery life 1 tahun?

---

## Petunjuk Umum

### Build & Upload
```bash
# Masuk ke folder percobaan
cd praktikum/ESP32/ESP32_01_Current_Measurement

# Build saja
pio run

# Build dan upload
pio run -t upload

# Monitor serial
pio device monitor -b 115200
```

### Python Debug & Analysis
```bash
# Install dependencies
pip install pyserial matplotlib numpy

# Jalankan debug script
python3 debug_analysis.py
```

### Troubleshooting
1. **Upload gagal**: Tekan tombol BOOT saat upload (ESP32) atau gunakan ST-Link (STM32)
2. **Serial tidak muncul**: Cek COM port di Device Manager / `ls /dev/ttyUSB*`
3. **Build error**: Jalankan `pio run -v` untuk verbose output
4. **Deep Sleep tidak wakeup**: Cek wiring wakeup pin dan pull-up/pull-down
5. **Arus terlalu tinggi di sleep**: Cek apakah LED, sensor, atau modul masih tersambung

---

*Jobsheet Modul 14 — Power Management pada Sistem Embedded*
