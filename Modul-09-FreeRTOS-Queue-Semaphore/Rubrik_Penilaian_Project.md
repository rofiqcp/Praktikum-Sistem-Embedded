# Rubrik Penilaian Project Modul 10
## Sistem Monitoring Parkir Cerdas dengan Queue dan Semaphore

### 📋 Informasi Umum
- **Total Nilai**: 100 poin
- **Passing Grade**: 70 poin
- **Deadline**: Sesuai jadwal praktikum

---

## 📊 Kriteria Penilaian

### 1. Counting Semaphore Implementation (20 poin)

| Aspek | Excellent (18-20) | Good (14-17) | Adequate (10-13) | Poor (0-9) |
|-------|-------------------|--------------|------------------|------------|
| **Slot Management** | Counting semaphore mengelola slot dengan benar, take saat entry, give saat exit, count akurat | Counting semaphore berfungsi, minor bug pada edge cases | Implementasi dasar, beberapa masalah pada counting | Tidak bekerja / tidak diimplementasi |
| **Full Handling** | Deteksi penuh akurat, reject entry saat slot=0, feedback tepat | Deteksi berfungsi dengan delay kecil | Deteksi tidak konsisten | Tidak ada handling |
| **Status Query** | uxSemaphoreGetCount() digunakan dengan benar untuk monitoring | Digunakan tapi tidak optimal | Jarang digunakan | Tidak digunakan |

---

### 2. Queue Implementation (20 poin)

| Aspek | Excellent (18-20) | Good (14-17) | Adequate (10-13) | Poor (0-9) |
|-------|-------------------|--------------|------------------|------------|
| **Event Queue** | Queue menyimpan event dengan struct lengkap, FIFO benar, tidak ada data loss | Queue berfungsi, occasional data ordering issue | Queue basic functional, beberapa data hilang | Queue tidak bekerja |
| **UART Communication** | Protocol frame valid, checksum benar, parsing akurat | Komunikasi berfungsi, minor parsing issue | Komunikasi basic, sering error | Komunikasi gagal |
| **Queue dari ISR** | xQueueSendFromISR dengan yield handling benar | FromISR digunakan tapi yield tidak optimal | Direct xQueueSend dari ISR | ISR tidak memanfaatkan queue |
| **Return Value Check** | Semua return value dicek dan di-handle | Sebagian besar dicek | Jarang dicek | Tidak dicek |

---

### 3. Mutex Usage (15 poin)

| Aspek | Excellent (13-15) | Good (10-12) | Adequate (7-9) | Poor (0-6) |
|-------|-------------------|--------------|----------------|------------|
| **UART Protection** | Printf/Serial di-protect mutex, output tidak tercampur | Protection berfungsi, rare interleaving | Kadang output tercampur | Tidak ada protection |
| **Display Protection** | OLED/LCD update atomic dengan mutex | Protection dengan minor glitch | Sesekali display corrupt | Tidak ada protection |
| **Timeout Handling** | Mutex take dengan timeout reasonable, fallback jelas | Timeout digunakan tapi tidak optimal | portMAX_DELAY sering digunakan | Tidak ada timeout |

---

### 4. STM32 Node Implementation (15 poin)

| Aspek | Excellent (13-15) | Good (10-12) | Adequate (7-9) | Poor (0-6) |
|-------|-------------------|--------------|----------------|------------|
| **Sensor Detection** | Entry/exit detection dengan debounce sempurna, edge detection akurat | Detection berfungsi dengan minor bounce | Bounce kadang mengganggu | Detection tidak reliable |
| **LED Controller** | LED indicator sesuai status (green/yellow/red), transisi smooth | LED berfungsi, transisi kadang delay | LED kadang tidak sinkron | LED tidak bekerja |
| **Buzzer Alert** | Alert saat penuh dengan timing tepat | Alert berfungsi, timing kurang presisi | Alert tidak konsisten | Tidak ada alert |
| **Task Structure** | Multi-task dengan priority tepat, tidak ada blocking issue | Task berfungsi, priority bisa dioptimasi | Task ada tapi struktur kurang baik | Single task / tidak terstruktur |

