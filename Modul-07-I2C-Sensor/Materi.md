# Modul 07: I2C Bus dan Integrasi Sensor

## Capaian Pembelajaran

Setelah menyelesaikan Modul 07, mahasiswa mampu:

1. Menjelaskan bus I2C: SDA, SCL, open-drain, pull-up, addressing, ACK/NACK, START/STOP, repeated START, speed mode, clock stretching, dan konsep 10-bit addressing.
2. Mengonfigurasi I2C pada ESP32 dengan ESP-IDF, termasuk GPIO matrix, dual bus, internal RTC, dan sinkronisasi SNTP.
3. Mengonfigurasi I2C pada STM32 dengan HAL, polling, interrupt, DMA, scan bus, error code, dan internal RTC.
4. Mengintegrasikan sensor/perangkat I2C: BME280 atau BMP280 fallback, SSD1306, DS3231, EEPROM, MPU6050, BH1750, dan LCD PCF8574.
5. Mendesain sistem multi-device, multi-master/master-slave, error recovery, troubleshooting, dan integrasi project weather station dual-MCU.

---

## 1. Dasar Bus I2C

I2C (Inter-Integrated Circuit) adalah protokol serial sinkron 2 jalur untuk komunikasi antar-chip jarak pendek. Jalur utama:

- **SDA (Serial Data)**: data bidirectional.
- **SCL (Serial Clock)**: clock dari master; slave dapat menahan SCL LOW untuk clock stretching.
- **GND bersama** wajib.

Topologi bus:

```text
3.3V
 │
 ├─[Rp]─ SDA ─┬─ Master ESP32/STM32
 │            ├─ Sensor BME280/BMP280
 └─[Rp]─ SCL ─┼─ OLED SSD1306
              ├─ RTC DS3231
              ├─ EEPROM AT24C32/24LC256
              └─ Sensor lain
```

Karakteristik utama:

| Aspek | I2C |
|---|---|
| Jalur | SDA, SCL, GND |
| Mode | Half-duplex |
| Topologi | Shared bus |
| Peran | Master/slave; multi-master memungkinkan |
| Address umum | 7-bit; 10-bit konsep/lanjutan |
| Kecepatan umum | 100 kHz, 400 kHz, 1 MHz, 3.4 MHz |
| Output | Open-drain/open-collector + pull-up |

---

## 2. Open-Drain dan Pull-Up

I2C tidak mendorong sinyal HIGH secara aktif. Device hanya menarik SDA/SCL ke LOW; resistor pull-up menariknya kembali ke HIGH.

Konsekuensi:

- Banyak device bisa berbagi satu jalur tanpa saling bentrok keras.
- LOW dominan; HIGH terjadi saat semua device melepas bus.
- Rise time tergantung nilai resistor dan kapasitansi bus.

Rumus batas umum:

```text
Rp_min = (VCC - VOL) / IOL
Rp_max = tr / (0.8473 × Cb)
```

Nilai praktis:

| Kondisi | Nilai awal |
|---|---:|
| Bus pendek 100 kHz | 4.7 kΩ |
| Bus pendek 400 kHz | 2.2–4.7 kΩ |
| Banyak modul pull-up onboard | ukur ekuivalen; hindari terlalu kecil |
| Low power, bus pendek | 10 kΩ, uji rise time |

Catatan: jika beberapa breakout sudah punya pull-up 10 kΩ, nilai paralel bisa turun. Empat modul 10 kΩ paralel ≈ 2.5 kΩ.

---

## 3. Addressing, ACK/NACK, dan Frame

Transaksi dasar:

```text
START → Address 7-bit + R/W → ACK → Data → ACK/NACK → STOP
```

- **Address 7-bit**: 0x00–0x7F; beberapa address reserved.
- **R/W bit**: 0 = write, 1 = read.
- **ACK**: receiver menarik SDA LOW pada clock ke-9.
- **NACK**: SDA tetap HIGH pada clock ke-9.
- **HAL STM32** sering memakai address digeser kiri: `addr << 1`.
- **ESP32/Wire/ESP-IDF** biasanya memakai address 7-bit asli pada API level tinggi.

10-bit addressing ada di spesifikasi I2C untuk device lebih banyak, tetapi jarang dipakai pada sensor praktikum. Konsepnya memakai prefix khusus di byte address pertama lalu lanjutan address pada byte berikutnya. Praktikum Modul 07 fokus 7-bit, tetapi mahasiswa harus tahu 10-bit dapat muncul pada sistem industri khusus.

