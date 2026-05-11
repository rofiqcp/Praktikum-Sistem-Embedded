# Project Modul 08: SPI Data Acquisition System

## Informasi Project

| Item | Keterangan |
|---|---|
| Modul | 08 — SPI Bus dan Komunikasi STM32-ESP32 |
| Struktur praktikum | 25 eksperimen: 7 STM32 SPI + 3 STM32 SPI RTOS + 7 ESP32 SPI + 3 ESP32 SPI RTOS + 3 Multi SPI non-RTOS + 2 Multi SPI RTOS |
| Platform | ESP32 DevKit + STM32F103/STM32F4 |
| Tema | Data Acquisition System dengan multi-device SPI |
| Durasi | 2 minggu |
| Output | Sistem hardware, source code, laporan, video demo |

---

## 1. Deskripsi Umum

Project Modul 08 menggabungkan seluruh materi SPI menjadi **Data Acquisition System dual-MCU**. ESP32 dan STM32 bekerja bersama untuk mengakuisisi data dari berbagai sensor, menampilkan data, menyimpan log ke Flash/SD, menghasilkan sinyal analog, dan memulihkan bus jika terjadi error.

Sistem wajib selaras dengan struktur final praktikum:

- **7 eksperimen STM32 SPI**: SPI Bus Scanner, W25Q64 Flash Read/Write, Micro SD Card File System, SSD1306 OLED SPI Graphics, MCP3008 ADC Multi-Channel, MCP4921 DAC Signal Generation, ADXL345 Accelerometer SPI.
- **3 eksperimen STM32 SPI RTOS**: SPI RTOS Multi Device, SPI RTOS DMA Transfer, SPI RTOS Data Logger.
- **7 eksperimen ESP32 SPI**: SPI Bus Master Configuration, W25Q64 Flash ID dan Memory Map, Micro SD Card SPI Mode, SSD1306 OLED SPI Display, MCP3008 ADC Read Potentiometer, MCP4921 DAC Waveform Output, ADXL345 SPI Acceleration.
- **3 eksperimen ESP32 SPI RTOS**: SPI RTOS Multi Task, SPI RTOS DMA Benchmark, SPI RTOS Interrupt Driven.
- **3 eksperimen Multi SPI non-RTOS**: SPI Master-Slave Basic, SPI Multi Slave CS Management, SPI Shared Bus Multi Device.
- **2 eksperimen Multi SPI RTOS**: SPI RTOS Gateway, SPI Data Acquisition System.

---

## 2. Skenario

Sebuah sistem akuisisi data industri membutuhkan pengukuran dan kontrol berbagai parameter:

- Pembacaan sensor analog (potentiometer, sensor tegangan) via MCP3008 ADC.
- Pengukuran akselerasi dan orientasi via ADXL345.
- Penyimpanan data ke W25Q64 Flash (log circular) dan Micro SD Card (file system).
- Penampilan data real-time pada SSD1306 OLED.
- Generasi sinyal kontrol via MCP4921 DAC.
- Komunikasi antar-MCU untuk agregasi data.
- Error recovery jika device SPI tidak responsif.

---

## 3. Arsitektur Sistem

```text
                    WiFi/UART to PC
                        │
                  ┌─────▼─────┐          
                  │   ESP32    │◄────────────────────────┐
                  │ Gateway    │                         │
                  └─────┬─────┘                         │
                        │ SPI Bus A (Master)             │
       ┌────────────────┼─────────────────┬──────────────┘
       │                │                 │
   W25Q64           SSD1306            MCP4921 DAC
   Flash             OLED                │
   CS=GPIO5          CS=GPIO5,DC=16     │
                                        │
               ┌────────────────────┐    │
               │      STM32         │────┘
               │ Sensor/Logger Node│
               └─────┬────────────┘
                     │ SPI Bus B (Slave/Master)
        ┌────────────┼─────────────┬─────────────┐
        │            │             │             │
   MCP3008 ADC   ADXL345      W25Q64 Flash   SSD1306
   CS=PA2         CS=PA0       CS=PA4        CS=PA3
```

ESP32 sebagai master utama pada Bus A, STM32 dapat berperan sebagai slave (kepada ESP32) atau master pada Bus B. Komunikasi antar-MCU melalui SPI master-slave (ESP32 master, STM32 slave) atau UART.

