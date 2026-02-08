# PPT Prompts — Modul 14: Power Management (Bagian Teori)

## Panduan Pembuatan Slide Presentasi

---

### Slide 1: Halaman Judul
**Judul**: Modul 14 — Power Management pada Sistem Embedded
**Konten**:
- Nama mata kuliah: Praktikum Sistem Embedded
- Platform: ESP32 (ESP-IDF) & STM32F103 (STM32Cube HAL)
- Jumlah percobaan: 24 (12 ESP32 + 12 STM32)
- Gambar: Ikon baterai, mikrokontroler, grafik konsumsi daya

**Speaker Notes**: Sampaikan bahwa modul ini membahas teknik-teknik hemat daya yang krusial untuk perangkat IoT dan embedded system battery-powered.

---

### Slide 2: Mengapa Power Management Penting?
**Judul**: Pentingnya Power Management
**Konten**:
- Battery Life: Perangkat IoT harus bertahan berbulan-bulan/bertahun-tahun
- Thermal: Daya berlebih → panas → kerusakan komponen
- Cost: Daya rendah = baterai kecil = biaya rendah
- Environmental: Efisiensi energi mengurangi dampak lingkungan
- Reliability: Sistem hemat daya lebih stabil

**Diagram**: Grafik pie chart kontribusi daya (CPU, peripheral, memory, IO, leakage)

**Speaker Notes**: Jelaskan skenario nyata seperti sensor cuaca di gunung yang harus bertahan 1 tahun tanpa ganti baterai.

---

### Slide 3: Komponen Konsumsi Daya
**Judul**: Dari Mana Daya Dikonsumsi?
**Konten**:
- Rumus: P_total = P_CPU + P_peripheral + P_memory + P_IO + P_leakage
- Daya Dinamis: P = C × V² × f (switching transistor)
- Daya Statis: P = V × I_leak (arus bocor, selalu ada)
- CPU: kontributor terbesar saat aktif
- WiFi/BT: kontributor terbesar saat transmisi (ESP32: 180mA)

**Diagram**: Tabel perbandingan daya dinamis vs statis

---

### Slide 4: Strategi Pengurangan Daya
**Judul**: 6 Strategi Hemat Daya
**Konten**:
1. Clock Gating — matikan clock peripheral tidak terpakai
2. Power Gating — matikan tegangan ke blok tidak aktif
3. Voltage Scaling — turunkan tegangan operasi
4. Frequency Scaling — turunkan frekuensi clock
5. Sleep Modes — matikan sebagian/seluruh sistem saat idle
6. Duty Cycling — aktif sesaat, tidur sebagian besar waktu

**Diagram**: Flowchart strategi dari yang paling ringan ke paling agresif

---

### Slide 5: Konsep Duty Cycle
**Judul**: Duty Cycle & Battery Life
**Konten**:
- Duty Cycle (D) = waktu aktif / total waktu
- I_avg = I_active × D + I_sleep × (1-D)
- T_battery = C_battery / I_avg
- Contoh: 2000mAh, D=1%, I_active=80mA, I_sleep=10μA → 103 hari

**Diagram**: Grafik waveform duty cycle (waktu aktif vs sleep)

**Speaker Notes**: Tunjukkan bahwa menurunkan D dari 10% ke 1% meningkatkan battery life 10×.

---

### Slide 6: Hirarki Mode Daya
**Judul**: Tingkatan Mode Low-Power
**Konten**:
```
Active → Light Sleep → Deep Sleep → Hibernate → Power Off
  ↓         ↓             ↓            ↓
Hemat↑   Wakeup↑       Data↓       Semua↓
```
- Trade-off: Hemat daya vs waktu wakeup vs data retention
- Setiap level semakin hemat tapi semakin banyak yang hilang

**Diagram**: Piramida terbalik dengan level daya

---

### Slide 7: ESP32 — Mode Daya
**Judul**: ESP32 Power Modes (5 Level)
**Konten**:
| Mode | Konsumsi | Deskripsi |
|------|----------|-----------|
| Active | 80-260mA | CPU + WiFi/BT aktif |
| Modem Sleep | 20-30mA | WiFi/BT off, CPU aktif |
| Light Sleep | 0.8mA | CPU paused, RAM retained |
| Deep Sleep | 10-150μA | CPU off, RTC aktif |
| Hibernation | 2.5-5μA | Hampir semua off |