---

## 4. START, STOP, dan Repeated START

- **START**: SDA turun LOW saat SCL HIGH.
- **STOP**: SDA naik HIGH saat SCL HIGH.
- **Repeated START**: START baru tanpa STOP; dipakai untuk baca register sensor secara atomik.

Contoh baca register:

```text
START → addr+W → ACK → reg → ACK → repeated START → addr+R → ACK → data → NACK → STOP
```

Repeated START penting karena master tidak melepas bus setelah menulis alamat register. Ini mencegah master lain mengambil bus sebelum operasi read selesai.

---

## 5. Speed Mode dan Kapasitansi Bus

| Mode | Clock | Penggunaan |
|---|---:|---|
| Standard | 100 kHz | paling aman, sensor lambat, kabel lebih panjang |
| Fast | 400 kHz | sensor modern, OLED, IMU |
| Fast Mode Plus | 1 MHz | bus pendek, pull-up kuat, device kompatibel |
| High Speed | 3.4 MHz | aplikasi khusus; jarang di modul praktikum |

Faktor pembatas:

- Kapasitansi kabel dan modul.
- Nilai pull-up.
- Clock stretching oleh slave.
- Kualitas ground dan noise.
- Level tegangan 3.3 V vs 5 V.

Mulai dari 100 kHz untuk bring-up, naik ke 400 kHz setelah stabil.

---

## 6. Daftar Sensor dan Perangkat Modul 07

| Device | Alamat umum | Fungsi | Catatan |
|---|---:|---|---|
| BME280 | 0x76/0x77 | suhu, tekanan, kelembapan | prioritas utama |
| BMP280 | 0x76/0x77 | suhu, tekanan | fallback jika BME280 tidak tersedia; tanpa humidity |
| SSD1306 OLED | 0x3C/0x3D | display grafis 128×64 | buffer display |
| DS3231 | 0x68 | RTC presisi | konflik dengan MPU6050 default |
| AT24C32 | 0x50–0x57 | EEPROM 4 KB | umum pada modul DS3231 |
| 24LC256 | 0x50–0x57 | EEPROM 32 KB | log lebih besar |
| MPU6050 | 0x68/0x69 | accel + gyro | set AD0=HIGH → 0x69 jika DS3231 dipakai |
| BH1750 | 0x23/0x5C | sensor cahaya lux | mode continuous/one-shot |
| LCD PCF8574 | 0x27/0x3F | LCD 16×2 via I2C | biasanya power 5 V |

---

## 7. BME280 dan BMP280 Fallback

BME280 membaca suhu, tekanan, dan kelembapan. BMP280 membaca suhu dan tekanan saja. Driver harus mendeteksi chip ID:

| Sensor | Chip ID umum | Data |
|---|---:|---|
| BMP280 | 0x58 | suhu, tekanan |
| BME280 | 0x60 | suhu, tekanan, kelembapan |

Strategi fallback:

1. Scan address 0x76 lalu 0x77.
2. Baca register chip ID 0xD0.
3. Jika 0x60, aktifkan humidity path.
4. Jika 0x58, set humidity sebagai `N/A` dan project tetap berjalan.
5. Tampilkan label sensor aktual di serial/OLED.

Register penting:

- `0xD0`: chip ID.
- `0xF2`: humidity control pada BME280.
- `0xF4`: temperature/pressure control.
- `0xF5`: config.
- `0xF7` dst: data pressure/temperature/humidity.

---

## 8. SSD1306 OLED

SSD1306 adalah display grafis monokrom 128×64 atau 128×32. Semua pixel dikirim melalui buffer.

Prinsip:

- Init address 0x3C atau 0x3D.
- Clear buffer.
- Draw text/grafik ke RAM.
- Flush buffer ke OLED.

Estimasi ukuran buffer 128×64:

```text
128 × 64 / 8 = 1024 byte
```

Pada I2C 100 kHz, full refresh lebih lambat; 400 kHz lebih nyaman untuk UI.

---

## 9. DS3231 dan RTC Internal

DS3231 memberi waktu akurat dengan backup baterai CR2032. Data waktu disimpan dalam BCD.

Fungsi project:

- Timestamp log.
- Jadwal sampling.
- Koreksi waktu setelah reset.

RTC internal:

