# Tugas Video — Modul 07: SPI Bus dan Storage

## Informasi Tugas

| Item | Detail |
|------|--------|
| Modul | 07 — SPI & Storage |
| Platform | STM32F103C8T6 & ESP32 DevKit V1 |
| Format | Video Presentasi + Demonstrasi Hardware |
| Durasi | 15–25 menit |
| Upload | YouTube (Unlisted) → link di e-learning |

---

## Deskripsi Tugas

Video laporan lengkap praktikum dan project Modul 07 — materi SPI/storage, 12 percobaan, dan project data logger.

---

## Ketentuan Teknis Video

- **Screen recording** + **Webcam** (PiP) wajib
- **Hardware recording** menunjukkan OLED, SD card, flash module, wiring SPI
- Resolusi minimal 720p, audio jelas

---

## Struktur Video

### 1. Pembukaan (1–2 menit)
### 2. Ringkasan Materi (2–3 menit)
- Protokol SPI: MOSI, MISO, SCLK, CS, mode (CPOL/CPHA)
- SPI vs I2C: kecepatan, jumlah pin, jumlah device
- Storage embedded: NVS, SPIFFS, FAT, internal flash
- SPI pada ESP32 vs STM32

### 3. Demonstrasi Percobaan (8–14 menit)

| No | Percobaan | Poin Penting Demo |
|----|-----------|-------------------|
| P01 | SPI Loopback | TX=RX verification, 5 test patterns |
| P02 | SPI OLED | Tampilan teks dan counter di OLED |
| P03 | Flash W25Q32 | JEDEC ID, erase→write→read→verify |
| P04 | SD Card | Mount, write file, baca file, dir listing |
| P05 | MCP3208 ADC | Putar potensio → 8-channel table |
| P06 | MCP4921 DAC | 4 waveform, ukur output |
| P07 | Multi-Slave | Transfer ke 2 slave bergantian |
| P08 | NVS/Flash KV | Boot counter persist setelah reset |
| P09 | SPIFFS/KV Store | File operations, storage usage |
| P10 | Speed Benchmark | Tabel throughput vs clock speed |
| P11 | Interrupt Mode | Blocking vs interrupt timing |
| P12 | Data Logger | CSV logging, statistics, storage |

### 4. Demonstrasi Project (3–5 menit)
- Skenario vulkanologi
- Multi-tier storage demo
- OLED display + sensor reading + alarm DAC

### 5. Penutup (1–2 menit)

---

## Penilaian & Penalti

| Komponen | Bobot |
|----------|-------|
| Pemahaman materi | 15% |
| Demonstrasi percobaan | 35% |
| Demonstrasi project | 20% |
| Demo hardware | 15% |
| Kualitas video | 15% |

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Webcam tidak terlihat | −20% |
| Tidak ada demo hardware | −20% |
| Durasi < 10 / > 30 menit | −10% / −5% |
| Terlambat | −10% per hari |

---

## Checklist

- [ ] YouTube Unlisted, 15–25 menit
- [ ] Webcam + audio jelas
- [ ] 12 percobaan (kedua platform)
- [ ] Project demo
- [ ] Hardware demo (OLED, flash, SD card, MCP3208/4921)
- [ ] Link di e-learning
