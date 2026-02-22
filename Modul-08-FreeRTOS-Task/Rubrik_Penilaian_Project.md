# Rubrik Penilaian Project
## Modul 09: FreeRTOS Task Management

### 📋 Informasi Umum
- **Nama Modul**: FreeRTOS Task Management
- **Judul Project**: Multi-Platform Task Orchestrator
- **Total Nilai**: 100 poin
- **Passing Grade**: 70 poin

---

## 📊 Kriteria Penilaian Detail

### 1. Fungsionalitas STM32 (20 poin)

| Kriteria | Excellent (18-20) | Good (14-17) | Satisfactory (10-13) | Needs Work (5-9) | Poor (0-4) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Task Creation** | Semua 5+ task berjalan sempurna, tidak ada crash | 4-5 task berjalan dengan minor issues | 3-4 task berjalan, beberapa tidak stabil | 2-3 task saja yang berfungsi | Task tidak berjalan/crash |
| **Sensor Reading** | Semua sensor akurat, data konsisten | Sensor berfungsi dengan sedikit noise | Beberapa sensor bermasalah | Hanya 1-2 sensor berfungsi | Sensor tidak terbaca |
| **Actuator Control** | Semua aktuator responsif dan akurat | Minor delay tapi berfungsi | Beberapa aktuator tidak responsif | Aktuator sering error | Aktuator tidak berfungsi |
| **Stack Management** | Optimal, no overflow, HWM monitored | Memadai, sedikit margin | Tight margin, potential overflow | Stack sering overflow | Crash karena stack |

#### Checklist Penilaian STM32:
- [ ] SensorTask membaca DHT22 dengan benar
- [ ] SensorTask membaca MQ-135 dengan benar
- [ ] SensorTask membaca LDR dengan benar
- [ ] ActuatorTask mengontrol relay
- [ ] AlertTask memicu buzzer saat threshold
- [ ] UARTTransmitTask mengirim data
- [ ] MonitorTask menampilkan status
- [ ] Tidak ada stack overflow
- [ ] Tidak ada watchdog reset
- [ ] LED status indicator berfungsi

---

### 2. Fungsionalitas ESP32 (20 poin)

| Kriteria | Excellent (18-20) | Good (14-17) | Satisfactory (10-13) | Needs Work (5-9) | Poor (0-4) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Dual-Core Usage** | Core 0 & 1 optimal, load balanced | Dual-core digunakan dengan baik | Single core dominan | Dual-core tidak efektif | Tidak memanfaatkan dual-core |
| **WiFi Stability** | Koneksi stabil, auto-reconnect | Sesekali disconnect, recover cepat | Sering disconnect | WiFi tidak stabil | WiFi tidak berfungsi |
| **Cloud Integration** | Data terkirim konsisten, no data loss | >90% data terkirim | 70-90% data terkirim | <70% data terkirim | Cloud tidak terkoneksi |
| **Display Update** | Real-time, smooth refresh | Minor lag tapi readable | Refresh lambat | Display sering freeze | Display tidak berfungsi |

#### Checklist Penilaian ESP32:
- [ ] UARTReceiveTask menerima data dari STM32
- [ ] WiFiTask maintain koneksi
- [ ] CloudTask upload data ke server
- [ ] DisplayTask update OLED
- [ ] MonitorTask print status
- [ ] Task assignment ke core sudah optimal
- [ ] WiFi reconnect otomatis
- [ ] Data tidak loss saat transmisi
- [ ] Memory heap sufficient
- [ ] Button mode berfungsi

---

### 3. Integrasi Sistem (15 poin)

| Kriteria | Excellent (14-15) | Good (11-13) | Satisfactory (8-10) | Needs Work (4-7) | Poor (0-3) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **UART Communication** | Bidirectional, zero error | <1% error rate | 1-5% error rate | >5% error rate | Komunikasi gagal |
| **Data Synchronization** | Real-time sync, no latency | <500ms latency | 500ms-2s latency | >2s latency | Data tidak sinkron |
| **Protocol Design** | Robust, error handling, checksum | Basic error handling | Minimal error handling | No error handling | Protocol tidak jelas |

#### Checklist Integrasi:
- [ ] STM32 → ESP32 data flow benar
- [ ] ESP32 → STM32 command flow (jika ada)
- [ ] Format data konsisten
- [ ] Timestamp synchronized
- [ ] Error recovery implemented
- [ ] Baud rate optimal
- [ ] No data corruption

---

### 4. Task Management (15 poin)

| Kriteria | Excellent (14-15) | Good (11-13) | Satisfactory (8-10) | Needs Work (4-7) | Poor (0-3) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Priority Assignment** | Optimal, justified, documented | Reasonable, mostly correct | Beberapa priority tidak tepat | Priority assignment acak | Tidak menggunakan priority |
| **Scheduling** | No starvation, responsive | Minor starvation issues | Occasional starvation | Frequent starvation | System unresponsive |
| **Resource Usage** | Efficient, minimal waste | Good efficiency | Moderate efficiency | Inefficient | Very wasteful |
| **Timing Accuracy** | Precise periodic tasks | <5% timing error | 5-10% timing error | >10% timing error | Timing tidak akurat |

#### Checklist Task Management:
- [ ] Priority sesuai dengan kepentingan task
- [ ] Tidak ada priority inversion
- [ ] vTaskDelayUntil digunakan dengan benar
- [ ] Stack size tepat untuk setiap task
- [ ] Task tidak blocking terlalu lama
- [ ] Idle task tidak starved
- [ ] Critical section dihandle dengan benar