---

### 5. ESP32 Gateway Implementation (15 poin)

| Aspek | Excellent (13-15) | Good (10-12) | Adequate (7-9) | Poor (0-6) |
|-------|-------------------|--------------|----------------|------------|
| **UART Receiver** | Frame parsing robust, checksum validation, error recovery | Parsing berfungsi, occasional error | Parsing basic, sering error | Parsing gagal |
| **Display Update** | OLED menampilkan slot count, status, statistics real-time | Display update dengan minor delay | Display kadang tidak update | Display tidak bekerja |
| **Dual-Core Usage** | Task distribution optimal (UART di Core 0, Display di Core 1) | Dual-core digunakan tapi tidak optimal | Single core usage | Tidak aware dual-core |
| **Data Processing** | Event processing lengkap, statistics tracking akurat | Processing berfungsi, stats kadang off | Processing basic | Processing tidak berfungsi |

---

### 6. System Integration (10 poin)

| Aspek | Excellent (9-10) | Good (7-8) | Adequate (5-6) | Poor (0-4) |
|-------|------------------|------------|----------------|------------|
| **UART Communication** | STM32-ESP32 komunikasi lancar, no data loss, < 100ms latency | Komunikasi berfungsi, occasional delay | Komunikasi kadang putus | Komunikasi gagal |
| **End-to-End Flow** | Entry→Queue→Semaphore→UART→ESP32→Display seamless | Flow berfungsi dengan minor hiccup | Flow sering terputus | Flow tidak lengkap |
| **Error Handling** | Graceful degradation, reconnection handling | Basic error handling | Minimal error handling | Crash saat error |

---

### 7. Documentation & Presentation (5 poin)

| Aspek | Excellent (5) | Good (4) | Adequate (3) | Poor (0-2) |
|-------|---------------|----------|--------------|------------|
| **Code Comments** | Kode well-documented, penjelasan logic jelas | Comment cukup | Comment minimal | Tidak ada comment |
| **README** | Setup instruction, architecture diagram, troubleshooting | README lengkap | README basic | Tidak ada README |
| **Video Demo** | Demo jelas, semua fitur ditunjukkan, penjelasan teknis baik | Demo lengkap tapi kurang detail | Demo minimal | Tidak ada demo |

---

## 📈 Bonus Points (Max +10)

| Bonus | Poin | Kriteria |
|-------|------|----------|
| WiFi Cloud Upload | +3 | ESP32 upload data ke cloud (ThingSpeak/Blynk) |
| Real-Time Clock | +2 | Timestamp menggunakan RTC bukan tick count |
| SD Card Logging | +2 | Log event ke SD card |
| Web Dashboard | +3 | Simple web interface untuk monitoring |
| Statistics Analysis | +2 | Peak hours, average occupancy calculation |
| Multiple Entry/Exit | +3 | Support lebih dari 1 entry/exit point |

---

## 📝 Perhitungan Nilai Akhir

```
Nilai Total = Counting Semaphore (20) + Queue (20) + Mutex (15) + 
              STM32 (15) + ESP32 (15) + Integration (10) + Documentation (5)
              + Bonus (max 10)

Maximum: 110 poin (capped at 100)
```

---

## ⚠️ Pengurangan Nilai

| Pelanggaran | Pengurangan |
|-------------|-------------|
| Deadline terlambat (per hari) | -5 poin |
| Plagiarism | -100% (nilai 0) |
| Tidak ada video demo | -10 poin |
| Build error/tidak dapat compile | -20 poin |
| Hardware tidak berfungsi saat demo | -15 poin |

---

## ✅ Checklist Submission

- [ ] Source code STM32 (main.c, FreeRTOSConfig.h)
- [ ] Source code ESP32 (main.cpp, platformio.ini)
- [ ] README.md dengan setup instructions
- [ ] Schematic diagram (fritzing/pdf)
- [ ] Video demo (5-10 menit)
- [ ] Laporan PDF (opsional untuk bonus)

---

*Rubrik Penilaian Project Modul 10 - FreeRTOS Queue dan Semaphore*