- **STM32**: RTC berjalan dari LSE 32.768 kHz atau LSI; backup domain via VBAT. Cocok untuk timekeeping lokal.
- **ESP32**: RTC domain mendukung deep sleep; waktu sistem dapat diset manual atau disinkronkan via SNTP setelah WiFi aktif.

Perbandingan:

| Aspek | RTC internal | DS3231 |
|---|---|---|
| Komponen | tanpa modul tambahan | modul eksternal |
| Akurasi | bergantung clock | sangat baik, ±2 ppm tipikal |
| Backup | STM32 VBAT; ESP32 RTC domain terbatas | baterai modul |
| Project | fallback | timestamp utama |

---

## 10. EEPROM: AT24C32 dan 24LC256

EEPROM I2C dipakai untuk data log non-volatile.

| EEPROM | Kapasitas | Page size umum | Catatan |
|---|---:|---:|---|
| AT24C32 | 32 Kbit = 4096 byte | 32 byte | sering pada modul DS3231 |
| 24LC256 | 256 Kbit = 32768 byte | 64 byte | cocok log panjang |

Operasi tulis:

1. Kirim address EEPROM.
2. Kirim alamat memori 2 byte.
3. Kirim data.
4. Tunggu write cycle ±5 ms atau lakukan ACK polling.

Batas project:

- Jika record 16 byte dan memakai AT24C32: `4096 / 16 = 256 record` maksimal mentah. Sisakan metadata, gunakan **maks. 240 record**.
- Jika memakai 24LC256: `32768 / 16 = 2048 record` mentah. Sisakan metadata, gunakan **maks. 2000 record**.

Hindari klaim AT24C32 menyimpan 1000 record 16 byte; itu melampaui kapasitas.

---

## 11. MPU6050

MPU6050 berisi akselerometer 3-axis dan giroskop 3-axis.

Alamat:

- AD0=LOW → 0x68.
- AD0=HIGH → 0x69.

Jika DS3231 ada pada bus yang sama, set MPU6050 ke 0x69.

Data penting:

- Wake-up register `0x6B`.
- Accel range: ±2g, ±4g, ±8g, ±16g.
- Gyro range: ±250, ±500, ±1000, ±2000 °/s.
- Burst read 14 byte: accel, temperature, gyro.

Aplikasi Modul 07:

- Orientasi node weather station.
- Deteksi guncangan/tamper.
- Latihan read raw multi-byte register.

---

## 12. BH1750

BH1750 mengukur intensitas cahaya dalam lux.

Mode umum:

| Mode | Resolusi | Waktu ukur |
|---|---:|---:|
| Continuous High | 1 lux | ±120–180 ms |
| Continuous High2 | 0.5 lux | ±120–180 ms |
| Continuous Low | 4 lux | ±16–24 ms |
| One-shot | sesuai mode | hemat daya |

Aplikasi:

- Deteksi siang/malam.
- Auto brightness display.
- Korelasi cahaya terhadap suhu/kelembapan pada weather station.

---

## 13. LCD PCF8574

PCF8574 adalah I/O expander I2C untuk LCD karakter HD44780 16×2.

Catatan:

- Alamat sering 0x27 atau 0x3F.
- LCD biasanya butuh 5 V untuk backlight dan kontras.
- Jalur I2C tetap harus aman terhadap 3.3 V; cek pull-up backpack. Jika pull-up ke 5 V, gunakan level shifter atau modifikasi pull-up ke 3.3 V.
- Trimpot kontras harus disetel.

---

## 14. STM32 HAL I2C

STM32F103 umum:

| Peripheral | SCL | SDA |
|---|---|---|
| I2C1 | PB6 | PB7 |
| I2C1 remap | PB8 | PB9 |
| I2C2 | PB10 | PB11 |

Konfigurasi HAL:

- GPIO mode: Alternate Function Open-Drain.
- Speed: 100 kHz atau 400 kHz.
- Addressing: 7-bit untuk praktikum.
- NoStretchMode: disable agar clock stretching didukung.

Operasi penting:

```c
HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 2, 10);
HAL_I2C_Mem_Read(&hi2c1, addr << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
HAL_I2C_Master_Transmit_DMA(&hi2c1, addr << 1, buf, len);
HAL_I2C_Master_Receive_DMA(&hi2c1, addr << 1, buf, len);
```

DMA berguna untuk transfer besar seperti update OLED atau baca blok EEPROM, sambil CPU menjalankan tugas lain.

