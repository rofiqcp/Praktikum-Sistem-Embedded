# Prompt untuk Slide Presentasi Teori (Minggu 1)
## Topik: Power Management Fundamentals & Sleep Modes

Buatkan outline slide presentasi yang mendalam tentang dasar manajemen daya pada sistem embedded.

### Slide 1: Judul
- **Judul**: Power Management & Low-Power Design
- **Subjudul**: Merancang Sistem Embedded Hemat Daya untuk Aplikasi IoT & Battery-Powered
- **Visual**: Ilustrasi mikrokontroler dengan ikon baterai, panel surya, dan simbol tidur (💤).

### Slide 2: Mengapa Power Management Penting?
- **Poin Utama**:
  - Perangkat IoT harus beroperasi berbulan-bulan/bertahun-tahun dengan baterai.
  - Baterai coin cell (CR2032) hanya 220mAh — ESP32 aktif menghabiskannya dalam ~1 jam!
  - Panel surya kecil hanya menghasilkan ~100mW — harus dikelola efisien.
  - Regulasi: Energy harvesting, green computing.
- **Visual**: Grafik perbandingan umur baterai: tanpa optimasi vs dengan optimasi.

### Slide 3: Prinsip Konsumsi Daya Digital
- **Poin Utama**:
  - Rumus: P = C × V² × f + I_leak × V
  - **Dynamic Power**: Proporsional dengan frekuensi dan jumlah gate switching.
  - **Static Power (Leakage)**: Selalu ada, bahkan saat idle.
  - 3 Strategi: Turunkan frekuensi, turunkan tegangan, matikan modul.
- **Visual**: Diagram pie chart sumber konsumsi daya di MCU.

### Slide 4: STM32 Low-Power Modes (STM32Cube HAL)
- **Poin Utama**:
  - **Sleep Mode**: `HAL_PWR_EnterSLEEPMode()` — CPU stop, peripheral jalan, ~10mA, bangun cepat (~1µs).
  - **Stop Mode**: `HAL_PWR_EnterSTOPMode()` — semua clock stop, SRAM retained, ~20µA. **Perlu reconfigure clock setelah wake-up**.
  - **Standby Mode**: `HAL_PWR_EnterSTANDBYMode()` — ultra-low ~2µA, SRAM hilang, seperti reset. Data simpan di Backup Register.
- **Visual**: Tabel perbandingan 3 mode dengan indikator warna (hijau/kuning/merah).

### Slide 5: ESP32 Low-Power Modes (ESP-IDF)
- **Poin Utama**:
  - **Active**: ~240mA (WiFi TX), full power.
  - **Modem Sleep**: ~20mA, `esp_wifi_stop()`.
  - **Light Sleep**: ~0.8mA, `esp_light_sleep_start()`, SRAM retained.
  - **Deep Sleep**: ~10µA, `esp_deep_sleep_start()`, hanya RTC + ULP aktif.
  - **Hibernation**: ~5µA, `esp_sleep_pd_config()`.
- **Visual**: Diagram level bertingkat (staircase) dari Active ke Hibernation.

### Slide 6: Wake-up Sources
- **Poin Utama**:
  - **Timer/RTC**: `esp_sleep_enable_timer_wakeup()` / `HAL_RTC_SetAlarm_IT()`
  - **External GPIO**: `esp_sleep_enable_ext0_wakeup()` / `HAL_PWR_EnableWakeUpPin()`
  - **Touch Pad (ESP32)**: `esp_sleep_enable_touchpad_wakeup()`
  - **ULP Co-processor (ESP32)**: `esp_sleep_enable_ulp_wakeup()`
  - **Watchdog**: Safety wake-up.
- **Visual**: Diagram MCU sleeping dengan panah-panah wake-up source.

### Slide 7: Clock Gating & Dynamic Frequency Scaling
- **Poin Utama**:
  - Clock Gating: `__HAL_RCC_XXX_CLK_DISABLE()` / `periph_module_disable()`
  - DFS ESP-IDF: `esp_pm_configure()` — CPU 240MHz ↔ 10MHz otomatis.
  - DFS STM32: Switch HSI 8MHz ↔ PLL 72MHz manual.
  - Trade-off: Latency vs Power Saving.
- **Visual**: Grafik timeline: frekuensi CPU vs waktu.

### Slide 8: Studi Kasus & Kesimpulan
- **Poin Utama**:
  - Contoh: Sensor cuaca IoT, baterai 2000mAh.
  - Tanpa optimasi: 8 jam. Dengan deep sleep duty cycling: 50+ hari.
  - Kunci: Pilih mode sleep tepat, minimalkan waktu aktif, matikan yang tidak perlu.
  - Minggu depan: ULP, Battery Monitoring, Solar Weather Station.
- **Visual**: Before/After battery life comparison chart.
