# Jobsheet Modul 07: Menguasai SPI Bus dan Storage — Data Persistent

## Praktikum Sistem Embedded

**Semester:** Genap 2025/2026  
**Durasi:** 3 × 50 menit (2 pertemuan)  
**Platform:** ESP32 DevKit V1 & STM32 Blue Pill (STM32F103C8T6)

---

## 1. Tujuan Praktikum

Setelah menyelesaikan praktikum ini, mahasiswa diharapkan mampu:

1. **Memahami protokol SPI** — Menjelaskan komunikasi SPI (MOSI, MISO, SCLK, CS), mode SPI (CPOL, CPHA), dan perbedaan dengan I2C.
2. **Mengakses perangkat SPI** — Menggunakan SPI untuk mengontrol OLED, membaca ADC eksternal, menulis DAC, dan berkomunikasi dengan flash memory.
3. **Mengimplementasikan penyimpanan data** — Menggunakan NVS, SPIFFS (ESP32), dan internal flash (STM32) untuk menyimpan data secara persistent.
4. **Mengoptimalkan performa SPI** — Mengukur throughput SPI pada berbagai kecepatan clock, membandingkan mode blocking vs interrupt.
5. **Membangun sistem data logging** — Menggabungkan sensor, storage, dan file system untuk membuat data logger yang reliable.

---

## 2. Peralatan dan Komponen

### 2.1 Perangkat Keras

| No | Komponen | Jumlah | Keterangan |
|----|----------|--------|------------|
| 1 | ESP32 DevKit V1 | 1 | 4× SPI, HSPI + VSPI |
| 2 | STM32 Blue Pill (STM32F103C8T6) | 1 | 2× SPI, SPI1 + SPI2 |
| 3 | ST-Link V2 | 1 | Programmer STM32 |
| 4 | OLED SSD1306 128×64 (SPI) | 1 | Display SPI 7-pin |
| 5 | W25Q32 Flash Module | 1 | SPI NOR Flash 4MB |
| 6 | Micro SD Card Module | 1 | SPI SD Card adapter |
| 7 | Micro SD Card | 1 | FAT32 formatted, ≤ 32GB |
| 8 | MCP3208 ADC Module (opsional) | 1 | 8-ch 12-bit SPI ADC |
| 9 | MCP4921 DAC Module (opsional) | 1 | 12-bit SPI DAC |
| 10 | Breadboard 830 titik | 1 | Papan rangkaian |
| 11 | Kabel jumper | 20 | Male-Male & Male-Female |
| 12 | Kabel USB Micro-B | 2 | ESP32 + ST-Link |

### 2.2 Perangkat Lunak

| No | Software | Keterangan |
|----|----------|------------|
| 1 | VS Code + PlatformIO | IDE pengembangan |
| 2 | Serial Monitor (115200 baud) | Output program |
| 3 | Python 3.x + matplotlib (opsional) | Analisis data |

---

## 3. Teori Singkat

### 3.1 Protokol SPI

SPI (Serial Peripheral Interface) adalah protokol komunikasi serial synchronous full-duplex menggunakan 4 jalur: **MOSI** (Master Out Slave In), **MISO** (Master In Slave Out), **SCLK** (Serial Clock), dan **CS/SS** (Chip Select, active low). SPI mendukung kecepatan tinggi (hingga puluhan MHz) dan transfer full-duplex. Pemilihan slave dilakukan melalui pin CS individual — satu CS per slave.

### 3.2 SPI pada ESP32 dan STM32

ESP32 memiliki 4 SPI controller (SPI0/SPI1 untuk flash internal, SPI2/HSPI dan SPI3/VSPI untuk umum). Pin HSPI default: MOSI=GPIO13, MISO=GPIO12, SCLK=GPIO14, CS=GPIO15. Pin VSPI default: MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18, CS=GPIO5.

STM32F103 memiliki 2 SPI (SPI1 di APB2 max 36MHz, SPI2 di APB1 max 18MHz). SPI1 default: SCK=PA5, MOSI=PA7, MISO=PA6, NSS=PA4.

### 3.3 Storage pada Embedded System

