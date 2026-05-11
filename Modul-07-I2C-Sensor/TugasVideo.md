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
| ESP32_02 | SSD1306 OLED display graphics | teks/grafik tampil |
| ESP32_03 | BME280 environmental sensor (addr 0x76/0x77, SDA GPIO21, SCL GPIO22) | suhu/tekanan/humidity valid |
| ESP32_04 | MPU6050 IMU raw/angle | accel/gyro/angle berubah saat digerakkan |
| ESP32_05 | AT24C32 EEPROM data logger | tulis-baca record; kapasitas benar |
| ESP32_06 | DS3231 RTC alarm/temperature (addr 0x68) | waktu terbaca, alarm berfungsi |
| ESP32_07 | BH1750 light adaptive display (addr 0x23) | lux berubah saat cahaya berubah |
| ESP32_08 | I2C RTOS multi-task sensor | multiple task baca sensor via I2C RTOS |
| ESP32_09 | I2C RTOS data logger | logging data sensor via I2C RTOS |
| ESP32_10 | I2C RTOS interrupt-driven | interrupt-based I2C transfer berjalan |

---

### 5.2 STM32 — 10 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| STM32_01 | HAL I2C scanner | address map via UART |
| STM32_02 | SSD1306 OLED HAL driver | OLED tampil via STM32 HAL |
| STM32_03 | BME280 register driver | chip ID + data cuaca valid |
| STM32_04 | MPU6050 IMU interrupt ready | accel/gyro/interrupt berfungsi |
| STM32_05 | AT24C32 EEPROM page buffer | data valid setelah reset, page boundary benar |
| STM32_06 | DS3231 external RTC BCD (I2C1 PB6/7, addr 0x68) | waktu DS3231 tampil, BCD terkonversi |
| STM32_07 | Internal RTC backup register | waktu RTC internal + backup register valid |
| STM32_08 | I2C RTOS multi-task | multiple task I2C via STM32 RTOS |
| STM32_09 | I2C RTOS DMA transfer | DMA transfer I2C valid |
| STM32_10 | I2C RTOS error recovery | error recovery I2C via RTOS berjalan |

---

### 5.3 Multi STM32-ESP32 — 5 Eksperimen

| Kode | Demo wajib | Bukti output |
|---|---|---|
| MULTI_01 | I2C master-slave basic (STM32 slave addr 0x42) | data transfer master-slave valid |
| MULTI_02 | I2C role swap command | role master/slave swap via command |
| MULTI_03 | I2C shared sensor | sensor dibagi antara master dan slave |
| MULTI_04 | I2C RTOS gateway | gateway I2C RTOS berfungsi |
| MULTI_05 | I2C RTOS weather station | integrasi weather station final |

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
