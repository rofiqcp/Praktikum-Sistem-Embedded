# Modul 08: SPI Bus dan Komunikasi STM32-ESP32

## Capaian Pembelajaran

Setelah menyelesaikan Modul 08, mahasiswa mampu:

1. Menjelaskan bus SPI: MOSI, MISO, SCLK, CS/SS, full-duplex, half-duplex, simplex, CPOL, CPHA, daisy-chain, dan multi-slave configuration.
2. Mengonfigurasi SPI pada ESP32 dengan ESP-IDF, termasuk GPIO matrix, multiple buses, SPI master/slave mode, dan DMA.
3. Mengonfigurasi SPI pada STM32 dengan HAL, register, polling, interrupt, DMA, multi-slave, dan error handling.
4. Mengintegrasikan device SPI: W25Q64 Flash, Micro SD Card, SSD1306 OLED, MCP3008 ADC, MCP4921 DAC, dan ADXL345 Accelerometer.
5. Mendesain sistem multi-device SPI, master-slave communication, error recovery, troubleshooting, dan integrasi project Data Acquisition System dual-MCU.

---

## 1. Dasar Bus SPI

SPI (Serial Peripheral Interface) adalah protokol komunikasi serial synchronous 4 jalur untuk komunikasi berkecepatan tinggi antar-chip jarak pendek. Jalur utama:

- **MOSI (Master Out Slave In)**: data dari master ke slave.
- **MISO (Master In Slave Out)**: data dari slave ke master.
- **SCLK (Serial Clock)**: clock dari master ke semua slave.
- **CS/SS (Chip Select/Slave Select)**: aktif LOW untuk memilih slave tertentu.

Topologi bus:

```text
Master ESP32/STM32
  │
  ├─ SCLK ──────────────────────────┐
  ├─ MOSI ──────────────────────────┤
  ├─ MISO ──────────────────────────┤
  │
  ├─ CS0 ──→ W25Q64 Flash
  ├─ CS1 ──→ SSD1306 OLED
  ├─ CS2 ──→ MCP3008 ADC
  ├─ CS3 ──→ MCP4921 DAC
  └─ CS4 ──→ ADXL345
```

Karakteristik utama:

| Aspek | SPI |
|---|---|
| Jalur | MOSI, MISO, SCLK, CS per slave |
| Mode | Full-duplex (biasanya) |
| Topologi | Master-Slave dengan multi-slave |
| Kecepatan | 10+ MHz umum, hingga 50+ MHz |
| Output | Push-pull (tidak perlu pull-up) |
| Chip Select | Aktif LOW, satu per slave |

---

## 2. Clock Polarity dan Clock Phase (CPOL/CPHA)

SPI menggunakan dua bit konfigurasi untuk menentukan timing data:

- **CPOL (Clock Polarity)**: menentukan level SCLK saat idle.
  - CPOL=0: SCLK idle LOW.
  - CPOL=1: SCLK idle HIGH.

- **CPHA (Clock Phase)**: menentukan kapan data disample.
  - CPHA=0: data disample pada rising edge (transisi LOW→HIGH).
  - CPHA=1: data disample pada falling edge (transisi HIGH→LOW).

---

## 3. SPI Modes

Kombinasi CPOL dan CPHA menghasilkan 4 mode SPI:

| Mode | CPOL | CPHA | Deskripsi | Device Umum |
|---|---:|---:|---|---|
| 0 | 0 | 0 | SCLK idle LOW, sample rising edge | W25Q64, SD Card, OLED |
| 1 | 0 | 1 | SCLK idle LOW, sample falling edge | Beberapa sensor |
| 2 | 1 | 0 | SCLK idle HIGH, sample falling edge | Beberapa device |
| 3 | 1 | 1 | SCLK idle HIGH, sample rising edge | ADXL345 |

Mode 0 dan 3 adalah yang paling umum. Pastikan master dan slave menggunakan mode yang sama.

---

## 4. Full-Duplex, Half-Duplex, dan Simplex

- **Full-Duplex**: data dapat ditransfer secara bersamaan MOSI dan MISO. Satu byte dikirim master→slave, slave→master secara simultan.
- **Half-Duplex**: data hanya satu arah pada satu waktu. Bisa menggunakan MOSI saja atau MISO saja.
- **Simplex**: satu arah tetap (hanya transmit atau hanya receive).

