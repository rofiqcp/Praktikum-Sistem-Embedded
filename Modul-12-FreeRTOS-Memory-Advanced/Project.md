# Project Modul 12: Sistem Monitor Kesehatan Embedded Real-Time

## 🎯 Deskripsi Project

Rancang dan implementasikan sebuah **Sistem Monitor Kesehatan Embedded Real-Time** yang mampu memantau, menganalisis, dan melaporkan status kesehatan sistem FreeRTOS secara komprehensif. Sistem ini menggabungkan semua konsep yang dipelajari di Modul 09-12: task management, queue, semaphore, timer, notification, memory management, dan advanced FreeRTOS features.

## 📋 Latar Belakang

Dalam sistem embedded produksi, kemampuan untuk memantau kesehatan sistem (*system health monitoring*) sangat penting. Pesawat terbang, perangkat medis, dan sistem otomotif memerlukan self-diagnostic yang berjalan secara kontinu di background. Sistem ini harus mendeteksi anomali seperti memory leak, stack overflow, task starvation, dan resource contention sebelum menyebabkan kegagalan fatal.

## 🏗️ Arsitektur Sistem

```
┌─────────────────────────────────────────────────┐
│              SYSTEM HEALTH MONITOR              │
│                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐     │
│  │ Worker   │  │ Worker   │  │ Sensor   │     │
│  │ Task 1   │  │ Task 2   │  │ Task     │     │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘     │
│       │              │              │            │
│       ▼              ▼              ▼            │
│  ┌─────────────────────────────────────────┐    │
│  │         Data Collection Queue           │    │
│  └────────────────────┬────────────────────┘    │
│                       │                          │
│                       ▼                          │
│  ┌─────────────────────────────────────────┐    │
│  │       Health Monitor Task               │    │
│  │  - Heap usage tracking                  │    │
│  │  - Stack watermark monitoring           │    │
│  │  - Task runtime statistics              │    │
│  │  - Anomaly detection                    │    │
│  └────────────────────┬────────────────────┘    │
│                       │                          │
│            ┌──────────┼──────────┐               │
│            ▼          ▼          ▼               │
│       ┌────────┐ ┌────────┐ ┌────────┐          │
│       │ Serial │ │  LED   │ │ Alert  │          │
│       │ Report │ │ Status │ │ System │          │
│       └────────┘ └────────┘ └────────┘          │
└─────────────────────────────────────────────────┘
```

## 📝 Requirements

### Level 1: Basic (Nilai Maksimal: 70)

1. **Heap Monitor**: Laporan free heap, min-ever free setiap 5 detik
2. **Stack Monitor**: Cek stack high water mark semua task
3. **Task List**: Tampilkan nama, state, prioritas semua task
4. **Serial Output**: Format output terstruktur dan mudah dibaca
5. **LED Heartbeat**: LED berkedip menandakan sistem aktif
6. **Minimal 3 worker tasks** yang melakukan operasi berbeda

### Level 2: Intermediate (Nilai Maksimal: 85)

Semua requirement Level 1, ditambah:

7. **Runtime Statistics**: Persentase CPU usage per task
8. **Anomaly Detection**: Deteksi otomatis jika heap terus berkurang (leak)
9. **Memory Pool**: Gunakan memory pool untuk alokasi internal (bukan heap)
10. **Alert System**: LED indikator warna berbeda untuk status (hijau=OK, kuning=warning, merah=critical)
11. **Python Dashboard**: Script Python yang menampilkan data secara real-time

### Level 3: Advanced (Nilai Maksimal: 100)

Semua requirement Level 2, ditambah:

12. **Predictive Analysis**: Estimasi waktu sampai heap habis berdasarkan trend
13. **Auto-Recovery**: Jika terdeteksi leak, restart task yang bermasalah
14. **Log to Flash**: Simpan log anomali ke NVS (ESP32) atau Flash (STM32)
15. **Remote Monitoring**: Kirim data kesehatan via UART ke PC dengan protokol custom
16. **Comprehensive Python GUI**: Dashboard grafis dengan grafik real-time

## 🔧 Spesifikasi Teknis

### Platform Target
- **ESP32**: esp32dev, Framework ESP-IDF
- **STM32**: bluepill_f103c8, Framework STM32Cube HAL

### FreeRTOS Features yang HARUS Digunakan
- Minimal 5 tasks (termasuk monitor)
- Minimal 2 queues
- Minimal 1 mutex atau semaphore
- Minimal 1 software timer
- Task notification untuk event signaling
- Critical section untuk proteksi data

### Format Output Serial
```
[SYS] timestamp=12345 uptime=00:12:05
[HEAP] free=12048 min_ever=11200 largest=8192 frag_idx=0.15
[TASK] name=Worker1 state=B pri=2 stack_hwm=230 cpu=5.2%
[TASK] name=Worker2 state=R pri=2 stack_hwm=225 cpu=8.1%
[TASK] name=Monitor state=X pri=3 stack_hwm=180 cpu=15.3%
[ALERT] type=WARNING msg="Heap decreased 512 bytes in 30s"
```

## 📊 Rubrik Penilaian

| Aspek | Bobot | Kriteria |
|-------|-------|----------|
| Fungsionalitas | 30% | Semua fitur berjalan sesuai requirement level |
| Kode Program | 25% | Clean code, modular, well-commented, compilable |
| Analisis & Laporan | 20% | Penjelasan arsitektur, flowchart, hasil pengujian |
| Python Script | 15% | Parsing, plotting, logging, real-time display |
| Presentasi/Video | 10% | Demo sistem berjalan, penjelasan konsep |

## 📅 Deliverables

1. **Source Code** — Kode C lengkap (`src/main.c`) + Python script
2. **Dokumentasi** — README.md berisi cara compile, upload, dan penggunaan
3. **Laporan** — Penjelasan arsitektur, diagram, dan hasil analisis
4. **Video Demo** — 5-10 menit menunjukkan sistem berjalan dan penjelasan
5. **Data Log** — CSV file dari Python script minimal 5 menit monitoring

## ⏰ Timeline

| Minggu | Aktivitas |
|--------|-----------|
| 1 | Desain arsitektur, buat skeleton code |
| 2 | Implementasi core features (Level 1) |
| 3 | Tambah advanced features + Python script |
| 4 | Testing, debugging, dokumentasi, video |

## 💡 Tips

- Mulai dari Level 1, pastikan stabil, baru naik level
- Gunakan `configGENERATE_RUN_TIME_STATS = 1` untuk CPU usage
- Jangan gunakan `printf()` terlalu sering — bisa mengganggu timing
- Test dengan beban berbeda: idle vs heavy load
- Gunakan Python `matplotlib.animation` untuk real-time plot
- Commit ke Git secara berkala

## 📖 Referensi Tambahan

- FreeRTOS API Reference: https://www.freertos.org/a00106.html
- ESP-IDF System API: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/
- STM32Cube HAL Documentation: https://www.st.com/resource/en/user_manual/