---

## 15. STM32 Internal RTC

STM32 memakai backup domain. Pilihan clock:

- **LSE 32.768 kHz**: lebih akurat, butuh kristal.
- **LSI internal**: tanpa kristal, lebih drift.

Fungsi Modul 07:

- Timestamp fallback jika DS3231 tidak tersedia.
- Alarm sampling periodik.
- Backup register untuk menyimpan pointer log terakhir.

Praktik baik:

- Aktifkan akses backup domain.
- Jangan reset backup domain setiap boot.
- Validasi marker backup register untuk tahu RTC sudah diset atau belum.

---

## 16. ESP32 ESP-IDF I2C

ESP32 punya dua controller I2C dan GPIO matrix fleksibel.

Konfigurasi praktikum Modul 07:
- **SDA**: GPIO21
- **SCL**: GPIO22
- **I2C port**: I2C_NUM_0
- **Clock speed**: 100 kHz

Konsep ESP-IDF:

- Pilih `I2C_NUM_0` atau `I2C_NUM_1`.
- Tentukan SDA/SCL ke GPIO valid (praktikum: GPIO21/22).
- Aktifkan pull-up internal hanya untuk bus sangat pendek; pull-up eksternal tetap disarankan.
- Set clock 100 kHz/400 kHz.
- Gunakan timeout dan cek return `esp_err_t`.

GPIO matrix memungkinkan:

- SDA/SCL dipindah ke pin lain tanpa perubahan hardware besar.
- Dual bus: bus sensor dan bus display dipisah.
- Recovery dengan deinit driver lalu bit-bang GPIO.

---

## 17. ESP32 Internal RTC dan SNTP

ESP32 memiliki RTC domain untuk waktu saat deep sleep, tetapi akurasi bergantung sumber clock. Untuk project IoT, waktu dapat disinkronkan via SNTP.

Alur:

1. Boot.
2. Jika WiFi tersedia, sinkronkan SNTP.
3. Set waktu sistem ESP32.
4. Opsional: tulis waktu ke DS3231 agar node tetap akurat offline.
5. Saat offline, gunakan DS3231 atau RTC internal sebagai fallback.

SNTP berguna untuk weather station yang mengirim data ke jaringan. DS3231 tetap dipakai agar timestamp tidak hilang saat WiFi tidak tersedia.

---

## 18. Error Handling dan Bus Recovery

Masalah umum:

| Gejala | Penyebab | Solusi |
|---|---|---|
| Device tidak muncul scan | alamat salah, power salah, SDA/SCL tertukar | cek wiring, scan, datasheet |
| NACK address | device mati/salah alamat | retry, cek pull-up |
| Timeout | SCL/SDA stuck, clock stretching lama | timeout, recovery |
| Data acak | pull-up lemah, noise, kabel panjang | turunkan speed, rapikan kabel |
| SDA LOW terus | slave macet di tengah byte | pulse SCL 9 kali + STOP |

Recovery standar:

1. Deinit I2C peripheral.
2. Jadikan SCL/SDA GPIO open-drain/input pull-up.
3. Pulse SCL 9 kali.
4. Buat STOP manual: SDA LOW → SCL HIGH → SDA HIGH.
5. Init ulang I2C.
6. Scan ulang device.

---

## 19. Clock Stretching dan 10-bit Addressing

Clock stretching terjadi saat slave menahan SCL LOW. Master harus menunggu sampai SCL dilepas. Sensor lambat, EEPROM saat write cycle, atau device bridge dapat memakai mekanisme ini.

Praktik:

- Pastikan driver/peripheral tidak menonaktifkan clock stretching.
- Gunakan timeout agar sistem tidak hang permanen.
- Catat device yang sering stretching terlalu lama.

10-bit addressing:

- Memperluas jumlah address.
- Memakai format address 2 byte.
- Jarang digunakan pada modul sensor umum.
- STM32/ESP32 mendukung pada level tertentu, tetapi library sensor biasanya 7-bit.

---

## 20. Bus Ownership: Multi-Master dan Master-Slave

I2C mendukung multi-master, tetapi project praktikum harus mengatur kepemilikan bus agar tidak terjadi collision.

Pendekatan aman dual-MCU:

1. **Single active master**: ESP32 master utama; STM32 tidak mengakses bus sensor saat ESP32 aktif.
2. **Time-slot ownership**: ESP32 memberi izin ke STM32 lewat GPIO/UART.
3. **Bus request/grant**: STM32 minta akses; ESP32 menyelesaikan transaksi lalu melepas bus.
4. **Master-slave split**: ESP32 master pada bus sensor; STM32 menjadi slave I2C pada bus terpisah atau komunikasi via UART.
5. **Dual bus fisik**: sensor kritis dipisah untuk menghindari kontensi.

Pada Modul 07, multi STM32-ESP32 memakai kontrol ownership eksplisit: tidak ada dua master melakukan transaksi bersamaan.

---

## 21. Struktur Praktikum Final Modul 07

Praktikum final berisi tepat **25 eksperimen**:

### 10 Eksperimen ESP32 (GPIO21=SDA, GPIO22=SCL, 100 kHz, I2C_NUM_0)

| No | Kode | Topik | Sensor/Device | Alamat |
|---|---|---|---|---|
| 1 | ESP32_01_I2C_Bus_Scanner_Address_Map | I2C scanner, address map | Semua device | Scan 0x08-0x77 |
| 2 | ESP32_02_SSD1306_OLED_Display_Graphics | OLED graphics | SSD1306 | 0x3C/0x3D |
| 3 | ESP32_03_BME280_Environmental_Sensor | Sensor suhu/tekanan/lembap | BME280 | 0x76/0x77 |
| 4 | ESP32_04_MPU6050_IMU_Raw_And_Angle | IMU raw data, sudut | MPU6050 | 0x68/0x69 |
| 5 | ESP32_05_AT24C32_EEPROM_Data_Log | Data logging EEPROM | AT24C32 | 0x50-0x57 |
| 6 | ESP32_06_DS3231_RTC_Alarm_Temperature | RTC eksternal, alarm | DS3231 | 0x68 |
| 7 | ESP32_07_BH1750_Light_Adaptive_Display | Sensor cahaya, adaptive display | BH1750 | 0x23/0x5C |
| 8 | ESP32_08_I2C_RTOS_Multi_Task_Sensor | FreeRTOS multi-task sensor | BME280/BH1750 | 0x76/0x23 |
| 9 | ESP32_09_I2C_RTOS_Data_Logger | FreeRTOS data logger | EEPROM/Sensor | 0x50-0x57 |
| 10 | ESP32_10_I2C_RTOS_Interrupt_Driven | FreeRTOS interrupt-driven | Sensor+OLED | 0x76/0x3C |

### 10 Eksperimen STM32 (I2C1 PB6=SCL, PB7=SDA, HAL I2C)

| No | Kode | Topik | Sensor/Device | Alamat |
|---|---|---|---|---|
| 1 | STM32_01_I2C_HAL_Scanner_Address_Map | I2C HAL scanner | Semua device | Scan 0x08-0x77 |
| 2 | STM32_02_SSD1306_OLED_HAL_Driver | OLED HAL driver | SSD1306 | 0x3C/0x3D |
| 3 | STM32_03_BME280_Register_Driver | BME280 register-level | BME280 | 0x76/0x77 |
| 4 | STM32_04_MPU6050_IMU_Interrupt_Ready | IMU interrupt ready | MPU6050 | 0x68 |
| 5 | STM32_05_AT24C32_EEPROM_Page_Buffer | EEPROM page buffer | AT24C32 | 0x50-0x57 |
| 6 | STM32_06_DS3231_RTC_External_BCD | RTC eksternal BCD | DS3231 | 0x68 |
| 7 | STM32_07_Internal_RTC_Backup_Register | RTC internal, backup register | RTC internal | - |
| 8 | STM32_08_I2C_RTOS_Multi_Task | FreeRTOS multi-task | Sensor+Display | 0x76/0x3C |
| 9 | STM32_09_I2C_RTOS_DMA_Transfer | FreeRTOS DMA transfer | OLED/EEPROM | 0x3C/0x50 |
| 10 | STM32_10_I2C_RTOS_Error_Recovery | FreeRTOS error recovery | Sensor+Watchdog | 0x76/0x68 |

### 5 Eksperimen Multi STM32-ESP32

| No | Kode | Topik | Fokus |
|---|---|---|---|
| 1 | Multi_01_I2C_Master_Slave_Basic | Master-slave dasar | STM32 slave addr 0x42 |
| 2 | Multi_02_I2C_Role_Swap_Command | Role swap via command | Dynamic master-slave |
| 3 | Multi_03_I2C_Shared_Sensor | Shared sensor access | Bus arbitration |
| 4 | Multi_04_I2C_RTOS_Gateway | RTOS gateway | Data aggregation |
| 5 | Multi_05_I2C_RTOS_Weather_Station | Weather station integration | Project integration |