---

## 4. Peran Platform

### ESP32

- Gateway dan koordinator sistem.
- SPI master configuration dengan ESP-IDF (SPI2_HOST/HSPI atau SPI3_HOST/VSPI).
- MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18 sebagai default.
- ESP-IDF SPI master API (`spi_bus_initialize`, `spi_bus_add_device`).
- Akses W25Q64 Flash untuk log data (ESP32_02).
- Micro SD Card untuk file system penyimpanan besar (ESP32_03).
- SSD1306 OLED SPI display dashboard (ESP32_04).
- MCP3008 ADC read potentiometer/sensor (ESP32_05).
- MCP4921 DAC waveform generation (ESP32_06).
- ADXL345 accelerometer SPI (ESP32_07).
- RTOS multi-task SPI (ESP32_08).
- RTOS DMA benchmark (ESP32_09).
- RTOS interrupt-driven (ESP32_10).
- Komunikasi SPI dengan STM32 slave (MULTI_01).

### STM32

- Node sensor deterministik.
- STM32F103C8 / STM32F401CC / STM32F411CE.
- SPI1 PA5/PA6/PA7 SCK/MISO/MOSI, SPI2 PB13/PB14/PB15 (opsional).
- W25Q64 Flash Read/Write dengan register/HAL (STM32_02).
- Micro SD Card File System (STM32_03).
- SSD1306 OLED SPI via HAL (STM32_04).
- MCP3008 ADC multi-channel (STM32_05).
- MCP4921 DAC signal generation (STM32_06).
- ADXL345 accelerometer SPI (STM32_07).
- SPI RTOS multi device (STM32_08).
- SPI RTOS DMA transfer (STM32_09).
- SPI RTOS data logger (STM32_10).
- Dapat berperan sebagai SPI slave (MULTI_01).

---

## 5. Device SPI Wajib

| Device | Mode | Fungsi | Ketentuan |
|---|---:|---|---|
| W25Q64 Flash | 0 | Log data circular, file storage | STM32_02, ESP32_02; JEDEC ID=0xEF4017 |
| Micro SD Card | 0 | File system penyimpanan besar | STM32_03, ESP32_03; init 400 kHz |
| SSD1306 OLED | 0 | Display grafis 128×64 SPI | STM32_04, ESP32_04; CS, DC, RES pins |
| MCP3008 ADC | 0 | 10-bit 8-channel ADC | STM32_05, ESP32_05; 0-1023 range |
| MCP4921 DAC | 0 | 12-bit single DAC output | STM32_06, ESP32_06; 0-4095 range |
| ADXL345 | 3 | 3-axis accelerometer | STM32_07, ESP32_07; DEVID=0xE5 |

---

## 6. Format Record Log

Gunakan record tetap 16 byte agar mudah dihitung dan dibaca ulang.

| Offset | Field | Ukuran | Format |
|---:|---|---:|---|
| 0 | Timestamp (ms) | 4 byte | uint32, millis() |
| 4 | ADC Channel 0 | 2 byte | uint16, 0-1023 |
| 6 | ADC Channel 1 | 2 byte | uint16, 0-1023 |
| 8 | Accel X | 2 byte | int16, mg |
| 10 | Accel Y | 2 byte | int16, mg |
| 12 | Accel Z | 2 byte | int16, mg |
| 14 | DAC Output | 2 byte | uint16, 0-4095 |
| 16 | Status flags | 1 byte | bitfield device/error |
| 17 | Checksum | 1 byte | XOR/CRC8 sederhana |

Metadata Flash disimpan di awal W25Q64:

| Field | Ukuran | Catatan |
|---|---:|---|
| magic | 2 byte | contoh 0x4D08 |
| version | 1 byte | format record |
| record_size | 1 byte | 18 (atau 16 tanpa timestamp) |
| write_index | 4 byte | alamat Flash circular |
| record_count | 2 byte | jumlah record valid |
| reserved | 8 byte | ekspansi |

---

## 7. Kapasitas Penyimpanan

### W25Q64 Flash (8 MB = 8388608 byte)

- Metadata: 16 byte.
- Record size: 18 byte.
- Ruang record: 8388608 - 16 = 8388592 byte.
- Jumlah record: 8388592 / 18 = 466033 record mentah.
- Batas aman project: **maksimal 400000 record**.