SPI umumnya beroperasi dalam full-duplex, di mana setiap transaksi melibatkan pertukaran byte:
- Master kirim byte → Slave terima di MOSI, Slave kirim byte → Master terima di MISO.

---

## 5. Chip Select (CS/SS) dan Multi-Slave

Setiap slave SPI membutuhkan pin CS/SS sendiri dari master.

Aturan CS:
- CS aktif LOW: slave dipilih saat CS=LOW, dinonaktifkan saat CS=HIGH.
- Hanya satu slave yang aktif pada satu waktu (kecuali daisy-chain).
- Saat CS=HIGH, slave mengabaikan SCLK dan mematikan output MISO (high-impedance).

Multi-slave pada satu bus SPI:
```text
Master
  │
  ├─ SCLK ─────────────────────┐
  ├─ MOSI ─────────────────────┤
  ├─ MISO ─────────────────────┤
  │
  ├─ CS0 ──→ Slave 0
  ├─ CS1 ──→ Slave 1
  └─ CS2 ──→ Slave 2
```

Untuk akses Slave 0: CS0=LOW, CS1=HIGH, CS2=HIGH.
Untuk akses Slave 1: CS0=HIGH, CS1=LOW, CS2=HIGH.

---

## 6. Daisy-Chain SPI

Daisy-chain menghubungkan multiple slave secara seri: MOSI→Slave0→Slave1→Slave2→MISO.

```text
Master MOSI → Slave0 MOSI → Slave0 MISO → Slave1 MOSI → Slave1 MISO → Slave2 MOSI → Slave2 MISO → Master MISO
```

Semua slave menggunakan CS yang sama. Data ditransfer bergeser melalui chain.

Keuntungan: hemat pin CS.
Kekurangan: latensi lebih tinggi, kompleksitas software.

---

## 7. Daftar Device Modul 08

| Device | Mode | Fungsi | Catatan |
|---|---:|---|---|
| W25Q64 Flash | 0 | 8 MB SPI Flash memory | JEDEC ID: 0xEF4017 |
| Micro SD Card | 0 | Penyimpanan massal | Init 400 kHz, lalu naik ke 10+ MHz |
| SSD1306 OLED 128×64 | 0 | Display grafis SPI | CS, DC, RES pins |
| MCP3008 | 0 | 10-bit 8-channel ADC | Single-ended/differential |
| MCP4921 | 0 | 12-bit single DAC | Output 0-4.096V (Vref=4.096V) |
| ADXL345 | 3 | 3-axis accelerometer | ±2g/±4g/±8g/±16g |

---

## 8. W25Q64 SPI Flash

W25Q64 adalah SPI Flash memory 8 MB (64 Mbit).

Fitur:
- 256-byte pages.
- 4 KB sectors, 32/64 KB blocks.
- Write Enable (0x06) diperlukan sebelum Write/Erase.
- Read JEDEC ID (0x9F): Manufacturer=0xEF, Type=0x40, Capacity=0x17.

Register penting:
- Status Register 1 (0x05): BUSY, WEL bits.
- Write Status Register (0x01).
- Read Data (0x03): baca data dari alamat.
- Page Program (0x02): tulis hingga 256 byte.
- Sector Erase (0x20): hapus 4 KB.
- Chip Erase (0xC7): hapus seluruh chip.

Operasi tulis:
1. Write Enable (0x06).
2. Tunggu WEL=1 (bit 1 di Status Register).
3. Page Program dengan alamat dan data.
4. Tunggu BUSY=0 (bit 0 di Status Register).

---

## 9. Micro SD Card

SD Card dapat diakses dengan SPI mode (alternatif dari SD Bus 4-bit).

Langkah inisialisasi SPI:
1. Clock 400 kHz (wajib untuk init).
2. CMD0 (GO_IDLE_STATE): reset SD card ke SPI mode.
3. CMD8 (SEND_IF_COND): cek voltage range (0x1AA).
4. ACMD41 (SD_SEND_OP_COND): polling hingga card ready (bit 31=1).
5. CMD58 (READ_OCR): baca Operating Conditions Register.
6. CMD16 (SET_BLOCKLEN): set block length = 512 byte.
7. Naikkan clock ke 10+ MHz.