**Diagram**: State diagram transisi antar mode

---

### Slide 8: ESP32 — Light Sleep
**Judul**: ESP32 Light Sleep Mode
**Konten**:
- CPU clock dihentikan, RAM dan state dipertahankan
- Wakeup cepat (μs), eksekusi dilanjutkan dari titik sleep
- Wakeup: timer, GPIO, UART, touch pad
- API: `esp_light_sleep_start()`
- Konsumsi: ~0.8mA

**Code Snippet**:
```c
esp_sleep_enable_timer_wakeup(5000000);
esp_light_sleep_start();
// lanjut dari sini setelah wakeup
```

---

### Slide 9: ESP32 — Deep Sleep
**Judul**: ESP32 Deep Sleep Mode
**Konten**:
- CPU dan RAM dimatikan, RTC controller tetap aktif
- 8KB RTC SLOW memory + 8KB RTC FAST memory bertahan
- Setelah wakeup = FULL RESET (dimulai dari app_main)
- RTC_DATA_ATTR untuk variabel yang bertahan
- Wakeup: timer, ext0, ext1, touch, ULP

**Code Snippet**:
```c
RTC_DATA_ATTR int boot_count = 0;
void app_main(void) {
    boot_count++;
    esp_sleep_enable_timer_wakeup(10000000);
    esp_deep_sleep_start();
}
```

---

### Slide 10: ESP32 — Hibernation
**Judul**: ESP32 Hibernation Mode
**Konten**:
- Mode paling hemat: 2.5-5μA
- RTC memory TIDAK dipertahankan
- Hanya RTC timer dan ext0/ext1 bisa wakeup
- Semua power domain dimatikan secara eksplisit
- Cocok untuk wakeup jarang (jam/hari)

---

### Slide 11: STM32F103 — Mode Daya
**Judul**: STM32F103 Power Modes (3 Level)
**Konten**:
| Mode | Konsumsi | Regulator | Clock | RAM |
|------|----------|-----------|-------|-----|
| Sleep | 10-15mA | ON | CPU off | OK |
| Stop | 20μA | Low-power | HSI/HSE off | OK |
| Standby | 2μA | OFF | Semua off | Hilang |

**Diagram**: Block diagram showing what's on/off in each mode

---

### Slide 12: STM32 — Sleep Mode (WFI/WFE)
**Judul**: STM32 Sleep Mode
**Konten**:
- Hanya CPU clock dimatikan
- Semua peripheral tetap berjalan
- Wakeup dari interrupt apapun (WFI) atau event (WFE)
- Wakeup tercepat (1-2 clock cycles)
- `HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI)`

---

### Slide 13: STM32 — Stop Mode
**Judul**: STM32 Stop Mode
**Konten**:
- Semua clock dimatikan, regulator low-power
- RAM dan register DIPERTAHANKAN
- Wakeup hanya dari EXTI (GPIO interrupt)
- PENTING: Setelah wakeup, clock kembali ke HSI 8MHz!
- Harus rekonfigurasi SystemClock_Config() setelah wakeup

---

### Slide 14: STM32 — Standby Mode
**Judul**: STM32 Standby Mode
**Konten**:
- Konsumsi terendah: ~2μA
- Regulator dimatikan → RAM hilang!
- Hanya backup register (20 bytes) yang bertahan
- Wakeup dari WKUP pin (PA0) atau RTC alarm
- Setelah wakeup = FULL RESET (seperti power-on)

---

### Slide 15: Perbandingan Wakeup Sources
**Judul**: Sumber Wakeup — ESP32 vs STM32
**Konten**:
| Source | ESP32 Light | ESP32 Deep | STM32 Sleep | STM32 Stop | STM32 Standby |
|--------|------------|------------|------------|------------|---------------|
| Timer | ✅ | ✅ | — | — | ✅ (RTC) |
| GPIO | ✅ | ✅ (RTC) | ✅ (NVIC) | ✅ (EXTI) | ✅ (PA0) |
| Touch | ✅ | ✅ | — | — | — |
| ULP | ✅ | ✅ | — | — | — |
| UART | ✅ | — | ✅ | — | — |

---

### Slide 16: Data Retention
**Judul**: Menyimpan Data Saat Sleep
**Konten**:
- ESP32: 8KB RTC memory (RTC_DATA_ATTR)
  - Cukup untuk array, struct, buffer sensor
  - Bertahan di Deep Sleep, hilang di Hibernation
