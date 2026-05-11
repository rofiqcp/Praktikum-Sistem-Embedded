# Project Modul 07: Weather Station Dual-MCU I2C

## Informasi Project

| Item | Keterangan |
|---|---|
| Modul | 07 — I2C Bus dan Integrasi Sensor |
| Struktur praktikum | 10 ESP32 + 10 STM32 + 5 Multi STM32-ESP32 |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Tema | Weather station dual-MCU multi-sensor |
| Durasi | 2 minggu |
| Output | Sistem hardware, source code, laporan, video demo |

---

## 1. Deskripsi Umum

Project Modul 07 menggabungkan seluruh materi I2C menjadi **weather station dual-MCU**. ESP32 dan STM32 bekerja bersama untuk membaca sensor cuaca, menampilkan data, menyimpan log, menjaga waktu, dan memulihkan bus jika terjadi error.

Sistem wajib selaras dengan struktur final praktikum:

- **10 eksperimen ESP32**: I2C bus scanner, SSD1306 OLED, BME280 environmental, MPU6050 IMU, AT24C32 EEPROM log, DS3231 RTC, BH1750 light sensor, I2C RTOS multi-task, I2C RTOS data logger, I2C RTOS interrupt-driven.
- **10 eksperimen STM32**: I2C HAL scanner, SSD1306 OLED HAL, BME280 register driver, MPU6050 IMU interrupt, AT24C32 EEPROM buffer, DS3231 RTC BCD, internal RTC backup register, I2C RTOS multi-task, I2C RTOS DMA transfer, I2C RTOS error recovery.
- **5 eksperimen Multi STM32-ESP32**: I2C master-slave basic, I2C role swap command, I2C shared sensor, I2C RTOS gateway, I2C RTOS weather station.

---

## 2. Skenario

Sebuah stasiun cuaca untuk pertanian cerdas harus mengukur kondisi lingkungan lokal:

- Suhu udara.
- Tekanan udara.
- Kelembapan jika BME280 tersedia.
- Intensitas cahaya.
- Orientasi/guncangan perangkat.
- Waktu pengambilan data.
- Status sensor dan error I2C.

Data ditampilkan pada OLED SSD1306, disimpan ke EEPROM secara circular, dan tetap dapat berjalan saat salah satu sensor mengalami gangguan.

---

## 3. Arsitektur Sistem

```text
                 WiFi/SNTP
                    │
              ┌─────▼─────┐          UART / REQ-GRANT
              │   ESP32   │◄────────────────────────┐
              │ Gateway   │                         │
              └─────┬─────┘                         │
                    │ I2C bus A / display            │
       ┌────────────┼─────────────┐                  │
       │            │             │                  │
   SSD1306       DS3231       EEPROM                 │
   0x3C          0x68         0x50                   │
                                                     │
              ┌───────────────────┐                  │
              │       STM32       │──────────────────┘
              │ Sensor/Logger Node│
              └─────┬─────────────┘
                    │ I2C bus B / sensor
        ┌────────────┼─────────────┬─────────────┐
        │            │             │             │
  BME280/BMP280    BH1750       MPU6050
  0x76/0x77        0x23         0x69
```

Alternatif: satu bus bersama boleh digunakan hanya jika ada mekanisme **bus ownership** eksplisit. Dua master tidak boleh mengakses bus bersamaan.

---

## 4. Peran Platform

### ESP32

- Gateway dan koordinator sistem.
- I2C scanner saat startup (ESP32_01).
- GPIO21/22 SDA/SCL untuk ESP32, GPIO8/9 untuk S2/S3, 100kHz I2C frequency.
- ESP-IDF I2C master API (i2c_master_bus_handle_t).
- SNTP untuk sinkronisasi waktu saat WiFi tersedia.
- OLED SSD1306 dashboard (ESP32_02).
- BH1750 light adaptive display (ESP32_07).
- RTOS multi-task sensor, data logger, interrupt-driven (ESP32_08/09/10).
- Komunikasi I2C dengan STM32 slave addr 0x42 (Multi_01).