Command umum:
- CMD17 (READ_SINGLE_BLOCK): baca 512 byte.
- CMD24 (WRITE_SINGLE_BLOCK): tulis 512 byte.
- CMD55 (APP_CMD): prefix untuk ACMD.
- ACMD41: inisialisasi.

Gunakan FAT16/FAT32 filesystem (FatFS library) untuk manajemen file.

---

## 10. SSD1306 OLED SPI

SSD1306 OLED 128×64 dengan interface SPI butuh 3 kontrol pins selain SPI: CS, DC (Data/Command), RES (Reset).

Inisialisasi:
1. RES LOW → delay → RES HIGH (hardware reset).
2. Kirim sequence inisialisasi via SPI (set display off, set clock, set contrast, dll).
3. Set display on.

Perbedaan I2C vs SPI:
- I2C: hanya SDA, SCL, address 0x3C/0x3D.
- SPI: MOSI, SCLK, CS, DC, RES; lebih cepat refresh.

Framebuffer: 128×64/8 = 1024 byte.
CS aktif LOW saat kirim command/data.
DC=LOW: command; DC=HIGH: data (display RAM).

---

## 11. MCP3008 ADC

MCP3008 adalah 10-bit 8-channel ADC dengan interface SPI.

Konfigurasi pembacaan:
1. CS LOW.
2. Start bit (1): bit pertama.
3. Single-ended/differential (S/D bit): 1=single, 0=differential.
4. Channel select (D2, D1, D0): 000-111 untuk channel 0-7.
5. Baca 10-bit hasil (MSB first).

Format byte untuk channel 0 single-ended:
```text
Byte1: 00000001 (start bit + S/D=1)
Byte2: 10000000 (D2=1? No, D2D1D0=000 for ch0, so 0x80? Wait)
Actually: Start bit (1) + S/D (1) + D2 (0) = 3 bits. Then D1, D0.
For channel 0: 1 1 0 0 0 0 0 0 = 0x80? Let's simplify.
```

Sederhana: kirim 1 byte konfigurasi, baca 2 byte hasil, gabungkan 10-bit.

Vref biasanya 3.3V atau 5V.
Nilai ADC: 0-1023 → Tegangan = (ADC/1023) × Vref.

---

## 12. MCP4921 DAC

MCP4921 adalah 12-bit single DAC dengan interface SPI.

Format data (16-bit):
```text
Bit 15: 0 = Write to DAC A
Bit 14: 1 = Buffered (Vref input buffered)
Bit 13: 1 = Gain 1x, 0 = Gain 2x
Bit 12: 1 = Output enable, 0 = Output disable
Bit 11-0: 12-bit DAC data
```

Contoh: output setengah (2048/4096): 0x7FF.
Vout = (DAC_data / 4096) × Vref × Gain.

Vref=4.096V, Gain=1x → Vout = (DAC_data / 4096) × 4.096V.
Vref=3.3V, Gunakan Vref sesuai hardware.

---

## 13. ADXL345 Accelerometer

ADXL345 adalah 3-axis accelerometer dengan interface SPI/I2C (pilih mode dengan pin CS).

Mode SPI: CS=LOW untuk SPI mode (jika CS=HIGH, mode I2C).
SPI Mode: 3 (CPOL=1, CPHA=1).

Register penting:
- DEVID (0x00): 0xE5 (fixed device ID).
- POWER_CTL (0x2D): bit 3=1 (measure mode).
- DATA_FORMAT (0x31): bit 1-0 untuk range (0=±2g, 1=±4g, 2=±8g, 3=±16g).
- DATAX0, DATAX1 (0x32-0x33): X-axis data.
- DATAY0, DATAY1 (0x34-0x35): Y-axis data.
- DATAZ0, DATAZ1 (0x36-0x37): Z-axis data.

Data format: 16-bit two's complement.
Nilai g = (raw_data / 256) × range (misal ±2g → range=2).

---

## 14. STM32 SPI HAL

STM32 memiliki beberapa peripheral SPI (SPI1, SPI2, SPI3) tergantung seri.