- **NVS (ESP32):** Key-value store di flash internal, mendukung int/string/blob
- **SPIFFS (ESP32):** File system di flash internal, mendukung file/directory
- **Internal Flash (STM32):** Akses langsung ke flash 64–128KB, page erase + halfword program

---

## 4. Langkah Percobaan

> **Catatan Umum:**
> - Serial Monitor: **115200 baud**
> - Setiap percobaan ada versi ESP32 dan STM32
> - SPI loopback: hubungkan MOSI ke MISO untuk self-test
> - Format SD card ke **FAT32** sebelum digunakan

---

### Percobaan 01: SPI Loopback — Self-Test

**Tujuan:** Memahami komunikasi SPI dasar dengan menguji pengiriman dan penerimaan data melalui koneksi loopback (MOSI → MISO).

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI | GPIO13 (HSPI) | PA7 |
| MISO | GPIO12 (HSPI) | PA6 |
| SCLK | GPIO14 | PA5 |
| CS | GPIO15 | PA4 (software) |

> Hubungkan MOSI langsung ke MISO (loopback) untuk self-test.

#### Langkah Kerja

1. Buka project `ESP32_01` atau `STM32_01`.
2. Pelajari inisialisasi SPI: clock speed, mode (CPOL=0, CPHA=0), bit order.
3. Perhatikan 5 test pattern yang dikirim (counter, alternating, all-0xFF, all-0x00, random).
4. Build dan upload. Amati hex dump TX vs RX untuk setiap pattern.
5. Verifikasi semua 5 test PASS.
6. Coba lepas kabel MOSI→MISO — amati apa yang terjadi.

#### Tabel Pengamatan

| No | Test Pattern | TX (hex) | RX (hex) | Pass/Fail |
|----|-------------|----------|----------|-----------|
| 1 | Counter | | | |
| 2 | Alternating | | | |
| 3 | All 0xFF | | | |
| 4 | All 0x00 | | | |
| 5 | Random | | | |

#### Pertanyaan Analisa

1. Mengapa SPI loopback test penting sebelum menghubungkan perangkat slave?
2. Apa arti CPOL dan CPHA? Berapa kombinasi mode SPI yang mungkin?
3. Apa yang terjadi saat MOSI tidak terhubung ke MISO? Mengapa nilainya demikian?
4. Bandingkan inisialisasi SPI pada ESP32 (`spi_bus_initialize`) vs STM32 (`HAL_SPI_Init`).

---

### Percobaan 02: SPI OLED SSD1306

**Tujuan:** Mengontrol display OLED 128×64 melalui SPI dan memahami perbedaan SPI OLED (command/data pin) dengan I2C OLED.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI (DIN/SDA) | GPIO23 | PA7 |
| SCLK (CLK) | GPIO18 | PA5 |
| CS | GPIO5 | PA4 |
| DC (Data/Command) | GPIO21 | PB0 |
| RST (Reset) | GPIO22 | PB1 |

#### Langkah Kerja

1. Hubungkan OLED SSD1306 SPI (7-pin) sesuai tabel.
2. Buka project `ESP32_02` atau `STM32_02`.
3. Pelajari init sequence SSD1306 (display off, set clock, multiplex ratio, display offset, start line, charge pump, segment remap, COM scan, COM pins, contrast, precharge, VCOMH, display on).
4. Perhatikan implementasi font 5×7 dan frame buffer.
5. Build dan upload. Amati OLED menampilkan teks dan counter.
6. Coba modifikasi teks yang ditampilkan.

#### Tabel Pengamatan

| No | Tampilan OLED | Serial Log | Catatan |
|----|-------------|------------|---------|
| 1 | Init | | |
| 2 | Teks baris 1 | | |
| 3 | Counter increment | | |
| 4 | Setelah modifikasi | | |

#### Pertanyaan Analisa

1. Apa fungsi pin DC pada SPI OLED? Bagaimana membedakan command vs data?
2. Mengapa SPI OLED lebih cepat dari I2C OLED? Berapa kecepatan refresh rate perkiraan?
3. Apa itu frame buffer? Mengapa tidak langsung kirim pixel ke OLED satu per satu?
4. Berapa ukuran frame buffer untuk OLED 128×64 monokrom?