---

### 5. Dokumentasi (15 poin)

| Kriteria | Excellent (14-15) | Good (11-13) | Satisfactory (8-10) | Needs Work (4-7) | Poor (0-3) |
|----------|-------------------|--------------|----------------------|------------------|------------|
| **Laporan Teknis** | Lengkap, detail, analisis mendalam >15 hal | Lengkap, analisis cukup | Memenuhi minimum | Incomplete | Tidak ada laporan |
| **Diagram** | Semua diagram lengkap, profesional | Diagram lengkap, readable | Diagram minimal | Diagram tidak jelas | Tidak ada diagram |
| **Code Documentation** | Semua kode documented, README lengkap | Mostly documented | Partial documentation | Minimal comments | No documentation |
| **Schematic** | Professional schematic, labeled | Clear schematic | Basic schematic | Incomplete | No schematic |

#### Checklist Dokumentasi:
- [ ] Laporan minimal 15 halaman
- [ ] Cover dan daftar isi
- [ ] Bab pendahuluan
- [ ] Bab desain sistem
- [ ] Bab implementasi
- [ ] Bab pengujian
- [ ] Bab analisis
- [ ] Kesimpulan dan saran
- [ ] Flowchart setiap task
- [ ] State diagram
- [ ] Timing diagram
- [ ] Wiring diagram/schematic
- [ ] README.md lengkap
- [ ] Inline code comments

---

### 6. Video & Presentasi (10 poin)

| Kriteria | Excellent (9-10) | Good (7-8) | Satisfactory (5-6) | Needs Work (3-4) | Poor (0-2) |
|----------|------------------|------------|-------------------|------------------|------------|
| **Video Quality** | HD, clear audio, well-edited | Good quality | Acceptable quality | Poor quality | Unwatchable |
| **Content Coverage** | Semua aspek covered, detail | Mostly covered | Basic coverage | Incomplete | Missing key parts |
| **Demo Effectiveness** | Convincing demo, all features | Good demo | Adequate demo | Weak demo | No working demo |
| **Presentation Skills** | Professional, engaging | Good delivery | Acceptable | Needs improvement | Poor presentation |

#### Checklist Video:
- [ ] Durasi 5-10 menit
- [ ] Intro dan overview sistem
- [ ] Demo hardware setup
- [ ] Demo setiap task berjalan
- [ ] Demo komunikasi STM32-ESP32
- [ ] Demo cloud/dashboard
- [ ] Demo alert system
- [ ] Audio jelas dan tidak ada noise
- [ ] Video tidak blur
- [ ] Editing profesional

#### Checklist Presentasi:
- [ ] Maksimal 15 slide
- [ ] Slide overview
- [ ] Slide arsitektur
- [ ] Slide implementasi
- [ ] Slide demo
- [ ] Q&A preparation
- [ ] Live demo berhasil

---

### 7. Inovasi dan Bonus (5 poin)

| Kriteria | Poin |
|----------|------|
| Fitur tambahan yang tidak diminta | +1-2 |
| Optimisasi kode yang signifikan | +1 |
| UI/UX dashboard yang baik | +1 |
| Error handling yang komprehensif | +1 |
| Testing otomatis | +1 |
| Power management/sleep mode | +1 |
| OTA update capability | +2 |

**Maksimal bonus: 5 poin (tidak melebihi total 100)**

---

## 📝 Form Penilaian

```
Nama Kelompok: _______________________
Anggota:
1. ___________________ (NIM: _________)
2. ___________________ (NIM: _________)
3. ___________________ (NIM: _________)

Tanggal Demo: ____________

NILAI:
1. Fungsionalitas STM32    : ____ / 20
2. Fungsionalitas ESP32    : ____ / 20
3. Integrasi Sistem        : ____ / 15
4. Task Management         : ____ / 15
5. Dokumentasi             : ____ / 15
6. Video & Presentasi      : ____ / 10
7. Inovasi (Bonus)         : ____ / 5

TOTAL                      : ____ / 100

Grade:
[ ] A  (90-100)
[ ] AB (85-89)
[ ] B  (80-84)
[ ] BC (75-79)
[ ] C  (70-74)
[ ] D  (60-69)
[ ] E  (<60)

Catatan Penilai:
_________________________________________________
_________________________________________________
_________________________________________________

Tanda Tangan Penilai: _____________
Tanggal: _____________
```

---

## ⚠️ Ketentuan Khusus

### Pengurangan Nilai
| Pelanggaran | Pengurangan |
|-------------|-------------|
| Keterlambatan per hari | -10% |
| Tidak menggunakan STM32 | -30% |
| Tidak menggunakan ESP32 | -30% |
| Plagiarisme | -100% (nilai 0) |
| Tidak ada komunikasi antar device | -20% |
| Task kurang dari 5 per platform | -2% per task |

### Ketentuan Minimum
Untuk lulus (passing grade 70), mahasiswa HARUS memenuhi:
1. ✅ Menggunakan KEDUA platform (STM32 dan ESP32)
2. ✅ Minimal 5 task per platform
3. ✅ Ada komunikasi UART antar device
4. ✅ Menyerahkan source code yang bisa dikompilasi
5. ✅ Menyerahkan laporan teknis
6. ✅ Menyerahkan video demonstrasi

---

*Rubrik ini bersifat final dan tidak dapat diganggu gugat*
