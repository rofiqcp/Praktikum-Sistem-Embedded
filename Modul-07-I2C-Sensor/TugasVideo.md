# Tugas Video — Modul 07: I2C Bus dan Integrasi Sensor

## Informasi Tugas

| Item | Detail |
|---|---|
| Modul | 07 — I2C Bus dan Integrasi Sensor |
| Struktur praktikum | 25 eksperimen: 10 ESP32 + 10 STM32 + 5 Multi STM32-ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Format | Video presentasi + demonstrasi hardware |
| Durasi | 20–35 menit |
| Upload | YouTube Unlisted atau link e-learning sesuai instruksi dosen |

---

## 1. Deskripsi

Video adalah bukti pemahaman dan demonstrasi praktikum Modul 07. Isi video harus selaras dengan struktur final:

- **10 eksperimen ESP32**.
- **10 eksperimen STM32**.
- **5 eksperimen Multi STM32-ESP32**.
- **Project final weather station dual-MCU**.

Bahasa presentasi: Indonesia.

---

## 2. Ketentuan Teknis

- Screen recording wajib: kode, serial monitor, output build/upload.
- Rekaman hardware wajib: ESP32, STM32, sensor, display, wiring SDA/SCL.
- Webcam/presenter disarankan terlihat.
- Resolusi minimal 720p; 1080p disarankan.
- Audio jelas.
- Tampilkan hasil nyata, bukan hanya slide.
- Boleh mempercepat bagian upload/compile, tetapi output penting harus terbaca.

---

## 3. Struktur Video Disarankan

| Bagian | Durasi | Isi |
|---|---:|---|
| Pembukaan | 1–2 menit | nama, NIM, Modul 07, daftar hardware |
| Ringkasan teori | 3–5 menit | I2C bus basics, open-drain, pull-up, addressing, ACK/NACK, START/STOP/repeated START, speed mode |
| Demo ESP32 | 6–8 menit | 10 eksperimen ESP32 ringkas |
| Demo STM32 | 6–8 menit | 10 eksperimen STM32 ringkas |
| Demo Multi | 4–6 menit | 5 eksperimen multi STM32-ESP32 |
| Project final | 4–6 menit | weather station dual-MCU |
| Penutup | 1–2 menit | kendala, troubleshooting, kesimpulan |

---

## 4. Materi Teori yang Wajib Disebut

1. SDA dan SCL sebagai bus 2-wire.
2. Open-drain dan fungsi pull-up resistor.
3. Address 7-bit dan konsep 10-bit addressing.
4. ACK/NACK pada bit ke-9.
5. START, STOP, repeated START.
6. Speed mode: 100 kHz, 400 kHz, 1 MHz, 3.4 MHz.
7. Clock stretching.
8. ESP32 ESP-IDF, GPIO matrix, internal RTC, SNTP.
9. STM32 HAL, DMA, internal RTC.
10. Error recovery: 9 pulse SCL + STOP + reinit.
11. Bus ownership untuk multi-master/master-slave ESP32-STM32.
12. Integrasi project dan troubleshooting.

---

## 5. Checklist Demo 25 Eksperimen

### 5.1 ESP32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| ESP32_01 | I2C scanner | address map di serial |
| ESP32_02 | BME280/BMP280 fallback | suhu/tekanan/humidity atau `N/A` |
| ESP32_03 | SSD1306 OLED dashboard | teks/grafik tampil |
| ESP32_04 | DS3231 + internal RTC + SNTP | waktu terbaca/sinkron |
| ESP32_05 | EEPROM AT24C32/24LC256 logger | tulis-baca record; kapasitas benar |
| ESP32_06 | MPU6050 raw/library | accel/gyro berubah saat digerakkan |
| ESP32_07 | BH1750 lux | lux berubah saat cahaya berubah |
| ESP32_08 | LCD PCF8574 | LCD menampilkan data |
| ESP32_09 | GPIO matrix, dual bus, speed test | pin remap/speed 100 vs 400 kHz |
| ESP32_10 | error handling dan recovery | sensor dicabut lalu reconnect |

---

### 5.2 STM32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_01 | HAL I2C scanner | address map via UART |
| STM32_02 | BME280/BMP280 raw driver | chip ID + data cuaca |
| STM32_03 | SSD1306 via HAL | OLED tampil |
| STM32_04 | DS3231 + internal RTC | waktu DS3231 dan RTC internal |
| STM32_05 | EEPROM logger + page boundary | data valid setelah reset |
| STM32_06 | MPU6050 burst read | 14 byte accel/gyro valid |
| STM32_07 | BH1750 mode one-shot/continuous | lux dan mode tampil |
| STM32_08 | LCD PCF8574 via HAL | LCD tampil via STM32 |
| STM32_09 | I2C DMA benchmark | polling vs interrupt vs DMA |
| STM32_10 | HAL error code dan recovery | error code + bus recovery |

