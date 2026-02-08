# 🎨 PPT Prompts Modul 06 — Bagian 1: Teori I2C Protocol (Slide 1-20)

## Petunjuk Penggunaan
Gunakan prompt berikut untuk membuat slide presentasi di PowerPoint, Google Slides, atau Canva. Setiap prompt menghasilkan 1 slide.

---

## Slide 1: Cover
```
Buatkan slide cover presentasi dengan judul "Modul 06: Komunikasi I2C & Interfacing Sensor" 
dengan subtitle "Praktikum Sistem Embedded". Gunakan gambar latar yang menunjukkan 
PCB dengan chip sensor dan jalur I2C. Tambahkan logo universitas, nama dosen, 
dan semester. Warna tema: biru navy dan putih.
```

## Slide 2: Outline Materi
```
Buatkan slide outline dengan judul "Topik Pembahasan" berisi daftar:
1. Pengantar Protokol I2C
2. Arsitektur Bus I2C (SDA & SCL)
3. Sinyal START, STOP, ACK, NACK
4. Addressing 7-bit dan 10-bit
5. Mode Transfer Data (Read/Write)
6. Clock Stretching & Arbitrasi
7. Pull-up Resistor & Desain Hardware
8. I2C pada ESP32 (ESP-IDF)
9. I2C pada STM32 (HAL)
10. Sensor & Device I2C Populer
Gunakan icon yang relevan untuk setiap topik. Layout dua kolom.
```

## Slide 3: Sejarah & Pengantar I2C
```
Buatkan slide dengan judul "Apa itu I2C?" dengan konten:
- I2C = Inter-Integrated Circuit, dikembangkan oleh Philips (sekarang NXP) tahun 1982
- Protokol serial synchronous half-duplex
- Hanya membutuhkan 2 jalur: SDA (data) dan SCL (clock)
- Multi-master, multi-slave pada satu bus
- Kecepatan: Standard (100kHz), Fast (400kHz), Fast+ (1MHz), High-speed (3.4MHz)
- Digunakan di hampir semua sensor, EEPROM, RTC, display OLED, ADC/DAC
Tambahkan gambar logo I2C dan timeline perkembangan.
```

## Slide 4: Perbandingan Protokol Komunikasi
```
Buatkan slide dengan tabel perbandingan I2C vs SPI vs UART:

| Aspek | I2C | SPI | UART |
|-------|-----|-----|------|
| Jumlah Pin | 2 (SDA, SCL) | 4+ (MOSI, MISO, SCK, CS) | 2 (TX, RX) |
| Kecepatan | 100K - 3.4M | 10M - 50M+ | 9600 - 921600 |
| Multi-Device | Ya (addressing) | Ya (CS per device) | Tidak (P2P) |
| Jarak | Pendek (<1m) | Sangat pendek | Menengah |
| Full/Half Duplex | Half-duplex | Full-duplex | Full-duplex |
| Complexity | Medium | Low | Low |

Highlight keunggulan I2C: minimum pin, multi-device. Gunakan warna berbeda per kolom.
```

## Slide 5: Arsitektur Bus I2C
```
Buatkan slide dengan judul "Arsitektur Bus I2C" berisi diagram:
- Satu bus dengan jalur SDA dan SCL
- Pull-up resistor ke VCC pada kedua jalur
- 1 Master (MCU) dan 4 Slave (Sensor, EEPROM, RTC, OLED)
- Setiap device terhubung paralel ke bus
- Open-drain/open-collector output
- Label address setiap slave (0x76, 0x50, 0x68, 0x3C)
Tambahkan keterangan: "Semua device berbagi 2 jalur yang sama"
```

## Slide 6: Sinyal SDA dan SCL
```
Buatkan slide dengan judul "Sinyal SDA & SCL" berisi:
- Diagram timing menunjukkan hubungan SDA dan SCL
- SDA: Data berubah saat SCL LOW
- SDA: Data harus stabil saat SCL HIGH
- Penjelasan: SCL dikontrol oleh Master, SDA oleh Master atau Slave
- Ilustrasi open-drain: device hanya bisa pull LOW, release HIGH via pull-up
- Diagram equivalent circuit open-drain dengan pull-up resistor
```

## Slide 7: Kondisi START dan STOP
```
Buatkan slide dengan judul "START & STOP Condition" berisi:
- Diagram timing START: SDA turun dari HIGH ke LOW saat SCL HIGH
- Diagram timing STOP: SDA naik dari LOW ke HIGH saat SCL HIGH
- Repeated START: START tanpa STOP sebelumnya (untuk combined transfer)
- Keterangan: "START dan STOP adalah SATU-SATUNYA kondisi dimana SDA berubah saat SCL HIGH"
- Ilustrasi sequence: IDLE → START → Data Transfer → STOP → IDLE
Gunakan warna merah untuk START, hijau untuk STOP.
```