---

### Percobaan 03: SPI Flash W25Q32

**Tujuan:** Mengakses SPI NOR Flash (W25Q32) untuk membaca JEDEC ID, erase sector, program page, dan memverifikasi data.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI (DI) | GPIO23 | PA7 |
| MISO (DO) | GPIO19 | PA6 |
| SCLK (CLK) | GPIO18 | PA5 |
| CS (/CS) | GPIO5 | PA4 |

#### Langkah Kerja

1. Hubungkan W25Q32 flash module.
2. Buka project `ESP32_03` atau `STM32_03`.
3. Pelajari command set W25Q: Read JEDEC ID (0x9F), Write Enable (0x06), Sector Erase (0x20), Page Program (0x02), Read Data (0x03), Read Status (0x05).
4. Build dan upload. Amati:
   - JEDEC ID: Manufacturer=0xEF (Winbond), Type=0x40, Capacity=0x16 (32Mbit)
   - Sequence: erase → write → read → verify
5. Verifikasi data match antara write dan readback.

#### Tabel Pengamatan

| No | Operasi | Data/ID | Waktu (ms) | Catatan |
|----|---------|---------|-----------|---------|
| 1 | Read JEDEC ID | | | |
| 2 | Sector Erase | | | |
| 3 | Page Program (256B) | | | |
| 4 | Read & Verify | | | |

#### Pertanyaan Analisa

1. Mengapa flash memory harus di-erase sebelum bisa ditulis? Apa artinya "erase sets all bits to 1"?
2. Apa perbedaan sector erase (4KB) dan block erase (64KB)? Kapan masing-masing digunakan?
3. Mengapa page program dibatasi 256 byte? Apa yang terjadi jika menulis lebih dari 256 byte sekaligus?
4. Berapa kapasitas total W25Q32? Berapa sector dan page yang ada?

---

### Percobaan 04: SPI SD Card

**Tujuan:** Mengakses SD Card melalui SPI untuk operasi file sistem (ESP32: FAT via VFS, STM32: raw block read/write).

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI (DI) | GPIO23 | PA7 |
| MISO (DO) | GPIO19 | PA6 |
| SCLK (CLK) | GPIO18 | PA5 |
| CS (CS) | GPIO5 | PA4 |

> Gunakan SD card module yang memiliki level shifter 3.3V.

#### Langkah Kerja

1. Format SD card ke FAT32.
2. Buka project `ESP32_04` atau `STM32_04`.
3. ESP32: Pelajari mount VFS FAT (`esp_vfs_fat_sdspi_mount`), operasi file (fopen/fwrite/fread), directory listing.
4. STM32: Pelajari bare-metal SD init (CMD0→CMD8→ACMD41→CMD58), raw block read/write.
5. Build dan upload. Amati:
   - ESP32: Card info, file write → read → verify, directory listing
   - STM32: Card type detection (SDSC/SDHC), raw block write → read → verify
6. Cabut SD card dan cek isi file via komputer (ESP32).

#### Tabel Pengamatan

| No | Operasi | Hasil | Ukuran/Block | Catatan |
|----|---------|-------|-------------|---------|
| 1 | Card init/detect | | | |
| 2 | Write file/block | | | |
| 3 | Read file/block | | | |
| 4 | Directory listing | | | |

#### Pertanyaan Analisa

1. Mengapa init SD card dimulai dengan kecepatan SPI rendah (400kHz) lalu beralih ke cepat?
2. Apa perbedaan SDSC dan SDHC dalam mode SPI? Bagaimana cara mendeteksi jenisnya?
3. Bandingkan pendekatan ESP32 (VFS FAT) vs STM32 (raw block) — mana yang lebih mudah digunakan?
4. Mengapa SD card harus di-unmount sebelum dicabut? Apa risiko jika langsung dicabut?

---

### Percobaan 05: SPI MCP3208 ADC Eksternal