### GUI Monitor

- **GUI_I2C.py**: tkinter I2C monitor untuk visualisasi data sensor real-time, serial connection, grafik, dan simulasi I2C scanner.

Ringkasan fokus:

| Kelompok | Jumlah | Fokus |
|---|---:|---|
| ESP32 | 10 | ESP-IDF, GPIO21/22 SDA/SCL, 100kHz, SNTP/RTC, sensor/display, FreeRTOS |
| STM32 | 10 | HAL I2C1 PB6/7, DMA, internal RTC backup register, raw register, FreeRTOS error recovery |
| Multi STM32-ESP32 | 5 | bus ownership, master-slave (addr 0x42), role swap, shared sensor, project integration |

---

## 22. Integrasi Project Weather Station

Project akhir Modul 07 adalah weather station dual-MCU dengan monitor GUI.

Peran utama:

- **ESP32**: gateway, display OLED, SNTP, konfigurasi, agregasi data, error recovery.
- **STM32**: node akuisisi deterministik, internal RTC fallback, DMA/logging lokal, pembacaan raw register.
- **DS3231**: timestamp presisi.
- **EEPROM**: log circular.
- **BME280/BMP280**: cuaca; BME280 jika tersedia, BMP280 fallback.
- **BH1750**: cahaya.
- **MPU6050**: orientasi/guncangan.
- **SSD1306/LCD**: tampilan lokal.
- **GUI_I2C.py**: tkinter monitor untuk visualisasi data sensor real-time via serial.

Record log disarankan 16 byte:

| Field | Byte |
|---|---:|
| Unix timestamp | 4 |
| temperature x100 | 2 |
| pressure hPa x10 | 2 |
| humidity x100 atau 0xFFFF jika BMP280 | 2 |
| lux | 2 |
| accel magnitude x100 | 2 |
| status/error flags | 2 |

Kapasitas aman:

- AT24C32: maksimal 240 record + metadata.
- 24LC256: maksimal 2000 record + metadata.

---

## 23. Troubleshooting Checklist

1. Pastikan GND semua device tersambung.
2. Pastikan SDA/SCL tidak tertukar.
3. Jalankan I2C scanner sebelum driver sensor.
4. Cek address konflik: DS3231 0x68 vs MPU6050 0x68.
5. Set MPU6050 AD0=HIGH jika DS3231 dipakai.
6. Cek pull-up efektif dengan multimeter/logic analyzer.
7. Turunkan clock ke 100 kHz jika tidak stabil.
8. Untuk LCD 5 V, pastikan pull-up I2C tidak menarik ke 5 V langsung ke MCU 3.3 V.
9. Untuk EEPROM, tunggu write cycle atau ACK polling.
10. Jika bus stuck, lakukan 9 pulse SCL + STOP + reinit.
11. Jika BME280 tidak ada, fallback ke BMP280 dan tampilkan humidity `N/A`.
12. Untuk multi-master, pakai ownership; jangan akses bersamaan.

---

## 24. Best Practice Desain

Hardware:

- Gunakan 3.3 V untuk pull-up bus utama.
- Kabel pendek dan rapi.
- Pisahkan bus display OLED jika refresh tinggi mengganggu sensor.
- Tambahkan kapasitor decoupling dekat sensor.
- Hindari address conflict sejak skema awal.

Software:

- Selalu cek return code.
- Pakai timeout.
- Pisahkan driver sensor, display, storage, dan recovery.
- Gunakan state machine untuk device online/offline.
- Simpan metadata EEPROM: magic, version, write index, record count.
- Jangan blocking lama di loop utama.
- Log error count per device.

---

## 25. Referensi

1. NXP UM10204 — I2C-bus specification and user manual.
2. Espressif ESP-IDF Programming Guide — I2C, GPIO matrix, SNTP, system time.
3. STM32 Reference Manual dan HAL Driver Documentation — I2C, DMA, RTC.
4. Datasheet BME280, BMP280, SSD1306, DS3231, AT24C32, 24LC256, MPU6050, BH1750, PCF8574.
5. Application note terkait I2C pull-up sizing, bus recovery, dan multi-master arbitration.
