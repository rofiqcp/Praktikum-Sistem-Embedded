# Rubrik Penilaian Project
## Modul 07: Universal Data Logger System (SPI Master/Slave)

---

## 📋 Informasi Penilaian

| Item | Keterangan |
|------|------------|
| **Nama Project** | Universal Data Logger System |
| **Bobot Total** | 100 poin |
| **Passing Grade** | 55 poin |

---

## 📊 Komponen Penilaian

### A. SPI Communication Architecture (30 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | ESP32 Master menginisialisasi Bus dengan benar | 5 | |
| 2 | Multiple Slave Selection (CS) berfungsi tanpa konflik | 10 | |
| 3 | Sinyal MOSI/MISO tidak korup (clean signal) | 5 | |
| 4 | Clock polarity/phase matching (Mode 0/3) | 5 | |
| 5 | Kecepatan transmisi optimal (>1 MHz) | 5 | |
| | **Subtotal A** | **30** | |

**Panduan Penilaian:**
- Full poin jika komunikasi stabil jangka panjang.
- Kurangi poin jika terjadi sering data error atau hang yang membutuhkan reset.

---

### B. Functional Requirements (30 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | **Config Parsing**: System membaca parameter dari SD Card | 10 | |
| 2 | **Logging Engine**: Data tersimpan ke file .CSV dengan format benar | 10 | |
| 3 | **Sensor Data**: Nilai dari STM32 terkirim akurat ke ESP32 | 10 | |
| | **Subtotal B** | **30** | |

---

### C. STM32 Firmware Implementation (15 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Konfigurasi SPI Slave Mode benar | 5 | |
| 2 | Penggunaan Interrupt/DMA (No blocking wait) | 5 | |
| 3 | Simulasi sensor data (ADC/Counter) variatif | 5 | |
| | **Subtotal C** | **15** | |

---

### D. Data Integrity & Reliability (10 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Error Handling (Try-Catch / Timeout mechanism) | 5 | |
| 2 | Data Consistency (Checksum validation) | 5 | |
| | **Subtotal D** | **10** | |

---

### E. Dokumentasi & Laporan (15 poin)

| No | Kriteria | Poin Max | Skor |
|----|----------|----------|------|
| 1 | Schematic Diagram (Fritzing/EasyEDA) | 5 | |
| 2 | Analisis Logic Analyzer (Screenshot PulseView) | 5 | |
| 3 | Code Comments & Readability | 5 | |
| | **Subtotal E** | **15** | |

---

## 🌟 Bonus Points (Maximum +15 poin)

| No | Kriteria Bonus | Poin | Skor |
|----|----------------|------|------|
| 1 | **DMA Implementation**: Full DMA pada STM32 Tx/Rx | +10 | |
| 2 | **Status Display**: Tambahkan OLED/LCD I2C untuk monitoring | +5 | |
| 3 | **Web Dashboard**: ESP32 serve file CSV via Web Server | +5 | |
| 4 | **PCB Design**: Implementasi di custom PCB | +5 | |
| | **Total Bonus** | **(+15)** | |

---

## 📈 Rekapitulasi Nilai

| Komponen | Bobot | Skor | Nilai |
|----------|-------|------|-------|
| A. SPI Architecture | 30 | | |
| B. Functional Reqs | 30 | | |
| C. STM32 Firmware | 15 | | |
| D. Reliability | 10 | | |
| E. Dokumentasi | 15 | | |
| **Total Base** | **100** | | |
| Bonus Points | (+15) | | |
| **TOTAL AKHIR** | **Max 115** | | |

---

## 📊 Konversi Grade

| Skor | Grade | Predikat |
|------|-------|----------|
| 85-100+ | A | Excellent - Industrial Quality |
| 75-84 | B+ | Very Good |
| 70-74 | B | Good - Meets Requirements |
| 65-69 | C+ | Above Average |
| 55-64 | C | Average |
| 45-54 | D | Below Average |
| <45 | E | Fail |

---

## 📝 Catatan Penilai

**Kekuatan:**
```
[Tuliskan aspek yang dikerjakan dengan baik]
```

**Area Perbaikan:**
```
[Tuliskan aspek yang perlu ditingkatkan]
```

---

| | |
|----------|------------|
| **Penilai** | _________________ |
| **Tanggal** | _________________ |
| **Tanda Tangan** | _________________ |