**Tujuan:** Membaca ADC 12-bit 8-channel MCP3208 melalui SPI dan menampilkan pembacaan multi-channel dalam format tabel.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI (DIN) | GPIO23 | PA7 |
| MISO (DOUT) | GPIO19 | PA6 |
| SCLK (CLK) | GPIO18 | PA5 |
| CS (/CS) | GPIO5 | PA4 |
| MCP3208 VREF | 3.3V | 3.3V |
| MCP3208 CH0 | Potensiometer | Potensiometer |

> Hubungkan potensiometer ke CH0. Channel lain bisa dibiarkan floating atau dihubungkan ke sumber tegangan.

#### Langkah Kerja

1. Hubungkan MCP3208 sesuai tabel.
2. Buka project `ESP32_05` atau `STM32_05`.
3. Pelajari frame SPI 3-byte untuk MCP3208: start bit + channel selection + read result.
4. Build dan upload. Amati tabel 8-channel dengan raw value dan tegangan.
5. Putar potensiometer pada CH0 — amati nilai berubah.
6. Amati statistik (min, max, average) yang dikumpulkan.

#### Tabel Pengamatan

| CH | Raw (min pot) | V (min pot) | Raw (mid) | V (mid) | Raw (max pot) | V (max pot) |
|----|-------------|-----------|---------|-------|-------------|-----------|
| 0 | | | | | | |
| 1 | | | | | | |
| 2 | | | | | | |

#### Pertanyaan Analisa

1. Mengapa perlu 3 byte SPI untuk membaca MCP3208? Gambarkan diagram timing frame-nya.
2. Apa keuntungan menggunakan ADC eksternal (MCP3208) vs ADC internal ESP32/STM32?
3. Berapa resolusi dan accuracy MCP3208? Bandingkan dengan ADC internal ESP32 (12-bit).
4. Bagaimana cara memilih channel pada MCP3208 melalui bit konfigurasi di byte pertama?

---

### Percobaan 06: SPI DAC MCP4921

**Tujuan:** Menghasilkan waveform analog (sine, sawtooth, triangle, square) menggunakan DAC 12-bit MCP4921 melalui SPI.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI (SDI) | GPIO23 | PA7 |
| SCLK (SCK) | GPIO18 | PA5 |
| CS (/CS) | GPIO5 | PA4 |
| LDAC | GPIO21 | PB0 |
| MCP4921 VREF | 3.3V | 3.3V |

> Hubungkan osiloskop atau multimeter ke pin VOUT MCP4921.

#### Langkah Kerja

1. Hubungkan MCP4921 sesuai tabel.
2. Buka project `ESP32_06` atau `STM32_06`.
3. Pelajari format frame MCP4921: `[0x3000 | DAC_VALUE]` (16-bit, unbuffered, 1× gain, active).
4. Perhatikan implementasi sine LUT (Look-Up Table) — ESP32 menggunakan `sinf()`, STM32 menggunakan integer Bhaskara approximation.
5. Build dan upload. Amati waveform berganti setiap 5 detik: sine → sawtooth → triangle → square.
6. Ukur output VOUT dengan multimeter/osiloskop.

#### Tabel Pengamatan

| No | Waveform | DAC Min | DAC Max | V Min | V Max | Frekuensi Perkiraan |
|----|----------|---------|---------|-------|-------|---------------------|
| 1 | Sine | | | | | |
| 2 | Sawtooth | | | | | |
| 3 | Triangle | | | | | |
| 4 | Square | | | | | |

#### Pertanyaan Analisa

1. Mengapa STM32 menggunakan integer approximation untuk sine wave alih-alih `sinf()`?
2. Apa fungsi pin LDAC pada MCP4921? Apa bedanya dengan update langsung?
3. Berapa frekuensi waveform maximum yang dapat dihasilkan? Faktor apa yang membatasi?
4. Apa perbedaan resolusi output DAC MCP4921 (12-bit) dengan DAC internal ESP32 (8-bit)?

---

### Percobaan 07: SPI Multi-Slave

**Tujuan:** Mengoperasikan beberapa perangkat SPI pada bus yang sama menggunakan chip select (CS) independen dan mengukur timing transfer per slave.