- STM32: 20 bytes Backup Register (10 × 16-bit)
  - Sangat terbatas, hanya counter/flag
  - Bertahan di Standby (jika VBAT ada)

---

### Slide 17: Dynamic Frequency Scaling
**Judul**: DFS — Automatic Frequency Control
**Konten**:
- P = C × V² × f → turunkan f, turunkan P
- ESP32: DFS otomatis (min_freq ↔ max_freq)
- STM32: Manual clock switching (HSI 8MHz ↔ PLL 72MHz)
- PM Lock: cegah DFS saat critical section
- Trade-off: frekuensi rendah = eksekusi lambat

---

### Slide 18: Peripheral Clock & Power Gating
**Judul**: Matikan Yang Tidak Dipakai
**Konten**:
- Clock Gating: matikan clock → dynamic power = 0
- Power Gating: matikan tegangan → total power = 0
- ESP32: periph_module_disable(), esp_wifi_deinit()
- STM32: __HAL_RCC_GPIOx_CLK_DISABLE()
- Selalu disable peripheral sebelum sleep

---

### Slide 19: ULP Coprocessor (ESP32)
**Judul**: Ultra-Low-Power Coprocessor
**Konten**:
- Prosesor sederhana dalam ESP32 (~150μA)
- Beroperasi saat main CPU dalam deep sleep
- Bisa baca ADC, kontrol GPIO
- Bangunkan main CPU hanya jika diperlukan
- Sangat efisien untuk monitoring threshold-based

**Diagram**: Ilustrasi ULP vs main CPU power consumption

---

### Slide 20: Battery-Powered Design
**Judul**: Merancang Sistem Battery-Powered
**Konten**:
- Pilih baterai: Li-Ion 3.7V, LiPo, coin cell
- Regulator: LDO (simple) vs Buck (efisien)
- Monitoring: voltage divider + ADC
- Charging: TP4056 module
- Software: deep sleep + adaptive duty cycle + batch data

**Diagram**: Block diagram: Battery → Charger → Regulator → MCU

---

### Slide 21: Power Budget Analysis
**Judul**: Menghitung Power Budget
**Konten**:
- Identifikasi semua mode operasi
- Ukur/estimasi arus per mode
- Tentukan durasi per mode (duty cycle)
- Hitung I_avg = Σ(I_i × D_i)
- T_battery = C_battery / I_avg
- Tambahkan margin 20-30%

**Tabel**: Contoh power budget dengan 4 mode

---

### Slide 22: Pengukuran Arus
**Judul**: Cara Mengukur Konsumsi Arus
**Konten**:
- Multimeter: sederhana, range mA
- Shunt Resistor + ADC: I = V_shunt / R_shunt
- INA219: sensor I2C, range μA-A
- Power Profiler: Nordic PPK2, resolusi nA
- Oscilloscope: untuk melihat transient current

---

### Slide 23: Perbandingan ESP32 vs STM32
**Judul**: ESP32 vs STM32 — Power Management
**Konten**:
| Fitur | ESP32 | STM32F103 |
|-------|-------|-----------|
| Deep Sleep | 10μA | 2μA |
| RTC Memory | 16KB | 20 bytes |
| ULP | ✅ | ❌ |
| DFS | Otomatis | Manual |
| Wakeup Sources | 5 jenis | 3 jenis |
| WiFi Power Save | Built-in | External |

---

### Slide 24: Best Practices
**Judul**: Tips Power Management
**Konten**:
1. Matikan peripheral sebelum sleep
2. Set GPIO ke analog/output-low saat sleep
3. Gunakan RTC memory untuk state persistence
4. Batch data sebelum transmisi wireless
5. Implementasikan watchdog untuk recovery
6. Selalu ukur arus real (jangan percaya datasheet 100%)
7. Tambahkan margin 20-30% pada power budget

---

### Slide 25: Penutup & Tugas
**Judul**: Rangkuman & Tugas
**Konten**:
- 24 percobaan (12 ESP32 + 12 STM32)
- Setiap percobaan ada debug_analysis.py
- Project: Smart Weather Station / Power Monitor / ULP Sensor Node
- Tugas video: demo sleep mode + power budget analysis
- Deadline: [tanggal]

---

*PPT Prompts Modul 14 — Bagian Teori*
