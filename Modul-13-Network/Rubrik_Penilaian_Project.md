# Rubrik Penilaian Project Modul 13: Network Communication

## Informasi Umum

| Item | Keterangan |
|------|------------|
| **Nama Project** | Smart Environmental Monitoring Network |
| **Modul** | 13 - Network Communication |
| **Total Nilai** | 100 poin |
| **Batas Waktu** | 4 minggu |

---

## A. Fungsionalitas Sistem (40 poin)

### A1. ESP32 Sensor Nodes (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **WiFi Connection** | Auto-connect, reconnect otomatis, status indicator | Reconnect manual, indicator ada | Connect berhasil, tanpa reconnect | Connect tidak stabil | Tidak bisa connect |
| **MQTT Publishing** | Semua data dengan QoS, retain, format JSON benar | Data lengkap, QoS default | Data terkirim, format kurang | Data tidak lengkap | Tidak publish |
| **Sensor Reading** | Akurat, validasi data, error handling | Akurat dengan validasi | Reading basic | Reading tidak akurat | Tidak bisa read |

**Penilaian Detail:**
- [ ] WiFi Station mode berfungsi (2 poin)
- [ ] Auto-reconnect saat disconnect (2 poin)
- [ ] MQTT publish dengan format JSON (2 poin)
- [ ] QoS dan retain message (2 poin)
- [ ] LED status indicator (2 poin)

### A2. STM32 Data Processor (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **Serial Communication** | Baud rate optimal, buffer management | Komunikasi stabil | Basic communication | Sering error | Tidak berfungsi |
| **Data Analytics** | Min/max/avg, trend detection, anomaly detection | Statistics lengkap | Basic calculations | Hanya satu metric | Tidak ada analytics |
| **Alert System** | Multi-threshold, notifikasi real-time | Single threshold alert | Alert delayed | Alert tidak konsisten | Tidak ada alert |

**Penilaian Detail:**
- [ ] UART communication dengan ESP32 (2 poin)
- [ ] Parsing JSON dari gateway (2 poin)
- [ ] Kalkulasi statistics (min/max/avg) (2 poin)
- [ ] Threshold detection (2 poin)
- [ ] Forward analytics ke gateway (2 poin)

### A3. ESP32 Gateway (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **MQTT Bridge** | Subscribe all topics, forward lengkap | Subscribe sebagian | Basic subscription | Forward tidak reliable | Tidak berfungsi |
| **WebSocket Server** | Multi-client, broadcast, ping/pong | Single client | Basic WebSocket | Koneksi tidak stabil | Tidak berfungsi |
| **Serial Bridge** | Bidirectional, parsing benar | Satu arah | Basic forwarding | Data loss | Tidak berfungsi |

**Penilaian Detail:**
- [ ] MQTT subscribe ke semua sensor topics (2 poin)
- [ ] WebSocket server untuk dashboard (2 poin)
- [ ] HTTP server untuk static files (2 poin)
- [ ] Serial communication ke STM32 (2 poin)
- [ ] Data forwarding (MQTT↔WS↔Serial) (2 poin)

### A4. Web Dashboard (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **Real-time Updates** | Instant update, smooth animation | Update < 2 detik | Update < 5 detik | Update lambat | Tidak real-time |
| **Data Visualization** | Charts, gauges, history | Charts basic | Nilai numerik saja | Tampilan minimal | Tidak ada visualisasi |
| **Control Interface** | Multi-device control, feedback | Single control | Basic button | Tidak responsif | Tidak ada control |

**Penilaian Detail:**
- [ ] Sensor data display real-time (2 poin)
- [ ] Device status indicators (2 poin)
- [ ] Control buttons (LED, buzzer) (2 poin)
- [ ] Alert notifications (2 poin)
- [ ] Responsive design (2 poin)

---

## B. Kualitas Kode (25 poin)

### B1. Struktur & Organisasi (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **Modularitas** | Functions terpisah, reusable | Sebagian modular | Function besar | Monolithic | Tidak terstruktur |
| **Naming Convention** | Konsisten, deskriptif | Sebagian konsisten | Campuran | Tidak jelas | Random naming |
| **File Organization** | Folder terstruktur, separation of concerns | Organized | Basic structure | Minimal organization | Berantakan |

**Checklist:**
- [ ] Setiap platform memiliki folder sendiri (2 poin)
- [ ] platformio.ini configured dengan benar (2 poin)
- [ ] Functions dengan single responsibility (2 poin)
- [ ] Constants dan configurations terpisah (2 poin)
- [ ] Konsisten coding style (2 poin)

### B2. Error Handling (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **Network Errors** | Retry logic, exponential backoff | Basic retry | Timeout handling | Crash on error | No handling |
| **Data Validation** | JSON validation, range check | Basic validation | Null check | Partial validation | No validation |
| **Recovery** | Auto-recovery, graceful degradation | Manual recovery | Restart needed | Stuck state | System crash |

**Checklist:**
- [ ] WiFi reconnection dengan timeout (2 poin)
- [ ] MQTT reconnection dengan backoff (2 poin)
- [ ] JSON parsing error handling (2 poin)
- [ ] Sensor read validation (2 poin)
- [ ] Serial communication error handling (2 poin)

### B3. Dokumentasi Kode (5 poin)

| Kriteria | Excellent (5) | Good (4) | Satisfactory (3) | Needs Work (2) | Poor (0-1) |
|----------|---------------|----------|------------------|----------------|------------|
| **Comments** | Setiap function documented | Function utama | Beberapa comments | Minimal | Tidak ada |
| **README** | Setup lengkap, wiring, usage | Basic instructions | Minimal info | Tidak lengkap | Tidak ada |

