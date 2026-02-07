# Materi Modul 14: Power Management & Low-Power Design

## 1. Pengantar Power Management pada Embedded Systems
Dalam sistem embedded, terutama yang berjalan dengan baterai (wearable, IoT sensor node, remote logger), **konsumsi daya** menjadi faktor kritis. Sebuah ESP32 yang selalu aktif dengan WiFi bisa menghabiskan ~240mA, sementara dalam mode deep sleep hanya ~10µA — perbedaan **24.000x lipat**! Memahami teknik low-power design memungkinkan perangkat bertahan berminggu-minggu bahkan bertahun-tahun dengan satu baterai coin cell.

### Prinsip Dasar Konsumsi Daya
Daya yang dikonsumsi mikrokontroler ditentukan oleh:
$$P = C \times V^2 \times f + I_{leak} \times V$$
Dimana:
- $C$ = Kapasitansi switching (proporsional dengan jumlah gate aktif)
- $V$ = Tegangan supply
- $f$ = Frekuensi clock
- $I_{leak}$ = Arus bocor (leakage current)

Dari rumus ini, ada 3 strategi utama:
1. **Turunkan frekuensi** ($f$) → Clock gating, dynamic frequency scaling
2. **Turunkan tegangan** ($V$) → Dynamic voltage scaling
3. **Matikan modul tidak terpakai** → Peripheral clock gating, sleep modes

---

## 2. Low-Power Modes

### STM32 Low-Power Modes
STM32 menyediakan beberapa level low-power yang progressif:

| Mode | Konsumsi | CPU | SRAM | Peripheral | Wake-up Time |
|------|----------|-----|------|------------|--------------|
| **Run** | ~30-50 mA | Aktif | Aktif | Aktif | - |
| **Sleep** | ~10-15 mA | Stop | Aktif | Aktif | ~1 µs |
| **Stop** | ~2-20 µA | Stop | Aktif (retained) | Stop | ~5 µs |
| **Standby** | ~2-3 µA | Stop | Lost | Stop | ~50 µs (reset) |

- **Sleep Mode**: CPU berhenti, peripheral tetap jalan. Dibangunkan oleh interrupt apapun.
- **Stop Mode**: Semua clock berhenti, regulator low-power. SRAM & register dipertahankan. Dibangunkan oleh EXTI, RTC alarm.
- **Standby Mode**: Daya terhemat, tapi SRAM hilang (seperti reset). Hanya wake-up pin, RTC alarm, atau IWDG reset yang bisa membangunkan.

### ESP32 Low-Power Modes

| Mode | Konsumsi | CPU | WiFi/BT | RTC | Wake-up Time |
|------|----------|-----|---------|-----|--------------|
| **Active** | ~160-240 mA | Aktif | Aktif | Aktif | - |
| **Modem Sleep** | ~20 mA | Aktif | Off | Aktif | Instant |
| **Light Sleep** | ~0.8 mA | Paused | Off | Aktif | ~1 ms |
| **Deep Sleep** | ~10 µA | Off | Off | Aktif | ~200 ms |
| **Hibernation** | ~5 µA | Off | Off | Minimal | ~200 ms |

- **Modem Sleep**: WiFi/BT radio dimatikan, CPU masih aktif. Cocok untuk processing tanpa komunikasi.
- **Light Sleep**: CPU di-pause, RTC dan ULP masih jalan. SRAM dipertahankan.
- **Deep Sleep**: Hanya RTC controller, RTC memory (8KB), dan ULP co-processor yang aktif. Main CPU dan sebagian besar RAM dimatikan.
- **Hibernation**: RTC timer saja yang aktif, RTC memory juga dimatikan. Konsumsi terendah (~5µA).

---

## 3. Wake-up Sources (Sumber Pembangun)

### STM32 Wake-up Sources
1. **EXTI (External Interrupt)**: Pin tertentu (biasanya WKUP pin) dapat membangunkan dari Standby.
2. **RTC Alarm**: Timer RTC bisa dijadwalkan untuk membangunkan MCU pada waktu tertentu.
3. **RTC Wakeup Timer**: Periodic wake-up menggunakan sub-second timer RTC.
4. **IWDG (Independent Watchdog)**: Reset dari watchdog bisa membangunkan dari Standby.
5. **NRST Pin**: Reset eksternal.

### ESP32 Wake-up Sources
1. **Timer**: RTC timer dengan resolusi microsecond — paling umum digunakan.
2. **Touch Pad**: Sensor kapasitif bisa membangunkan dari deep sleep.
3. **External Wake-up (ext0)**: Satu GPIO RTC tertentu, level HIGH/LOW.
4. **External Wake-up (ext1)**: Beberapa GPIO RTC, mendukung logic ANY/ALL.
5. **ULP Co-processor**: Program di ULP bisa membangunkan main CPU berdasarkan kondisi.
6. **GPIO**: Light sleep bisa dibangunkan oleh perubahan GPIO.

---

## 4. Clock Gating & Dynamic Frequency Scaling

### Clock Gating
Konsep: matikan clock peripheral yang tidak digunakan. Tanpa clock, transistor tidak switching, sehingga **tidak ada dynamic power consumption**.

**STM32:**
```c
// Aktifkan clock hanya saat dibutuhkan
__HAL_RCC_GPIOA_CLK_ENABLE();
// ... gunakan GPIOA ...
__HAL_RCC_GPIOA_CLK_DISABLE();  // Matikan saat tidak dipakai
```