### Micro SD Card (4 GB, 8 GB, 16 GB, 32 GB)

- Gunakan FAT32 file system.
- Simpan dalam file CSV: `datalog.csv`.
- Setiap baris CSV: timestamp,adc0,adc1,accel_x,accel_y,accel_z,dac,status.
- Kapasitas hampir tidak terbatas untuk praktikum (GB level).

---

## 8. Alur Operasi

### Startup

1. ESP32 dan STM32 boot.
2. Jalankan SPI scanner (baca ID Flash, detect device).
3. Init W25Q64: baca status, cek metadata.
4. Init SD Card: baca CSD, mount file system.
5. Init OLED: reset, sequence inisialisasi.
6. Init ADC, DAC, ADXL345.
7. Tampilkan device terdeteksi di serial/OLED.

### Normal Loop

1. Baca ADC periodik tiap 100-1000 ms.
2. Baca ADXL345 untuk akselerasi.
3. Validasi data range.
4. Update OLED SSD1306 dengan dashboard.
5. Buat record 16-18 byte.
6. Simpan ke W25Q64 Flash circular.
7. Simpan ke SD Card (setiap 10 record atau per menit).
8. Generate DAC output (misal: ramp mengikuti ADC).
9. Kirim ringkasan antar MCU via SPI slave/master.
10. Catat error count.

### Error Mode

1. Jika device tidak respons/timeout, tandai offline.
2. Sistem tetap berjalan dengan device lain.
3. Lakukan retry berkala.
4. Jika SPI bus error, lakukan reinit SPI.
5. Setelah recovery, scan ulang dan reinit device.

---

## 9. SPI Bus Management Multi-MCU

Jika ESP32 dan STM32 berada pada bus SPI yang sama, wajib ada kontrol CS.

Eksperimen MULTI_01: STM32 sebagai SPI slave, ESP32 sebagai master.

Skema komunikasi SPI master-slave:
```
ESP32 (Master)                    STM32 (Slave)
MOSI ─────────────────────────→ MOSI (jika ada data dari master)
MISO ←────────────────────────── MISO (data dari slave)
SCLK ─────────────────────────→ SCLK
CS   ─────────────────────────→ NSS/CS
```

Urutan komunikasi:
1. ESP32 set CS=LOW (select STM32 slave).
2. ESP32 kirim command byte (misal: 0x01=read sensor, 0x02=set config).
3. STM32 slave respon dengan data (misal: ADC value, Accel data).
4. ESP32 set CS=HIGH (deselect slave).

Alternatif: pisahkan bus SPI (Bus A untuk ESP32 master, Bus B untuk STM32 master) dan gunakan UART untuk komunikasi antar-MCU.

---

## 10. Mapping 25 Eksperimen ke Project

| Eksperimen | Kontribusi project |
|---|---|
| STM32_01, ESP32_01 | SPI scanner dan device detection |
| STM32_02, ESP32_02 | W25Q64 Flash log/file storage |
| STM32_03, ESP32_03 | Micro SD Card file system |
| STM32_04, ESP32_04 | SSD1306 OLED real-time dashboard |
| STM32_05, ESP32_05 | MCP3008 ADC pembacaan sensor |
| STM32_06, ESP32_06 | MCP4921 DAC generasi sinyal |
| STM32_07, ESP32_07 | ADXL345 accelerometer |
| STM32_08, ESP32_08 | SPI RTOS multi-device management |
| STM32_09, ESP32_09 | SPI DMA transfer benchmark |
| STM32_10, ESP32_10 | SPI RTOS data logger |
| MULTI_01 | SPI master-slave basic (STM32 slave) |
| MULTI_02 | SPI multi slave CS management |
| MULTI_03 | SPI shared bus multi device |
| MULTI_04 | SPI RTOS gateway |
| MULTI_05 | SPI Data Acquisition System final integration |

---

## 11. Fitur Minimal Wajib

