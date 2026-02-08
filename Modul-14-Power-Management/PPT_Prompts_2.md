# PPT Prompts — Modul 14: Power Management (Bagian Praktikum)

## Panduan Pembuatan Slide Praktikum

---

### Slide 1: Halaman Judul Praktikum
**Judul**: Praktikum Modul 14 — Power Management
**Konten**:
- 24 Percobaan: 12 ESP32 (ESP-IDF) + 12 STM32 (STM32Cube HAL)
- Tools: PlatformIO, Serial Monitor, Python (matplotlib, serial)
- Hardware: ESP32 DevKit, STM32 Blue Pill, multimeter, resistor shunt

---

### Slide 2: Overview Percobaan ESP32
**Judul**: Daftar Percobaan ESP32
**Konten**:
| No | Percobaan | Topik Utama |
|----|-----------|-------------|
| 1 | Current Measurement | Pengukuran arus berbagai mode |
| 2 | Light Sleep Basic | Timer wakeup, CPU paused |
| 3 | Deep Sleep Timer | Timer wakeup, full reset |
| 4 | Deep Sleep GPIO | ext0/ext1 wakeup |
| 5 | Deep Sleep RTC | RTC memory persistence |
| 6 | Sleep Data Retention | Struct di RTC memory |
| 7 | Touch Wakeup | Kapasitif touch pad |
| 8 | ULP Coprocessor | Monitoring saat tidur |
| 9 | Dynamic Frequency | DFS & PM lock |
| 10 | Peripheral Power Gate | Clock & power gating |
| 11 | Battery Logger | Sistem battery-powered |
| 12 | Power Budget | Analisis & estimasi |

---

### Slide 3: Overview Percobaan STM32
**Judul**: Daftar Percobaan STM32
**Konten**:
| No | Percobaan | Topik Utama |
|----|-----------|-------------|
| 1 | Current Measurement | Pengukuran arus berbagai mode |
| 2 | Light Sleep Basic | Sleep mode (WFI) |
| 3 | Deep Sleep Timer | Stop mode + RTC alarm |
| 4 | Deep Sleep GPIO | Stop mode + EXTI |
| 5 | Deep Sleep RTC | Standby mode + RTC |
| 6 | Sleep Data Retention | Backup register |
| 7 | Touch Wakeup | Multi-pin EXTI wakeup |
| 8 | ULP Coprocessor | Peripheral clock gating |
| 9 | Dynamic Frequency | Clock speed switching |
| 10 | Peripheral Power Gate | Matikan modul tak terpakai |
| 11 | Battery Logger | Sistem battery-powered |
| 12 | Power Budget | Analisis & estimasi |

---

### Slide 4: Tools yang Dibutuhkan
**Judul**: Persiapan Praktikum
**Konten**:
- **Hardware**: ESP32 DevKit V1, STM32 Blue Pill F103C8, ST-Link V2
- **Pengukuran**: Multimeter, resistor shunt 1Ω, (opsional) INA219 module
- **Software**: VS Code + PlatformIO, Python 3 + pip
- **Python packages**: `pip install pyserial matplotlib numpy`
- **Kabel**: USB micro-B, jumper wire, push button, LED

**Diagram**: Foto setup lab dengan label komponen

---

### Slide 5: Demo — Cara Mengukur Arus
**Judul**: Teknik Pengukuran Arus
**Konten**:
1. **Multimeter Serial**: Potong jalur VCC, sisipkan multimeter
2. **Shunt Resistor**: R = 1Ω, ukur V_shunt, I = V/R
3. **ADC Internal**: Baca tegangan shunt via ADC
4. **INA219**: Sensor I2C, resolusi tinggi

**Diagram**: Skematik rangkaian pengukuran arus dengan shunt resistor
```
USB 5V → [R_shunt=1Ω] → ESP32 VIN
              │
              └→ Multimeter (mV mode) → I = V/1Ω
```

**Speaker Notes**: Demonstrasikan pengukuran menggunakan multimeter. Tunjukkan perbedaan arus pada mode Active vs Deep Sleep.

---

### Slide 6: Demo — ESP32 Light Sleep
**Judul**: Percobaan 2: Light Sleep ESP32
**Konten**:
- Build: `pio run -t upload`
- Amati serial monitor: timestamp sebelum sleep dan setelah wakeup
- Variabel tetap bernilai (RAM preserved)
- Wakeup cause: timer/GPIO/touch
- Latency wakeup: <1ms

**Screenshot**: Contoh output serial monitor:
```
[Active] Time: 1000ms, counter=5
[Entering Light Sleep for 3s...]
[Wakeup] Cause: TIMER, Time: 4001ms, counter=5
```