### STM32

- Node sensor deterministik.
- STM32F103C8 / STM32F401CC / STM32F411CE.
- I2C1 PB6/7 SCL/SDA, 100kHz, HAL I2C.
- BME280 register driver (STM32_03), MPU6050 IMU interrupt (STM32_04).
- DS3231 RTC external BCD (STM32_06), internal RTC backup register (STM32_07).
- AT24C32 EEPROM page buffer (STM32_05).
- SSD1306 OLED HAL driver (STM32_02).
- RTOS multi-task (STM32_08), DMA transfer (STM32_09), error recovery (STM32_10).
- Dapat berperan sebagai I2C slave addr 0x42 (Multi_01).

---

## 5. Device I2C Wajib

| Device | Address | Fungsi | Ketentuan |
|---|---:|---|---|
| BME280 | 0x76/0x77 | suhu, tekanan, kelembapan | utama (ESP32_03, STM32_03) |
| BMP280 | 0x76/0x77 | suhu, tekanan | fallback; humidity=`N/A` |
| SSD1306 | 0x3C/0x3D | OLED dashboard | ESP32_02, STM32_02 |
| DS3231 | 0x68 | timestamp akurat | ESP32_06, STM32_06; baterai terpasang |
| EEPROM AT24C32 | 0x50 | data log | ESP32_05, STM32_05; kapasitas 4096 byte |
| MPU6050 | 0x69 disarankan | orientasi/guncangan | ESP32_04, STM32_04; AD0=HIGH jika DS3231 ada |
| BH1750 | 0x23 | lux | ESP32_07; mode continuous/one-shot |

---

## 6. Format Record Log

Gunakan record tetap 16 byte agar mudah dihitung dan dibaca ulang.

| Offset | Field | Ukuran | Format |
|---:|---|---:|---|
| 0 | Unix timestamp | 4 byte | uint32 |
| 4 | temperature | 2 byte | int16, °C × 100 |
| 6 | pressure | 2 byte | uint16, hPa × 10 |
| 8 | humidity | 2 byte | uint16, %RH × 100; `0xFFFF` jika BMP280 |
| 10 | lux | 2 byte | uint16 |
| 12 | accel magnitude | 2 byte | uint16, g × 1000 |
| 14 | status flags | 1 byte | bitfield sensor/error |
| 15 | checksum | 1 byte | XOR/CRC8 sederhana |

Metadata EEPROM disimpan di awal:

| Field | Ukuran | Catatan |
|---|---:|---|
| magic | 2 byte | contoh 0x4D07 |
| version | 1 byte | format record |
| record_size | 1 byte | 16 |
| write_index | 2 byte | index circular |
| record_count | 2 byte | jumlah record valid |
| reserved | 8 byte | ekspansi |

Total metadata disarankan 16 byte.

---

## 7. Kapasitas EEPROM yang Benar

Project tidak boleh memakai klaim kapasitas yang salah.

### Jika memakai AT24C32

- Kapasitas: 32 Kbit = **4096 byte**.
- Metadata: 16 byte.
- Ruang record: 4080 byte.
- Record 16 byte: 4080 / 16 = 255 record mentah.
- Batas aman project: **maksimal 240 record**.

### Jika memakai 24LC256

- Kapasitas: 256 Kbit = **32768 byte**.
- Metadata: 16 byte.
- Ruang record: 32752 byte.
- Record 16 byte: 2047 record mentah.
- Batas aman project: **maksimal 2000 record**.

Aturan final:

- **AT24C32: batasi 240 record.**
- **24LC256: boleh log besar sampai 2000 record.**
- Jika ingin 1000+ record, gunakan 24LC256, bukan AT24C32.

---

## 8. Alur Operasi

### Startup

1. ESP32 dan STM32 boot.
2. Jalankan I2C scanner.
3. Deteksi BME280/BMP280 via chip ID.
4. Deteksi konflik address DS3231/MPU6050.
5. Init RTC: SNTP → DS3231 → RTC internal fallback.
6. Init EEPROM metadata.
7. Tampilkan address map di serial/OLED.