## Slide 8: Format Address I2C
```
Buatkan slide dengan judul "I2C Addressing (7-bit)" berisi:
- Byte pertama setelah START: 7-bit address + 1-bit R/W
- Format: [A6][A5][A4][A3][A2][A1][A0][R/W]
- R/W = 0: Master Write ke Slave
- R/W = 1: Master Read dari Slave
- Contoh: BMP280 address 0x76 → Write: 0xEC, Read: 0xED
- Tabel reserved address: 0x00 (General Call), 0x01-0x07, 0x78-0x7F
- Total: 112 address yang bisa digunakan (0x08-0x77)
Tambahkan diagram bit-level dari address byte.
```

## Slide 9: ACK dan NACK
```
Buatkan slide dengan judul "ACK & NACK Response" berisi:
- Diagram timing: Setelah 8 bit data, Master release SDA untuk bit ke-9
- ACK: Slave pull SDA LOW pada clock ke-9 → data diterima
- NACK: SDA tetap HIGH pada clock ke-9 → data tidak diterima
- Kapan NACK terjadi:
  1. Slave tidak ada di bus (wrong address)
  2. Slave sedang busy
  3. Master sinyal bahwa Read selesai (Master NACK)
  4. Register yang diminta tidak valid
- Ilustrasi: Timeline 9 clock cycles dengan ACK/NACK pada bit ke-9
```

## Slide 10: Write Operation
```
Buatkan slide dengan judul "I2C Write Operation" berisi diagram sequence:
1. Master → START condition
2. Master → Slave Address + W (0) [8 bit]
3. Slave → ACK [1 bit]
4. Master → Register Address [8 bit]
5. Slave → ACK [1 bit]
6. Master → Data Byte [8 bit]
7. Slave → ACK [1 bit]
8. Master → STOP condition

Contoh: Menulis 0x27 ke register 0xF4 pada BMP280 (0x76)
→ [START][0xEC][ACK][0xF4][ACK][0x27][ACK][STOP]
Gunakan warna biru untuk Master, hijau untuk Slave.
```

## Slide 11: Read Operation
```
Buatkan slide dengan judul "I2C Read Operation" berisi diagram sequence:
1. Master → START condition
2. Master → Slave Address + W (0) [set register pointer]
3. Slave → ACK
4. Master → Register Address
5. Slave → ACK
6. Master → Repeated START
7. Master → Slave Address + R (1)
8. Slave → ACK
9. Slave → Data Byte
10. Master → NACK (last byte) atau ACK (more bytes)
11. Master → STOP

Highlight: "Read membutuhkan 2 fase: Write register address, lalu Read data"
```

## Slide 12: Multi-Byte Transfer
```
Buatkan slide dengan judul "Multi-Byte Read/Write" berisi:
- Burst Write: Master mengirim banyak byte berturut-turut setelah register address
  → Register auto-increment pada kebanyakan device
- Burst Read: Slave mengirim banyak byte, Master ACK setiap byte, NACK pada byte terakhir
- Diagram: [START][ADDR+R][ACK][DATA0][ACK][DATA1][ACK][DATA2][NACK][STOP]
- Contoh: Baca 6 byte dari BMP280 (temp + pressure = 3 byte × 2)
- Keuntungan: Lebih efisien daripada read satu-satu (kurang overhead START/STOP)
```

## Slide 13: Clock Stretching
```
Buatkan slide dengan judul "Clock Stretching" berisi:
- Definisi: Slave menahan SCL LOW untuk memperlambat transfer
- Kapan terjadi: Slave butuh waktu lebih untuk memproses data
- Diagram timing: Master release SCL, tapi SCL tetap LOW karena Slave hold
- Master harus menunggu sampai SCL benar-benar HIGH sebelum lanjut
- Contoh: EEPROM write cycle (5-10ms), sensor ADC conversion
- Peringatan: Tidak semua Master mendukung clock stretching!
- ESP32: Mendukung ✅ | STM32: Mendukung ✅
```

## Slide 14: Arbitrasi Multi-Master
```
Buatkan slide dengan judul "Multi-Master Arbitration" berisi:
- Skenario: 2 Master mencoba berkomunikasi bersamaan
- Proses arbitrasi: Setiap Master monitor SDA saat mengirim
- Jika Master kirim HIGH tapi SDA LOW → Master lain menang → mundur
- Arbitrasi terjadi tanpa data corruption (non-destructive)
- Diagram: Master A dan Master B mengirim address, salah satu mundur
- Catatan: Dalam praktikum ini, kita hanya menggunakan single-master
```

