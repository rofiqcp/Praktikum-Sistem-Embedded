# Project Modul 07: Universal Data Logger System

## 📋 Informasi Project

| Item | Keterangan |
|------|------------|
| **Mata Kuliah** | Praktikum Sistem Embedded |
| **Modul** | 07 - SPI Bus dan Storage |
| **Tingkat Kesulitan** | ⭐⭐⭐ (Intermediate) |
| **Waktu Pengerjaan** | 2-3 minggu |
| **Platform** | STM32F103C8T6 + ESP32 DevKit + SD Card |

---

## 🎯 Deskripsi Project

Project ini bertujuan untuk membangun **Universal Data Logger** yang memanfaatkan arsitektur **SPI Multi-Slave**. Anda akan menggabungkan kekuatan pemrosesan real-time STM32 dengan konektivitas dan storage ESP32.

Dalam sistem ini:
- **ESP32** bertindak sebagai **SPI Master** utama.
- **STM32** bertindak sebagai **SPI Slave** (Intelligent Sensor Hub).
- **SD Card** bertindak sebagai **SPI Slave** (Storage).

ESP32 akan membaca konfigurasi dari SD Card, mengirimkan parameter ke STM32, kemudian secara periodik mengambil data akuisisi dari STM32 untuk disimpan kembali ke SD Card dalam format Log (CSV).

---

## 📐 Arsitektur Sistem

### Diagram Blok

```
                    ┌───────────────────────────┐
                    │    UNIVERSAL DATA LOGGER   │
                    └───────────────────────────┘
                    
                             SPI BUS (SHARED)
          ┌─────────────────────────────────────────────────────┐
          │     MOSI, MISO, SCLK                                │
          │                                                     │
  ┌───────▼──────┐          ┌───────▼──────┐          ┌────────▼───────┐
  │              │          │              │          │                │
  │    ESP32     │          │    STM32     │          │    SD CARD     │
  │   (MASTER)   │          │    (SLAVE)   │          │    (SLAVE)     │
  │              │          │              │          │                │
  │           CS1├─────────►│NSS           │          │                │
  │           CS2├──────────┼──────────────┼─────────►│CS              │
  │              │          │              │          │                │
  └──────────────┘          └───────┬──────┘          └────────────────┘
                                    │
                             ┌──────▼──────┐
                             │   SENSORS   │
                             │ (ADC/GPIO)  │
                             └─────────────┘
```

### Koneksi Hardware

**SPI Bus Connections:**
| Signal | ESP32 (Master) | STM32 (Slave) | SD Card (Slave) |
|--------|----------------|---------------|-----------------|
| MOSI | GPIO 23 | PA7 (SF1) / PB5 (AF) | MOSI |
| MISO | GPIO 19 | PA6 (SF1) / PB4 (AF) | MISO |
| SCLK | GPIO 18 | PA5 (SF1) / PB3 (AF) | SCK |
| CS 1 | GPIO 5 | PA4 (NSS) | - |
| CS 2 | GPIO 4 | - | CS |

*Catatan: STM32 NSS pin harus dikonfigurasi sebagai Input Floating atau Hardware NSS Input.*

---

## 📝 Spesifikasi Fungsional

### 1. File Konfigurasi (SD Card)
Sistem harus membaca file `config.txt` pada saat startup yang berisi:
```ini
INTERVAL=1000      ; Logging interval in ms (100ms - 5000ms)
MODE=ANALOG        ; Mode akuisisi (ANALOG/DIGITAL)
THRESHOLD=2048     ; Threshold value alert
```

### 2. Protokol Komunikasi (SPI)
Definisikan protokol packet sederhana antara ESP32 dan STM32:

**Packet Structure (Fixed Size: 8 Bytes):**
```c
struct DataPacket {
    uint8_t startByte;   // 0xAA
    uint8_t command;     // 0x01=CONFIG, 0x02=DATA_REQ
    uint16_t value1;     // Config Param or Sensor Data
    uint16_t value2;     // Checksum or Extra Data
    uint8_t stopByte;    // 0x55
};
```