---

### Slide 7: Demo — ESP32 Deep Sleep
**Judul**: Percobaan 3-5: Deep Sleep ESP32
**Konten**:
- Deep Sleep = FULL RESET setelah wakeup
- RTC_DATA_ATTR variabel bertahan
- Boot count increment setiap wakeup
- Wakeup sources: timer, ext0, ext1, touch

**Screenshot**: Contoh output serial:
```
Boot count: 1, Wakeup cause: UNDEFINED (power-on)
Boot count: 2, Wakeup cause: TIMER
Boot count: 3, Wakeup cause: EXT0
```

**Hands-on**: Mahasiswa tekan tombol saat ESP32 deep sleep → amati wakeup cause

---

### Slide 8: Demo — ESP32 RTC Memory
**Judul**: Percobaan 5-6: Data Retention
**Konten**:
- RTC SLOW memory: 8KB, tersedia saat deep sleep
- Simpan data sensor, counter, config di RTC memory
- Cold boot vs warm boot detection
- `RTC_DATA_ATTR` vs `RTC_NOINIT_ATTR`

**Demo**: Cabut power → data RTC hilang → bandingkan dengan wakeup dari deep sleep

---

### Slide 9: Demo — ESP32 Touch Pad Wakeup
**Judul**: Percobaan 7: Touch Pad Wakeup
**Konten**:
- Touch pad = kapasitif sensor pada GPIO ESP32
- 10 touch pad tersedia (T0-T9)
- Threshold menentukan sensitivitas
- Bisa wakeup dari deep sleep

**Hands-on**: Sentuh pin GPIO4 (T0) → ESP32 bangun dari deep sleep

---

### Slide 10: Demo — ESP32 DFS
**Judul**: Percobaan 9: Dynamic Frequency Scaling
**Konten**:
- Konfigurasi: max_freq = 240MHz, min_freq = 40MHz
- CPU otomatis turun frekuensi saat idle
- PM Lock untuk prevent DFS saat critical
- Ukur arus pada setiap frekuensi

**Screenshot**: Serial output menunjukkan transisi frekuensi

---

### Slide 11: Demo — STM32 Sleep/Stop/Standby
**Judul**: Percobaan 13-17: STM32 Low-Power Modes
**Konten**:
- **Sleep (WFI)**: CPU off, peripheral jalan → ~10mA
- **Stop (EXTI)**: Semua clock off, RAM OK → ~20μA
- **Standby (WKUP/RTC)**: Semua off, RAM hilang → ~2μA

**Demo Steps**:
1. Upload program
2. Ukur arus mode Active dengan multimeter
3. Tekan button → masuk sleep → ukur arus sleep
4. Tekan button → wakeup → amati log

**Penting**: Setelah Stop mode, harus SystemClock_Config() lagi!

---

### Slide 12: Demo — STM32 Backup Register
**Judul**: Percobaan 18: Data Retention STM32
**Konten**:
- Backup Register: 10 × 16-bit = 20 bytes
- Bertahan di Standby (jika VBAT tersambung)
- Enable: PWR_CLK + BKP_CLK + HAL_PWR_EnableBkUpAccess()
- Write: HAL_RTCEx_BKUPWrite(&hrtc, DR1, value)
- Read: HAL_RTCEx_BKUPRead(&hrtc, DR1)

**Perbandingan**:
| | ESP32 RTC | STM32 BKP |
|---|-----------|-----------|
| Ukuran | 16KB | 20 bytes |
| Tipe | Apapun | uint16_t |
| Syarat | Deep Sleep | VBAT |

---

### Slide 13: Demo — Dynamic Frequency STM32
**Judul**: Percobaan 21: Clock Speed Switching
**Konten**:
- 72MHz (HSE+PLL) → 36MHz → 8MHz (HSI)
- UART baud rate berubah saat ganti clock!
- Harus reconfigure UART setelah ganti frekuensi
- Mengukur arus pada setiap frekuensi

---

### Slide 14: Demo — Battery-Powered Logger
**Judul**: Percobaan 11 & 23: Battery-Powered System
**Konten**:
- Siklus: Wakeup → Read Sensor → Log Data → Check Battery → Sleep
- Battery monitoring via voltage divider + ADC
- Adaptive duty cycle: baterai rendah → sleep lebih lama
- Low battery warning: LED blink atau serial alert

**Diagram**: Flowchart siklus operasi battery-powered logger

---