Konfigurasi HAL:
```c
hspi.Instance = SPI1;
hspi.Init.Mode = SPI_MODE_MASTER;
hspi.Init.Direction = SPI_DIRECTION_2LINES; // full-duplex
hspi.Init.DataSize = SPI_DATASIZE_8BIT;
hspi.Init.CLKPolarity = SPI_POLARITY_LOW; // CPOL=0
hspi.Init.CLKPhase = SPI_PHASE_1EDGE; // CPHA=0 (mode 0)
hspi.Init.NSS = SPI_NSS_SOFT; // software CS
hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // clock speed
hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
HAL_SPI_Init(&hspi);
```

Operasi penting:
```c
// Transmit
HAL_SPI_Transmit(&hspi, tx_data, size, timeout);

// Receive
HAL_SPI_Receive(&hspi, rx_data, size, timeout);

// Transmit and Receive (full-duplex)
HAL_SPI_TransmitReceive(&hspi, tx_data, rx_data, size, timeout);

// DMA Transmit
HAL_SPI_Transmit_DMA(&hspi, tx_data, size);

// DMA Receive
HAL_SPI_Receive_DMA(&hspi, rx_data, size);
```

Callback:
- `HAL_SPI_TxCpltCallback()`
- `HAL_SPI_RxCpltCallback()`
- `HAL_SPI_TxRxCpltCallback()`

---

## 15. STM32 SPI Registers

Untuk pemahaman mendalam, akses register SPI langsung:

Register utama:
- **SPIx_CR1**: konfigurasi CPOL, CPHA, baud rate, master/slave, dll.
- **SPIx_CR2**: konfigurasi interrupt, DMA, frame format.
- **SPIx_SR**: status register (TXE, RXNE, BSY flags).
- **SPIx_DR**: data register (transmit/receive).

Contoh transmit polling:
```c
// Wait TXE=1 (transmit buffer empty)
while (!(SPI1->SR & SPI_SR_TXE));

// Send data
SPI1->DR = data;

// Wait RXNE=1 (receive buffer not empty)
while (!(SPI1->SR & SPI_SR_RXNE));

// Read received data (to clear RXNE)
uint8_t rx = SPI1->DR;
```

---

## 16. ESP32 ESP-IDF SPI

ESP32 memiliki 3 controller SPI (SPI0, SPI1, SPI2/HSPI, SPI3/VSPI).

Konfigurasi ESP-IDF:
```c
spi_bus_config_t buscfg = {
    .mosi_io_num = 23,
    .miso_io_num = 19,
    .sclk_io_num = 18,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1
};

spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
    .mode = 0, // SPI mode 0
    .spics_io_num = 5,
    .queue_size = 7
};

spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
spi_bus_add_device(SPI2_HOST, &devcfg, &spi_device);
```

Transaksi SPI:
```c
spi_transaction_t t;
memset(&t, 0, sizeof(t));
t.length = 8 * size; // in bits
t.tx_buffer = tx_data;
t.rx_buffer = rx_data;

spi_device_transmit(spi_device, &t);
```

GPIO matrix ESP32 memungkinkan pin SPI dipindah ke GPIO lain.

---

## 17. SPI with FreeRTOS

SPI dengan FreeRTOS membutuhkan mekanisme sinkronisasi untuk akses shared bus.

Mutex untuk SPI bus:
```c
SemaphoreHandle_t spi_mutex;

// Init
spi_mutex = xSemaphoreCreateMutex();

// Akses SPI
if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) {
    // Lakukan transaksi SPI
    spi_device_transmit(spi_device, &t);
    xSemaphoreGive(spi_mutex);
}
```

Queue untuk kirim data antar task:
```c
QueueHandle_t spi_queue;
spi_queue = xQueueCreate(10, sizeof(spi_message_t));
```

Task ESP32/STM32 dapat melakukan:
- Task baca sensor (ADC, Accel) → kirim ke queue.
- Task update display → terima dari queue.
- Task logger → simpan data ke Flash/SD.

---

## 18. Error Handling dan Bus Recovery

Masalah umum SPI:

| Gejala | Penyebab | Solusi |
|---|---|---|
| Tidak ada respon slave | CS salah, mode salah, wiring salah | cek CS, mode, wiring |
| Data korup | Clock terlalu tinggi, noise | turunkan speed, periksa kabel |
| Timeout | Slave tidak respond, CS tidak aktif | cek pin CS, reset slave |
| Data tidak sinkron | CPOL/CPHA salah | pastikan master-slave sama |
| Flash BUSY lama | Write/erase belum selesai | polling status register |