### Normal Loop

1. Baca sensor periodik tiap 2–10 detik.
2. Validasi data range.
3. Update OLED SSD1306.
4. Buat record 16 byte.
5. Simpan ke EEPROM circular.
6. Kirim ringkasan antar MCU via I2C.
7. Catat error count.

### Error Mode

1. Jika NACK/timeout, tandai device offline.
2. Sistem tetap berjalan dengan data sensor lain.
3. Lakukan retry berkala.
4. Jika SDA/SCL stuck, lakukan bus recovery.
5. Setelah recovery, scan ulang dan reinit device.

---

## 9. Bus Ownership Multi-MCU

Jika ESP32 dan STM32 berada pada bus I2C yang sama, wajib ada kontrol akses.

Eksperimen Multi_01: STM32 sebagai I2C slave addr 0x42, ESP32 sebagai master.

Skema role swap (Multi_02_I2C_Role_Swap_Command):

| Sinyal | Arah | Fungsi |
|---|---|---|
| BUS_REQ | STM32 → ESP32 | STM32 meminta akses bus |
| BUS_GRANT | ESP32 → STM32 | ESP32 memberi izin |
| BUS_BUSY opsional | ESP32/STM32 | status transaksi aktif |

Urutan:
1. ESP32 master default (Multi_01, Multi_04).
2. STM32 set BUS_REQ.
3. ESP32 menyelesaikan transaksi berjalan.
4. ESP32 set BUS_GRANT.
5. STM32 melakukan transaksi I2C.
6. STM32 melepas BUS_REQ.
7. ESP32 melepas BUS_GRANT dan mengambil kembali bus.

Alternatif lebih aman: pisahkan bus sensor STM32 dan bus display/log ESP32, lalu gunakan I2C antar MCU (Multi_01) atau RTOS gateway (Multi_04).

---

## 10. Mapping 25 Eksperimen ke Project

| Eksperimen | Kontribusi project |
|---|---|
| ESP32_01, STM32_01 | startup scan dan address map |
| ESP32_02, STM32_02 | OLED SSD1306 display graphics/dashboard |
| ESP32_03, STM32_03 | BME280 environmental/register driver, suhu/tekanan/kelembapan |
| ESP32_04, STM32_04 | MPU6050 IMU raw/interrupt, orientasi/guncangan |
| ESP32_05, STM32_05 | AT24C32 EEPROM data log/page buffer |
| ESP32_06, STM32_06 | DS3231 RTC alarm/external BCD, timestamp |
| ESP32_07 | BH1750 light adaptive display, lux |
| STM32_07 | Internal RTC backup register, fallback waktu |
| ESP32_08 | I2C RTOS multi-task sensor |
| STM32_08 | I2C RTOS multi-task (BME280, OLED, EEPROM) |
| ESP32_09 | I2C RTOS data logger |
| STM32_09 | I2C RTOS DMA transfer benchmark |
| ESP32_10 | I2C RTOS interrupt-driven |
| STM32_10 | I2C RTOS error recovery, watchdog |
| MULTI_01 | I2C master-slave basic, STM32 slave addr 0x42 |
| MULTI_02 | I2C role swap command, bus ownership |
| MULTI_03 | I2C shared sensor |
| MULTI_04 | I2C RTOS gateway |
| MULTI_05 | I2C RTOS weather station, integrasi final |

---

## 11. Fitur Minimal Wajib

1. Startup scanner menampilkan semua address (ESP32_01, STM32_01).
2. BME280 berjalan dengan register driver (ESP32_03, STM32_03); fallback BMP280 jika ada.
3. BH1750 membaca lux (ESP32_07).
4. MPU6050 IMU raw/interrupt (ESP32_04, STM32_04).
5. DS3231 external RTC BCD dan internal RTC backup register (ESP32_06, STM32_06, STM32_07).
6. SSD1306 OLED menampilkan data real-time (ESP32_02, STM32_02).
7. AT24C32 EEPROM log dengan kapasitas benar (ESP32_05, STM32_05).
8. Error recovery dan watchdog (STM32_10).
9. STM32 memakai minimal satu register driver raw (STM32_03 BME280_Register_Driver).
10. ESP32 dan STM32 menggunakan RTOS multi-task (ESP32_08/09/10, STM32_08/09/10).
11. Multi-MCU berkomunikasi via I2C master-slave atau RTOS gateway (Multi_01/04).