#### Rangkaian

| Komponen | ESP32 | STM32 |
|----------|-------|-------|
| MOSI | GPIO23 | PA7 |
| MISO | GPIO19 | PA6 |
| SCLK | GPIO18 | PA5 |
| CS1 (Slave 1) | GPIO5 | PA4 |
| CS2 (Slave 2) | GPIO17 | PB0 |

> Hubungkan dua perangkat SPI (misal: flash + ADC) atau gunakan mode simulasi dalam program.

#### Langkah Kerja

1. Buka project `ESP32_07` atau `STM32_07`.
2. Pelajari konfigurasi multi-slave:
   - ESP32: `spi_bus_initialize()` sekali, `spi_bus_add_device()` dua kali dengan CS berbeda
   - STM32: SPI init sekali, toggle CS manual per slave
3. Build dan upload. Amati transfer ke masing-masing slave.
4. Perhatikan timing per transfer (μs) dan statistik per slave.
5. Amati demo back-to-back transfer (slave1 → slave2 berturut-turut).

#### Tabel Pengamatan

| No | Slave | Transfer # | TX (hex) | RX (hex) | Waktu (μs) |
|----|-------|-----------|----------|----------|-----------|
| 1 | Slave 1 | 1 | | | |
| 2 | Slave 1 | 2 | | | |
| 3 | Slave 2 | 1 | | | |
| 4 | Back-to-back | | | | |

#### Pertanyaan Analisa

1. Mengapa setiap slave memerlukan pin CS terpisah? Bisakah berbagi CS?
2. Apakah bus SPI bisa melayani dua slave secara simultan? Mengapa?
3. Apa overhead waktu CS select/deselect antara transfer ke slave berbeda?
4. Bandingkan multi-device SPI vs multi-device I2C — mana yang lebih efisien untuk banyak device?

---

### Percobaan 08: NVS Key-Value Store

**Tujuan:** Menyimpan data konfigurasi secara persistent menggunakan NVS (ESP32) atau Flash Key-Value Store (STM32) yang bertahan saat restart.

#### Rangkaian

Tidak memerlukan rangkaian tambahan — menggunakan flash internal. UART via USB/TTL converter.

#### Langkah Kerja

1. Buka project `ESP32_08` atau `STM32_08`.
2. ESP32: Pelajari NVS API — `nvs_open()`, `nvs_set_i32()`, `nvs_get_i32()`, `nvs_commit()`, `nvs_set_str()`, `nvs_set_blob()`.
3. STM32: Pelajari internal flash read/write — `HAL_FLASH_Unlock()`, `HAL_FLASHEx_Erase()`, `HAL_FLASH_Program()`.
4. Build dan upload. Amati boot counter yang bertambah setiap restart.
5. Reset board beberapa kali — verifikasi boot counter tetap naik.
6. Amati penyimpanan string, float, dan blob (struct) pada ESP32.
7. Pada STM32, amati calibration data struct yang disimpan dengan checksum.

#### Tabel Pengamatan

| No | Reset ke- | Boot Counter | String Stored | Blob Valid? | Catatan |
|----|-----------|-------------|---------------|-------------|---------|
| 1 | 1 | | | | |
| 2 | 2 | | | | |
| 3 | 3 | | | | |
| 4 | (erase NVS) | | | | |

#### Pertanyaan Analisa

1. Apa perbedaan NVS (ESP32) dengan flash key-value store manual (STM32)?
2. Mengapa flash perlu di-erase sebelum ditulis ulang? Apa granularity erase pada ESP32 vs STM32?
3. Apa itu wear leveling dan mengapa penting untuk flash storage?
4. Bagaimana cara memastikan integritas data (checksum/CRC) pada penyimpanan flash?

---

### Percobaan 09: SPIFFS File System

**Tujuan:** Menggunakan SPIFFS file system pada ESP32 untuk menyimpan file teks dan binary secara persistent (ESP32), atau Flash KV Store pada STM32.

#### Rangkaian

Tidak memerlukan rangkaian tambahan.

#### Langkah Kerja