## Slide 15: Pull-up Resistor Design
```
Buatkan slide dengan judul "Pull-up Resistor: Kenapa Penting?" berisi:
- I2C menggunakan open-drain → butuh pull-up untuk level HIGH
- Tanpa pull-up: bus mengambang (float), data corrupt
- Pemilihan nilai resistor:
  - Terlalu besar (10kΩ+): rise time lambat, kecepatan terbatas
  - Terlalu kecil (1kΩ): arus berlebih saat LOW
  - Optimal: 4.7kΩ untuk 400kHz, 2.2kΩ untuk 1MHz
- Formula: R_pull-up = V_CC / I_sink_max
- Diagram equivalent circuit dengan pull-up
- Foto: Efek di osiloskop tanpa vs dengan pull-up
```

## Slide 16: Level Shifting & Voltage
```
Buatkan slide dengan judul "Level Shifting I2C" berisi:
- Masalah: Sensor 3.3V + MCU 5V pada satu bus → bisa merusak sensor!
- Solusi 1: Level shifter bidirectional (TXS0102, BSS138)
- Solusi 2: Gunakan MCU dan sensor dengan VCC yang sama (3.3V)
- ESP32: 3.3V I/O → langsung kompatibel dengan sensor 3.3V
- STM32F103: 3.3V I/O, tapi 5V tolerant pada pin I2C ✅
- Diagram: Level shifter circuit dengan MOSFET BSS138
- Tabel kompatibilitas tegangan
```

## Slide 17: Bus Capacitance & Trace Length
```
Buatkan slide dengan judul "Limitasi Bus I2C" berisi:
- Maximum bus capacitance: 400 pF (Standard/Fast mode)
- Setiap device menambah ~10-15 pF
- Kabel/trace menambah ~50-100 pF/meter
- Implikasi: Jumlah device dan panjang kabel terbatas
- Rekomendasi: <1 meter total panjang bus
- Terlalu banyak device → rise time lambat → error
- Solusi: I2C buffer/repeater (PCA9600) untuk bus panjang
- Diagram: Kapasitansi parasitik pada bus
```

## Slide 18: Tabel Address Device Populer
```
Buatkan slide dengan judul "Address Device I2C Populer" berisi tabel besar:
| Address | Device | Fungsi |
|---------|--------|--------|
| 0x20-0x27 | PCF8574 | I/O Expander |
| 0x23, 0x5C | BH1750 | Light Sensor |
| 0x3C, 0x3D | SSD1306 | OLED Display |
| 0x48-0x4B | ADS1115 | ADC 16-bit |
| 0x50-0x57 | AT24Cxx | EEPROM |
| 0x68 | DS3231 | RTC |
| 0x68, 0x69 | MPU6050 | IMU 6-axis |
| 0x76, 0x77 | BMP280 | Temp/Pressure |
Highlight konflik: DS3231 dan MPU6050 sama-sama 0x68!
Solusi: I2C Multiplexer TCA9548A
```

## Slide 19: I2C Error Types
```
Buatkan slide dengan judul "Jenis Error pada I2C" berisi:
1. NACK Error: Device tidak merespon → address salah atau device mati
2. Bus Busy: SDA/SCL stuck LOW → perlu bus reset
3. Arbitration Lost: Multi-master collision
4. Timeout: Slave tidak merespon dalam waktu tertentu
5. Clock Stretching Timeout: Slave hold SCL terlalu lama
6. Data Corruption: Noise pada bus → CRC/checksum mismatch

Untuk setiap error, tambahkan:
- Penyebab umum
- Cara mendeteksi
- Cara recovery
Gunakan icon warning merah untuk setiap error.
```

## Slide 20: I2C Bus Recovery
```
Buatkan slide dengan judul "I2C Bus Recovery" berisi:
- Masalah: SDA stuck LOW (slave freeze mid-transfer)
- Penyebab: Power glitch, noise, slave hang
- Recovery procedure:
  1. Master toggle SCL 9x → force slave release SDA
  2. Generate STOP condition
  3. Re-initialize I2C peripheral
  4. Re-scan bus untuk verifikasi

- ESP32: i2c_master_bus_reset() / manual GPIO toggle
- STM32: HAL_I2C_DeInit() + GPIO toggle + HAL_I2C_Init()
- Diagram flowchart recovery procedure
- Tips: Tambahkan timeout + retry di production code
```

---

*Modul 06 — Praktikum Sistem Embedded*
*PPT Prompts Bagian 1: Teori Protokol I2C*