1. Startup scanner menampilkan semua device SPI (STM32_01, ESP32_01).
2. W25Q64 Flash berjalan: read ID, read/write/verify (STM32_02, ESP32_02).
3. Micro SD Card terbaca dan dapat baca/tulis blok (STM32_03, ESP32_03).
4. SSD1306 OLED menampilkan dashboard real-time (STM32_04, ESP32_04).
5. MCP3008 ADC membaca minimal 2 channel (STM32_05, ESP32_05).
6. MCP4921 DAC menghasilkan waveform terukur (STM32_06, ESP32_06).
7. ADXL345 membaca akselerasi 3-axis (STM32_07, ESP32_07).
8. STM32 dan ESP32 menggunakan RTOS untuk multi-task SPI (STM32_08/09/10, ESP32_08/09/10).
9. Multi-MCU berkomunikasi via SPI master-slave atau RTOS gateway (MULTI_01/04).
10. Error recovery dan logging berjalan otomatis.

---

## 12. Output Tampilan Minimum

### OLED

```text
MODUL 08 SPI
ADC0: 512  ADC1: 256
ACC: X=100 Y=-50 Z=980
DAC: 2048   LOG: 1234
```

### Serial Log

```text
SCAN: W25Q64 ID=EF4017, SD Card 4GB, OLED OK, ADC OK, DAC OK, ADXL DEVID=E5
REC: ts=1000000,adc0=512,adc1=256,ax=100,ay=-50,az=980,dac=2048,flags=0x00
```

---

## 13. Rubrik Penilaian

| Komponen | Bobot |
|---|---:|
| SPI Flash W25Q64 read/write/verify | 15% |
| Micro SD Card SPI mode dan file system | 15% |
| SSD1306 OLED SPI real-time dashboard | 10% |
| MCP3008 ADC multi-channel (STM32/ESP32) | 15% |
| MCP4921 DAC waveform generation | 10% |
| ADXL345 accelerometer SPI (mode 3) | 10% |
| STM32/ESP32 SPI RTOS multi-task | 10% |
| Multi-MCU SPI master-slave/gateway | 10% |
| Error recovery dan logging | 5% |
| Kode modular, laporan, video | 5% |

---

## 14. Struktur Kode Disarankan

```text
project_modul_08/
├─ esp32/
│  ├─ spi_bus.c
│  ├─ w25q64_flash.c
│  ├─ sd_card.c
│  ├─ display_oled.c
│  ├─ adc_mcp3008.c
│  ├─ dac_mcp4921.c
│  ├─ accel_adxl345.c
│  └─ spi_recovery.c
├─ stm32/
│  ├─ spi_hal.c
│  ├─ flash_w25q64.c
│  ├─ sdcard_spi.c
│  ├─ oled_ssd1306_spi.c
│  ├─ adc_mcp3008.c
│  ├─ dac_mcp4921.c
│  ├─ accel_adxl345.c
│  └─ dma_benchmark.c
└─ shared/
    ├─ record_format.h
    └─ protocol_spi.md
```

Dokumen baru tidak wajib dibuat; struktur ini hanya panduan implementasi.

---

## 15. Checklist Demo

- [ ] Menunjukkan hardware ESP32 + STM32.
- [ ] Menunjukkan MOSI, MISO, SCLK, CS pins.
- [ ] Scanner menemukan semua device SPI.
- [ ] Menunjukkan W25Q64 Flash read/write berhasil.
- [ ] Menunjukkan SD Card terbaca dan file system berfungsi.
- [ ] Menunjukkan OLED SSD1306 update real-time.
- [ ] Menunjukkan ADC MCP3008 berubah saat potentiometer diputar.
- [ ] Menunjukkan DAC MCP4921 menghasilkan waveform.
- [ ] Menunjukkan ADXL345 berubah saat digerakkan.
- [ ] Menunjukkan log data ke Flash/SD.
- [ ] Menunjukkan komunikasi ESP32-STM32 SPI.
- [ ] Menunjukkan error recovery atau reconnect.

---

## 16. Catatan Keselamatan dan Keandalan

- Pastikan level logika semua device sama (3.3V) atau gunakan level shifter jika berbeda.
- Jangan kutak-katik CS pin saat sedang transaksi SPI.
- Jangan melewati page boundary Flash saat page program (256 byte).
- Jangan menggunakan clock terlalu tinggi saat init (mulai dari 1 MHz, naik bertahap).
- SD Card butuh init 400 kHz, jangan langsung 10+ MHz.
- Selalu gunakan timeout dan cek return code.
- CS harus aktif LOW saat transaksi, HIGH saat idle.