1. Buka project `ESP32_09` atau `STM32_09`.
2. ESP32: Pelajari SPIFFS mount (`esp_vfs_spiffs_register`), file operations (`fopen/fwrite/fread/fclose`), directory listing, storage info.
3. STM32: Pelajari key-value store yang lebih lengkap — set/get int, float, string, delete, compact, list all.
4. Build dan upload. Amati:
   - ESP32: File create → write → read, binary records, directory listing, storage usage
   - STM32: KV store demo, compaction, key listing
5. Pada ESP32, cek storage usage (total/used/free).

#### Tabel Pengamatan

| No | Operasi | File/Key | Ukuran | Waktu (ms) | Catatan |
|----|---------|----------|--------|-----------|---------|
| 1 | Write text file | | | | |
| 2 | Read text file | | | | |
| 3 | Write binary struct | | | | |
| 4 | Directory listing | | | | |
| 5 | Storage usage | total=? used=? free=? | | | |

#### Pertanyaan Analisa

1. Apa perbedaan SPIFFS, LittleFS, dan FAT sebagai file system di embedded?
2. Mengapa SPIFFS tidak mendukung directory hierarchy?
3. Apa keuntungan menggunakan file system dibanding raw flash access?
4. Pada STM32, bagaimana kompaksi (compaction) bekerja untuk menangani entry yang dihapus?

---

### Percobaan 10: SPI Speed Benchmark

**Tujuan:** Mengukur throughput SPI aktual pada berbagai kecepatan clock dan ukuran buffer, serta menghitung efisiensi transfer.

#### Rangkaian

Sama dengan Percobaan 01 — SPI loopback (MOSI → MISO).

#### Langkah Kerja

1. Buka project `ESP32_10` atau `STM32_10`.
2. Pelajari variasi parameter yang diuji:
   - ESP32: 7 kecepatan (1–40 MHz), 5 ukuran buffer (16–4096 byte)
   - STM32: prescaler /2 hingga /256, beberapa ukuran buffer
3. Build dan upload. Amati tabel benchmark:
   - Kecepatan aktual (MHz)
   - Throughput (KB/s, Mbps)
   - Efisiensi vs kecepatan teoritis
4. Identifikasi konfigurasi dengan throughput terbaik dan efisiensi terbaik.

#### Tabel Pengamatan

| Clock (MHz) | Buffer (B) | Throughput (KB/s) | Efisiensi (%) | DMA? |
|------------|-----------|------------------|--------------|------|
| 1 | 256 | | | |
| 10 | 256 | | | |
| 20 | 256 | | | |
| 40 | 256 | | | |
| 10 | 16 | | | |
| 10 | 4096 | | | |

#### Pertanyaan Analisa

1. Apakah throughput aktual selalu sama dengan kecepatan clock × jumlah bit? Mengapa ada overhead?
2. Bagaimana ukuran buffer mempengaruhi throughput? Mengapa buffer besar lebih efisien?
3. Apa peran DMA dalam meningkatkan throughput SPI?
4. Bandingkan kecepatan SPI maksimum ESP32 vs STM32 — mana yang lebih cepat dan mengapa?

---

### Percobaan 11: SPI Interrupt Mode

**Tujuan:** Membandingkan performa SPI mode blocking (polling) vs non-blocking (interrupt) dan mengukur utilitas CPU.

#### Rangkaian

Sama dengan Percobaan 01 — SPI loopback (MOSI → MISO).

#### Langkah Kerja

1. Buka project `ESP32_11` atau `STM32_11`.
2. Pelajari perbedaan mode:
   - ESP32: `spi_device_transmit()` (blocking) vs `spi_device_queue_trans()` + `spi_device_get_trans_result()` (non-blocking/queued)
   - STM32: `HAL_SPI_Transmit()` (blocking) vs `HAL_SPI_Transmit_IT()` (interrupt)
3. Build dan upload. Amati:
   - Waktu transfer blocking vs interrupt
   - CPU work units yang dapat dilakukan selama transfer non-blocking
   - Callback invocations (pre/post transfer pada ESP32, TxCplt/Error pada STM32)
4. Catat perbandingan timing.