### 3. ESP32 Implementation (Master)
- **Init:** Mount SD Card check `config.txt`.
- **Config:** Kirim parameter (Interval, Threshold) ke STM32 via SPI.
- **Loop:**
  - Timer check berdasarkan interval.
  - Assert CS_STM32 (Low).
  - SPI Transmit: Request Command.
  - SPI Receive: Sensor Data.
  - De-assert CS_STM32 (High).
  - Validasi data.
  - Format data ke string CSV: `Timestamp,RawValue,Voltage,Status`.
  - Assert CS_SD (Low) -> Write to file -> De-assert CS_SD.

### 4. STM32 Implementation (Slave)
- **Init:** SPI Slave Mode (Interrupt/DMA recommended).
- **Loop:**
  - Baca nilai ADC (Potensiometer/LDR pada PA0).
  - Update `tx_buffer` dengan nilai terbaru.
  - Tunggu transaksi SPI dari Master.
  - Jika terima paket CONFIG: Update variable internal.
  - Jika terima paket DATA_REQ: Siapkan data di register SPI untuk transaksi berikutnya (atau gunakan full-duplex exchange).

---

## 📊 Kriteria Penilaian

### Komponen Utama
| Kriteria | Bobot | Deskripsi |
|----------|-------|-----------|
| **SPI Multi-Slave** | 30% | ESP32 sukses mengontrol STM32 dan SD Card pada bus yang sama tanpa konflik. |
| **Data Integrity** | 20% | Data yang diterima ESP32 sama persis dengan yang dikirim STM32 (tidak shift bit, no noise). |
| **Config Parsing** | 15% | Sistem merespon perubahan isi file `config.txt`. |
| **Data Logging** | 15% | File CSV terbentuk rapi dan valid (bisa dibuka di Excel). |
| **STM32 Firmware** | 10% | Implementasi Slave mode yang efisien (Interrupt/DMA). |
| **Documentation** | 10% | Laporan dan Video demo. |

### Bonus (+15 Poin)
1. **DMA Implementation (+10):** Gunakan DMA pada STM32 untuk transfer SPI agar CPU tidak terbebani.
2. **High Speed (+5):** Capai stable transfer di atas 4 MHz clock speed dengan kabel jumper (perlu wiring rapi).

---

## 📝 Skenario Pengujian

1. **Boot Test:**
   - Masukkan SD Card dengan config.
   - Nyalakan sistem.
   - ESP32 harus print "Config Loaded" di Serial.
   - STM32 harus blink LED tanda terima config.

2. **Logging Test:**
   - Biarkan berjalan 1 menit.
   - Ubah nilai potensiometer di STM32 secara variatif.
   - Stop sistem, cabut SD Card, buka di PC.
   - Verifikasi data CSV mencerminkan perubahan potensiometer.

3. **Config Change Test:**
   - Ubah `INTERVAL=100` di PC.
   - Pasang lagi.
   - Sistem harus logging 10x lebih cepat.

---

## 📦 Deliverables

1. **Source Code:**
   - Folder `stm32_slave/` (PlatformIO project)
   - Folder `esp32_master/` (PlatformIO project)
2. **Documentation:**
   - `Laporan_Project_Modul07.pdf`
   - `Schematic_Wiring.png`
3. **Demo Video:**
   - Link YouTube unlisted.
   - Tunjukkan proses boot, logging, dan verifikasi data di PC.

---

## 💡 Tips & Tricks

- **Chip Select (CS) is Key:** Pastikan hanya SATU CS yang Low pada satu waktu. Saat mengakses SD Card, pastikan CS STM32 High, dan sebaliknya.
- **SPI Mode Matching:** Pastikan CPOL dan CPHA sama di kedua MCU. Mode 0 atau Mode 3 paling aman.
- **Cek Data Type:** Hati-hati endianness saat mengirim `uint16_t` atau `float` antar MCU berbeda arsitektur (meskipun STM32 dan ESP32 sama-sama Little Endian).
- **Buffer:** Gunakan buffer array untuk data SPI, jangan kirim byte per byte jika bisa sekaligus packet.

Selamat Mengerjakan! 🚀