---

## 12. Output Tampilan Minimum

### OLED

```text
MODUL 07 WEATHER
T: 28.45 C  H: 70.2%
P: 1008.6 hPa
L: 340 lux
IMU: OK  LOG: 128/240
```

Jika BMP280:

```text
H: N/A (BMP280)
```

### Serial Log

```text
SCAN: 0x23 BH1750, 0x3C SSD1306, 0x50 EEPROM, 0x68 DS3231, 0x69 MPU6050, 0x76 BME280
REC: ts=1710000000,temp=2845,press=10086,hum=7020,lux=340,imu=1002,flags=0x00
```

---

## 13. Rubrik Penilaian

| Komponen | Bobot |
|---|---:|
| Integrasi sensor I2C BME280/BMP280, BH1750, MPU6050 | 20% |
| Display OLED SSD1306 real-time | 15% |
| RTC DS3231 external BCD + internal RTC backup register | 10% |
| EEPROM AT24C32 logging dengan kapasitas benar | 15% |
| STM32 HAL raw register BME280 + DMA transfer | 10% |
| ESP32 ESP-IDF + I2C RTOS multi-task | 10% |
| Multi-MCU I2C master-slave/role swap (STM32 slave 0x42) | 10% |
| Error recovery dan watchdog | 5% |
| Kode modular, laporan, video | 5% |

---

## 14. Struktur Kode Disarankan

```text
project_modul_07/
├─ esp32/
│  ├─ i2c_bus.c
│  ├─ sensor_weather.c
│  ├─ display_oled.c
│  ├─ rtc_sntp.c
│  ├─ eeprom_log.c
│  └─ recovery.c
├─ stm32/
│  ├─ i2c_hal.c
│  ├─ bme280_raw.c
│  ├─ mpu6050_raw.c
│  ├─ bh1750_raw.c
│  ├─ rtc_internal.c
│  └─ dma_benchmark.c
└─ shared/
    ├─ record_format.h
    └─ protocol_i2c.md
```

Dokumen baru tidak wajib dibuat; struktur ini hanya panduan implementasi.

---

## 15. Checklist Demo

- [ ] Menunjukkan hardware ESP32 + STM32.
- [ ] Menunjukkan SDA/SCL, pull-up, common ground.
- [ ] Scanner menemukan address device.
- [ ] Menjelaskan konflik 0x68 dan solusi MPU6050 AD0=HIGH.
- [ ] Menunjukkan BME280 atau BMP280 fallback.
- [ ] Menunjukkan lux BH1750 berubah saat cahaya berubah.
- [ ] Menunjukkan MPU6050 berubah saat digerakkan.
- [ ] Menunjukkan OLED SSD1306 update.
- [ ] Menunjukkan log EEPROM dan batas record benar.
- [ ] Menunjukkan timestamp DS3231/SNTP/RTC.
- [ ] Menunjukkan error recovery atau reconnect.
- [ ] Menunjukkan komunikasi ESP32-STM32.

---

## 16. Catatan Keselamatan dan Keandalan

- Jangan pull-up SDA/SCL ke 5 V jika MCU 3.3 V tidak toleran.
- Jangan memakai dua master aktif tanpa ownership.
- Jangan melewati page boundary EEPROM saat page write.
- Jangan menyimpan 1000 record pada AT24C32 dengan record 16 byte.
- Jangan menyatukan DS3231 dan MPU6050 di 0x68 tanpa mengubah AD0 MPU6050.
- Selalu gunakan timeout dan cek return code.