Recovery SPI:
1. Deinit SPI peripheral.
2. Re-init SPI dengan konfigurasi benar.
3. Reset device jika ada pin RES.
4. Lakukan dummy read/write untuk sync.
5. Cek status register device.

---

## 19. SPI vs I2C Comparison

| Aspek | SPI | I2C |
|---|---|---|
| Jalur | 4+ (MOSI, MISO, SCLK, CSx) | 2 (SDA, SCL) |
| Speed | 10+ MHz | 100 kHz - 3.4 MHz |
| Mode | Full-duplex umum | Half-duplex |
| Addressing | CS pin (hardware) | 7/10-bit address (software) |
| Pull-up | Tidak perlu (push-pull) | Perlu (open-drain) |
| Multi-slave | CS per slave | Address per slave |
| Complexity | Lebih sederhana | Lebih kompleks |
| Distance | Pendek | Pendek |

SPI lebih cepat tetapi butuh lebih banyak pin.
I2C lebih hemat pin tetapi lebih lambat.

---

## 20. Struktur Praktikum Final Modul 08

Praktikum final berisi tepat **25 eksperimen**:

### 7 Eksperimen STM32 SPI (SPI1 PA5=SCK, PA6=MISO, PA7=MOSI)

| No | Kode | Topik | Device | Mode |
|---|---|---|---|---|
| 1 | STM32_01 | SPI Bus Scanner | W25Q64, OLED, ADC, DAC, ADXL | 0/3 |
| 2 | STM32_02 | W25Q64 Flash Read/Write | W25Q64 Flash | 0 |
| 3 | STM32_03 | Micro SD Card File System | Micro SD Card | 0 |
| 4 | STM32_04 | SSD1306 OLED SPI Graphics | SSD1306 OLED | 0 |
| 5 | STM32_05 | MCP3008 ADC Multi-Channel | MCP3008 ADC | 0 |
| 6 | STM32_06 | MCP4921 DAC Signal Generation | MCP4921 DAC | 0 |
| 7 | STM32_07 | ADXL345 Accelerometer SPI | ADXL345 | 3 |

### 3 Eksperimen STM32 SPI RTOS

| No | Kode | Topik | Fokus |
|---|---|---|---|
| 8 | STM32_08 | SPI RTOS Multi Device | FreeRTOS multi-task SPI |
| 9 | STM32_09 | SPI RTOS DMA Transfer | DMA + FreeRTOS |
| 10 | STM32_10 | SPI RTOS Data Logger | FreeRTOS logging ke Flash |

### 7 Eksperimen ESP32 SPI (MOSI=GPIO23, MISO=GPIO19, SCLK=GPIO18)

| No | Kode | Topik | Device | Mode |
|---|---|---|---|---|
| 11 | ESP32_01 | SPI Bus Master Configuration | W25Q64 Flash | 0 |
| 12 | ESP32_02 | W25Q64 Flash ID dan Memory Map | W25Q64 Flash | 0 |
| 13 | ESP32_03 | Micro SD Card SPI Mode | Micro SD Card | 0 |
| 14 | ESP32_04 | SSD1306 OLED SPI Display | SSD1306 OLED | 0 |
| 15 | ESP32_05 | MCP3008 ADC Read Potentiometer | MCP3008 ADC | 0 |
| 16 | ESP32_06 | MCP4921 DAC Waveform Output | MCP4921 DAC | 0 |
| 17 | ESP32_07 | ADXL345 SPI Acceleration | ADXL345 | 3 |

### 3 Eksperimen ESP32 SPI RTOS

| No | Kode | Topik | Fokus |
|---|---|---|---|
| 18 | ESP32_08 | SPI RTOS Multi Task | FreeRTOS multi-task |
| 19 | ESP32_09 | SPI RTOS DMA Benchmark | DMA benchmark |
| 20 | ESP32_10 | SPI RTOS Interrupt Driven | Interrupt-driven SPI |

### 3 Eksperimen Multi STM32-ESP32 SPI (non-RTOS)