**ESP32:**
```c
// Disable WiFi untuk hemat daya
esp_wifi_stop();
// Disable Bluetooth
esp_bt_controller_disable();
```

### Dynamic Frequency Scaling (DFS)
Menurunkan frekuensi CPU saat beban rendah, menaikkan saat butuh performa tinggi.

**ESP32** mendukung DFS secara native dengan `esp_pm_configure()`:
- CPU bisa turun ke 80MHz, 40MHz, bahkan 10MHz.
- Frekuensi otomatis naik saat ada interrupt atau task aktif.

**STM32** bisa menurunkan clock melalui PLL reconfiguration atau prescaler:
- Dari 72MHz ke 8MHz (HSI) saat idle.
- Peripheral clock bisa di-prescale independen.

---

## 5. ULP Co-Processor (ESP32)
ESP32 memiliki **Ultra-Low Power co-processor** — sebuah prosesor kecil yang bisa berjalan saat main CPU dalam deep sleep.

### Kemampuan ULP:
- Baca sensor via ADC
- Baca GPIO
- Kontrol GPIO
- Operasi I2C (pada beberapa varian)
- Akses RTC memory (8KB shared)

### Use Case:
1. **Periodic sensor reading**: ULP baca suhu setiap 10 detik. Jika di atas threshold, bangunkan main CPU untuk kirim alarm via WiFi.
2. **Battery monitoring**: ULP monitor tegangan baterai via ADC, bangunkan CPU jika low battery.
3. **Motion detection**: ULP pantau accelerometer via I2C, bangunkan CPU saat gerakan terdeteksi.

### Konsumsi ULP:
- ULP aktif: ~150 µA (jauh lebih hemat dari main CPU ~50mA)
- ULP + deep sleep: ~10-150 µA tergantung aktivitas

---

## 6. RTC Memory & Data Persistence

### ESP32 RTC Memory
ESP32 memiliki **8KB RTC SLOW memory** yang tetap aktif selama deep sleep. Ini memungkinkan:
- Menyimpan counter/variabel antar deep sleep cycle
- Menyimpan state sensor terakhir
- Menyimpan boot count untuk diagnostik

```c
RTC_DATA_ATTR int bootCount = 0;  // Tetap tersimpan di deep sleep
```

### STM32 Backup Domain
STM32 memiliki **Backup registers** (20 x 16-bit pada F1, lebih banyak pada F4/H7) dan **VBAT domain**:
- Backup registers tetap tersimpan selama VBAT ada tegangan
- RTC tetap berjalan dengan baterai coin cell di VBAT pin
- Data hilang hanya jika VBAT dan VDD sama-sama hilang

---

## 7. Battery Monitoring & Optimization

### Teknik Monitoring Baterai:
1. **Voltage Divider + ADC**: Bagi tegangan baterai dengan resistor divider, baca via ADC.
2. **Internal VREFINT (STM32)**: Bandingkan Vref internal untuk kalibrasi pengukuran.
3. **ESP32 ADC**: Gunakan ADC dengan attenuation yang tepat (max 3.3V input).

### Strategi Optimasi Baterai:
1. **Duty Cycling**: Aktif sebentar, tidur lama (contoh: 10 detik aktif, 5 menit tidur).
2. **Adaptive Sampling**: Tingkatkan frekuensi sampling hanya saat ada event penting.
3. **Data Batching**: Kumpulkan beberapa pembacaan sensor, kirim sekaligus via WiFi.
4. **WiFi Power Save**: Gunakan DTIM interval tinggi, modem sleep antar transmisi.
5. **Peripheral Power Control**: Matikan sensor/modul eksternal via MOSFET switch saat tidak dipakai.

### Estimasi Umur Baterai:
$$T_{battery} = \frac{C_{battery}}{I_{avg}}$$

Dimana $I_{avg}$ untuk duty cycling:
$$I_{avg} = \frac{I_{active} \times t_{active} + I_{sleep} \times t_{sleep}}{t_{active} + t_{sleep}}$$

**Contoh:**
- Baterai 2000mAh, active 100mA selama 5 detik, sleep 10µA selama 295 detik
- $I_{avg} = \frac{100 \times 5 + 0.01 \times 295}{300} = 1.68\text{ mA}$
- $T = \frac{2000}{1.68} \approx 1190\text{ jam} \approx 50\text{ hari}$

---

## 8. Hardware Implementation

### ESP32 (Built-in Power Management)
ESP32 memiliki power management unit (PMU) internal yang mengelola semua domain daya. Konfigurasi cukup melalui software API (`esp_sleep`, `esp_pm`).

### STM32 (HAL Power API)
STM32 menggunakan HAL Power API:
- `HAL_PWR_EnterSLEEPMode()` — Sleep mode
- `HAL_PWR_EnterSTOPMode()` — Stop mode  
- `HAL_PWR_EnterSTANDBYMode()` — Standby mode
- `HAL_PWREx_EnableLowPowerRunMode()` — Low-power run (pada L-series)

### Pertimbangan Hardware:
- **Decoupling capacitor** pada VBAT untuk backup domain STM32
- **Voltage regulator** efisiensi tinggi (LDO vs switching regulator)
- **Power MOSFET** untuk mematikan peripheral eksternal
- **Pull-up/pull-down** pada pin floating untuk menghindari arus bocor
- **LED** current limiter — LED status bisa menghabiskan 10-20mA!
