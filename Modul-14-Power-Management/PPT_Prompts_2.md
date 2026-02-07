# Prompt untuk Slide Presentasi Teori (Minggu 2)
## Topik: Advanced Power Optimization, ULP, & Battery Management

Buatkan outline slide presentasi tentang teknik advanced power management dan desain solar-powered.

### Slide 1: Judul
- **Judul**: Advanced Power Optimization & Battery Management
- **Subjudul**: ULP Co-Processor, Battery Monitoring, dan Solar-Powered Design
- **Visual**: Diagram sistem solar → baterai → MCU → sensor → cloud.

### Slide 2: ULP Co-Processor (ESP32)
- **Poin Utama**:
  - Apa itu ULP? Prosesor kecil 8MHz yang berjalan saat main CPU tidur.
  - Kemampuan: Baca ADC, kontrol GPIO, operasi I2C.
  - Konsumsi: ~150µA saat aktif (vs main CPU ~50mA).
  - Use case: Periodic sensor check tanpa bangunkan main CPU.
- **Visual**: Diagram blok ESP32 internal, highlight RTC domain (ULP, RTC memory, RTC GPIO).

### Slide 3: RTC Memory & Data Persistence
- **Poin Utama**:
  - **ESP32 RTC Memory**: 8KB yang bertahan di deep sleep.
  - **STM32 Backup Registers**: 20x16-bit, bertahan selama VBAT ada.
  - Penggunaan: Boot counter, sensor readings buffer, state machine.
  - Keyword: `RTC_DATA_ATTR` (ESP32), Backup Domain (STM32).
- **Analogi**: RTC memory seperti "catatan kecil di saku" sebelum tidur — masih ada saat bangun.

### Slide 4: Battery Monitoring Techniques
- **Poin Utama**:
  - **Voltage Divider + ADC**: Metode paling umum dan murah.
  - **Lookup Table**: Konversi voltage → percentage berdasarkan kurva discharge Li-Ion.
  - **Coulomb Counting**: Metode akurat, hitung arus masuk/keluar.
  - **Kalibrasi**: ESP32 eFuse calibration, STM32 VREFINT.
- **Visual**: Grafik kurva discharge Li-Ion (voltage vs capacity %).

### Slide 5: Adaptive Duty Cycling
- **Poin Utama**:
  - Konsep: Sesuaikan frekuensi sampling dengan kondisi baterai.
  - Baterai tinggi → sampling sering. Baterai rendah → sampling jarang.
  - Formula: I_avg = (I_active × t_active + I_sleep × t_sleep) / (t_active + t_sleep)
  - Implementasi: State machine berdasarkan battery level.
- **Visual**: Diagram state machine: NORMAL → LOW_POWER → CRITICAL → EMERGENCY.

### Slide 6: Solar Energy Harvesting
- **Poin Utama**:
  - Panel surya mini: ~5V, 100-500mW.
  - Charging IC: TP4056 (Li-Ion charger with protection).
  - MPPT (Maximum Power Point Tracking) — untuk efisiensi optimal.
  - Desain: Panel → Charger → Baterai → LDO/Buck → MCU.
- **Visual**: Schematic sederhana rangkaian solar charging.

### Slide 7: Power Budget Calculation
- **Poin Utama**:
  - Langkah 1: Ukur konsumsi di setiap state (active, TX, sleep).
  - Langkah 2: Tentukan duty cycle (berapa lama di setiap state).
  - Langkah 3: Hitung I_avg dan estimasi battery life.
  - Langkah 4: Bandingkan dengan energy input (solar).
  - Tool: Spreadsheet kalkulator power budget.
- **Visual**: Tabel power budget dengan contoh angka.

### Slide 8: Best Practices & Kesimpulan
- **Poin Utama**:
  - ✅ Selalu ukur dulu, optimasi kemudian (jangan asumsi).
  - ✅ Matikan LED status saat produksi (10-20mA per LED!).
  - ✅ Gunakan pull-up/down pada pin floating (cegah arus bocor).
  - ✅ Pilih regulator efisien (LDO vs Buck converter).
  - ✅ Pertimbangkan suhu operasi (baterai kurang efisien di suhu ekstrem).
  - 🎯 Project: Solar-Powered Weather Station!
- **Visual**: Checklist best practices dengan ikon centang.
