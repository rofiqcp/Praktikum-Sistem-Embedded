# Rubrik Penilaian Project — Modul 12: FreeRTOS Memory Management

## Skala Penilaian: 0-100

---

### A. Fungsionalitas Sistem (30%)

| Kriteria | Excellent (25-30) | Good (18-24) | Fair (10-17) | Poor (0-9) |
|----------|-------------------|--------------|--------------|------------|
| Heap Monitoring | Real-time monitoring dengan trend analysis, alert otomatis | Monitoring dengan reporting periodik | Hanya print free heap | Tidak ada monitoring |
| Stack Monitoring | HWM semua task, warning threshold | HWM beberapa task | Hanya 1 task | Tidak ada |
| Task Management | Task list + runtime stats + CPU% | Task list + state | Hanya nama task | Tidak ada |
| Anomaly Detection | Deteksi leak + fragmentation + overflow | Deteksi 2 dari 3 | Deteksi 1 | Tidak ada |
| Worker Tasks | ≥5 task dengan fungsi berbeda | 4 task | 3 task | <3 task |
| LED Indicator | Multi-color status (OK/warn/critical) | Heartbeat + 1 status | Heartbeat saja | Tidak ada |

### B. Kualitas Kode (25%)

| Kriteria | Excellent (21-25) | Good (15-20) | Fair (8-14) | Poor (0-7) |
|----------|-------------------|--------------|-------------|------------|
| Struktur | Modular, fungsi terpisah, clean architecture | Cukup modular | Satu file besar | Kode berantakan |
| Comment | Setiap fungsi dan blok penting di-comment | Comment pada fungsi utama | Comment minimal | Tidak ada comment |
| Error Handling | Semua return value dicek, hook function lengkap | Sebagian besar dicek | Cek minimal | Tidak ada error handling |
| Naming | Konsisten, deskriptif (camelCase/snake_case) | Sebagian besar konsisten | Inkonsisten | Nama tidak bermakna |
| Compilable | `pio run` PASS tanpa warning | PASS dengan minor warning | PASS dengan banyak warning | Tidak bisa compile |

### C. Analisis & Laporan (20%)

| Kriteria | Excellent (17-20) | Good (12-16) | Fair (6-11) | Poor (0-5) |
|----------|-------------------|--------------|-------------|------------|
| Arsitektur | Diagram lengkap (block, sequence, state) | Block diagram + penjelasan | Hanya penjelasan teks | Tidak ada |
| Data Analysis | Grafik + statistik + interpretasi | Grafik + penjelasan singkat | Hanya data mentah | Tidak ada data |
| Flowchart | Flowchart setiap komponen utama | Flowchart sistem keseluruhan | Flowchart sederhana | Tidak ada |
| Kesimpulan | Insight mendalam, rekomendasi perbaikan | Kesimpulan logis | Kesimpulan generic | Tidak ada |

### D. Python Script (15%)

| Kriteria | Excellent (13-15) | Good (9-12) | Fair (4-8) | Poor (0-3) |
|----------|-------------------|-------------|------------|------------|
| Serial Parsing | Robust parser, handle errors, regex | Parser bekerja stabil | Parser sederhana | Tidak ada |
| Visualization | Real-time plot + multi-chart + save | Real-time plot 1 chart | Static plot | Tidak ada plot |
| Data Logging | CSV + timestamp + filtering | CSV logging | Print ke terminal | Tidak ada |
| Usability | CLI arguments, help text, error messages | Beberapa options | Hardcoded | Sulit digunakan |

### E. Presentasi / Video Demo (10%)

| Kriteria | Excellent (9-10) | Good (6-8) | Fair (3-5) | Poor (0-2) |
|----------|------------------|------------|------------|------------|
| Demo | Sistem berjalan lancar, semua fitur ditunjukkan | Sebagian besar fitur | Fitur dasar saja | Tidak berjalan |
| Penjelasan | Jelas, teknis, menjawab pertanyaan | Cukup jelas | Kurang detail | Membaca script |
| Durasi | 5-10 menit, pace tepat | Terlalu cepat/lambat sedikit | Terlalu singkat/panjang | <2 atau >15 menit |

---

## Bonus Poin (+5 per item, maks +15)
- Implementasi di kedua platform (ESP32 + STM32)
- Auto-recovery mechanism (restart leaking task)
- Predictive analysis (estimasi waktu heap habis)
- GUI Python dengan tkinter/PyQt

## Penalti
- Kode tidak bisa compile: **-20**
- Copy-paste tanpa modifikasi: **-30**
- Tidak ada Python script: **-15**
- Terlambat submit: **-5 per hari**