### Slide 15: Demo — Power Budget Analysis
**Judul**: Percobaan 12 & 24: Power Budget
**Konten**:
- Identifikasi semua mode operasi dan durasinya
- Isi tabel power budget:
  | Mode | I (mA) | Durasi (s) | D (%) | I×D (mA) |
  |------|--------|------------|-------|----------|
  | Sleep | 0.01 | 290 | 96.7% | 0.0097 |
  | Sensor | 25 | 2 | 0.67% | 0.167 |
  | TX | 180 | 5 | 1.67% | 3.0 |
  | Process | 40 | 3 | 1.0% | 0.4 |
- I_avg = 3.577mA → Baterai 3000mAh → 35 hari

---

### Slide 16: Python Debug & Analysis Tools
**Judul**: debug_analysis.py — Visualisasi Data
**Konten**:
- Setiap percobaan memiliki `debug_analysis.py`
- Membaca data serial secara real-time
- Visualisasi menggunakan matplotlib:
  - Grafik arus vs waktu
  - Pie chart distribusi energi
  - Timeline mode operasi
  - Power budget comparison bar chart

**Screenshot**: Contoh grafik matplotlib dari debug script

---

### Slide 17: Cara Menjalankan Python Script
**Judul**: Langkah Menggunakan debug_analysis.py
**Konten**:
```bash
# 1. Install dependencies
pip install pyserial matplotlib numpy

# 2. Pastikan serial port tersambung
ls /dev/ttyUSB*    # Linux
# atau cek di Device Manager (Windows)

# 3. Jalankan script
cd praktikum/ESP32/ESP32_01_Current_Measurement
python3 debug_analysis.py

# 4. Script akan otomatis:
#    - Connect ke serial port
#    - Parse data dari MCU
#    - Generate grafik matplotlib
```

---

### Slide 18: Tips Pengukuran Arus
**Judul**: Best Practices Pengukuran
**Konten**:
1. Lepas LED power dan regulator onboard jika memungkinkan
2. Ukur arus BOARD saja, bukan USB cable
3. Gunakan shunt resistor kecil (0.1-1Ω) agar tidak mengganggu tegangan
4. Untuk sleep mode, gunakan range μA pada multimeter
5. Tunggu arus stabil sebelum catat (>5 detik)
6. Rata-ratakan beberapa pengukuran
7. Catat kondisi lingkungan (suhu, periferal terpasang)

---

### Slide 19: Troubleshooting Power Measurement
**Judul**: Masalah Umum & Solusi
**Konten**:
| Masalah | Penyebab | Solusi |
|---------|----------|--------|
| Arus sleep terlalu tinggi | LED/sensor masih nyala | Cabut komponen eksternal |
| Tidak bisa deep sleep | GPIO floating | Tambah pull-up/pull-down |
| Wakeup tidak trigger | Pin salah | Cek datasheet RTC GPIO |
| UART hilang setelah Stop | Clock reset ke HSI | Reconfigure SystemClock |
| Battery reading tidak akurat | ADC noise | Tambah capacitor, averaging |

---

### Slide 20: Perbandingan Hasil ESP32 vs STM32
**Judul**: Hasil Perbandingan
**Konten**:
- Buat tabel perbandingan untuk laporan:
  | Parameter | ESP32 | STM32 |
  |-----------|-------|-------|
  | Active (mA) | ? | ? |
  | Sleep (μA) | ? | ? |
  | Wakeup Time (ms) | ? | ? |
  | Data Retention | ? | ? |
  | Battery Life (hari) | ? | ? |

**Tugas**: Isi tabel berdasarkan pengukuran real di lab

---

### Slide 21: Deliverables Praktikum
**Judul**: Yang Harus Dikumpulkan
**Konten**:
1. **Laporan** (PDF): Tabel pengamatan, grafik, analisis
2. **Screenshot**: Serial monitor & matplotlib output
3. **Video** (5-8 menit): Demo minimal 3 percobaan
4. **Kode**: Push ke GitHub (opsional)
5. **Power Budget**: Spreadsheet kalkulasi lengkap

**Deadline**: [tanggal sesuai kalender akademik]

---

### Slide 22: Penutup
**Judul**: Rangkuman Praktikum
**Konten**:
- Power management = kunci perangkat IoT battery-powered
- ESP32: built-in WiFi + DFS + ULP → IoT device
- STM32: ultra-low standby (2μA) → sensor node sederhana
- Selalu ukur arus real, hitung power budget
- Optimasi terbesar: kurangi waktu WiFi TX

**Pertanyaan refleksi**: Jika harus membuat sensor cuaca outdoor yang bertahan 1 tahun, platform mana yang Anda pilih dan mengapa?

---

*PPT Prompts Modul 14 — Bagian Praktikum*