---

### 5.3 Multi STM32-ESP32 — 5 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| MULTI_01 | ESP32 master/gateway + STM32 sensor node via UART | data STM32 tampil di ESP32 |
| MULTI_02 | Bus ownership request/grant | tidak ada collision saat akses bergantian |
| MULTI_03 | ESP32 SNTP sync ke STM32/DS3231 | waktu sinkron |
| MULTI_04 | Multi-display dashboard | OLED ESP32 + LCD STM32 konsisten |
| MULTI_05 | Final weather station integration | semua sensor/display/log/recovery aktif |

---

## 6. Demo Project Final

Project final yang ditampilkan: **Weather Station Dual-MCU Modul 07**.

Wajib terlihat:

1. ESP32 dan STM32 aktif.
2. Scanner menemukan device I2C.
3. BME280 atau BMP280 fallback berjalan.
4. Jika BMP280 dipakai, humidity ditampilkan `N/A`, bukan angka palsu.
5. BH1750 membaca lux.
6. MPU6050 membaca orientasi/guncangan.
7. DS3231 memberi timestamp.
8. ESP32 SNTP/internal RTC dijelaskan sebagai sinkronisasi/fallback.
9. STM32 internal RTC dijelaskan sebagai fallback.
10. EEPROM logging berjalan dengan kapasitas benar:
    - AT24C32: maksimal 240 record 16 byte.
    - 24LC256: maksimal 2000 record 16 byte.
11. OLED SSD1306 dan/atau LCD PCF8574 menampilkan data.
12. Error recovery ditunjukkan dengan sensor disconnect/reconnect.
13. Bus ownership atau split bus dijelaskan untuk mencegah multi-master collision.

---

## 7. Narasi Minimum untuk Kapasitas EEPROM

Saat menjelaskan data logger, sebutkan:

> Record log berukuran 16 byte. Jika memakai AT24C32 4 KB, setelah metadata sistem dibatasi maksimal 240 record. Jika memakai 24LC256 32 KB, sistem boleh menyimpan sampai 2000 record. Jadi 1000 record tidak valid untuk AT24C32, tetapi valid untuk 24LC256.

---

## 8. Format Pengumpulan

Kumpulkan:

1. Link video.
2. Link repository atau arsip source code.
3. Laporan PDF/MD.
4. Foto wiring final.
5. Tabel hasil 25 eksperimen.

Nama file/link disarankan:

```text
Modul07_I2C_Nama_NIM
```

---

## 9. Rubrik Penilaian Video

| Komponen | Bobot |
|---|---:|
| Pemahaman teori I2C lengkap | 15% |
| Demo 10 eksperimen ESP32 | 15% |
| Demo 10 eksperimen STM32 | 15% |
| Demo 5 eksperimen Multi STM32-ESP32 | 15% |
| Demo project weather station dual-MCU | 20% |
| Kualitas hardware demo dan wiring explanation | 10% |
| Kualitas audio/video/struktur presentasi | 10% |

---

## 10. Penalti

| Pelanggaran | Penalti |
|---|---:|
| Tidak ada demo hardware | −25% |
| Tidak menunjukkan 25 eksperimen | proporsional jumlah yang hilang |
| Salah menyebut Modul 06, bukan Modul 07 | −5% |
| Mengklaim AT24C32 menyimpan 1000 record 16 byte | −10% |
| Tidak menjelaskan BME280/BMP280 fallback | −5% |
| Tidak menjelaskan konflik DS3231-MPU6050 0x68 | −5% |
| Tidak ada output serial/display | −10% |
| Audio tidak jelas | −10% |
| Video terlalu pendek (<15 menit) | −10% |
| Terlambat | sesuai kebijakan kelas |

---

## 11. Checklist Akhir Sebelum Upload

- [ ] Judul video menyebut **Modul 07**.
- [ ] Bahasa Indonesia.
- [ ] Hardware ESP32 dan STM32 terlihat.
- [ ] SDA/SCL, pull-up, common ground dijelaskan.
- [ ] 10 eksperimen ESP32 ditampilkan.
- [ ] 10 eksperimen STM32 ditampilkan.
- [ ] 5 eksperimen Multi ditampilkan.
- [ ] Project final ditampilkan.
- [ ] EEPROM capacity benar.
- [ ] BME280/BMP280 fallback benar.
- [ ] Error recovery ditunjukkan.
- [ ] Link bisa diakses.