**Checklist:**
- [ ] Header comments di setiap file (1 poin)
- [ ] Function documentation (1 poin)
- [ ] Inline comments untuk logic kompleks (1 poin)
- [ ] README dengan setup instructions (1 poin)
- [ ] Wiring diagram atau deskripsi (1 poin)

---

## C. Integrasi Sistem (20 poin)

### C1. Multi-Platform Integration (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **ESP32 Nodes** | Multiple nodes, unique ID, coordinated | 2 nodes working | 1 node | Nodes conflict | Tidak berfungsi |
| **STM32 Integration** | Full bidirectional, data processing | Receive only | Basic serial | Intermittent | Tidak connect |
| **Gateway Function** | Complete bridge, all protocols | Most protocols | Basic bridging | Partial | Tidak bridge |

**Checklist:**
- [ ] Minimal 2 ESP32 sensor nodes aktif (3 poin)
- [ ] STM32 menerima dan memproses data (3 poin)
- [ ] Gateway meneruskan ke semua endpoint (2 poin)
- [ ] Data consistency across platforms (2 poin)

### C2. Multi-Protocol Communication (10 poin)

| Kriteria | Excellent (10) | Good (8) | Satisfactory (6) | Needs Work (4) | Poor (0-2) |
|----------|---------------|----------|------------------|----------------|------------|
| **MQTT** | Pub/Sub, QoS, retain, wildcard | Basic pub/sub | Publish only | Unreliable | Tidak berfungsi |
| **WebSocket** | Real-time, bidirectional, multi-client | Single direction | Basic connection | Disconnect sering | Tidak berfungsi |
| **Serial** | Reliable, framing, error detection | Basic serial | Simple forwarding | Data loss | Tidak berfungsi |

**Checklist:**
- [ ] MQTT publish dari semua nodes (2 poin)
- [ ] MQTT subscribe untuk control (2 poin)
- [ ] WebSocket real-time ke browser (2 poin)
- [ ] Serial bridge ESP32↔STM32 (2 poin)
- [ ] Protocol translation benar (2 poin)

---

## D. Fitur Tambahan (15 poin)

### D1. Alert System (5 poin)

| Kriteria | Excellent (5) | Good (4) | Satisfactory (3) | Needs Work (2) | Poor (0-1) |
|----------|---------------|----------|------------------|----------------|------------|
| **Implementation** | Multi-level, visual+audio, configurable | Single threshold | Basic alert | Delayed | Tidak ada |

**Checklist:**
- [ ] Temperature threshold alert (2 poin)
- [ ] Visual notification (LED/dashboard) (2 poin)
- [ ] Audio notification (buzzer) (1 poin)

### D2. Data Persistence/History (5 poin)

| Kriteria | Excellent (5) | Good (4) | Satisfactory (3) | Needs Work (2) | Poor (0-1) |
|----------|---------------|----------|------------------|----------------|------------|
| **Implementation** | Historical chart, export, analytics | Basic chart | In-memory history | Minimal | Tidak ada |

**Checklist:**
- [ ] Data history dalam memory (2 poin)
- [ ] Historical chart di dashboard (2 poin)
- [ ] Analytics summary (1 poin)

### D3. User Interface (5 poin)

| Kriteria | Excellent (5) | Good (4) | Satisfactory (3) | Needs Work (2) | Poor (0-1) |
|----------|---------------|----------|------------------|----------------|------------|
| **Design** | Professional, responsive, intuitive | Good design | Functional | Basic | Minimal |

**Checklist:**
- [ ] Responsive layout (2 poin)
- [ ] Clear data presentation (2 poin)
- [ ] Intuitive controls (1 poin)

---

## E. Dokumentasi & Presentasi (Bonus 10 poin)

### E1. Video Demo (5 poin bonus)

| Kriteria | Excellent (5) | Good (4) | Satisfactory (3) | Needs Work (2) | Poor (0-1) |
|----------|---------------|----------|------------------|----------------|------------|
| **Quality** | Clear explanation, full demo, professional | Good demo | Basic walkthrough | Incomplete | Poor quality |

### E2. Technical Documentation (5 poin bonus)

| Kriteria | Excellent (5) | Good (4) | Satisfactory (3) | Needs Work (2) | Poor (0-1) |
|----------|---------------|----------|------------------|----------------|------------|
| **Completeness** | Architecture, API, setup, troubleshooting | Most sections | Basic docs | Incomplete | Minimal |

---

## Ringkasan Penilaian

| Kategori | Bobot | Nilai |
|----------|-------|-------|
| A. Fungsionalitas Sistem | 40 | /40 |
| B. Kualitas Kode | 25 | /25 |
| C. Integrasi Sistem | 20 | /20 |
| D. Fitur Tambahan | 15 | /15 |
| **Total** | **100** | **/100** |
| E. Bonus | +10 | /10 |
| **Grand Total** | **110** | **/110** |

---

## Konversi Nilai

| Range | Grade | Keterangan |
|-------|-------|------------|
| 90-100 | A | Excellent |
| 80-89 | B+ | Very Good |
| 70-79 | B | Good |
| 60-69 | C+ | Satisfactory |
| 50-59 | C | Adequate |
| < 50 | D/E | Needs Improvement |

---

## Catatan Penilai

### Kelebihan:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

### Area Perbaikan:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

### Komentar Tambahan:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

**Tanggal Penilaian:** _______________

**Penilai:** _______________

**Tanda Tangan:** _______________
