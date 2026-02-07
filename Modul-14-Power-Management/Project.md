# Project Modul 14: Solar-Powered Weather Station

## 🎯 Deskripsi Proyek
Buatlah sistem Weather Station bertenaga surya (solar) yang beroperasi secara otonom dengan manajemen daya cerdas. Sistem harus mampu bertahan berbulan-bulan hanya dengan panel surya kecil dan baterai Li-Ion, sambil tetap mengirim data cuaca secara periodik.

## 📋 Spesifikasi Sistem

### 1. Hardware
- **ESP32** (ESP-IDF) atau **STM32** (STM32Cube HAL) sebagai mikrokontroler utama.
- Sensor: DHT22/BME280 (suhu & kelembapan), BH1750/LDR (cahaya), atau sensor dummy (random).
- Baterai Li-Ion 3.7V (atau simulasi via potensiometer pada ADC).
- Panel surya (opsional, bisa disimulasikan dengan LDR sebagai indikator cahaya).
- LED sebagai indikator status.

### 2. Manajemen Daya
- **Wajib** menggunakan minimal satu mode low-power (deep sleep/stop/standby).
- **Wajib** ada wake-up source (timer sebagai minimum).
- **Wajib** ada monitoring tegangan baterai (ADC atau simulasi).
- Sistem harus menerapkan **duty cycling** yang adaptif.

### 3. Fitur Utama
1. **Periodic Data Collection**:
   - Bangun dari deep sleep setiap N detik (konfigurabel).
   - Baca semua sensor.
   - Simpan data ke RTC memory (ESP32) / Backup registers (STM32).

2. **Adaptive Power Management**:
   - Jika baterai > 80%: kirim data setiap 1 menit.
   - Jika baterai 50-80%: kirim data setiap 5 menit.
   - Jika baterai 20-50%: kirim data setiap 15 menit.
   - Jika baterai < 20%: kirim data setiap 30 menit + matikan LED.
   - Jika baterai < 10%: emergency mode — hanya kirim 1x per jam.

3. **Data Transmission** (pilih salah satu):
   - Serial/UART output (minimum).
   - MQTT publish (nilai tambah — ESP32).
   - HTTP POST ke server (nilai tambah — ESP32).

4. **Status Reporting**:
   - Boot count (berapa kali sudah wake-up).
   - Battery level & estimated remaining time.
   - Uptime total (akumulatif dari boot count × sleep duration).
   - Last sensor readings.

### 4. Framework & Platform
- **ESP32**: PlatformIO, platform `espressif32`, framework `esp-idf`
- **STM32**: PlatformIO, platform `ststm32`, framework `stm32cube`

## 🛠️ Langkah Pengerjaan
1. Setup PlatformIO project sesuai platform masing-masing.
2. Implementasi pembacaan sensor (atau data dummy) via ESP-IDF `driver/adc.h` atau STM32 HAL ADC.
3. Implementasi battery monitoring via ADC.
4. Implementasi deep sleep (ESP-IDF `esp_deep_sleep_start()`) atau Standby (HAL `HAL_PWR_EnterSTANDBYMode()`).
5. Implementasi adaptive duty cycling berdasarkan level baterai.
6. Implementasi data persistence: `RTC_DATA_ATTR` (ESP32) atau `HAL_RTCEx_BKUPWrite()` (STM32).
7. Implementasi output data (Serial minimum, MQTT/HTTP opsional).
8. Testing & pengukuran konsumsi daya.

## 📝 Format Laporan
1. **Diagram Blok Sistem**: Alur dari panel surya → baterai → MCU → sensor → output.
2. **Flowchart Program**: Logika wake-up, baca sensor, adaptive sleep, dan transmisi data.
3. **Source Code**: Full code dengan komentar penjelasan setiap bagian power management.
4. **Tabel Pengukuran Daya**: Konsumsi pada setiap mode (active, transmitting, sleeping).
5. **Kalkulasi Battery Life**: Estimasi berapa lama sistem bisa bertahan pada skenario tertentu.
6. **Dokumentasi**: Foto/Screenshot Serial output yang menunjukkan adaptive duty cycling bekerja.
7. **Analisa**: 
   - Apa trade-off antara frekuensi pengiriman dan umur baterai?
   - Bagaimana strategi Anda jika baterai benar-benar habis dan baru terisi kembali?

## 🌟 Tantangan (Opsional - Nilai Tambah)
- Implementasi **ULP co-processor** (ESP32) untuk monitoring baterai saat deep sleep.
- Tambahkan **external wake-up** via button untuk memaksa pengiriman data segera.
- Implementasi **data batching**: kumpulkan 10 pembacaan di RTC memory, kirim sekaligus.
- Hitung dan tampilkan **estimasi sisa waktu baterai** berdasarkan konsumsi rata-rata aktual.
- Implementasi **watchdog timer** untuk recovery jika sistem hang.
- Simpan konfigurasi (interval, threshold) di **NVS** (ESP32) atau **Backup Register** (STM32).
