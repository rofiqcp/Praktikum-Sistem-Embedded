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

### Slide 4: STM32 Low-Power Modes
- **Poin Utama**:
  - **Sleep Mode**: CPU stop, peripheral jalan, ~10mA, bangun cepat (~1µs).
  - **Stop Mode**: Semua clock stop, SRAM retained, ~20µA, bangun ~5µs.
  - **Standby Mode**: Ultra-low ~2µA, SRAM hilang, seperti reset.
  - Perbandingan: apa yang dipertahankan vs apa yang hilang di setiap mode.
- **Visual**: Tabel perbandingan 3 mode dengan indikator warna (hijau/kuning/merah).

### Slide 5: ESP32 Low-Power Modes
- **Poin Utama**:
  - **Active**: ~240mA (WiFi TX), full power.
  - **Modem Sleep**: ~20mA, WiFi/BT off, CPU aktif.
  - **Light Sleep**: ~0.8mA, CPU paused, SRAM retained.
  - **Deep Sleep**: ~10µA, hanya RTC + ULP aktif.
  - **Hibernation**: ~5µA, minimum absolute.
- **Visual**: Diagram level bertingkat (staircase) dari Active ke Hibernation.

### Slide 6: Wake-up Sources
- **Poin Utama**:
  - **Timer/RTC**: Paling umum, periodik wake-up.
  - **External GPIO (EXTI)**: Button press, sensor interrupt.
  - **Touch Pad (ESP32)**: Wake-up kapasitif tanpa tombol fisik.
  - **ULP Co-processor (ESP32)**: Wake-up berdasarkan kondisi sensor.
  - **Watchdog**: Safety wake-up jika sistem hang.
- **Visual**: Diagram MCU sleeping dengan panah-panah wake-up source.

### Slide 7: Clock Gating & Dynamic Frequency Scaling
- **Poin Utama**:
  - Clock Gating: Matikan clock peripheral tidak terpakai.
  - DFS: Turunkan frekuensi saat idle, naikkan saat perlu performa.
  - Contoh: ESP32 bisa 240MHz → 10MHz (hemat ~10x daya dinamis).
  - Trade-off: Latency vs Power Saving.
- **Visual**: Grafik timeline: frekuensi CPU vs waktu dengan label "processing" dan "idle".

### Slide 8: Studi Kasus & Kesimpulan
- **Poin Utama**:
  - Contoh nyata: Sensor cuaca IoT dengan baterai 2000mAh.
  - Tanpa optimasi: 8 jam. Dengan deep sleep duty cycling: 50+ hari.
  - Kunci: Pilih mode sleep tepat, minimalkan waktu aktif, matikan yang tidak perlu.
  - Minggu depan: ULP, Battery Monitoring, dan Project Solar Weather Station.
- **Visual**: Before/After battery life comparison chart.