#### Tabel Pengamatan

| No | Mode | Transfer Size | Waktu (μs) | CPU Work Units | Catatan |
|----|------|-------------|-----------|---------------|---------|
| 1 | Blocking | 256B | | 0 (waiting) | |
| 2 | Interrupt | 256B | | | |
| 3 | Blocking | 1024B | | 0 | |
| 4 | Interrupt | 1024B | | | |

#### Pertanyaan Analisa

1. Apa keuntungan mode interrupt dibandingkan polling untuk SPI?
2. Berapa banyak "work" yang dapat dilakukan CPU saat SPI transfer berjalan di background?
3. Apa overhead interrupt handler? Apakah interrupt selalu lebih baik dari polling?
4. Kapan lebih baik menggunakan DMA dibanding interrupt untuk SPI transfer?

---

### Percobaan 12: Storage Data Logger

**Tujuan:** Membangun sistem data logger lengkap yang menyimpan data sensor ke storage secara periodik dengan format CSV dan manajemen file.

#### Rangkaian

Tidak memerlukan rangkaian tambahan (data sensor disimulasikan). Menggunakan SPIFFS (ESP32) atau internal flash (STM32).

#### Langkah Kerja

1. Buka project `ESP32_12` atau `STM32_12`.
2. ESP32: Pelajari arsitektur dual-task:
   - Collector task: membaca sensor simulasi, push ke ring buffer
   - Writer task: pop dari ring buffer, tulis ke file CSV di SPIFFS
   - File rotation saat ukuran tertentu tercapai
3. STM32: Pelajari circular page management di internal flash:
   - RAM buffer dengan periodic flush ke flash
   - Circular page management (8 halaman)
   - Entry counter dan page wrap handling
4. Build dan upload. Amati:
   - Data sensor yang di-log secara periodik
   - Storage statistics (total/used/free, entries, flushes)
5. Biarkan berjalan beberapa menit, kemudian periksa jumlah entry yang tersimpan.

#### Tabel Pengamatan

| No | Waktu | Entries | Storage Used | Flushes | Page Wraps | Catatan |
|----|-------|---------|-------------|---------|-----------|---------|
| 1 | 1 min | | | | | |
| 2 | 5 min | | | | | |
| 3 | 10 min | | | | | |

#### Pertanyaan Analisa

1. Mengapa digunakan ring buffer sebagai perantara antara sensor reading dan flash writing?
2. Apa itu file rotation dan circular page management? Mengapa diperlukan?
3. Berapa estimasi lifetime flash jika data ditulis setiap detik? (Hint: flash endurance ~100K erase cycles)
4. Bagaimana cara memastikan data tidak corrupt jika sistem mati saat sedang menulis?

---

## 5. Tabel Komparatif STM32 vs ESP32

| Aspek | STM32F103 | ESP32 |
|-------|-----------|-------|
| SPI Controller | 2× (SPI1 max 36MHz, SPI2 max 18MHz) | 4× (SPI2/HSPI, SPI3/VSPI untuk user) |
| Pin Mapping | Fixed alternate function | Flexible GPIO matrix |
| DMA Support | DMA1 Channel | SPI DMA built-in |
| Storage | Internal Flash 64-128KB | NVS + SPIFFS + FAT (partition) |
| File System | Manual (raw flash) | VFS (SPIFFS, FAT, LittleFS) |
| SD Card | Bare-metal SPI | VFS FAT auto-mount |
| Key-Value | Manual implementation | NVS built-in |

---

## 6. Referensi

1. STM32F103xx Reference Manual (RM0008) — Chapter 25: SPI, Flash Programming
2. ESP-IDF Programming Guide — SPI Master, NVS, SPIFFS, SD/MMC
3. W25Q32 Datasheet — Winbond Serial NOR Flash
4. SD Specifications Part 1 — Physical Layer Simplified Specification
5. Mastering STM32, Carmine Noviello — Chapter 16: SPI
6. Kolban's Book on ESP32 — SPI & Storage

---

*Jobsheet Modul 07 — SPI & Storage | Praktikum Sistem Embedded | 2025/2026*