| No | Kode | Topik | Fokus |
|---|---|---|---|
| 21 | MULTI_01 | SPI Master-Slave Basic | STM32 slave, ESP32 master |
| 22 | MULTI_02 | SPI Multi Slave CS Management | Multi-slave CS |
| 23 | MULTI_03 | SPI Shared Bus Multi Device | Shared bus |

### 2 Eksperimen Multi STM32-ESP32 SPI RTOS

| No | Kode | Topik | Fokus |
|---|---|---|---|
| 24 | MULTI_04 | SPI RTOS Gateway | RTOS data aggregation |
| 25 | MULTI_05 | SPI Data Acquisition System | Project integration |

Ringkasan fokus:

| Kelompok | Jumlah | Fokus |
|---|---:|---|
| STM32 SPI | 7 | STM32 HAL/register, SPI1, Flash, SD, OLED, ADC, DAC, Accel |
| STM32 SPI RTOS | 3 | FreeRTOS, DMA, multi-task, logging |
| ESP32 SPI | 7 | ESP-IDF, GPIO matrix, Flash, SD, OLED, ADC, DAC, Accel |
| ESP32 SPI RTOS | 3 | FreeRTOS, DMA, interrupt-driven |
| Multi STM32-ESP32 SPI | 5 | Master-slave, CS management, shared bus, RTOS gateway, project |

---

## 21. Integrasi Project Data Acquisition System

Project akhir Modul 08 adalah Data Acquisition System dual-MCU dengan SPI.

Peran utama:

- **ESP32**: gateway, display OLED, konfigurasi, agregasi data, error recovery, SD Card logging.
- **STM32**: node akuisisi deterministik, SPI register-level, DMA, real-time sampling, ADC/Accel reading.
- **W25Q64 Flash**: penyimpanan log data.
- **Micro SD Card**: penyimpanan file system besar.
- **SSD1306 OLED**: tampilan lokal data.
- **MCP3008 ADC**: pembacaan sensor analog/potentiometer.
- **MCP4921 DAC**: generasi sinyal analog/simulasi output.
- **ADXL345**: deteksi orientasi/guncangan.

---

## 22. Troubleshooting Checklist

1. Pastikan GND semua device tersambung.
2. Pastikan CS tidak tertukar (setiap slave CS tersendiri).
3. Pastikan mode SPI (CPOL/CPHA) sesuai slave.
4. Jalankan SPI scanner sederhana sebelum driver lengkap.
5. Cek speed clock: mulai dari rendah (100 kHz - 1 MHz) lalu naik.
6. Untuk SD Card: init 400 kHz wajib, lalu naik setelah init.
7. Untuk Flash: cek status BUSY sebelum operasi baru.
8. Untuk OLED: pastikan pin DC dan RES terhubung benar.
9. Gunakan oscilloscope/logic analyzer untuk validasi waveform.
10. Jika ESP32 dan STM32 pada bus sama, atur CS dengan benar.

---

## 23. Best Practice Desain

Hardware:

- Gunakan level logika yang sama (3.3V) untuk semua device.
- Kabel pendek dan rapi (SPI tinggi frekuensi rentan noise).
- Tambahkan resistor 10-100 ohm pada MOSI/MISO dekat master untuk signal integrity.
- Decoupling capacitor 0.1µF dekat setiap device.
- CS pin jangan floating; gunakan pull-up jika perlu saat init.

Software:

- Selalu cek return code/status.
- Gunakan timeout untuk mencegah hang.
- Pisahkan driver device, display, storage.
- Gunakan mutex/semaphore untuk shared SPI bus.
- Implementasikan state machine untuk device online/offline.
- Log error count per device.
- Jangan blocking lama di loop utama.

---

## 24. Referensi

1. Winbond W25Q64 Datasheet — SPI Flash Memory.
2. SD Association Physical Layer Simplified Specification — SD Card SPI Mode.
3. Solomon SSD1306 Datasheet — OLED Display.
4. Microchip MCP3008 Datasheet — 10-bit 8-Channel ADC.
5. Microchip MCP4921 Datasheet — 12-bit DAC.
6. Analog Devices ADXL345 Datasheet — 3-Axis Accelerometer.
7. Espressif ESP-IDF Programming Guide — SPI Master, SPI Slave, DMA.
8. STM32 Reference Manual dan HAL Driver Documentation — SPI, DMA, FreeRTOS.
9. NXP UM10204 — I2C comparison (jika perlu).
