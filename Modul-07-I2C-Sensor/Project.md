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

- **10 eksperimen ESP32**: scanner, BME280/BMP280, OLED, RTC/SNTP, EEPROM, MPU6050, BH1750, LCD, GPIO matrix/speed, recovery.
- **10 eksperimen STM32**: HAL scanner, raw driver, OLED, RTC internal, EEPROM, MPU6050, BH1750, LCD, DMA, recovery.
- **5 eksperimen Multi STM32-ESP32**: UART node, bus ownership, time sync, multi-display, final integration.

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

Data ditampilkan pada OLED dan/atau LCD, disimpan ke EEPROM secara circular, dan tetap dapat berjalan saat salah satu sensor mengalami gangguan.

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
 BME280/BMP280    BH1750       MPU6050       LCD PCF8574
 0x76/0x77        0x23/0x5C    0x69          0x27/0x3F
```

Alternatif: satu bus bersama boleh digunakan hanya jika ada mekanisme **bus ownership** eksplisit. Dua master tidak boleh mengakses bus bersamaan.

---

## 4. Peran Platform

### ESP32

- Gateway dan koordinator sistem.
- I2C scanner saat startup.
- GPIO matrix untuk remap pin atau dual bus.
- SNTP untuk sinkronisasi waktu saat WiFi tersedia.
- OLED dashboard.
- Error recovery level sistem.
- Komunikasi UART dengan STM32.

### STM32

- Node sensor deterministik.
- Raw register access untuk BME280/BMP280, MPU6050, BH1750.
- I2C HAL polling/interrupt/DMA.
- Internal RTC fallback.
- LCD status lokal.
- Logging atau verifikasi EEPROM bila bus dimiliki.

---

## 5. Device I2C Wajib

| Device | Address | Fungsi | Ketentuan |
|---|---:|---|---|
| BME280 | 0x76/0x77 | suhu, tekanan, kelembapan | utama jika tersedia |
| BMP280 | 0x76/0x77 | suhu, tekanan | fallback; humidity=`N/A` |
| SSD1306 | 0x3C/0x3D | OLED dashboard | minimal tampil status sensor |
| DS3231 | 0x68 | timestamp akurat | baterai terpasang |
| EEPROM AT24C32/24LC256 | 0x50–0x57 | data log | kapasitas wajib benar |
| MPU6050 | 0x69 disarankan | orientasi/guncangan | AD0=HIGH jika DS3231 ada |
| BH1750 | 0x23/0x5C | lux | mode continuous/one-shot |
| LCD PCF8574 | 0x27/0x3F | display lokal | cek pull-up 5 V |

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
7. Tampilkan address map di serial/OLED/LCD.

### Normal Loop

1. Baca sensor periodik tiap 2–10 detik.
2. Validasi data range.
3. Update OLED dan LCD.
4. Buat record 16 byte.
5. Simpan ke EEPROM circular.
6. Kirim ringkasan antar MCU via UART.
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

Skema minimal:

| Sinyal | Arah | Fungsi |
|---|---|---|
| BUS_REQ | STM32 → ESP32 | STM32 meminta akses bus |
| BUS_GRANT | ESP32 → STM32 | ESP32 memberi izin |
| BUS_BUSY opsional | ESP32/STM32 | status transaksi aktif |

Urutan:

1. ESP32 master default.
2. STM32 set BUS_REQ.
3. ESP32 menyelesaikan transaksi berjalan.
4. ESP32 set BUS_GRANT.
5. STM32 melakukan transaksi I2C.
6. STM32 melepas BUS_REQ.
7. ESP32 melepas BUS_GRANT dan mengambil kembali bus.

Alternatif lebih aman: pisahkan bus sensor STM32 dan bus display/log ESP32, lalu gunakan UART untuk data antar MCU.

---

## 10. Mapping 25 Eksperimen ke Project

| Eksperimen | Kontribusi project |
|---|---|
| ESP32_01, STM32_01 | startup scan dan address map |
| ESP32_02, STM32_02 | pembacaan cuaca BME280/BMP280 fallback |
| ESP32_03, STM32_03 | OLED dashboard dan framebuffer |
| ESP32_04, STM32_04 | DS3231, RTC internal, SNTP/time sync |
| ESP32_05, STM32_05 | EEPROM log circular dan page write |
| ESP32_06, STM32_06 | orientasi/guncangan MPU6050 |
| ESP32_07, STM32_07 | pembacaan lux BH1750 |
| ESP32_08, STM32_08 | LCD status lokal |
| ESP32_09 | GPIO matrix, dual bus, speed tuning |
| STM32_09 | DMA transfer benchmark |
| ESP32_10, STM32_10 | error handling dan recovery |
| MULTI_01 | sensor node via UART |
| MULTI_02 | bus ownership |
| MULTI_03 | sinkronisasi waktu |
| MULTI_04 | multi-display dashboard |
| MULTI_05 | integrasi final lengkap |

---

## 11. Fitur Minimal Wajib

1. Startup scanner menampilkan semua address.
2. BME280 berjalan; jika hanya BMP280, sistem tetap valid dengan humidity `N/A`.
3. BH1750 membaca lux.
4. MPU6050 membaca orientasi/guncangan.
5. DS3231 memberi timestamp; ESP32 SNTP menjadi koreksi jika tersedia.
6. OLED atau LCD menampilkan data real-time; nilai plus jika keduanya aktif.
7. EEPROM log circular dengan kapasitas benar.
8. Error recovery untuk sensor disconnect.
9. STM32 memakai minimal satu driver raw register.
10. ESP32 memakai GPIO matrix atau dual bus/speed test.
11. Multi-MCU berkomunikasi via UART atau bus ownership.

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

### LCD 16×2

```text
T28.4 P1008.6
L340 LOG128 OK
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
| Display OLED/LCD real-time | 15% |
| RTC DS3231 + internal RTC/SNTP sync | 10% |
| EEPROM logging dengan kapasitas benar | 15% |
| STM32 HAL raw register + DMA/benchmark | 10% |
| ESP32 ESP-IDF/GPIO matrix/speed handling | 10% |
| Multi-MCU UART atau bus ownership | 10% |
| Error recovery dan troubleshooting | 5% |
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
   └─ protocol_uart.md
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
- [ ] Menunjukkan OLED/LCD update.
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
